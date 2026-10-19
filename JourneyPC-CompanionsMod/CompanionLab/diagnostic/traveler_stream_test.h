#ifndef JC_TRAVELER_STREAM_TEST_H
#define JC_TRAVELER_STREAM_TEST_H
#include "companion_script_peer.h"
static unsigned traveler_stream_waiting[2];
static unsigned traveler_network_active;
#ifdef JC_NETWORK_UDP
static void traveler_network_slot_close(unsigned slot);
#endif
#ifdef JC_STREAM_UDP
#include "traveler_stream_udp.h"
#else
static void traveler_stream_close(unsigned slot) {(void)slot;}
#endif
static unsigned traveler_stream_enabled;
static struct jc_script_peer traveler_streams[2];
#ifdef JC_NETWORK_UDP
#include "traveler_network.h"
#endif
static int traveler_stream_sample(unsigned slot,int first,uint64_t peer,uint64_t generation,
        unsigned outfit,unsigned symbol,struct jc_actor_sender *sender) {
    #ifdef JC_NETWORK_UDP
    if(traveler_network_active)return traveler_network_sample(slot,first,sender);
    #endif
    struct jc_script_peer *s=&traveler_streams[slot];uint64_t now=GetTickCount64();
    if(first && !traveler_stream_waiting[slot]) {
        int code=jc_script_peer_begin(s,sender,peer,generation,now,slot,outfit,symbol);if(code)return code;
#ifdef JC_STREAM_UDP
        code=traveler_udp_open(slot);if(code)return code;
        s->deliver=traveler_udp_send;s->delivery_context=&traveler_udp[slot];
        jc_receiver_store(s->anchor.state.bytes,jc_receiver_float(s->anchor.state.bytes)+(slot?-3.f:2.f));
        s->serial=0;s->stream.sequence=0;
        traveler_stream_waiting[slot]=1;
        s->next=now+250;return JC_STREAM_HELD;
#endif
    }
#ifdef JC_STREAM_UDP
    if(first) {
        if(s->stream.peer!=peer || s->stream.generation!=generation)return JC_STREAM_IDENTITY;
        if(now<s->started || now-s->started>=2000){traveler_stream_close(slot);return JC_STREAM_TIMEOUT;}
        if(now>=s->next) {
            int code=traveler_udp_send(&traveler_udp[slot],&s->stream,++s->serial,&s->anchor,now);
            if(code){traveler_stream_close(slot);return code;}
            s->next=now+50;
        }
        int code=traveler_udp_poll(slot,&s->stream,now);if(code)return code;
        code=jc_actor_stream_initial(&s->stream,sender);
        if(!code){traveler_stream_waiting[slot]=0;s->started=now;s->next=now+50;}
        return code;
    }
    uint64_t previous=s->stream.sequence;
    int code=traveler_udp_poll(slot,&s->stream,now);if(code)return code;
    code=jc_script_peer_step(s,now,sender);
    if(code && code!=JC_STREAM_HELD){traveler_stream_close(slot);return code;}
    code=traveler_udp_poll(slot,&s->stream,now);if(code)return code;
    if(s->stream.sequence==previous)return JC_STREAM_HELD;
    *sender=s->stream.sample;return 0;
#else
    return jc_script_peer_step(s,now,sender);
#endif
}
#endif
