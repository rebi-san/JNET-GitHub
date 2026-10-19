#ifndef JC_COMPANION_ACTOR_STREAM_H
#define JC_COMPANION_ACTOR_STREAM_H
#include "native_actor_live.h"
enum {JC_STREAM_OK,JC_STREAM_ARGUMENT,JC_STREAM_IDENTITY,JC_STREAM_REPLAY,JC_STREAM_CHAPTER,
    JC_STREAM_APPEARANCE,JC_STREAM_CLOCK,JC_STREAM_HELD=100,JC_STREAM_TIMEOUT};
struct jc_actor_stream {
    uint64_t peer,generation,sequence,last_seen,clock;
    unsigned departing,accepted,rejected;
    struct jc_actor_sender sample;
};
static inline int jc_actor_stream_open(struct jc_actor_stream *s,uint64_t peer,uint64_t generation,
        const struct jc_actor_sender *first,uint64_t now) {
    if(!s || !peer || !generation || generation<=s->generation || !jc_actor_sender_valid(first))return JC_STREAM_ARGUMENT;
    *s=(struct jc_actor_stream){.peer=peer,.generation=generation,.last_seen=now,.clock=now,.sample=*first};return 0;
}
static inline int jc_actor_stream_receive(struct jc_actor_stream *s,uint64_t peer,uint64_t generation,
        uint64_t sequence,const struct jc_actor_sender *sample,uint64_t now) {
    if(!s || !jc_actor_sender_valid(sample) || !sequence)return JC_STREAM_ARGUMENT;
    int code=0;
    if(now<s->clock)code=JC_STREAM_CLOCK;
    else if(s->departing || peer!=s->peer || generation!=s->generation)code=JC_STREAM_IDENTITY;
    else if(sequence<=s->sequence)code=JC_STREAM_REPLAY;
    else if(sample->chapter!=s->sample.chapter)code=JC_STREAM_CHAPTER;
    if(code){++s->rejected;return code;}
    s->clock=s->last_seen=now;s->sequence=sequence;s->sample=*sample;++s->accepted;return 0;
}
static inline int jc_actor_stream_initial(const struct jc_actor_stream *s,struct jc_actor_sender *out) {
    if(!s || !out || s->departing)return JC_STREAM_ARGUMENT;
    if(!s->accepted || !s->sequence)return JC_STREAM_HELD;
    if(!jc_actor_sender_valid(&s->sample))return JC_STREAM_ARGUMENT;
    *out=s->sample;return 0;
}
static inline int jc_actor_stream_depart(struct jc_actor_stream *s,uint64_t peer,uint64_t generation) {
    if(!s || !peer || peer!=s->peer || generation!=s->generation)return JC_STREAM_IDENTITY;
    s->departing=1;return 0;
}
static inline int jc_actor_stream_poll(struct jc_actor_stream *s,uint64_t now,uint64_t timeout) {
    if(!s || !s->peer || !timeout)return JC_STREAM_ARGUMENT;
    if(now<s->clock)return JC_STREAM_CLOCK;
    s->clock=now;
    if(s->departing || now-s->last_seen>=timeout){s->departing=1;return JC_STREAM_TIMEOUT;}
    return 0;
}
#endif
