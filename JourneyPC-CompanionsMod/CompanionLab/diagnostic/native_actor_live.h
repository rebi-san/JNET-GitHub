#ifndef JC_NATIVE_ACTOR_LIVE_H
#define JC_NATIVE_ACTOR_LIVE_H
#include "native_layout.h"
#include "native_receiver.h"
#include <xmmintrin.h>

#if defined(__x86_64__) && !defined(_WIN32)
#define JC_ACTOR_ABI __attribute__((ms_abi))
#else
#define JC_ACTOR_ABI
#endif

/* 117790 is a PlayerBarn dispatcher. These are its per-Dude operations, with
   the matrix retained by the participant instead of context+180/+1c0. Caller
   owns the actor and render queues throughout this native submission phase. */
struct jc_actor_submission {
    uint64_t actor, body_queue, symbol_queue;
    _Alignas(16) unsigned char matrix[64];
    unsigned meshes;
};
struct jc_actor_input {
    _Alignas(16) float axes[4], direction[4];
    float packet_age,scalars[2],body_scale,glow_targets[3],chirp_shape[2];
    uint64_t peer,generation,sequence;
    uint32_t chapter;
    unsigned char inactive, interaction_gate;
};
struct jc_actor_sender {
    struct jc_receiver_state state;
    float axes[4],direction[4],scalars[2],body_scale,glow_targets[3],chirp_shape[2];
    uint32_t chapter;
};
static inline int jc_actor_chirp_shape_valid(const float shape[2]) {
    return isfinite(shape[0]) && shape[0]>=0 && shape[0]<=1000000 &&
        isfinite(shape[1]) && shape[1]>=0.000001f && shape[1]<=1000;
}
static inline int jc_actor_sender_valid(const struct jc_actor_sender *sender) {
    if (!sender || !jc_receiver_state_valid(&sender->state) || sender->chapter>31 ||
        !jc_actor_chirp_shape_valid(sender->chirp_shape) || !isfinite(sender->body_scale) || sender->body_scale<=0 || sender->body_scale>1000) return 0;
    for (unsigned n=0;n<4;++n) if (!isfinite(sender->axes[n]) || !isfinite(sender->direction[n]) ||
        fabsf(sender->axes[n])>1000000 || fabsf(sender->direction[n])>1000000) return 0;
    for (unsigned n=0;n<2;++n) if (!isfinite(sender->scalars[n]) || fabsf(sender->scalars[n])>1000000) return 0;
    for (unsigned n=0;n<3;++n) if (!isfinite(sender->glow_targets[n]) ||
        sender->glow_targets[n]<0 || sender->glow_targets[n]>16) return 0;
    return 1;
}
static inline int jc_actor_sender_spawn(const float position[4],uint32_t chapter,float body_scale,
        const float chirp_shape[2],const uint32_t appearance[4],struct jc_actor_sender *out) {
    if(!position || !chirp_shape || !appearance || !out)return 0;
    struct jc_actor_sender initial={0};
    memcpy(initial.state.bytes,position,16);
    memcpy(initial.state.bytes+0x58,appearance,16);
    initial.chapter=chapter;initial.body_scale=body_scale;initial.direction[2]=1;
    memcpy(initial.chirp_shape,chirp_shape,sizeof(initial.chirp_shape));
    if(!jc_actor_sender_valid(&initial))return 0;
    *out=initial;return 1;
}
static inline unsigned jc_actor_sender_invalid_field(const struct jc_actor_sender *s) {
    if(!s)return 1;
    for(unsigned i=0;i<0x40;i+=4){float v=jc_receiver_float(s->state.bytes+i);if(!isfinite(v) || fabsf(v)>1000000)return 100+i;}
    for(unsigned i=0x4c;i<=0x54;i+=4){float v=jc_receiver_float(s->state.bytes+i);if(!isfinite(v) || fabsf(v)>1000000)return 100+i;}
    uint32_t a[4];memcpy(a,s->state.bytes+0x58,sizeof(a));
    if(a[0]>a[1])return 2;
    if(a[1]>4096)return 3;
    if(a[2]>6)return 4;
    if(a[3]>20)return 5;
    if(s->chapter>31)return 6;
    if(!jc_actor_chirp_shape_valid(s->chirp_shape))return 7;
    if(!isfinite(s->body_scale) || s->body_scale<=0 || s->body_scale>1000)return 8;
    for(unsigned n=0;n<4;++n)if(!isfinite(s->axes[n]) || !isfinite(s->direction[n]) || fabsf(s->axes[n])>1000000 || fabsf(s->direction[n])>1000000)return 9;
    for(unsigned n=0;n<2;++n)if(!isfinite(s->scalars[n]) || fabsf(s->scalars[n])>1000000)return 10;
    for(unsigned n=0;n<3;++n)if(!isfinite(s->glow_targets[n]) || s->glow_targets[n]<0 || s->glow_targets[n]>16)return 11+n;
    return 0;
}
static struct jc_actor_sender jc_actor_capture_rejected;
static unsigned jc_actor_capture_rejected_field;
/* Call on the participant owner thread (or under its publication lock). Input
   and actor state are admitted together, only after all packet fields validate. */
static inline int jc_actor_receive(struct jc_receiver *receiver,struct jc_actor_input *input,
        const struct jc_actor_sender *sender,uint64_t peer,uint64_t generation,uint64_t sequence,
        uint32_t chapter_match) {
    if (!receiver || !input || !sender) return JC_RECEIVER_ARGUMENT;
    if (!jc_actor_sender_valid(sender)) return JC_RECEIVER_VALUE;
    if (chapter_match>31 || sender->chapter!=chapter_match) return JC_RECEIVER_SESSION;
    int code=jc_receiver_receive(receiver,peer,generation,sequence,&sender->state);
    if (code) return code;
    memcpy(input->axes,sender->axes,sizeof(input->axes));
    memcpy(input->direction,sender->direction,sizeof(input->direction));
    memcpy(input->scalars,sender->scalars,sizeof(input->scalars));
    input->body_scale=sender->body_scale;
    memcpy(input->chirp_shape,sender->chirp_shape,sizeof(input->chirp_shape));
    memcpy(input->glow_targets,sender->glow_targets,sizeof(input->glow_targets));
    input->peer=peer;input->generation=generation;input->sequence=sequence;input->chapter=sender->chapter;
    input->packet_age=0;return 0;
}
/* 117600 serializes these numeric fields from Local Barn+10 into Remote+60;
   de18f/a1660 separately serializes the actor state. Never transmit pointers. */
static inline int jc_actor_capture_sender(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t game,struct jc_actor_sender *out) {
    if (!read || !out) return JC_LAYOUT_RANGE;
    uint64_t barn,type,actor_type,again;
    if (!jc_layout_field(read,ctx,game,0x1e0,&barn,8) ||
        !jc_layout_field(read,ctx,barn,0,&type,8) ||
        !jc_layout_field(read,ctx,barn,0xb0,&actor_type,8)) return JC_LAYOUT_READ;
    if (type!=image+0x5da340 || actor_type!=image+0x5d0380) return JC_LAYOUT_TYPE;
    struct jc_actor_sender sample[2];memset(sample,0,sizeof(sample));
    for (unsigned i=0;i<2;++i) {
        if (!jc_layout_field(read,ctx,barn,0xc0,sample[i].state.bytes,JC_RECEIVER_STATE_BYTES) ||
            !jc_layout_field(read,ctx,barn,0x30,sample[i].axes,16) ||
            !jc_layout_field(read,ctx,barn,0x40,sample[i].direction,16) ||
            !jc_layout_field(read,ctx,barn,0x20,sample[i].scalars,8) ||
            !jc_layout_field(read,ctx,barn,0x14e0,&sample[i].body_scale,4) ||
            !jc_layout_field(read,ctx,barn,0x1520,sample[i].chirp_shape,4) ||
            !jc_layout_field(read,ctx,barn,0x1f14,sample[i].chirp_shape+1,4) ||
            !jc_layout_field(read,ctx,barn,0x14,&sample[i].chapter,4)) return JC_LAYOUT_READ;
        for (unsigned n=0;n<3;++n)
            if (!jc_layout_field(read,ctx,barn,0x14f8+n*8,sample[i].glow_targets+n,4)) return JC_LAYOUT_READ;
        jc_receiver_store(sample[i].state.bytes+0x0c,0);
        jc_receiver_store(sample[i].state.bytes+0x1c,0);
    }
    if (memcmp(&sample[0],&sample[1],sizeof(sample[0])) ||
        !jc_layout_field(read,ctx,game,0x1e0,&again,8) || again!=barn) return JC_LAYOUT_CHANGED;
    if (!jc_actor_sender_valid(&sample[0])) {
        jc_actor_capture_rejected=sample[0];jc_actor_capture_rejected_field=jc_actor_sender_invalid_field(&sample[0]);
        return JC_LAYOUT_RANGE;
    }
    *out=sample[0];return 0;
}
static inline __m128 jc_actor_constant(uint64_t image,unsigned rva) {
    return _mm_loadu_ps((const float *)(uintptr_t)(image+rva));
}
static inline float jc_actor_scalar(uint64_t image,unsigned rva) {
    float value; memcpy(&value,(const void *)(uintptr_t)(image+rva),4); return value;
}
/* Literal numeric path 118401..1185f8. This operates only on the caller's owned
   numeric snapshot. It neither constructs a fake Barn nor passes that snapshot
   to a native actor/resource function. Normalize remains original 7a1b0. */
static inline int jc_actor_correct_input(uint64_t image,unsigned char *actor,size_t bytes,
        const float incoming_position[4],struct jc_actor_input *input,float *delay,float dt) {
    if (!image || !actor || bytes<JC_RECEIVER_ACTOR_BYTES || !incoming_position || !input ||
        !delay || !isfinite(*delay) || *delay<0 || *delay>3600 || !isfinite(dt) || dt<=0 || dt>1 || input->inactive>1)
        return JC_RECEIVER_ARGUMENT;
    float current[4];memcpy(current,actor+0x10,16);
    for (unsigned n=0;n<4;++n) if (!isfinite(current[n]) || !isfinite(incoming_position[n]) ||
        !isfinite(input->axes[n]) || !isfinite(input->direction[n]) || fabsf(current[n])>1000000 ||
        fabsf(incoming_position[n])>1000000 || fabsf(input->axes[n])>1000000 ||
        fabsf(input->direction[n])>1000000) return JC_RECEIVER_VALUE;
    if (input->inactive) { *delay=jc_actor_scalar(image,0x68b490);return 0; }
    if (*delay>0) {
        __m128 zero=_mm_setzero_ps(),direction=_mm_loadu_ps(input->direction);
        __m128 up=jc_actor_constant(image,0x5c1e70),half=jc_actor_constant(image,0x5c1f20);
        __m128 three=jc_actor_constant(image,0x5c1f30),one=jc_actor_constant(image,0x5c1ea0);
        __m128 cross=_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(up,up,0xc9),direction),
            _mm_mul_ps(_mm_shuffle_ps(direction,direction,0xc9),up));
        cross=_mm_shuffle_ps(cross,cross,0x49);
        __m128 axes=_mm_loadu_ps(input->axes);
        __m128 requested=_mm_add_ps(_mm_mul_ps(direction,_mm_shuffle_ps(axes,axes,0x55)),
            _mm_mul_ps(cross,_mm_shuffle_ps(axes,axes,0)));
        __m128 difference=_mm_sub_ps(_mm_loadu_ps(incoming_position),_mm_loadu_ps(current));
        __m128 square=_mm_mul_ps(difference,difference);
        __m128 sum=_mm_add_ps(_mm_shuffle_ps(square,square,0xaa),
            _mm_add_ps(_mm_shuffle_ps(square,square,0x55),square));
        __m128 length2=_mm_shuffle_ps(sum,sum,0);
        __m128 mask=_mm_cmplt_ps(jc_actor_constant(image,0x68c000),length2);
        mask=_mm_shuffle_ps(mask,mask,0);
        length2=_mm_or_ps(_mm_and_ps(length2,mask),_mm_andnot_ps(mask,one));
        __m128 inverse=_mm_rsqrt_ps(length2);
        __m128 special=_mm_or_ps(_mm_cmpeq_ps(zero,length2),_mm_cmpeq_ps(zero,inverse));
        __m128 refined=_mm_mul_ps(_mm_mul_ps(half,inverse),
            _mm_sub_ps(three,_mm_mul_ps(_mm_mul_ps(length2,inverse),inverse)));
        __m128 length=_mm_mul_ps(_mm_mul_ps(half,refined),
            _mm_sub_ps(three,_mm_mul_ps(_mm_mul_ps(length2,refined),refined)));
        length=_mm_mul_ps(length,length2);
        length=_mm_and_ps(mask,_mm_or_ps(_mm_andnot_ps(special,length),_mm_and_ps(length2,special)));
        __m128 factor=_mm_mul_ss(_mm_sub_ss(length,_mm_set_ss(jc_actor_scalar(image,0x68b080))),
            _mm_set_ss(jc_actor_scalar(image,0x68b1e4)));
        factor=_mm_min_ss(_mm_max_ss(factor,zero),_mm_set_ss(jc_actor_scalar(image,0x68b24c)));
        _Alignas(16) float delta[4],normal[4];_mm_store_ps(delta,difference);
        ((void *(JC_ACTOR_ABI *)(float *,const float *,float))(uintptr_t)(image+0x7a1b0))
            (normal,delta,jc_actor_scalar(image,0x68ae88));
        __m128 fraction=_mm_mul_ss(_mm_set_ss(*delay),_mm_set_ss(jc_actor_scalar(image,0x68b0d4)));
        fraction=_mm_min_ss(_mm_max_ss(fraction,zero),_mm_set_ss(jc_actor_scalar(image,0x68b24c)));
        __m128 correction=_mm_mul_ps(_mm_load_ps(normal),_mm_shuffle_ps(factor,factor,0));
        correction=_mm_add_ps(requested,_mm_mul_ps(_mm_sub_ps(correction,requested),
            _mm_shuffle_ps(fraction,fraction,0)));
        _mm_storeu_ps((float *)(void *)(actor+0x1bc0),correction);
        actor[0x1bd4]=1;memset(actor+0xcc,0,4);
        if (_mm_cvtss_f32(length)>jc_actor_scalar(image,0x68b4e4)) {
            memcpy(actor+0x10,incoming_position,16);*delay=0;
        }
    }
    *delay=_mm_cvtss_f32(_mm_max_ss(_mm_sub_ss(_mm_set_ss(*delay),_mm_set_ss(dt)),_mm_setzero_ps()));
    return 0;
}
/* 118600..118655 feeds packet-age attenuated axes into the original input
   routine. The preceding 11841a..1185dc catch-up correction is separate: caller
   must perform it before this path if the receiver's interaction delay is live. */
static inline int jc_actor_native_input(uint64_t image,uint64_t actor,uint64_t game,
        uint64_t local_actor,struct jc_actor_input *input) {
    if (!input || !image || !actor || !game || !local_actor || input->inactive>1 ||
        input->interaction_gate>1 || !isfinite(input->packet_age) ||
        input->packet_age<0 || input->packet_age>3600) return JC_RECEIVER_ARGUMENT;
    for (unsigned i=0;i<4;++i)
        if (!isfinite(input->axes[i]) || !isfinite(input->direction[i]) ||
            fabsf(input->axes[i])>1000000 || fabsf(input->direction[i])>1000000)
            return JC_RECEIVER_VALUE;
    _Alignas(16) float attenuated[4]={0};
    if (!input->inactive) {
        float scale=((float (JC_ACTOR_ABI *)(float))(uintptr_t)(image+0x117fd0))(input->packet_age);
        if (!isfinite(scale)) return JC_RECEIVER_VALUE;
        for (unsigned i=0;i<4;++i) attenuated[i]=input->axes[i]*scale;
    }
    ((void (JC_ACTOR_ABI *)(uint64_t,uint64_t,const float *,float *,uint64_t,unsigned char))
        (uintptr_t)(image+0xa54a0))(actor,game,attenuated,input->direction,local_actor,input->interaction_gate);
    return 0;
}
static inline int jc_actor_submit(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t actor,uint64_t render,struct jc_actor_submission *out) {
    if (!read || !out) return JC_LAYOUT_RANGE;
    uint64_t type,body,symbol,chain[2][3]={{0}},uniform[2]={0};
    unsigned char enabled[2],matrix[64];
    if (!jc_layout_field(read,ctx,actor,0,&type,8) ||
        !jc_layout_field(read,ctx,render,0x18,&body,8) ||
        !jc_layout_field(read,ctx,render,0x60,&symbol,8) ||
        !jc_layout_field(read,ctx,actor,0xd40,&enabled[0],1) ||
        !jc_layout_field(read,ctx,actor,0x13e0,&enabled[1],1) ||
        !jc_layout_field(read,ctx,actor,0x690,matrix,64)) return JC_LAYOUT_READ;
    if (type!=image+0x5d0380 || !body || !symbol) return JC_LAYOUT_TYPE;
    static const unsigned material[3]={0x830,0xed0,0x1e0};
    static const unsigned meshes[3]={0x160,0x7b0,0xe50};
    for (unsigned n=0;n<2;++n) {
        if (!jc_layout_field(read,ctx,actor,material[n]+0x30,&chain[n][0],8)) return JC_LAYOUT_READ;
        if (chain[n][0]) {
            if (!jc_layout_field(read,ctx,chain[n][0],0x10,&chain[n][1],8)) return JC_LAYOUT_READ;
            if (chain[n][1] && !jc_layout_field(read,ctx,chain[n][1],0x18,&chain[n][2],8)) return JC_LAYOUT_READ;
        }
    }
    /* Native lookup can legitimately return zero. The two lookups/writes occur
       before all three commits, exactly as in both branches of 117790. */
    for (unsigned n=0;n<2;++n) if (chain[n][1]) {
        uniform[n]=((uint64_t (JC_ACTOR_ABI *)(uint64_t,uint64_t))(uintptr_t)(image+0x273a00))
            (chain[n][2],image+0x5dfaf8);
        if (uniform[n]) { uint32_t one=0x3f800000; memcpy((void *)(uintptr_t)uniform[n],&one,4); }
    }
    for (unsigned n=0;n<3;++n)
        ((void (JC_ACTOR_ABI *)(uint64_t))(uintptr_t)(image+0x274d70))(actor+material[n]);
    ((void (JC_ACTOR_ABI *)(uint64_t,uint64_t))(uintptr_t)(image+0x263aa0))(actor+meshes[0],body);
    unsigned count=1;
    for (unsigned n=0;n<2;++n) if (enabled[n]) {
        ((void (JC_ACTOR_ABI *)(uint64_t,uint64_t))(uintptr_t)(image+0x263aa0))(actor+meshes[n+1],symbol);
        ++count;
    }
    if (!jc_layout_field(read,ctx,actor,0x690,matrix,64)) return JC_LAYOUT_READ;
    out->actor=actor; out->body_queue=body; out->symbol_queue=symbol; out->meshes=count;
    memcpy(out->matrix,matrix,64);
    return 0;
}

/* The fourth register argument is not read by a6350; the fifth byte controls
   remote simulation at a6ab3 and must occupy the native fifth argument slot.
   Calling this requires the full retained-Dude lifecycle and scoped private
   appearance routing for its internal a9470 call, plus an accepting cloth frame. */
static inline void jc_actor_native_update(uint64_t image,uint64_t actor,uint64_t game,
        const float interaction_vector[4],unsigned char connected) {
    ((void (JC_ACTOR_ABI *)(uint64_t,uint64_t,const float *,uint64_t,unsigned char))
        (uintptr_t)(image+0xa6350))(actor,game,interaction_vector,0,connected);
}
static inline void jc_actor_native_finish(uint64_t image,uint64_t actor) {
    ((void (JC_ACTOR_ABI *)(uint64_t))(uintptr_t)(image+0xa9d20))(actor);
}

struct jc_actor_identity {
    uint64_t image,game,barn,actor,glyph,peer,generation;
};
typedef int (*jc_actor_writer)(void *,uint64_t,const void *,size_t);
typedef int (*jc_actor_retired)(void *,const struct jc_actor_identity *);
enum { JC_ACTOR_OK,JC_ACTOR_ARGUMENT,JC_ACTOR_IDENTITY,JC_ACTOR_READ,
    JC_ACTOR_APPEARANCE_BUSY,JC_ACTOR_RECEIVER,JC_ACTOR_CHANGED,JC_ACTOR_WRITE,
    JC_ACTOR_QUARANTINED,JC_ACTOR_REPLAY };
#define JC_ACTOR_PUBLICATION_BYTES 0x1e68u
struct jc_actor_publication {
    struct jc_receiver saved;
    unsigned char before[JC_ACTOR_PUBLICATION_BYTES],after[JC_ACTOR_PUBLICATION_BYTES];
    struct { uint16_t offset,bytes; } journal[512];
    uint64_t frame,peer,generation;
    unsigned changed_spans,attempted_spans,restored_spans,quarantined,detail;
    unsigned receiver_stage;
    float clocks[2];
};
static inline int jc_actor_identity_check(jc_layout_reader read,void *ctx,
        const struct jc_actor_identity *id) {
    uint64_t barn,barn_type,actor_type,mesh,glyph;
    if (!id || !id->peer || !id->generation || !id->glyph || id->actor==id->barn+0xb0 ||
        id->actor==id->barn+0x146f0) return 0;
    return jc_layout_field(read,ctx,id->game,0x1e0,&barn,8) && barn==id->barn &&
        jc_layout_field(read,ctx,barn,0,&barn_type,8) && barn_type==id->image+0x5da340 &&
        jc_layout_field(read,ctx,id->actor,0,&actor_type,8) && actor_type==id->image+0x5d0380 &&
        jc_layout_field(read,ctx,id->actor,0xa620,&mesh,8) && mesh==id->actor+0x160 &&
        jc_layout_field(read,ctx,id->actor,0x2240,&glyph,8) && glyph==id->glyph;
}
static inline int jc_actor_publication_owner(jc_layout_reader read,void *ctx,
        const struct jc_actor_identity *id,uint32_t chapter) {
    uint32_t current;
    return chapter<=31 && jc_actor_identity_check(read,ctx,id) &&
        jc_layout_field(read,ctx,id->barn,0x14,&current,4) && current==chapter;
}
static inline int jc_actor_numeric_byte(unsigned offset) {
    static const uint16_t spans[][2]={
        {0x10,0x6c},{0x90,16},{0xcc,4},{0x1430,4},{0x1448,4},{0x1450,4},{0x1458,4},{0x1470,4},{0x1479,2},{0x1e64,4},
        {0x17f0,8},{0x1800,0x68},{0x1870,0xf0},{0x1960,1},{0x1964,4},
        {0x1970,0xbc},{0x1a30,8},{0x1a40,2},{0x1a44,4},{0x1a50,0x58},
        {0x1ba0,4},{0x1bc0,16},{0x1bd4,1},{0x1bf0,32}
    };
    for (unsigned i=0;i<sizeof(spans)/sizeof(spans[0]);++i)
        if (offset>=spans[i][0] && offset-spans[i][0]<spans[i][1]) return 1;
    return 0;
}
static inline void jc_actor_receiver_native(void *context,void *barn,void *game) {
    uint64_t image=(uint64_t)(uintptr_t)context;
    ((void (JC_ACTOR_ABI *)(void *,void *))(uintptr_t)(image+0x118730))(barn,game);
}
/* Caller serializes on the retained actor's game thread before simulation, and
   supplies the retained identity instead of discovering/adopting an arbitrary
   Dude. Retirement callback proves no pending mesh/cloth/render references when
   admitting a changed scarf capacity, outfit or symbol. This publishes numeric
   data only: a54a0/a6350 remain outside, under the caller's appearance scope. */
static inline int jc_actor_publish_numeric(jc_layout_reader read,jc_actor_writer write,
        jc_actor_retired retired,void *ctx,const struct jc_actor_identity *id,
        uint64_t frame,struct jc_receiver *receiver,struct jc_actor_input *input,
        struct jc_actor_publication *work) {
    if (!read || !write || !retired || !id || !frame || !receiver || !input || !work)
        return JC_ACTOR_ARGUMENT;
    if (work->quarantined) return JC_ACTOR_QUARANTINED;
    if (!jc_actor_chirp_shape_valid(input->chirp_shape) || !isfinite(input->body_scale) || input->body_scale<=0 || input->body_scale>1000)
        return JC_ACTOR_ARGUMENT;
    for (unsigned n=0;n<3;++n) if (!isfinite(input->glow_targets[n]) ||
        input->glow_targets[n]<0 || input->glow_targets[n]>16) return JC_ACTOR_ARGUMENT;
    if (!receiver->present || !receiver->received || receiver->peer!=id->peer ||
        receiver->generation!=id->generation || input->peer!=receiver->peer ||
        input->generation!=receiver->generation || input->sequence!=receiver->sequence ||
        !jc_actor_identity_check(read,ctx,id)) return JC_ACTOR_IDENTITY;
    uint32_t chapter;
    if (!jc_layout_field(read,ctx,id->barn,0x14,&chapter,4)) return JC_ACTOR_READ;
    if (chapter>31 || chapter!=input->chapter) return JC_ACTOR_IDENTITY;
    if (id->generation<work->generation || (id->generation==work->generation &&
        (id->peer!=work->peer || frame<=work->frame))) return JC_ACTOR_REPLAY;
    work->changed_spans=work->attempted_spans=work->restored_spans=work->detail=0;
    uint64_t clock,clock_again;float clocks[2];
    if (!jc_layout_field(read,ctx,id->actor,0,work->before,sizeof(work->before)) ||
        !jc_layout_field(read,ctx,id->game,0x68,&clock,8) ||
        !jc_layout_field(read,ctx,clock,0xc,clocks,sizeof(clocks))) return JC_ACTOR_READ;
    uint32_t appearance[4];memcpy(appearance,receiver->image+0x28d88,sizeof(appearance));
    if ((memcmp(work->before+0x6c,appearance+1,12) ||
        memcmp(work->before+0x1430,&input->body_scale,4)) && !retired(ctx,id)) return JC_ACTOR_APPEARANCE_BUSY;
    memcpy(&work->saved,receiver,sizeof(*receiver));
    memcpy(work->after,work->before,sizeof(work->after));
    struct jc_actor_input input_before=*input;
    struct jc_receiver_frame native_frame={.dt=clocks[0],.native_gate_a4=input->inactive};
    memcpy(native_frame.current.bytes,work->before+0x10,JC_RECEIVER_STATE_BYTES);
    memcpy(&native_frame.chirp_cooldown,work->before+0x1468,4);
    native_frame.interaction_delay=jc_receiver_float(receiver->image+0x28da0);
    struct jc_receiver_output evaluated={0};
    int code=JC_ACTOR_RECEIVER;
    memcpy(work->clocks,clocks,sizeof(clocks));work->receiver_stage=1;
    work->detail=(unsigned)jc_receiver_step(receiver,&native_frame,jc_actor_receiver_native,
        (void *)(uintptr_t)id->image,&evaluated);
    if (work->detail) goto restore_numeric;
    work->receiver_stage=2;
    work->detail=(unsigned)jc_receiver_project(receiver,&native_frame,work->after,sizeof(work->after));
    if (work->detail) goto restore_numeric;
    /* The original receiver preserves recipient +74; admission uses the sender
       identity. All four fields come from the accepted peer packet. */
    memcpy(work->after+0x68,appearance,sizeof(appearance));
    memcpy(work->after+0x1430,&input->body_scale,4);
    memcpy(work->after+0x1470,input->chirp_shape,4);
    memcpy(work->after+0x1e64,input->chirp_shape+1,4);
    for (unsigned n=0;n<3;++n) {
        float current=jc_receiver_float(work->before+0x1448+n*8);
        if (!isfinite(current) || current<0 || current>16) {work->detail=JC_RECEIVER_VALUE;goto restore_numeric;}
        jc_receiver_store(work->after+0x1448+n*8,fmaxf(current,input->glow_targets[n]));
    }
    float incoming[4];memcpy(incoming,receiver->image+0x28d30,16);
    float delay=evaluated.interaction_delay;
    work->receiver_stage=3;
    work->detail=(unsigned)jc_actor_correct_input(id->image,work->after,sizeof(work->after),incoming,input,&delay,clocks[1]);
    if (work->detail) goto restore_numeric;
    jc_receiver_store(receiver->image+0x28da0,delay);
    input->packet_age=jc_receiver_float(receiver->image+0x28dc4);
    for (unsigned at=0;at<sizeof(work->after);) {
        if (work->before[at]==work->after[at]) { ++at;continue; }
        unsigned begin=at;
        do {
            if (!jc_actor_numeric_byte(at)) { code=JC_ACTOR_CHANGED;work->detail=at;goto restore_numeric; }
            ++at;
        } while (at<sizeof(work->after) && work->before[at]!=work->after[at]);
        if (work->changed_spans>=sizeof(work->journal)/sizeof(work->journal[0])) {
            code=JC_ACTOR_CHANGED;goto restore_numeric;
        }
        work->journal[work->changed_spans].offset=(uint16_t)begin;
        work->journal[work->changed_spans++].bytes=(uint16_t)(at-begin);
    }
    unsigned char fresh[JC_ACTOR_PUBLICATION_BYTES];
    if (!jc_actor_publication_owner(read,ctx,id,input->chapter) ||
        !jc_layout_field(read,ctx,id->game,0x68,&clock_again,8) || clock_again!=clock) {
        code=JC_ACTOR_IDENTITY;goto restore_numeric;
    }
    if (!jc_layout_field(read,ctx,id->actor,0,fresh,sizeof(fresh)) ||
        memcmp(fresh,work->before,sizeof(fresh))) { code=JC_ACTOR_CHANGED;goto restore_numeric; }
    for (unsigned n=0;n<work->changed_spans;++n) {
        unsigned at=work->journal[n].offset,bytes=work->journal[n].bytes;
        if (!jc_actor_publication_owner(read,ctx,id,input->chapter) || !jc_layout_field(read,ctx,id->actor,at,fresh,bytes) ||
            memcmp(fresh,work->before+at,bytes)) { code=JC_ACTOR_CHANGED;goto rollback; }
        work->attempted_spans=n+1;
        if (!write(ctx,id->actor+at,work->after+at,bytes) ||
            !jc_layout_field(read,ctx,id->actor,at,fresh,bytes) || memcmp(fresh,work->after+at,bytes)) {
            code=JC_ACTOR_WRITE;goto rollback;
        }
    }
    if (!jc_actor_publication_owner(read,ctx,id,input->chapter) ||
        !jc_layout_field(read,ctx,id->actor,0,fresh,sizeof(fresh)) || memcmp(fresh,work->after,sizeof(fresh))) {
        code=JC_ACTOR_CHANGED;goto rollback;
    }
    work->frame=frame;work->peer=id->peer;work->generation=id->generation;return JC_ACTOR_OK;
rollback:
    for (unsigned n=work->attempted_spans;n>0;--n) {
        unsigned at=work->journal[n-1].offset,bytes=work->journal[n-1].bytes;
        if (!jc_actor_publication_owner(read,ctx,id,input->chapter) || !jc_layout_field(read,ctx,id->actor,at,fresh,bytes)) {
            work->quarantined=1;break;
        }
        for (unsigned k=0;k<bytes;++k) if (fresh[k]!=work->before[at+k] && fresh[k]!=work->after[at+k])
            work->quarantined=1;
        if (work->quarantined) break;
        if (memcmp(fresh,work->before+at,bytes) &&
            (!write(ctx,id->actor+at,work->before+at,bytes) ||
             !jc_layout_field(read,ctx,id->actor,at,fresh,bytes) || memcmp(fresh,work->before+at,bytes))) {
            work->quarantined=1;break;
        }
        ++work->restored_spans;
    }
    if (!work->quarantined && (!jc_actor_publication_owner(read,ctx,id,input->chapter) ||
        !jc_layout_field(read,ctx,id->actor,0,fresh,sizeof(fresh)) || memcmp(fresh,work->before,sizeof(fresh))))
        work->quarantined=1;
restore_numeric:
    memcpy(receiver,&work->saved,sizeof(*receiver));*input=input_before;
    return work->quarantined ? JC_ACTOR_QUARANTINED : code;
}
#undef JC_ACTOR_ABI
#endif
