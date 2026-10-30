import unittest
from collections import Counter

from hull_resources import EXTENT, lua_chunks, records
from test_paradise import ParadiseTests


class HullResourceTests(unittest.TestCase):
    def scene(self):
        lua, wrapper = ParadiseTests().scene()
        for chunk in lua_chunks():
            lua.execute(chunk)
        lua.execute('''function source_hull(c)
            local h=hull(c[4])
            h.GetTriCount=function() return c[1] end
            h.GetPlaneCount=function() return c[2] end
            h.GetEdgeCount=function() return c[3] end
            return h
        end''')
        return lua, wrapper

    def test_sources_and_centered_nonzero_bounds(self):
        rows = records()
        self.assertEqual(Counter(r['level'] for r in rows.values()),
                         {'Canyon': 2, 'Desert': 1, 'Mountain': 26, 'Barrens': 17})
        for key, row in rows.items():
            expected = {'02bc2db6a75a57ad340108bb639c0e62': [20, 10, 20, 12],
                        '30bab2f719f1a4ffa32f3e393cb6ad0d': [20, 8, 18, 12]}.get(key, [12, 6, 12, 8])
            self.assertEqual(row['counts'], expected)
            widths = []
            for axis in range(3):
                values = row['vertices'][axis::3]
                self.assertAlmostEqual(min(values)+max(values), 0)
                self.assertGreater(max(values)-min(values), 0)
                widths.append(max(values)-min(values))
            self.assertAlmostEqual(max(widths), EXTENT)

    def test_targets_duplicates_reload_and_unlisted_hull(self):
        lua, wrapper = self.scene()
        g = lua.globals()
        for _ in range(2):
            barn = lua.table()
            for key, row in records().items():
                counts = lua.table_from(row['counts'])
                barn[key] = lua.table_from([g.source_hull(counts), g.source_hull(counts)])
            untouched = g.hull(8)
            barn['unlisted'] = lua.table_from([untouched])
            before = g.writes
            lua.execute(wrapper+' __JNETApplyScenery()')
            g.PutHullsInNamesTable(barn)
            self.assertEqual(g.writes-before, 2*sum(r['counts'][3] for r in records().values()))
            self.assertIsNone(untouched[0])
            for key, row in records().items():
                for h in barn[key].values():
                    for i, expected in enumerate(row['vertices']):
                        self.assertAlmostEqual(h[i//3][i % 3+1], expected, places=12)

    def test_restored_barrens_hulls_keep_loaded_geometry(self):
        restored = '''837ac3b5a756ef40142b3640eddbb49c
8e1c805362609f4d9ad0e8077890ce83
e83702f72f06bbcd267886bd51822293
d55fe56c22b2c81488a2c87417a33b26
9cc5612c8685c78233a7dfb5a99db471
bd3bb98909f955a376d5622c4021e6c2
9dcd85503a283d2b0f7d0be4ab4596c3
b95f1929dc0d76209eb29e89648913c8'''.splitlines()
        lua, _ = self.scene()
        g = lua.globals()
        barn = lua.table()
        for key in restored:
            self.assertNotIn(key, records())
            h = g.hull(8)
            h[0] = lua.table_from([123, 456, 789])
            barn[key] = lua.table_from([h])
        g.PutHullsInNamesTable(barn)
        self.assertEqual((g.resets, g.writes), (0, 0))
        for group in barn.values():
            self.assertEqual(list(group[1][0].values()), [123, 456, 789])

    def test_topology_mismatch_leaves_hull_untouched(self):
        lua, _ = self.scene()
        g = lua.globals()
        key = next(iter(records()))
        bad = g.hull(7)
        g.PutHullsInNamesTable(lua.table_from({key: lua.table_from([bad])}))
        self.assertEqual((g.resets, g.writes), (0, 0))

    def test_native_userdata_shaped_name_entry(self):
        lua, _ = self.scene()
        lua.execute('''
            local native_type=type
            function type(x)
                if native_type(x)=='table' and x.native_hull then return 'userdata' end
                return native_type(x)
            end
        ''')
        g = lua.globals()
        h = g.hull(8)
        h.native_hull = True
        g.PutHullsInNamesTable(lua.table_from({next(iter(records())): h}))
        self.assertEqual((g.resets, g.writes), (1, 8))


if __name__ == '__main__':
    unittest.main()
