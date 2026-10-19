import unittest

from inspect_build import ROOT, inside
from startup_resources import extract_persistent, prepare_candidate, SCRIPT_SIZE


class StartupResourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.image = inside(ROOT / 'JourneyPC/Journey.exe').read_bytes()
        cls.source = extract_persistent(cls.image)

    def test_exact_three_changes_and_input_preserved(self):
        original = bytes(self.source)
        candidate, changes = prepare_candidate(self.source)
        self.assertEqual(self.source, original)
        self.assertEqual(len(candidate), SCRIPT_SIZE)
        self.assertEqual([c['buffer_offset'] for c in changes], [0x320, 0x913, 0xb5b])
        self.assertEqual([(i, a, b) for i, (a, b) in enumerate(zip(original, candidate)) if a != b],
                         [(0x320, ord('2'), ord('4')), (0x913, ord('2'), ord('4')), (0xb5b, ord('2'), ord('4'))])

    def test_modified_and_truncated_inputs_rejected(self):
        candidate, _ = prepare_candidate(self.source)
        for data in (b'', self.source[:-1], self.source + b'\0', candidate,
                     b'!' + self.source[1:]):
            with self.subTest(length=len(data)):
                with self.assertRaises(ValueError):
                    prepare_candidate(data)

    def test_active_traveler_adds_only_remote_cloth_capacity(self):
        candidate, changes = prepare_candidate(self.source, active_traveler=True)
        self.assertEqual(len(candidate), SCRIPT_SIZE)
        self.assertEqual([(c['resource'], c['before'], c['after']) for c in changes], [
            ('BlendNinja', 2, 4), ('MeshDudeRemote', 2, 4), ('MeshDudeRemoteFrozen', 2, 4),
            ('ClothRemote', 1, 5), ('ClothRemoteScarf', 1, 5)])
        self.assertEqual([i for i, (a, b) in enumerate(zip(self.source, candidate)) if a != b],
                         [c['buffer_offset'] for c in changes])
        self.assertEqual(extract_persistent(self.image), self.source)
        with self.assertRaises(ValueError):
            prepare_candidate(candidate, active_traveler=True)
        normal, normal_changes = prepare_candidate(self.source)
        self.assertEqual(len(normal_changes), 3)
        self.assertNotEqual(normal, candidate)

    def test_unsupported_executable_rejected(self):
        for data in (b'MZ', b'!' + self.image[1:]):
            with self.assertRaisesRegex(ValueError, 'executable hash'):
                extract_persistent(data)
