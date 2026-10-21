#include "native_actor_departure.h"
#include "native_actor_runtime.h"
#include "native_peer_lifecycle.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "actor_code.h"
#define ABI __attribute__((ms_abi))
static unsigned audio_calls,writes;
static uint64_t denied_read;
static int read_memory(void *ctx,uint64_t p,void *out,size_t n) {
    (void)ctx;if(p==denied_read)return 0;memcpy(out,(void *)(uintptr_t)p,n);return 1;
}
static int write_memory(void *ctx,uint64_t p,const void *value,size_t n) {
    uint64_t actor=(uintptr_t)ctx;
    assert(p>=actor && p+n<=actor+0x14640);++writes;memcpy((void *)(uintptr_t)p,value,n);return 1;
}
static void put(void *p,unsigned offset,uint64_t value) {memcpy((char *)p+offset,&value,8);}
static void ABI stop_audio(unsigned handle,float seconds) {assert(handle==123 && seconds==.5f);++audio_calls;}
static char *ABI copy_name(char *dst,const char *src,size_t n) {return strncpy(dst,src,n);}
#define __declspec(x)
#define HEAP_ZERO_MEMORY 0
#define JC_CLOTH_FRAMES 4
#define JC_CLOTH_BUFFERS 4
#define JC_CHIRP_SLOTS 16
typedef int32_t LONG;
struct jc_environment_probe {uint64_t peer,generation;unsigned pins,quarantined,produced;};
struct jc_cloth_live_material {uint64_t image,peer,generation;unsigned role,ready,quarantined,pins,atlas_outfit;void *atlas_holders;struct {unsigned pins;} atlas_snapshots[6];};
struct traveler_live_storage {
    struct {
        struct {unsigned pins,mapped;} buffers[JC_CLOTH_BUFFERS];
        struct jc_environment_probe probes[2];
        struct {unsigned closing;} frames;
        unsigned failed;
    } backend;
    struct jc_cloth_live_material materials[JC_CLOTH_FRAMES][2];
    uint64_t peer,generation;
};
static struct traveler_live_storage *traveler_live;
static struct jc_actor_runtime *traveler_actor;
static uint64_t traveler_object,traveler_game;
static unsigned traveler_peer_depart_requested,traveler_peer_restart_requested,traveler_actor_trial_draining;
static LONG JNETActorTrial[5];
static unsigned cloth_participant_ready,cloth_participant,traveler_chirp_glyph,traveler_actor_materials;
static struct {unsigned quarantined;} cloth_producer;
static struct {uint64_t event,peer,generation;uint32_t counter;} traveler_chirp_pending[JC_CHIRP_SLOTS];
static uint64_t now;
static unsigned retired=1,heaps,allocations;
static LONG InterlockedExchange(volatile LONG *p,LONG v) {LONG old=*p;*p=v;return old;}
static uint64_t GetTickCount64(void) {return now;}
static int GetProcessHeap(void) {return 1;}
static void *HeapAlloc(int heap,int flags,size_t n) {(void)heap;(void)flags;++heaps;++allocations;return calloc(1,n);}
static void HeapFree(int heap,int flags,void *p) {(void)heap;(void)flags;assert(heaps);--heaps;free(p);}
static void message(const char *p) {assert(p && *p);}
static int traveler_context(void) {return 1;}
static uint64_t traveler_actor_thread(void *ctx) {(void)ctx;return 1;}
static void traveler_actor_report(int code) {(void)code;}
static int own_read(void *ctx,uint64_t p,void *out,size_t n) {return read_memory(ctx,p,out,n);}
static int jc_local_windows_write(uint64_t p,const void *v,size_t n) {memcpy((void *)(uintptr_t)p,v,n);return 1;}
static int traveler_live_identity(const struct jc_actor_identity *id) {
    return id->actor==traveler_object && id->peer==traveler_live->peer && id->generation==traveler_live->generation;
}
static int traveler_live_retired(void *ctx,const struct jc_actor_identity *id) {(void)ctx;return traveler_live_identity(id) && retired;}
static int fixture_ready(void *ctx,const struct jc_actor_identity *id) {(void)ctx;return traveler_live_identity(id) && !traveler_live->backend.frames.closing;}
static int fixture_prepare(void *ctx,const struct jc_actor_identity *id,uint64_t f) {(void)ctx;(void)id;(void)f;return 1;}
static int fixture_begin(void *ctx,const struct jc_actor_identity *id,unsigned phase,uint64_t f,uint64_t r) {
    (void)ctx;(void)id;(void)phase;(void)f;(void)r;return 1;
}
static int fixture_end(void *ctx,const struct jc_actor_identity *id,unsigned phase,uint64_t f,int success) {
    (void)ctx;(void)id;(void)phase;(void)f;return success;
}
static void fixture_drain(void *ctx,const struct jc_actor_identity *id) {(void)ctx;(void)id;traveler_live->backend.frames.closing=1;}
static int JNETActorStop(void) {return jc_actor_runtime_stop(traveler_actor);}
#define jc_actor_material_independent(read,ctx,id,proof) 0
#define jc_cloth_live_owner(backend) 1
#define jc_environment_probe_check(read,ctx,probe) 0
static uint64_t traveler_actor_trial_started;
static uint32_t traveler_actor_trial_outfit=UINT32_MAX,traveler_actor_admission_outfit=UINT32_MAX;
static unsigned atlas_releases;
#define JC_FRAME_OWNERSHIP 90
struct jc_cloth_graphics_token {unsigned held;};
static unsigned graphics_locked;
static int jc_cloth_live_graphics_enter(void *backend,struct jc_cloth_graphics_token *t) {
    assert(backend==&traveler_live->backend && !graphics_locked);graphics_locked=t->held=1;return 1;
}
static int jc_cloth_live_graphics_leave(void *backend,struct jc_cloth_graphics_token *t) {
    assert(backend==&traveler_live->backend && graphics_locked && t->held);graphics_locked=t->held=0;return 1;
}
static int jc_cloth_live_material_destroy(int (*read)(void *,uint64_t,void *,size_t),void *ctx,struct jc_cloth_live_material *m) {
    (void)read;(void)ctx;assert(!m->pins && graphics_locked && m->ready);
    memset(m,0,sizeof(*m));++atlas_releases;return 0;
}
static int jc_cloth_live_material_create(int (*read)(void *,uint64_t,void *,size_t),void *ctx,uint64_t image,
        uint64_t barn,uint64_t manager,unsigned role,uint64_t peer,uint64_t generation,
        struct jc_cloth_live_material *m) {
    (void)read;(void)ctx;assert(graphics_locked && !m->ready && !m->atlas_holders && barn && manager);
    *m=(struct jc_cloth_live_material){.image=image,.role=role,.peer=peer,.generation=generation,.ready=1};return 0;
}
#include "material_reset_fixture.h"
static unsigned repeat_start_calls,repeat_stop_calls;
static int repeat_start_result;
static int JNETActorTrialStart(void) {
    ++repeat_start_calls;
    if(!repeat_start_result)traveler_peer_restart_requested=1;
    return repeat_start_result;
}
static void traveler_actor_trial_stop(int cause) {
    assert(!cause);++repeat_stop_calls;traveler_peer_depart_requested=1;
}
static int cloth_actor_growth_stop(void){return 1;}
#include "traveler_peer_lifecycle.h"
static void lifecycle_cases(const struct jc_actor_identity *id) {
    unsigned char shout[0x710]={0},clock[0x20]={0},glyph[0x710]={0};
    struct traveler_live_storage storage={0};traveler_live=&storage;
    traveler_object=id->actor;traveler_game=id->game;
    put((void *)(uintptr_t)id->game,0x108,1);put((void *)(uintptr_t)id->game,0x28,2);
    put((void *)(uintptr_t)id->actor,0x2240,(uintptr_t)glyph);
    put((void *)(uintptr_t)id->game,0x1b8,(uintptr_t)shout);
    put((void *)(uintptr_t)id->game,0x68,(uintptr_t)clock);jc_receiver_store(clock+0x10,1.f/60);
    struct jc_actor_runtime_ops ops={own_read,write_memory,traveler_live_retired,traveler_actor_thread,
        fixture_ready,fixture_prepare,fixture_begin,fixture_end,fixture_drain,NULL};
    traveler_actor=HeapAlloc(1,0,sizeof(*traveler_actor));
    struct jc_actor_identity current=*id;current.glyph=(uintptr_t)glyph;
    storage.peer=current.peer;storage.generation=current.generation;
    for(unsigned f=0;f<JC_CLOTH_FRAMES;++f)for(unsigned n=0;n<2;++n)
        storage.materials[f][n]=(struct jc_cloth_live_material){.image=id->image,.role=4+n,.peer=current.peer,.generation=current.generation,.ready=1};
    storage.backend.probes[0]=(struct jc_environment_probe){current.peer,current.generation,0,0,1};
    assert(!jc_actor_runtime_bind(traveler_actor,&current,&ops));
    unsigned char expected_gate=1;glyph[0x70c]=expected_gate;
    glyph[0x70d]=0x12;glyph[0x70e]=0x34;glyph[0x70f]=0x56;
    for(unsigned cycle=0;cycle<128;++cycle) {
        traveler_actor->state=JC_ACTOR_RUNTIME_ACTIVE;
        assert(!traveler_peer_admit(current.peer,current.generation));
        assert(glyph[0x70c]==expected_gate);
        assert(glyph[0x70d]==0x12 && glyph[0x70e]==0x34 && glyph[0x70f]==0x56);
        assert(!traveler_peer_vacant());
        put((void *)(uintptr_t)traveler_departure.effects[0],0xc0,1);
        traveler_peer_depart_requested=cycle%2?2:1;
        retired=0;assert(!traveler_peer_before_input() && traveler_peer_depart_requested);
        retired=1;assert(traveler_peer_before_input());
        assert(traveler_actor->departing && traveler_actor->input.inactive);
        if(!(cycle%2)) {
            assert(traveler_peer.phase==JC_PEER_READY_TO_MED);
            uint64_t c=traveler_object+0xa390;
            put((void *)(uintptr_t)c,0x64a8,c+0x33e0);*((unsigned char *)(uintptr_t)c+0x64cd)=1;
            ++now;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_MED);
            ++now;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_MED_WAIT);
            now+=1999;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_MED_WAIT);
            ++now;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_DUST);
            for(unsigned frame=0;frame<601;++frame)traveler_peer_after_input();
            traveler_peer_after_update();
            jc_receiver_store((void *)(uintptr_t)(traveler_object+0x2000),1);
            ++now;assert(traveler_peer_before_input());
        }
        assert(traveler_peer.phase==JC_PEER_FADING && traveler_actor->state==JC_ACTOR_RUNTIME_RETIRED);
        uint64_t effect=traveler_departure.effects[0];put((void *)(uintptr_t)effect,0xc0,1);
        ++now;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_FADING);
        assert(traveler_peer_replace(current.peer+1,current.generation+1)==JC_ACTOR_RUNTIME_BUSY);
        put((void *)(uintptr_t)effect,0xc0,0);
        traveler_chirp_pending[0].event=(uintptr_t)shout+0x10;
        traveler_chirp_pending[0].peer=current.peer;traveler_chirp_pending[0].generation=current.generation;
        traveler_chirp_pending[0].counter=7;shout[0x68]=1;unsigned counter=7;memcpy(shout+0x50,&counter,4);
        ++now;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_FADING);
        shout[0x68]=0;++now;assert(traveler_peer_before_input() && traveler_peer.phase==JC_PEER_DRAINING);
        expected_gate=cycle%7?1:0;glyph[0x70c]=expected_gate;++now;assert(traveler_peer_before_input() && traveler_peer_vacant() && !glyph[0x70c]);
        assert(!traveler_chirp_pending[0].event && JNETActorTrial[0]==3);
        assert(traveler_peer_replace(current.peer,current.generation)==JC_ACTOR_RUNTIME_BUSY);
        traveler_actor_admission_outfit=cycle%7;
        storage.materials[0][0].atlas_outfit=(cycle+1)%7;
        storage.materials[0][0].atlas_holders=&storage;
        unsigned released=atlas_releases;
        retired=0;
        storage.materials[0][0].pins=1;
        assert(traveler_live_material_rebuild(&storage.materials[0][0])==JC_FRAME_OWNERSHIP);
        storage.materials[0][0].pins=0;storage.materials[0][0].atlas_snapshots[0].pins=1;
        assert(traveler_live_material_rebuild(&storage.materials[0][0])==JC_FRAME_OWNERSHIP);
        storage.materials[0][0].atlas_snapshots[0].pins=0;
        assert(!traveler_live_material_rebuild(&storage.materials[0][0]));
        assert(atlas_releases==released+1 && !storage.materials[0][0].atlas_holders);
        retired=1;storage.materials[0][0].atlas_holders=&storage;
        storage.materials[0][0].atlas_outfit=(cycle+1)%7;released=atlas_releases;
        storage.materials[0][0].pins=1;
        assert(traveler_peer_replace(current.peer+1,current.generation+1)==JC_ACTOR_QUARANTINED);
        storage.materials[0][0].pins=0;storage.backend.buffers[0].mapped=1;
        assert(traveler_peer_replace(current.peer+1,current.generation+1)==JC_ACTOR_RUNTIME_BUSY);
        assert(atlas_releases==released);
        storage.backend.buffers[0].mapped=0;
        ++current.peer;++current.generation;
        assert(!traveler_peer_replace(current.peer,current.generation) && heaps==1);
        assert(atlas_releases==released+1 && !storage.materials[0][0].atlas_holders);
        assert(traveler_actor->state==JC_ACTOR_RUNTIME_WAITING && !traveler_actor->departing);
        assert(storage.backend.probes[0].generation==current.generation && !storage.backend.probes[0].produced);
        assert(!storage.backend.frames.closing && storage.materials[0][0].peer==current.peer);
        assert(jc_receiver_float((void *)(uintptr_t)(traveler_object+0x2000))==0);
    }
    assert(traveler_peer.admissions==128 && traveler_peer.departures==128 && traveler_peer.abrupt_departures==64);
    assert(allocations==129);
    traveler_actor->state=JC_ACTOR_RUNTIME_RETIRED;JNETActorTrial[0]=3;
    assert(!traveler_repeat_start(0) && traveler_repeat.state==1 && traveler_peer_restart_requested);
    assert(traveler_repeat_start(0)==JC_ACTOR_RUNTIME_BUSY);
    unsigned starts=repeat_start_calls;
    for(unsigned cycle=0;cycle<10;++cycle) {
        traveler_peer_restart_requested=0;traveler_peer.phase=JC_PEER_ACTIVE;
        traveler_actor->state=JC_ACTOR_RUNTIME_ACTIVE;JNETActorTrial[0]=1;
        traveler_actor_trial_started=now;now+=14999;traveler_repeat_tick();
        assert(repeat_stop_calls==cycle && !traveler_peer_depart_requested);
        ++now;traveler_repeat_tick();assert(repeat_stop_calls==cycle+1 && traveler_peer_depart_requested==1);
        traveler_peer_depart_requested=0;traveler_peer.phase=JC_PEER_FADING;
        now+=60000;traveler_repeat_tick();assert(repeat_start_calls==starts);
        traveler_peer.phase=JC_PEER_VACANT;++traveler_peer.identity.generation;
        traveler_actor->state=JC_ACTOR_RUNTIME_RETIRED;JNETActorTrial[0]=3;
        traveler_repeat_tick();assert(traveler_repeat.completed==cycle+1);
        if(cycle<9){++starts;assert(repeat_start_calls==starts && traveler_peer_restart_requested);}
        else assert(traveler_repeat.state==2 && !traveler_peer_restart_requested && repeat_start_calls==starts);
    }
    traveler_peer_report();assert(JNETPeerLifecycle[8]==2 && JNETPeerLifecycle[9]==10);
    assert(!traveler_repeat_start(0));traveler_repeat_cancel();
    assert(traveler_repeat.state==3 && !traveler_peer_restart_requested);
    starts=repeat_start_calls;now+=10000;traveler_repeat_tick();assert(repeat_start_calls==starts);
    assert(!traveler_repeat_start(0));++traveler_peer.abrupt_departures;traveler_repeat_tick();
    assert(traveler_repeat.state==-1 && traveler_repeat.result==JC_PEER_FAILED && !traveler_peer_restart_requested);
    assert(!traveler_repeat_start(1) && traveler_repeat.target==7);
    for(unsigned outfit=0;outfit<7;++outfit) {
        assert(traveler_repeat_outfit()==outfit);
        traveler_peer_restart_requested=0;traveler_peer.phase=JC_PEER_VACANT;
        ++traveler_peer.identity.generation;
        traveler_repeat_tick();now+=2000;traveler_repeat_tick();
        assert(traveler_repeat.completed==outfit+1);
    }
    assert(traveler_repeat.state==2 && traveler_repeat_outfit()==UINT32_MAX);
    assert(!traveler_repeat_start(1));traveler_repeat_cancel();
    assert(traveler_repeat_outfit()==UINT32_MAX);
    repeat_start_result=JC_ACTOR_RUNTIME_ARGUMENT;
    assert(traveler_repeat_start(0)==JC_ACTOR_RUNTIME_ARGUMENT && traveler_repeat.state==-1);
    HeapFree(1,0,traveler_actor);traveler_actor=NULL;
    puts("Repeat scheduler: ten clean cycles and seven outfits, active/drain/pause timing, no early or eleventh admission, cancellation, emergency-fallback stop and admission failure passed; admission service mocked.");
    (void)traveler_actor_materials;
    puts("Production peer lifecycle: 128 alternating clean/abrupt cycles passed, with pending particles/chirps and pinned/mapped resources blocking replacement; generation reset, retired outfit material rebuild, glyph clearing and exact saved-gate restoration on admission (including disabled gates), adjacent bytes unchanged and balanced runtime allocations verified. Cloth/GPU retirement mocked; no Journey session executed.");
}
int main(void) {
    unsigned char *image=mmap(NULL,0x700000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_code(image);
    image[0x2b6e70]=0x48;image[0x2b6e71]=0xb8;put(image,0x2b6e72,(uintptr_t)stop_audio);
    image[0x2b6e7a]=0xff;image[0x2b6e7b]=0xe0;put(image,0x553d98,(uintptr_t)copy_name);
    assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    unsigned char *actor=calloc(1,0x14640),*barn=calloc(1,0x29000),*snapshot=malloc(0x29000);
    unsigned char *sounds=calloc(1,0x40220),game[0x1e8]={0},effects[14][0x1c0]={{0}};
    assert(actor && barn && sounds && snapshot);
    put(game,0x1e0,(uintptr_t)barn);put(game,0x150,(uintptr_t)sounds);
    put(barn,0,(uintptr_t)image+0x5da340);put(actor,0,(uintptr_t)image+0x5d0380);
    put(actor,0xa620,(uintptr_t)actor+0x160);put(actor,0x2240,(uintptr_t)actor+0x1000);
    const char *names[]={"FootSandDot","FootSandSmoke","AddStrandTail","UseStrandTail","LoseScarf",
        "LoseScarfGlowing","ContrailGlow","DudeSandFall","DudeSandFallDot","DudeSandFallCrash",
        "DudeSandFallCrashDot","DudeDissolveDust","DudeDissolveChunk","DudeRollingInDirt"};
    for(unsigned i=0;i<14;++i) {
        uint64_t node=(uintptr_t)actor+jc_actor_emitter(i);
        put(actor,jc_actor_emitter(i),(uintptr_t)effects[i]);put(effects[i],0,node);put(effects[i],8,node);
        strcpy((char *)effects[i]+0x18,names[i]);actor[jc_actor_emitter(i)+0x84]=1;
        put(actor,0x1780+8*i,UINT64_MAX);put(barn,0xb0+0x1780+8*i,UINT64_MAX);
        put(barn,0x146f0+0x1780+8*i,UINT64_MAX);
    }
    struct jc_actor_identity id={(uintptr_t)image,(uintptr_t)game,(uintptr_t)barn,(uintptr_t)actor,
        (uintptr_t)actor+0x1000,77,1};
    struct jc_actor_departure s={0};
    assert(jc_actor_departure_capture(read_memory,actor,&id,&s));
    memcpy(snapshot,barn,0x29000);
    assert(jc_actor_departure_emitters(read_memory,write_memory,actor,&s,1));
    for(unsigned i=0;i<14;++i)assert(actor[jc_actor_emitter(i)+0x84]==(i==11 || i==12));
    assert(jc_actor_departure_particles_idle(read_memory,actor,&s)==0);
    assert(jc_actor_departure_emitters(read_memory,write_memory,actor,&s,0));
    put(effects[0],0xc0,0x1234);assert(jc_actor_departure_particles_idle(read_memory,actor,&s)==0);
    assert(*(uint64_t *)(effects[0]+0xc0)==0x1234);
    put(effects[0],0xc0,0);assert(jc_actor_departure_particles_idle(read_memory,actor,&s)==1);
    denied_read=(uintptr_t)effects[1]+0xc0;assert(jc_actor_departure_particles_idle(read_memory,actor,&s)==-1);denied_read=0;
    put(effects[0],8,(uintptr_t)barn);unsigned before=writes;
    assert(!jc_actor_departure_emitters(read_memory,write_memory,actor,&s,0) && writes==before);
    put(effects[0],8,(uintptr_t)actor+jc_actor_emitter(0));
    const uint64_t handle=(UINT64_C(7)<<32)|3;
    put(actor,0x1780,handle);put(sounds,3*0x200+0x214,7);sounds[3*0x200+0x218]=1;
    unsigned native=123;memcpy(sounds+3*0x200+0x1e0,&native,4);
    assert(jc_actor_departure_sounds(read_memory,actor,&s,0)==0 && !audio_calls);
    put(barn,0xb0+0x1780,handle);assert(jc_actor_departure_sounds(read_memory,actor,&s,1)==-1 && !audio_calls);
    put(barn,0xb0+0x1780,UINT64_MAX);
    assert(jc_actor_departure_sounds(read_memory,actor,&s,1)==1 && audio_calls==1);
    assert(jc_actor_departure_sounds(read_memory,actor,&s,1)==1 && audio_calls==1);
    sounds[3*0x200+0x218]=1;native=8;memcpy(sounds+3*0x200+0x214,&native,4);
    assert(jc_actor_departure_sounds(read_memory,actor,&s,1)==1 && audio_calls==1 && sounds[3*0x200+0x218]);
    put(actor,0x1780,512);assert(jc_actor_departure_sounds(read_memory,actor,&s,1)==-1);
    put(actor,0x1780,UINT64_MAX);
    unsigned char *controller=actor+0xa390;unsigned actions=2;
    memcpy(controller+0x64a0,&actions,4);strcpy((char *)controller+0x33c8,"ReadyToMed");
    strcpy((char *)controller+0x3408,"Med");jc_receiver_store(controller+0x33e8,1);
    assert(jc_actor_departure_animation(read_memory,write_memory,actor,&s,1));
    assert(*(uint32_t *)(controller+0x6734)==1 && !strcmp((char *)controller+0x6738,"ReadyToMed"));
    assert(jc_actor_departure_med(read_memory,actor,&s)==0);
    put(controller,0x64a8,(uintptr_t)controller+0x33e0);controller[0x64cd]=1;
    assert(jc_actor_departure_med(read_memory,actor,&s)==1);
    put(controller,0x64a8,1);assert(jc_actor_departure_med(read_memory,actor,&s)==-1);
    assert(jc_actor_departure_animation(read_memory,write_memory,actor,&s,0));
    assert(!controller[0x64cd] && !*(uint32_t *)(controller+0x6734));
    for(unsigned i=0;i<600;++i)assert(jc_actor_departure_input(read_memory,write_memory,actor,&s,1,1.f/60));
    assert(s.dissolve>.999f && s.dissolve<=1 && actor[0x56] && actor[0x1e1c] && !actor[0x50] && !actor[0x51]);
    before=writes;assert(!jc_actor_departure_input(read_memory,write_memory,actor,&s,1,NAN) && writes==before);
    assert(!memcmp(snapshot,barn,0x29000));
    lifecycle_cases(&id);assert(!memcmp(snapshot,barn,0x29000));
    free(actor);free(barn);free(sounds);free(snapshot);assert(!munmap(image,0x700000));
    puts("Native departure: original ReadyToMed queue, Med query and sound-stop code executed; dust timing, draining particles, stale/aliased sounds, invalid animation pointers and unchanged native players passed. Animation progression and audio output remain runtime checks.");
}
