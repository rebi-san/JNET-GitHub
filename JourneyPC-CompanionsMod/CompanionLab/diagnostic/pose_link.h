#ifndef JC_POSE_LINK_H
#define JC_POSE_LINK_H
#include "companion_room.h"
#include "native_animation_playback.h"
/* Windows sockets must be included before platform.h by the caller. All addresses are loopback. */
struct jc_pose_link {
    SOCKET socket;
    BCRYPT_ALG_HANDLE algorithm;
    unsigned char key[32],identity[16];
    struct jc_pose_receiver receiver;
    struct jc_pose_room room;
    uint64_t sequence,deadline,last_send;
    unsigned mode,started,ws;
    DWORD thread; uintptr_t game;
    int error;
    struct jc_anim_layout animation;
    struct jc_pose_sample published;
};
static struct jc_pose_link pose_link={.socket=INVALID_SOCKET};
struct jc_capture_budget { uint64_t deadline; int expired; };
static int pose_capture_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct jc_capture_budget *budget=opaque;
    if (GetTickCount64()>=budget->deadline) { budget->expired=1; return 0; }
    return own_read(NULL,address,out,size);
}
__declspec(dllexport) volatile LONG JNETLink[16]={[7]=-1,[8]=-1,[11]=-1,[13]=-1,[14]=-1};
__declspec(dllexport) volatile LONG JNETRoom[20]={0};
__declspec(dllexport) volatile LONG JNETMotion[28]={[0]=-1,[4]=-1,[11]=-1};
__declspec(dllexport) unsigned char JNETLinkConfig[36]={0};
static int pose_mac(void *opaque,const unsigned char *data,size_t length,unsigned char tag[32]) {
    struct jc_pose_link *link=opaque; BCRYPT_HASH_HANDLE hash=NULL;
    int ok=BCryptCreateHash(link->algorithm,&hash,NULL,0,link->key,32,0)>=0;
    if (ok) ok=BCryptHashData(hash,(PUCHAR)data,(ULONG)length,0)>=0 && BCryptFinishHash(hash,tag,32,0)>=0;
    if (hash) BCryptDestroyHash(hash);
    return ok;
}
static int pose_link_io_defer(int error) {
    if (!jc_room_io_defer(pose_link.mode,error)) return 0;
    if (error!=WSAEWOULDBLOCK) {
        InterlockedExchange(&JNETFlight[11],error);
        InterlockedIncrement(&JNETRoom[3]);
    }
    return 1;
}
static void pose_link_depart(void) {
    if (pose_link.mode<4 || !pose_link.published.sequence || pose_link.sequence==UINT64_MAX ||
        pose_link.socket==INVALID_SOCKET || !pose_link.algorithm) return;
    struct jc_pose_sample sample=pose_link.published;
    sample.sequence=++pose_link.sequence; sample.flags|=JC_POSE_LEAVE;
    memset(&sample.animation,0,sizeof(sample.animation));
    unsigned char packet[JC_POSE_BYTES];
    if (!jc_pose_encode(jc_game_hash,&sample,pose_mac,&pose_link,packet)) return;
    /* Same authenticated sequence is repeated; receivers commit it at most once. */
    for (unsigned repeat=0;repeat<3;++repeat) {
        if (pose_link.mode==5) {
            send(pose_link.socket,(const char *)packet,sizeof(packet),0);
        } else for (unsigned i=0;i<2;++i) {
            const struct jc_pose_receiver *p=&pose_link.room.peers[i];
            if (!p->paired || p->departed) continue;
            struct sockaddr_in peer={0}; peer.sin_family=AF_INET;
            peer.sin_addr.s_addr=htonl(INADDR_LOOPBACK); peer.sin_port=htons(p->peer_port);
            sendto(pose_link.socket,(const char *)packet,sizeof(packet),0,(const struct sockaddr *)&peer,sizeof(peer));
        }
    }
}
static void pose_link_close(int error) {
    if (error==5) InterlockedExchange(&JNETFlight[11],WSAGetLastError());
    if (pose_link.socket!=INVALID_SOCKET) closesocket(pose_link.socket);
    if (pose_link.algorithm) BCryptCloseAlgorithmProvider(pose_link.algorithm,0);
    if (pose_link.ws) WSACleanup();
    pose_link.socket=INVALID_SOCKET; pose_link.algorithm=NULL; pose_link.ws=0; pose_link.mode=0;
    SecureZeroMemory(pose_link.key,sizeof(pose_link.key)); SecureZeroMemory(JNETLinkConfig,sizeof(JNETLinkConfig));
    pose_link.error=error;
    InterlockedExchange(&JNETLink[0],error ? -1 : 3);
    InterlockedExchange(&JNETRoom[5],0); InterlockedExchange(&JNETRoom[13],0);
    for (unsigned i=0;i<2;++i) if (pose_link.room.peers[i].paired)
        InterlockedExchange(&JNETRoom[11+8*i],pose_link.room.peers[i].departed ? 3 : 5);
    InterlockedExchange(&JNETLink[1],error); InterlockedExchange(&JNETLink[6],0);
}
static void pose_link_applied(unsigned peer) {
    InterlockedIncrement(&JNETLink[9]);
    if (pose_link.mode>=4 && peer<2)
        InterlockedIncrement(&JNETRoom[10+8*peer]);
}
static int pose_link_start(unsigned mode) {
    if (pose_link.started || (mode<1 || mode>5)) return 0;
    pose_link.started=1;
    unsigned char config[36]; memcpy(config,JNETLinkConfig,sizeof(config));
    unsigned port=(unsigned)jc_session_get(config,4);
    unsigned key_present=0; for (unsigned i=4;i<36;++i) key_present|=config[i];
    if ((mode!=1 && mode!=4) && (!port || port>65535 || !key_present)) { SecureZeroMemory(config,sizeof(config)); pose_link_close(1); return 0; }
    if (mode!=1 && mode!=4) memcpy(pose_link.key,config+4,32);
    SecureZeroMemory(config,sizeof(config));
    WSADATA data;
    if (WSAStartup(MAKEWORD(2,2),&data)) { pose_link_close(2); return 0; }
    pose_link.ws=1;
    if (mode==1 || mode==4) {
        if (BCryptGenRandom(NULL,pose_link.key,32,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0) { pose_link_close(3); return 0; }
    }
    if (BCryptGenRandom(NULL,pose_link.identity,16,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0 ||
        BCryptOpenAlgorithmProvider(&pose_link.algorithm,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0) { pose_link_close(3); return 0; }
    pose_link.socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (pose_link.socket==INVALID_SOCKET) { pose_link_close(4); return 0; }
    BOOL exclusive=TRUE; u_long nonblocking=1; int buffer=16384;
    if (setsockopt(pose_link.socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char *)&exclusive,sizeof(exclusive)) ||
        setsockopt(pose_link.socket,SOL_SOCKET,SO_RCVBUF,(const char *)&buffer,sizeof(buffer)) ||
        ioctlsocket(pose_link.socket,FIONBIO,&nonblocking)) { pose_link_close(4); return 0; }
    struct sockaddr_in address={0}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (bind(pose_link.socket,(const struct sockaddr *)&address,sizeof(address))) { pose_link_close(4); return 0; }
    if (mode==1 || mode==4) {
        int bytes=sizeof(address);
        if (getsockname(pose_link.socket,(struct sockaddr *)&address,&bytes)) { pose_link_close(4); return 0; }
        jc_session_put(JNETLinkConfig,ntohs(address.sin_port),4); memcpy(JNETLinkConfig+4,pose_link.key,32);
    } else {
        address.sin_port=htons((u_short)port);
        if (connect(pose_link.socket,(const struct sockaddr *)&address,sizeof(address))) { pose_link_close(4); return 0; }
        pose_link.receiver.peer_port=(uint16_t)port;
        SecureZeroMemory(JNETLinkConfig,sizeof(JNETLinkConfig));
    }
    memcpy(pose_link.receiver.own_identity,pose_link.identity,16);
    pose_link.receiver.required_flags=mode==3 ? JC_POSE_DUPLEX : 0;
    if (mode>=4) jc_room_init(&pose_link.room,mode==4,pose_link.identity,(uint16_t)port);
    pose_link.thread=GetCurrentThreadId(); pose_link.game=(uintptr_t)JNETGame;
    pose_link.mode=mode; pose_link.deadline=GetTickCount64()+600000;
    InterlockedExchange(&JNETLink[2],mode); InterlockedExchange(&JNETLink[0],1);
    return 1;
}
static int pose_link_send_to(const unsigned char packet[JC_POSE_BYTES],uint16_t port) {
    struct sockaddr_in peer={0}; peer.sin_family=AF_INET; peer.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    peer.sin_port=htons(port);
    int sent=sendto(pose_link.socket,(const char *)packet,JC_POSE_BYTES,0,(const struct sockaddr *)&peer,sizeof(peer));
    if (sent==JC_POSE_BYTES) return 1;
    if (sent==SOCKET_ERROR && pose_link_io_defer(WSAGetLastError())) return 0;
    pose_link_close(5); return -1;
}
static void pose_room_status(uint32_t chapter,uint64_t now) {
    unsigned count=0;
    for (unsigned i=0;i<2;++i) {
        const struct jc_pose_receiver *p=&pose_link.room.peers[i]; unsigned row=4+8*i;
        count+=p->paired;
        InterlockedExchange(&JNETRoom[row],p->paired);
        InterlockedExchange(&JNETRoom[row+1],jc_pose_fresh(p,chapter,now));
        InterlockedExchange(&JNETRoom[row+2],p->paired ? (LONG)(now-p->last_seen>INT32_MAX ? INT32_MAX : now-p->last_seen) : -1);
        InterlockedExchange(&JNETRoom[row+3],p->paired ? (LONG)p->sample.chapter : -1);
        InterlockedExchange(&JNETRoom[row+4],(LONG)pose_link.room.received[i]);
        InterlockedExchange(&JNETRoom[row+5],p->paired && (p->sample.flags&JC_POSE_DUPLEX));
        InterlockedExchange(&JNETRoom[row+7],jc_pose_presence(p,chapter,now));
    }
    InterlockedExchange(&JNETRoom[0],count);
    InterlockedExchange(&JNETRoom[1],pose_link.room.selected<2 ? (LONG)pose_link.room.selected+1 : 0);
    jc_room_selected(&pose_link.room,&pose_link.receiver);
    InterlockedExchange(&JNETLink[8],pose_link.receiver.paired ? (LONG)pose_link.receiver.sample.chapter : -1);
}
/* Runs on the game update thread; bounded nonblocking reads and at most ten publishes per second. */
static void pose_link_tick(uintptr_t game) {
    if (!pose_link.mode) return;
    if (GetCurrentThreadId()!=pose_link.thread || game!=pose_link.game) { pose_link_close(7); return; }
    uint64_t now=GetTickCount64();
    if (now>=pose_link.deadline) { pose_link_depart(); pose_link_close(0); return; }
    InterlockedExchange(&JNETLink[10],(LONG)((pose_link.deadline-now+999)/1000));
    int chapter=-1;
    if (jc_render_level(own_read,NULL,game,&chapter)) { pose_link.receiver.ready=0; InterlockedExchange(&JNETLink[6],0); return; }
    if (pose_link.mode!=2) {
        for (unsigned attempt=0;attempt<8;++attempt) {
            unsigned char packet[JC_POSE_BYTES+1]; struct sockaddr_in source={0}; int bytes=sizeof(source);
            int got=recvfrom(pose_link.socket,(char *)packet,sizeof(packet),0,(struct sockaddr *)&source,&bytes);
            if (got==SOCKET_ERROR) {
                int error=WSAGetLastError();
                if (error==WSAEWOULDBLOCK) break;
                if (error==WSAEMSGSIZE) { InterlockedIncrement(&JNETLink[4]); continue; }
                if (pose_link_io_defer(error)) continue;
                pose_link_close(5); return;
            }
            unsigned accepted=2;
            int code;
            if (bytes!=sizeof(source) || source.sin_family!=AF_INET || source.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)) code=3;
            else if (pose_link.mode>=4)
                code=jc_room_receive(&pose_link.room,jc_game_hash,packet,(size_t)got,(uint32_t)chapter,now,ntohs(source.sin_port),pose_mac,&pose_link,&accepted);
            else if (got==JC_POSE_BYTES && (jc_session_get(packet+52,4)&JC_POSE_ROOM)) code=3;
            else code=jc_pose_receive_from(&pose_link.receiver,jc_game_hash,packet,(size_t)got,(uint32_t)chapter,now,ntohs(source.sin_port),pose_mac,&pose_link);
            if ((!code || code==8) && pose_link.mode==4) {
                uint16_t target=jc_room_relay_port(&pose_link.room,accepted);
                if (target) {
                    unsigned repeats=pose_link.room.peers[accepted].departed ? 3 : 1;
                    for (unsigned repeat=0;repeat<repeats;++repeat) {
                        int sent=pose_link_send_to(packet,target);
                        if (sent<0) return;
                        if (sent) InterlockedIncrement(&JNETRoom[2]);
                    }
                }
            }
            if (!code || code==8) {
                InterlockedIncrement(&JNETLink[3]); InterlockedExchange(&JNETLink[8],(LONG)pose_link.receiver.sample.chapter);
                InterlockedExchange(&JNETLink[15],(pose_link.receiver.sample.flags&JC_POSE_DUPLEX)!=0);
            } else InterlockedIncrement(&JNETLink[4]);
            InterlockedExchange(&JNETLink[5],code);
            if (!code && pose_link.mode==5 && accepted==0 && pose_link.room.peers[0].departed) {
                pose_room_status((uint32_t)chapter,now); pose_link_close(0); return;
            }
        }
        if (pose_link.mode>=4) pose_room_status((uint32_t)chapter,now);
        int fresh=jc_pose_fresh(&pose_link.receiver,(uint32_t)chapter,now);
        InterlockedExchange(&JNETLink[6],fresh);
        InterlockedExchange(&JNETLink[7],pose_link.receiver.paired ? (LONG)((now-pose_link.receiver.last_seen)>INT32_MAX ? INT32_MAX : now-pose_link.receiver.last_seen) : -1);
    }
    if (pose_link.mode==1 && (!pose_link.receiver.paired || !(pose_link.receiver.sample.flags&JC_POSE_DUPLEX))) return;
    if (now-pose_link.last_send<100) return;
    pose_link.last_send=now;
    uint64_t barn,type,again; float check[16]; struct jc_pose_sample sample={0};
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    if (!jc_render_trial_level(chapter) || !own_read(NULL,game+0x1e0,&barn,8) ||
        !own_read(NULL,barn,&type,8) || type!=image+0x5da340 ||
        !own_read(NULL,barn+0xb0,&type,8) || type!=image+0x5d0380 ||
        !own_read(NULL,barn+0x210,&type,8) || type!=image+0x67f3f8 ||
        !own_read(NULL,barn+0x740,sample.transform,64) || !own_read(NULL,barn+0x740,check,64) ||
        memcmp(sample.transform,check,64) || !own_read(NULL,game+0x1e0,&again,8) || again!=barn) {
        InterlockedExchange(&JNETLink[13],7); if (pose_link.mode==2) InterlockedExchange(&JNETLink[5],7); return;
    }
    int chapter_again=-1;
    if (jc_render_level(own_read,NULL,game,&chapter_again) || chapter_again!=chapter) {
        InterlockedExchange(&JNETLink[13],7); if (pose_link.mode==2) InterlockedExchange(&JNETLink[5],7); return;
    }
    ULONGLONG capture_started=GetTickCount64();
    struct jc_capture_budget capture_budget={.deadline=capture_started+100};
    int animation_code=jc_anim_capture(pose_capture_read,&capture_budget,image,barn+0x210,&pose_link.animation,&sample.animation);
    if (capture_budget.expired) animation_code=9;
    if (animation_code) memset(&sample.animation,0,sizeof(sample.animation));
    InterlockedExchange(&JNETMotion[0],animation_code);
    InterlockedExchange(&JNETMotion[1],(LONG)sample.animation.count);
    InterlockedExchange(&JNETMotion[2],(LONG)sample.animation.total);
    InterlockedExchange(&JNETMotion[3],(LONG)(GetTickCount64()-capture_started));
    InterlockedExchange(&JNETMotion[18],(LONG)pose_link.animation.field);
    InterlockedExchange(&JNETMotion[19],(LONG)pose_link.animation.row);
    InterlockedExchange(&JNETMotion[20],(LONG)(uint32_t)pose_link.animation.value);
    InterlockedExchange(&JNETMotion[21],(LONG)(uint32_t)(pose_link.animation.value>>32));
    memcpy(sample.identity,pose_link.identity,16); sample.chapter=(uint32_t)chapter;
    sample.flags=pose_link.mode>=4 ? JC_POSE_ROOM | (pose_link.mode==4 ? JC_POSE_DUPLEX : 0) : pose_link.mode==2 ? 0 : JC_POSE_DUPLEX;
    uint32_t appearance[4],appearance_again[4];
    if (own_read(NULL,barn+0xb0+0x68,appearance,sizeof(appearance)) &&
        own_read(NULL,barn+0xb0+0x68,appearance_again,sizeof(appearance_again)) &&
        !memcmp(appearance,appearance_again,sizeof(appearance)) &&
        own_read(NULL,game+0x1e0,&again,8) && again==barn &&
        jc_pose_appearance_valid(JC_POSE_APPEARANCE,appearance)) {
        memcpy(sample.appearance,appearance,sizeof(appearance)); sample.flags|=JC_POSE_APPEARANCE;
    }
    if (pose_link.sequence==UINT64_MAX) { pose_link_close(6); return; }
    sample.sequence=++pose_link.sequence;
    unsigned char packet[JC_POSE_BYTES];
    if (!jc_pose_encode(jc_game_hash,&sample,pose_mac,&pose_link,packet)) { pose_link_close(6); return; }
    pose_link.published=sample;
    if (pose_link.mode==4) {
        unsigned sent=0;
        for (unsigned i=0;i<2;++i) if (pose_link.room.peers[i].paired && !pose_link.room.peers[i].departed) {
            int result=pose_link_send_to(packet,pose_link.room.peers[i].peer_port);
            if (result<0) return;
            sent+=(unsigned)result;
        }
        if (sent) { InterlockedExchangeAdd(&JNETLink[12],(LONG)sent); InterlockedExchange(&JNETLink[14],chapter); }
        InterlockedExchange(&JNETLink[13],sent ? 0 : pose_link.room.peers[0].paired || pose_link.room.peers[1].paired ? 9 : -1);
        return;
    }
    struct sockaddr_in peer={0}; peer.sin_family=AF_INET; peer.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    peer.sin_port=htons(pose_link.receiver.peer_port);
    int sent=pose_link.mode==1 ? sendto(pose_link.socket,(const char *)packet,sizeof(packet),0,(const struct sockaddr *)&peer,sizeof(peer)) :
        send(pose_link.socket,(const char *)packet,sizeof(packet),0);
    if (sent==sizeof(packet)) {
        InterlockedIncrement(&JNETLink[12]); InterlockedExchange(&JNETLink[13],0);
        InterlockedExchange(&JNETLink[14],chapter);
        if (pose_link.mode==2) {
            InterlockedIncrement(&JNETLink[3]); InterlockedExchange(&JNETLink[5],0);
            InterlockedExchange(&JNETLink[8],chapter);
        }
    }
    else if (sent==SOCKET_ERROR && pose_link_io_defer(WSAGetLastError())) InterlockedExchange(&JNETLink[13],9);
    else { pose_link_close(5); return; }
}
#endif
