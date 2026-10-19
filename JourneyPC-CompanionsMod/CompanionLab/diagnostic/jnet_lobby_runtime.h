#ifndef JNET_LOBBY_RUNTIME_H
#define JNET_LOBBY_RUNTIME_H
#define JNET_LOBBY_BYTES 160
static struct {
    SOCKET socket;unsigned initialized,access,mask,host,ready;
    uint64_t room,sequence,next_send,next_probe,next_capture,last_ready,request,deadline,pending_host,pending_room;
    char password[33],pending_password[33],status[80];
    struct { uint64_t epoch,sequence,seen,room;unsigned access,mask,host; } peers[2];
    uint64_t member_seen[4];
} jnet_room={.socket=INVALID_SOCKET};
static unsigned jnet_lobby_bit(unsigned id){return id>=1 && id<=3?1u<<id:0;}
static int jnet_lobby_allows(uint64_t peer) {
    if(traveler_network_external){for(unsigned i=0;i<2;++i)if(peer && traveler_network.peers[i].connection.peer==peer && traveler_network.peers[i].connection.admitted)return 1;return 0;}
    return jnet_room.room && (jnet_room.mask&jnet_lobby_bit((unsigned)peer));
}
static unsigned jnet_lobby_population(unsigned mask) {
    return !!(mask&2)+!!(mask&4)+!!(mask&8);
}
static int jnet_lobby_proof(const char *password,uint64_t epoch,uint64_t request,unsigned peer,unsigned host,unsigned char out[32]) {
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char input[24]={0};int ok=0;
    jc_packet_put(input,epoch,8);jc_packet_put(input+8,request,8);jc_packet_put(input+16,peer,4);jc_packet_put(input+20,host,4);
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)goto done;
    if(BCryptCreateHash(algorithm,&hash,NULL,0,(PUCHAR)password,(ULONG)strlen(password),0)<0)goto done;
    ok=BCryptHashData(hash,input,sizeof(input),0)>=0 && BCryptFinishHash(hash,out,32,0)>=0;
done:
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return ok;
}
static int jnet_lobby_mac_equal(const unsigned char *a,const unsigned char *b) {
    unsigned diff=0;for(unsigned i=0;i<32;++i)diff|=a[i]^b[i];return !diff;
}
static void jnet_lobby_send(unsigned slot,unsigned type,uint64_t request) {
    struct jc_network_peer *peer=&traveler_network.peers[slot];
    unsigned char p[JNET_LOBBY_BYTES]={0};memcpy(p,"JNL1",4);jc_packet_put(p+4,type,4);
    memcpy(p+8,jc_game_hash,16);jc_packet_put(p+24,traveler_network.identity,8);
    jc_packet_put(p+32,peer->connection.peer,8);jc_packet_put(p+40,traveler_network.epoch,8);
    jc_packet_put(p+48,++jnet_room.sequence,8);
    unsigned visible=jnet_room.room && jnet_room.host==traveler_network.identity &&
        (jnet_room.access==JNET_PUBLIC || (jnet_room.mask&jnet_lobby_bit((unsigned)peer->connection.peer)));
    if(type==2) {
        jc_packet_put(p+56,jnet_room.pending_room,8);jc_packet_put(p+88,request,8);
        if(!jnet_lobby_proof(jnet_room.pending_password,jnet_room.peers[slot].epoch,request,
            (unsigned)traveler_network.identity,(unsigned)peer->connection.peer,p+96))return;
    } else if(type==4 || type==5 || visible || type==3) {
        jc_packet_put(p+56,jnet_room.room,8);jc_packet_put(p+64,jnet_room.host,8);
        jc_packet_put(p+72,jnet_room.mask,4);jc_packet_put(p+76,jnet_room.access,4);
        jc_packet_put(p+80,JNET_JOURNEY,4);jc_packet_put(p+84,2,4);jc_packet_put(p+88,request,8);
    }
    if(!traveler_udp_mac(&peer->authentication,p,128,p+128))return;
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    address.sin_port=htons((u_short)(43940+peer->connection.peer));
    sendto(jnet_room.socket,(const char *)p,sizeof(p),0,(const struct sockaddr *)&address,sizeof(address));
}
static void jnet_lobby_depart(void) {
    if(jnet_room.room)for(unsigned i=0;i<2;++i)jnet_lobby_send(i,4,0);
    jnet_room.room=0;jnet_room.host=jnet_room.mask=0;jnet_room.request=0;
    SecureZeroMemory(jnet_room.password,sizeof(jnet_room.password));
    SecureZeroMemory(jnet_room.pending_password,sizeof(jnet_room.pending_password));
    traveler_network.paused=1;JNETNetwork[0]=2;
    traveler_local_stop();if(traveler_actor)traveler_actor_trial_stop(0);
    traveler_actor_trial_join_pending=0;traveler_peer_restart_requested=0;
    for(unsigned i=0;i<2;++i)traveler_network.peers[i].connection.last_seen=0;
}
static int jnet_lobby_connect(void) {
    int code=traveler_local_network_connect();
    if(code){snprintf(jnet_room.status,sizeof(jnet_room.status),"Connection unavailable (%d)",code);return 0;}
    return 1;
}
static int jnet_lobby_idle(void) {
    return !jnet_room.room && !jnet_room.request && (traveler_local_state==0 || traveler_local_state==4) &&
        (!traveler_actor || traveler_peer_vacant()) && !traveler_actor_trial_join_pending;
}
static void jnet_lobby_receive(uint64_t now) {
    for(unsigned n=0;n<24;++n) {
        unsigned char p[JNET_LOBBY_BYTES+1],tag[32];struct sockaddr_in address;int size=sizeof(address);
        int bytes=recvfrom(jnet_room.socket,(char *)p,sizeof(p),0,(struct sockaddr *)&address,&size);
        if(bytes==SOCKET_ERROR) {if(WSAGetLastError()==WSAEMSGSIZE)continue;break;}
        if(bytes!=JNET_LOBBY_BYTES || memcmp(p,"JNL1",4) || memcmp(p+8,jc_game_hash,16) ||
            address.sin_family!=AF_INET || address.sin_addr.s_addr!=htonl(INADDR_LOOPBACK))continue;
        unsigned slot;for(slot=0;slot<2;++slot)if(ntohs(address.sin_port)==43940+traveler_network.peers[slot].connection.peer)break;
        if(slot==2)continue;
        unsigned id=(unsigned)traveler_network.peers[slot].connection.peer;
        if(jc_packet_get(p+24,8)!=id || jc_packet_get(p+32,8)!=traveler_network.identity ||
            !traveler_udp_mac(&traveler_network.peers[slot].authentication,p,128,tag) || !jnet_lobby_mac_equal(tag,p+128))continue;
        uint64_t epoch=jc_packet_get(p+40,8),sequence=jc_packet_get(p+48,8),room=jc_packet_get(p+56,8),request=jc_packet_get(p+88,8);
        if(!epoch || !sequence || epoch<jnet_room.peers[slot].epoch ||
            (epoch==jnet_room.peers[slot].epoch && sequence<=jnet_room.peers[slot].sequence))continue;
        if(epoch!=jnet_room.peers[slot].epoch) {
            jnet_room.peers[slot].room=0;
            if(jnet_room.host==id)jnet_lobby_depart();
            if(jnet_room.host==traveler_network.identity)jnet_room.mask&=~jnet_lobby_bit(id);
        }
        jnet_room.peers[slot].epoch=epoch;jnet_room.peers[slot].sequence=sequence;jnet_room.peers[slot].seen=now;
        unsigned type=(unsigned)jc_packet_get(p+4,4),host=(unsigned)jc_packet_get(p+64,8),mask=(unsigned)jc_packet_get(p+72,4);
        unsigned access=(unsigned)jc_packet_get(p+76,4);
        if(type==1 || type==3) {
            int valid=room && host==id && access<=JNET_PRIVATE && !(mask&~14u) && (mask&jnet_lobby_bit(id)) &&
                jc_packet_get(p+80,4)==JNET_JOURNEY && jc_packet_get(p+84,4)==2;
            jnet_room.peers[slot].room=valid?room:0;jnet_room.peers[slot].access=access;
            jnet_room.peers[slot].mask=valid?mask:0;jnet_room.peers[slot].host=id;
            if(type==3 && valid && jnet_room.request && request==jnet_room.request &&
                (!jnet_room.pending_host || id==jnet_room.pending_host) &&
                (!jnet_room.pending_room || room==jnet_room.pending_room) && (mask&jnet_lobby_bit((unsigned)traveler_network.identity))) {
                if(jnet_room.ready && jnet_lobby_connect()) {
                    jnet_room.room=room;jnet_room.host=id;jnet_room.mask=mask;jnet_room.access=access;
                    jnet_room.request=0;SecureZeroMemory(jnet_room.pending_password,sizeof(jnet_room.pending_password));
                    strcpy(jnet_room.status,"Joined lobby");
                }
            }
            if(jnet_room.room && jnet_room.host==id) {
                if(!valid || room!=jnet_room.room || !(mask&jnet_lobby_bit((unsigned)traveler_network.identity))) {
                    jnet_lobby_depart();strcpy(jnet_room.status,"Lobby closed or membership ended");
                } else jnet_room.mask=mask;
            }
        } else if(type==2 && jnet_room.ready && jnet_room.room && jnet_room.host==traveler_network.identity && request &&
            (room==jnet_room.room || (!room && jnet_room.access==JNET_PRIVATE))) {
            unsigned char proof[32];
            if(!jnet_lobby_proof(jnet_room.password,traveler_network.epoch,request,id,(unsigned)traveler_network.identity,proof) ||
                !jnet_lobby_mac_equal(proof,p+96))continue;
            if(!(jnet_room.mask&jnet_lobby_bit(id)) && jnet_lobby_population(jnet_room.mask)>=3)continue;
            jnet_room.mask|=jnet_lobby_bit(id);jnet_room.member_seen[id]=now;
            jnet_lobby_send(slot,3,request);jnet_room.next_send=0;
        } else if(type==4 && room==jnet_room.room) {
            if(jnet_room.host==id){jnet_lobby_depart();strcpy(jnet_room.status,"Host closed the lobby");}
            else if(jnet_room.host==traveler_network.identity){jnet_room.mask&=~jnet_lobby_bit(id);jnet_room.next_send=0;}
        } else if(type==5 && room==jnet_room.room && jnet_room.host==traveler_network.identity && (jnet_room.mask&jnet_lobby_bit(id)))
            jnet_room.member_seen[id]=now;
    }
}
static void jnet_lobby_tick(uintptr_t game) {
    uint64_t now=GetTickCount64();struct jnet_lobby_action action;
    AcquireSRWLockExclusive(&jnet_lobby_lock);action=jnet_lobby_action;SecureZeroMemory(&jnet_lobby_action,sizeof(jnet_lobby_action));ReleaseSRWLockExclusive(&jnet_lobby_lock);
    if(now>=jnet_room.next_capture) {
        struct jc_actor_sender sample;
        jnet_room.ready=JNETTraveler[0]==3 && !jc_actor_capture_sender(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),game,&sample) && sample.chapter==2;
        if(jnet_room.ready)jnet_room.last_ready=now;
        jnet_room.next_capture=now+250;
    }
    unsigned ready=jnet_room.ready;
    if(!jnet_room.initialized && ready && now>=jnet_room.next_probe) {
        jnet_room.next_probe=now+2000;
        int code=traveler_network_start();traveler_network.paused=1;
        if(!code) {
            jnet_room.socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);u_long nonblocking=1;BOOL exclusive=TRUE;
            struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
            address.sin_port=htons((u_short)(43940+traveler_network.identity));
            if(jnet_room.socket!=INVALID_SOCKET && !setsockopt(jnet_room.socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char *)&exclusive,sizeof(exclusive)) &&
                !ioctlsocket(jnet_room.socket,FIONBIO,&nonblocking) && !bind(jnet_room.socket,(const struct sockaddr *)&address,sizeof(address))) {
                jnet_room.initialized=1;strcpy(jnet_room.status,"Local lobbies ready");
            } else {if(jnet_room.socket!=INVALID_SOCKET)closesocket(jnet_room.socket);jnet_room.socket=INVALID_SOCKET;strcpy(jnet_room.status,"Lobby port unavailable");}
        } else snprintf(jnet_room.status,sizeof(jnet_room.status),"Lobby setup unavailable (%d)",code);
    }
    if(jnet_room.initialized) {
        if(!ready && jnet_room.room && now-jnet_room.last_ready>=2000){jnet_lobby_depart();strcpy(jnet_room.status,"Lobby ended - enter Desert");}
        jnet_lobby_receive(now);
        if(jnet_room.request && now>=jnet_room.deadline) {
            jnet_room.request=0;SecureZeroMemory(jnet_room.pending_password,sizeof(jnet_room.pending_password));strcpy(jnet_room.status,"Join timed out - check password and lobby");
        }
        if(jnet_room.host==traveler_network.identity)for(unsigned id=1;id<=3;++id)
            if(id!=jnet_room.host && (jnet_room.mask&jnet_lobby_bit(id)) && now-jnet_room.member_seen[id]>5000)jnet_room.mask&=~jnet_lobby_bit(id);
        if(jnet_room.room && jnet_room.host!=traveler_network.identity)for(unsigned i=0;i<2;++i)
            if(traveler_network.peers[i].connection.peer==jnet_room.host && now-jnet_room.peers[i].seen>5000){jnet_lobby_depart();strcpy(jnet_room.status,"Host timed out");}
        if(action.kind==3) {jnet_lobby_depart();strcpy(jnet_room.status,"Left lobby - departure in progress");}
        else if(action.kind) {
            if(!ready)strcpy(jnet_room.status,"Enter Desert before creating or joining");
            else if(!jnet_lobby_idle())strcpy(jnet_room.status,"Leave the lobby and wait for departure first");
            else if(action.kind==1) {
                if(action.mode!=JNET_JOURNEY)strcpy(jnet_room.status,"Rocket Deathmatch is not implemented");
                else if(action.access==JNET_PRIVATE && !action.password[0])strcpy(jnet_room.status,"Private lobby needs a password");
                else if(jnet_lobby_connect()) {
                    FILETIME ft;GetSystemTimeAsFileTime(&ft);memcpy(&jnet_room.room,&ft,8);
                    jnet_room.host=(unsigned)traveler_network.identity;jnet_room.mask=jnet_lobby_bit(jnet_room.host);
                    jnet_room.access=action.access;memcpy(jnet_room.password,action.password,33);jnet_room.next_send=0;
                    strcpy(jnet_room.status,action.access?"Private lobby created":"Public lobby created");
                }
            } else if(action.kind==2) {
                if(!action.host && !action.password[0])strcpy(jnet_room.status,"Select a public lobby or enter a private password");
                else if(BCryptGenRandom(NULL,(PUCHAR)&jnet_room.request,8,BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0 && jnet_room.request) {
                    jnet_room.pending_host=action.host;jnet_room.pending_room=action.room;jnet_room.deadline=now+5000;
                    memcpy(jnet_room.pending_password,action.password,33);jnet_room.next_send=0;strcpy(jnet_room.status,"Requesting admission");
                }
            }
        }
        if(now>=jnet_room.next_send) {
            for(unsigned i=0;i<2;++i) {
                jnet_lobby_send(i,1,0);
                if(jnet_room.room && jnet_room.host!=traveler_network.identity && traveler_network.peers[i].connection.peer==jnet_room.host)jnet_lobby_send(i,5,0);
                if(jnet_room.request && jnet_room.peers[i].epoch && (!jnet_room.pending_host || traveler_network.peers[i].connection.peer==jnet_room.pending_host))jnet_lobby_send(i,2,jnet_room.request);
            }
            jnet_room.next_send=now+350;
        }
    } else if(action.kind)strcpy(jnet_room.status,"Enter Desert to initialize local lobbies");
    struct jnet_lobby_view view={.identity=(unsigned)traveler_network.identity,.ready=ready,.room=jnet_room.room};
    snprintf(view.status,sizeof(view.status),"%s",jnet_room.status[0]?jnet_room.status:"Enter Desert for local lobbies");
    if(jnet_room.room && jnet_room.host==traveler_network.identity)
        view.rows[view.count++]=(struct jnet_lobby_row){jnet_room.room,jnet_room.host,jnet_lobby_population(jnet_room.mask),jnet_room.access};
    for(unsigned i=0;i<2;++i)if(jnet_room.peers[i].room && now-jnet_room.peers[i].seen<2500 &&
        (jnet_room.peers[i].access==JNET_PUBLIC || jnet_room.peers[i].room==jnet_room.room))
        view.rows[view.count++]=(struct jnet_lobby_row){jnet_room.peers[i].room,jnet_room.peers[i].host,jnet_lobby_population(jnet_room.peers[i].mask),jnet_room.peers[i].access};
    AcquireSRWLockExclusive(&jnet_lobby_lock);jnet_lobby_view=view;ReleaseSRWLockExclusive(&jnet_lobby_lock);
    JNETMenu[13]=(LONG)view.identity;JNETMenu[14]=(LONG)jnet_room.host;
    JNETMenu[15]=(LONG)jnet_lobby_population(jnet_room.mask);JNETMenu[16]=(LONG)ready;
    JNETMenu[17]=jnet_room.request?1:0;JNETMenu[18]=(LONG)jnet_room.initialized;
    SecureZeroMemory(&action,sizeof(action));
}
#endif
