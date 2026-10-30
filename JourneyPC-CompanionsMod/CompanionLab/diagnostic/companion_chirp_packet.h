#ifndef JC_COMPANION_CHIRP_PACKET_H
#define JC_COMPANION_CHIRP_PACKET_H
#include "companion_actor_packet.h"
#include "native_companion_chirp.h"
#define JC_CHIRP_PACKET_BODY 88u
#define JC_CHIRP_PACKET_BYTES 120u
struct jc_chirp_inbox {
    struct jc_chirp_event event;
    uint64_t sequence,received_at;
    unsigned pending,accepted,rejected;
};
static inline int jc_chirp_packet_encode(const unsigned char build[16],uint32_t chapter,
        const struct jc_chirp_event *e,jc_actor_packet_mac mac,void *ctx,unsigned char out[JC_CHIRP_PACKET_BYTES]) {
    if(!build || chapter>31 || !jc_chirp_event_valid(e) || !mac || !out)return 0;
    memset(out,0,JC_CHIRP_PACKET_BYTES);memcpy(out,"JNC1",4);
    jc_packet_put(out+4,1,2);jc_packet_put(out+6,JC_CHIRP_PACKET_BYTES,2);memcpy(out+8,build,16);
    jc_packet_put(out+24,e->participant,8);jc_packet_put(out+32,e->epoch,8);jc_packet_put(out+40,e->sequence,8);
    jc_packet_put(out+48,chapter,4);jc_packet_put(out+52,e->sound,4);
    float values[8];memcpy(values,e->position,12);memcpy(values+3,e->velocity,12);values[6]=e->radius;values[7]=e->duration;
    for(unsigned i=0;i<8;++i){uint32_t bits;memcpy(&bits,values+i,4);jc_packet_put(out+56+i*4,bits,4);}
    return mac(ctx,out,JC_CHIRP_PACKET_BODY,out+JC_CHIRP_PACKET_BODY);
}
static inline int jc_chirp_packet_receive(struct jc_chirp_inbox *in,const struct jc_actor_stream *stream,
        const unsigned char build[16],const unsigned char *p,size_t bytes,uint64_t now,jc_actor_packet_mac mac,void *ctx) {
    if(!in || !stream || !build || !p || bytes!=JC_CHIRP_PACKET_BYTES || memcmp(p,"JNC1",4) ||
        jc_packet_get(p+4,2)!=1 || jc_packet_get(p+6,2)!=bytes || memcmp(p+8,build,16) || !mac)return JC_STREAM_ARGUMENT;
    unsigned char tag[32];if(!mac(ctx,p,JC_CHIRP_PACKET_BODY,tag))return JC_STREAM_ARGUMENT;
    unsigned difference=0;for(unsigned i=0;i<32;++i)difference|=tag[i]^p[JC_CHIRP_PACKET_BODY+i];
    if(difference)return JC_STREAM_IDENTITY;
    struct jc_chirp_event e={.participant=jc_packet_get(p+24,8),.epoch=jc_packet_get(p+32,8),
        .sequence=jc_packet_get(p+40,8),.sound=(uint32_t)jc_packet_get(p+52,4)};
    float values[8];for(unsigned i=0;i<8;++i){uint32_t bits=(uint32_t)jc_packet_get(p+56+i*4,4);memcpy(values+i,&bits,4);}
    memcpy(e.position,values,12);memcpy(e.velocity,values+3,12);e.radius=values[6];e.duration=values[7];
    int code=0;
    if(!jc_chirp_event_valid(&e))code=JC_STREAM_ARGUMENT;
    else if(stream->departing || e.participant!=stream->peer || e.epoch!=stream->generation)code=JC_STREAM_IDENTITY;
    else if(jc_packet_get(p+48,4)!=stream->sample.chapter)code=JC_STREAM_CHAPTER;
    else if(now<stream->clock || now<stream->last_seen)code=JC_STREAM_CLOCK;
    else if(now-stream->last_seen>=2000)code=JC_STREAM_TIMEOUT;
    else if(e.sequence<=in->sequence)code=JC_STREAM_REPLAY;
    else if(in->pending)code=JC_STREAM_HELD;
    if(code){++in->rejected;return code;}
    in->event=e;in->sequence=e.sequence;in->received_at=now;in->pending=1;++in->accepted;return 0;
}
static inline int jc_chirp_inbox_take(struct jc_chirp_inbox *in,const struct jc_actor_stream *stream,
        uint64_t now,struct jc_chirp_event *event) {
    if(!in || !stream || !event || !in->pending)return 0;
    in->pending=0;
    if(stream->departing || in->event.participant!=stream->peer || in->event.epoch!=stream->generation ||
        now<in->received_at || now-in->received_at>500 || now<stream->last_seen || now-stream->last_seen>=2000)return 0;
    *event=in->event;return 1;
}
#endif
