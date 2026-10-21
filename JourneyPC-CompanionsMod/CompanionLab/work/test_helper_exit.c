
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <assert.h>
#define JC_PATH_CAP 4096
static wchar_t preview_text[20000];
static struct { wchar_t directory[JC_PATH_CAP];FILETIME created; } exit_watch;
static void append_exit_text(const wchar_t *value) {
    size_t used=wcslen(preview_text),count=wcslen(value);
    if (count>19999-used) count=19999-used;
    wmemcpy(preview_text+used,value,count);preview_text[used+count]=0;
}
static void append_log_tail(const wchar_t *path,const wchar_t *label,DWORD limit) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file==INVALID_HANDLE_VALUE) return;
    LARGE_INTEGER size,at;char bytes[6001];DWORD got=0;
    if (limit>6000) limit=6000;
    if (!GetFileSizeEx(file,&size) || size.QuadPart<0) {CloseHandle(file);return;}
    at.QuadPart=size.QuadPart>limit ? size.QuadPart-limit : 0;
    int ok=SetFilePointerEx(file,at,NULL,FILE_BEGIN) && ReadFile(file,bytes,limit,&got,NULL);
    CloseHandle(file);if (!ok) return;
    bytes[got]=0;char *start=bytes;
    if (at.QuadPart) {char *line=strchr(bytes,'\n');if(line)start=line+1;}
    wchar_t decoded[6001];
    if (!MultiByteToWideChar(CP_UTF8,0,start,-1,decoded,6001)) return;
    append_exit_text(L"\r\n\r\n");append_exit_text(label);append_exit_text(L"\r\n");
    append_exit_text(got ? decoded : L"No entries recorded.");
}
static void append_session_log(const wchar_t *kind,DWORD pid,DWORD limit) {
    wchar_t pattern[JC_PATH_CAP],selected[JC_PATH_CAP]={0};WIN32_FIND_DATAW data;FILETIME newest={0};
    int n=swprintf(pattern,JC_PATH_CAP,L"%ls\\%ls-*-%lu.log",exit_watch.directory,kind,(unsigned long)pid);
    if (n<0 || n>=JC_PATH_CAP) return;
    HANDLE search=FindFirstFileW(pattern,&data);if(search==INVALID_HANDLE_VALUE)return;
    do {
        if (!(data.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) &&
            CompareFileTime(&data.ftLastWriteTime,&exit_watch.created)>=0 &&
            CompareFileTime(&data.ftLastWriteTime,&newest)>=0) {
            n=swprintf(selected,JC_PATH_CAP,L"%ls\\%ls",exit_watch.directory,data.cFileName);
            if(n<0 || n>=JC_PATH_CAP)selected[0]=0;else newest=data.ftLastWriteTime;
        }
    } while(FindNextFileW(search,&data));
    FindClose(search);
    if(selected[0])append_log_tail(selected,kind,limit);
}
static DWORD force_close_journey(HANDLE process,DWORD expected_pid) {
    if (!process || !expected_pid || GetProcessId(process)!=expected_pid) return ERROR_INVALID_HANDLE;
    DWORD waited=WaitForSingleObject(process,0);
    if (waited==WAIT_OBJECT_0) return 0;
    if (waited!=WAIT_TIMEOUT) return GetLastError();
    return TerminateProcess(process,0xe04a4302u) ? 0 : GetLastError();
}

static PROCESS_INFORMATION child(void) {
 wchar_t exe[4096],command[4200];assert(GetModuleFileNameW(NULL,exe,4096));
 swprintf(command,4200,L"\"%ls\" --child",exe);
 STARTUPINFOW startup={.cb=sizeof(startup)};PROCESS_INFORMATION info={0};
 assert(CreateProcessW(exe,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&startup,&info));CloseHandle(info.hThread);return info;
}
static void log_file(const wchar_t *name,const char *text,int stale) {
 HANDLE f=CreateFileW(name,GENERIC_WRITE|FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);assert(f!=INVALID_HANDLE_VALUE);
 DWORD n=0;assert(WriteFile(f,text,(DWORD)strlen(text),&n,NULL)&&n==strlen(text));
 if(stale){FILETIME old={1,0};assert(SetFileTime(f,NULL,NULL,&old));}CloseHandle(f);
}
int main(int argc,char **argv) {
 (void)argv;if(argc>1){Sleep(INFINITE);return 0;}
 PROCESS_INFORMATION a=child(),b=child();
 assert(force_close_journey(NULL,a.dwProcessId)==ERROR_INVALID_HANDLE);
 assert(force_close_journey(a.hProcess,b.dwProcessId)==ERROR_INVALID_HANDLE);
 assert(WaitForSingleObject(a.hProcess,0)==WAIT_TIMEOUT);
 assert(!force_close_journey(a.hProcess,a.dwProcessId));
 assert(WaitForSingleObject(a.hProcess,5000)==WAIT_OBJECT_0);DWORD code=0;
 assert(GetExitCodeProcess(a.hProcess,&code)&&code==0xe04a4302u);
 assert(!force_close_journey(a.hProcess,a.dwProcessId));
 assert(WaitForSingleObject(b.hProcess,0)==WAIT_TIMEOUT);
 assert(TerminateProcess(b.hProcess,0));assert(WaitForSingleObject(b.hProcess,5000)==WAIT_OBJECT_0);
 CloseHandle(a.hProcess);CloseHandle(b.hProcess);
 wchar_t folder[80];swprintf(folder,80,L"helper-exit-fixture-%lu",(unsigned long)GetCurrentProcessId());
 assert(CreateDirectoryW(folder,NULL));assert(SetCurrentDirectoryW(folder));
 assert(GetCurrentDirectoryW(JC_PATH_CAP,exit_watch.directory));GetSystemTimeAsFileTime(&exit_watch.created);
 log_file(L"jc-probe-2000-123.log","STALE_SESSION",1);
 log_file(L"jc-probe-2001-124.log","OTHER_PROCESS",0);
 log_file(L"jc-probe-2002-123.log","[JC] cloth registration failed detail=322\r\n",0);
 append_session_log(L"jc-probe",123,6000);
 assert(wcsstr(preview_text,L"detail=322") && !wcsstr(preview_text,L"STALE_SESSION") && !wcsstr(preview_text,L"OTHER_PROCESS"));
 log_file(L"empty.log","",0);append_log_tail(L"empty.log",L"FAULT",1600);assert(wcsstr(preview_text,L"No entries recorded"));
 wchar_t large[21000];for(unsigned i=0;i<20999;++i)large[i]=L'x';large[20999]=0;
 append_exit_text(large);assert(wcslen(preview_text)==19999);append_exit_text(L"overflow");assert(wcslen(preview_text)==19999);
 puts("PASS: exact attached handle/PID, mismatched target refused, other process preserved, marked forced exit, already-closed target, session-specific log selection, empty logs and report bounds.");
 return 0;
}
