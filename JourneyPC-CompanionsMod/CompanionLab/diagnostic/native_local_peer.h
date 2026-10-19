#ifndef JC_NATIVE_LOCAL_PEER_H
#define JC_NATIVE_LOCAL_PEER_H
#include "native_actor_live.h"
struct jc_local_peer_patch {unsigned offset,bytes;unsigned char value[JC_RECEIVER_STATE_BYTES],before[JC_RECEIVER_STATE_BYTES];};
struct jc_local_peer_packet {struct jc_local_peer_patch rows[40];unsigned count;};
static inline void jc_local_peer_add(struct jc_local_peer_packet *p,unsigned offset,const void *value,unsigned bytes) {
    struct jc_local_peer_patch *r=p->rows+p->count++;r->offset=offset;r->bytes=bytes;memcpy(r->value,value,bytes);
}
static inline int jc_local_peer_packet_make(const struct jc_actor_sender *sender,unsigned first,
        unsigned outfit,unsigned symbol,struct jc_local_peer_packet *p) {
    if(!p || !jc_actor_sender_valid(sender) || first>1 || outfit>6 || symbol>20)return 1;
    memset(p,0,sizeof(*p));struct jc_receiver_state state=sender->state;
    memcpy(state.bytes+0x60,&outfit,4);memcpy(state.bytes+0x64,&symbol,4);
    if(!jc_receiver_state_valid(&state))return 1;
    float zero=0;unsigned char gates[3]={1,0,0},snap=1;
    jc_local_peer_add(p,0x28d30,state.bytes,sizeof(state.bytes));
    jc_local_peer_add(p,0x64,&sender->chapter,4);
    jc_local_peer_add(p,0x70,sender->scalars,8);
    jc_local_peer_add(p,0x80,sender->axes,16);
    jc_local_peer_add(p,0x90,sender->direction,16);
    jc_local_peer_add(p,0x28dc4,&zero,4);
    jc_local_peer_add(p,0x146f0+0x1430,&sender->body_scale,4);
    jc_local_peer_add(p,0x146f0+0x1470,sender->chirp_shape,4);
    jc_local_peer_add(p,0x146f0+0x1e64,sender->chirp_shape+1,4);
    for(unsigned i=0;i<3;++i)jc_local_peer_add(p,0x146f0+0x1448+i*8,sender->glow_targets+i,4);
    jc_local_peer_add(p,0x146f0+0x74,&symbol,4);
    if(first) {
        unsigned char clear=0;uint32_t empty=0;
        jc_local_peer_add(p,0x146f0+0x1e00,&zero,4);
        jc_local_peer_add(p,0x146f0+0x2000,&zero,4);
        const unsigned flags[]={0x56,0x1e1c,0x1dfc,0x201c};
        for(unsigned i=0;i<4;++i)jc_local_peer_add(p,0x146f0+flags[i],&clear,1);
        jc_local_peer_add(p,0x146f0+0xa390+0x999a,&clear,1);
        jc_local_peer_add(p,0x146f0+0xa390+0x6734,&empty,4);
        for(unsigned i=0;i<16;++i)jc_local_peer_add(p,0x146f0+0xa390+0x64cd+i*0x28,&clear,1);
        jc_local_peer_add(p,0xa1,&snap,1);
    }
    jc_local_peer_add(p,0xa2,gates,3);
    return 0;
}
static inline int jc_local_peer_publish(jc_layout_reader read,jc_actor_writer write,void *ctx,
        uint64_t barn,struct jc_local_peer_packet *p) {
    if(!p || !p->count || p->count>40)return 1;
    for(unsigned i=0;i<p->count;++i)
        if(!read(ctx,barn+p->rows[i].offset,p->rows[i].before,p->rows[i].bytes))return 2;
    for(unsigned i=0;i<p->count;++i) {
        struct jc_local_peer_patch *r=p->rows+i;
        if(r->bytes==4 && (r->offset==0x15b38 || r->offset==0x15b40 || r->offset==0x15b48)) {
            float prior=jc_receiver_float(r->before),incoming=jc_receiver_float(r->value);
            if(!isfinite(prior) || !isfinite(incoming))return 2;
            jc_receiver_store(r->value,fmaxf(prior,incoming));
        }
    }
    unsigned attempted=0;unsigned char check[JC_RECEIVER_STATE_BYTES];
    for(unsigned i=0;i<p->count;++i) {
        struct jc_local_peer_patch *r=p->rows+i;attempted=i+1;
        if(!write(ctx,barn+r->offset,r->value,r->bytes) || !read(ctx,barn+r->offset,check,r->bytes) ||
            memcmp(check,r->value,r->bytes))goto rollback;
    }
    return 0;
rollback:
    for(unsigned i=attempted;i;--i) {
        struct jc_local_peer_patch *r=p->rows+i-1;
        if(!write(ctx,barn+r->offset,r->before,r->bytes) || !read(ctx,barn+r->offset,check,r->bytes) ||
            memcmp(check,r->before,r->bytes))return 4;
    }
    return 3;
}
#endif
