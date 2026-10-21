
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef int32_t LONG;
typedef void *HANDLE;
#define JC_HELPER_VERSION L"development"
#define JC_ACTOR_TRIAL_RVA 950
#define JC_ACTOR_RVA 900
#define JC_TRAVELER_RVA 800
#define JC_NATIVE_PLAYBACK_RVA 100
#define JC_COMMAND_RVA 200
#define JC_BODIES_RVA 300
#define JC_COMPANION_FEATURES_RVA 400
#define JC_ROOM_RVA 500
#define JC_NATIVE_OUTPUTS_RVA 600
static LONG playback[48],body[48],features[40],room[20],outputs[16],traveler[44],actor[12],actor_trial[5],pending;
static unsigned watch_poll;
static int remote_read(HANDLE p,uintptr_t at,void *out,size_t size) {
 (void)p;
 if(watch_poll>=2)return 0;
 if(at==950) {assert(size==sizeof(actor_trial));memcpy(out,actor_trial,size);return 1;}
 if(at==900) {assert(size==sizeof(actor));memcpy(out,actor,size);return 1;}
 if(at==800) {assert(size==sizeof(traveler));memcpy(out,traveler,size);return 1;}
 if(at==700) {memset(out,0,size);return 1;}
 if(at==100) {assert(size==sizeof(playback));memcpy(out,playback,size);}
 else if(at==200) {assert(size==sizeof(pending));memcpy(out,&pending,size);}
 else if(at==300) {assert(size==sizeof(body));memcpy(out,body,size);}
 else if(at==400) {assert(size==sizeof(features));memcpy(out,features,size);}
 else if(at==500) {assert(size==sizeof(room));memcpy(out,room,size);}
 else if(at==600) {assert(size==sizeof(outputs));memcpy(out,outputs,size);}
 else assert(0);
 return 1;
}
static int traveler_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG actor[12],trial[5],pending,features[40];
    if (!remote_read(process,base+JC_ACTOR_TRIAL_RVA,trial,sizeof(trial)) ||
        !remote_read(process,base+JC_ACTOR_RVA,actor,sizeof(actor)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (actor[0] || trial[0] || pending==22) {
        if (!remote_read(process,base+JC_COMPANION_FEATURES_RVA,features,sizeof(features))) return -1;
        if (!cap) return 1;
        int n=swprintf(text,cap,L"ACTIVE TRAVELER - Scarfhelper " JC_HELPER_VERSION L"\r\n"
            L"Local diagnostic input; not an online third player.\r\n\r\n"
            L"Trial state/result: %ld / %ld. Elapsed: %ld ms.\r\n"
            L"Trial: 1 active; 2 draining; 3 complete; -1 declined.\r\n"
            L"Actor state/result: %ld / %ld. Pending command: %ld.\r\n"
            L"States: 0 unbound; 1 waiting; 2 active; 3 draining; 4 retired; 5 quarantined.\r\n"
            L"Native input/update/render/finish frame IDs: %ld / %ld / %ld / %ld.\r\n"
            L"Material guard/result rows: %ld / %ld / %ld.\r\n"
            L"Input sequence: %ld. Changed spans: %ld. Publication quarantine: %ld.\r\n"
            L"Reader/check: %ld / %ld (1 protected local; 2 compatibility).\r\n"
            L"Active game updates: %ld. Last/peak update cost: %ld / %ld ms.\r\n"
            L"Cloth prepared/registered: %ld / %ld. Declines/skipped workers: %ld / %ld.\r\n"
            L"Cloth last result/stage/detail: %ld / %ld / %ld.\r\n"
            L"Finish stages: 100 ownership; 101 workers; 102 counters; 103 native finish; 104 unmap; 105 render registration.\r\n"
            L"If quarantined, Copy All then close Journey: this force-exits only Journey (E04A4301).\r\n\r\n"
            L"One extra full native actor follows offset local diagnostic input for sixty seconds. "
            L"Its outfit follows the diagnostic sample; its symbol differs from both native slots. Online participants must supply their own identity.\r\n"
            L"Walk, turn, glide, and exchange short/charged chirps. Check robe, scarf, chest/chirp symbols and responsiveness. "
            L"Stop Link ends activation. After retirement, chapter select tests cleanup; quit normally with this window open.\r\n"
            L"Saved visibility log lines distinguish registration, dispatch, command building and forwarding. Native counts do not prove visible correctness, three-player interactions or Internet transport. Status is sampled, not atomic.",
            (long)trial[0],(long)trial[1],(long)trial[2],(long)actor[0],(long)actor[1],(long)pending,(long)actor[2],(long)actor[3],(long)actor[4],(long)actor[8],
            (long)actor[9],(long)actor[10],(long)actor[11],
            (long)actor[5],(long)actor[6],(long)actor[7],
            (long)features[8],(long)features[9],(long)features[39],(long)features[37],(long)features[38],
            (long)features[35],(long)features[36],(long)features[30],(long)features[31],
            (long)features[32],(long)features[33],(long)features[34]);
        if(n<0 || (size_t)n>=cap) text[cap-1]=0;
        return 1;
    }
    LONG r[44];
    if (!remote_read(process,base+JC_TRAVELER_RVA,r,sizeof(r)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!r[0] && !r[19] && pending!=21) return 0;
    if (!cap) return 1;
    const wchar_t *state=r[19] ? L"Waiting for normal Desert initialization; enter Desert" : pending==21 ? L"Queued; return to Journey" : r[0]==1 ? L"Armed; chapter select, then Desert portal" :
        r[0]==2 ? L"Native operation in progress" : r[0]==3 ? L"Full object retained; select Active Traveler" :
        r[0]==4 ? L"Native cleanup and deletion completed" : r[0]==-2 ? L"Quarantined; restart Journey" : L"Declined; restart Journey";
    int n=swprintf(text,cap,
        L"TRAVELER LIFECYCLE - Scarfhelper " JC_HELPER_VERSION L"\r\n%ls.\r\n\r\n"
        L"State / stage / result: %ld / %ld / %ld. Pending: %ld.\r\n"
        L"Context detail / guard detail: %ld / %ld. Initial geometry slots/occupied/free: %ld / %ld / %ld.\r\n"
        L"Presence snapshot: layout %ld; flags a2/a3/a4 %ld/%ld/%ld; blocked %ld; chapters %ld/%ld; native present %ld; transport %ld (-1 unavailable).\r\n"
        L"Native preparation calls: %ld. Controller points to own mesh before/after: %ld / %ld (-1 unread).\r\n"
        L"Full initialization / cleanup / deletion calls: %ld / %ld / %ld. Calls alone do not prove successful ownership.\r\n"
        L"Glyph pool count before/after: %ld / %ld. Distinct sound handles: %ld.\r\n"
        L"Separate clone / pose / animation storage: %ld / %ld / %ld. Callback check / cleanup restoration: %ld / %ld.\r\n\r\n"
        L"Callback scan/result %ld / %ld; originals before %ld/%ld, after %ld/%ld; extra %ld; shared resource %ld. Detail: 1 scan; 2 originals changed; 3 source; 4 subscriber mismatch.\r\n"
        L"Geometry cleanup: occupied before/after %ld/%ld; code %ld; phase %ld (1 capture, 2 release check).\r\n"
        L"This creates one dormant, fully initialized extra Dude using Journey's complete initializer. It does not draw a third player or run its simulation. No outfit is copied from the host.\r\n"
        L"Stages: 1 arm; 2 context/capacity; 3 factory; 4 full initializer; 5 ownership; 6 cleanup; 7 restoration; 8 deleted; 9 native preparation (between factory and full initializer).\r\n"
        L"Results: 1 read; 2 type/controller; 3 changed; 4 bounds; 5 glyph; 6 table overlap; 7 sound alias; 8 context/capacity; 9 timeout; 10 factory; 11 mesh callback; 12 clone; 13 pose; 14 animation; 15 callbacks; 16 cleanup fields; 17 geometry occupancy; 18 checkpoint log; 19 preparation/ownership.\r\n\r\n"
        L"Glyph guard detail: 80 hook/state; 81 bounds; 82 read; 83 occupied; 84 anchor; 85 capacity; 86 changed pool; 87 slot; 88 storage; 89 alias; 90 thread; 91 assignment.\r\n"
        L"Context: 23 native companion present; 24 transport connected; 30 layout unreadable/changed (layout: 1 read, 2 type, 3 changed, 4 bounds); 31 transport unreadable.\r\n"
        L"Keep this window open. Select Active Traveler for the new local test. After it retires, chapter select checks cleanup. Quit normally, then Copy All once.\r\n"
        L"Chapter changes test resource setup/teardown only. Online departure and same-level replacement must reuse a resident slot; they must not require chapter select. Full cloth routing, player updates and peer admission are not enabled here. Status snapshots are not atomic.",
        state,(long)r[0],(long)r[1],(long)r[2],(long)pending,
        (long)r[15],(long)r[16],(long)r[17],(long)r[3],(long)r[18],
        (long)r[28],(long)r[20],(long)r[21],(long)r[22],(long)r[23],(long)r[24],(long)r[25],(long)r[26],(long)r[27],
        (long)r[29],(long)r[30],(long)r[31],(long)r[4],(long)r[5],(long)r[6],
        (long)r[7],(long)r[8],(long)r[9],(long)r[10],(long)r[11],(long)r[12],(long)r[13],(long)r[14],
        (long)r[32],(long)r[39],(long)r[33],(long)r[34],(long)r[35],(long)r[36],(long)r[37],(long)r[38],(long)r[40],(long)r[41],(long)r[42],(long)r[43]);
    if (n<0 || (size_t)n>=cap) text[cap-1]=0;
    return 1;
}
static int native_playback_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG playback[48],bodies[48],pending,features[40],room[20],outputs[16];
    if (!remote_read(process,base+JC_NATIVE_PLAYBACK_RVA,playback,sizeof(playback)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!playback[0] && !playback[24] && pending!=20) return 0;
    if (!remote_read(process,base+JC_BODIES_RVA,bodies,sizeof(bodies)) ||
        !remote_read(process,base+JC_COMPANION_FEATURES_RVA,features,sizeof(features)) ||
        !remote_read(process,base+JC_ROOM_RVA,room,sizeof(room)) ||
        !remote_read(process,base+JC_NATIVE_OUTPUTS_RVA,outputs,sizeof(outputs))) return -1;
    int n=swprintf(text,cap,L"NATIVE COMPANIONS - Scarfhelper " JC_HELPER_VERSION L"\r\n%ls.\r\n\r\n"
        L"Two retained bodies, independent native clip evaluation and skin updates. No evaluated-bone copying. Target: up to 10 evaluations/sec/body; inspections costing 8 ms or more yield three times their cost to normal game updates; companion sampling slows accordingly. Inexpensive position following alternates bodies once per game frame during inspection rest.\r\n",
        pending==20 ? L"Queued; return to gameplay" : playback[23] ? L"Room input; three-minute window" : L"Local input; sixty-second window");
    if (n<=0 || (size_t)n>=cap) return -1;
    size_t used=(size_t)n;
    for (unsigned i=0;i<2;++i) {
        const LONG *p=playback+i*24,*b=bodies+i*24;
        const wchar_t *state=p[0]==1 && p[1]==17 ? L"active; allocator retry pending" : p[0]==1 ? L"active/waiting" : p[0]==2 ? L"stopped" : p[0]==3 ? L"complete" :
            p[0]<0 && !p[16] ? L"declined before native submission" : p[0]<0 ? L"stopped after native submission" : L"not selected";
        n=swprintf(text+used,cap-used,L"\r\nBody %u: %ls. Render %ld; draws %ld; render failure %ld.\r\n"
            L"Elapsed %ld ms; accepted %ld; changed-bone samples %ld; accepted/sec %.2f; submissions %ld.\r\n"
            L"Result / stage / detail: %ld / %ld / %ld. Guard ready %ld; postcheck %ld.\r\n"
            L"Evaluation ms last/peak %ld / %ld; reads %ld; active clips %ld. Graph nodes/bindings/clocks %ld / %ld / %ld; scheduler %ld; scratch/temp %ld / %ld bytes.\r\n"
            L"Bind row %ld; stamp rearms %ld; bind code %ld.\r\n"
            L"Position-only updates %ld; last/peak %ld / %ld ms; reads %ld; result %ld.\r\n",
            i+1,state,(long)b[1],(long)b[2],(long)b[3],(long)p[6],(long)p[4],(long)p[5],
            p[6]>0 ? (double)p[4]*1000.0/(double)p[6] : 0.0,(long)p[16],
            (long)p[1],(long)p[2],(long)p[3],(long)p[19],(long)p[18],
            (long)p[7],(long)p[8],(long)p[9],(long)p[17],(long)p[10],(long)p[11],(long)p[12],
            (long)p[13],(long)p[14],(long)p[15],(long)p[20],(long)p[21],(long)p[22],
            (long)features[14+i],(long)features[22+i],(long)features[24+i],(long)features[28+i],(long)features[26+i]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
        if (p[23]) {
            unsigned at=30+i*5;
            n=swprintf(text+used,cap-used,L"Sender appearance: %ls; outfit %ld (red0-3, white4-6); scarf %ld/%ld; symbol %ld. Metadata only; room cloth is disabled.\r\n",
                features[at] ? L"captured" : L"unavailable",(long)features[at+3],
                (long)features[at+1],(long)features[at+2],(long)features[at+4]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
        if (p[2]==2 && p[3]>=1 && p[3]<=7) {
            const wchar_t *checks[]={L"scheduler identity/queue",L"scratch capacity",L"work-unit capacity",L"allocator lock",L"ring layout",L"ring reservation",L"task pool"};
            n=swprintf(text+used,cap-used,L"Scheduler guard: %ls.\r\n",checks[(unsigned)p[3]-1]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
        if (p[2]==9 && p[3]>=4 && p[3]<24) {
            const wchar_t *checks[]={L"counter pointer",L"four-byte alignment",L"counter read",L"live reference count",L"counter storage"};
            const wchar_t *owners[]={L"Local",L"vanilla Remote",L"body 1",L"body 2"};
            n=swprintf(text+used,cap-used,L"Reference guard: %ls; owner %ls.\r\n",
                checks[(unsigned)p[3]/4-1],owners[(unsigned)p[3]%4]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
        if (p[2]==4 && p[3]>=20*1024 && p[3]<29*1024) {
            const wchar_t *checks[]={L"mapping index",L"binding index",L"unsupported output type",
                L"unsupported binding kind",L"destination outside owned skeleton/copy sources",L"destination alignment",
                L"destination not writable",L"no bound outputs",L"model copy table"};
            unsigned check=(unsigned)p[3]/1024,row=(unsigned)p[3]%1024;
            n=swprintf(text+used,cap-used,L"Output guard: %ls; %ls row %u (zero-based).\r\n",
                checks[check-20],check==28 ? L"model owner" : check<23 ? L"mapping" : L"binding",row);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
            const LONG *o=outputs+i*8;
            n=swprintf(text+used,cap-used,L"Output context: items/bindings %lu/%lu; index/binding %lu/%lu; kind %lu; type RVA 0x%08lX; copy bytes %lu; destination owners %lu.\r\n",
                (unsigned long)(uint32_t)o[0],(unsigned long)(uint32_t)o[1],
                (unsigned long)(uint32_t)o[2],(unsigned long)(uint32_t)o[3],
                (unsigned long)(uint32_t)o[4],(unsigned long)(uint32_t)o[5],
                (unsigned long)(uint32_t)o[6],(unsigned long)(uint32_t)o[7]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
    }
    n=swprintf(text+used,cap-used,L"\r\nCloth state/result/stage: %ld / %ld / %ld. Updates %ld; retirements %ld.\r\n"
        L"Cloth capture detail %ld. Reader %ld (2 batched compatibility; 1 protected local; 0 compatibility); isolated reader check %ld (-1 not run; 0 passed; 1-6 failed; 100 launch; 101 timeout; 102 wait; 103 abnormal exit).\r\n"
        L"Cloth original failure stage %ld; call detail %ld; storage code %ld. Inspection rest interval %ld ms.\r\n"
        L"Chirp state/result: %ld / %ld. Captured %ld; body 1 / body 2 queued %ld / %ld; skipped %ld.\r\n"
        L"Room peers %ld; pending command %ld.\r\n\r\n"
        L"Cloth: state 0 off, 1 waiting, 2 live, 3 retired, -1 declined, -2 quarantined. Codes 1 read, 2 identity, 3 changed, 4 busy, 5 native peer/occupied role, 6 capacity, 7 ownership, 8 call, 9 phase, 10 properties, 11 context, 12 definition, 13 finish, 14 body. Stages 1 init, 2 attach, 3 collisions, 4 appearance, 5 release, 6 pool scan, 7 materials, 8 empty handles, 9 properties, 10 atlas resources, 11 scarf appearance. Atlas call detail: 30/31 context; 40-45 resource preflight; 100-156 binding check; 200-256 postcheck. For the latter two, subtract 100/200: tens identify robe base/glow/mask then scarf base/glow/mask; units 1 read, 2 binding, 3 payload, 4 texture, 5 scarf sampler, 6 changed, 0 writable/postcheck.\r\n"
        L"Chirps: state 0 off, 1 listening, 2 queued, 3 stopped, -1 disabled. Codes 1 read, 2 argument, 3 changed, 4 bounds, 5 full, 6 sound, 7 writable, 8 postcheck, 9 native peer.\r\n\r\n"
        L"Result codes: 0 accepted; 1 read; 2 type; 3 changed; 4 bounds; 5 capacity; 6 overlap; 7 pending; 8 context; 9 budget; 10 argument; 11 scheduler; 12 busy; 13 space; 14 clock; 15 graph; 16 writable; 17 allocator retry; 20 lifetime; 21 pose; 22 animation input; 23 binding/queue; 24 submission; 25 postcheck; 26 allocation.\r\n"
        L"Stages: 1 layout; 2 scheduler; 3 clocks; 4 outputs; 5 execution; 6 nodes; 7 disjoint/writable; 8 fresh recheck; 9 reference counters; 100 lifetime; 101 capture; 102 apply; 103 bind; 104 submit; 105 finish; 106 skin; 107 shader; 108 postcheck; 109 position-only update.\r\n"
        L"Bind details: 1 argument; 2 clone; 3 overlap; 4 model/control; 5 frame; 6 fresh check; 7 stamp write; 8 stamp postcheck; 9 native stamp; 10 sample; 11 row read; 12/13 request flags; 14 weight; 15 additive; 16 time; 17 duration; 18 speed; 101 count; 102 queue pair; 103 ref read; 104 ref count.\r\n\r\n"
        L"Stay in Desert. Walk, turn, jump/glide and chirp. Stop Link stops the test. Copy All, then quit Journey normally; restart before another mode. Counts do not prove visible animation. Status snapshots are not atomic.\r\n"
        L"Chirp pulses use native environmental effects: no player glyph or scarf charging. Cloth is a single-body remote-role experiment. Shared triggers, chapter changes and Internet transport are unfinished.",
        (long)features[0],(long)features[1],(long)features[2],(long)features[3],(long)features[4],
        (long)features[7],(long)features[8],(long)features[9],(long)features[10],(long)features[11],(long)features[12],(long)features[13],
        (long)features[16],(long)features[17],(long)features[18],(long)features[19],(long)features[20],(long)features[21],
        (long)room[0],(long)pending);
    return n>0 && (size_t)n<cap-used ? 1 : -1;
}
static void format_exit_report(wchar_t *text,size_t cap,int obtained,uint32_t code,const wchar_t *last) {
    if (!cap) return;
    text[0]=0;
    int n=obtained ? swprintf(text,cap,L"Journey exited. Process exit code: 0x%08lX.\r\n",(unsigned long)code) :
        swprintf(text,cap,L"Journey process unavailable; exit code could not be read.\r\n");
    if (n<0 || (size_t)n>=cap) { text[cap-1]=0; return; }
    size_t used=(size_t)n;
    const wchar_t *parts[]={L"Keep Scarfhelper open briefly while its exit recorder finishes. Native results are saved in jc-exit/jc-probe logs. No crash cause is inferred from the exit code alone.\r\n\r\n",
        last[0] ? L"Last readable live report (sampled before exit):\r\n" : L"No live report was retained in this window; check the logs.\r\n",last};
    for (unsigned i=0;i<3;++i) {
        size_t count=wcslen(parts[i]); if (count>cap-used-1) count=cap-used-1;
        wmemcpy(text+used,parts[i],count); used+=count; text[used]=0;
    }
}

#define WINAPI
#define WAIT_TIMEOUT 258
#define WAIT_OBJECT_0 0
#define JC_PATH_CAP 1024
typedef struct { unsigned low,high; } FILETIME;
#define JC_FLIGHT_FIELDS 18
#define JC_FLIGHT_RVA 700
#define CP_UTF8 65001
typedef uint32_t DWORD;
static char captured_log[50000];static size_t log_used;static unsigned flushes,closes;
static DWORD WaitForSingleObject(HANDLE h,DWORD ms) {(void)h;(void)ms;return ++watch_poll<4 ? WAIT_TIMEOUT : WAIT_OBJECT_0;}
static int GetExitCodeProcess(HANDLE h,DWORD *code) {(void)h;*code=0;return 1;}
static DWORD GetLastError(void) {return 0;}
static int WriteFile(HANDLE h,const void *data,DWORD n,DWORD *written,void *unused) {
 (void)h;(void)unused;assert(n<sizeof(captured_log)-log_used);
 memcpy(captured_log+log_used,data,n);log_used+=n;captured_log[log_used]=0;*written=n;return 1;
}
static int FlushFileBuffers(HANDLE h) {(void)h;++flushes;return 1;}
static int CloseHandle(HANDLE h) {(void)h;++closes;return 1;}
static int WideCharToMultiByte(unsigned cp,unsigned flags,const wchar_t *wide,int size,char *out,int cap,void *a,void *b) {
 (void)flags;(void)a;(void)b;assert(cp==CP_UTF8 && size==-1);size_t n=wcslen(wide)+1;
 if(n>(size_t)cap)return 0;for(size_t i=0;i<n;++i){assert(wide[i]<128);out[i]=(char)wide[i];}return (int)n;
}
struct exit_watch {
    HANDLE process,file,thread;
    uintptr_t base;
    DWORD pid;
    FILETIME created;
    wchar_t directory[JC_PATH_CAP],path[JC_PATH_CAP],last_report[10000];
};
static struct exit_watch exit_watch;
static DWORD WINAPI watch_exit(void *opaque) {
    struct exit_watch *watch=opaque;
    LONG samples[8][JC_FLIGHT_FIELDS]={0}; unsigned captured=0,next=0;
    wchar_t last_report[10000]={0},report[10000];
    DWORD waited;
    while ((waited=WaitForSingleObject(watch->process,100))==WAIT_TIMEOUT) {
        LONG sample[JC_FLIGHT_FIELDS];
        if (remote_read(watch->process,watch->base+JC_FLIGHT_RVA,sample,sizeof(sample))) {
            memcpy(samples[next],sample,sizeof(sample)); next=(next+1)%8;
            if (captured<8) ++captured;
        }
        if (traveler_report(watch->process,watch->base,report,10000)==1 ||
            native_playback_report(watch->process,watch->base,report,10000)==1)
            wcscpy(last_report,report);
    }
    DWORD code=0,error=0,written;
    int obtained=waited==WAIT_OBJECT_0 && GetExitCodeProcess(watch->process,&code);
    if (!obtained) error=GetLastError();
    char line[256];
    int n=snprintf(line,sizeof(line),"[JC-exit] pid=%lu wait=%lu obtained=%d code=%08lx error=%lu samples=%u\r\n",
        (unsigned long)watch->pid,(unsigned long)waited,obtained,(unsigned long)code,(unsigned long)error,captured);
    WriteFile(watch->file,line,(DWORD)n,&written,NULL);
    for (unsigned i=0;i<captured;++i) {
        LONG *v=samples[(next+8-captured+i)%8];
        n=snprintf(line,sizeof(line),"[JC-exit] phase=%ld body=%ld frame=%ld thread=%ld serial=%ld fault=%08lx module=%ld rva=%08lx observer=%ld\r\n",
            (long)v[0],(long)v[1],(long)v[2],(long)v[3],(long)v[4],(unsigned long)(uint32_t)v[5],
            (long)v[6],(unsigned long)(uint32_t)v[7],(long)v[8]);
        WriteFile(watch->file,line,(DWORD)n,&written,NULL);
    }
    if (last_report[0]) {
        const char marker[]="[JC-exit] last readable native report; sampled before exit, not atomic\r\n";
        char utf8[40000];
        int bytes=WideCharToMultiByte(CP_UTF8,0,last_report,-1,utf8,sizeof(utf8),NULL,NULL);
        if (bytes>1) {
            WriteFile(watch->file,marker,sizeof(marker)-1,&written,NULL);
            WriteFile(watch->file,utf8,(DWORD)bytes-1,&written,NULL);
            WriteFile(watch->file,"\r\n",2,&written,NULL);
        }
    }
    wcscpy(watch->last_report,last_report);
    FlushFileBuffers(watch->file);
    CloseHandle(watch->file); CloseHandle(watch->process);
    return 0;
}

int main(void) {
 wchar_t report[10000];
 assert(native_playback_report(0,0,report,10000)==0);
 assert(traveler_report(0,0,report,10000)==0);
 actor[0]=2; actor[1]=44;
 features[8]=1;features[9]=0;features[30]=27;features[32]=8;features[33]=3;features[34]=2;features[35]=12;features[36]=11;features[37]=9;features[38]=17;features[39]=28;
 assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"ACTIVE TRAVELER") && wcsstr(report,L"2 / 44"));
 assert(wcsstr(report,L"frame IDs") && wcsstr(report,L"12 / 11") && wcsstr(report,L"8 / 3 / 2") && wcsstr(report,L"9 / 17 ms"));
 memset(features,0,sizeof(features));
 for(size_t cap=1;cap<500;++cap){report[cap]=L'Z';assert(traveler_report(0,0,report,cap)==1 && report[cap-1]==0 && report[cap]==L'Z');}
 memset(actor,0,sizeof(actor));
 traveler[19]=1;assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"normal Desert initialization"));
 traveler[19]=0;traveler[0]=3;
 assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"Full object retained"));
 for(size_t cap=1;cap<500;++cap) {
   for(size_t j=0;j<10000;++j)report[j]=L'Z';
   assert(traveler_report(0,0,report,cap)==1 && report[cap-1]==0 && report[cap]==L'Z');
 }
 traveler[0]=4;assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"cleanup and deletion completed"));
 traveler[0]=0;
 pending=20;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Queued"));
 pending=0;playback[0]=-1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 1: declined before native submission"));
 playback[16]=1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 1: stopped after native submission"));
 playback[0]=0;playback[24]=-1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 2: declined before native submission"));
 playback[40]=1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 2: stopped after native submission"));
 playback[23]=1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Room input"));
 for(unsigned check=20;check<29;++check) {
  playback[2]=4;playback[3]=(LONG)(check*1024+7);outputs[5]=0x72c720;
  assert(native_playback_report(0,0,report,10000)==1);
  assert(wcsstr(report,L"Output guard:") && wcsstr(report,L"row 7") && wcsstr(report,L"0x0072C720"));
 }
 const wchar_t *checks[]={L"counter pointer",L"four-byte alignment",L"counter read",L"live reference count",L"counter storage"};
 const wchar_t *owners[]={L"Local",L"vanilla Remote",L"body 1",L"body 2"};
 for(unsigned check=1;check<=5;++check)for(unsigned owner=0;owner<4;++owner) {
  playback[2]=9;playback[3]=(LONG)(check*4+owner);
  assert(native_playback_report(0,0,report,10000)==1);
  wchar_t expected[128];swprintf(expected,128,L"Reference guard: %ls; owner %ls.",checks[check-1],owners[owner]);
  assert(wcsstr(report,expected) && !wcsstr(report,L"Output guard:"));
 }
 for(unsigned check=1;check<=7;++check) {
  playback[2]=2;playback[3]=(LONG)check;
  assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"Scheduler guard:"));
 }
 playback[0]=1;playback[1]=17;playback[2]=2;playback[3]=6;
 assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"allocator retry pending") && wcsstr(report,L"ring reservation"));
 playback[2]=0;assert(native_playback_report(0,0,report,10000)==1);assert(!wcsstr(report,L"Output guard:"));
 playback[23]=1;features[30]=1;features[31]=12;features[32]=24;features[33]=6;features[34]=20;
 assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"Sender appearance: captured; outfit 6") && wcsstr(report,L"scarf 12/24; symbol 20"));
 features[30]=0;assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"Sender appearance: unavailable"));
 for(unsigned i=0;i<48;++i)playback[i]=body[i]=INT32_MAX;for(unsigned i=0;i<40;++i)features[i]=INT32_MAX;for(unsigned i=0;i<20;++i)room[i]=INT32_MAX;
 playback[0]=playback[24]=1;
 assert(native_playback_report(0,0,report,10000)==1);size_t maximum=wcslen(report);assert(maximum<6500);assert(wcsstr(report,L"8 fresh recheck") && wcsstr(report,L"Bind details") && wcsstr(report,L"Body 2:"));
 struct {wchar_t before;wchar_t short_text[16];wchar_t after;} small={.before=0x1234,.after=0x5678};
 assert(native_playback_report(0,0,small.short_text,16)==-1);assert(small.before==0x1234 && small.after==0x5678);
 wchar_t exit_report[10000];
 format_exit_report(exit_report,10000,1,0,report);
 assert(wcsstr(exit_report,L"0x00000000") && wcsstr(exit_report,L"NATIVE COMPANIONS") && wcsstr(exit_report,L"Last readable live report"));
 format_exit_report(exit_report,10000,0,0,L"");assert(wcsstr(exit_report,L"No live report was retained"));
 for(size_t cap=1;cap<500;++cap) {
  wchar_t bounded[502];for(unsigned i=0;i<502;++i)bounded[i]=0x1234;
  format_exit_report(bounded,cap,1,0,report);assert(bounded[cap]==0x1234);assert(wmemchr(bounded,0,cap));
 }
 memset(playback,0,sizeof(playback));playback[0]=-1;playback[1]=15;playback[2]=4;playback[3]=22*1024+7;
 outputs[5]=0x72c720;
 watch_exit(&exit_watch);
 assert(strstr(captured_log,"code=00000000") && strstr(captured_log,"last readable native report") &&
  strstr(captured_log,"unsupported output type") && strstr(captured_log,"row 7") && strstr(captured_log,"0x0072C720"));
 assert(flushes==1 && closes==2);
 watch_poll=0;log_used=0;captured_log[0]=0;memset(playback,0,sizeof(playback));
 watch_exit(&exit_watch);assert(strstr(captured_log,"code=00000000") && !strstr(captured_log,"NATIVE COMPANIONS"));
 printf("native report: 49 production-body scenarios passed; maximal numeric report %zu / 10000 characters; exit report retention and 499 bounded capacities and two exit-recorder cases passed\n",maximum);
}
