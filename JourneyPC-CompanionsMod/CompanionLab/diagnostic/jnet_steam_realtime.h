#ifndef JNET_STEAM_REALTIME_H
#define JNET_STEAM_REALTIME_H
static unsigned jnet_realtime_enabled=0;
static int jnet_realtime_ready(void) {
    uint64_t now=GetTickCount64();
    if(JNETSteam[0]!=5 || !jnet_sl.room || jnet_sl.kind || jnet_sn.failed ||
        jnet_sn.room!=jnet_sl.room || jnet_sn.generation!=jnet_sl.generation ||
        jnet_sg.room!=jnet_sl.room || jnet_sg.generation!=jnet_sl.generation ||
        !JNETSteamLobby[9] || JNETSteamLobby[10] ||
        jnet_sn_members_dirty || jnet_sl_metadata_dirty || !jnet_sn_members.loaded ||
        jnet_sn_members.room!=jnet_sl.room || jnet_sn_members.generation!=jnet_sl.generation ||
        now>=jnet_sn_members.expires)return 0;
    unsigned admitted=0;
    for(unsigned i=0;i<7;++i)admitted+=jnet_sn.peers[i].connected && jnet_sn.peers[i].admitted;
    return admitted!=0;
}
static int jnet_realtime_flush(void) {
    if(!jnet_realtime_ready())return 0;
    AcquireSRWLockExclusive(&jnet_sg_lock);
    int pending=jnet_sg_mail.outgoing.sequence>jnet_sg.local.sequence || jnet_sg_mail.outgoing_count ||
        (jnet_sg.pending_state && GetTickCount64()>=jnet_sg.next_send);
    ReleaseSRWLockExclusive(&jnet_sg_lock);
    if(!pending)return 0;
    uint64_t started=GetTickCount64();
    jnet_sn_fast_poll=1;
    jnet_sg_tick();
    jnet_sn_fast_poll=0;
    ++JNETSteamGame[99];
    JNETSteamGame[100]=(LONG)(GetTickCount64()-started);
    if(JNETSteamGame[100]>JNETSteamGame[101])JNETSteamGame[101]=JNETSteamGame[100];
    return 1;
}
static int jnet_realtime_service(void) {
    uint64_t now=GetTickCount64();
    if(!jnet_realtime_ready())return 0;
    jnet_sn_fast_poll=1;
    for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].connected && jnet_sn.peers[i].admitted &&
        jnet_sn_member(jnet_sn.peers[i].identity))jnet_sn_receive(i);
    jnet_sg_tick();
    jnet_sn_fast_poll=0;
    ++JNETSteamGame[63];
    JNETSteamGame[66]=(LONG)(GetTickCount64()-now);
    if(JNETSteamGame[66]>JNETSteamGame[67])JNETSteamGame[67]=JNETSteamGame[66];
    return 1;
}
#ifndef JNET_REALTIME_FIXTURE
static SRWLOCK jnet_realtime_owner=SRWLOCK_INIT;
static __declspec(thread) unsigned jnet_realtime_depth;
static HANDLE jnet_realtime_event,jnet_realtime_thread;
static volatile LONG jnet_realtime_stopping;
static void jnet_realtime_enter(void) {
    if(!jnet_realtime_depth) {
        uint64_t started=GetTickCount64();AcquireSRWLockExclusive(&jnet_realtime_owner);
        LONG cost=(LONG)(GetTickCount64()-started);
        JNETSteamGame[70]=cost;if(cost>JNETSteamGame[71])JNETSteamGame[71]=cost;
    }
    ++jnet_realtime_depth;
}
static void jnet_realtime_leave(void) {
    if(!--jnet_realtime_depth)ReleaseSRWLockExclusive(&jnet_realtime_owner);
}
static void jnet_realtime_after_capture(void) {
    if(JNETSteamGame[62]==1) {
        if(JNETSteamLobby[12]>0 && TryAcquireSRWLockExclusive(&jnet_maintenance_lifetime)) {
            if(!jnet_maintenance_stopping && jnet_realtime_event)SetEvent(jnet_realtime_event);
            ReleaseSRWLockExclusive(&jnet_maintenance_lifetime);
        }
        return;
    }
    if(!jnet_realtime_depth) {
        if(!TryAcquireSRWLockExclusive(&jnet_realtime_owner)){InterlockedIncrement(&JNETSteamGame[64]);return;}
        JNETSteamGame[70]=0;
    }
    ++jnet_realtime_depth;
    jnet_realtime_flush();
    jnet_realtime_leave();
}
static DWORD WINAPI jnet_realtime_worker(void *unused) {
    (void)unused;uint64_t last=0;DWORD timeout=8;
    for(;;) {
        DWORD waited=WaitForSingleObject(jnet_realtime_event,timeout);
        if(waited==WAIT_FAILED){InterlockedExchange(&JNETSteamGame[62],-1);return 1;}
        if(InterlockedCompareExchange(&jnet_realtime_stopping,0,0))return 0;
        if(!TryAcquireSRWLockExclusive(&jnet_realtime_owner)){InterlockedIncrement(&JNETSteamGame[64]);continue;}
        uint64_t now=GetTickCount64();
        int serviced=jnet_realtime_service();
        timeout=serviced?8:250;
        if(serviced) {
            if(last){JNETSteamGame[68]=(LONG)(now-last>60000?60000:now-last);if(JNETSteamGame[68]>JNETSteamGame[69])JNETSteamGame[69]=JNETSteamGame[68];}
            last=now;
        } else {++JNETSteamGame[65];last=0;}
        ReleaseSRWLockExclusive(&jnet_realtime_owner);
        if(serviced)jnet_maintenance_release();
    }
}
static void jnet_realtime_stop(void) {
    InterlockedExchange(&jnet_realtime_stopping,1);
    if(jnet_realtime_event)SetEvent(jnet_realtime_event);
    if(jnet_realtime_thread){WaitForSingleObject(jnet_realtime_thread,INFINITE);CloseHandle(jnet_realtime_thread);jnet_realtime_thread=NULL;}
    if(jnet_realtime_event){CloseHandle(jnet_realtime_event);jnet_realtime_event=NULL;}
    JNETSteamGame[62]=0;
}
static void jnet_realtime_poll(void) {
    jnet_sl_tick();
    if(!jnet_realtime_enabled || !jnet_sl.room || JNETSteamLobby[50]!=1 || JNETSteamGame[62])return;
    AcquireSRWLockExclusive(&jnet_maintenance_lifetime);
    if(!jnet_maintenance_stopping && !jnet_realtime_stopping) {
        jnet_maintenance_stop_transport=jnet_realtime_stop;
        jnet_realtime_event=CreateEventW(NULL,FALSE,FALSE,NULL);
        if(jnet_realtime_event)jnet_realtime_thread=CreateThread(NULL,0,jnet_realtime_worker,NULL,0,NULL);
        JNETSteamGame[62]=jnet_realtime_thread?1:-1;
        if(!jnet_realtime_thread && jnet_realtime_event){CloseHandle(jnet_realtime_event);jnet_realtime_event=NULL;}
    }
    ReleaseSRWLockExclusive(&jnet_maintenance_lifetime);
}
#endif
#endif
