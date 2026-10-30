#ifndef JC_COMPANION_ACTOR_PACKET_H
#define JC_COMPANION_ACTOR_PACKET_H
#include "companion_actor_stream.h"
#define JC_ACTOR_PACKET_BODY 224u
#define JC_ACTOR_PACKET_BYTES 256u
typedef int (*jc_actor_packet_mac)(void *,const unsigned char *,size_t,unsigned char[32]);
static inline void jc_packet_put(unsigned char *p,uint64_t v,unsigned n) {
    for(unsigned i=0;i<n;++i)p[i]=(unsigned char)(v>>(i*8));
}
static inline uint64_t jc_packet_get(const unsigned char *p,unsigned n) {
    uint64_t v=0;for(unsigned i=0;i<n;++i)v|=(uint64_t)p[i]<<(i*8);return v;
}
static inline int jc_actor_packet_encode(const unsigned char build[16],uint64_t peer,uint64_t generation,
        uint64_t sequence,const struct jc_actor_sender *s,jc_actor_packet_mac mac,void *ctx,
        unsigned char out[JC_ACTOR_PACKET_BYTES]) {
    if(!build || !peer || !generation || !sequence || !jc_actor_sender_valid(s) || !mac || !out)return 0;
    memset(out,0,JC_ACTOR_PACKET_BYTES);memcpy(out,"JNA1",4);
    jc_packet_put(out+4,1,2);jc_packet_put(out+6,JC_ACTOR_PACKET_BYTES,2);memcpy(out+8,build,16);
    jc_packet_put(out+24,peer,8);jc_packet_put(out+32,generation,8);jc_packet_put(out+40,sequence,8);
    jc_packet_put(out+48,s->chapter,4);
    for(unsigned i=0;i<JC_RECEIVER_STATE_BYTES;i+=4) {
        uint32_t bits;memcpy(&bits,s->state.bytes+i,4);jc_packet_put(out+52+i,bits,4);
    }
    float values[16];memcpy(values,s->axes,16);memcpy(values+4,s->direction,16);
    memcpy(values+8,s->scalars,8);values[10]=s->body_scale;
    memcpy(values+11,s->glow_targets,12);memcpy(values+14,s->chirp_shape,8);
    for(unsigned i=0;i<16;++i){uint32_t bits;memcpy(&bits,values+i,4);jc_packet_put(out+160+i*4,bits,4);}
    return mac(ctx,out,JC_ACTOR_PACKET_BODY,out+JC_ACTOR_PACKET_BODY);
}
static inline int jc_actor_packet_decode(struct jc_actor_sender *out,const unsigned char build[16],
        const unsigned char *p,size_t bytes,uint64_t now,jc_actor_packet_mac mac,void *ctx) {
    if(!out || !build || !p || bytes!=JC_ACTOR_PACKET_BYTES || memcmp(p,"JNA1",4) ||
        jc_packet_get(p+4,2)!=1 || jc_packet_get(p+6,2)!=bytes || memcmp(p+8,build,16) || !mac)return JC_STREAM_ARGUMENT;
    unsigned char tag[32];if(!mac(ctx,p,JC_ACTOR_PACKET_BODY,tag))return JC_STREAM_ARGUMENT;
    unsigned difference=0;for(unsigned i=0;i<32;++i)difference|=tag[i]^p[JC_ACTOR_PACKET_BODY+i];
    if(difference)return JC_STREAM_IDENTITY;
    struct jc_actor_sender s={0};s.chapter=(uint32_t)jc_packet_get(p+48,4);
    for(unsigned i=0;i<JC_RECEIVER_STATE_BYTES;i+=4) {
        uint32_t bits=(uint32_t)jc_packet_get(p+52+i,4);memcpy(s.state.bytes+i,&bits,4);
    }
    float values[16];for(unsigned i=0;i<16;++i){uint32_t bits=(uint32_t)jc_packet_get(p+160+i*4,4);memcpy(values+i,&bits,4);}
    memcpy(s.axes,values,16);memcpy(s.direction,values+4,16);memcpy(s.scalars,values+8,8);
    s.body_scale=values[10];memcpy(s.glow_targets,values+11,12);memcpy(s.chirp_shape,values+14,8);
    (void)now;
    if(!jc_actor_sender_valid(&s))return JC_STREAM_ARGUMENT;
    *out=s;return 0;
}
static inline int jc_actor_packet_receive(struct jc_actor_stream *stream,const unsigned char build[16],
        const unsigned char *p,size_t bytes,uint64_t now,jc_actor_packet_mac mac,void *ctx) {
    struct jc_actor_sender s;
    int code=jc_actor_packet_decode(&s,build,p,bytes,now,mac,ctx);
    if(code)return code;
    return jc_actor_stream_receive(stream,jc_packet_get(p+24,8),jc_packet_get(p+32,8),jc_packet_get(p+40,8),&s,now);
}
#endif
