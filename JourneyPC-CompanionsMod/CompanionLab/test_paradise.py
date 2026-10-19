import ast
import re
import struct
import unittest
from pathlib import Path

from lupa import LuaRuntime
from inspect_build import pe_sections
from native_map import offset_for

LAB = Path(__file__).resolve().parent
ROOT = LAB.parent.parent
HULL = 'edd23401f7232e3829cfd2c5695b5967'


class ParadiseTests(unittest.TestCase):
    def scene(self):
        lua = LuaRuntime()
        source = (LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        wrapper = source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper = ''.join(ast.literal_eval(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', wrapper))
        lua.execute('''
            Names={} calls=0 resets=0 writes=0
            function PutHullsInNamesTable(barn) calls=calls+1 Names=barn end
            function hull(count)
                return {GetTriCount=function()return 12 end,GetPlaneCount=function()return 6 end,
                    GetEdgeCount=function()return 12 end,GetVertCount=function()return count end,
                    ResetAabb=function()resets=resets+1 end,
                    SetVert=function(self,i,v) writes=writes+1 self[i]=v end}
            end
        ''')
        lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()")
        return lua, wrapper

    def test_normal_cutscene_and_fresh_normal_load(self):
        lua, wrapper = self.scene()
        g = lua.globals()
        first = g.hull(8)
        barn = lua.table_from({HULL: lua.table_from([first])})
        g['__JNETParadiseCutscene'] = False
        g.PutHullsInNamesTable(barn)
        self.assertEqual(g.writes, 0)
        g['__JNETParadiseCutscene'] = True
        lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()")
        g.PutHullsInNamesTable(barn)
        self.assertEqual((g.calls, g.resets, g.writes), (2, 1, 8))
        for axis, expected in enumerate(((0, 512), (-2.00266, 2000), (-415.817, 1024)), 1):
            values = [first[i][axis] for i in range(8)]
            self.assertEqual((min(values), max(values)), expected)
        g['__JNETParadiseCutscene'] = False
        g.PutHullsInNamesTable(lua.table_from({HULL: lua.table_from([g.hull(8)])}))
        self.assertEqual(g.writes, 8)

    def test_other_hulls_and_invalid_topology_unchanged(self):
        lua, _ = self.scene()
        g = lua.globals()
        g['__JNETParadiseCutscene'] = True
        g.PutHullsInNamesTable(lua.table_from({'other': lua.table_from([g.hull(8)])}))
        g.PutHullsInNamesTable(lua.table_from({HULL: lua.table_from([g.hull(7)])}))
        self.assertEqual((g.resets, g.writes), (0, 0))

    def test_south_face_matches_furthest_pc_hull(self):
        lua=LuaRuntime()
        lua.execute((ROOT/'JourneyIOS/Data/Scripts/Level_Summit/HullInstances.lua').read_text())
        rows=[row for row in lua.globals().HullInstances.values() if hasattr(row,'items')]
        south=min(rows,key=lambda row:min(list(row['Verts'].values())[2::3]))
        self.assertEqual(south['ObjectName'],'2033e9c8a56a0c717801adfa60d49459')
        vertices=list(south['Verts'].values())
        self.assertEqual(min(vertices[2::3]),-415.817)
        packed=b''.join(struct.pack('<4f',*vertices[i:i+3],0) for i in range(0,len(vertices),3))
        self.assertEqual((ROOT/'JourneyPC/Data/Scripts/Level_Summit/HullInstances.lua.bin').read_bytes().count(packed),1)

    def test_source_identity_and_terrain_limits(self):
        lua = LuaRuntime()
        lua.execute((ROOT/'JourneyIOS/Data/Scripts/Level_Summit/HullInstances.lua').read_text())
        hull = next(r for r in lua.globals().HullInstances.values()
                    if hasattr(r, 'items') and r['ObjectName'] == HULL)
        self.assertEqual(hull['GardenerName'], '|Hull31222|Hull31222_G')
        vertices = list(hull['Verts'].values())
        binary = (ROOT/'JourneyPC/Data/Scripts/Level_Summit/HullInstances.lua.bin').read_bytes()
        packed = b''.join(struct.pack('<4f', *vertices[i:i+3], 0) for i in range(0, 24, 3))
        self.assertEqual(binary.count(packed), 1)
        self.assertEqual(binary.count(HULL.encode()), 1)
        executable = (ROOT/'JourneyPC/Journey.exe').read_bytes()
        sections = pe_sections(executable)
        for rva, value in ((0x68b600, 510), (0x68b634, 1022), (0x68b630, 1000)):
            self.assertEqual(struct.unpack_from('<f', executable, offset_for(sections, rva))[0], value)
        lua.execute('Names=setmetatable({},{__index=function(t,k)return k end})')
        lua.execute((LAB/'work/pc_scripts/Level_Summit/TriggerInstances.lua').read_text())
        triggers = {r['ObjectName']: r for r in lua.globals().TriggerInstances.values() if hasattr(r, 'items')}
        event = triggers['beea50eff629664b1d4165d6f870c80b']['Vars']
        self.assertEqual(event['hulls'], HULL)
        self.assertTrue(event['onExit'])
        self.assertEqual(triggers[event['effects']]['Vars']['objects'], '3d7bb9dd12b20c1f67cae7437992edf5')


if __name__ == '__main__':
    unittest.main()
