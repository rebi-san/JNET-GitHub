#include <assert.h>
#include <stdio.h>
#include "native_companion_chirp.h"
static unsigned char game_data[0x200],barn_data[JC_CHIRP_BARN_BYTES],sound_data[0x7c7b8],code_data[16],trampoline[24];
static const uint64_t game=0x100000,barn=0x200000,sounds=0x300000,image=0x140000000;
static unsigned reads,mutate,fail,deny;
static struct jc_chirp_capture capture;
static struct jc_chirp_batch batch;
static struct jc_chirp_bridge bridge;
static void u32(unsigned char *p,uint32_t n) { memcpy(p,&n,4); }
static void u64(unsigned char *p,uint64_t n) { memcpy(p,&n,8); }
static void f32(unsigned char *p,float n) { memcpy(p,&n,4); }
static unsigned char *map(uint64_t at,size_t n) {
    struct { uint64_t at; unsigned char *p; size_t n; } ranges[]={
        {game,game_data,sizeof(game_data)},{barn,barn_data,sizeof(barn_data)},
        {image+0x100,trampoline,sizeof(trampoline)},{sounds,sound_data,sizeof(sound_data)},{image+0x13fbb0,code_data,sizeof(code_data)}};
    for (unsigned i=0;i<5;++i) if (at>=ranges[i].at && n<=ranges[i].n && at-ranges[i].at<=ranges[i].n-n)
        return ranges[i].p+at-ranges[i].at;
    return NULL;
}
static int read_memory(void *ctx,uint64_t at,void *out,size_t n) {
    (void)ctx; ++reads;
    if (fail && reads==fail) return 0;
    if (mutate && reads==mutate) ++barn_data[0x714];
    unsigned char *p=map(at,n); if (!p) return 0; memcpy(out,p,n); return 1;
}
static int writable(void *ctx,uint64_t at,size_t n) { (void)ctx; return !deny && at==barn && n==sizeof(barn_data); }
static void fixture(void) {
    memset(game_data,0,sizeof(game_data)); memset(barn_data,0,sizeof(barn_data)); memset(sound_data,0,sizeof(sound_data));
    memset(&capture,0,sizeof(capture)); memset(&batch,0,sizeof(batch)); reads=mutate=fail=deny=0;
    u64(game_data+0x1b8,barn); u64(game_data+0x150,sounds);
    u32(sound_data+0x7c7b4,8);
    for (unsigned i=0;i<8;++i) strcpy((char *)sound_data+0x70234+i*0x13c,jc_chirp_sound_name(i%4+1,i>=4));
    const unsigned char code[]={0x40,0x56,0x57,0x48,0x81,0xec,0xa8,0,0,0,0x33,0xc0,0x48,0x8b,0xf9,0x48};
    memcpy(code_data,code,16);
}
static void shout(unsigned slot,unsigned id,unsigned local,unsigned remote,unsigned sound) {
    unsigned char *s=barn_data+slot*0x70;
    f32(s+0x20,10); f32(s+0x24,20); f32(s+0x28,30); f32(s+0x2c,1);
    f32(s+0x30,1); f32(s+0x40,3); f32(s+0x44,0.5f);
    u32(s+0x48,sound); s[0x4c]=(unsigned char)local; s[0x4d]=(unsigned char)remote;
    u32(s+0x50,id); s[0x68]=1; u32(barn_data+0x714,id+1);
}
static int poll(uint64_t frame) { return jc_chirp_capture_local(read_memory,NULL,game,frame,101,7,&capture,&batch); }
static void ready(void) { fixture(); assert(!poll(1) && !batch.count); shout(3,0,1,0,1); assert(!poll(2) && batch.count==1); }
static struct jc_chirp_event extra(unsigned participant) {
    struct jc_chirp_event e=batch.source[0].event; e.participant=participant; e.position[0]+=6; return e;
}
static int prepare(const struct jc_chirp_event *e,uint64_t frame) {
    return jc_chirp_bridge_prepare(read_memory,writable,NULL,image,game,frame,&batch,0,e,&bridge);
}
static void native_stub(const struct jc_chirp_bridge *b) {
    assert(((uintptr_t)b->input&15)==0 && !b->input[0x2c] && b->input[0x2d]<=1);
    unsigned char *s=barn_data+b->slot*0x70;
    s[0x68]=s[0x78]=1; memcpy(s+0x20,b->input,0x2e);
    u32(s+0x50,jc_chirp_u32(barn_data+0x714)); memcpy(s+0x54,b->input+0x34,4);
    u32(barn_data+0x714,jc_chirp_u32(barn_data+0x714)+1);
}
static unsigned relay_emits,break_post;
static void relay_emit(void *ctx,const struct jc_chirp_bridge *b) {
    (void)ctx;++relay_emits;native_stub(b);
    if (break_post) barn_data[0x700]^=1;
}
static void relay_cases(void) {
    struct jc_chirp_relay relay={0};const float head[3]={24,27,32};
    fixture();relay_emits=break_post=0;
    assert(!jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,1,201,7,head,&relay));
    assert(!relay_emits);
    shout(3,0,1,0,1);
    assert(!jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,2,201,7,head,&relay));
    assert(relay_emits==1 && relay.emitted==1 && !memcmp(barn_data+0x20,head,12));
    assert(!barn_data[0x4c] && !barn_data[0x4d] && jc_chirp_u32(barn_data+0x48)==5);
    assert(!jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,2,201,7,head,&relay));
    assert(!jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,3,201,7,head,&relay));
    assert(relay_emits==1);
    assert(jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,4,202,7,head,&relay)==JC_CHIRP_CHANGED);
    shout(3,jc_chirp_u32(barn_data+0x714),1,0,3);
    assert(!jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,4,201,7,head,&relay));
    assert(relay_emits==2 && jc_chirp_u32(barn_data+0x70+0x48)==7);
    for (unsigned i=0;i<JC_CHIRP_SLOTS;++i) barn_data[i*0x70+0x68]=1;
    shout(3,jc_chirp_u32(barn_data+0x714),1,0,1);
    assert(jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,5,201,7,head,&relay)==JC_CHIRP_FULL);
    assert(!relay.stopped && relay.dropped==1 && relay_emits==2);
    fixture();memset(&relay,0,sizeof(relay));
    assert(!jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,1,201,7,head,&relay));
    shout(3,0,1,0,1);break_post=1;
    assert(jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,2,201,7,head,&relay)==JC_CHIRP_POSTCHECK);
    assert(relay.stopped && relay.emitted==1 && relay.delivery.sequence==1);
    assert(jc_chirp_relay_tick(read_memory,writable,relay_emit,NULL,image,game,3,201,7,head,&relay)==JC_CHIRP_ARGUMENT);
    break_post=0;
    puts("Actor chirp relay: head position, sound selection, duplicate suppression, feedback exclusion, full pool and irreversible enqueue checks passed.");
}
static int hooked_read(void *ctx,uint64_t at,void *out,size_t n) {
    if(at==image+0x13fbb0 && n==16)return jc_chirp_hook_entry_read(read_memory,ctx,image,image+0x100,out);
    return read_memory(ctx,at,out,n);
}
static void hooked_events(void) {
    fixture();memcpy(trampoline,code_data,10);memset(trampoline+10,0xcc,14);memset(code_data,0xe9,5);
    struct jc_chirp_event e={.participant=99,.epoch=1,.sequence=1,.sound=2,.radius=3,.duration=.5f};
    struct jc_chirp_delivery delivery={0};
    assert(jc_chirp_bridge_prepare_event(read_memory,writable,NULL,image,game,1,&e,&bridge)==JC_CHIRP_CODE);
    assert(!jc_chirp_deliver_event(hooked_read,writable,relay_emit,NULL,image,game,1,&e,&delivery,1));
    ++e.sequence;trampoline[0]^=1;
    assert(jc_chirp_bridge_prepare_event(hooked_read,writable,NULL,image,game,2,&e,&bridge)==JC_CHIRP_CODE);
    trampoline[0]^=1;code_data[15]^=1;
    assert(jc_chirp_bridge_prepare_event(hooked_read,writable,NULL,image,game,2,&e,&bridge)==JC_CHIRP_CODE);
}
static void direct_events(void) {
    fixture();relay_emits=break_post=0;
    struct jc_chirp_delivery delivery={0};
    struct jc_chirp_event e={.participant=900,.epoch=3,.sequence=1,.sound=2,
        .position={5,6,7},.radius=3,.duration=.5f};
    assert(!jc_chirp_deliver_event(read_memory,writable,relay_emit,NULL,image,game,1,&e,&delivery,1));
    assert(relay_emits==1 && barn_data[0x4d]==1 && !barn_data[0x4c]);
    assert(!memcmp(barn_data+0x20,e.position,12));
    assert(jc_chirp_deliver_event(read_memory,writable,relay_emit,NULL,image,game,2,&e,&delivery,1)==JC_CHIRP_ARGUMENT);
    ++e.sequence;++e.epoch;
    assert(jc_chirp_deliver_event(read_memory,writable,relay_emit,NULL,image,game,2,&e,&delivery,1)==JC_CHIRP_ARGUMENT);
    --e.epoch;++e.participant;
    assert(jc_chirp_deliver_event(read_memory,writable,relay_emit,NULL,image,game,2,&e,&delivery,1)==JC_CHIRP_ARGUMENT);
    --e.participant;
    assert(!jc_chirp_deliver_event(read_memory,writable,relay_emit,NULL,image,game,2,&e,&delivery,0));
    assert(relay_emits==2 && !barn_data[0x70+0x4d]);
    ++e.sequence;break_post=1;
    assert(jc_chirp_deliver_event(read_memory,writable,relay_emit,NULL,image,game,3,&e,&delivery,1)==JC_CHIRP_POSTCHECK);
    assert(delivery.sequence==3);break_post=0;
    puts("Shared companion events: source-independent delivery, native ownership, identity and duplicate guards passed.");
}
int main(void) {
    hooked_events();
    fixture();assert(!poll(1));reads=0;
    assert(!poll(2) && !batch.count && reads==6 && capture.frame==2);
    printf("Idle chirp capture: %u protected reads, down from 13; no sound table scan.\n",reads);
    reads=0;mutate=4;assert(poll(3)==JC_CHIRP_CHANGED);

    direct_events();
    relay_cases();
    ready(); assert(batch.source[0].event.sound==2 && batch.source[0].slot==3);
    struct jc_chirp_event e=extra(201); struct jc_chirp_delivery delivery={0};
    assert(jc_chirp_delivery_fresh(&delivery,&e)); assert(!prepare(&e,2));
    assert(bridge.slot==0 && bridge.sound_index==5 && jc_chirp_u32(bridge.input+0x28)==5);
    assert(!bridge.input[0x2c] && !bridge.input[0x2d]); native_stub(&bridge);
    assert(!jc_chirp_bridge_post(read_memory,NULL,2,&bridge));
    jc_chirp_delivery_commit(&delivery,&e); assert(!jc_chirp_delivery_fresh(&delivery,&e));
    e=extra(202); assert(!prepare(&e,2)); native_stub(&bridge); assert(!jc_chirp_bridge_post(read_memory,NULL,2,&bridge));
    assert(barn_data[3*0x70+0x4c]==1 && !barn_data[3*0x70+0x4d]);
    assert(!poll(3) && !batch.count); /* Both injected environment events are ignored. */
    ready(); e=extra(201); assert(!prepare(&e,2)); native_stub(&bridge);
    e=extra(202); assert(!prepare(&e,2)); native_stub(&bridge);
    shout(4,3,1,0,2); assert(!poll(3) && batch.count==1 && batch.source[0].id==3 && batch.source[0].event.sequence==2);
    assert(!poll(4) && !batch.count); /* Own counter increments never hide the next local event. */
    ready(); assert(!poll(2) && !batch.count); /* Same callback is never captured twice. */
    fixture(); shout(0,9,1,0,0); assert(!poll(1) && !batch.count); /* Prime skips older events. */
    shout(1,10,0,1,4); assert(!poll(2) && !batch.count); /* Vanilla remote never retransmits. */
    fixture(); u32(barn_data+0x714,UINT32_MAX); assert(!poll(1));
    shout(2,UINT32_MAX,1,0,0); assert(!poll(2) && batch.count==1); /* Native ID wraps. */
    fixture(); u32(barn_data+0x714,UINT32_MAX-1); assert(!poll(1));
    shout(3,UINT32_MAX-1,1,0,0); assert(!poll(2) && batch.count==1);
    e=extra(201); assert(!prepare(&e,2)); native_stub(&bridge); assert(!jc_chirp_bridge_post(read_memory,NULL,2,&bridge));
    e=extra(202); assert(!prepare(&e,2)); native_stub(&bridge); assert(!jc_chirp_bridge_post(read_memory,NULL,2,&bridge));
    shout(4,1,1,0,3); assert(!poll(3) && batch.count==1 && batch.source[0].id==1 && batch.source[0].event.sound==4);
    fixture(); assert(!poll(1)); shout(8,0,1,0,0); shout(1,1,1,0,1);
    assert(!poll(2) && batch.count==2 && batch.source[0].slot==8 && batch.source[1].slot==1);
    fixture(); assert(!poll(1)); shout(1,0,1,0,0); shout(2,0,1,0,0); assert(poll(2)==JC_CHIRP_CHANGED);
    fixture(); assert(!poll(1)); for (unsigned i=0;i<16;++i) shout(i,i,1,0,i%4);
    assert(!poll(2) && batch.count==16 && batch.source[15].event.sequence==16);
    for (unsigned i=0;i<16;++i) barn_data[i*0x70+0x68]=0;
    shout(0,16,1,0,1); assert(!poll(3) && batch.count==1 && batch.source[0].event.sequence==17);
    ready(); e=extra(201); assert(prepare(&e,3)==JC_CHIRP_ARGUMENT);
    ready(); e=extra(201); ++barn_data[3*0x70+0x50]; assert(prepare(&e,2)==JC_CHIRP_CHANGED);
    ready(); e=extra(201); f32(barn_data+3*0x70+0x20,11); assert(prepare(&e,2)==JC_CHIRP_CHANGED);
    ready(); e=extra(201); e.position[0]=NAN; assert(prepare(&e,2)==JC_CHIRP_ARGUMENT);
    ready(); e=extra(201); e.radius=200; assert(prepare(&e,2)==JC_CHIRP_ARGUMENT);
    ready(); e=extra(201); code_data[0]^=1; assert(prepare(&e,2)==JC_CHIRP_CODE);
    ready(); e=extra(201); deny=1; assert(prepare(&e,2)==JC_CHIRP_WRITE);
    ready(); e=extra(201); sound_data[0x70234+5*0x13c]='X'; assert(prepare(&e,2)==JC_CHIRP_SOUND);
    ready(); e=extra(201); memcpy(sound_data+0x70234+6*0x13c,sound_data+0x70234+5*0x13c,32);
    assert(prepare(&e,2)==JC_CHIRP_SOUND);
    ready(); e=extra(201); batch.count=17; assert(prepare(&e,2)==JC_CHIRP_ARGUMENT);
    ready(); e=extra(201); u32(sound_data+0x7c7b4,161); assert(prepare(&e,2)==JC_CHIRP_SOUND);
    for (unsigned sound=0;sound<4;++sound) {
        fixture(); assert(!poll(1)); shout(3,0,1,0,sound); assert(!poll(2) && batch.count==1);
        e=extra(201); assert(!prepare(&e,2) && bridge.sound_index==sound+4);
    }
    for(unsigned sound=5;sound<=22;++sound){
        fixture();u32(sound_data+0x7c7b4,9);
        strcpy((char *)sound_data+0x70234+8*0x13c,jc_chirp_sound_name(sound,0));
        assert(!poll(1));shout(3,0,1,0,8);
        assert(!poll(2) && batch.count==1 && batch.source[0].event.sound==sound);
        e=extra(201);
        assert(!prepare(&e,2) && bridge.sound_index==8);
        assert(!strcmp((char *)bridge.name,jc_chirp_sound_name(sound,0)));
        native_stub(&bridge);assert(!jc_chirp_bridge_post(read_memory,NULL,2,&bridge));
    }
    ready(); e=extra(201); for (unsigned i=0;i<16;++i) barn_data[i*0x70+0x68]=1;
    assert(prepare(&e,2)==JC_CHIRP_FULL);
    ready(); e=extra(201); assert(!prepare(&e,2)); native_stub(&bridge); ++barn_data[3*0x70+0x4d];
    assert(jc_chirp_bridge_post(read_memory,NULL,2,&bridge)==JC_CHIRP_POSTCHECK);
    fixture(); mutate=4; assert(poll(1)==JC_CHIRP_CHANGED);
    fixture(); fail=1; assert(poll(1)==JC_CHIRP_READ);
    ready(); e=extra(201); struct jc_chirp_delivery wrong={201,8,0}; assert(!jc_chirp_delivery_fresh(&wrong,&e));
    puts("Companion chirp: stable capture, priming, serial wrap/order, same-frame binding, two independent environmental pulses, no local/remote replay, exact enqueue postcheck, native fingerprint/sound/capacity guards; no Journey calls.");
    return 0;
}
