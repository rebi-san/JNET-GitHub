#ifndef JNET_STEAM_MAINTENANCE_H
#define JNET_STEAM_MAINTENANCE_H
struct jnet_maintenance_snapshot {
    uint64_t room,generation,host,identity,started,finished,ids[JNET_STEAM_CAPACITY];
    LONG membership_event,data_event;
    char nonce[33];
    unsigned count,daynight;
    int valid;
};
static struct {
    SRWLOCK lock;
    struct jnet_maintenance_snapshot request,result;
    unsigned pending,ready;
} jnet_maintenance={.lock=SRWLOCK_INIT};
static struct jnet_maintenance_snapshot jnet_maintenance_cache;
static uint64_t jnet_maintenance_next;
static int jnet_maintenance_same(const struct jnet_maintenance_snapshot *a,const struct jnet_maintenance_snapshot *b) {
    return a->room==b->room && a->generation==b->generation && a->host==b->host && a->identity==b->identity &&
        a->membership_event==b->membership_event && a->data_event==b->data_event && !strcmp(a->nonce,b->nonce);
}
static void jnet_maintenance_read(struct jnet_maintenance_snapshot *job,struct jnet_maintenance_snapshot *metadata) {
    job->started=GetTickCount64();job->valid=0;job->count=0;
    if(!metadata->valid || !jnet_maintenance_same(job,metadata) || job->started<metadata->started || job->started-metadata->started>=2000) {
        *metadata=*job;
        metadata->valid=!jnet_sl_metadata_check(job->room,job->host,job->nonce);
        metadata->daynight=metadata->valid?jnet_sl_lighting(job->room):0;
    }
    if(!metadata->valid)goto done;
    job->daynight=metadata->daynight;
    if(jnet_sl_api.owner(jnet_sl_api.mm,job->room)!=job->host)goto done;
    int count=jnet_sl_api.members(jnet_sl_api.mm,job->room);
    if(count<1 || count>JNET_STEAM_CAPACITY)goto done;
    int local=0,host=0;
    for(int i=0;i<count;++i) {
        uint64_t id=jnet_sl_api.member(jnet_sl_api.mm,job->room,i);
        if(!id)goto done;
        for(int j=0;j<i;++j)if(job->ids[j]==id)goto done;
        job->ids[i]=id;local|=id==job->identity;host|=id==job->host;
    }
    job->count=(unsigned)count;job->valid=local && host;
done:
    job->finished=GetTickCount64();
}
static void (*jnet_maintenance_stop_transport)(void);
#ifndef JNET_MAINTENANCE_FIXTURE
static HANDLE jnet_maintenance_event,jnet_maintenance_thread;
static volatile LONG jnet_maintenance_stopping;
static SRWLOCK jnet_maintenance_lifetime=SRWLOCK_INIT;
static void (*jnet_maintenance_release)(void),(*jnet_maintenance_original_shutdown)(void);
static void jnet_maintenance_shutdown(void) {
    AcquireSRWLockExclusive(&jnet_maintenance_lifetime);
    InterlockedExchange(&jnet_maintenance_stopping,1);
    if(jnet_maintenance_stop_transport)jnet_maintenance_stop_transport();
    if(jnet_maintenance_event)SetEvent(jnet_maintenance_event);
    if(jnet_maintenance_thread) {
        WaitForSingleObject(jnet_maintenance_thread,INFINITE);
        CloseHandle(jnet_maintenance_thread);jnet_maintenance_thread=NULL;
    }
    ReleaseSRWLockExclusive(&jnet_maintenance_lifetime);
    jnet_maintenance_original_shutdown();
}
static int jnet_maintenance_start(void);
static DWORD WINAPI jnet_maintenance_worker(void *unused) {
    (void)unused;struct jnet_maintenance_snapshot metadata={0};
    for(;;) {
        if(WaitForSingleObject(jnet_maintenance_event,INFINITE)!=WAIT_OBJECT_0)return 1;
        if(InterlockedCompareExchange(&jnet_maintenance_stopping,0,0))return 0;
        struct jnet_maintenance_snapshot job;
        AcquireSRWLockExclusive(&jnet_maintenance.lock);job=jnet_maintenance.request;ReleaseSRWLockExclusive(&jnet_maintenance.lock);
        jnet_maintenance_read(&job,&metadata);
        jnet_maintenance_release();
        AcquireSRWLockExclusive(&jnet_maintenance.lock);
        jnet_maintenance.result=job;jnet_maintenance.ready=1;jnet_maintenance.pending=0;
        ReleaseSRWLockExclusive(&jnet_maintenance.lock);
    }
}
static int jnet_maintenance_start_inner(void) {
    static int state;
    if(InterlockedCompareExchange(&jnet_maintenance_stopping,0,0))return 0;
    if(state)return state>0;
    HMODULE pinned=NULL;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCWSTR)(uintptr_t)jnet_maintenance_worker,&pinned)) {state=-1;return 0;}
    HMODULE steam=GetModuleHandleW(L"steam_api64.dll");
    void *shutdown=steam?(void *)GetProcAddress(steam,"SteamAPI_Shutdown"):NULL;
    jnet_maintenance_release=steam?(void *)GetProcAddress(steam,"SteamAPI_ReleaseCurrentThreadMemory"):NULL;
    if(!shutdown || !jnet_maintenance_release){state=-1;return 0;}
    jnet_maintenance_event=CreateEventW(NULL,FALSE,FALSE,NULL);
    if(!jnet_maintenance_event){state=-1;return 0;}
    if(MH_CreateHook(shutdown,(void *)jnet_maintenance_shutdown,(void **)&jnet_maintenance_original_shutdown)!=MH_OK) {
        CloseHandle(jnet_maintenance_event);jnet_maintenance_event=NULL;state=-1;return 0;
    }
    if(MH_EnableHook(shutdown)!=MH_OK) {
        MH_RemoveHook(shutdown);CloseHandle(jnet_maintenance_event);jnet_maintenance_event=NULL;state=-1;return 0;
    }
    jnet_maintenance_thread=CreateThread(NULL,0,jnet_maintenance_worker,NULL,0,NULL);
    if(!jnet_maintenance_thread){state=-1;return 0;}
    state=1;return 1;
}
static int jnet_maintenance_start(void) {
    AcquireSRWLockExclusive(&jnet_maintenance_lifetime);
    int ready=jnet_maintenance_start_inner();
    ReleaseSRWLockExclusive(&jnet_maintenance_lifetime);return ready;
}
static void jnet_maintenance_wake(void){SetEvent(jnet_maintenance_event);}
#endif
static int jnet_maintenance_poll(unsigned *population) {
    if(!jnet_maintenance_start()){JNETSteamLobby[50]=-1;return 0;}
    JNETSteamLobby[50]=1;
    uint64_t now=GetTickCount64();
    struct jnet_maintenance_snapshot key={.room=jnet_sl.room,.generation=jnet_sl.generation,.host=jnet_sl.host,.identity=jnet_sl.identity,
        .membership_event=JNETSteamP2P[16],.data_event=JNETSteamLobby[28]};
    memcpy(key.nonce,jnet_sl.nonce,33);
    unsigned wake=0;
    AcquireSRWLockExclusive(&jnet_maintenance.lock);
    if(jnet_maintenance.ready) {
        struct jnet_maintenance_snapshot result=jnet_maintenance.result;jnet_maintenance.ready=0;
        if(jnet_maintenance_same(&key,&result) && now>=result.started && now-result.started<500) {
            jnet_maintenance_cache=result;++JNETSteamLobby[53];
            JNETSteamLobby[51]=(LONG)(result.finished-result.started);
            if(JNETSteamLobby[51]>JNETSteamLobby[52])JNETSteamLobby[52]=JNETSteamLobby[51];
        } else ++JNETSteamLobby[55];
    }
    if(!jnet_maintenance.pending && (now>=jnet_maintenance_next || !jnet_maintenance_same(&key,&jnet_maintenance_cache))) {
        jnet_maintenance.request=key;jnet_maintenance.pending=1;jnet_maintenance_next=now+250;wake=1;
    }
    ReleaseSRWLockExclusive(&jnet_maintenance.lock);
    if(wake)jnet_maintenance_wake();
    if(!jnet_maintenance_same(&key,&jnet_maintenance_cache) || now<jnet_maintenance_cache.started || now-jnet_maintenance_cache.started>=500 || jnet_sn_members_dirty || jnet_sl_metadata_dirty) {
        ++JNETSteamLobby[54];return 0;
    }
    *population=jnet_maintenance_cache.count;
    if(!jnet_maintenance_cache.valid){++JNETSteamLobby[57];return -1;}
    jnet_daynight_host=key.host!=key.identity?jnet_maintenance_cache.daynight:~0u;
    jnet_sn_members.room=key.room;jnet_sn_members.generation=key.generation;jnet_sn_members.loaded=1;
    jnet_sn_members.count=*population;jnet_sn_members.expires=jnet_maintenance_cache.started+500;
    memcpy(jnet_sn_members.ids,jnet_maintenance_cache.ids,sizeof(jnet_sn_members.ids));
    ++JNETSteamLobby[56];return 1;
}
#endif
