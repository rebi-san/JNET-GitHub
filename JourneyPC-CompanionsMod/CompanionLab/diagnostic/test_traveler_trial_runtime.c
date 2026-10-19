/* Production orchestration with simulated native calls; not game ABI validation. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __declspec
#undef __declspec
#endif
#define __declspec(x)
#include "jnet_session_log.h"
static struct jnet_session_logs logs,*session_logs=&logs;
#include "native_allocation.h"
#include "native_mesh_callbacks.h"
#include "native_mesh_guard.h"
#include "native_geometry_pool.h"
typedef int32_t LONG;
typedef uint32_t DWORD;
typedef uint64_t ULONGLONG;
static const uintptr_t image=0x100000;
static unsigned char game[0x300],players[0x28dd0],manager[0x20],callback_manager[0x80040],decorations[0x434020],match[0x11dc0];
static unsigned char entries[3][0x200];
static uint64_t node[5],original_node[5],resource_node[3],context_node[3],callback_type=0x99990000;
static int subscriptions=1;
static uint64_t geometry_source[3],geometry_payload[50],geometry_rows[24];
static unsigned char *allocated;
static unsigned creates,deletes,prepares,initializes,cleans,fail_mode,global_reset;
static uint64_t unreadable;
static unsigned gate_reads,change_gate;
static uint64_t clock_ms=1000;
static int level=2,lock_ok=1,log_ok=1;
static LONG JNETStartup[5]={1,1,1,0,1},JNETMesh[6],JNETFlight[18]={[3]=7},JNETResource;
static void *JNETGame=game,*guarded_mesh;
static uint64_t resource_game,resource_barn,mesh_context;
static DWORD resource_thread;
static int allocation_attempted,bound_state,mesh_fault,lifetime_lock;
static struct { int started; } pose_link;
static struct { int mesh_guard_failure; } body_contexts[2],*body_context;
static void put(uint64_t at,uint64_t value) { memcpy((void *)(uintptr_t)at,&value,8); }
static int own_read(void *ctx,uint64_t at,void *out,size_t size) {
    (void)ctx;
    if (at==unreadable || (fail_mode==1 && at==(uintptr_t)geometry_source+0x10)) return 0;
    if (at==(uintptr_t)players+0xa2 && ++gate_reads==2 && change_gate) players[0xa3]^=1;
    if (at==image+0x37c7198 && size==1) { *(unsigned char *)out=(unsigned char)global_reset; return 1; }
    if(at<UINT64_C(0x100000000))return 0;
    memcpy(out,(void *)(uintptr_t)at,size);return 1;
}
struct callback_read_budget { ULONGLONG deadline; int expired; };
static int callback_read(void *ctx,uint64_t at,void *out,size_t n) { (void)ctx;return own_read(NULL,at,out,n); }
static LONG InterlockedExchange(volatile LONG *p,LONG v) { LONG old=*p;*p=v;return old; }
static void *InterlockedExchangePointer(void **p,void *v) { void *old=*p;*p=v;return old; }
static DWORD GetCurrentThreadId(void) { return 7; }
static ULONGLONG GetTickCount64(void) { return clock_ms; }
static void *GetModuleHandleW(void *x) { (void)x;return (void *)image; }
static void *GetProcessHeap(void) { return NULL; }
static void *HeapAlloc(void *heap,unsigned flags,size_t n) { (void)heap;(void)flags;return malloc(n); }
static void HeapFree(void *heap,unsigned flags,void *p) { (void)heap;(void)flags;free(p); }
static int TryAcquireSRWLockExclusive(int *p) { (void)p;return lock_ok; }
static void ReleaseSRWLockExclusive(int *p) { (void)p; }
#include "diagnostic_checkpoint.h"
static unsigned checkpoint_count;
static void boundary_checkpoint_begin(void) {checkpoint_count=0;}
static int boundary_checkpoint(const char *s) {
    assert(strlen(s)<=160);
    if(checkpoint_count>=JC_CHECKPOINT_LIMIT)return 0;
    ++checkpoint_count;return log_ok;
}
static void flight_mark(LONG p,LONG b) { (void)p;(void)b; }
static int jc_render_level(jc_layout_reader r,void *c,uint64_t g,int *out) { (void)r;(void)c;(void)g;*out=level;return 0; }
static int jc_three_clone_check(jc_layout_reader r,void *c,uint64_t i,const uint64_t m[3],int *d) {
    (void)r;(void)c;(void)i;(void)m;*d=1;return fail_mode==2;
}
static int jc_three_pose_storage(jc_layout_reader r,void *c,uint64_t i,const uint64_t m[3],int *a,int *b) {
    (void)r;(void)c;(void)i;(void)m;*a=*b=1;return 0;
}
struct jc_animation_workspace { int unused; };
struct jc_animation_result { int disjoint; };
static int jc_three_animation_storage(jc_layout_reader r,void *c,uint64_t i,const uint64_t m[3],struct jc_animation_result *a,struct jc_animation_workspace *w) {
    (void)r;(void)c;(void)i;(void)m;a->disjoint=1;return !w;
}
static void types(uint64_t d) {
    put(d,image+0x5d0380);put(d+0x160,image+0x67f3f8);put(d+0x7b0,image+0x67f3f8);
    put(d+0xe50,image+0x67f3f8);put(d+0xa390,image+0x5d0360);
}
static uint64_t trial_create(void *c) { (void)c;++creates;allocated=calloc(1,0x14640);assert(allocated);types((uintptr_t)allocated);return (uintptr_t)allocated; }
static void trial_destroy(void *c,uint64_t d) { (void)c;assert(d==(uintptr_t)allocated);++deletes;free(allocated);allocated=NULL; }
static void initialized(uint64_t d,unsigned index,unsigned glyph) {
    put(d+0x1f0,(uintptr_t)geometry_source);put(d+0x210,(uintptr_t)&geometry_rows[index*6]);
    geometry_rows[index*6]=geometry_rows[index*6+1]=d+0x210;
    types(d);put(d+0x6d8,(uintptr_t)entries[index]);put(d+0x6e0,4);
    if (index!=2) put(d+0xa620,d+0x160);
    put(d+0x2240,(uintptr_t)decorations+0x10+glyph*0x10d0);
    for (unsigned i=0;i<14;++i) put(d+0x1780+i*8,index*100+i);
}
static void trial_prepare(uintptr_t base,uint64_t d,uint64_t m) {
    assert(base==image && d==(uintptr_t)allocated && m==(uintptr_t)manager);
    uint64_t link;memcpy(&link,allocated+0xa620,8);
    assert(!link && !prepares && !initializes);++prepares;
    if (fail_mode!=5) put(d+0xa620,fail_mode==6 ? (uintptr_t)players+0x210 : d+0x160);
    if (fail_mode==7) put((uintptr_t)players+0xb0+0x2240,0);
}
static uintptr_t fake_init(uintptr_t d,uintptr_t m,uintptr_t e,uintptr_t u,uintptr_t lua,uintptr_t random,uintptr_t name,uintptr_t sound,uintptr_t dec,uintptr_t remote) {
    assert(m==(uintptr_t)manager && e==0x500000 && u==0x1234 && lua==0x600000 && random==0x700000);
    assert(name==image+0x5dffa0 && sound==0x800000 && dec==(uintptr_t)decorations && (unsigned char)remote==1);
    if (d!=(uintptr_t)allocated) return 0x3456;
    uint64_t link;memcpy(&link,allocated+0xa620,8);
    assert(prepares==1 && link==d+0x160); /* Reproduces the native prerequisite; initializer does not supply it. */
    ++initializes;initialized(d,2,2);uint32_t count=3;memcpy(decorations+0x434010,&count,4);
    allocated[0x81]=1;
    put(d+0x168,(uintptr_t)resource_node);put(d+0x190,0x56780000);put(d+0x9bb0,0x99990000);
    node[0]=(uintptr_t)original_node;node[1]=d+0x160;node[4]=(uintptr_t)&callback_type;
    if (subscriptions && fail_mode!=8) put((uintptr_t)callback_manager+0x80030,(uintptr_t)node);
    if (fail_mode==9) original_node[1]=0;
    if (fail_mode==10) put(d+0x168,(uintptr_t)context_node);
    JNETMesh[2]=1;
    if (fail_mode==3) put(d+0x1780,0);
    return 0x7890;
}
static uintptr_t fake_cleanup(uintptr_t d,uintptr_t sound) {
    assert(d==(uintptr_t)allocated && sound==0x800000);++cleans;
    unsigned empty[]={0x168,0x190,0x6d8,0x6e0,0x9b40,0x9bb0};
    for (unsigned i=0;i<6;++i) put(d+empty[i],0);
    if (fail_mode==4) put(d+0x190,1);
    if (fail_mode!=11) geometry_rows[12]=geometry_rows[13]=0;
    if (fail_mode==12) geometry_rows[0]=0;
    put(d+0x1f0,0);put(d+0x210,0);put(d+0x218,0);put(d+0x220,0);
    put((uintptr_t)callback_manager+0x80030,subscriptions ? (uintptr_t)original_node : 0);return 0;
}
static uintptr_t (*original_cleanup)(uintptr_t,uintptr_t)=fake_cleanup;
static struct {unsigned failed,state;uint64_t actor;} traveler_glyph_init;
static unsigned glyph_begins,glyph_ends,glyph_releases;
static int traveler_glyph_init_begin(uint64_t base,uint64_t actor,uint64_t anchor,uint64_t dec,uint32_t before) {
    assert(base==image && actor==(uintptr_t)allocated && dec==(uintptr_t)decorations && before==2);
    assert(anchor==(uintptr_t)decorations+0x10+0x10d0+0x90);++glyph_begins;
    if(fail_mode==13){traveler_glyph_init.failed=85;return 0;}
    traveler_glyph_init.actor=actor;traveler_glyph_init.state=1;return 1;
}
static int traveler_glyph_init_end(void) {
    assert(traveler_glyph_init.state==1);++glyph_ends;
    if(fail_mode==14){traveler_glyph_init.failed=89;return 0;}
    traveler_glyph_init.state=2;return 1;
}
static int traveler_glyph_init_release(uint64_t actor) {
    assert(actor==traveler_glyph_init.actor && deletes==1);++glyph_releases;
    traveler_glyph_init.state=3;return 1;
}
static int traveler_body_release(uint64_t actor) {
    assert(actor==(uintptr_t)allocated && cleans==1 && !deletes);
    uint64_t material=1;memcpy(&material,allocated+0x210,8);assert(!material);return 1;
}
#include "traveler_trial_runtime.h"
static void reset(void) {
    free(allocated);allocated=NULL;traveler_object=0;
    memset(&traveler_glyph_init,0,sizeof(traveler_glyph_init));glyph_begins=glyph_ends=glyph_releases=0;
    memset((void *)JNETTraveler,0,sizeof(JNETTraveler));memset(game,0,sizeof(game));memset(players,0,sizeof(players));
    memset(manager,0,sizeof(manager));memset(callback_manager,0,sizeof(callback_manager));memset(JNETMesh,0,sizeof(JNETMesh));
    memset(match,0,sizeof(match));unreadable=0;gate_reads=change_gate=0;
    creates=deletes=prepares=initializes=cleans=fail_mode=global_reset=0;allocation_attempted=0;clock_ms=1000;level=2;
    memset(geometry_rows,0,sizeof(geometry_rows));
    geometry_source[2]=(uintptr_t)geometry_payload;geometry_payload[0x180/8]=(uintptr_t)geometry_rows;
    put((uintptr_t)geometry_payload+0x188,4);
    subscriptions=1;memset(original_node,0,sizeof(original_node));
    original_node[1]=(uintptr_t)players+0x14850;original_node[4]=(uintptr_t)&callback_type;
    put((uintptr_t)callback_manager+0x80030,(uintptr_t)original_node);
    resource_node[2]=0xabc000;context_node[2]=(uintptr_t)callback_manager;
    put((uintptr_t)game+0x200,(uintptr_t)callback_manager);
    put((uintptr_t)players+0x14858,(uintptr_t)resource_node);
    original_full_init=fake_init;JNETGame=game;lock_ok=1;log_ok=1;
    put((uintptr_t)players,image+0x5da340);initialized((uintptr_t)players+0xb0,0,0);initialized((uintptr_t)players+0x146f0,1,1);
    put((uintptr_t)game+0x1e0,(uintptr_t)players);put((uintptr_t)game+0x28,(uintptr_t)manager);
    put((uintptr_t)game+0xb0,0x500000);put((uintptr_t)game+0x20,0x600000);put((uintptr_t)game+0x1a0,0x700000);
    put((uintptr_t)game+0x150,0x800000);put((uintptr_t)game+0x138,(uintptr_t)decorations);put((uintptr_t)game+0x180,(uintptr_t)match);
    put((uintptr_t)manager+8,(uintptr_t)context_node);uint32_t count=2;memcpy(decorations+0x434010,&count,4);
}
static void invoke(void) {
    assert(full_init_observer((uintptr_t)players+0x146f0,(uintptr_t)manager,0x500000,0x1234,0x600000,0x700000,
        image+0x5dffa0,0x800000,(uintptr_t)decorations,0x12340001)==0x3456);
}
int main(void) {
    reset();
    traveler_barn=(uintptr_t)players;
    traveler_admission_initialized((uintptr_t)players+0x146f0);
    assert(traveler_admission_storage_matches(image));
    memset(traveler_original,0,sizeof(traveler_original));
    assert(traveler_admission_storage_matches(image));
    uint64_t glyph=traveler_admission_identity[1].glyph;
    put((uintptr_t)players+0x146f0+0x2240,glyph+16);
    assert(!traveler_admission_storage_matches(image));
    put((uintptr_t)players+0x146f0+0x2240,glyph);
    traveler_admission_initialized((uintptr_t)players+0xb0);
    assert(!traveler_admission_storage_matches(image));
    traveler_admission_initialized((uintptr_t)players+0x146f0);
    assert(traveler_admission_storage_matches(image));
    traveler_cleanup((uintptr_t)players+0xb0,0);
    assert(!traveler_admission_storage_matches(image));
    reset();checkpoint_count=JC_CHECKPOINT_LIMIT;JNETTraveler[19]=1;invoke();
    assert(checkpoint_count>0 && checkpoint_count<JC_CHECKPOINT_LIMIT);
    assert(JNETTraveler[0]==3 && initializes==1 && creates==1);
    assert(JNETTraveler[29]==1 && JNETTraveler[30]==0 && JNETTraveler[31]==1);
    invoke();assert(initializes==1);traveler_cleanup((uintptr_t)players+0x146f0,0x800000);assert(!cleans);
    traveler_cleanup((uintptr_t)players+0xb0,0x800000);assert(JNETTraveler[0]==4 && cleans==1 && deletes==1);
    assert(glyph_begins==1 && glyph_ends==1 && glyph_releases==1);
    traveler_cleanup((uintptr_t)players+0xb0,0x800000);assert(cleans==1);
    reset();JNETTraveler[19]=1;level=0;invoke();assert(creates==1 && JNETTraveler[0]==3);
    traveler_cleanup((uintptr_t)players+0xb0,0x800000);
    reset();JNETTraveler[19]=1;level=2;invoke();assert(JNETTraveler[0]==3);traveler_cleanup((uintptr_t)players+0xb0,0x800000);
    for (unsigned bad=1;bad<=4;++bad) {
        reset();traveler_arm((uintptr_t)game);fail_mode=bad;invoke();
        if (bad==1) assert(JNETTraveler[0]==-1 && !creates && !initializes);
        else if (bad==2 || bad==3) assert(JNETTraveler[0]==-2 && initializes==1 && !cleans && !deletes);
        else { traveler_cleanup((uintptr_t)players+0xb0,0x800000);assert(JNETTraveler[0]==-2 && cleans==1 && !deletes); }
    }
    reset();traveler_arm((uintptr_t)game);global_reset=1;invoke();assert(JNETTraveler[0]==-1 && !creates);
    reset();traveler_arm((uintptr_t)game);clock_ms+=600001;invoke();assert(JNETTraveler[0]==-1 && !creates);
    reset();traveler_arm((uintptr_t)game);invoke();traveler_cleanup((uintptr_t)players+0xb0,0x1234);assert(JNETTraveler[0]==-2 && !cleans);
    reset();JNETTraveler[19]=1;log_ok=0;invoke();assert(JNETTraveler[0]==-1 && JNETTraveler[2]==18 && !creates);
    /* Native getter presence, not raw activity: all gate/block/chapter/transport combinations. */
    for (unsigned flags=0;flags<8;++flags) for (unsigned blocked=0;blocked<2;++blocked)
    for (unsigned different=0;different<2;++different) for (unsigned connected=0;connected<2;++connected) {
        reset();players[0xa2]=flags&1;players[0xa3]=(flags>>1)&1;players[0xa4]=(flags>>2)&1;
        players[0x1670c]=blocked;players[0x14]=2;players[0x64]=different ? 0 : 2;
        match[0x11db4]=connected;JNETTraveler[19]=1;invoke();
        int present=(flags&5) && !blocked && !different;
        assert(JNETTraveler[26]==present && JNETTraveler[28]==0);
        assert(players[0xa2]==(flags&1) && players[0xa3]==((flags>>1)&1) && players[0xa4]==((flags>>2)&1));
        if (present || connected) {
            assert(JNETTraveler[0]==-1 && !creates && !initializes);
            assert(JNETTraveler[15]==(present ? 23 : 24));
            traveler_barn=(uintptr_t)players;
            assert(traveler_admission_storage_matches(image));
        } else {
            assert(JNETTraveler[0]==3 && creates==1 && initializes==1);
            traveler_cleanup((uintptr_t)players+0xb0,0x800000);assert(JNETTraveler[0]==4);
        }
    }
    /* A preflight change after arming is checked again before the factory call. */
    reset();traveler_arm((uintptr_t)game);players[0xa2]=1;invoke();
    assert(JNETTraveler[15]==23 && JNETTraveler[1]==2 && !creates);
    reset();traveler_arm((uintptr_t)game);unreadable=(uintptr_t)players+0xa2;invoke();
    assert(JNETTraveler[15]==30 && JNETTraveler[28]==JC_LAYOUT_READ && !creates);
    reset();traveler_arm((uintptr_t)game);gate_reads=0;change_gate=1;invoke();
    assert(JNETTraveler[15]==30 && JNETTraveler[28]==JC_LAYOUT_CHANGED && !creates);
    reset();traveler_arm((uintptr_t)game);unreadable=(uintptr_t)match+0x11db4;invoke();
    assert(JNETTraveler[15]==31 && JNETTraveler[27]==-1 && !creates);
    for (unsigned bad=5;bad<=7;++bad) {
        reset();JNETTraveler[19]=1;fail_mode=bad;invoke();
        assert(JNETTraveler[0]==-2 && JNETTraveler[1]==9 && JNETTraveler[2]==19);
        assert(prepares==1 && !initializes && !cleans && !deletes);
    }
    reset();subscriptions=0;put((uintptr_t)callback_manager+0x80030,0);JNETTraveler[19]=1;invoke();
    assert(JNETTraveler[0]==3 && JNETTraveler[13]==1 && JNETTraveler[38]==1 && !JNETTraveler[37]);
    traveler_cleanup((uintptr_t)players+0xb0,0x800000);assert(JNETTraveler[0]==4);
    for (unsigned bad=8;bad<=10;++bad) {
        reset();JNETTraveler[19]=1;fail_mode=bad;invoke();
        assert(JNETTraveler[0]==-2 && JNETTraveler[2]==15 && !deletes);
        assert(JNETTraveler[39]==(bad==8 ? 4 : bad==9 ? 2 : 3));
    }
    reset();JNETTraveler[19]=1;context_node[2]=(uintptr_t)manager;invoke();
    assert(JNETTraveler[0]==-1 && JNETTraveler[2]==15 && JNETTraveler[32]==JC_LAYOUT_TYPE && !creates);
    reset();JNETTraveler[19]=1;unreadable=(uintptr_t)game+0x200;invoke();
    assert(JNETTraveler[0]==-1 && JNETTraveler[32]==JC_LAYOUT_READ && !creates);
    /* Native peers can leave the extra object's pool before chapter cleanup. */
    reset();JNETTraveler[19]=1;invoke();geometry_rows[0]=geometry_rows[1]=geometry_rows[6]=geometry_rows[7]=0;
    traveler_cleanup((uintptr_t)players+0xb0,0x800000);
    assert(JNETTraveler[0]==4 && JNETTraveler[40]==1 && JNETTraveler[41]==0 && deletes==1);
    for (unsigned bad=11;bad<=12;++bad) {
        reset();JNETTraveler[19]=1;invoke();fail_mode=bad;
        traveler_cleanup((uintptr_t)players+0xb0,0x800000);
        assert(JNETTraveler[0]==-2 && JNETTraveler[2]==17 && !deletes && cleans==1);
    }
    for(unsigned bad=13;bad<=14;++bad) {
        reset();JNETTraveler[19]=1;fail_mode=bad;invoke();
        assert(JNETTraveler[0]==-2 && JNETTraveler[2]==5 && glyph_begins==1 && !glyph_releases);
        assert(initializes==(bad==14) && glyph_ends==(bad==14));
        assert(JNETTraveler[16]==(bad==13?85:89));
    }
    reset();puts("Full traveler orchestration: automatic setup, chapter filtering, single initialization, ownership refusal, matching cleanup, restoration and quarantine passed");
    return 0;
}
