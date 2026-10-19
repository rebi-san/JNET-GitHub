
#include <assert.h>
#include <stdint.h>
#include <wchar.h>
#include "../../diagnostic/native_connection_scan.h"
typedef void *HANDLE;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint64_t ULONGLONG;
#define WAIT_TIMEOUT 258
#define JC_FRAMES_RVA 8
#define JC_GAME_RVA 16
static uint64_t ticks=100,game_value=0x100000;
static unsigned frame_value=1,alive=1,io_fail=0,flush_fail=0,write_calls=0,scan_calls=0,deep_calls=0,connected_value=0,mutate_game=0,mutate_frame=0,budget_exhaust=0;
static ULONGLONG GetTickCount64(void) { return ticks; }
static DWORD WaitForSingleObject(HANDLE p,DWORD ms) { (void)p; (void)ms; return alive ? WAIT_TIMEOUT : 0; }
static int remote_read(HANDLE p,uintptr_t address,void *out,size_t n) {
    (void)p;
    if(address==JC_FRAMES_RVA && n==4) { memcpy(out,&frame_value,4); return 1; }
    if(address==JC_GAME_RVA && n==8) { memcpy(out,&game_value,8); return 1; }
    return 0;
}
static int WriteFile(HANDLE p,const void *text,DWORD size,DWORD *written,void *overlapped) {
    (void)p;(void)text;(void)overlapped; ++write_calls; *written=io_fail ? size-1 : size; return 1;
}
static int FlushFileBuffers(HANDLE p) { (void)p; return !flush_fail; }
static int scan_fixture(jc_layout_reader reader,void *ctx,uint64_t image,uint64_t game,int deep,unsigned role,struct jc_connection_snapshot *out) {
    (void)reader;(void)ctx;(void)image;(void)game;(void)role;
    ++scan_calls; deep_calls+=deep; memset(out,0,sizeof(*out)); out->barn=0x200000; out->connected=connected_value;out->remote_gate=connected_value;
    out->dude[0].code=out->dude[1].code=1;
    if(mutate_game) ++game_value;
    if(mutate_frame) ++frame_value;
    if(budget_exhaust) ticks+=76;
    return 0;
}
#define jc_connection_read scan_fixture
static const wchar_t *level_label(int level) { (void)level; return L"Level_Desert"; }
struct jc_connection_recorder {
    HANDLE process,file;
    uintptr_t base,image;
    ULONGLONG started,deadline,read_deadline;
    unsigned reads,rows,good,bad,bytes,last_ms,peak_ms,connects,disconnects,gate_changes,spine_changes[2],spine_have[2];
    unsigned last_connected,last_gate,have,frame,have_frame,deep_tick,stalled,spine_samples[2],shout_samples,cloth_samples;
    unsigned cloth_attempts,cloth_last_stage;
    int cloth_last_code,solo_test;
    uint64_t barn;
    float spine[2][16];
    int active,error,budget;
    struct jc_connection_snapshot sample;
    wchar_t filename[160],summary[3000];
};
static int connection_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct jc_connection_recorder *r=opaque;
    if (r->reads>=512 || GetTickCount64()>=r->read_deadline) { r->budget=1; return 0; }
    ++r->reads;
    return remote_read(r->process,(uintptr_t)address,out,size);
}
static void connection_summary(struct jc_connection_recorder *r) {
    const struct jc_connection_snapshot *s=&r->sample;
    swprintf(r->summary,3000,L"CONNECTION SCAN - Scarfhelper 0.74\r\n%ls. Error: %d. Elapsed: %llu seconds. Limit: %u seconds.\r\n\r\n"
        L"Recorded samples: %u. Accepted base snapshots: %u. Declined: %u.\r\n"
        L"Native connected: %ls. Observed connect/disconnect edges: %u / %u. Remote gate changes: %u.\r\n"
        L"Game updates: %u. Update counter unchanged: %ls. Chapter: %ls.\r\n"
        L"Read time last/peak: %u / %u ms. Last reads: %u. Read budget reached: %d.\r\n"
        L"Valid relative-spine samples local/remote: %u / %u. Changes: %u / %u.\r\n"
        L"Shout samples: %u. Cloth shader samples: %u / %u attempts. Last cloth code/stage: %d / %u.\r\n"
        L"Cloth codes: 0 accepted; 1 read; 2 name absent; 3 changed; 4 bounds. Stages: 0 accepted; 1 material; 2 instance; 3 table; 4 entry; 5 scalar; 6 recheck. Partial/changed snapshots are labeled in the log.\r\n\r\n"
        L"Automatic log beside Scarfhelper:\r\n%ls\r\n\r\n"
        L"%ls\r\n\r\n"
        L"No Capacity Test, Prepare Trial, invite or third game is needed. Keep this window open. If background gameplay pauses, return to Journey so its state advances. Only the game-update capture hook is installed; all snapshot reads and log writes run in Scarfhelper. Native matchmaking is unchanged. Restart Journey before a ghost-room test.\r\n"
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
    if (!length || length>32000 || r->bytes>32u*1024u*1024u-length) { r->error=3; r->active=0; return 0; }
    if (!WriteFile(r->file,text,(DWORD)length,&written,NULL) || written!=length) { r->error=2; r->active=0; return 0; }
    r->bytes+=(unsigned)length; return 1;
}
static void connection_sample(struct jc_connection_recorder *r) {
    ULONGLONG now=GetTickCount64();
    if (!r->active) return;
    if (now>=r->deadline || r->rows>=4800) { r->active=0; return; }
    if (WaitForSingleObject(r->process,0)!=WAIT_TIMEOUT) { r->active=0; return; }
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
static struct jc_connection_recorder fresh(void) {
    struct jc_connection_recorder r={0}; r.active=1; r.started=ticks; r.deadline=ticks+1200000; return r;
}
int main(void) {
    struct jc_connection_recorder r=fresh(); connection_sample(&r);
    assert(r.rows==1 && r.good==1 && scan_calls==1 && deep_calls==1 && write_calls==1);
    connection_sample(&r); assert(r.rows==1 && r.stalled && scan_calls==1);
    connected_value=1; ++frame_value; connection_sample(&r); assert(r.connects==1 && r.gate_changes==1);
    connected_value=0; ++frame_value; connection_sample(&r); assert(r.disconnects==1 && r.gate_changes==2);
    ++frame_value; connection_sample(&r); ++frame_value; connection_sample(&r); assert(deep_calls==2);
    mutate_game=1; ++frame_value; connection_sample(&r); assert(r.sample.code==3 && r.bad==1); mutate_game=0;
    mutate_frame=1; ++frame_value; connection_sample(&r); assert(!r.sample.code); mutate_frame=0;
    budget_exhaust=1; ++frame_value; connection_sample(&r); assert(r.sample.code==9 && r.active && r.budget); budget_exhaust=0;
    io_fail=1; ++frame_value; connection_sample(&r); assert(!r.active && r.error==2); io_fail=0;
    r=fresh(); r.bytes=32u*1024u*1024u; connection_sample(&r); assert(!r.active && r.error==3);
    r=fresh(); flush_fail=1; connection_sample(&r); assert(!r.active && r.error==2); flush_fail=0;
    r=fresh(); r.rows=4800; connection_sample(&r); assert(!r.active);
    r=fresh(); ticks=r.deadline; connection_sample(&r); assert(!r.active);
    r=fresh(); alive=0; connection_sample(&r); assert(!r.active && !r.rows && !r.error);
    r.solo_test=1; connection_summary(&r); assert(wcslen(r.summary)<3000 && wcsstr(r.summary,L"SOLO CLOTH CHECK"));
    puts("Recorder production functions: pause skip, deep cadence, connection edges, torn game pointer, advancing frames, read deadline, short write, flush failure, file/sample/time limits and process exit passed. APIs simulated; Journey not launched.");
}
