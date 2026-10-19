import re
import ast
from lupa import LuaRuntime
import struct
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from control_resources import menu_bytes
from inspect_build import ROOT, LAB, pe_sections
from native_map import offset_for


def decode(data):
    key=0x6341f337
    plain=bytearray()
    for (value,) in struct.iter_unpack('<I',data):
        plain+=struct.pack('<I',value^key)
        key=(key+value)&0xffffffff
    return ET.fromstring(plain.rstrip(b'\0'))


class ControlsTests(unittest.TestCase):
    def test_native_widgets_and_unrelated_rows(self):
        original=decode((ROOT/'JourneyPC/MenuSystem.xme').read_bytes())
        edited=decode(menu_bytes())
        controls=edited.find('./Menu/Options/Controls')
        for branch,device,count in [('Keyboard',0,256),('Mouse',1,6),('Pad/DualShock',2,17),('Pad/Input',2,17)]:
            rows=list(controls.find(branch))
            at=next(i for i,row in enumerate(rows) if row.tag=='Fly')
            self.assertEqual([row.get('Text') for row in rows[at+1:at+8]],['Crouch','Frontflip','Backflip','SideflipL','SideflipR','Spin','Spotlight'])
            for i,row in enumerate(rows[at+1:at+8]):
                self.assertEqual(len(row.get('Options').split(';')),count)
                self.assertEqual(row.get('Setting'),f'%JNET_BIND_{device}_{[0,4,1,5,6,2,3][i]}%')
                controls.find(branch).remove(row)
            controls.find(branch).find('Chime').set('Text','$Strings.CtrlChime')
        menu=edited.find('./Menu')
        tags=[row.tag for row in menu]
        at=tags.index('Options')
        self.assertEqual(tags[at-2:at],['JNETCharacter','JNETWorld'])
        for name in ('JNETCharacter','JNETWorld'):
            self.assertEqual(menu.find(name).get('Action'),'%JNET_OPEN%')
            menu.remove(menu.find(name))
        for branch,before,name,above in [('Audio','Back','JNETCredits','MusicVol'),('Graphics','Default','JNETFPS','Anisotrophy')]:
            parent=menu.find('Options/'+branch)
            tags=[row.tag for row in parent]
            at=tags.index(name)
            self.assertEqual(tags[at-1:at+2],[above,name,before])
            self.assertEqual(parent.find(name).get('Setting'),'%'+name+'%')
            parent.remove(parent.find(name))
        menu.find('Start').set('Text',original.find('./Menu/Start').get('Text'))
        self.assertEqual(ET.tostring(original),ET.tostring(edited))

    def test_native_entry_guards(self):
        source=(LAB/'diagnostic/probe_dll.c').read_text()
        binary=(ROOT/'JourneyPC/Journey.exe').read_bytes()
        sections=pe_sections(binary)
        for name,rva in [('xml',0x2ecf60),('setting',0x1084f0),('action',0xaea60),('dispatch',0x101e30),('push',0x107c70)]:
            values=re.search(r'controls_'+name+r'_entry\[\]=\{([^}]+)\}',source)[1]
            expected=bytes(int(v,16) for v in values.split(','))
            at=offset_for(sections,rva,len(expected))
            self.assertEqual(binary[at:at+len(expected)],expected)

    def test_creature_sound_aliases_exist_and_fit_native_table(self):
        lua=LuaRuntime()
        lua.execute((LAB/'work/pc_scripts/SoundEffectData.lua').read_text())
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        wrapper=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper=''.join(ast.literal_eval(part) for part in re.findall(r'"(?:\\.|[^"\\])*"',wrapper))
        lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()")
        sounds=lua.globals().soundEffectData
        bank=(ROOT/'JourneyPC/Data/Sounds/LvlSfxCave-SFX.bnk').read_bytes()
        for alias,event in [('JNTWarVoice','GuardianCameoBy'),('JNTWarLock','GuardianFinalStretchHungry1'),('JNTWarHungry','GuardianFinalStretchHungry2'),('JNTWarAway','GuardianFlyAway')]:
            self.assertEqual(sounds[alias].soundName,event)
            self.assertEqual(sounds[alias].bankName,'JNTMech')
            self.assertIn(event.encode()+b'\0',bank)
        self.assertEqual(sounds.JNTFishShort.soundName,'fish-idle-move-short')
        self.assertEqual(sounds.JNTFishCall.soundName,'fish-come-here')
        self.assertIsNone(sounds.JNTFlowSnake)
        self.assertIsNone(sounds.JNTFlowHunter)
        self.assertLessEqual(len(list(sounds.items())),160)

    def test_registered_animation_actions(self):
        lua=LuaRuntime()
        lua.execute((LAB/'work/pc_scripts/DudeAnimation.lua').read_text())
        source=(LAB/'work/pc_scripts/DudeAnimation.lua').read_text()
        for name in ('Crouch','CrouchToIdle','SlideJump1','SlideJump2','RollF','RollL','RollR'):
            self.assertRegex(source,rf'\b{name}\s*=\s*\{{\s*isAction\s*=\s*true')

    def test_spin_registration_and_clip(self):
        lua=LuaRuntime()
        lua.execute((LAB/'work/pc_scripts/DudeAnimation.lua').read_text())
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        wrapper=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper=''.join(ast.literal_eval(part) for part in re.findall(r'"(?:\\.|[^"\\])*"',wrapper))
        before=list(lua.globals().GetDudeActionAnims().values())
        self.assertNotIn('SlideJump3',before)
        native_sequences=list(lua.globals().GetDudeAnimSequences().values())
        for name in ('RollL','RollR'):
            self.assertIn(name,before)
            self.assertNotIn(name,native_sequences)
        lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()");lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()")
        after=list(lua.globals().GetDudeActionAnims().values())
        self.assertEqual(after,before+['JNETSpin3','JNETFlipL','JNETFlipR'])
        self.assertLessEqual(len(after),196)
        lua.execute("""
            loaded={} queued={} sequences={}
            barn={AddActionAnimation=function(self)
                return {SetName=function(self,name) loaded[#loaded+1]=name end,
                        isAction=function() end,isAdditive=function() end,
                        isDucker=function() end,isDucked=function() end}
            end,AddAnimSequence=function(self,name) sequences[#sequences+1]=name end,QueueAnimation=function(self,...) queued={...} end}
            LoadDudeActionAnimations(barn)
            LoadDudeAnimSequences(barn)
            QueueAnimationSequence(barn,'JNETSpin3')
        """)
        self.assertEqual(list(lua.globals().loaded.values()),after)
        self.assertIn('JNETSpin3',list(lua.globals().sequences.values()))
        from character_resources import character_resource_script, wayfarer_resource_script, character_animation_files, character_animation_file_header
        binary=(ROOT/'JourneyPC/Journey.exe').read_bytes()
        from startup_resources import extract_persistent, prepare_candidate
        native, _ = prepare_candidate(extract_persistent(binary), active_traveler=True)
        startup=wayfarer_resource_script()+native+character_resource_script(binary)
        self.assertEqual(startup.count(b'resource "MeshAnimation" "JNETSpin3"'),1)
        self.assertLess(startup.index(b'resource "MeshAnimation" "JNETSpin3"'),
                        startup.index(b'resource "Mesh" "BlendNinja"'))
        self.assertIn(b'resource "MeshAnimation" "JNETSpin3" { source="C_ClothNinja_A_SlideJump3.PSSG", meshName="BlendNinja" }',startup)
        files=character_animation_files(binary)
        self.assertIn('JNETSpin3.D3D11x64',files)
        self.assertIn('"JNETSpin3","JNETSpin3.D3D11x64"',character_animation_file_header(files))
        self.assertIn(b'JNET/Meshes/C_ClothNinja_A_SlideJump3.dae#',files['JNETSpin3.D3D11x64'])
        self.assertEqual(lua.globals().queued[1],'JNETSpin3')
        for name,clip in (('JNETFlipL','RollL'),('JNETFlipR','RollR')):
            self.assertIn(name,list(lua.globals().sequences.values()))
            self.assertEqual(list(lua.globals().GetDudeAnimSequences().values()).count(name),1)
            declaration=('resource "MeshAnimation" "'+name+'"').encode()
            self.assertEqual(startup.count(declaration),1)
            self.assertLess(startup.index(declaration),startup.index(b'resource "Mesh" "BlendNinja"'))
            self.assertIn(('JNET/Meshes/C_ClothNinja_A_'+clip+'.dae#').encode(),files[name+'.D3D11x64'])
            lua.execute("QueueAnimationSequence(barn,'"+name+"')")
            self.assertEqual(list(lua.globals().queued.values()),[name,0.2,0.3,2,1,0])
        self.assertLessEqual(len(list(lua.globals().sequences.values())),196)
        lua.execute("QueueAnimationSequence(barn,'SlideJump1')")
        self.assertEqual(lua.globals().queued[1],'SlideJump1')
        for name in ('SlideJump1','SlideJump3'):
            self.assertTrue((ROOT/f'JourneyPC/Data/Meshes/C_ClothNinja_A_{name}.D3D11x64').is_file())

    def test_setup_dispatcher_caches_each_state_and_updates_flags(self):
        from lupa.lua53 import LuaRuntime as Lua53
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        block=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        chunks=[''.join(ast.literal_eval(v) for v in re.findall(r'"(?:\\.|[^"\\])*"',part))+' end ' for part in block.split('" end ",')[:-1]]
        block=source.split('snprintf(code,sizeof(code),')[1].split('jnet_credits?')[0]
        dispatcher=''.join(ast.literal_eval(v) for v in re.findall(r'"(?:\\.|[^"\\])*"',block))
        for _ in range(2):
            lua=Lua53();prepared=[]
            def prepare():
                prepared.append(True)
                for chunk in chunks:lua.execute(chunk)
            for i in range(100):
                lua.execute(dispatcher % ('true' if i%2 else 'false','false'),prepare)
                self.assertEqual(lua.globals()['__JNETCreditsEnabled'],bool(i%2))
            self.assertEqual(len(prepared),1)
            lua.execute((LAB/'work/pc_scripts/DudeAnimation.lua').read_text())
            lua.execute(dispatcher % ('true','true'),prepare)
            self.assertEqual(list(lua.globals().GetDudeActionAnims().values()).count('JNETSpin3'),1)
            self.assertTrue(lua.globals()['__JNETParadiseCutscene'])
            self.assertEqual(len(prepared),1)

    def test_setup_cache_lua53_pool_pressure(self):
        import ctypes as c
        import lupa.lua53
        lib=c.CDLL(lupa.lua53.__file__)
        heap=c.CDLL(None)
        heap.realloc.argtypes=[c.c_void_p,c.c_size_t];heap.realloc.restype=c.c_void_p
        heap.free.argtypes=[c.c_void_p]
        allocator=c.CFUNCTYPE(c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_size_t)
        lib.lua_newstate.argtypes=[allocator,c.c_void_p];lib.lua_newstate.restype=c.c_void_p
        lib.luaL_openlibs.argtypes=[c.c_void_p]
        lib.luaL_loadbufferx.argtypes=[c.c_void_p,c.c_char_p,c.c_size_t,c.c_char_p,c.c_char_p]
        lib.lua_pcallk.argtypes=[c.c_void_p,c.c_int,c.c_int,c.c_int,c.c_ssize_t,c.c_void_p]
        lib.lua_settop.argtypes=[c.c_void_p,c.c_int];lib.lua_close.argtypes=[c.c_void_p]
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        parts=source.split('static const char *const chunks[]={')[1].split('    };')[0].split('" end ",')[:-1]
        chunks=[''.join(ast.literal_eval(v) for v in re.findall(r'"(?:\\.|[^"\\])*"',part))+' end ' for part in parts]
        old=''.join(chunk.split('() ',1)[1].rsplit(' end ',1)[0] for chunk in chunks)
        def peak(cached):
            live={};high=0
            @allocator
            def allocate(user,ptr,oldsize,size):
                nonlocal high
                if not size:
                    live.pop(ptr,None);heap.free(ptr);return None
                result=heap.realloc(ptr,size)
                if result:
                    live.pop(ptr,None);live[result]=size
                    high=max(high,sum(4096<n<=8192 for n in live.values()))
                return result
            state=lib.lua_newstate(allocate,None)
            self.assertTrue(state)
            def execute(code):
                data=code.encode()
                self.assertEqual(lib.luaL_loadbufferx(state,data,len(data),b'fixture',b't'),0)
                self.assertEqual(lib.lua_pcallk(state,0,0,0,0,None),0)
                lib.lua_settop(state,0)
            try:
                lib.luaL_openlibs(state)
                execute("collectgarbage('stop')")
                if cached:
                    for chunk in chunks:execute(chunk)
                for _ in range(100):
                    execute('__JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()' if cached else old)
                return high
            finally:lib.lua_close(state)
        binary=(ROOT/'JourneyPC/Journey.exe').read_bytes();sections=pe_sections(binary)
        self.assertEqual(binary[offset_for(sections,0xf0967)],0xcc)
        table=offset_for(sections,0x5c38f0)
        self.assertIn((16,8192),[struct.unpack_from('<II',binary,table+i*8) for i in range(15)])
        previous,current=peak(False),peak(True)
        self.assertGreaterEqual(previous,16)
        self.assertLess(current,16)
        print(f'Lua 5.3 setup 8192-byte bucket peak: repeated compilation {previous}, cached {current}')

    def test_settings_apply_revert(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        functions=source[source.index('static unsigned jnet_saved_credits'):source.index('static uintptr_t jnet_menu_xml')]
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary)
            (folder/'settings_fixture.h').write_text(functions)
            exe=folder/'settings'
            subprocess.run(['clang','-std=c11','-fsanitize=address,undefined','-I'+str(folder),str(LAB/'diagnostic/test_jnet_settings.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_action_queue_and_button_edges(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        functions=source[source.index('static int jnet_controls_action('):source.rindex('#endif')]
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary)
            (folder/'controls_fixture.h').write_text(functions)
            exe=folder/'controls'
            subprocess.run(['clang','-std=c11','-fsanitize=address,undefined','-I'+str(folder),str(LAB/'diagnostic/test_jnet_controls.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__=='__main__':
    unittest.main()
