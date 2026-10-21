#ifdef _WIN32
#include <windows.h>
#endif
#include "native_cloth_producer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct fixture {
    unsigned char *barn,*dude,*collection;
    unsigned char game[0x200],owner[2][32],link[2][32];
    uint32_t passes[7],reserve;
    struct jc_cloth_frames frames;
    struct jc_cloth_producer_participant participant;
    unsigned prepare,abandon,begin,configure,adopt,cancel,acquired,released,fail,mutate;
    unsigned char vertices[2][4096],indices[2][4096];
};
static void put64(void *p,uint64_t v) { memcpy(p,&v,8); }
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static int read_memory(void *ctx,uint64_t address,void *out,size_t n) {
    struct fixture *f=ctx;
    if (address==UINT64_C(0x140731f60) && n==28) { memcpy(out,f->passes,n);return 1; }
    if (address==UINT64_C(0x140731f84) && n==4) { memcpy(out,&f->reserve,n);return 1; }
    const void *blocks[]={f->barn,f->dude,f->collection,f->game,f->owner,f->link};
    const size_t sizes[]={0x6bd000,0x9b80,0x9200,sizeof(f->game),sizeof(f->owner),sizeof(f->link)};
    for (unsigned i=0;i<6;++i) if (address>=(uintptr_t)blocks[i] && address-(uintptr_t)blocks[i]<=sizes[i] &&
        n<=sizes[i]-(address-(uintptr_t)blocks[i])) { memcpy(out,(void *)(uintptr_t)address,n);return 1; }
    return 0;
}
static int acquire(void *ctx,unsigned kind,uint64_t object) {
    struct fixture *f=ctx;(void)kind;assert(object);++f->acquired;return 1;
}
static void release(void *ctx,unsigned kind,uint64_t object) {
    struct fixture *f=ctx;(void)kind;assert(object);++f->released;
}
static int done(void *ctx,uint64_t generation) { (void)ctx;assert(generation);return 1; }
static int pass_done(void *ctx,uint64_t generation,unsigned pass) { assert(pass<=1);return done(ctx,generation); }
static int publish(void *ctx,uint64_t generation,const struct jc_cloth_frame_draw *draws,unsigned n,uint64_t *ticket) {
    (void)ctx;(void)generation;(void)draws;(void)n;(void)ticket;assert(0);return 0;
}
static int retired(void *ctx,uint64_t ticket) { (void)ctx;(void)ticket;return 1; }
static int prepare(void *ctx,const struct jc_cloth_partition *p,struct jc_cloth_stream streams[7]) {
    struct fixture *f=ctx;++f->prepare;if (f->fail==4) return JC_FRAME_BUSY;
    memset(streams,0,7*sizeof(*streams));
    for (unsigned r=4;r<=5;++r) streams[r]=(struct jc_cloth_stream){
        .vertex=100+r,.index=200+r,.vertex_bytes=sizeof(f->vertices[0]),.index_bytes=sizeof(f->indices[0]),
        .vertices=p->vertex_capacity[r],.indices=p->index_capacity[r],
        .mapped_vertices=f->vertices[r-4],.mapped_indices=f->indices[r-4]};
    if (f->mutate==1) f->passes[4]^=1;
    if (f->mutate==2) put32(f->barn+0x270080+2*0xe0+0x54,99);
    if (f->mutate==3) put64(f->dude+0x9b58,0);
    return 0;
}
static void abandon(void *ctx,struct jc_cloth_stream streams[7]) {
    struct fixture *f=ctx;++f->abandon;memset(streams,0,7*sizeof(*streams));
}
static int begin(void *ctx,uint64_t generation,const struct jc_cloth_partition_job *jobs,size_t count,
        const void *args,const struct jc_cloth_stream streams[7],const struct jc_cloth_material *materials,
        unsigned material_count,unsigned *index) {
    struct fixture *f=ctx;++f->begin;if (f->fail==6) return JC_FRAME_BUSY;
    return jc_cloth_frame_begin(&f->frames,generation,jobs,count,args,streams,materials,material_count,index);
}
static int configure(void *ctx,unsigned index,const uint32_t passes[7]) {
    struct fixture *f=ctx;++f->configure;if (f->fail==7) return JC_FRAME_ARGUMENT;
    return jc_cloth_frame_configure_passes(&f->frames,index,passes,pass_done,f);
}
static int adopt(void *ctx,unsigned index,uint64_t generation,unsigned pass) {
    struct fixture *f=ctx;++f->adopt;struct jc_cloth_frame *frame=f->frames.frames+index;
    assert(frame->phase==JC_FRAME_WRITING && frame->generation==generation && frame->pass_required==(1u<<pass));
    if (f->fail==8) return JC_FRAME_OWNERSHIP;
    if (f->fail==9) { frame->phase=JC_FRAME_UNKNOWN;return JC_FRAME_QUARANTINED; }
    return 0;
}
static int cancel(void *ctx,unsigned index) {
    struct fixture *f=ctx;++f->cancel;if (f->fail==10) return JC_FRAME_BUSY;
    assert(f->frames.frames[index].phase==JC_FRAME_WRITING);
    jc_cloth_frame_release(&f->frames,f->frames.frames+index);return 0;
}
static void setup(struct fixture *f,unsigned pass) {
    memset(f,0,sizeof(*f));
    f->barn=calloc(1,0x6bd000);f->dude=calloc(1,0x9b80);f->collection=calloc(1,0x9200);
    assert(f->barn && f->dude && f->collection);f->reserve=768;
    f->passes[4]=f->passes[5]=pass;
    put64(f->game+0x108,(uintptr_t)f->barn);put64(f->game+0x1e0,UINT64_C(0x200000000));
    put64(f->dude,UINT64_C(0x1405d0380));f->dude[0x81]=1;
    uint32_t appearance[4]={5,10,2,4};memcpy(f->dude+0x68,appearance,sizeof(appearance));
    put64(f->dude+0x9b40,(uintptr_t)f->collection);put32(f->collection+0x91b0,2);
    for (unsigned i=0;i<2;++i) {
        unsigned slot=i+1,role=5-i;
        put64(f->dude+0x9b58+i*16,(uintptr_t)f->collection+i*32);
        put64(f->dude+0x9b60+i*16,(uintptr_t)f->collection+0xc0+i*0x1828);
        put64(f->collection+i*32,(uintptr_t)f->owner[i]);put64(f->collection+i*32+0x18,(uintptr_t)f->barn);
        put64(f->owner[i]+0x10,(uintptr_t)f->link[i]);put64(f->link[i]+0x18,(uintptr_t)f->barn+0x80+slot*0xd00);
        put32(f->barn+0xd14+slot*0xd00,role);put32(f->barn+0x270118+slot*0xe0,1);
    }
    put64(f->barn+0x669960,(uintptr_t)f->barn+0x80);put64(f->barn+0x669968,(uintptr_t)f->barn+0x270080);
    for (unsigned k=0;k<3;++k) put32(f->barn+0x6bca10+pass*0x60+k*0x20,4);
    for (unsigned slot=1;slot<=4;++slot) {
        unsigned role=slot&1 ? 5 : 4;
        put32(f->barn+0xd14+slot*0xd00,role);put32(f->barn+0x270118+slot*0xe0,1);
        put32(f->barn+0x270080+slot*0xe0+0x54,5);put32(f->barn+0x270080+slot*0xe0+0x5c,9);
        unsigned char *entry=f->barn+0x669d10+(pass ? 0 : 768*16)+(slot-1)*16;
        put32(entry,slot);put32(entry+4,role);entry[9]=1;
    }
    f->participant=(struct jc_cloth_producer_participant){.game=(uintptr_t)f->game,.barn=(uintptr_t)f->barn,
        .dude=(uintptr_t)f->dude,.peer=7,.generation=9};
    for (unsigned i=0;i<2;++i) {
        struct jc_cloth_material *m=f->participant.materials+i;
        *m=(struct jc_cloth_material){.peer=7,.generation=9,.material=1000+i,.uniforms=2000+i*128,
            .uniform_bytes=64,.environment=5000,.participant=1,.role=4+i,.outfit=2,.symbol=4,.scarf=5,.scarf_capacity=10};
        for (unsigned j=0;j<6;++j) m->atlas[j]=3000+i*100+j;
    }
    assert(!jc_cloth_frames_init(&f->frames,(struct jc_cloth_frame_backend){f,acquire,release,done,publish,retired}));
}
static void cleanup(struct fixture *f) { free(f->barn);free(f->dude);free(f->collection); }

typedef uint32_t DWORD;
typedef int32_t LONG;
typedef size_t SIZE_T;
struct jc_cloth_live { struct jc_cloth_frames frames; unsigned failed; };
struct jc_cloth_producer_live {
 struct jc_cloth_live *backend;void *context;
 int (*adopt)(void *,unsigned,uint64_t,unsigned);
 int (*pass_done)(void *,uint64_t,unsigned);
};
static struct fixture *current;
static struct jc_cloth_live backend,*active_cloth_backend;
static struct jc_cloth_producer cloth_producer;
static struct jc_cloth_producer_participant cloth_participant;
static unsigned cloth_participant_ready,cloth_production_pass,cloth_map_lock;
static uint64_t cloth_production_serial,cloth_production_job;
static DWORD cloth_production_thread;
static struct { unsigned roles,index,launched_roles,seen[2],finish_failed;volatile LONG worker_reported;uint64_t barn,generation; } cloth_maps;
static volatile LONG JCProbeCompanionFeatures[40];
static unsigned dependencies,stock_workers,routed_workers,original_finishes,seals,commits,finalizes,unmaps;
static unsigned owner_ok=1,flip_busy,write_fail,launch_second_pass;
static void *traveler_actor;
static LONG JCProbeActorTrial[5];
enum { JC_ACTOR_RUNTIME_SCOPE=77 };
static int jc_actor_runtime_quarantine(void *p,int code) { (void)p;return code; }
static void traveler_actor_report(int code) { (void)code; }
static unsigned char job_object[0x60];
static int cloth_produce_before_dependency(uintptr_t dependency);
static void *GetModuleHandleW(void *v) { (void)v;return (void *)(uintptr_t)UINT64_C(0x140000000); }
static DWORD GetCurrentThreadId(void) { return 7; }
static void *GetCurrentProcess(void) { return NULL; }
static void *GetProcessHeap(void) { return NULL; }
static void *HeapAlloc(void *h,unsigned f,size_t n) { (void)h;(void)f;return malloc(n); }
static void HeapFree(void *h,unsigned f,void *p) { (void)h;(void)f;free(p); }
static LONG InterlockedIncrement(volatile LONG *v) { return ++*v; }
static LONG InterlockedExchange(volatile LONG *v,LONG n) { LONG old=*v;*v=n;return old; }
static LONG InterlockedCompareExchange(volatile LONG *v,LONG n,LONG expected) { LONG old=*v;if(old==expected)*v=n;return old; }
static void message(const char *s) { assert(!strncmp(s,"[JC]",4)); }
static void AcquireSRWLockExclusive(unsigned *p) { assert(!*p);*p=1; }
static void ReleaseSRWLockExclusive(unsigned *p) { assert(*p==1);*p=0; }
static void AcquireSRWLockShared(unsigned *p) { AcquireSRWLockExclusive(p); }
static void ReleaseSRWLockShared(unsigned *p) { ReleaseSRWLockExclusive(p); }
static int own_read(void *ctx,uint64_t at,void *out,size_t n) {
 (void)ctx;
 if(at==UINT64_C(0x140731f70)&&n==8) { memcpy(out,current->passes+4,n);return 1; }
 if(at==(uintptr_t)job_object+0x58&&n==1) {
  memcpy(out,job_object+0x58,1);if(flip_busy)job_object[0x58]=0;return 1;
 }
 return read_memory(current,at,out,n);
}
static int jc_local_windows_write(uint64_t to,const void *from,size_t n) {
 if(write_fail)return 0;memcpy((void *)(uintptr_t)to,from,n);return 1;
}
static int jc_cloth_live_owner(struct jc_cloth_live *s) { return s==&backend&&owner_ok; }
static int cloth_producer_adopt(void *ctx,unsigned index,uint64_t gen,unsigned pass) {
 (void)ctx;(void)index;(void)gen;(void)pass;assert(0);return 1;
}
static int cloth_producer_pass_done(void *ctx,uint64_t gen,unsigned pass) { return pass_done(ctx,gen,pass); }
static int adopt_live(void *ctx,unsigned index,uint64_t generation,unsigned pass) {
 int code=adopt(ctx,index,generation,pass);
 if(!code) {
  backend.frames=current->frames;
  cloth_maps.roles=(1u<<4)|(1u<<5);cloth_maps.barn=(uintptr_t)current->barn;
  cloth_maps.generation=generation;cloth_maps.index=index;
  cloth_maps.seen[0]=cloth_maps.seen[1]=cloth_maps.roles;
 }
 return code;
}
static struct jc_cloth_producer_ops jc_cloth_producer_live_ops(struct jc_cloth_producer_live *p) {
 assert(p->backend==&backend);
 return (struct jc_cloth_producer_ops){current,prepare,abandon,begin,configure,adopt_live,cancel};
}
static uint32_t original_cloth_dependency(uintptr_t dep) {
 assert(dep==(uintptr_t)current->barn+0x6bce00);++dependencies;return 123;
}
static void original_worker(void *w,const void *entry,void *args,void *streams,void *descriptor,void *state) {
 (void)w;(void)entry;(void)args;(void)streams;(void)descriptor;(void)state;++stock_workers;
}
static void (*original_cloth_worker[2])(void *,const void *,void *,void *,void *,void *)={original_worker,original_worker};
static void cloth_worker_call(void *ctx,void *w,const void *e,void *a,void *s,void *d,void *v) {
 (void)ctx;(void)w;(void)e;(void)a;(void)s;(void)d;(void)v;
}
static int fixture_dispatch(struct jc_cloth_frame *f,uint64_t gen,unsigned roles,void *streams,
 void *ctx,jc_cloth_worker_call call,void *w,const void *e,void *args,void *actual,void *desc,void *state) {
 (void)f;(void)gen;(void)roles;(void)streams;(void)ctx;(void)call;(void)w;(void)e;(void)args;(void)actual;(void)desc;(void)state;
 ++routed_workers;return 1;
}
#define jc_cloth_frame_dispatch fixture_dispatch
static uintptr_t original_cloth_finish(uintptr_t barn,void *result,unsigned pass,uintptr_t fourth) {
 assert(barn==(uintptr_t)current->barn&&pass<=1&&fourth==99);++original_finishes;
 if(launch_second_pass) {
  assert(pass==0 && cloth_production_pass==1 && cloth_production_thread==7);
  assert(cloth_produce_before_dependency(barn+0x6bce00)==1);
  launch_second_pass=0;
 }
 unsigned char *bytes=result;memset(bytes,0,16);bytes[0]=!job_object[0x58];
 uint64_t job=(uintptr_t)job_object;if(job_object[0x58])memcpy(bytes+8,&job,8);return (uintptr_t)result;
}
struct cloth_finish_call { uintptr_t barn,result,fourth,returned; unsigned roles,detail,called; };
static int cloth_finish_native(void *ctx,uint64_t generation,unsigned pass) {
 struct cloth_finish_call *c=ctx;assert(generation==cloth_maps.generation);
 c->called=1;c->returned=original_cloth_finish(c->barn,(void *)c->result,pass,c->fourth);return 1;
}
static int fixture_seal(struct jc_cloth_frames *s,unsigned index,unsigned pass) {
 assert(s==&backend.frames&&index==cloth_maps.index&&pass==0&&!job_object[0x58]);++seals;return 0;
}
static int fixture_commit(struct jc_cloth_frame *f,uint64_t gen,unsigned pass,void *args,size_t n) {
 assert(f->generation==gen&&pass==0&&args==f->source_args&&n==JC_CLOTH_WORKER_ARGS);++commits;return 0;
}
static int fixture_finalize(struct jc_cloth_frames *s,unsigned index,unsigned pass,
 int (*call)(void *,uint64_t,unsigned),void *ctx) {
 ++finalizes;assert(seals==1&&commits==1);assert(call(ctx,s->frames[index].generation,pass)==1);
 s->frames[index].phase=JC_FRAME_READY;return 0;
}
static int jc_cloth_live_unmap(struct jc_cloth_live *s,unsigned index) {
 assert(s==&backend&&index==cloth_maps.index);++unmaps;return 0;
}
#define jc_cloth_frame_seal_pass fixture_seal
#define jc_cloth_frame_commit_pass fixture_commit
#define jc_cloth_frame_finalize_pass fixture_finalize
static int cloth_produce_before_dependency(uintptr_t dependency) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_participant_ready || cloth_production_thread!=GetCurrentThreadId() ||
        dependency!=cloth_participant.barn+0x6bce00) return 0;
    uint32_t pass[2]; int code=JC_FRAME_OWNERSHIP;unsigned stage=0,quarantined=0;
    if (!own_read(NULL,(uintptr_t)GetModuleHandleW(NULL)+0x731f70,pass,sizeof(pass))) goto failed;
    if (pass[0]!=cloth_production_pass || pass[1]!=cloth_production_pass) return 0;
    if (cloth_maps.roles) { quarantined=1;goto failed; }
    if (!jc_cloth_live_owner(s) || s->frames.closing || cloth_production_serial==UINT64_MAX) goto failed;
    struct jc_cloth_producer_live adapter={s,NULL,cloth_producer_adopt,cloth_producer_pass_done};
    struct jc_cloth_producer_ops ops=jc_cloth_producer_live_ops(&adapter);
    code=jc_cloth_producer_prepare(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),
        &cloth_participant,++cloth_production_serial,cloth_production_pass,&ops,&cloth_producer);
    stage=cloth_producer.stage;quarantined=cloth_producer.quarantined;
    if (!code) {
        InterlockedExchange(&JCProbeCompanionFeatures[32],0);
        InterlockedExchange(&JCProbeCompanionFeatures[33],0);
        InterlockedExchange(&JCProbeCompanionFeatures[34],0);
        InterlockedIncrement(&JCProbeCompanionFeatures[35]);
        cloth_participant_ready=0;return 1;
    }
failed:
    InterlockedExchange(&JCProbeCompanionFeatures[32],code);
    InterlockedExchange(&JCProbeCompanionFeatures[33],(LONG)stage);
    InterlockedExchange(&JCProbeCompanionFeatures[34],(LONG)cloth_producer.detail);
    cloth_participant_ready=0;
    if (!quarantined) {
        LONG declines=InterlockedIncrement(&JCProbeCompanionFeatures[30]);
        if (declines==1 || (declines&63)==0) {
            char line[192];
            snprintf(line,sizeof(line),"[JC] private cloth declined code=%d stage=%u detail=%u jobs=%u roles=%u declines=%ld",
                code,stage,cloth_producer.detail,cloth_producer.jobs.count,cloth_producer.roles,(long)declines);
            message(line);
        }
        return 0;
    }
    s->failed=1;
    message("[JC] automatic cloth preparation failed before worker release; retained for inspection");
    return -1;
}
static uint32_t cloth_dependency_observer(uintptr_t dependency) {
    if (cloth_produce_before_dependency(dependency)<0) return 0;
    AcquireSRWLockExclusive(&cloth_map_lock);
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_maps.roles || dependency!=cloth_maps.barn+0x6bce00) {
        ReleaseSRWLockExclusive(&cloth_map_lock);
        return original_cloth_dependency(dependency);
    }
    struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
    unsigned roles=atomic_load_explicit(&f->route.active_roles,memory_order_acquire);
    /* A completed Remote-only frame does not own the later environment pass. */
    if (!roles) {
        ReleaseSRWLockExclusive(&cloth_map_lock);
        return original_cloth_dependency(dependency);
    }
    unsigned pass=roles==f->pass_roles[0] ? 0 : 1;
    struct jc_cloth_native_jobs *capture=NULL;
    int valid=jc_cloth_live_owner(s) && f->phase==JC_FRAME_WRITING &&
        f->generation==cloth_maps.generation && roles==f->pass_roles[pass] &&
        !(roles&cloth_maps.launched_roles) &&
        (cloth_maps.seen[0]&roles)==roles && (cloth_maps.seen[1]&roles)==roles;
    if (valid) capture=HeapAlloc(GetProcessHeap(),0,sizeof(*capture));
    unsigned char owners[JC_CLOTH_PARTITION_SLOTS]={0};
    unsigned expected=0;
    for (unsigned slot=0;slot<JC_CLOTH_PARTITION_SLOTS;++slot) if (f->route.present[slot]) {
        owners[slot]=(unsigned char)f->route.slots[slot].participant;
        expected+=(roles&(1u<<f->route.slots[slot].role))!=0;
    }
    if (!capture || jc_cloth_native_jobs_read(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),
            cloth_maps.barn,pass,roles,owners,capture) || capture->count!=expected ||
        capture->arguments!=(uintptr_t)f->source_args ||
        jc_cloth_worker_prefix_difference(capture->prefix,f->route.args[0].bytes)!=0x210) valid=0;
    if (valid) for (unsigned i=0;i<capture->count;++i) {
        const struct jc_cloth_partition_job *job=capture->jobs+i;
        if (!f->route.present[job->slot] ||
            memcmp(job,f->route.slots+job->slot,sizeof(*job))) { valid=0; break; }
    }
    if (capture) HeapFree(GetProcessHeap(),0,capture);
    if (valid) cloth_maps.launched_roles|=roles;
    else {
        atomic_store_explicit(&f->route.failed,JC_PARTITION_ARGUMENT,memory_order_release);
        s->failed=1; f->phase=JC_FRAME_UNKNOWN;
        message("[JC] native cloth launch refused: current jobs differ from owned frame; resources retained");
    }
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return valid ? original_cloth_dependency(dependency) : 0;
}
static void cloth_worker_observer(unsigned kind,void *worker,const void *entry,void *args,
                                  void *streams,void *descriptor,void *state) {
    AcquireSRWLockShared(&cloth_map_lock);
    struct jc_cloth_live *s=active_cloth_backend;
    int handled=0;
    if (s && cloth_maps.roles && (uintptr_t)streams==cloth_maps.barn+0x669960) {
        struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
        unsigned char captured[16];
        if (!own_read(NULL,(uintptr_t)entry,captured,sizeof(captured))) {
            if (!InterlockedCompareExchange(&cloth_maps.worker_reported,1,0)) {
                char line[128];
                snprintf(line,sizeof(line),"[JC] cloth worker entry unreadable kind=%u entry=%llx",
                    kind,(unsigned long long)(uintptr_t)entry);
                message(line);
            }
            atomic_store_explicit(&f->route.failed,JC_PARTITION_ARGUMENT,memory_order_release);
            handled=-1;
        } else {
            unsigned slot=jc_cloth_u32(captured),role=jc_cloth_u32(captured+4);
            if (role<7 && (cloth_maps.roles&(1u<<role)) &&
                (!(cloth_maps.launched_roles&(1u<<role)) || slot>=JC_CLOTH_PARTITION_SLOTS ||
                 (uintptr_t)state!=cloth_maps.barn+0x270080+(uintptr_t)slot*0xe0)) {
                atomic_store_explicit(&f->route.failed,JC_PARTITION_SLOT,memory_order_release);
                handled=-1;
            } else handled=jc_cloth_frame_dispatch(f,cloth_maps.generation,cloth_maps.roles,
                (void *)(cloth_maps.barn+0x669960),&original_cloth_worker[kind],cloth_worker_call,
                worker,captured,args,streams,descriptor,state);
            if (handled<0 && !InterlockedCompareExchange(&cloth_maps.worker_reported,1,0)) {
                uintptr_t vertex=0,index=0;
                if (role<7) {
                    own_read(NULL,(uintptr_t)streams+0x10+role*8,&vertex,8);
                    own_read(NULL,(uintptr_t)streams+0x48+role*8,&index,8);
                }
                char line[512];
                snprintf(line,sizeof(line),"[JC] cloth worker rejected kind=%u slot=%u role=%u route=%u rejection=%u phase=%u args=%llx/%llx vertex=%llx/%llx index=%llx/%llx visible=%u subdivided=%u",
                    kind,slot,role,atomic_load(&f->route.failed),atomic_load(&f->route.rejection),f->phase,
                    (unsigned long long)(uintptr_t)args,(unsigned long long)(uintptr_t)f->source_args,
                    (unsigned long long)vertex,(unsigned long long)(uintptr_t)(role<7 ? f->streams[role].mapped_vertices : NULL),
                    (unsigned long long)index,(unsigned long long)(uintptr_t)(role<7 ? f->streams[role].mapped_indices : NULL),
                    captured[9],captured[8]);
                message(line);
            }
        }
    }
    if (!handled && s && cloth_participant.dude) {
        unsigned char captured[16];
        if (own_read(NULL,(uintptr_t)entry,captured,sizeof(captured)) &&
            jc_cloth_producer_owns_worker(own_read,NULL,&cloth_participant,&cloth_producer.lease,captured,
                (uintptr_t)args,(uintptr_t)streams,(uintptr_t)state)) {
            InterlockedIncrement(&JCProbeCompanionFeatures[31]);handled=1;
        }
    }
    if (!handled) original_cloth_worker[kind](worker,entry,args,streams,descriptor,state);
    ReleaseSRWLockShared(&cloth_map_lock);
}
static uintptr_t cloth_finish_observer(uintptr_t barn,void *result,unsigned pass,uintptr_t fourth) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_maps.roles || barn!=cloth_maps.barn || pass>1) {
        DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
        if (s && cloth_participant_ready && barn==cloth_participant.barn && pass==0) {
            cloth_production_thread=GetCurrentThreadId();cloth_production_pass=1;
        }
        uintptr_t returned=original_cloth_finish(barn,result,pass,fourth);
        cloth_production_thread=old_thread;cloth_production_pass=old_pass;
        return returned;
    }
    struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
    if (cloth_maps.finish_failed ||
        !(f->pass_required&(1u<<pass)) || (f->pass_finalized&(1u<<pass))) {
        DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
        cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=1;
        uintptr_t returned=original_cloth_finish(barn,result,pass,fourth);
        cloth_production_thread=old_thread; cloth_production_pass=old_pass;
        return returned;
    }
    unsigned char pending=0,done=0,busy=0; uintptr_t job=0;
    unsigned guard=0;
    if (!jc_cloth_live_owner(s)) guard=1;
    else if (f->phase!=JC_FRAME_WRITING) guard=2;
    else if (f->generation!=cloth_maps.generation) guard=3;
    else if (!own_read(NULL,barn+0x6bcf84,&pending,1)) guard=4;
    else if (pending!=1) guard=5;
    else if (!own_read(NULL,barn+0x6bcad0+pass,&done,1)) guard=6;
    else if (done) guard=7;
    else if (!own_read(NULL,barn+0x6bcef0,&job,8) || !job) guard=8;
    else if (!own_read(NULL,job+0x58,&busy,1)) guard=9;
    int valid=!guard;
    if (valid && busy) {
        unsigned char waiting[16]={0};
        memcpy(waiting+8,&job,8);
        if (jc_local_windows_write((uintptr_t)result,waiting,sizeof(waiting))) return (uintptr_t)result;
        valid=0;guard=10;
    }
    unsigned stage=100,phase=f->phase;
    int code=valid ? JC_FRAME_OK : JC_FRAME_OWNERSHIP;
    if (!code) { stage=101;code=jc_cloth_frame_seal_pass(&s->frames,cloth_maps.index,pass); }
    if (!code) { stage=102;code=jc_cloth_frame_commit_pass(f,f->generation,pass,(void *)f->source_args,JC_CLOTH_WORKER_ARGS); }
    struct cloth_finish_call call={.barn=barn,.result=(uintptr_t)result,.fourth=fourth,.roles=f->pass_roles[pass]};
    if (!code) { stage=103;code=jc_cloth_frame_finalize_pass(&s->frames,cloth_maps.index,pass,cloth_finish_native,&call); }
    if (!code && f->phase==JC_FRAME_READY) { stage=104;code=jc_cloth_live_unmap(s,cloth_maps.index); }
    if (code) {
        unsigned route=atomic_load_explicit(&f->route.failed,memory_order_acquire);
        unsigned rejection=atomic_load_explicit(&f->route.rejection,memory_order_acquire);
        unsigned detail=stage==100 ? guard : call.detail ? call.detail : rejection ? rejection : route;
        InterlockedExchange(&JCProbeCompanionFeatures[32],code);
        InterlockedExchange(&JCProbeCompanionFeatures[33],(LONG)stage);
        InterlockedExchange(&JCProbeCompanionFeatures[34],(LONG)detail);
        char line[384];
        snprintf(line,sizeof(line),"[JC] cloth finish failure code=%d stage=%u detail=%u pass=%u phase=%u pending=%u done=%u busy=%u route=%u completed=%u jobs=%u sealed=%u committed=%u finalized=%u native-called=%u",
            code,stage,detail,pass,phase,pending,done,busy,route,
            atomic_load_explicit(&f->route.completed,memory_order_acquire),(unsigned)f->route.jobs,
            f->pass_sealed,f->pass_committed,f->pass_finalized,call.called);
        message(line);
        for (unsigned owner=0;owner<JC_CLOTH_PARTICIPANTS;++owner)
            for (unsigned role=0;role<7;++role) if (f->pass_roles[pass]&(1u<<role)) {
                const struct jc_cloth_partition_range *range=&f->route.plan.ranges[owner][role];
                if (!range->jobs) continue;
                snprintf(line,sizeof(line),"[JC] cloth finish counters owner=%u role=%u vertices=%u/%u indices=%u/%u jobs=%u",
                    owner,role,jc_cloth_u32(f->route.args[owner].bytes+0x210+4*role),
                    range->vertex_begin+range->vertices,jc_cloth_u32(f->route.args[owner].bytes+0x22c+4*role),
                    range->index_begin+range->indices,range->jobs);
                message(line);
            }
        cloth_maps.finish_failed=1;
        s->failed=1; f->phase=JC_FRAME_UNKNOWN;cloth_participant_ready=0;
        atomic_store_explicit(&f->route.active_roles,0,memory_order_release);
        if (traveler_actor)
            traveler_actor_report(jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE));
        JCProbeActorTrial[0]=-1;JCProbeActorTrial[1]=JC_ACTOR_RUNTIME_SCOPE;
        /* Native completion and private publication are separate contracts.
           Keep all private storage pinned; never replace the native result with
           an incomplete result that its caller will poll forever. */
        return call.called ? call.returned : original_cloth_finish(barn,result,pass,fourth);
    }
    return call.returned;
}

typedef unsigned UINT;
enum { WM_CLOSE=16, INVALID_HANDLE_VALUE=-1 };
static int logfile=1,terminated,flushed,terminate_ok=1;
static int FlushFileBuffers(int file) { assert(file==1);++flushed;return 1; }
static int TerminateProcess(void *process,unsigned code) {
 assert(process==GetCurrentProcess() && code==0xe04a4301u);++terminated;return terminate_ok;
}
static int cloth_quarantined_close(UINT msg) {
    if (msg!=WM_CLOSE || !active_cloth_backend || !active_cloth_backend->failed) return 0;
    message("[JC] user requested close after cloth quarantine; terminating this Journey process, exit e04a4301");
    if (logfile!=INVALID_HANDLE_VALUE) FlushFileBuffers(logfile);
    if (!TerminateProcess(GetCurrentProcess(),0xe04a4301u))
        message("[JC] quarantined Journey process termination failed");
    return 1;
}

static void start_case(unsigned fail) {
 setup(current,0);current->fail=fail;
 memset(&backend,0,sizeof(backend));memset(&cloth_maps,0,sizeof(cloth_maps));memset(&cloth_producer,0,sizeof(cloth_producer));
 memset((void *)JCProbeCompanionFeatures,0,sizeof(JCProbeCompanionFeatures));memset(job_object,0,sizeof(job_object));
 cloth_participant=current->participant;cloth_participant_ready=1;cloth_production_thread=7;cloth_production_pass=0;cloth_production_serial=0;
 active_cloth_backend=&backend;owner_ok=1;dependencies=stock_workers=routed_workers=original_finishes=seals=commits=finalizes=unmaps=0;flip_busy=write_fail=0;
}
static void run_worker(unsigned slot) {
 unsigned char *entry=current->barn+0x669d10+768*16+(slot-1)*16;
 cloth_worker_observer(slot&1,NULL,entry,current->barn+0x669ac0,current->barn+0x669960,NULL,current->barn+0x270080+slot*0xe0);
}
int main(void) {
 current=calloc(1,sizeof(*current));assert(current);unsigned scenarios=0;
 start_case(0);cleanup(current);setup(current,1);cloth_participant=current->participant;
 cloth_maps.roles=0;cloth_participant_ready=1;cloth_production_thread=0;cloth_production_pass=0;
 launch_second_pass=1;unsigned char second_result[16];
 assert(cloth_finish_observer((uintptr_t)current->barn,second_result,0,99)==(uintptr_t)second_result);
 assert(cloth_producer.adopted && cloth_producer.pass==1 && !cloth_participant_ready);
 assert(!cloth_production_thread && !cloth_production_pass);cleanup(current);++scenarios;
 for(unsigned fail=4;fail<=8;++fail) {
  if(fail==5)continue;start_case(fail);
  assert(cloth_dependency_observer((uintptr_t)current->barn+0x6bce00)==123);
  assert(dependencies==1&&!backend.failed&&JCProbeCompanionFeatures[30]==1&&!cloth_participant_ready);
  run_worker(1);run_worker(2);run_worker(3);run_worker(4);
  assert(stock_workers==2&&!routed_workers&&JCProbeCompanionFeatures[31]==2);
  assert(cloth_producer.lease.state==JC_CLOTH_LIVE);
  current->dude[0x81]=0;run_worker(1);
  assert(stock_workers==3&&JCProbeCompanionFeatures[31]==2);
  cleanup(current);++scenarios;
 }
 start_case(9);assert(cloth_dependency_observer((uintptr_t)current->barn+0x6bce00)==0);
 assert(!dependencies&&backend.failed&&cloth_producer.quarantined&&!JCProbeCompanionFeatures[30]);cleanup(current);++scenarios;
 start_case(0);assert(cloth_dependency_observer((uintptr_t)current->barn+0x6bce00)==123);
 assert(dependencies==1&&cloth_maps.launched_roles==cloth_maps.roles&&!backend.failed);
 run_worker(1);run_worker(2);assert(routed_workers==2&&!stock_workers&&!JCProbeCompanionFeatures[31]);cleanup(current);++scenarios;
 start_case(4);active_cloth_backend=NULL;run_worker(1);assert(stock_workers==1&&!JCProbeCompanionFeatures[31]);cleanup(current);++scenarios;
 start_case(4);cloth_participant.dude=0;run_worker(1);assert(stock_workers==1&&!JCProbeCompanionFeatures[31]);cleanup(current);++scenarios;
 for(unsigned variant=0;variant<3;++variant) {
  start_case(0);cloth_participant_ready=0;cloth_maps.roles=0x30;cloth_maps.barn=(uintptr_t)current->barn;cloth_maps.generation=1;
  struct jc_cloth_frame *frame=backend.frames.frames;frame->generation=1;frame->phase=JC_FRAME_WRITING;
  frame->pass_required=1;frame->pass_roles[0]=0x30;frame->source_args=current->barn+0x669ac0;
  current->barn[0x6bcf84]=1;put64(current->barn+0x6bcef0,(uintptr_t)job_object);job_object[0x58]=1;
  flip_busy=variant!=0;write_fail=variant==2;
  unsigned char result[16];memset(result,0xcd,sizeof(result));
  assert(cloth_finish_observer((uintptr_t)current->barn,result,0,99)==(uintptr_t)result);
  assert(original_finishes==(variant==2)&&!seals&&!commits);
  if(variant<2) {
   uint64_t pending_job;memcpy(&pending_job,result+8,8);
   assert(!result[0]&&!backend.failed&&pending_job==(uintptr_t)job_object);
   job_object[0x58]=0;flip_busy=0;
   assert(cloth_finish_observer((uintptr_t)current->barn,result,0,99)==(uintptr_t)result);
   assert(original_finishes==1&&seals==1&&commits==1&&finalizes==1&&unmaps==1&&result[0]==1);
  } else assert(backend.failed&&frame->phase==JC_FRAME_UNKNOWN);
  cleanup(current);++scenarios;
 }
 active_cloth_backend=NULL;assert(!cloth_quarantined_close(WM_CLOSE) && !terminated);
 active_cloth_backend=&backend;backend.failed=0;assert(!cloth_quarantined_close(WM_CLOSE) && !terminated);
 backend.failed=1;assert(!cloth_quarantined_close(0) && !terminated);
 assert(cloth_quarantined_close(WM_CLOSE) && terminated==1 && flushed==1);
 terminate_ok=0;assert(cloth_quarantined_close(WM_CLOSE) && terminated==2 && flushed==2);
 puts("Quarantined close: only an explicit close on the failed current process requests termination; normal close/messages remain native; OS calls simulated");
 free(current);printf("Production cloth integration: %u ordinary decline, stock preservation, quarantine, owned routing and native busy-result race scenarios passed.\n",scenarios);
}
