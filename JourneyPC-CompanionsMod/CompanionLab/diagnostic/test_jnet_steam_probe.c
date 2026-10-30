#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __declspec(x)
typedef int32_t LONG;
typedef void *HMODULE;
typedef size_t SIZE_T;
typedef struct {unsigned State,Protect;} MEMORY_BASIC_INFORMATION;
#define MEM_COMMIT 1
#define PAGE_GUARD 2
#define PAGE_NOACCESS 4
#define PAGE_EXECUTE 8
#define PAGE_EXECUTE_READ 16
#define PAGE_EXECUTE_READWRITE 32
#define PAGE_EXECUTE_WRITECOPY 64
#define MH_OK 0
static unsigned original_calls,queries,online=1,app=638230,interfaces=1,modern=1,handles=1,missing;
static unsigned relay_initializations,auth_initializations,protected_method=1;
static int relay_availability=3,auth_availability=2;
static void *sockets_methods[24],*utils_methods[8];
static void **sockets_object=sockets_methods,**utils_object=utils_methods;
static void *GetCurrentProcess(void){return (void *)1;}
static int ReadProcessMemory(void *process,const void *source,void *target,size_t bytes,size_t *copied){assert(process);memcpy(target,source,bytes);*copied=bytes;return 1;}
static size_t VirtualQuery(void *method,MEMORY_BASIC_INFORMATION *info,size_t bytes){assert(method);info->State=MEM_COMMIT;info->Protect=protected_method?PAGE_EXECUTE_READ:PAGE_NOACCESS;return bytes;}
static uint64_t clock_ms=100;
static LONG InterlockedIncrement(volatile LONG *p){return ++*p;}
static LONG InterlockedExchange(volatile LONG *p,LONG v){LONG old=*p;*p=v;return old;}
static LONG InterlockedCompareExchange(volatile LONG *p,LONG v,LONG compare){LONG old=*p;if(old==compare)*p=v;return old;}
static uint64_t GetTickCount64(void){return clock_ms;}
static HMODULE GetModuleHandleW(const void *s){(void)s;return (void *)1;}
static void message(const char *s){(void)s;}
static void original(void){++original_calls;}
static void *client(void){return (void *)1;}
static int handle(void){return handles;}
static void *get_interface(void *c,int u,int p,const char *s){assert(c && u && p && s);++queries;return (void *)(uintptr_t)interfaces;}
static void *get_utils(void *c,int p,const char *s){assert(c && p && s);return (void *)1;}
static void *get_generic(void *c,int u,int p,const char *s){assert(c && u && p && s);++queries;if(!modern)return NULL;return strstr(s,"Sockets")?(void *)&sockets_object:(void *)&utils_object;}
static uint64_t get_id(void *u){assert(u);return UINT64_C(76561198000000000);}
static unsigned char logged(void *u){assert(u);return online;}
static uint32_t get_app(void *u){assert(u);return app;}
static void *(*extra_exports)(const char *);
static void *GetProcAddress(HMODULE m,const char *s) {
    assert(m);if(missing)return NULL;
    if(!strcmp(s,"SteamAPI_RunCallbacks"))return (void *)original;
    if(!strcmp(s,"SteamClient"))return (void *)client;
    if(!strcmp(s,"SteamAPI_GetHSteamUser") || !strcmp(s,"SteamAPI_GetHSteamPipe"))return (void *)handle;
    if(!strcmp(s,"SteamAPI_ISteamClient_GetISteamUtils"))return (void *)get_utils;
    if(!strcmp(s,"SteamAPI_ISteamUser_GetSteamID"))return (void *)get_id;
    if(!strcmp(s,"SteamAPI_ISteamUser_BLoggedOn"))return (void *)logged;
    if(!strcmp(s,"SteamAPI_ISteamUtils_GetAppID"))return (void *)get_app;
    if(!strcmp(s,"SteamAPI_ISteamClient_GetISteamUser"))return (void *)get_interface;
    if(!strcmp(s,"SteamAPI_ISteamClient_GetISteamMatchmaking"))return (void *)get_interface;
    if(!strcmp(s,"SteamAPI_ISteamClient_GetISteamGenericInterface"))return (void *)get_generic;
    if(extra_exports)return extra_exports(s);
    assert(!"unexpected Steam call");return NULL;
}
static int MH_CreateHook(void *target,void *hook,void **trampoline){assert(target && hook);*trampoline=target;return MH_OK;}
#include "jnet_steam_probe.h"
static unsigned char relay_init(void *self,float age){assert(self==&utils_object && age==1e10f);++relay_initializations;return 0;}
static int auth_init(void *self){assert(self==&sockets_object);++auth_initializations;return auth_availability;}
static int relay_status(void *self,struct jnet_steam_relay_status *out){assert(self==&utils_object);out->availability=relay_availability;out->configuration=100;out->any_relay=relay_availability;out->pinging=1;memset(out->text,'x',sizeof(out->text));out->text[0]='\n';return relay_availability;}
static int auth_status(void *self,struct jnet_steam_auth_status *out){assert(self==&sockets_object);out->availability=auth_availability;strcpy(out->text,"test authentication");return auth_availability;}
static unsigned owner_enters,owner_leaves;
static void owner_enter(void){++owner_enters;}
static void owner_leave(void){++owner_leaves;}
int main(void) {
    sockets_methods[22]=(void *)auth_init;sockets_methods[23]=(void *)auth_status;
    utils_methods[1]=(void *)relay_status;utils_methods[7]=(void *)relay_init;
    jnet_steam_install_probe();assert(JNETSteam[0]==1 && JNETSteam[1]==1);
    jnet_steam_callbacks();assert(original_calls==1 && JNETSteam[2]==1 && JNETSteam[0]==5);
    assert(JNETSteam[5]==638230 && JNETSteam[6]==1 && JNETSteam[10]==1 && JNETSteam[11]==1);
    assert(JNETSteam[13]==2 && JNETSteam[12]==6 && relay_initializations==1 && auth_initializations==1);
    assert(strlen(JNETSteamServiceText[0])==255 && JNETSteamServiceText[0][0]==' ');
    unsigned before=queries;jnet_steam_callbacks();assert(original_calls==2 && queries==before);
    online=0;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[0]==3);
    assert(jnet_steam_account_identity==UINT64_C(76561198000000000));
    app=480;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[0]==-5 && !JNETSteam[10]);
    handles=0;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[0]==2 && !JNETSteam[7]);
    handles=1;app=638230;online=1;interfaces=0;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[0]==-4);
    interfaces=1;modern=0;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[0]==4 && JNETSteam[9] && !JNETSteam[10]);
    missing=1;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[0]==-3);
    assert(original_calls==8 && JNETSteam[2]==8);
    missing=0;modern=1;relay_availability=auth_availability=100;clock_ms+=2000;jnet_steam_callbacks();
    assert(JNETSteam[13]==1 && JNETSteam[20]>0 && relay_initializations==1 && auth_initializations==1);
    auth_availability=-101;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[13]==3);
    protected_method=0;clock_ms+=2000;jnet_steam_callbacks();assert(JNETSteam[13]==-1 && JNETSteam[12]==1);
    assert(original_calls==11 && JNETSteam[2]==11);
    jnet_steam_owner_enter=owner_enter;jnet_steam_owner_leave=owner_leave;
    jnet_steam_callbacks();assert(owner_enters==1 && owner_leaves==1);
    jnet_steam_probing=1;jnet_steam_callbacks();assert(owner_enters==2 && owner_leaves==2);
    jnet_steam_probing=0;jnet_steam_owner_enter=jnet_steam_owner_leave=NULL;
    puts("Steam probe: original callback preserved, bounded polling/initialization, identity/context checks, service pending/ready/failure, executable binding guard and bounded diagnostic text passed; live Steam services not exercised.");
    return 0;
}
