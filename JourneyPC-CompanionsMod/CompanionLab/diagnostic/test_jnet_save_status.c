#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
typedef void *HMODULE,*HWND,*HANDLE;
typedef int HRESULT;
typedef unsigned DWORD;
typedef wchar_t *LPWSTR;
#define WINAPI
#define MAX_PATH 260
#define SUCCEEDED(x) ((x)>=0)
#define INVALID_HANDLE_VALUE ((void *)(intptr_t)-1)
#define GENERIC_READ 1
#define FILE_SHARE_READ 2
#define FILE_SHARE_WRITE 4
#define OPEN_EXISTING 8
#define FILE_ATTRIBUTE_NORMAL 16
static unsigned char disk[0x8000],files[0x20300],jnet_appearance_save_expected[0x1ba8];
static unsigned jnet_appearance_save_captured,jnet_appearance_saving,jnet_menu_page=1;
static char jnet_appearance_status[80];
static unsigned jnet_save_requested;
static uint64_t jnet_appearance_applied_at;
static struct {struct {unsigned character;} applied;} jnet_menu_edit;
static unsigned readable=1,disk_size=sizeof(disk),reads,long_folder,opened;
static uint64_t now=100;
static uint64_t GetTickCount64(void){return now;}
static HMODULE LoadLibraryW(const wchar_t *name){assert(!wcscmp(name,L"shell32.dll"));return (void *)1;}
static HRESULT folder(HWND w,int id,HANDLE token,DWORD flags,LPWSTR out){
    (void)w;(void)token;(void)flags;assert(id==0x1c);
    if(long_folder){wmemset(out,L'x',MAX_PATH-1);out[MAX_PATH-1]=0;}
    else wcscpy(out,L"C:\\Users\\\u65c5\u7a0b\\AppData\\Local");
    return 0;
}
static void *GetProcAddress(HMODULE m,const char *name){assert(m && !strcmp(name,"SHGetFolderPathW"));return (void *)folder;}
static void FreeLibrary(HMODULE m){assert(m);}
static HANDLE CreateFileW(const wchar_t *path,unsigned access,unsigned share,void *security,unsigned mode,unsigned flags,void *template){
    (void)share;(void)security;(void)flags;(void)template;
    assert(access==GENERIC_READ && mode==OPEN_EXISTING);
    assert(!wcscmp(path,L"C:\\Users\\\u65c5\u7a0b\\AppData\\Local\\Annapurna Interactive\\Journey\\Steam\\SAVE.BIN"));++opened;
    return readable?disk:INVALID_HANDLE_VALUE;
}
static DWORD GetFileSize(HANDLE f,void *high){(void)high;assert(f==disk);return disk_size;}
static int ReadFile(HANDLE f,void *out,unsigned bytes,DWORD *got,void *overlap){
    (void)overlap;assert(f==disk && bytes==sizeof(jnet_appearance_save_expected));
    memcpy(out,disk,bytes);*got=bytes;++reads;return 1;
}
static void CloseHandle(HANDLE f){assert(f==disk);}
static int own_read(void *ctx,uintptr_t at,void *out,size_t bytes){(void)ctx;memcpy(out,(void *)at,bytes);return 1;}
static void message(const char *line){assert(strstr(line,"local payload verified="));}
static void original(uintptr_t f){assert(f==(uintptr_t)files);}
static void (*jnet_save_poll_original)(uintptr_t)=original;
#include "save_poll_code.h"
static void state(unsigned value){memcpy(files+0x200bc,&value,4);}
static void begin(void){jnet_appearance_applied_at=0;jnet_appearance_saving=1;strcpy(jnet_appearance_status,"Saving");now+=2000;}
int main(void){
    assert(setlocale(LC_CTYPE,"en_US.UTF-8") || setlocale(LC_CTYPE,"C.UTF-8"));
    memset(jnet_appearance_save_expected,0x27,sizeof(jnet_appearance_save_expected));
    memcpy(disk,jnet_appearance_save_expected,sizeof(jnet_appearance_save_expected));
    jnet_appearance_save_captured=1;
    begin();state(4);jnet_save_poll((uintptr_t)files);
    assert(jnet_appearance_saving && !reads);
    state(1);now+=1000;jnet_save_poll((uintptr_t)files);
    assert(!jnet_appearance_saving && !jnet_appearance_status[0] && reads==1 && jnet_appearance_applied_at==now);
    begin();disk[12]^=1;state(4);jnet_save_poll((uintptr_t)files);
    now+=1000;jnet_save_poll((uintptr_t)files);
    assert(!strcmp(jnet_appearance_status,"Save failed"));
    begin();readable=0;state(3);jnet_save_poll((uintptr_t)files);
    assert(!strcmp(jnet_appearance_status,"Save failed"));
    begin();state(4);jnet_save_poll((uintptr_t)files);now+=1000;jnet_save_poll((uintptr_t)files);
    assert(!strcmp(jnet_appearance_status,"Save failed"));
    readable=1;disk[12]^=1;disk_size=12;
    assert(!jnet_appearance_local_saved());disk_size=sizeof(disk);
    jnet_appearance_save_captured=0;assert(!jnet_appearance_local_saved());
    jnet_appearance_save_captured=1;long_folder=1;unsigned before=opened;
    assert(!jnet_appearance_local_saved() && opened==before);
    puts("Save status: Unicode Windows paths, path truncation, delayed local readback after native error, reset-to-idle, exact payload matching, missing/truncated files and disk-confirmed completion passed with mocked file APIs.");
}
