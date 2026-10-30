#include "native_chirp_charge.h"
#include "actor_code.h"
#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "native_actor_runtime.h"
static struct jc_actor_runtime runtime,*traveler_actor=&runtime;
static uint64_t traveler_barn,traveler_game;
static unsigned traveler_local_state=2,JNETFrames=1,thread_id=1;
static struct {uint64_t event,peer,generation;uint32_t counter;} traveler_chirp_pending[16];
static uint64_t thread(void *ctx){(void)ctx;return thread_id;}
static int traveler_context(void){return 1;}
static int own_read(void *ctx,uint64_t address,void *out,size_t n){(void)ctx;memcpy(out,(void *)(uintptr_t)address,n);return 1;}
static int jc_local_windows_write(uint64_t address,const void *value,size_t n){memcpy((void *)(uintptr_t)address,value,n);return 1;}
static int traveler_peer_fault(int code){assert(!code);return 0;}
static unsigned jnet_diagnostic_logs;
static void message(const char *s){assert(s && *s);}
#include "traveler_chirp_charge.h"
static void put(void *p,unsigned offset,uint64_t value){memcpy((unsigned char *)p+offset,&value,8);}
int main(void) {
    unsigned char *image=mmap(NULL,0x4000000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_code(image);load_runtime_constants(image);
    assert(!mprotect(image,0x690000,PROT_READ|PROT_EXEC));
    unsigned char pool[0x718]={0},before[0x718],out[0x38];unsigned owners[16]={0};uint32_t count=1;
    memcpy(pool+0x710,&count,4);pool[0x68]=1;pool[0x4c]=1;
    jc_receiver_store(pool+0x40,100.f);jc_receiver_store(pool+0x44,1.f);
    unsigned hits=0;float hit_age=0,hit_x=0;
    for(unsigned tick=0;tick<60;++tick)for(unsigned distance=0;distance<100;++distance) {
        float position[4]={(float)distance,0,0,0},dt=1.f/60;
        jc_receiver_store(pool+0x54,tick*dt);memcpy(before,pool,sizeof(pool));
        int native=((unsigned char (JC_CHARGE_ABI *)(uintptr_t,const float *,float,unsigned char,unsigned char,void *))
            (image+0x140790))((uintptr_t)pool,position,dt,1,0,out)!=0;
        if(native && !hits){hit_age=tick*dt;hit_x=(float)distance;}
        hits+=native;
        for(unsigned source=0;source<=3;++source)for(unsigned recipient=0;recipient<3;++recipient) {
            owners[0]=source;
            int eligible=source && source!=recipient+1 && (recipient==2 || source==3);
            assert(jc_chirp_charge_hit((uintptr_t)image,pool,owners,recipient,position,dt)==(native && eligible));
        }
        assert(!memcmp(before,pool,sizeof(pool)));
    }
    assert(hits);
    float position[4]={0};owners[0]=3;
    count=17;memcpy(pool+0x710,&count,4);
    assert(!jc_chirp_charge_hit((uintptr_t)image,pool,owners,0,position,.016f));
    count=1;memcpy(pool+0x710,&count,4);jc_receiver_store(pool+0x20,NAN);
    assert(!jc_chirp_charge_hit((uintptr_t)image,pool,owners,0,position,.016f));
    struct jc_chirp_charge charge={0};
    assert(jc_chirp_charge_tick(&charge,2,1,1,1,.01f));
    assert(!jc_chirp_charge_tick(&charge,2,2,1,0,.01f));
    assert(jc_chirp_charge_tick(&charge,0,2,1,1,.01f));
    for(unsigned i=0;i<130;++i)jc_chirp_charge_tick(&charge,0,2,1,0,.01f);
    assert(!charge.remaining[0]);
    assert(jc_chirp_charge_tick(&charge,1,2,1,1,.01f));
    assert(!jc_chirp_charge_tick(&charge,1,2,0,0,.01f));
    unsigned char *barn=calloc(1,JC_RECEIVER_BYTES),*actor=calloc(1,0x14640),game[0x200]={0},clock[0x20]={0};
    assert(barn && actor);put(game,0x150,(uintptr_t)clock);traveler_barn=(uintptr_t)barn;traveler_game=(uintptr_t)game;
    put(game,0x1e0,traveler_barn);put(game,0x1b8,(uintptr_t)pool);put(game,0x68,(uintptr_t)clock);
    jc_receiver_store(clock+0x10,1.f/60);put(barn,0,(uintptr_t)image+0x5da340);
    put(barn,0xb0,(uintptr_t)image+0x5d0380);put(barn,0x146f0,(uintptr_t)image+0x5d0380);
    put(actor,0,(uintptr_t)image+0x5d0380);put(actor,0xa620,(uintptr_t)actor+0x160);put(actor,0x2240,123456);
    barn[0xa2]=1;
    runtime.identity=(struct jc_actor_identity){(uintptr_t)image,traveler_game,traveler_barn,(uintptr_t)actor,123456,9,1};
    runtime.owner=1;runtime.ops.thread=thread;runtime.state=JC_ACTOR_RUNTIME_ACTIVE;
    runtime.input_frame=JNETFrames;
    for(unsigned i=0;i<3;++i)jc_receiver_store((i==0?barn+0xb0:i==1?barn+0x146f0:actor)+0x10,hit_x);
    jc_receiver_store(pool+0x20,0);jc_receiver_store(pool+0x54,hit_age);pool[0x4c]=0;
    traveler_chirp_pending[0].event=(uintptr_t)pool+0x10;traveler_chirp_pending[0].peer=9;traveler_chirp_pending[0].generation=1;
    uint32_t scarf[2]={0,8};memcpy(barn+0x118,scarf,8);
    traveler_charge_before_update(traveler_barn,traveler_game);
    memcpy(scarf,barn+0x118,8);assert(scarf[0]==1 && scarf[1]==8);
    assert(barn[0x100]==1 && barn[0x14740]==1 && !actor[0x50]);
    assert(jc_receiver_float(barn+0x14f8)==1.f && jc_receiver_float(barn+0x15b38)==1.f);
    traveler_charge_after_update();assert(!barn[0x100] && !barn[0x14740]);
    traveler_charge_before_update(traveler_barn,traveler_game);
    memcpy(scarf,barn+0x118,8);assert(scarf[0]==1);
    traveler_charge_after_update();
    traveler_charge_cleanup(traveler_barn+0xb0);assert(!traveler_charge.hits[0]);
    traveler_chirp_pending[0].counter=1;
    traveler_charge_before_update(traveler_barn,traveler_game);assert(!barn[0x100] && !barn[0x14740]);
    traveler_charge_after_update();
    pool[0x4c]=1;traveler_charge_before_update(traveler_barn,traveler_game);
    assert(actor[0x50]==1 && jc_receiver_float(actor+0x1448)==1.f);
    traveler_charge_after_update();assert(!actor[0x50]);
    traveler_charge_cleanup(traveler_barn+0xb0);
    count=0;memcpy(pool+0x710,&count,4);barn[0xb0+0x1e69]=1;
    for(unsigned i=0;i<30;++i) {
        traveler_charge_before_update(traveler_barn,traveler_game);
        assert(barn[0x100] && actor[0x50]);
        traveler_charge_after_update();
    }
    memcpy(scarf,barn+0x118,8);assert(scarf[0]>1 && scarf[0]<=8);
    assert(jc_receiver_float(barn+0x14f8)==1.f);
    runtime.input_frame=JNETFrames-1;traveler_charge_before_update(traveler_barn,traveler_game);
    assert(!barn[0x100] && !actor[0x50] && !traveler_charge.touch_elapsed);
    runtime.input_frame=JNETFrames;
    runtime.departing=1;traveler_charge_before_update(traveler_barn,traveler_game);
    assert(!barn[0x100] && !actor[0x50]);runtime.departing=0;
    assert(!jc_charge_touch((float[3]){NAN,0,0},(float[3]){0},0));
    assert(!jc_charge_touch((float[3]){1,0,0},(float[3]){0},0));
    traveler_charge_cleanup(traveler_barn+0xb0);thread_id=2;
    traveler_charge_before_update(traveler_barn,traveler_game);assert(!actor[0x50]);
    free(actor);free(barn);munmap(image,0x4000000);
    puts("Chirp charge: pinned native bubble intersection matched across 6000 age/distance samples and all sender/recipient routes; self/environment exclusion, bounds, expiry and generation reset passed. Production adapter recipient flags/glow, flag restoration, stale event rejection, chapter reset, touch glow, bounded native scarf refill, one refill per chirp and wrong-thread refusal passed. Gameplay recharge/glow remain runtime checks.");
}
