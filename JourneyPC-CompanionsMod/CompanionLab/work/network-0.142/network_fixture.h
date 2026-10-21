#ifndef JC_TRAVELER_NETWORK_H
#define JC_TRAVELER_NETWORK_H
#include "companion_connection.h"
struct jc_network_peer {
    struct jc_connection connection;
    struct jc_stream_udp authentication;
    struct jc_actor_sender sample;
    struct jc_chirp_inbox wire_chirps;
    uint16_t port;
    uint64_t consumed,render_epoch,render_generation;
};
static struct {
    SOCKET socket;
    unsigned ws,configured,paused;
    uint64_t identity,epoch,sequence,next_send,next_hello,timed_frames,total_us;
    uint32_t chapter;
    struct jc_chirp_capture chirps;
    struct jc_network_peer peers[2];
} traveler_network={.socket=INVALID_SOCKET};
__declspec(dllexport) volatile LONG JNETNetwork[24];
static void traveler_network_send(unsigned slot,const unsigned char *data,unsigned bytes) {
#ifdef JNET_LOBBY_ENABLED
    if(bytes>=4 && memcmp(data,"JCH1",4) && !jnet_lobby_allows(traveler_network.peers[slot].connection.peer))return;
#endif
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    address.sin_port=htons(traveler_network.peers[slot].port);
    int result=sendto(traveler_network.socket,(const char *)data,(int)bytes,0,(const struct sockaddr *)&address,sizeof(address));
    if(result==(int)bytes)++JNETNetwork[3];
    else if(WSAGetLastError()!=WSAEWOULDBLOCK)JNETNetwork[2]=122;
}
static uint64_t traveler_network_clock_us(void) {
    static LARGE_INTEGER frequency;
    LARGE_INTEGER value;
    if(!frequency.QuadPart && !QueryPerformanceFrequency(&frequency))return GetTickCount64()*1000;
    if(!QueryPerformanceCounter(&value) || frequency.QuadPart<=0)return GetTickCount64()*1000;
    return (uint64_t)(value.QuadPart/frequency.QuadPart)*1000000+
        (uint64_t)(value.QuadPart%frequency.QuadPart)*1000000/(uint64_t)frequency.QuadPart;
}
static void traveler_network_chirp_take(unsigned slot,uint64_t now) {
    struct jc_network_peer *p=&traveler_network.peers[slot];
    struct jc_actor_stream *render=&traveler_streams[slot].stream;
    if(!p->wire_chirps.pending)return;
    if(now<p->wire_chirps.received_at || now-p->wire_chirps.received_at>500) {
        p->wire_chirps.pending=0;++JNETNetwork[22];return;
    }
    if(render->departing || !render->accepted || p->render_epoch!=p->connection.remote_epoch ||
        render->sample.chapter!=p->sample.chapter || traveler_udp[slot].chirps.pending)return;
    struct jc_actor_stream wire={.peer=p->connection.peer,.generation=p->connection.remote_epoch,
        .last_seen=p->connection.last_seen,.clock=p->connection.last_seen,.sample=p->sample};
    struct jc_chirp_event event;
    if(!jc_chirp_inbox_take(&p->wire_chirps,&wire,now,&event))return;
    event.epoch=render->generation;traveler_udp[slot].chirps.event=event;
    traveler_udp[slot].chirps.received_at=p->wire_chirps.received_at;traveler_udp[slot].chirps.pending=1;
    ++traveler_udp[slot].chirps.accepted;++JNETNetwork[14];
}
static unsigned traveler_network_external;
static void traveler_network_tick(uintptr_t game,uint64_t frame) {
    if(traveler_network_external)return;
    if(!traveler_network_active)return;
    uint64_t started_us=traveler_network_clock_us(),now=GetTickCount64();
    for(unsigned slot=0;slot<2;++slot)traveler_network_chirp_take(slot,now);
    for(unsigned n=0;n<32;++n) {
        unsigned char packet[JC_ACTOR_PACKET_BYTES+1];struct sockaddr_in address;int size=sizeof(address);
        int bytes=recvfrom(traveler_network.socket,(char *)packet,sizeof(packet),0,(struct sockaddr *)&address,&size);
        if(bytes==SOCKET_ERROR) {int error=WSAGetLastError();if(error==WSAEMSGSIZE){++JNETNetwork[5];continue;}
            if(error!=WSAEWOULDBLOCK && error!=WSAECONNRESET)JNETNetwork[2]=122;break;}
        ++JNETNetwork[4];if(n==31)++JNETNetwork[20];unsigned slot;
        for(slot=0;slot<2;++slot)if(address.sin_family==AF_INET && address.sin_addr.s_addr==htonl(INADDR_LOOPBACK) &&
            ntohs(address.sin_port)==traveler_network.peers[slot].port)break;
        if(slot==2){++JNETNetwork[5];continue;}
#ifdef JNET_LOBBY_ENABLED
        if(bytes<4 || (memcmp(packet,"JCH1",4) && !jnet_lobby_allows(traveler_network.peers[slot].connection.peer)))continue;
#endif
        struct jc_network_peer *p=&traveler_network.peers[slot];int code=JC_STREAM_ARGUMENT;
        if(bytes>=4 && !memcmp(packet,"JCH1",4)) {
            uint64_t previous=p->connection.remote_epoch,candidate=p->connection.candidate_epoch;
            code=jc_connection_receive(&p->connection,jc_game_hash,packet,(size_t)bytes,traveler_udp_mac,&p->authentication);
            if(!code && previous!=p->connection.remote_epoch) {
                if(!jc_connection_key(&p->connection,traveler_udp_mac,&p->authentication,traveler_udp[slot].key))code=JC_STREAM_IDENTITY;
                memset(&p->wire_chirps,0,sizeof(p->wire_chirps));traveler_network.next_send=0;
            }
            if((!code || code==JC_STREAM_HELD) &&
                (candidate!=p->connection.candidate_epoch || previous!=p->connection.remote_epoch)) {
                unsigned char reply[JC_CONNECTION_HELLO_BYTES];
                if(jc_connection_hello(&p->connection,jc_game_hash,traveler_udp_mac,&p->authentication,reply))
                    traveler_network_send(slot,reply,sizeof(reply));
            }
        } else if(bytes>=4 && !memcmp(packet,"JCA1",4)) {
            code=jc_connection_state(&p->connection,jc_game_hash,packet,(size_t)bytes,now,
                traveler_udp_mac,&traveler_udp[slot],&p->sample);
        } else if(bytes>=4 && !memcmp(packet,"JCC1",4) && p->connection.admitted && p->connection.sequence) {
            struct jc_actor_stream wire={.peer=p->connection.peer,.generation=p->connection.remote_epoch,
                .last_seen=p->connection.last_seen,.clock=p->connection.last_seen,.sample=p->sample};
            code=jc_chirp_packet_receive(&p->wire_chirps,&wire,jc_game_hash,packet,(size_t)bytes,now,traveler_udp_mac,&traveler_udp[slot]);
            if(!code)traveler_network_chirp_take(slot,now);
        }
        if(code==JC_STREAM_REPLAY)++JNETNetwork[21];
        if(code && code!=JC_STREAM_HELD)++JNETNetwork[5];
    }
    if(now>=traveler_network.next_hello) {
        for(unsigned slot=0;slot<2;++slot) {
            unsigned char hello[JC_CONNECTION_HELLO_BYTES];struct jc_network_peer *p=&traveler_network.peers[slot];
            if(jc_connection_hello(&p->connection,jc_game_hash,traveler_udp_mac,&p->authentication,hello))traveler_network_send(slot,hello,sizeof(hello));
        }
        traveler_network.next_hello=now+500;
    }
    if(!traveler_network.paused && now>=traveler_network.next_send) {
        struct jc_actor_sender sample;int code=jc_actor_capture_sender(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),game,&sample);
        JNETNetwork[6]=code;traveler_network.next_send=now+33;
        if(!code && traveler_network.sequence!=UINT64_MAX) {
            traveler_network.chapter=sample.chapter;++traveler_network.sequence;
            for(unsigned slot=0;slot<2;++slot)if(traveler_network.peers[slot].connection.admitted) {
                unsigned char packet[JC_ACTOR_PACKET_BYTES];
                if(jc_actor_packet_encode(jc_game_hash,traveler_network.identity,traveler_network.epoch,
                    traveler_network.sequence,&sample,traveler_udp_mac,&traveler_udp[slot],packet))traveler_network_send(slot,packet,sizeof(packet));
            }
        }
    }
    struct jc_chirp_batch batch;
    int chirp=jc_chirp_capture_local(own_read,NULL,game,frame,traveler_network.identity,traveler_network.epoch,&traveler_network.chirps,&batch);
    JNETNetwork[15]=chirp;
    if(!chirp && !traveler_network.paused)for(unsigned n=0;n<batch.count;++n)for(unsigned slot=0;slot<2;++slot)if(traveler_network.peers[slot].connection.admitted) {
        unsigned char packet[JC_CHIRP_PACKET_BYTES];
        if(jc_chirp_packet_encode(jc_game_hash,traveler_network.chapter,&batch.source[n].event,traveler_udp_mac,&traveler_udp[slot],packet)) {
            traveler_network_send(slot,packet,sizeof(packet));traveler_network_send(slot,packet,sizeof(packet));
        }
    }
    for(unsigned slot=0;slot<2;++slot) {
        traveler_network_chirp_take(slot,now);
        struct jc_network_peer *p=&traveler_network.peers[slot];unsigned row=8+slot*3;
        JNETNetwork[row]=(LONG)p->connection.peer;
        JNETNetwork[row+1]=p->connection.admitted;
        JNETNetwork[row+2]=p->connection.sequence?(LONG)(now-p->connection.last_seen>60000?60000:now-p->connection.last_seen):-1;
    }
    uint64_t finished_us=traveler_network_clock_us();
    uint64_t elapsed_us=finished_us>=started_us?finished_us-started_us:0;
    if(elapsed_us>INT32_MAX)elapsed_us=INT32_MAX;
    JNETNetwork[16]=(LONG)elapsed_us;
    if(JNETNetwork[16]>JNETNetwork[17])JNETNetwork[17]=JNETNetwork[16];
    if(traveler_network.timed_frames==1000000){traveler_network.timed_frames=0;traveler_network.total_us=0;}
    ++traveler_network.timed_frames;traveler_network.total_us+=elapsed_us;
    JNETNetwork[18]=(LONG)(traveler_network.total_us/traveler_network.timed_frames);JNETNetwork[19]=33;
}
static int traveler_network_sample(unsigned slot,int first,struct jc_actor_sender *sender) {
    struct jc_network_peer *p=&traveler_network.peers[slot];struct jc_actor_stream *s=&traveler_streams[slot].stream;
    uint64_t now=GetTickCount64();
#ifdef JNET_LOBBY_ENABLED
    if(!jnet_lobby_allows(p->connection.peer))return first?JC_STREAM_HELD:JC_STREAM_TIMEOUT;
#endif
    if(traveler_network.paused || !p->connection.sequence || now<p->connection.last_seen ||
        now-p->connection.last_seen>=2000 || p->sample.chapter!=traveler_network.chapter)
        return first?JC_STREAM_HELD:JC_STREAM_TIMEOUT;
    if(first) {
        if(p->render_generation==UINT64_MAX)return JC_STREAM_ARGUMENT;
        ++p->render_generation;memset(s,0,sizeof(*s));
        int code=jc_actor_stream_open(s,p->connection.peer,p->render_generation,&p->sample,now);if(code)return code;
        p->render_epoch=p->connection.remote_epoch;p->consumed=0;
        memset(&traveler_udp[slot].chirps,0,sizeof(traveler_udp[slot].chirps));
    } else if(s->peer!=p->connection.peer || p->render_epoch!=p->connection.remote_epoch)return JC_STREAM_TIMEOUT;
    if(p->consumed==p->connection.sequence)return JC_STREAM_HELD;
    int code=jc_actor_stream_receive(s,p->connection.peer,p->render_generation,p->connection.sequence,&p->sample,now);
    if(code)return code;p->consumed=p->connection.sequence;*sender=p->sample;
    traveler_network_chirp_take(slot,now);return 0;
}
#endif
