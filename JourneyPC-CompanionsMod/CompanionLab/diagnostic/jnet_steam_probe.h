#ifndef JNET_STEAM_PROBE_H
#define JNET_STEAM_PROBE_H
__declspec(dllexport) volatile LONG JNETSteam[24];
__declspec(dllexport) char JNETSteamServiceText[2][256];
struct jnet_steam_relay_status { int32_t availability,pinging,configuration,any_relay;char text[256]; };
struct jnet_steam_auth_status { int32_t availability;char text[256]; };
_Static_assert(sizeof(struct jnet_steam_relay_status)==272,"Steam relay status ABI");
_Static_assert(sizeof(struct jnet_steam_auth_status)==260,"Steam authentication status ABI");
static struct {
    void *sockets,*utils;
    uint64_t identity,started;
    int pipe,user;
    unsigned initialized;
} jnet_steam_service;
static void (*jnet_steam_original_callbacks)(void);
static void (*jnet_steam_session_poll)(void);
static void (*jnet_steam_owner_enter)(void),(*jnet_steam_owner_leave)(void);
static volatile LONG jnet_steam_probing;
static uint64_t jnet_steam_next_probe;
static void *jnet_steam_method(void *object,unsigned index) {
    void **table=NULL,*method=NULL;SIZE_T copied=0;
    if(!object || !ReadProcessMemory(GetCurrentProcess(),object,&table,sizeof(table),&copied) || copied!=sizeof(table) || !table)return NULL;
    if(!ReadProcessMemory(GetCurrentProcess(),table+index,&method,sizeof(method),&copied) || copied!=sizeof(method) || !method)return NULL;
    MEMORY_BASIC_INFORMATION info;
    if(!VirtualQuery(method,&info,sizeof(info)) || info.State!=MEM_COMMIT ||
       (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
       !(info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return NULL;
    return method;
}
static void jnet_steam_status_text(char destination[256],const char source[256]) {
    unsigned i=0;
    for(;i<255 && source[i];++i)destination[i]=(unsigned char)source[i]>=32 && (unsigned char)source[i]<127?source[i]:' ';
    destination[i]=0;
}
static void jnet_steam_services(void *sockets,void *utils,uint64_t identity,int user,int pipe) {
    JNETSteam[12]=1;
    int (*relay_status)(void *,struct jnet_steam_relay_status *)=jnet_steam_method(utils,1);
    unsigned char (*init_relay)(void *,float)=jnet_steam_method(utils,7);
    int (*init_auth)(void *)=jnet_steam_method(sockets,22);
    int (*auth_status)(void *,struct jnet_steam_auth_status *)=jnet_steam_method(sockets,23);
    if(!relay_status || !init_relay || !init_auth || !auth_status){JNETSteam[13]=-1;return;}
    if(!jnet_steam_service.initialized || jnet_steam_service.sockets!=sockets || jnet_steam_service.utils!=utils ||
       jnet_steam_service.identity!=identity || jnet_steam_service.user!=user || jnet_steam_service.pipe!=pipe) {
        jnet_steam_service.sockets=sockets;jnet_steam_service.utils=utils;jnet_steam_service.identity=identity;
        jnet_steam_service.user=user;jnet_steam_service.pipe=pipe;jnet_steam_service.started=GetTickCount64();
        JNETSteam[12]=2;
        init_relay(utils,1e10f);
        JNETSteam[12]=3;
        JNETSteam[18]=init_auth(sockets);
        jnet_steam_service.initialized=1;
        InterlockedIncrement(&JNETSteam[19]);
    }
    struct jnet_steam_relay_status relay={0};struct jnet_steam_auth_status auth={0};
    JNETSteam[12]=4;JNETSteam[14]=relay_status(utils,&relay);
    JNETSteam[15]=relay.configuration;JNETSteam[16]=relay.any_relay;JNETSteam[17]=relay.pinging;
    JNETSteam[12]=5;JNETSteam[18]=auth_status(sockets,&auth);
    jnet_steam_status_text(JNETSteamServiceText[0],relay.text);
    jnet_steam_status_text(JNETSteamServiceText[1],auth.text);
    uint64_t elapsed=GetTickCount64()-jnet_steam_service.started;
    JNETSteam[20]=(LONG)(elapsed>INT32_MAX?INT32_MAX:elapsed);
    LONG previous=JNETSteam[13];
    JNETSteam[13]=JNETSteam[14]==100 && JNETSteam[18]==100?1:
        (JNETSteam[14]<0 || JNETSteam[18]<0)?3:2;
    JNETSteam[12]=6;
    if(previous!=JNETSteam[13]) {
        char line[768];snprintf(line,sizeof(line),
            "[JNET] Steam services result=%ld relay/config/router/auth=%ld/%ld/%ld/%ld; relay: %.255s; auth: %.255s",
            (long)JNETSteam[13],(long)JNETSteam[14],(long)JNETSteam[15],(long)JNETSteam[16],(long)JNETSteam[18],
            JNETSteamServiceText[0],JNETSteamServiceText[1]);message(line);
    }
}
static void jnet_steam_probe_sample(LONG *sample) {
    JNETSteam[12]=0;
    HMODULE module=GetModuleHandleW(L"steam_api64.dll");
    if(!module){sample[0]=-1;return;}
    void *(*client_fn)(void)=(void *)GetProcAddress(module,"SteamClient");
    int (*user_fn)(void)=(void *)GetProcAddress(module,"SteamAPI_GetHSteamUser");
    int (*pipe_fn)(void)=(void *)GetProcAddress(module,"SteamAPI_GetHSteamPipe");
    void *(*get_user)(void *,int,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamUser");
    void *(*get_utils)(void *,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamUtils");
    void *(*get_matchmaking)(void *,int,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamMatchmaking");
    void *(*get_generic)(void *,int,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamGenericInterface");
    uint64_t (*get_id)(void *)=(void *)GetProcAddress(module,"SteamAPI_ISteamUser_GetSteamID");
    unsigned char (*logged_on)(void *)=(void *)GetProcAddress(module,"SteamAPI_ISteamUser_BLoggedOn");
    uint32_t (*get_app)(void *)=(void *)GetProcAddress(module,"SteamAPI_ISteamUtils_GetAppID");
    if(!client_fn || !user_fn || !pipe_fn || !get_user || !get_utils || !get_matchmaking || !get_generic || !get_id || !logged_on || !get_app) {
        sample[0]=-3;return;
    }
    int user=user_fn(),pipe=pipe_fn();
    sample[3]=user!=0;sample[4]=pipe!=0;
    if(!user || !pipe){sample[0]=2;return;}
    void *client=client_fn();if(!client){sample[0]=2;return;}
    void *user_interface=get_user(client,user,pipe,"SteamUser019");
    void *utils=get_utils(client,pipe,"SteamUtils008");
    if(!user_interface || !utils){sample[0]=-4;return;}
    uint64_t identity=get_id(user_interface);
    sample[5]=(LONG)get_app(utils);sample[6]=logged_on(user_interface)!=0;
    sample[7]=(LONG)(uint32_t)identity;sample[8]=(LONG)(uint32_t)(identity>>32);
    if(sample[5]!=638230){sample[0]=-5;return;}
    sample[9]=get_matchmaking(client,user,pipe,"SteamMatchMaking009")!=NULL;
    void *sockets=get_generic(client,user,pipe,"SteamNetworkingSockets012");
    void *network_utils=get_generic(client,user,pipe,"SteamNetworkingUtils004");
    sample[10]=sockets!=NULL;sample[11]=network_utils!=NULL;
    sample[0]=!sample[6]?3:!identity?-6:
        (!sample[9] || !sample[10] || !sample[11])?4:5;
    if(sample[0]==5)jnet_steam_services(sockets,network_utils,identity,user,pipe);
}
static void jnet_steam_probe(void) {
    LONG sample[12]={0};
    sample[1]=JNETSteam[1];sample[2]=JNETSteam[2];
    jnet_steam_probe_sample(sample);
    for(unsigned i=3;i<=11;++i)JNETSteam[i]=sample[i];
    JNETSteam[0]=sample[0];
}
static void jnet_steam_callbacks(void) {
    if(jnet_steam_owner_enter)jnet_steam_owner_enter();
    jnet_steam_original_callbacks();
    InterlockedIncrement(&JNETSteam[2]);
    if(InterlockedCompareExchange(&jnet_steam_probing,1,0)){if(jnet_steam_owner_leave)jnet_steam_owner_leave();return;}
    uint64_t now=GetTickCount64();
    if(now>=jnet_steam_next_probe) {
        jnet_steam_next_probe=now+2000;
        LONG before=JNETSteam[0];
        jnet_steam_probe();
        if(before!=JNETSteam[0]) {
            char line[256];snprintf(line,sizeof(line),
                "[JNET] Steam capability state=%ld app=%ld online=%ld matchmaking009=%ld sockets012=%ld utils004=%ld; session status is reported separately",
                (long)JNETSteam[0],(long)JNETSteam[5],(long)JNETSteam[6],(long)JNETSteam[9],(long)JNETSteam[10],(long)JNETSteam[11]);
            message(line);
        }
    }
    if(jnet_steam_session_poll)jnet_steam_session_poll();
    InterlockedExchange(&jnet_steam_probing,0);
    if(jnet_steam_owner_leave)jnet_steam_owner_leave();
}
static void jnet_steam_install_probe(void) {
    HMODULE module=GetModuleHandleW(L"steam_api64.dll");
    void *target=module?(void *)GetProcAddress(module,"SteamAPI_RunCallbacks"):NULL;
    if(!target){JNETSteam[0]=-1;return;}
    if(MH_CreateHook(target,(void *)jnet_steam_callbacks,(void **)&jnet_steam_original_callbacks)!=MH_OK) {
        JNETSteam[0]=-2;return;
    }
    JNETSteam[0]=1;JNETSteam[1]=1;
}
#endif
