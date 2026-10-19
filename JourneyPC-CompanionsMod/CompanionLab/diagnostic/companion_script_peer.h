#ifndef JC_COMPANION_SCRIPT_PEER_H
#define JC_COMPANION_SCRIPT_PEER_H
#include "companion_actor_stream.h"
struct jc_script_peer {
    struct jc_actor_stream stream;
    struct jc_actor_sender anchor,delayed;
    uint64_t started,next,serial,delayed_sequence,chirp;
    unsigned slot,dropped;
    int (*deliver)(void *,struct jc_actor_stream *,uint64_t,const struct jc_actor_sender *,uint64_t);
    void *delivery_context;
};
static inline int jc_script_peer_begin(struct jc_script_peer *s,const struct jc_actor_sender *anchor,
        uint64_t peer,uint64_t generation,uint64_t now,unsigned slot,unsigned outfit,unsigned symbol) {
    if(!s || !anchor || slot>1 || outfit>6 || symbol>20)return JC_STREAM_ARGUMENT;
    struct jc_actor_sender first;
    float position[4];memcpy(position,anchor->state.bytes,sizeof(position));
    uint32_t scarf[4]={8+slot*4,8+slot*4,outfit,symbol};
    if(!jc_actor_sender_spawn(position,anchor->chapter,anchor->body_scale,anchor->chirp_shape,scarf,&first))
        return JC_STREAM_ARGUMENT;
    int code=jc_actor_stream_open(&s->stream,peer,generation,&first,now);if(code)return code;
    s->anchor=first;s->started=now;s->next=now;s->serial=s->delayed_sequence=s->chirp=0;s->slot=slot;s->dropped=0;return 0;
}
static inline int jc_script_peer_deliver(struct jc_script_peer *s,uint64_t sequence,const struct jc_actor_sender *sample,uint64_t now) {
    if(s->deliver)return s->deliver(s->delivery_context,&s->stream,sequence,sample,now);
    return jc_actor_stream_receive(&s->stream,s->stream.peer,s->stream.generation,sequence,sample,now);
}
static inline int jc_script_peer_step(struct jc_script_peer *s,uint64_t now,struct jc_actor_sender *out) {
    int code=jc_actor_stream_poll(&s->stream,now,2000);if(code)return code;
    if(now<s->next)return JC_STREAM_HELD;
    s->next=now+50;++s->serial;
    uint64_t elapsed=now-s->started;
    if(elapsed>=5000+s->slot*1000 && elapsed<8000+s->slot*1000){++s->dropped;return JC_STREAM_HELD;}
    struct jc_actor_sender sample=s->anchor;
    float t=(float)elapsed*.001f,phase=t*(s->slot?.6f:.8f);
    jc_receiver_store(sample.state.bytes,jc_receiver_float(s->anchor.state.bytes)+sinf(phase)*1.5f);
    jc_receiver_store(sample.state.bytes+8,jc_receiver_float(s->anchor.state.bytes+8)+cosf(phase)*1.5f);
    if(s->serial%7==0){++s->dropped;return JC_STREAM_HELD;}
    if(s->serial%5==0){s->delayed=sample;s->delayed_sequence=s->serial;++s->dropped;return JC_STREAM_HELD;}
    code=jc_script_peer_deliver(s,s->serial,&sample,now);
    if(code)return code;
    if(s->delayed_sequence) {
        code=jc_script_peer_deliver(s,s->delayed_sequence,&s->delayed,now);
        if(s->deliver && code)return code;
        s->delayed_sequence=0;
    }
    code=jc_script_peer_deliver(s,s->serial,&sample,now);
    if(s->deliver && code)return code;
    *out=s->stream.sample;return 0;
}
#endif
