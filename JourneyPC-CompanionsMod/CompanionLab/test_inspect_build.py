import gzip
import struct
import unittest
from unittest.mock import patch

import inspect_build as audit


class InspectionTests(unittest.TestCase):
    def test_gzip_text_and_limit(self):
        self.assertEqual(audit.gunzip_script(gzip.compress(b"return 1")), b"return 1")
        with patch.object(audit, "SCRIPT_LIMIT", 8):
            with self.assertRaises(ValueError):
                audit.gunzip_script(gzip.compress(b"x" * 9))
        with self.assertRaises((EOFError, OSError)):
            audit.gunzip_script(gzip.compress(b"test")[:-5])

    def test_signatures_count_overlapping_matches_and_skip_data(self):
        sections = [dict(executable=True, raw_offset=0, raw_size=4, rva=4096),
                    dict(executable=False, raw_offset=4, raw_size=4, rva=8192)]
        hits = audit.signature_hits(b"AAAA" * 2, sections, "41 ?? 41")
        self.assertEqual([h["file_offset"] for h in hits], [0, 1])
        self.assertEqual([h["rva"] for h in hits], [4096, 4097])
        self.assertEqual(audit.signature_hits(b"AAAA" * 2, sections, "42"), [])

    def test_bad_pe_rejected(self):
        with self.assertRaises(ValueError):
            audit.pe_sections(b"not PE")
        with self.assertRaises((ValueError, struct.error)):
            audit.pe_sections(b"MZ")

    def test_path_escape_rejected_without_reading_target(self):
        with self.assertRaises(ValueError):
            audit.inside(audit.ROOT / ".." / "not-read")
        with self.assertRaises(ValueError):
            audit.local_output(audit.ROOT / "JourneyPC" / "not-written")


if __name__ == "__main__":
    unittest.main()
