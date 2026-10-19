#ifndef JC_NATIVE_RECEIVER_H
#define JC_NATIVE_RECEIVER_H
#include <stdint.h>
#include <string.h>
#include <math.h>

/* Private numeric workspace for PC 118730, not a constructed PlayerBarn.
   The only indirect actor access (+68) points back inside this workspace.
   Never pass it to a constructor, destructor, scheduler or render routine. */
#define JC_RECEIVER_BYTES 0x28dd0u
#define JC_RECEIVER_STATE_BYTES 0x6cu
#define JC_RECEIVER_DUDE 0x146f0u
#define JC_RECEIVER_ACTOR_BYTES 0x1c10u
struct jc_receiver_state { unsigned char bytes[JC_RECEIVER_STATE_BYTES]; };
struct jc_receiver {
    _Alignas(16) unsigned char image[JC_RECEIVER_BYTES];
    uint64_t peer, generation, sequence;
    unsigned present, received, first, evaluated;
    struct jc_receiver_state evaluated_input;
    float evaluated_cooldown;
};
struct jc_receiver_frame {
    struct jc_receiver_state current;
    float dt, interaction_delay, chirp_cooldown;
    unsigned native_gate_a4;
};
struct jc_receiver_output {
    struct jc_receiver_state state;
    uint32_t sender_appearance[4]; /* Dude +68/+6c/+70/+74; not host identity. */
    float interaction_delay;
    unsigned snapped;
};
/* Wrapper invokes the pinned Microsoft-ABI function. No replacement math. */
typedef void (*jc_receiver_call)(void *,void *,void *);
enum { JC_RECEIVER_OK, JC_RECEIVER_ARGUMENT, JC_RECEIVER_SESSION,
       JC_RECEIVER_REPLAY, JC_RECEIVER_VALUE, JC_RECEIVER_EMPTY };
static inline float jc_receiver_float(const unsigned char *p) {
    float value; memcpy(&value,p,4); return value;
}
static inline void jc_receiver_store(unsigned char *p,float value) { memcpy(p,&value,4); }
static inline int jc_receiver_state_valid(const struct jc_receiver_state *s) {
    if (!s) return 0;
    /* Only fields used as floats by 118730; opaque flags/integers stay opaque. */
    for (unsigned i=0;i<0x40;i+=4) {
        float f=jc_receiver_float(s->bytes+i);
        if (!isfinite(f) || fabsf(f)>1000000.0f) return 0;
    }
    for (unsigned i=0x4c;i<=0x54;i+=4) {
        float f=jc_receiver_float(s->bytes+i);
        if (!isfinite(f) || fabsf(f)>1000000.0f) return 0;
    }
    uint32_t a[4]; memcpy(a,s->bytes+0x58,sizeof(a));
    return a[0]<=a[1] && a[1]<=4096 && a[2]<=6 && a[3]<=20;
}
/* Call once on zero-initialized storage, then monotonically increase generation.
   This resets receiver state only; visual dust and resident resources live elsewhere. */
static inline int jc_receiver_replace(struct jc_receiver *r,uint64_t peer,uint64_t generation) {
    if (!r || !peer || !generation) return JC_RECEIVER_ARGUMENT;
    if (generation<=r->generation) return JC_RECEIVER_SESSION;
    memset(r,0,sizeof(*r)); r->peer=peer; r->generation=generation;
    r->present=r->first=1;
    return JC_RECEIVER_OK;
}
static inline int jc_receiver_depart(struct jc_receiver *r,uint64_t peer,uint64_t generation) {
    if (!r || !r->present || r->peer!=peer || r->generation!=generation) return JC_RECEIVER_SESSION;
    r->present=r->evaluated=0; return JC_RECEIVER_OK;
}
static inline int jc_receiver_receive(struct jc_receiver *r,uint64_t peer,uint64_t generation,
        uint64_t sequence,const struct jc_receiver_state *state) {
    if (!r || !state || !sequence) return JC_RECEIVER_ARGUMENT;
    if (!r->present || r->peer!=peer || r->generation!=generation) return JC_RECEIVER_SESSION;
    if (sequence<=r->sequence) return JC_RECEIVER_REPLAY;
    if (!jc_receiver_state_valid(state)) return JC_RECEIVER_VALUE;
    memcpy(r->image+0x28d30,state->bytes,sizeof(state->bytes));
    jc_receiver_store(r->image+0x28dc4,0);
    r->sequence=sequence; r->received=1; r->evaluated=0;
    return JC_RECEIVER_OK;
}
/* Isolated state evaluation only. No live actor writes or resource pointers.
   Caller supplies the current simulated numeric state each frame. The eventual
   game adapter must also implement the native snap/controller reset contract. */
static inline int jc_receiver_step(struct jc_receiver *r,const struct jc_receiver_frame *f,
        jc_receiver_call call,void *context,struct jc_receiver_output *out) {
    if (!r || !f || !call || !out) return JC_RECEIVER_ARGUMENT;
    r->evaluated=0;
    if (!r->present) return JC_RECEIVER_SESSION;
    if (!r->received) return JC_RECEIVER_EMPTY;
    float age=jc_receiver_float(r->image+0x28dc4)+f->dt;
    if (!jc_receiver_state_valid(&f->current) || !isfinite(f->dt) || f->dt<=0 || f->dt>1 ||
        !isfinite(age) || age>3600 || !isfinite(f->interaction_delay) ||
        fabsf(f->interaction_delay)>3600 || !isfinite(f->chirp_cooldown) ||
        fabsf(f->chirp_cooldown)>3600 || f->native_gate_a4>1) return JC_RECEIVER_VALUE;
    unsigned char game[0x70]={0},time[0x10]={0};
    uintptr_t actor=(uintptr_t)(r->image+JC_RECEIVER_DUDE),timing=(uintptr_t)time;
    _Static_assert(sizeof(uintptr_t)==8,"64-bit native receiver required");
    memcpy(game+0x68,&timing,8); jc_receiver_store(time+0xc,f->dt);
    memcpy(r->image+0x68,&actor,8);
    memcpy(r->image+0x14700,f->current.bytes,sizeof(f->current.bytes));
    jc_receiver_store(r->image+0x15b58,f->chirp_cooldown);
    jc_receiver_store(r->image+0x28da0,f->interaction_delay);
    jc_receiver_store(r->image+0x28dc4,age);
    r->image[0xa1]=(unsigned char)r->first;
    r->image[0xa4]=(unsigned char)f->native_gate_a4;
    /* Native snap writes this byte; clear it to observe this invocation only. */
    r->image[JC_RECEIVER_DUDE+0x147a]=0;
    call(context,r->image,game);
    struct jc_receiver_output value={0};
    memcpy(value.state.bytes,r->image+0x14700,sizeof(value.state.bytes));
    memcpy(value.sender_appearance,r->image+0x28d88,sizeof(value.sender_appearance));
    value.interaction_delay=jc_receiver_float(r->image+0x28da0);
    value.snapped=r->image[JC_RECEIVER_DUDE+0x147a]!=0;
    /* 118730 preserves the recipient's symbol, including on snap. Expose the
       sender's symbol separately; admission must bind its glyph resources. */
    r->first=r->image[0xa1]!=0;
    r->evaluated_input=f->current; r->evaluated_cooldown=f->chirp_cooldown;
    r->evaluated=1;
    *out=value; return JC_RECEIVER_OK;
}
/* Project onto an owned actor snapshot, after step. A live caller must establish
   actor ownership and game-thread lifetime before publishing the changed spans.
   Holes in b1730's controller structure must retain the recipient's bytes. */
static inline int jc_receiver_project(struct jc_receiver *r,
        const struct jc_receiver_frame *frame,unsigned char *actor,size_t bytes) {
    if (!r || !frame || !actor || bytes<JC_RECEIVER_ACTOR_BYTES) return JC_RECEIVER_ARGUMENT;
    if (!r->present || !r->received) return JC_RECEIVER_SESSION;
    if (!r->evaluated) return JC_RECEIVER_EMPTY;
    if (memcmp(frame->current.bytes,r->evaluated_input.bytes,JC_RECEIVER_STATE_BYTES) ||
        memcmp(&frame->chirp_cooldown,&r->evaluated_cooldown,4)) return JC_RECEIVER_VALUE;
    if (memcmp(actor+0x10,frame->current.bytes,JC_RECEIVER_STATE_BYTES) ||
        memcmp(actor+0x1468,&frame->chirp_cooldown,4)) return JC_RECEIVER_VALUE;
    const unsigned char *evaluated=r->image+JC_RECEIVER_DUDE;
    memcpy(actor+0x10,evaluated+0x10,JC_RECEIVER_STATE_BYTES);
    if (evaluated[0x147a]) {
        static const uint16_t reset[][2]={
            {0x000,8},{0x010,0x68},{0x080,0xf0},{0x170,1},{0x174,4},
            {0x180,0xbc},{0x240,8},{0x250,2},{0x254,4},{0x260,0x58},{0x3b0,4}
        };
        memcpy(actor+0x90,evaluated+0x90,16);
        actor[0x147a]=evaluated[0x147a];
        memcpy(actor+0x1bf0,evaluated+0x1bf0,32);
        for (unsigned i=0;i<sizeof(reset)/sizeof(reset[0]);++i)
            memcpy(actor+0x17f0+reset[i][0],evaluated+0x17f0+reset[i][0],reset[i][1]);
    }
    r->evaluated=0;
    return JC_RECEIVER_OK;
}
#endif
