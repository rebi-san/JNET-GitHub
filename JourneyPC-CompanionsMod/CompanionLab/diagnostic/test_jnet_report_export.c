#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "jnet_session_log.h"
typedef void *HANDLE;
typedef void *HWND;
typedef uint32_t DWORD;
typedef size_t SIZE_T;
#define JC_PATH_CAP 32768
#define MAXDWORD UINT32_MAX
#define INVALID_HANDLE_VALUE ((void *)(intptr_t)-1)
#define GENERIC_WRITE 1
#define FILE_SHARE_READ 2
#define CREATE_ALWAYS 3
#define FILE_ATTRIBUTE_NORMAL 4
#define OFN_OVERWRITEPROMPT 1
#define OFN_PATHMUSTEXIST 2
#define OFN_NOCHANGEDIR 4
#define GWLP_USERDATA 1
#define CP_UTF8 65001
struct picker {size_t lStructSize;HWND hwndOwner;const wchar_t *lpstrFilter;wchar_t *lpstrFile;DWORD nMaxFile;const wchar_t *lpstrDefExt;DWORD Flags;};
typedef struct picker OPENFILENAMEW;
static struct jnet_memory_log helper_log,exit_log,hang_log;
static struct jnet_session_logs logs;
static struct {struct jnet_session_logs *logs;}exit_watch={&logs};
static char output[16384];static size_t used;static int selected,opens,closes,short_write;
static wchar_t status_text[128];
static int GetSaveFileNameW(OPENFILENAMEW *p){assert(p->Flags&OFN_OVERWRITEPROMPT);return selected;}
static intptr_t GetWindowLongPtrW(HWND w,int i){(void)w;(void)i;return (intptr_t)L"Displayed session status";}
static int WideCharToMultiByte(unsigned cp,int flags,const wchar_t *text,int length,char *out,int cap,void *a,void *b){
    (void)cp;(void)flags;(void)length;(void)a;(void)b;int n=(int)wcslen(text)+1;
    if(out){assert(cap>=n);for(int i=0;i<n;++i)out[i]=(char)text[i];}return n;
}
static HANDLE GetProcessHeap(void){return (void *)1;}
static void *HeapAlloc(HANDLE h,int flags,size_t size){(void)h;(void)flags;return malloc(size);}
static void HeapFree(HANDLE h,int flags,void *p){(void)h;(void)flags;free(p);}
static HANDLE CreateFileW(const wchar_t *path,int access,int share,void *security,int mode,int flags,void *template){
    (void)path;(void)access;(void)share;(void)security;(void)flags;(void)template;
    assert(mode==CREATE_ALWAYS);++opens;used=0;output[0]=0;return (void *)1;
}
static int WriteFile(HANDLE file,const char *text,DWORD bytes,DWORD *written,void *overlap){
    (void)file;(void)overlap;assert(used+bytes<sizeof(output));
    *written=short_write && bytes?bytes-1:bytes;memcpy(output+used,text,*written);used+=*written;output[used]=0;return 1;
}
static int FlushFileBuffers(HANDLE h){(void)h;return 1;}
static void CloseHandle(HANDLE h){(void)h;++closes;}
static void SetDlgItemTextW(HWND w,int id,const wchar_t *text){(void)w;assert(id==117);wcscpy(status_text,text);}
#include "jnet_report_under_test.h"
int main(void){
    atomic_store(&logs.ready,1);
    jnet_log_append(&helper_log,"helper details\n",15);
    jnet_log_append(&logs.runtime,"runtime details\n",16);
    jnet_log_append(&logs.fault,"crash details\n",14);
    save_session_report(NULL);assert(!opens && !closes);
    selected=1;save_session_report(NULL);assert(opens==1 && closes==1);
    assert(strstr(output,"Displayed session status") && strstr(output,"helper details") && strstr(output,"runtime details") && strstr(output,"crash details"));
    assert(!wcscmp(status_text,L"Session report saved"));
    short_write=1;save_session_report(NULL);assert(opens==2 && closes==2);
    assert(wcsstr(status_text,L"Could not save"));
    puts("Report export: cancel creates nothing; explicit save includes client/runtime/fault records; short writes report failure.");
}
