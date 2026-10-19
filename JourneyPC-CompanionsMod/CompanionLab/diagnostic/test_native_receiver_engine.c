#include "native_receiver.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "receiver_code.h"

typedef void (__attribute__((ms_abi)) *native_apply)(void *,void *);
static native_apply apply;
static unsigned invocations,snaps,blends;
static unsigned char before[JC_RECEIVER_BYTES];
static int writable(unsigned at) {
    if (at==0xa1 || (at>=0x28da0 && at<0x28da4)) return 1;
    if (at<JC_RECEIVER_DUDE) return 0;
    at-=JC_RECEIVER_DUDE;
    return (at>=0x10 && at<0x7c) || (at>=0x90 && at<0xa0) || at==0x147a ||
           (at>=0x17f0 && at<0x1ba4) || (at>=0x1bf0 && at<0x1c10);
}
static void engine(void *context,void *barn,void *game) {
    (void)context;
    memcpy(before,barn,sizeof(before));
    unsigned char game_before[0x70],time_before[0x10]; uintptr_t time;
    memcpy(game_before,game,sizeof(game_before)); memcpy(&time,game_before+0x68,8);
    memcpy(time_before,(void *)time,sizeof(time_before));
    apply(barn,game);
    assert(!memcmp(game_before,game,sizeof(game_before)));
    assert(!memcmp(time_before,(void *)time,sizeof(time_before)));
    for (unsigned i=0;i<sizeof(before);++i)
        if (before[i]!=((unsigned char *)barn)[i]) assert(writable(i));
    ++invocations;
}
static struct jc_receiver_state sample(float x,unsigned outfit,unsigned symbol) {
    struct jc_receiver_state s={0};
    jc_receiver_store(s.bytes,x); jc_receiver_store(s.bytes+4,100);
    jc_receiver_store(s.bytes+8,200); jc_receiver_store(s.bytes+0x20,0.25f);
    uint32_t appearance[4]={3,9,outfit,symbol};
    memcpy(s.bytes+0x58,appearance,16);
    return s;
}
static void reference_frame(unsigned char *barn,const struct jc_receiver_frame *f) {
    unsigned char game[0x70]={0},time[0x10]={0};
    uintptr_t actor=(uintptr_t)(barn+0x146f0),clock=(uintptr_t)time;
    memcpy(barn+0x68,&actor,8); memcpy(game+0x68,&clock,8);
    memcpy(barn+0x14700,f->current.bytes,0x6c);
    jc_receiver_store(time+0xc,f->dt);
    jc_receiver_store(barn+0x28dc4,jc_receiver_float(barn+0x28dc4)+f->dt);
    jc_receiver_store(barn+0x28da0,f->interaction_delay);
    jc_receiver_store(barn+0x15b58,f->chirp_cooldown);
    barn[0xa4]=(unsigned char)f->native_gate_a4;
    barn[0x146f0+0x147a]=0;
    engine(NULL,barn,game);
}
static void compare(const struct jc_receiver *r,const unsigned char *reference) {
    /* Self pointers differ. Every other byte, including controller reset, must agree. */
    assert(!memcmp(r->image,reference,0x68));
    assert(!memcmp(r->image+0x70,reference+0x70,JC_RECEIVER_BYTES-0x70));
}
static void projection_cases(void) {
    struct jc_receiver *r=calloc(1,sizeof(*r));
    unsigned char *reference=malloc(JC_RECEIVER_BYTES);
    unsigned char *actor=malloc(JC_RECEIVER_ACTOR_BYTES);
    unsigned char *saved=malloc(JC_RECEIVER_ACTOR_BYTES);
    assert(r && reference && actor && saved);
    for (unsigned n=0;n<64;++n) {
        memset(reference,0xa5,JC_RECEIVER_BYTES);
        struct jc_receiver_frame f={0};
        f.current=sample(0,0,7); f.dt=1.0f/60;
        f.native_gate_a4=(n>>1)&1;
        f.interaction_delay=(n>>2)&1 ? 0.3f : 0;
        f.chirp_cooldown=(n>>3)&1 ? 0.5f : 0;
        struct jc_receiver_state incoming=sample(n&16 ? 10000 : 1,6,20);
        assert(jc_receiver_replace(r,1,n+1)==0);
        r->first=n&1;
        assert(jc_receiver_receive(r,1,n+1,1,&incoming)==0);
        unsigned char *body=reference+JC_RECEIVER_DUDE;
        memcpy(body+0x10,f.current.bytes,JC_RECEIVER_STATE_BYTES);
        jc_receiver_store(body+0x1468,f.chirp_cooldown);
        memcpy(actor,body,JC_RECEIVER_ACTOR_BYTES);
        unsigned char game[0x70]={0},time[0x10]={0};
        uintptr_t clock=(uintptr_t)time,recipient=(uintptr_t)body;
        memcpy(game+0x68,&clock,8); memcpy(reference+0x68,&recipient,8);
        jc_receiver_store(time+0xc,f.dt);
        memcpy(reference+0x28d30,incoming.bytes,JC_RECEIVER_STATE_BYTES);
        jc_receiver_store(reference+0x28dc4,f.dt);
        jc_receiver_store(reference+0x28da0,f.interaction_delay);
        reference[0xa1]=(unsigned char)(n&1);
        reference[0xa4]=(unsigned char)f.native_gate_a4;
        engine(NULL,reference,game);
        struct jc_receiver_output out;
        assert(jc_receiver_step(r,&f,engine,NULL,&out)==0);
        actor[0x10]^=1; memcpy(saved,actor,JC_RECEIVER_ACTOR_BYTES);
        assert(jc_receiver_project(r,&f,actor,JC_RECEIVER_ACTOR_BYTES)==JC_RECEIVER_VALUE);
        assert(!memcmp(saved,actor,JC_RECEIVER_ACTOR_BYTES)); actor[0x10]^=1;
        assert(jc_receiver_project(r,&f,actor,JC_RECEIVER_ACTOR_BYTES)==0);
        assert(!memcmp(actor,body,JC_RECEIVER_ACTOR_BYTES));
        assert(jc_receiver_project(r,&f,actor,JC_RECEIVER_ACTOR_BYTES)==JC_RECEIVER_EMPTY);
    }
    free(saved); free(actor); free(reference); free(r);
}
static unsigned dispatch_updates,dispatch_idle;
static unsigned char *dispatch_barn,*dispatch_game;
static void __attribute__((ms_abi)) record_update(void *dude,void *game,void *vector,
                                                uintptr_t unused,unsigned char present) {
    (void)unused;
    unsigned remote=dispatch_updates++;
    assert(remote<2 && game==dispatch_game);
    assert(dude==dispatch_barn+(remote ? 0x146f0 : 0xb0));
    assert(vector==dispatch_barn+(remote ? 0x90 : 0x40));
    unsigned expected=remote ? (dispatch_barn[0xa2]!=0 && dispatch_barn[0xa4]==0) : dispatch_barn[0xa2];
    assert(present==expected);
}
static void __attribute__((ms_abi)) record_idle(void *dude,void *clock) {
    assert(dispatch_updates==1 && !dispatch_idle++);
    assert(dude==dispatch_barn+0x146f0 && clock==dispatch_game+0x158);
}
static void redirect(unsigned char *at,void *target) {
    const unsigned char jump[]={0xff,0x25,0,0,0,0};
    memcpy(at,jump,sizeof(jump)); memcpy(at+sizeof(jump),&target,sizeof(target));
}
static void dispatcher_cases(unsigned char *code) {
    dispatch_barn=calloc(1,JC_RECEIVER_BYTES); dispatch_game=calloc(1,0x160);
    assert(dispatch_barn && dispatch_game);
    uintptr_t clock=(uintptr_t)(dispatch_game+0x158);
    memcpy(dispatch_game+0x150,&clock,sizeof(clock));
    native_apply dispatch=(native_apply)(void *)(code+0x118690);
    const unsigned char flags[]={0,1,2,255};
    for (unsigned a=0;a<sizeof(flags);++a) for (unsigned b=0;b<sizeof(flags);++b) {
        dispatch_barn[0xa2]=flags[a]; dispatch_barn[0xa4]=flags[b];
        dispatch_updates=dispatch_idle=0;
        dispatch(dispatch_barn,dispatch_game);
        assert(dispatch_updates==(a || b ? 2u : 1u));
        assert(dispatch_idle==(a || b ? 0u : 1u));
    }
    free(dispatch_game); free(dispatch_barn);
    puts("Native actor dispatcher: 16 gate combinations; exact actor/vector/clock and fifth-byte arguments verified; actor update intercepted, no gameplay");
}
int main(void) {
    size_t size=0x2890000;
    unsigned char *code=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(code!=MAP_FAILED);
    load_pieces(code);
    redirect(code+0xa6350,(void *)record_update);
    redirect(code+0xa40e0,(void *)record_idle);
    /* Same globals are shared by both native invocations, exactly as in-game. */
    jc_receiver_store(code+0x28427f8,0);
    jc_receiver_store(code+0x2887514,0.05f);
    assert(mprotect(code,size,PROT_READ)==0);
    assert(mprotect(code+0xb1000,0x1000,PROT_READ|PROT_EXEC)==0);
    assert(mprotect(code+0x117000,0x2000,PROT_READ|PROT_EXEC)==0);
    assert(mprotect(code+0xa4000,0x1000,PROT_READ|PROT_EXEC)==0);
    assert(mprotect(code+0xa6000,0x1000,PROT_READ|PROT_EXEC)==0);
    dispatcher_cases(code);
    apply=(native_apply)(void *)(code+0x118730);
    struct jc_receiver *r[2]; unsigned char *ref[2];
    struct jc_receiver_frame frames[2]={0};
    uint64_t sequence[2]={0,0};
    for (unsigned p=0;p<2;++p) {
        r[p]=calloc(1,sizeof(*r[p])); ref[p]=calloc(1,JC_RECEIVER_BYTES);
        assert(r[p] && ref[p]);
        assert(jc_receiver_replace(r[p],p+1,p+1)==0); ref[p][0xa1]=1;
        frames[p].current=sample(0,0,7+p); frames[p].dt=1.0f/60;
    }
    for (unsigned frame=0;frame<720;++frame) {
        for (unsigned p=0;p<2;++p) {
            struct jc_receiver_frame *f=&frames[p];
            /* Cover varying frame times, heading wrap, receiver cooldown,
               interaction delay, native a4 gate, and distance snap. */
            f->dt=frame%3==0 ? 1.0f/30 : 1.0f/60;
            f->interaction_delay=frame%31==0 ? 0.25f : 0;
            f->chirp_cooldown=frame%17==0 ? 0.2f : 0;
            f->native_gate_a4=(frame>=600 && frame<630);
            if (frame%6==0) {
                float x=(float)frame*0.04f*(p ? -1 : 1);
                if (frame>=360 && frame<400) x+=10000;
                struct jc_receiver_state s=sample(x,p ? 6 : 1,p ? 20 : 2);
                jc_receiver_store(s.bytes+0x20,frame%12 ? -3.13f : 3.13f);
                for (unsigned i=0x40;i<0x49;++i) s.bytes[i]=(frame+i)%2;
                assert(jc_receiver_receive(r[p],p+1,p+1,++sequence[p],&s)==0);
                memcpy(ref[p]+0x28d30,s.bytes,sizeof(s.bytes));
                jc_receiver_store(ref[p]+0x28dc4,0);
            }
            struct jc_receiver_output out;
            assert(jc_receiver_step(r[p],f,engine,NULL,&out)==0);
            reference_frame(ref[p],f); compare(r[p],ref[p]);
            assert(out.sender_appearance[2]==(p ? 6u : 1u));
            assert(out.sender_appearance[3]==(p ? 20u : 2u));
            uint32_t symbol; memcpy(&symbol,out.state.bytes+0x64,4);
            assert(symbol==7+p); /* Native receiver deliberately preserves this. */
            assert(jc_receiver_state_valid(&out.state));
            if (out.snapped) {
                ++snaps; assert(jc_receiver_float(out.state.bytes+0x10)==0);
                assert(out.interaction_delay==0);
            } else ++blends;
            if (frame==0) assert(out.snapped);
            if (frame==100) {
                float x=jc_receiver_float(out.state.bytes);
                assert(p ? (x<0 && x>-4) : (x>0 && x<4));
            }
            f->current=out.state;
        }
    }
    /* A third peer replaces peer 1 in the same receiver slot; peer 2 is untouched. */
    unsigned char *neighbor=malloc(sizeof(*r[1])); assert(neighbor);
    memcpy(neighbor,r[1],sizeof(*r[1]));
    assert(jc_receiver_depart(r[0],1,1)==0);
    assert(jc_receiver_replace(r[0],3,3)==0);
    struct jc_receiver_state replacement=sample(42,4,11);
    assert(jc_receiver_receive(r[0],1,1,999,&replacement)==JC_RECEIVER_SESSION);
    assert(jc_receiver_receive(r[0],3,3,1,&replacement)==0);
    struct jc_receiver_output out; frames[0].native_gate_a4=0;
    assert(jc_receiver_step(r[0],&frames[0],engine,NULL,&out)==0);
    assert(out.snapped && jc_receiver_float(out.state.bytes)==42);
    assert(out.sender_appearance[2]==4 && out.sender_appearance[3]==11);
    assert(!memcmp(neighbor,r[1],sizeof(*r[1])));
    assert(snaps>=6 && blends>1000);
    projection_cases();
    printf("Original PC receiver: %u native calls; 1440 differential frames; %u snaps, %u blend/gated frames; 64 full actor/controller projection cases; independent peers and replacement passed.\n",invocations,snaps,blends);
    free(neighbor);
    for (unsigned p=0;p<2;++p) { free(r[p]); free(ref[p]); }
    assert(munmap(code,size)==0);
    return 0;
}
