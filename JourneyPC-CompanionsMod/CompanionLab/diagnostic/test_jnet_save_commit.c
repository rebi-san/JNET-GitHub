#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
typedef void *HANDLE,*HMODULE,*HWND;
typedef unsigned DWORD;
typedef int HRESULT;
typedef wchar_t *LPWSTR;
#define WINAPI
#define MAX_PATH 260
#define SUCCEEDED(x) ((x)>=0)
#define INVALID_HANDLE_VALUE ((void *)(intptr_t)-1)
#define GENERIC_READ 1
#define GENERIC_WRITE 2
#define FILE_SHARE_READ 4
#define FILE_SHARE_WRITE 8
#define OPEN_EXISTING 16
#define CREATE_ALWAYS 32
#define FILE_ATTRIBUTE_NORMAL 64
#define MOVEFILE_REPLACE_EXISTING 1
#define MOVEFILE_WRITE_THROUGH 2
#define ERROR_FILE_NOT_FOUND 2
static unsigned char disk[0x8000],temporary[0x8000],save[0x1bb0],files[0x20300];
static unsigned exists=1,size=sizeof(disk),failure,flushed,moved,queued,long_folder,opened;
static const wchar_t user_folder[]=L"C:\\Users\\\u65c5\u7a0b\\AppData\\Local";
static HMODULE LoadLibraryW(const wchar_t *s){assert(!wcscmp(s,L"shell32.dll"));return (void *)1;}
static HRESULT folder(HWND w,int id,HANDLE t,DWORD f,LPWSTR out){
    (void)w;(void)t;(void)f;assert(id==0x1c);
    if(long_folder){wmemset(out,L'x',MAX_PATH-1);out[MAX_PATH-1]=0;}else wcscpy(out,user_folder);
    return 0;
}
static void *GetProcAddress(HMODULE m,const char *s){assert(m && !strcmp(s,"SHGetFolderPathW"));return (void *)folder;}
static void FreeLibrary(HMODULE m){(void)m;}
static unsigned GetLastError(void){return ERROR_FILE_NOT_FOUND;}
static HANDLE CreateFileW(const wchar_t *p,unsigned a,unsigned sh,void *se,unsigned mode,unsigned f,void *t){
    (void)sh;(void)se;(void)f;(void)t;
    ++opened;assert(!wcsncmp(p,user_folder,wcslen(user_folder)));
    if(a==GENERIC_READ){assert(mode==OPEN_EXISTING && wcsstr(p,L"SAVE.BIN"));return exists?disk:INVALID_HANDLE_VALUE;}
    assert(a==GENERIC_WRITE && mode==CREATE_ALWAYS && wcsstr(p,L"SAVE.BIN.jnet-tmp"));return temporary;
}
static DWORD GetFileSize(HANDLE f,void *h){(void)h;assert(f==disk);return size;}
static int ReadFile(HANDLE f,void *out,DWORD n,DWORD *got,void *o){(void)o;assert(f==disk);memcpy(out,disk,n);*got=n;return 1;}
static int WriteFile(HANDLE f,const void *in,DWORD n,DWORD *w,void *o){(void)o;assert(f==temporary && n==sizeof(disk));memcpy(temporary,in,n);*w=failure==1?n-1:n;return 1;}
static int FlushFileBuffers(HANDLE f){assert(f==temporary);flushed=1;return failure!=2;}
static void CloseHandle(HANDLE f){assert(f==disk || f==temporary);}
static int MoveFileExW(const wchar_t *a,const wchar_t *b,unsigned f){assert(!wcsncmp(a,user_folder,wcslen(user_folder)) && !wcsncmp(b,user_folder,wcslen(user_folder)) && flushed && f==3);if(failure==3)return 0;memcpy(disk,temporary,sizeof(disk));exists=1;moved=1;return 1;}
static int DeleteFileW(const wchar_t *p){assert(!wcsncmp(p,user_folder,wcslen(user_folder)));return 1;}
static int own_read(void *c,uintptr_t a,void *o,size_t n){(void)c;memcpy(o,(void *)a,n);return 1;}
static void message(const char *s){assert(strstr(s,"save"));}
#define JNET_SCARF_MAX 30
static int jc_local_windows_write(uintptr_t a,const void *in,size_t n){memcpy((void *)a,in,n);return 1;}
#include "save_commit_code.h"
static void native(uintptr_t s,uintptr_t f){assert(s==(uintptr_t)save && f==(uintptr_t)files);++queued;}
int main(void){
    assert(setlocale(LC_CTYPE,"en_US.UTF-8") || setlocale(LC_CTYPE,"C.UTF-8"));
    jnet_save_serialize_original=native;unsigned state=1;memcpy(files+0x200bc,&state,4);
    memset(save+8,0x57,0x1ba8);unsigned short version=22;memcpy(save+8,&version,2);
    for(failure=0;failure<4;++failure){
        memset(disk,0x31,sizeof(disk));flushed=moved=queued=0;
        jnet_save_serialize((uintptr_t)save,(uintptr_t)files);assert(queued==1);
        if(!failure){assert(moved && !memcmp(disk,save+8,0x1ba8));assert(disk[0x1ba8]==0x31);}
        else {assert(!moved && disk[0]==0x31 && disk[sizeof(disk)-1]==0x31);}
    }
    failure=0;exists=0;flushed=moved=0;jnet_save_serialize((uintptr_t)save,(uintptr_t)files);
    assert(moved && !memcmp(disk,save+8,0x1ba8) && disk[0x1ba8]==0);
    size=12;moved=0;jnet_save_serialize((uintptr_t)save,(uintptr_t)files);assert(!moved);
    size=sizeof(disk);state=2;memcpy(files+0x200bc,&state,4);moved=0;
    jnet_save_serialize((uintptr_t)save,(uintptr_t)files);assert(!moved);
    memset(disk,0,sizeof(disk));memset(save+8,0,0x1ba8);
    unsigned fields[]={22,0,5,20,10,10,2};uint64_t stamp=100;
    memcpy(disk,fields,sizeof(fields));memcpy(disk+0x40,&stamp,8);
    assert(jnet_save_restore_local((uintptr_t)save+8));
    assert(!memcmp(save+8,disk,0x1ba8));
    assert(!jnet_save_restore_local((uintptr_t)save+8));
    stamp=99;memcpy(disk+0x40,&stamp,8);
    assert(!jnet_save_restore_local((uintptr_t)save+8));
    stamp=101;memcpy(disk+0x40,&stamp,8);disk[12]=21;
    assert(!jnet_save_restore_local((uintptr_t)save+8));
    size=12;assert(!jnet_save_restore_local((uintptr_t)save+8));
    long_folder=1;unsigned before=opened;assert(!jnet_save_restore_local((uintptr_t)save+8));
    state=1;memcpy(files+0x200bc,&state,4);queued=0;jnet_save_serialize((uintptr_t)save,(uintptr_t)files);
    assert(opened==before && queued==1);
    puts("Save commit: Unicode Windows paths, path truncation, payload, preserved tail, new save, partial write, flush/replace failure, invalid file and busy guards passed; native queue retained.");
}
