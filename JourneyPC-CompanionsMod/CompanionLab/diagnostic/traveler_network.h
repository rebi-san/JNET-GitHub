#ifndef JC_TRAVELER_NETWORK_H
#define JC_TRAVELER_NETWORK_H
#include "companion_connection.h"
static int traveler_network_config_valid(const unsigned char *config,size_t bytes) {
    static const unsigned char legacy_config_magic[4]={'J','C','N','1'};
    if(!config || bytes!=112 || (memcmp(config,"JNN1",4) && memcmp(config,legacy_config_magic,4)))return 0;
    uint64_t id=jc_packet_get(config+8,8);unsigned port=(unsigned)jc_packet_get(config+4,4);
    if(id<1 || id>3 || port!=43840+id)return 0;
    for(unsigned i=0;i<2;++i) {
        const unsigned char *p=config+16+i*48;uint64_t peer=jc_packet_get(p,8);unsigned any=0;
        for(unsigned n=16;n<48;++n)any|=p[n];
        if(peer<1 || peer>3 || peer==id || jc_packet_get(p+8,8)!=43840+peer || !any ||
            (i && peer==jc_packet_get(config+16,8)))return 0;
    }
    return 1;
}
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
static void traveler_network_slot_close(unsigned slot) {
    traveler_stream_waiting[slot]=0;
    traveler_streams[slot].stream.departing=1;
    traveler_udp[slot].chirps.pending=0;
}
static void traveler_network_close(void) {
    if(traveler_network.socket!=INVALID_SOCKET)closesocket(traveler_network.socket);
    traveler_network.socket=INVALID_SOCKET;
    for(unsigned i=0;i<2;++i) {
        struct jc_stream_udp *a=&traveler_network.peers[i].authentication;
        if(a->algorithm)BCryptCloseAlgorithmProvider(a->algorithm,0);
        SecureZeroMemory(a,sizeof(*a));
        if(traveler_udp[i].algorithm)BCryptCloseAlgorithmProvider(traveler_udp[i].algorithm,0);
        SecureZeroMemory(&traveler_udp[i],sizeof(traveler_udp[i]));
    }
    if(traveler_network.ws)WSACleanup();
    traveler_network.ws=0;traveler_network_active=0;
}
static int traveler_network_start(void) {
    if(traveler_network.configured) {
        traveler_network.paused=0;traveler_network.next_send=0;JNETNetwork[0]=1;return 0;
    }
    unsigned char config[112]={0};DWORD count=0;wchar_t path[MAX_PATH];HMODULE module=NULL;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)(uintptr_t)traveler_network_start,&module))return 120;
    DWORD length=GetModuleFileNameW(module,path,MAX_PATH);
    if(!length || length>=MAX_PATH)return 120;
    wchar_t *tail=wcsrchr(path,L'\\');if(!tail || tail-path+18>=MAX_PATH)return 120;
    wcscpy(tail+1,L"session-peers.bin");
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 120;
    int ok=GetFileSize(file,NULL)==sizeof(config) && ReadFile(file,config,sizeof(config),&count,NULL) && count==sizeof(config);
    CloseHandle(file);
    if(!ok || !traveler_network_config_valid(config,sizeof(config))) {SecureZeroMemory(config,sizeof(config));return 120;}
    uint64_t id=jc_packet_get(config+8,8);unsigned port=(unsigned)jc_packet_get(config+4,4);
    WSADATA ws;if(WSAStartup(MAKEWORD(2,2),&ws)){SecureZeroMemory(config,sizeof(config));return 121;}
    traveler_network.ws=1;traveler_network.identity=id;
    FILETIME ft;GetSystemTimeAsFileTime(&ft);memcpy(&traveler_network.epoch,&ft,8);
    for(unsigned i=0;i<2;++i) {
        struct jc_network_peer *p=&traveler_network.peers[i];const unsigned char *record=config+16+i*48;
        p->connection.local=id;p->connection.peer=jc_packet_get(record,8);p->connection.epoch=traveler_network.epoch;
        p->port=(uint16_t)jc_packet_get(record+8,8);memcpy(p->authentication.key,record+16,32);
        if(BCryptGenRandom(NULL,p->connection.nonce,16,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0 ||
            BCryptOpenAlgorithmProvider(&p->authentication.algorithm,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0 ||
            BCryptOpenAlgorithmProvider(&traveler_udp[i].algorithm,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)goto fail;
        traveler_udp[i].ready=1;
    }
    traveler_network.socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(traveler_network.socket==INVALID_SOCKET)goto fail;
    BOOL exclusive=TRUE;u_long nonblocking=1;
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons((u_short)port);
    if(setsockopt(traveler_network.socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char *)&exclusive,sizeof(exclusive)) ||
        ioctlsocket(traveler_network.socket,FIONBIO,&nonblocking) ||
        bind(traveler_network.socket,(const struct sockaddr *)&address,sizeof(address)))goto fail;
    SecureZeroMemory(config,sizeof(config));traveler_network.configured=traveler_network_active=1;
    traveler_stream_enabled=1;JNETNetwork[0]=1;JNETNetwork[1]=(LONG)id;return 0;
fail:
    SecureZeroMemory(config,sizeof(config));traveler_network_close();return 121;
}
static void traveler_network_send(unsigned slot,const unsigned char *data,unsigned bytes) {
#ifdef JNET_LOBBY_ENABLED
    if(bytes>=4 && memcmp(data,"JNH1",4) && !jnet_lobby_allows(traveler_network.peers[slot].connection.peer))return;
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
        if(bytes<4 || (memcmp(packet,"JNH1",4) && !jnet_lobby_allows(traveler_network.peers[slot].connection.peer)))continue;
#endif
        struct jc_network_peer *p=&traveler_network.peers[slot];int code=JC_STREAM_ARGUMENT;
        if(bytes>=4 && !memcmp(packet,"JNH1",4)) {
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
        } else if(bytes>=4 && !memcmp(packet,"JNA1",4)) {
            code=jc_connection_state(&p->connection,jc_game_hash,packet,(size_t)bytes,now,
                traveler_udp_mac,&traveler_udp[slot],&p->sample);
        } else if(bytes>=4 && !memcmp(packet,"JNC1",4) && p->connection.admitted && p->connection.sequence) {
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
    if(traveler_network_external && !JNETNetwork[23] && jnet_travel_peer_wait &&
        jnet_travel_peer_wait(p->connection.peer,now) &&
        (traveler_network.paused || !p->connection.admitted || !p->connection.sequence ||
         now<p->connection.last_seen || now-p->connection.last_seen>=2000 ||
         p->sample.chapter!=traveler_network.chapter))return JC_STREAM_HELD;
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
