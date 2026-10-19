#include "companion_actor_packet.h"
#include <assert.h>
#include <stdio.h>
#define JC_STREAM_UDP 1
#define JC_TRAVELER_STREAM_UDP_H
static unsigned traveler_stream_waiting[2];
static uint64_t ticks,key=7;
static unsigned char build[16]={1};
static struct {unsigned ready,pending,deliver,opens;unsigned char packet[JC_ACTOR_PACKET_BYTES];} traveler_udp[2];
static uint64_t GetTickCount64(void){return ticks;}
static int mac(void *ctx,const unsigned char *p,size_t n,unsigned char out[32]) {
    (void)ctx;uint64_t value=key;for(size_t i=0;i<n;++i)value=(value^p[i])*UINT64_C(1099511628211);
    for(unsigned i=0;i<4;++i)jc_packet_put(out+i*8,value+i,8);return 1;
}
static void traveler_stream_close(unsigned slot) {
    traveler_stream_waiting[slot]=0;traveler_udp[slot].ready=traveler_udp[slot].pending=0;
}
static int traveler_udp_open(unsigned slot) {
    traveler_stream_close(slot);traveler_udp[slot].ready=1;++traveler_udp[slot].opens;return 0;
}
static int traveler_udp_send(void *ctx,struct jc_actor_stream *stream,uint64_t sequence,
        const struct jc_actor_sender *sender,uint64_t now) {
    (void)now;unsigned slot=ctx==&traveler_udp[1];assert(ctx==&traveler_udp[slot] && traveler_udp[slot].ready);
    assert(jc_actor_packet_encode(build,stream->peer,stream->generation,sequence,sender,mac,NULL,traveler_udp[slot].packet));
    traveler_udp[slot].pending=1;return 0;
}
static int traveler_udp_poll(unsigned slot,struct jc_actor_stream *stream,uint64_t now) {
    if(traveler_udp[slot].deliver && traveler_udp[slot].pending) {
        traveler_udp[slot].pending=0;
        jc_actor_packet_receive(stream,build,traveler_udp[slot].packet,JC_ACTOR_PACKET_BYTES,now,mac,NULL);
    }
    return 0;
}
#include "traveler_stream_test.h"
int main(void) {
    traveler_stream_enabled=1;assert(traveler_stream_enabled);
    struct jc_actor_sender anchor={.body_scale=.1f,.chapter=2,.chirp_shape={1,1}},sample=anchor;
    jc_receiver_store(anchor.state.bytes,10);sample=anchor;ticks=100;
    assert(traveler_stream_sample(0,1,42,1,3,4,&sample)==JC_STREAM_HELD);
    assert(traveler_stream_waiting[0] && traveler_streams[0].stream.accepted==0);
    ticks=150;assert(traveler_stream_sample(1,1,43,1,4,5,&sample)==JC_STREAM_HELD);
    ticks=349;assert(traveler_stream_sample(0,1,42,1,3,4,&sample)==JC_STREAM_HELD);
    assert(!traveler_udp[0].pending && traveler_udp[0].opens==1);
    ticks=350;assert(traveler_stream_sample(0,1,42,1,3,4,&sample)==JC_STREAM_HELD);
    assert(traveler_udp[0].pending);
    traveler_udp[0].packet[100]^=1;traveler_udp[0].deliver=1;
    ticks=351;assert(traveler_stream_sample(0,1,42,1,3,4,&sample)==JC_STREAM_HELD);
    assert(!traveler_streams[0].stream.accepted);
    ticks=400;assert(!traveler_stream_sample(0,1,42,1,3,4,&sample));
    assert(!traveler_stream_waiting[0] && traveler_stream_waiting[1]);
    assert(jc_receiver_float(sample.state.bytes)==12 && sample.state.bytes[0x60]==3 && sample.state.bytes[0x64]==4);
    assert(traveler_streams[0].stream.sequence==2 && traveler_udp[0].opens==1);
    ticks=2150;assert(traveler_stream_sample(1,1,43,1,4,5,&sample)==JC_STREAM_TIMEOUT);
    assert(!traveler_udp[1].ready && traveler_udp[0].ready);
    sample=anchor;ticks=2200;
    assert(traveler_stream_sample(1,1,43,2,5,6,&sample)==JC_STREAM_HELD);
    traveler_udp[1].deliver=1;ticks=2450;
    assert(!traveler_stream_sample(1,1,43,2,5,6,&sample));
    assert(jc_receiver_float(sample.state.bytes)==7 && sample.state.bytes[0x60]==5);
    assert(traveler_udp[1].opens==2 && traveler_streams[0].stream.generation==1);
    traveler_stream_close(0);traveler_stream_close(1);
    assert(!traveler_stream_waiting[0] && !traveler_stream_waiting[1]);
    puts("Production admission adapter: delayed first packet, corrupted packet, retries without reopening, independent slots, timeout, reconnect and cancellation passed; socket I/O mocked.");
}
