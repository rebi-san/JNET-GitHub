
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <stddef.h>
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uintptr_t HANDLE;
#define JC_PATH_CAP 1024
#define JC_READER_PASS_EXIT 0x4a430001u
#define FALSE 0
#define CREATE_NO_WINDOW 1
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258
#define WAIT_FAILED 0xffffffffu
#define STILL_ACTIVE 259
struct startup { unsigned cb; }; typedef struct startup STARTUPINFOW;
struct process { HANDLE hProcess,hThread; }; typedef struct process PROCESS_INFORMATION;
static unsigned scenario,waits,closed,terminated;
static DWORD GetModuleFileNameW(void *m,wchar_t *out,DWORD size) {
 (void)m;assert(size==JC_PATH_CAP);wcscpy(out,L"C:\\Game Folder\\JC-Diagnostics\\Scarfhelper.exe");return (DWORD)wcslen(out);
}
static int CreateProcessW(const wchar_t *exe,wchar_t *command,void *a,void *b,int inherit,DWORD flags,void *env,void *cwd,STARTUPINFOW *s,PROCESS_INFORMATION *p) {
 assert(!a&&!b&&!inherit&&!env&&!cwd&&flags==CREATE_NO_WINDOW&&s->cb==sizeof(*s));
 assert(wcsstr(command,exe) && command[0]=='"' && wcsstr(command,L" --reader-check-child"));
 if(scenario==3)return 0;*p=(PROCESS_INFORMATION){10,11};return 1;
}
static int CloseHandle(HANDLE h) {assert(h==10||h==11);++closed;return 1;}
static DWORD WaitForSingleObject(HANDLE h,DWORD timeout) {
 assert(h==10);++waits;assert(timeout==(waits==1?5000:1000));
 return scenario==2 ? WAIT_TIMEOUT : scenario==4 ? WAIT_FAILED : WAIT_OBJECT_0;
}
static int GetExitCodeProcess(HANDLE h,DWORD *out) {
 assert(h==10);if(scenario==5)return 0;
 *out=scenario==0?JC_READER_PASS_EXIT:scenario==1?3:scenario==6?0:0xc0000005u;return 1;
}
static int TerminateProcess(HANDLE h,unsigned code) {assert(h==10&&code==104);++terminated;return 1;}
static void trace(const char *stage,DWORD exit_code,DWORD result) {(void)stage;(void)exit_code;(void)result;}
static LONG reader_check_isolated(void) {
    wchar_t executable[JC_PATH_CAP],command[JC_PATH_CAP+64];
    DWORD length=GetModuleFileNameW(NULL,executable,JC_PATH_CAP);
    if (!length || length>=JC_PATH_CAP) return 100;
    int n=swprintf(command,JC_PATH_CAP+64,L"\"%ls\" --reader-check-child",executable);
    if (n<=0 || n>=JC_PATH_CAP+64) return 100;
    STARTUPINFOW startup={.cb=sizeof(startup)}; PROCESS_INFORMATION child={0};
    if (!CreateProcessW(executable,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&startup,&child)) return 100;
    CloseHandle(child.hThread);
    DWORD waited=WaitForSingleObject(child.hProcess,5000),exit_code=STILL_ACTIVE;
    LONG result=102;
    if (waited==WAIT_OBJECT_0 && GetExitCodeProcess(child.hProcess,&exit_code))
        result=exit_code==JC_READER_PASS_EXIT ? 0 : exit_code>=1 && exit_code<=6 ? (LONG)exit_code : 103;
    else {
        result=waited==WAIT_TIMEOUT ? 101 : 102;
        TerminateProcess(child.hProcess,104);
        WaitForSingleObject(child.hProcess,1000);
    }
    CloseHandle(child.hProcess);
    trace("isolated_reader_check",exit_code,(DWORD)result);
    return result;
}


int main(void) {
 const LONG expected[]={0,3,101,100,102,102,103,103};
 for(scenario=0;scenario<8;++scenario) {
  waits=closed=terminated=0;assert(reader_check_isolated()==expected[scenario]);
  assert(closed==(scenario==3?0:2));
  assert(terminated==((scenario==2||scenario==4||scenario==5)?1u:0u));
 }
 puts("Reader watchdog: pass, fault rejection, timeout, spawn/wait/exit-query failure, premature success and crash exit checked; only the owned child is terminated.");
}
