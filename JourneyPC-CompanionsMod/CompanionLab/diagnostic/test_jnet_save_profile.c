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
#define OPEN_EXISTING 16
#define CREATE_ALWAYS 32
#define FILE_ATTRIBUTE_NORMAL 64
#define MOVEFILE_REPLACE_EXISTING 1
#define MOVEFILE_WRITE_THROUGH 2
#define ERROR_ALREADY_EXISTS 183
#define JNET_SCARF_MAX 30
static struct file {wchar_t path[MAX_PATH];unsigned char bytes[48];unsigned size;} disk[2],temp;
static unsigned failure,flushed;
static HMODULE LoadLibraryW(const wchar_t *s){assert(!wcscmp(s,L"shell32.dll"));return (void *)1;}
static HRESULT folder(HWND w,int id,HANDLE t,DWORD f,LPWSTR out){(void)w;(void)t;(void)f;assert(id==0x1c);wcscpy(out,L"D:\\Users\\\u65c5\u4eba\\Local");return 0;}
static void *GetProcAddress(HMODULE m,const char *s){(void)m;assert(!strcmp(s,"SHGetFolderPathW"));return (void *)folder;}
static void FreeLibrary(HMODULE m){(void)m;}
static unsigned GetLastError(void){return ERROR_ALREADY_EXISTS;}
static int CreateDirectoryW(const wchar_t *p,void *s){(void)s;assert(wcsstr(p,L"\\JNET"));return 1;}
static HANDLE CreateFileW(const wchar_t *p,unsigned a,unsigned sh,void *se,unsigned mode,unsigned f,void *t){
    (void)sh;(void)se;(void)f;(void)t;
    assert(wcsstr(p,L"D:\\Users\\\u65c5\u4eba\\Local\\JNET\\638230-"));
    if(a==GENERIC_READ){assert(mode==OPEN_EXISTING);for(unsigned i=0;i<2;++i)if(!wcscmp(p,disk[i].path))return disk+i;return INVALID_HANDLE_VALUE;}
    assert(a==GENERIC_WRITE && mode==CREATE_ALWAYS && wcsstr(p,L".save.tmp"));flushed=0;wcscpy(temp.path,p);return &temp;
}
static DWORD GetFileSize(HANDLE f,void *h){(void)h;return ((struct file *)f)->size;}
static int ReadFile(HANDLE f,void *out,DWORD n,DWORD *got,void *o){(void)o;memcpy(out,((struct file *)f)->bytes,n);*got=n;return 1;}
static int WriteFile(HANDLE f,const void *in,DWORD n,DWORD *w,void *o){(void)o;assert(f==&temp && n==48);memcpy(temp.bytes,in,n);temp.size=n;*w=failure==1?n-1:n;return 1;}
static int FlushFileBuffers(HANDLE f){assert(f==&temp);flushed=1;return failure!=2;}
static void CloseHandle(HANDLE f){(void)f;}
static int MoveFileExW(const wchar_t *a,const wchar_t *b,unsigned f){assert(!wcscmp(a,temp.path) && flushed && f==3);if(failure==3)return 0;
    for(unsigned i=0;i<2;++i)if(!disk[i].path[0] || !wcscmp(b,disk[i].path)){disk[i]=temp;wcscpy(disk[i].path,b);return 1;}return 0;
}
static int DeleteFileW(const wchar_t *p){assert(!wcscmp(p,temp.path));memset(&temp,0,sizeof(temp));return 1;}
#define _WIN32
#include "jnet_save_profile.h"
int main(void){
    assert(setlocale(LC_CTYPE,"en_US.UTF-8"));
    struct jnet_save_profile a={.identity=123456789,.outfit=5,.symbol=18,.scarf=14,.energy=14,.level=1},b=a,out={0};
    assert(jnet_profile_write(&a));b.identity=987654321;b.outfit=1;b.symbol=14;b.scarf=b.energy=30;
    assert(jnet_profile_write(&b));assert(jnet_profile_read(&out,a.identity) && out.outfit==5 && out.scarf==14);
    assert(jnet_profile_read(&out,b.identity) && out.outfit==1 && out.symbol==14 && out.scarf==30);
    a.scarf=a.energy=18;
    for(failure=1;failure<=3;++failure){assert(!jnet_profile_write(&a));assert(jnet_profile_read(&out,a.identity) && out.scarf==14);}
    failure=0;assert(jnet_profile_write(&a));assert(jnet_profile_read(&out,a.identity) && out.scarf==18);
    assert(!jnet_profile_read(&out,1));disk[0].bytes[24]^=1;assert(!jnet_profile_read(&out,a.identity));
    disk[1].size=47;assert(!jnet_profile_read(&out,b.identity));a.identity=0;assert(!jnet_profile_write(&a));
    puts("Account save I/O: Unicode user path, distinct accounts, verified replacement, partial write/flush/replace failure preservation, missing/corrupt/truncated record rejection passed.");
}
