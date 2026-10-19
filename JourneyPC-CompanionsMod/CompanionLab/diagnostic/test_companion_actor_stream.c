#include "companion_script_peer.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct jc_actor_sender a={.body_scale=.1f,.chirp_shape={1,1},.chapter=2},out;
    assert(!jc_actor_sender_invalid_field(&a));
    struct jc_actor_sender bad=a;bad.body_scale=0;assert(jc_actor_sender_invalid_field(&bad)==8 && !jc_actor_sender_valid(&bad));
    bad=a;bad.glow_targets[1]=17;assert(jc_actor_sender_invalid_field(&bad)==12 && !jc_actor_sender_valid(&bad));
    bad=a;bad.chirp_shape[1]=0;assert(jc_actor_sender_invalid_field(&bad)==7 && !jc_actor_sender_valid(&bad));
    bad=a;bad.state.bytes[0x58]=30;assert(jc_actor_sender_invalid_field(&bad)==2 && !jc_actor_sender_valid(&bad));
    bad.state.bytes[0x5c]=30;assert(!jc_actor_sender_invalid_field(&bad) && jc_actor_sender_valid(&bad));
    struct jc_actor_sender moving=a;
    for(unsigned i=0x10;i<0x40;i+=4)jc_receiver_store(moving.state.bytes+i,.75f);
    memset(moving.state.bytes+0x40,1,12);
    for(unsigned i=0x4c;i<=0x54;i+=4)jc_receiver_store(moving.state.bytes+i,.5f);
    moving.state.bytes[0x68]=3;
    for(unsigned i=0;i<4;++i){moving.axes[i]=.5f;moving.direction[i]=.25f;}
    moving.scalars[0]=.75f;moving.glow_targets[0]=1;
    struct jc_script_peer idle_peer={0},moving_peer={0};
    assert(jc_actor_sender_valid(&moving));
    assert(!jc_script_peer_begin(&idle_peer,&a,1,1,100,0,3,4));
    assert(!jc_script_peer_begin(&moving_peer,&moving,2,1,100,0,3,4));
    assert(!memcmp(&idle_peer.anchor,&moving_peer.anchor,sizeof(a)));
    assert(moving.axes[0]==.5f && moving.state.bytes[0x40]==1);
    assert(moving_peer.anchor.state.bytes[0x60]==3 && moving_peer.anchor.state.bytes[0x64]==4);
    struct jc_actor_stream genuine={0};
    assert(!jc_actor_stream_open(&genuine,3,1,&moving,100));
    assert(!memcmp(&genuine.sample,&moving,sizeof(moving)));
    puts("Spawn isolation: differing host motion/flags yield identical neutral peer states; source unchanged and genuine sender motion preserved.");
    struct jc_actor_stream s={0};
    assert(!jc_actor_stream_open(&s,10,1,&a,100));
    assert(!jc_actor_stream_receive(&s,10,1,2,&a,150));
    struct jc_actor_sender before=s.sample;
    assert(jc_actor_stream_receive(&s,10,1,2,&a,151)==JC_STREAM_REPLAY);
    assert(jc_actor_stream_receive(&s,10,1,1,&a,152)==JC_STREAM_REPLAY);
    assert(jc_actor_stream_receive(&s,11,1,3,&a,153)==JC_STREAM_IDENTITY);
    assert(jc_actor_stream_receive(&s,10,2,3,&a,153)==JC_STREAM_IDENTITY);
    ++a.chapter;assert(jc_actor_stream_receive(&s,10,1,3,&a,154)==JC_STREAM_CHAPTER);--a.chapter;

    assert(!memcmp(&before,&s.sample,sizeof(before)) && s.last_seen==150);
    assert(jc_actor_stream_poll(&s,149,2000)==JC_STREAM_CLOCK);
    assert(!jc_actor_stream_poll(&s,2149,2000));assert(jc_actor_stream_poll(&s,2150,2000)==JC_STREAM_TIMEOUT);
    assert(jc_actor_stream_receive(&s,10,1,3,&a,2151)==JC_STREAM_IDENTITY);
    assert(!jc_actor_stream_open(&s,10,2,&a,2200));
    assert(jc_actor_stream_receive(&s,10,1,99,&a,2201)==JC_STREAM_IDENTITY);
    assert(!jc_actor_stream_receive(&s,10,2,1,&a,2202));
    for(unsigned robe=0;robe<7;++robe){
        a.state.bytes[0x60]=(unsigned char)robe;
        a.state.bytes[0x58]=a.state.bytes[0x5c]=30;
        assert(!jc_actor_stream_receive(&s,10,2,2+robe,&a,2202));
        assert(s.sample.state.bytes[0x60]==robe && !s.departing && s.generation==2);
    }
    assert(!jc_actor_stream_depart(&s,10,2));
    assert(jc_actor_stream_receive(&s,10,2,2,&a,2203)==JC_STREAM_IDENTITY);
    a.body_scale=NAN;assert(jc_actor_stream_receive(&s,10,2,2,&a,2203)==JC_STREAM_ARGUMENT);a.body_scale=.1f;
    struct jc_script_peer peers[2]={0};
    assert(!jc_script_peer_begin(peers,&a,21,1,1,0,0,2));
    assert(!jc_script_peer_begin(peers+1,&a,22,1,901,1,6,3));
    unsigned expired[2]={0};
    for(uint64_t now=1;now<13000;now+=50)for(unsigned i=0;i<2;++i) {
        if(now<peers[i].started || expired[i])continue;
        int code=jc_script_peer_step(peers+i,now,&out);
        assert(!code || code==JC_STREAM_HELD || code==JC_STREAM_TIMEOUT);
        if(code==JC_STREAM_TIMEOUT)expired[i]=1;
        if(!code){assert(jc_actor_sender_valid(&out));assert(out.state.bytes[0x60]==(i?6:0));}
    }
    assert(expired[0] && expired[1]);
    for(unsigned i=0;i<2;++i)assert(peers[i].stream.accepted>20 && peers[i].stream.rejected>20 && peers[i].dropped>20);
    assert(!jc_script_peer_begin(peers,&a,21,2,14000,0,1,4));
    assert(!jc_script_peer_step(peers,14000,&out));assert(peers[1].stream.departing);
    puts("Actor streams: independent identities, complete numeric states, duplicate/reorder rejection, chapter guards, in-place robe/scarf changes, silence timeout, reconnect and stale-epoch rejection passed. Scripted source exercised deterministic loss and delay; no sockets or Journey session.");
    return 0;
}
