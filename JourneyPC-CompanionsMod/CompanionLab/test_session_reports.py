from pathlib import Path
import subprocess
import tempfile
import unittest

LAB = Path(__file__).resolve().parent
HERE = LAB / 'diagnostic'

class SessionReportTests(unittest.TestCase):
    def test_memory_logs_and_portable_paths(self):
        with tempfile.TemporaryDirectory(prefix='session-check-', dir=LAB / 'work') as directory:
            work = Path(directory)
            source = (HERE / 'platform.h').read_text()
            start = source.index('static inline int jc_parent(')
            end = source.index('static inline int jc_hash(', start)
            (work / 'jnet_paths_under_test.h').write_text(source[start:end])
            helper = (HERE / 'probe_helper.c').read_text()
            start = helper.index('static int session_write(')
            end = helper.index('static void record_hang_threads(', start)
            (work / 'jnet_report_under_test.h').write_text(helper[start:end])
            for name in ('test_jnet_session_log', 'test_jnet_paths', 'test_jnet_report_export'):
                binary = work / name
                subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                                '-fsanitize=address,undefined', '-pthread', '-I', str(HERE), '-I', str(work),
                                str(HERE / (name + '.c')), '-o', str(binary)], check=True)
                subprocess.run([str(binary)], check=True, timeout=30)

    def test_no_automatic_diagnostic_file_writers(self):
        dll = (HERE / 'probe_dll.c').read_text()
        helper = (HERE / 'probe_helper.c').read_text()
        self.assertNotIn('WriteFile(', dll)
        self.assertNotIn('CreateFileW(', dll)
        self.assertNotIn('jnet-runtime-', helper)
        self.assertNotIn('jnet-client-', helper)
        self.assertNotIn('jnet-exit-', helper)
        self.assertNotIn('jnet-fault-', helper)
        self.assertNotIn('jnet-hang-', helper)
        self.assertNotIn('Select text to pause live refresh', helper)
        self.assertNotIn('Meet Everywhere', (HERE / 'jnet_menu_live.h').read_text())
        self.assertIn('const unsigned meet=1,', (HERE / 'jnet_travel_live.h').read_text())

if __name__ == '__main__':
    unittest.main()
