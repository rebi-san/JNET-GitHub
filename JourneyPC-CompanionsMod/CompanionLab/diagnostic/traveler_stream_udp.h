#ifndef JC_TRAVELER_STREAM_UDP_H
#define JC_TRAVELER_STREAM_UDP_H
#include "companion_chirp_packet.h"
#include <bcrypt.h>
struct jc_stream_udp {
    SOCKET sockets[2];
    unsigned ready,ws;
    BCRYPT_ALG_HANDLE algorithm;
    unsigned char key[32];
    unsigned sent,received,rejected;
    struct jc_chirp_inbox chirps;
};
static struct jc_stream_udp traveler_udp[2];
static void traveler_stream_close(unsigned slot) {
    #ifdef JC_NETWORK_UDP
    if(traveler_network_active){traveler_network_slot_close(slot);return;}
    #endif
    struct jc_stream_udp *u=&traveler_udp[slot];
    traveler_stream_waiting[slot]=0;
    if(u->ws)for(unsigned i=0;i<2;++i)if(u->sockets[i]!=INVALID_SOCKET)closesocket(u->sockets[i]);
    if(u->algorithm)BCryptCloseAlgorithmProvider(u->algorithm,0);
    if(u->ws)WSACleanup();
    u->chirps.pending=0;
    u->ready=u->ws=0;u->algorithm=NULL;SecureZeroMemory(u->key,sizeof(u->key));
}
static int traveler_udp_mac(void *ctx,const unsigned char *data,size_t bytes,unsigned char tag[32]) {
    struct jc_stream_udp *u=ctx;BCRYPT_HASH_HANDLE hash=NULL;
    int ok=BCryptCreateHash(u->algorithm,&hash,NULL,0,u->key,32,0)>=0;
    if(ok)ok=BCryptHashData(hash,(PUCHAR)data,(ULONG)bytes,0)>=0 && BCryptFinishHash(hash,tag,32,0)>=0;
    if(hash)BCryptDestroyHash(hash);return ok;
}
static int traveler_udp_open(unsigned slot) {
    traveler_stream_close(slot);struct jc_stream_udp *u=&traveler_udp[slot];
    memset(u,0,sizeof(*u));u->sockets[0]=u->sockets[1]=INVALID_SOCKET;
    WSADATA ws;if(WSAStartup(MAKEWORD(2,2),&ws))return 110;u->ws=1;
    if(BCryptGenRandom(NULL,u->key,32,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0 ||
        BCryptOpenAlgorithmProvider(&u->algorithm,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)goto fail;
    struct sockaddr_in address[2];memset(address,0,sizeof(address));
    for(unsigned i=0;i<2;++i) {
        u->sockets[i]=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(u->sockets[i]==INVALID_SOCKET)goto fail;
        BOOL exclusive=TRUE;u_long nonblocking=1;
        if(setsockopt(u->sockets[i],SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char *)&exclusive,sizeof(exclusive)) ||
            ioctlsocket(u->sockets[i],FIONBIO,&nonblocking))goto fail;
        address[i].sin_family=AF_INET;address[i].sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if(bind(u->sockets[i],(const struct sockaddr *)&address[i],sizeof(address[i])))goto fail;
        int bytes=sizeof(address[i]);if(getsockname(u->sockets[i],(struct sockaddr *)&address[i],&bytes))goto fail;
    }
    for(unsigned i=0;i<2;++i)
        if(connect(u->sockets[i],(const struct sockaddr *)&address[1-i],sizeof(address[i])))goto fail;
    u->ready=1;return 0;
fail:
    traveler_stream_close(slot);return 110;
}
static int traveler_udp_send(void *ctx,struct jc_actor_stream *stream,uint64_t sequence,
        const struct jc_actor_sender *sample,uint64_t now) {
    (void)now;struct jc_stream_udp *u=ctx;unsigned char packet[JC_ACTOR_PACKET_BYTES];
    if(!u->ready || !jc_actor_packet_encode(jc_game_hash,stream->peer,stream->generation,sequence,
        sample,traveler_udp_mac,u,packet))return 111;
    int bytes=send(u->sockets[0],(const char *)packet,sizeof(packet),0);
    if(bytes==SOCKET_ERROR && WSAGetLastError()==WSAEWOULDBLOCK)return 0;
    if(bytes!=sizeof(packet))return 112;
    ++u->sent;return 0;
}
static int traveler_udp_chirp_send(unsigned slot,uint32_t chapter,const struct jc_chirp_event *event) {
    struct jc_stream_udp *u=&traveler_udp[slot];unsigned char packet[JC_CHIRP_PACKET_BYTES];
    if(!u->ready || !jc_chirp_packet_encode(jc_game_hash,chapter,event,traveler_udp_mac,u,packet))return 111;
    for(unsigned repeat=0;repeat<2;++repeat) {
        int bytes=send(u->sockets[0],(const char *)packet,sizeof(packet),0);
        if(bytes==SOCKET_ERROR && WSAGetLastError()==WSAEWOULDBLOCK)continue;
        if(bytes!=sizeof(packet))return 112;
        ++u->sent;
    }
    return 0;
}
static int traveler_udp_poll(unsigned slot,struct jc_actor_stream *stream,uint64_t now) {
    struct jc_stream_udp *u=&traveler_udp[slot];if(!u->ready)return 110;
    for(unsigned i=0;i<16;++i) {
        unsigned char packet[JC_ACTOR_PACKET_BYTES+1];
        int bytes=recv(u->sockets[1],(char *)packet,sizeof(packet),0);
        if(bytes==SOCKET_ERROR) {
            int error=WSAGetLastError();if(error==WSAEWOULDBLOCK)return 0;
            if(error==WSAEMSGSIZE){++u->rejected;continue;}return 112;
        }
        ++u->received;
        int code=bytes>=4 && !memcmp(packet,"JNC1",4)?
            jc_chirp_packet_receive(&u->chirps,stream,jc_game_hash,packet,(size_t)bytes,now,traveler_udp_mac,u):
            jc_actor_packet_receive(stream,jc_game_hash,packet,(size_t)bytes,now,traveler_udp_mac,u);
        if(code)++u->rejected;
    }
    return 0;
}
#endif
