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
            dll = (HERE / 'probe_dll.c').read_text()
            logging = []
            for begin, finish in (
                ('static int jnet_observation_line(', 'static lua_State *bound_state;'),
                ('static void log_line(', 'static void message('),
                ('static int lua_log(', 'static int execute('),
            ):
                start = dll.index(begin)
                logging.append(dll[start:dll.index(finish, start)])
            (work / 'jnet_dll_logging_under_test.h').write_text(''.join(logging))
            helper = (HERE / 'probe_helper.c').read_text()
            start = helper.index('static int session_write(')
            end = helper.index('static void record_hang_threads(', start)
            (work / 'jnet_report_under_test.h').write_text(helper[start:end])
            start = helper.index('static void format_exit_report(')
            end = helper.index('static void append_exit_text(', start)
            (work / 'jnet_exit_report_under_test.h').write_text(helper[start:end])
            start = helper.index('static int result_scale(')
            end = helper.index('static HWND result_control(', start)
            (work / 'jnet_client_layout_under_test.h').write_text(helper[start:end])
            recorder = (HERE / 'connection_recorder.h').read_text()
            start = recorder.index('static int connection_log(')
            end = recorder.index('static int lifecycle_arm(', start)
            report = recorder.index('static char *connection_report(')
            dialog = recorder.index('static INT_PTR CALLBACK connection_dialog(', report)
            (work / 'jnet_connection_export_under_test.h').write_text(recorder[start:end] + recorder[report:dialog])
            for name in ('test_jnet_session_log', 'test_jnet_paths', 'test_jnet_report_export'):
                binary = work / name
                subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                                '-fsanitize=address,undefined', '-pthread', '-I', str(HERE), '-I', str(work),
                                str(HERE / (name + '.c')), '-o', str(binary)], check=True)
                subprocess.run([str(binary)], check=True, timeout=30)

    def test_link_invitation_branding_and_compatibility(self):
        helper = (HERE / 'probe_helper.c').read_text()
        start = helper.index('size_t i=', helper.index('static INT_PTR CALLBACK invite_dialog('))
        parse = helper[start:helper.index('SecureZeroMemory(text,', start)]
        start = helper.index('for (unsigned b=0;b<4;++b) invite_config[b]', start)
        port_bytes = helper[start:helper.index('SetDlgItemTextW(', start)]
        start = helper.index('unsigned port=', helper.index('static int copy_link_invite('))
        format_invite = helper[start:helper.index('text[n]=0;', start) + len('text[n]=0;')]
        self.assertIn('Invalid invite (JN1-port-key)', helper)
        self.assertNotIn('Invalid invite (JC1-port-key)', helper)
        fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
static unsigned char invite_config[36];
static int parse_invite(const wchar_t *text) {
    memset(invite_config,0,sizeof(invite_config));
    PARSE_INVITE
    if (!valid || !any) return 0;
    PORT_BYTES
    return 1;
}
static int format_invite(const unsigned char config[36],wchar_t text[82]) {
    FORMAT_INVITE
    return n;
done:
    return 0;
}
int main(void) {
    unsigned char config[36]={0};
    struct { wchar_t text[82]; uint64_t guard; } output={{0},UINT64_C(0x123456789abcdef0)};
    const unsigned ports[]={1,9,10,255,256,9999,10000,65535};
    for (unsigned key=0;key<3;++key) {
        for (unsigned i=0;i<32;++i) config[4+i]=key==0?(unsigned char)i:key==1?255:0;
        if (key==2) config[35]=1;
        for (unsigned p=0;p<sizeof(ports)/sizeof(ports[0]);++p) {
            unsigned port=ports[p];config[0]=(unsigned char)port;config[1]=(unsigned char)(port>>8);
            for (unsigned i=0;i<82;++i) output.text[i]=L'!';
            int length=format_invite(config,output.text);
            assert(length>=70 && length<=74 && (size_t)length==wcslen(output.text));
            assert(!wcsncmp(output.text,L"JN1-",4) && output.text[length+1]==L'!');
            assert(output.guard==UINT64_C(0x123456789abcdef0));
            assert(parse_invite(output.text) && !memcmp(config,invite_config,36));
            for (unsigned legacy=0;legacy<3;++legacy) {
                wchar_t text[82];wcscpy(text,output.text);
                if (legacy==1) text[1]=L'C';
                if (legacy==2) {memmove(text+6,text+4,(wcslen(text+4)+1)*sizeof(*text));wmemcpy(text,L"JNET1-",6);}
                assert(parse_invite(text) && !memcmp(config,invite_config,36));
                size_t size=wcslen(text);
                for (size_t cut=0;cut<size;++cut) {
                    wchar_t saved=text[cut];text[cut]=0;assert(!parse_invite(text));text[cut]=saved;
                }
                for (size_t i=size-64;i<size;++i) if (text[i]>=L'a' && text[i]<=L'f') text[i]-=L'a'-L'A';
                assert(parse_invite(text) && !memcmp(config,invite_config,36));
                for (size_t i=size-64;i<size;++i) {
                    wchar_t saved=text[i];text[i]=L'g';assert(!parse_invite(text));
                    text[i]=0x100;assert(!parse_invite(text));text[i]=saved;
                }
                text[size]=L'0';text[size+1]=0;assert(!parse_invite(text));
                wmemset(text+size-64,L'0',64);text[size]=0;assert(!parse_invite(text));
            }
        }
    }
    const wchar_t *bad_ports[]={L"0",L"00",L"01",L"65536",L"9999999999",L"-1",L"+1",L"",L"1 ",L" 1"};
    const wchar_t *prefixes[]={L"JN1-",L"JC1-",L"JNET1-"};
    for (unsigned prefix=0;prefix<3;++prefix) for (unsigned p=0;p<sizeof(bad_ports)/sizeof(bad_ports[0]);++p) {
        wchar_t text[82];int n=swprintf(text,82,L"%ls%ls-",prefixes[prefix],bad_ports[p]);
        assert(n>0 && n+64<82);wmemset(text+n,L'f',64);text[n+64]=0;assert(!parse_invite(text));
    }
    const wchar_t *bad_prefixes[]={L"JN2-",L"jn1-",L"JNET2-",L"JNET-",L"jnet1-",L"JC2-",L"jc1-",L""};
    for (unsigned p=0;p<sizeof(bad_prefixes)/sizeof(bad_prefixes[0]);++p) {
        wchar_t text[82];int n=swprintf(text,82,L"%ls1-",bad_prefixes[p]);
        assert(n>0 && n+64<82);wmemset(text+n,L'f',64);text[n+64]=0;assert(!parse_invite(text));
    }
    for (unsigned length=80;length<=81;++length) {
        wchar_t text[82];wmemset(text,L'f',length);wmemcpy(text,L"JNET1-65535-",12);text[length]=0;assert(!parse_invite(text));
    }
    config[0]=config[1]=0;assert(!format_invite(config,output.text));
    config[0]=1;config[2]=1;assert(!format_invite(config,output.text));
    config[2]=0;config[3]=1;assert(!format_invite(config,output.text));
    return 0;
}
'''.replace('PARSE_INVITE', parse).replace('PORT_BYTES', port_bytes).replace('FORMAT_INVITE', format_invite)
        with tempfile.TemporaryDirectory(prefix='invitation-check-', dir=LAB / 'work') as directory:
            work = Path(directory)
            source, binary = work / 'invite.c', work / 'invite'
            source.write_text(fixture)
            subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', str(source), '-o', str(binary)], check=True)
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
        recorder = (HERE / 'connection_recorder.h').read_text()
        self.assertNotIn('jc-connection-', recorder)
        self.assertNotIn('CreateFile', recorder[recorder.index('static int connection_record('):])
        self.assertNotIn('Select text to pause live refresh', helper)
        self.assertNotIn('Meet Everywhere', (HERE / 'jnet_menu_live.h').read_text())
        self.assertIn('const unsigned meet=1,', (HERE / 'jnet_travel_live.h').read_text())

if __name__ == '__main__':
    unittest.main()
