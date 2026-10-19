#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>
typedef int32_t LONG;
typedef unsigned DWORD;
typedef int HANDLE;
#define MAX_PATH 260
#define GENERIC_WRITE 1
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 3
#define INVALID_HANDLE_VALUE -1
#define MOVEFILE_REPLACE_EXISTING 1
#define MOVEFILE_WRITE_THROUGH 2
#define MB_OK 0
#define MB_ICONERROR 0
static int jnet_bindings[3][7]={{67,66,86,71},{0},{0}},jnet_bindings_saved[3][7];
static unsigned jnet_credits=1,pushed,errors,writes,jnet_daynight_mode;
static LONG jnet_fps_limit;
static wchar_t jnet_controls_path[MAX_PATH]=L"JNET-settings.ini";
static char disk[1024],pending[1024];
static int fail;
static unsigned char menu[0xe00],path[0x180];
static HANDLE CreateFileW(const wchar_t *p,int a,int b,void *c,int d,int e,void *f){return 1;}
static int WriteFile(HANDLE h,const void *p,DWORD n,DWORD *out,void *x){memcpy(pending,p,n);pending[n]=0;*out=n;++writes;return 1;}
static int FlushFileBuffers(HANDLE h){return 1;}
static void CloseHandle(HANDLE h){}
static int MoveFileExW(const wchar_t *a,const wchar_t *b,int flags){if(fail)return 0;strcpy(disk,pending);return 1;}
static void DeleteFileW(const wchar_t *p){pending[0]=0;}
static int MessageBoxW(void *w,const wchar_t *m,const wchar_t *t,int f){++errors;return 1;}
static LONG InterlockedExchange(volatile LONG *p,LONG v){LONG old=*p;*p=v;return old;}
static int jnet_menu_visible_native(uintptr_t m){return m==(uintptr_t)menu;}
static int jnet_menu_read(uintptr_t p,void *out,size_t n){memcpy(out,(void *)p,n);return 1;}
static int jnet_menu_string_is(uintptr_t p,const char *name){return !strcmp((char *)p,name);}
static void push(uintptr_t m,const char *name){assert(m==(uintptr_t)menu && !strcmp(name,"Back"));++pushed;}
static uintptr_t self,fault_image;
static int (*jnet_menu_settings_action)(uintptr_t,uintptr_t);
static int jnet_controls_limit(unsigned device){return device==0?255:device==1?5:16;}
static DWORD GetModuleFileNameW(uintptr_t module,wchar_t *out,DWORD length){wcscpy(out,L"C:\\Journey\\JNET\\JNET.dll");return (DWORD)wcslen(out);}
static unsigned GetPrivateProfileIntW(const wchar_t *section,const wchar_t *key,int fallback,const wchar_t *filename){
    if(!wcsstr(filename,L"JNET-settings.ini"))return fallback;
    char target[64];snprintf(target,sizeof(target),"%ls=",key);
    const char *value=strstr(disk,target);return value?(unsigned)atoi(value+strlen(target)):(unsigned)fallback;
}
#include "settings_fixture.h"
static void page(const char *name,int prompt){
    memset(path,0,sizeof(path));strcpy((char *)path,"Root");strcpy((char *)path+0x30,"Menu");strcpy((char *)path+0x60,name);
    if(prompt)strcpy((char *)path+0x90,"Back");
    uintptr_t begin=(uintptr_t)path,end=begin+(prompt?0xc0:0x90);
    memcpy(menu+0xc10,&begin,8);memcpy(menu+0xc18,&end,8);
}
static int act(const char *name){return jnet_settings_action((uintptr_t)menu,(uintptr_t)name);}
int main(void){
    memcpy(jnet_bindings_saved,jnet_bindings,sizeof(jnet_bindings));jnet_settings_push=push;
    page("Audio",0);assert(!act("Back") && !writes && !pushed);
    jnet_credits=0;assert(act("Back") && pushed==1 && !writes);
    page("Audio",1);assert(!act("Revert") && jnet_credits==1 && !writes);
    jnet_credits=0;jnet_fps_limit=2;jnet_bindings[0][0]=90;
    assert(!act("Apply") && !jnet_settings_dirty(1) && jnet_settings_dirty(2) && jnet_settings_dirty(3));
    assert(strstr(disk,"Credits=0") && strstr(disk,"FPS=0") && strstr(disk,"Binding00=67"));
    page("Graphics",0);assert(act("Back"));page("Graphics",1);
    fail=1;char before[1024];strcpy(before,disk);assert(act("Apply") && errors==1 && jnet_settings_dirty(2) && !strcmp(before,disk));
    assert(!act("Revert") && jnet_fps_limit==0);fail=0;jnet_fps_limit=1;assert(!act("Apply") && strstr(disk,"FPS=1"));
    page("Controls",0);assert(act("Back"));page("Controls",1);assert(!act("Revert") && jnet_bindings[0][0]==67);
    jnet_bindings[0][4]=70;jnet_bindings[1][5]=2;jnet_bindings[2][6]=12;
    assert(!act("Revert") && !jnet_bindings[0][4] && !jnet_bindings[1][5] && !jnet_bindings[2][6]);
    jnet_bindings[0][4]=70;jnet_bindings[1][5]=2;jnet_bindings[2][6]=12;
    jnet_bindings[0][0]=88;assert(!act("Apply") && strstr(disk,"Binding00=88") && !jnet_settings_dirty(3));
    page("Keyboard",0);assert(!act("Back"));
    memset(jnet_bindings,0,sizeof(jnet_bindings));jnet_credits=1;jnet_fps_limit=0;
    jnet_controls_initialize();
    assert(jnet_bindings[0][4]==70 && jnet_bindings[1][5]==2 && jnet_bindings[2][6]==12);
    assert(jnet_bindings[0][1]==66 && jnet_bindings[0][2]==86 && jnet_bindings[0][3]==71);
    assert(jnet_bindings[0][0]==88 && jnet_credits==0 && jnet_fps_limit==1);
    assert(!jnet_settings_dirty(1) && !jnet_settings_dirty(2) && !jnet_settings_dirty(3));
    for(unsigned lighting=0;lighting<3;++lighting){
        jnet_daynight_mode=lighting;assert(jnet_settings_apply(4));
        jnet_daynight_mode=99;jnet_controls_initialize();assert(jnet_daynight_mode==lighting);
        assert(jnet_bindings[0][0]==88 && jnet_credits==0 && jnet_fps_limit==1);
    }
    jnet_daynight_mode=1;fail=1;assert(!jnet_settings_apply(4) && jnet_saved_lighting==2);fail=0;
    puts("JNET settings: clean Back, dirty prompt, isolated Apply, Revert, failed atomic save and original-action passthrough passed.");
}
