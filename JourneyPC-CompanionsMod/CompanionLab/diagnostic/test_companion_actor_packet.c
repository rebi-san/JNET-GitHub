#include "companion_chirp_packet.h"
#include "jnet_chat.h"
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
/* Deterministic integrity fixture only; production uses BCrypt HMAC-SHA256. */
static int test_mac(void *ctx,const unsigned char *p,size_t n,unsigned char out[32]) {
    uint64_t v=*(uint64_t *)ctx;for(size_t i=0;i<n;++i)v=(v^p[i])*UINT64_C(1099511628211);
    for(unsigned i=0;i<4;++i)jc_packet_put(out+i*8,v+i,8);return 1;
}
static void chirps(void) {
    unsigned char build[16]={17},packet[JC_CHIRP_PACKET_BYTES],bad[JC_CHIRP_PACKET_BYTES];uint64_t key=12345;
    struct jc_actor_sender sample={.body_scale=.1f,.chirp_shape={1,1},.chapter=2};
    struct jc_actor_stream stream={0};assert(!jc_actor_stream_open(&stream,42,1,&sample,100));
    struct jc_chirp_inbox inbox={0};
    struct jc_chirp_event e={.participant=42,.epoch=1,.sequence=1,.sound=3,.position={1,2,3},.velocity={.1f,.2f,.3f},.radius=3,.duration=.5f},out;
    assert(jc_chirp_packet_encode(build,2,&e,test_mac,&key,packet));
    assert(!memcmp(packet,"JNC1",4) && sizeof(packet)==120);
    memcpy(bad,packet,sizeof(packet));memcpy(bad,"JCC1",4);
    assert(test_mac(&key,bad,JC_CHIRP_PACKET_BODY,bad+JC_CHIRP_PACKET_BODY));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,bad,sizeof(bad),101,test_mac,&key)==JC_STREAM_ARGUMENT);
    assert(!inbox.pending && !inbox.sequence);
    uint64_t source=0,sequence=0;char text[121],nonce[33]={0};
    assert(!jnet_chat_unpack(packet,sizeof(packet),1,nonce,&source,&sequence,text));
    unsigned char chat[JNET_CHAT_PACKET]={0};memcpy(chat,"JNC1",4);
    jc_packet_put(chat+8,1,8);jc_packet_put(chat+16,42,8);jc_packet_put(chat+24,1,8);chat[64]='a';
    assert(jnet_chat_unpack(chat,sizeof(chat),1,nonce,&source,&sequence,text));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,chat,sizeof(chat),101,test_mac,&key)==JC_STREAM_ARGUMENT);
    assert(!inbox.pending && !inbox.sequence);
    for(unsigned i=0;i<sizeof(packet);++i) {
        memcpy(bad,packet,sizeof(packet));bad[i]^=1;
        assert(jc_chirp_packet_receive(&inbox,&stream,build,bad,sizeof(bad),101,test_mac,&key));
        assert(!inbox.pending && !inbox.sequence);
        assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,i,101,test_mac,&key));
    }
    assert(!jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),101,test_mac,&key));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),102,test_mac,&key)==JC_STREAM_REPLAY);
    ++e.sequence;assert(jc_chirp_packet_encode(build,2,&e,test_mac,&key,packet));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),102,test_mac,&key)==JC_STREAM_HELD);
    assert(jc_chirp_inbox_take(&inbox,&stream,102,&out) && out.sequence==1 && out.position[2]==3 && out.sound==3);
    assert(!jc_chirp_inbox_take(&inbox,&stream,103,&out));
    assert(!jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),104,test_mac,&key));
    assert(!jc_chirp_inbox_take(&inbox,&stream,605,&out));
    assert(inbox.sequence==2 && stream.last_seen==100);
    ++e.sequence;assert(jc_chirp_packet_encode(build,3,&e,test_mac,&key,packet));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),606,test_mac,&key)==JC_STREAM_CHAPTER);
    assert(jc_chirp_packet_encode(build,2,&e,test_mac,&key,packet));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),2100,test_mac,&key)==JC_STREAM_TIMEOUT);
    assert(!jc_actor_stream_open(&stream,42,2,&sample,2200));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),2201,test_mac,&key)==JC_STREAM_IDENTITY);
    inbox=(struct jc_chirp_inbox){0};e.epoch=2;e.sequence=1;
    assert(jc_chirp_packet_encode(build,2,&e,test_mac,&key,packet));
    assert(!jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),2201,test_mac,&key));
    assert(!jc_actor_stream_depart(&stream,42,2));
    assert(!jc_chirp_inbox_take(&inbox,&stream,2202,&out));
    assert(jc_chirp_packet_receive(&inbox,&stream,build,packet,sizeof(packet),2202,test_mac,&key)==JC_STREAM_IDENTITY);
    puts("Chirp packets: corruption/truncation, duplicate delivery, bounded inbox, pending expiry, chapter, silence, departure and reconnect rejection passed.");
}
int main(void) {
    chirps();
    unsigned char build[16]={17},packet[JC_ACTOR_PACKET_BYTES],bad[JC_ACTOR_PACKET_BYTES+1];
    uint64_t key=12345;struct jc_actor_sender a={.body_scale=.1f,.chirp_shape={1,1},.chapter=2};
    for(unsigned i=0;i<4;++i){a.axes[i]=(float)i*.1f;a.direction[i]=(float)i*.2f;}
    a.scalars[0]=.4f;a.scalars[1]=.7f;a.glow_targets[2]=.9f;
    struct jc_actor_stream stream={0};assert(!jc_actor_stream_open(&stream,42,1,&a,0));
    assert(jc_actor_packet_encode(build,42,1,1,&a,test_mac,&key,packet));
    assert(!memcmp(packet,"JNA1",4) && sizeof(packet)==256);
    assert(!jc_actor_packet_receive(&stream,build,packet,sizeof(packet),50,test_mac,&key));
    assert(!memcmp(&stream.sample,&a,sizeof(a)));
    for(size_t n=0;n<sizeof(packet);++n)assert(jc_actor_packet_receive(&stream,build,packet,n,100,test_mac,&key));
    memcpy(bad,packet,sizeof(packet));bad[sizeof(packet)]=0;
    assert(jc_actor_packet_receive(&stream,build,bad,sizeof(bad),100,test_mac,&key));
    for(unsigned i=0;i<sizeof(packet);++i) {
        memcpy(bad,packet,sizeof(packet));bad[i]^=1;
        struct jc_actor_stream before=stream;
        assert(jc_actor_packet_receive(&stream,build,bad,sizeof(packet),100,test_mac,&key));
        assert(!memcmp(&before,&stream,sizeof(stream)));
    }
    assert(jc_actor_packet_receive(&stream,build,packet,sizeof(packet),100,test_mac,&key)==JC_STREAM_REPLAY);
    assert(jc_actor_packet_encode(build,42,2,2,&a,test_mac,&key,packet));
    assert(jc_actor_packet_receive(&stream,build,packet,sizeof(packet),100,test_mac,&key)==JC_STREAM_IDENTITY);
    assert(stream.last_seen==50);
    assert(jc_actor_packet_encode(build,42,1,2,&a,test_mac,&key,packet));
    uint64_t wrong_key=9;
    assert(jc_actor_packet_receive(&stream,build,packet,sizeof(packet),100,test_mac,&wrong_key));
    unsigned char other_build[16]={18};
    assert(jc_actor_packet_receive(&stream,other_build,packet,sizeof(packet),100,test_mac,&key));
    jc_packet_put(packet+200,0x7fc00000,4);
    assert(test_mac(&key,packet,JC_ACTOR_PACKET_BODY,packet+JC_ACTOR_PACKET_BODY));
    assert(jc_actor_packet_receive(&stream,build,packet,sizeof(packet),100,test_mac,&key)==JC_STREAM_ARGUMENT);
    assert(stream.last_seen==50 && !memcmp(&stream.sample,&a,sizeof(a)));

    assert(!jc_actor_stream_open(&stream,42,2,&a,200));
    struct jc_actor_sender initial;memset(&initial,0x5a,sizeof(initial));
    struct jc_actor_sender untouched=initial;
    assert(jc_actor_stream_initial(&stream,&initial)==JC_STREAM_HELD);
    assert(!memcmp(&initial,&untouched,sizeof(initial)));
    jc_receiver_store(a.state.bytes,17.f);jc_receiver_store(a.state.bytes+0x20,.8f);
    a.axes[1]=.7f;a.direction[0]=.4f;

    int sockets[3];struct sockaddr_in address[3];
    for(unsigned i=0;i<3;++i) {
        sockets[i]=socket(AF_INET,SOCK_DGRAM,0);assert(sockets[i]>=0);
        address[i]=(struct sockaddr_in){.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
        assert(!bind(sockets[i],(struct sockaddr *)&address[i],sizeof(address[i])));
        socklen_t size=sizeof(address[i]);assert(!getsockname(sockets[i],(struct sockaddr *)&address[i],&size));
        struct timeval timeout={1,0};assert(!setsockopt(sockets[i],SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)));
    }
    struct jc_actor_stream second={0};assert(!jc_actor_stream_open(&second,43,2,&a,200));
    for(unsigned i=0;i<2;++i) {
        assert(jc_actor_packet_encode(build,42+i,2,2,&a,test_mac,&key,packet));
        assert(sendto(sockets[i+1],packet,sizeof(packet),0,(struct sockaddr *)&address[0],sizeof(address[0]))==sizeof(packet));
        assert(recv(sockets[0],bad,sizeof(bad),0)==sizeof(packet));
        assert(!jc_actor_packet_receive(i?&second:&stream,build,bad,sizeof(packet),250,test_mac,&key));
    }
    for(unsigned i=0;i<2;++i) {
        struct jc_chirp_event e={.participant=42+i,.epoch=2,.sequence=1,.sound=2,.radius=3,.duration=.5f},out;
        struct jc_chirp_inbox inbox={0};unsigned char event_packet[JC_CHIRP_PACKET_BYTES];
        assert(jc_chirp_packet_encode(build,2,&e,test_mac,&key,event_packet));
        for(unsigned copy=0;copy<2;++copy) {
            assert(sendto(sockets[i+1],event_packet,sizeof(event_packet),0,(struct sockaddr *)&address[0],sizeof(address[0]))==sizeof(event_packet));
            assert(recv(sockets[0],bad,sizeof(bad),0)==sizeof(event_packet));
            assert(jc_chirp_packet_receive(&inbox,i?&second:&stream,build,bad,sizeof(event_packet),251,test_mac,&key)==(copy?JC_STREAM_REPLAY:0));
        }
        assert(jc_chirp_inbox_take(&inbox,i?&second:&stream,252,&out) && out.participant==42+i);
        assert(inbox.accepted==1 && inbox.rejected==1);
    }
    for(unsigned i=0;i<3;++i)close(sockets[i]);
    assert(stream.accepted==1 && second.accepted==1);
    assert(!jc_actor_stream_initial(&stream,&initial));
    assert(!memcmp(&initial,&a,sizeof(a)));
    assert(jc_receiver_float(initial.state.bytes)==17.f && initial.axes[1]==.7f);
    puts("First authenticated packet: no initial publication while empty; sender position, heading and movement survive UDP admission unchanged.");
    assert(jc_actor_stream_poll(&stream,2250,2000)==JC_STREAM_TIMEOUT);
    assert(!jc_actor_stream_open(&stream,42,3,&a,2300));
    assert(jc_actor_packet_encode(build,42,2,99,&a,test_mac,&key,packet));
    assert(jc_actor_packet_receive(&stream,build,packet,sizeof(packet),2301,test_mac,&key)==JC_STREAM_IDENTITY);
    assert(stream.last_seen==2300);
    puts("Actor packet: full state roundtrip, every-byte corruption and truncation, replay, stale generation, timeout/reconnect and two UDP senders passed. Fixture MAC only; production BCrypt and Journey require runtime validation.");
}
