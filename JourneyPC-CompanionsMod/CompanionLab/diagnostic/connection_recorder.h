#ifndef JC_CONNECTION_RECORDER_H
#define JC_CONNECTION_RECORDER_H
#include "native_connection_scan.h"
#include "native_lifecycle_trace.h"
/* Included by the helper after its window-position helpers. No native game calls. */
struct jc_connection_recorder {
    HANDLE process,file;
    uintptr_t base,image;
    ULONGLONG started,deadline,read_deadline;
    unsigned reads,rows,good,bad,bytes,last_ms,peak_ms,connects,disconnects,gate_changes,spine_changes[2],spine_have[2];
    unsigned last_connected,last_gate,have,frame,have_frame,deep_tick,stalled,spine_samples[2],shout_samples,cloth_samples;
    unsigned cloth_attempts,cloth_last_stage;
    int cloth_last_code,solo_test,lifecycle;
    unsigned trace_cursor,trace_events,trace_lost,trace_dropped;
    uint64_t barn;
    float spine[2][16];
    int active,error,budget;
    struct jc_connection_snapshot sample;
    wchar_t filename[160],summary[3000];
};
static struct jc_connection_recorder connection_recorder;
static struct jc_lifecycle_ring lifecycle_copies[2];
static int connection_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct jc_connection_recorder *r=opaque;
    if (r->reads>=512 || GetTickCount64()>=r->read_deadline) { r->budget=1; return 0; }
    ++r->reads;
    return remote_read(r->process,(uintptr_t)address,out,size);
}
static void connection_summary(struct jc_connection_recorder *r) {
    if (r->lifecycle) {
        swprintf(r->summary,3000,L"NATIVE LIFECYCLE - JNET 0.93\r\n%ls. Error: %d. Elapsed: %llu seconds.\r\n\r\n"
            L"Native entries/frame boundaries saved: %u. Overwritten entries: %u. Observer contention drops: %u.\r\n"
            L"State snapshots: %u accepted / %u declined. Connect/disconnect edges: %u / %u.\r\n"
            L"Automatic log: %ls\r\n\r\n"
            L"Meet a normal companion, then walk, turn, glide and exchange short/charged chirps. Observe their departure for thirty seconds. When solo, use chapter select and return through the Desert portal to capture resource cleanup/reinitialization. Stop and Save before quitting Journey.\r\n\r\n"
            L"This records native calls; it creates no ghosts. Do not use Capacity Test or Prepare Trial in this session. Keep the recorder open and return to Journey. Limits: twenty minutes or 256 MiB.\r\n"
            L"Errors: 1 read/process; 2 file; 3 size limit; 4 timer; 5 recording control.\r\n"
            L"Entry order is not completion order. State snapshots are sampled, not atomic. Missing initialization before attachment cannot be inferred. Loss counters must be considered when analyzing call order. Restart Journey before another mode.",
            r->active ? L"Recording" : L"Recording ended",r->error,(unsigned long long)((GetTickCount64()-r->started)/1000),
            r->trace_events,r->trace_lost,r->trace_dropped,r->good,r->bad,r->connects,r->disconnects,r->filename);
        return;
    }
    const struct jc_connection_snapshot *s=&r->sample;
    swprintf(r->summary,3000,L"CONNECTION SCAN - JNET 0.93\r\n%ls. Error: %d. Elapsed: %llu seconds. Limit: %u seconds.\r\n\r\n"
        L"Recorded samples: %u. Accepted base snapshots: %u. Declined: %u.\r\n"
        L"Native connected: %ls. Observed connect/disconnect edges: %u / %u. Remote gate changes: %u.\r\n"
        L"Game updates: %u. Update counter unchanged: %ls. Chapter: %ls.\r\n"
        L"Read time last/peak: %u / %u ms. Last reads: %u. Read budget reached: %d.\r\n"
        L"Valid relative-spine samples local/remote: %u / %u. Changes: %u / %u.\r\n"
        L"Shout samples: %u. Cloth shader samples: %u / %u attempts. Last cloth code/stage: %d / %u.\r\n"
        L"Cloth codes: 0 accepted; 1 read; 2 name absent; 3 changed; 4 bounds. Stages: 0 accepted; 1 material; 2 instance; 3 table; 4 entry; 5 scalar; 6 recheck. Partial/changed snapshots are labeled in the log.\r\n\r\n"
        L"Automatic log beside JNET:\r\n%ls\r\n\r\n"
        L"%ls\r\n\r\n"
        L"No Capacity Test, Prepare Trial, invite or third game is needed. Keep this window open. If background gameplay pauses, return to Journey so its state advances. Only the game-update capture hook is installed; all snapshot reads and log writes run in JNET. Native matchmaking is unchanged. Restart Journey before a ghost-room test.\r\n"
        L"Error codes: 0 none; 1 read/process; 2 log write/flush; 3 formatting/size; 4 timer.",
        r->active ? L"Recording" : L"Recording ended",r->error,(unsigned long long)((GetTickCount64()-r->started)/1000),r->solo_test ? 30u : 1200u,
        r->rows,r->good,r->bad,!r->have ? L"not sampled" : r->last_connected ? L"yes" : L"no",r->connects,r->disconnects,r->gate_changes,
        r->frame,r->stalled ? L"yes" : L"no",level_label(!r->good || s->code ? -1 : s->level),r->last_ms,r->peak_ms,r->reads,r->budget,
        r->spine_samples[0],r->spine_samples[1],r->spine_changes[0],r->spine_changes[1],r->shout_samples,r->cloth_samples,r->cloth_attempts,r->cloth_last_code,r->cloth_last_stage,r->filename,
        r->solo_test ? L"SOLO CLOTH CHECK: only devishaz is needed. Return to Desert gameplay, move and chirp normally. Recording stops after thirty seconds. Return here, close this report and Copy all. No meeting or disconnect is needed; the prior connection baseline is already captured." :
        L"On devishaz only: record solo for ten seconds, then meet a companion normally. Walk, jump/glide and chirp. After the companion leaves, keep devishaz in gameplay for another thirty seconds, then Stop and Save.");
}
static int connection_log(struct jc_connection_recorder *r,const char *text,size_t length) {
    DWORD written=0;
    if (!length || length>32000 || r->bytes>(r->lifecycle ? 256u : 32u)*1024u*1024u-length) { r->error=3; r->active=0; return 0; }
    if (!WriteFile(r->file,text,(DWORD)length,&written,NULL) || written!=length) { r->error=2; r->active=0; return 0; }
    r->bytes+=(unsigned)length; return 1;
}
static int lifecycle_arm(struct jc_connection_recorder *r,LONG enabled) {
    SIZE_T written=0;
    return WriteProcessMemory(r->process,(void *)(r->base+JC_LIFECYCLE_RVA),&enabled,4,&written) && written==4;
}
static void lifecycle_drain(struct jc_connection_recorder *r) {
    if (!r->lifecycle) return;
    for (unsigned i=0;i<2;++i)
        if (!remote_read(r->process,r->base+JC_LIFECYCLE_RVA,lifecycle_copies+i,sizeof(lifecycle_copies[i]))) {
            r->error=1; r->active=0; return;
        }
    const struct jc_lifecycle_ring *a=lifecycle_copies,*b=lifecycle_copies+1;
    r->trace_dropped=b->dropped;
    if (!b->enabled) r->active=0;
    uint32_t first=jc_lifecycle_first(b->head);
    if (r->trace_cursor+1<first) {
        r->trace_lost+=first-r->trace_cursor-1; r->trace_cursor=first-1;
    }
    char batch[32000]; size_t used=0;
    for (uint32_t n=r->trace_cursor+1;n && n<=a->head;++n) {
        const struct jc_lifecycle_event *e=&a->events[(n-1)%JC_LIFECYCLE_CAP];
        if (!jc_lifecycle_stable(e,&b->events[(n-1)%JC_LIFECYCLE_CAP],n)) break;
        char line[384];
        int length=snprintf(line,sizeof(line),
            "{\"lc\":[%u,%u,%u,%u,%u,%u,%u,%u,\"%llx\",\"%llx\",\"%llx\",\"%llx\"]}\n",
            e->sequence,e->frame,e->tick,e->thread,e->event,e->caller,e->roles,e->xmm1,
            (unsigned long long)e->args[0],(unsigned long long)e->args[1],
            (unsigned long long)e->args[2],(unsigned long long)e->args[3]);
        if (length<=0 || (size_t)length>=sizeof(line)) { r->error=3; r->active=0; return; }
        if (used+(size_t)length>sizeof(batch)) {
            if (!connection_log(r,batch,used)) return;
            used=0;
        }
        memcpy(batch+used,line,(size_t)length); used+=(size_t)length;
        r->trace_cursor=n; ++r->trace_events;
    }
    if (used) connection_log(r,batch,used);
}
static void connection_sample(struct jc_connection_recorder *r) {
    ULONGLONG now=GetTickCount64();
    if (!r->active) return;
    if (now>=r->deadline || r->rows>=4800) { r->active=0; return; }
    if (WaitForSingleObject(r->process,0)!=WAIT_TIMEOUT) { r->active=0; return; }
    lifecycle_drain(r);
    if (!r->active) return;
    LONG before=0,after=0; uint64_t game=0,again=0;
    r->reads=0; r->budget=0; r->read_deadline=now+75;
    if (!connection_read(r,r->base+JC_FRAMES_RVA,&before,4) || !connection_read(r,r->base+JC_GAME_RVA,&game,8)) {
        r->error=1; r->active=0; return;
    }
    r->stalled=r->have_frame && r->frame==(unsigned)before;
    r->frame=(unsigned)before; r->have_frame=1;
    if (r->stalled) return;
    int deep=(r->deep_tick++%4)==0;
    struct jc_connection_snapshot *s=&r->sample;
    s->code=jc_connection_read(connection_read,r,r->image,game,deep,2+(r->deep_tick/4)%4,s);
    if (!connection_read(r,r->base+JC_GAME_RVA,&again,8) || !connection_read(r,r->base+JC_FRAMES_RVA,&after,4)) s->code=1;
    else if (again!=game) s->code=3;
    if (r->budget) s->code=9;
    r->last_ms=(unsigned)(GetTickCount64()-now); if (r->last_ms>r->peak_ms) r->peak_ms=r->last_ms;
    ++r->rows;
    if (s->code) ++r->bad;
    else {
        ++r->good;
        if (r->have) {
            if (r->last_connected!=!!s->connected) { if (s->connected) ++r->connects; else ++r->disconnects; }
            r->gate_changes+=r->last_gate!=(unsigned)s->remote_gate;
        }
        if (r->barn!=s->barn) { r->spine_have[0]=r->spine_have[1]=0; r->barn=s->barn; }
        r->last_connected=!!s->connected; r->last_gate=(unsigned)s->remote_gate; r->have=1;
        r->shout_samples+=s->shout_code==0; r->cloth_samples+=s->cloth_code==0;
        if (deep && s->cloth_code>=0) { ++r->cloth_attempts; r->cloth_last_code=s->cloth_code; r->cloth_last_stage=s->cloth_stage; }
        for (unsigned i=0;i<2;++i) if (deep && !s->dude[i].code && !s->dude[i].spine_code && before==after) {
            ++r->spine_samples[i];
            int changed=0;
            for (unsigned j=0;j<16;++j) if (fabsf(r->spine[i][j]-s->dude[i].spine[j])>0.0001f) changed=1;
            if (r->spine_have[i] && changed) ++r->spine_changes[i];
            memcpy(r->spine[i],s->dude[i].spine,sizeof(r->spine[i])); r->spine_have[i]=1;
        }
    }
    char text[32000];
    size_t length=jc_connection_format(text,sizeof(text),s,now-r->started,(unsigned)before,(unsigned)after,r->last_ms,r->reads,deep);
    if (!connection_log(r,text,length)) return;
    if (deep && !FlushFileBuffers(r->file)) { r->error=2; r->active=0; }
}
static INT_PTR CALLBACK connection_dialog(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
    struct jc_connection_recorder *r=&connection_recorder;
    (void)lp;
    if (msg==WM_INITDIALOG) {
        restore_result_position(window); SetWindowTextW(window,r->lifecycle ? L"JNET 0.93 - Native Lifecycle" : L"JNET 0.93 - Connection Scan");
        RECT area; GetClientRect(window,&area);
        connection_summary(r);
        HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",r->summary,WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY,
            12,12,area.right-24,area.bottom-64,window,(HMENU)(INT_PTR)100,NULL,NULL);
        HWND stop=CreateWindowExW(0,L"BUTTON",L"Stop and Save",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            area.right-172,area.bottom-40,160,28,window,(HMENU)(INT_PTR)IDOK,NULL,NULL);
        if (!edit || !stop || !SetTimer(window,2,250,NULL)) { r->error=4; r->active=0; EndDialog(window,0); }
        return TRUE;
    }
    if (msg==WM_TIMER && wp==2) {
        connection_sample(r); connection_summary(r);
        DWORD start=0,end=0; SendDlgItemMessageW(window,100,EM_GETSEL,(WPARAM)&start,(LPARAM)&end);
        if (start==end) SetDlgItemTextW(window,100,r->summary);
        if (!r->active) { KillTimer(window,2); SetDlgItemTextW(window,IDOK,L"Close report"); }
        return TRUE;
    }
    if (msg==WM_CLOSE || (msg==WM_COMMAND && (LOWORD(wp)==IDOK || LOWORD(wp)==IDCANCEL))) {
        r->active=0; EndDialog(window,0); return TRUE;
    }
    if (msg==WM_DESTROY) {
        KillTimer(window,2); RECT position;
        if (!IsIconic(window) && GetWindowRect(window,&position)) { result_position=position; result_position_valid=1; }
    }
    return FALSE;
}
static int connection_record(HANDLE process,uintptr_t base,uintptr_t image,const wchar_t *directory,DWORD pid,int solo_test) {
    struct jc_connection_recorder *r=&connection_recorder; memset(r,0,sizeof(*r));
    r->process=process; r->base=base; r->image=image; r->started=GetTickCount64(); r->solo_test=solo_test==1; r->lifecycle=solo_test==2;
    r->deadline=r->started+(r->solo_test ? 30000 : 1200000); r->cloth_last_code=-1;
    SYSTEMTIME time; GetSystemTime(&time);
    swprintf(r->filename,160,L"jc-connection-%04u%02u%02u-%02u%02u%02u-%lu.jsonl",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,(unsigned long)pid);
    wchar_t path[JC_PATH_CAP];
    int count=swprintf(path,JC_PATH_CAP,L"%ls\\%ls",directory,r->filename);
    if (count<=0 || count>=JC_PATH_CAP) return 0;
    r->file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if (r->file==INVALID_HANDLE_VALUE) return 0;
    r->active=1;
    const char *header="{\"format\":\"jc-native-connection-1\",\"version\":\"0.93\",\"game_sha256\":\"16a7c176b9cce893e0ca72c5d0e4b2e7377c4fabec179f95a5e1331b6c2a788f\",\"hook\":\"see lifecycle header if present\",\"target_hz\":4,\"deep_hz\":1,\"atomic\":false,\"shout_columns\":[\"code\",\"counter\",\"active\",\"local\",\"remote\"],\"cloth_columns\":[\"code\",\"role\",\"dissolve_milli\"],\"dissolve_columns\":[\"target_1e00\",\"render_2000\"]}\n";
    if (connection_log(r,header,strlen(header)) && r->lifecycle) {
        const char *trace_header="{\"lifecycle\":1,\"columns\":[\"sequence\",\"frame\",\"tick_ms_u32\",\"thread\",\"event\",\"caller_rva\",\"argument_roles_nibbles\",\"xmm1_bits\",\"rcx_hex\",\"rdx_hex\",\"r8_hex\",\"r9_hex\"],\"entry_only\":true,\"roles\":[\"other\",\"local\",\"remote\",\"player_barn\",\"game\",\"local_mesh\",\"remote_mesh\",\"local_state\",\"remote_state\"]}\n";
        if (!connection_log(r,trace_header,strlen(trace_header)) ||
            !remote_read(r->process,r->base+JC_LIFECYCLE_RVA+4,&r->trace_cursor,4) || !lifecycle_arm(r,1)) { r->error=5; r->active=0; }
    }
    if (r->active) {
        struct { DLGTEMPLATE dialog; WORD menu,window_class,title; } layout={0};
        layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
        if (!result_position_valid) layout.dialog.style|=DS_CENTER;
        layout.dialog.cx=380; layout.dialog.cy=310;
        if (DialogBoxIndirectParamW(GetModuleHandleW(NULL),&layout.dialog,NULL,connection_dialog,0)==-1) r->error=4;
    }
    r->active=0;
    if (r->lifecycle) {
        if (WaitForSingleObject(process,0)==WAIT_TIMEOUT) {
            if (!lifecycle_arm(r,0)) r->error=5;
            lifecycle_drain(r);
        }
        char loss[192];
        int n=snprintf(loss,sizeof(loss),"{\"lifecycle_end\":true,\"events\":%u,\"overwritten\":%u,\"contention_drops\":%u}\n",r->trace_events,r->trace_lost,r->trace_dropped);
        if (n>0 && (size_t)n<sizeof(loss)) connection_log(r,loss,(size_t)n);
    }
    char end[160]; int n=snprintf(end,sizeof(end),"{\"end\":true,\"samples\":%u,\"error\":%d,\"elapsed_ms\":%llu}\n",r->rows,r->error,(unsigned long long)(GetTickCount64()-r->started));
    if (n>0 && (size_t)n<sizeof(end)) connection_log(r,end,(size_t)n);
    if (!FlushFileBuffers(r->file)) r->error=2;
    CloseHandle(r->file); r->file=INVALID_HANDLE_VALUE;
    connection_summary(r); return !r->error;
}
#endif
