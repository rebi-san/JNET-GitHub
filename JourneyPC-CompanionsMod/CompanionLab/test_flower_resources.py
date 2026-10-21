import tempfile
import unittest
from pathlib import Path

import flower_resources as fr
from character_resources import LiteralReader

AVAILABLE = (fr.FLOWER/'Data/Scripts/FlowerDefs.lua').exists()


@unittest.skipUnless(AVAILABLE, 'FlowerPC data is not present')
class FlowerResourcesTest(unittest.TestCase):
    def test_every_flower_type_and_mesh_is_read(self):
        found = fr.flowers()
        self.assertEqual([f['flowerName'] for f in found], ['Yellow', 'Red', 'White', 'Blue', 'Pink', 'Purple', 'Special', 'Wheat'])
        self.assertEqual([f['index'] for f in found], list(range(8)))
        table = fr.meshes()
        self.assertEqual(len(table), 15)
        for flower in found:
            for layer in flower['petalLayers']:
                self.assertIn(layer['meshName'], table)

    def test_repeated_keys_follow_lua_only_for_flower_data(self):
        self.assertEqual(fr.FlowerReader('T={a=1,a=2}').assignment('T'), {'a': 2})
        with self.assertRaises(ValueError):
            LiteralReader('T={a=1,a=2}').assignment('T')

    def test_atlas_cells_and_colours_are_valid(self):
        grid = fr.atlas()
        for flower in fr.flowers():
            for layer in flower['petalLayers']:
                self.assertIn(layer.get('texture') or 'Default', grid)
                self.assertTrue(all(0 <= c <= 255 for c in layer['color0'] + layer['color1']))
        _, _, _, uv = fr.layer_geometry(fr.flowers()[0]['petalLayers'][0], fr.meshes()['PetalYellow'], grid)[0][0]
        self.assertTrue(0 <= uv[0] <= 1 and 0 <= uv[1] <= 1)

    def test_assembly_replicates_petals_and_stays_indexed(self):
        table, grid = fr.meshes(), fr.atlas()
        for flower in fr.flowers():
            for layer in flower['petalLayers']:
                mesh = table[layer['meshName']]
                vertices, triangles = fr.layer_geometry(layer, mesh, grid)
                count = len(next(iter(mesh['frames'].values())))
                self.assertEqual(len(vertices), count*layer['numPetals'])
                self.assertEqual(len(triangles)*3, len(mesh['indices'])*layer['numPetals'])
                # Purple deliberately lists a PetalPurple layer with zero petals.
                self.assertTrue(triangles or layer['numPetals'] == 0)
                if triangles: self.assertLess(max(max(t) for t in triangles), len(vertices))

    def test_export_writes_objs_textures_shaders_and_manifest(self):
        with tempfile.TemporaryDirectory() as folder:
            out = fr.export(folder)
            self.assertEqual(len(list((out/'flowers').glob('*.obj'))), 8)
            self.assertTrue((out/'textures/petalAtlas.dds').stat().st_size > 1000000)
            self.assertTrue(any(p.name == 'PetalInstance_vs.cso' for p in (out/'shaders').iterdir()))
            text = (out/'flowers/Red.obj').read_text()
            vertices = sum(1 for l in text.splitlines() if l.startswith('v '))
            faces = [l for l in text.splitlines() if l.startswith('f ')]
            self.assertTrue(faces)
            self.assertLessEqual(max(int(i.split('/')[0]) for l in faces for i in l.split()[1:]), vertices)

    def test_runtime_header_models_are_self_consistent(self):
        import re
        header = fr.runtime_header()
        vertex_total = int(re.search(r'jnet_flower_vertices\[(\d+)\]', header)[1])
        index_total = int(re.search(r'jnet_flower_indices\[(\d+)\]', header)[1])
        indices = [int(i) for i in re.search(r'jnet_flower_indices\[\d+\]=\{([^}]*)\}', header)[1].split(',')]
        models = [tuple(map(int, m[:4])) for m in re.findall(r'\{(\d+),(\d+),(\d+),(\d+),"(\w+)"\}', header)]
        self.assertEqual(len(models), 8)
        self.assertEqual(len(indices), index_total)
        first_vertex = first_index = 0
        for first_v, count_v, first_i, count_i in models:
            self.assertEqual((first_v, first_i), (first_vertex, first_index))
            self.assertEqual(count_i % 3, 0)
            # Indices are local to the model; the draw adds first_vertex as the base vertex.
            self.assertLess(max(indices[first_i:first_i + count_i]), count_v)
            first_vertex += count_v
            first_index += count_i
        self.assertEqual((first_vertex, first_index), (vertex_total, index_total))
        self.assertIn('JNET_FLOWER_MODEL_COUNT 8', header)

    def test_every_flower_model_stands_on_the_floor_with_a_stem(self):
        table, grid = fr.meshes(), fr.atlas()
        for flower in fr.flowers():
            vertices, triangles = fr.flower_model(flower, table, grid)
            ys = [v[0][1] for v in vertices]
            self.assertAlmostEqual(min(ys[:2*fr.STEM_SIDES]), 0)
            self.assertGreater(max(ys), fr.STEM_HEIGHT*fr.WORLD_UNIT*0.9)
            self.assertTrue(all(0 <= c <= 1 for v in vertices for c in v[2]))

    def test_atlas_file_matches_what_the_dll_accepts(self):
        import struct
        data = (fr.FLOWER/'Data/Textures'/fr.ATLAS).read_bytes()
        flags, height, width = struct.unpack_from('<III', data, 8)
        self.assertEqual((data[:4], height, width, struct.unpack_from('<I', data, 28)[0]), (b'DDS ', 2048, 1024, 12))
        self.assertTrue(flags & 0x20000)
        self.assertEqual(struct.unpack_from('<IIIIIII', data, 76), (32, 0x41, 0, 32, 0xff0000, 0xff00, 0xff))
        total = 128 + sum(max(1, 1024 >> n)*max(1, 2048 >> n)*4 for n in range(12))
        self.assertEqual(len(data), total)

    def test_models_carry_closed_pose_kinds_and_grass(self):
        table, grid = fr.meshes(), fr.atlas()
        for flower in fr.flowers():
            vertices, triangles = fr.flower_model(flower, table, grid)
            kinds = [v[4] for v in vertices]
            self.assertEqual({fr.KIND_STEM, fr.KIND_PETAL, fr.KIND_GRASS}, set(kinds))
            # Stem first, then petals, then grass; stems and grass do not morph, petals do.
            self.assertEqual(kinds, sorted(kinds, key=[fr.KIND_STEM, fr.KIND_PETAL, fr.KIND_GRASS].index))
            for position, normal, colour, uv, kind, closed_position, closed_colour in vertices:
                if kind != fr.KIND_PETAL:
                    self.assertEqual((position, colour), (closed_position, closed_colour))
            # Wheat's morph frames are identical, so it has no closed pose of its own.
            if flower['flowerName'] != 'Wheat':
                self.assertTrue(any(v[0] != v[5] for v in vertices if v[4] == fr.KIND_PETAL), flower['flowerName'])
            self.assertEqual(sum(k == fr.KIND_GRASS for k in kinds), 4*fr.GRASS_CARDS)

    def test_grass_cards_use_the_clump_sheet_and_stand_on_the_ground(self):
        vertices, triangles = fr.grass_geometry()
        self.assertEqual((len(vertices), len(triangles)), (4*fr.GRASS_CARDS, 2*fr.GRASS_CARDS))
        self.assertTrue(all(0 <= v[3][0] <= 1 and 0 <= v[3][1] <= 1 for v in vertices))
        self.assertEqual(min(v[0][1] for v in vertices), 0)
        self.assertTrue(all(v[2][1] > v[2][0] for v in vertices))

    def test_closed_pose_uses_unbloom_values_and_first_frame(self):
        table = fr.meshes()
        layer = next(l for f in fr.flowers() for l in f['petalLayers'] if 'scaleUnBloom' in l and 'color0UnBloom' in l
                     and fr.triple(l['scaleUnBloom'], (1, 1, 1)) != fr.triple(l.get('scale'), (1, 1, 1)))
        closed, bloom = fr.layer_pose(layer, True), fr.layer_pose(layer, False)
        self.assertNotEqual(closed['scale'], bloom['scale'])
        self.assertEqual(closed['base'], tuple(c/255 for c in layer['color0UnBloom']))
        self.assertEqual(bloom['base'], tuple(c/255 for c in layer['color0']))
        mesh = table[layer['meshName']]
        self.assertEqual(fr.closed_frame(mesh), next(iter(mesh['frames'])))
        self.assertEqual(fr.bloom_frame(mesh), list(mesh['frames'])[-1])

    def test_grass_texture_file_is_a_valid_mipped_bgra_dds(self):
        import struct
        data = fr.grass_dds()
        flags, height, width, pitch = struct.unpack_from('<IIII', data, 8)
        mips = struct.unpack_from('<I', data, 28)[0]
        self.assertEqual((data[:4], height, width, mips), (b'DDS ', 256, 256, 9))
        self.assertEqual(struct.unpack_from('<IIIIIII', data, 76), (32, 0x41, 0, 32, 0xff0000, 0xff00, 0xff))
        self.assertEqual(len(data), 128 + sum(max(1, 256 >> n)**2*4 for n in range(9)))
        self.assertTrue(any(data[128 + i] for i in range(3, 4*256*256, 4)), 'the sheet has opaque blades')
        self.assertTrue(any(not data[128 + i] for i in range(3, 4*256*256, 4)), 'the sheet has empty space')

    def test_header_vertex_layout_matches_the_shader_input(self):
        header = fr.runtime_header()
        self.assertIn('float position[3],normal[3],uv[2]; unsigned char color[4]; float closed[3]; unsigned char closed_color[4];', header)
        self.assertIn('JNET_FLOWER_GRASS_FILE L"JNET-grass.dds"', header)


if __name__ == '__main__':
    unittest.main()
