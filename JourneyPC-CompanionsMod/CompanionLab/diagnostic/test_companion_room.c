#include "companion_room.h"
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
typedef SOCKET room_socket;
typedef int room_socklen;
#else
#include <CommonCrypto/CommonHMAC.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int room_socket;
typedef socklen_t room_socklen;
#endif
static unsigned char key[32]={23},build[16]={42};
static int mac(void *ctx,const unsigned char *p,size_t n,unsigned char out[32]) {
    (void)ctx;
#ifdef _WIN32
    BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_HASH_HANDLE hash=NULL;
    int ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)>=0;
    if (ok) ok=BCryptCreateHash(alg,&hash,NULL,0,key,32,0)>=0;
    if (ok) ok=BCryptHashData(hash,(PUCHAR)p,(ULONG)n,0)>=0 && BCryptFinishHash(hash,out,32,0)>=0;
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg,0);
    return ok;
#else
    CCHmac(kCCHmacAlgSHA256,key,32,p,n,out); return 1;
#endif
}
static void packet_for(unsigned id,uint64_t sequence,unsigned flags,unsigned chapter,unsigned char out[JC_POSE_BYTES]) {
    struct jc_pose_sample sample={.sequence=sequence,.flags=flags,.chapter=chapter,
        .transform={1,0,0,0,0,1,0,0,0,0,1,0,0,20,30,1}};
    sample.identity[0]=(unsigned char)id; sample.transform[12]=(float)(id*10);
    assert(jc_pose_encode(build,&sample,mac,NULL,out));
}
static int receive(struct jc_pose_room *r,const unsigned char *p,size_t bytes,uint64_t now,uint16_t port,unsigned *slot) {
    return jc_room_receive(r,build,p,bytes,2,now,port,mac,NULL,slot);
}
static void policy(void) {
    struct jc_pose_room host,join; unsigned char own[16]={1},packet[JC_POSE_BYTES]; unsigned slot;
    jc_room_init(&host,1,own,0);
    packet_for(2,1,JC_POSE_ROOM,2,packet);
    packet[120]^=1; assert(receive(&host,packet,sizeof(packet),1000,2000,&slot)==4);
    assert(slot==2 && !host.peers[0].paired && !host.peers[0].peer_port);
    packet[120]^=1;
    assert(receive(&host,packet,sizeof(packet)-1,1001,2000,&slot)==3);
    assert(!receive(&host,packet,sizeof(packet),1002,2000,&slot) && slot==0);
    assert(receive(&host,packet,sizeof(packet),1003,2000,&slot)==6 && host.peers[0].last_seen==1002);
    packet_for(2,2,JC_POSE_ROOM,2,packet);
    assert(receive(&host,packet,sizeof(packet),1004,2001,&slot)==5 && host.peers[0].sample.sequence==1);
    packet_for(3,1,JC_POSE_ROOM,2,packet);
    assert(receive(&host,packet,sizeof(packet),1005,2000,&slot)==5 && !host.peers[1].paired);
    assert(!receive(&host,packet,sizeof(packet),1006,3000,&slot) && slot==1);
    assert(jc_room_relay_port(&host,slot)==2000);
    packet_for(4,1,JC_POSE_ROOM,2,packet);
    assert(receive(&host,packet,sizeof(packet),1007,4000,&slot)==10);
    assert(host.peers[0].sample.identity[0]==2 && host.peers[1].sample.identity[0]==3);
    struct jc_pose_receiver selected;
    assert(jc_room_selected(&host,&selected) && selected.sample.identity[0]==2);
    assert(jc_room_next(&host));
    assert(jc_room_selected(&host,&selected) && selected.sample.identity[0]==3 && selected.sample.sequence==1);
    assert(jc_room_next(&host) && host.selected==0);
    packet_for(2,2,JC_POSE_ROOM,3,packet);
    assert(receive(&host,packet,sizeof(packet),1008,2000,&slot)==8 && !host.peers[0].ready);
    assert(host.peers[0].sample.chapter==3 && jc_room_relay_port(&host,slot)==3000);
    packet_for(2,3,JC_POSE_ROOM,2,packet);
    assert(!receive(&host,packet,sizeof(packet),1009,2000,&slot));
    assert(jc_pose_fresh(&host.peers[0],2,6008) && !jc_pose_fresh(&host.peers[0],2,6009));
    assert(receive(&host,packet,sizeof(packet),1008,2000,&slot)==1);
    own[0]=2; jc_room_init(&join,0,own,1000);
    assert(!jc_room_next(&join) && !jc_room_selected(&join,&selected));
    packet_for(1,1,JC_POSE_ROLE_FLAGS,2,packet);
    assert(receive(&join,packet,sizeof(packet),1000,1001,&slot)==5);
    assert(!receive(&join,packet,sizeof(packet),1001,1000,&slot) && slot==0);
    packet_for(3,1,JC_POSE_ROOM,2,packet);
    assert(!receive(&join,packet,sizeof(packet),1002,1000,&slot) && slot==1);
    packet_for(1,2,JC_POSE_ROOM,2,packet);
    assert(receive(&join,packet,sizeof(packet),1003,1000,&slot)==5);
    packet_for(4,2,JC_POSE_ROLE_FLAGS,2,packet);
    assert(receive(&join,packet,sizeof(packet),1004,1000,&slot)==5);
    packet_for(2,2,JC_POSE_ROOM,2,packet);
    assert(receive(&join,packet,sizeof(packet),1005,1000,&slot)==5);
    packet_for(3,2,JC_POSE_DUPLEX,2,packet);
    assert(receive(&join,packet,sizeof(packet),1006,1000,&slot)==3);
    packet_for(3,2,JC_POSE_ROOM,2,packet); packet[60]^=1;
    assert(receive(&join,packet,sizeof(packet),1007,1000,&slot)==4 && join.received[1]==1);
    for (unsigned i=0;i<40;++i) receive(&join,packet,sizeof(packet),1008,1000,&slot);
    assert(receive(&join,packet,sizeof(packet),1008,1000,&slot)==2);
    packet[60]^=1;
    assert(!receive(&join,packet,sizeof(packet),2008,1000,&slot));
    assert(!jc_pose_fresh(&join.peers[0],2,6001) && jc_pose_fresh(&join.peers[1],2,6001));
}
#ifndef JC_NO_SOCKET_TESTS
static struct sockaddr_in address(uint16_t port) {
    struct sockaddr_in a={0}; a.sin_family=AF_INET; a.sin_addr.s_addr=htonl(INADDR_LOOPBACK); a.sin_port=htons(port); return a;
}
static void send_packet(room_socket s,uint16_t port,const unsigned char packet[JC_POSE_BYTES]) {
    struct sockaddr_in a=address(port);
    assert(sendto(s,(const char *)packet,JC_POSE_BYTES,0,(struct sockaddr *)&a,sizeof(a))==JC_POSE_BYTES);
}
static unsigned recv_packet(room_socket s,struct jc_pose_room *room,uint64_t now,unsigned char packet[JC_POSE_BYTES]) {
    struct sockaddr_in source={0}; room_socklen size=sizeof(source); unsigned slot;
    unsigned char readback[JC_POSE_BYTES+1];
    int n=(int)recvfrom(s,(char *)readback,sizeof(readback),0,(struct sockaddr *)&source,&size);
    assert(n==JC_POSE_BYTES && source.sin_addr.s_addr==htonl(INADDR_LOOPBACK));
    assert(!receive(room,readback,(size_t)n,now,ntohs(source.sin_port),&slot));
    memcpy(packet,readback,JC_POSE_BYTES); return slot;
}
static void three_sockets(void) {
#ifdef _WIN32
    WSADATA data; assert(!WSAStartup(MAKEWORD(2,2),&data));
#endif
    room_socket sockets[3]; uint16_t ports[3]; struct jc_pose_room rooms[3];
    for (unsigned i=0;i<3;++i) {
        sockets[i]=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
#ifdef _WIN32
        assert(sockets[i]!=INVALID_SOCKET); DWORD timeout=1000;
#else
        assert(sockets[i]>=0); struct timeval timeout={1,0};
#endif
        assert(!setsockopt(sockets[i],SOL_SOCKET,SO_RCVTIMEO,(const char *)&timeout,sizeof(timeout)));
        struct sockaddr_in a=address(0); room_socklen size=sizeof(a);
        assert(!bind(sockets[i],(struct sockaddr *)&a,sizeof(a)));
        assert(!getsockname(sockets[i],(struct sockaddr *)&a,&size)); ports[i]=ntohs(a.sin_port);
        unsigned char own[16]={(unsigned char)(i+1)}; jc_room_init(&rooms[i],i==0,own,i ? ports[0] : 0);
    }
    unsigned char packet[JC_POSE_BYTES];
    packet_for(2,1,JC_POSE_ROOM,2,packet); send_packet(sockets[1],ports[0],packet);
    unsigned slot=recv_packet(sockets[0],&rooms[0],1000,packet);
    assert(!jc_room_relay_port(&rooms[0],slot));
    packet_for(1,1,JC_POSE_ROLE_FLAGS,2,packet); send_packet(sockets[0],ports[1],packet);
    recv_packet(sockets[1],&rooms[1],1001,packet);
    packet_for(3,1,JC_POSE_ROOM,2,packet); send_packet(sockets[2],ports[0],packet);
    slot=recv_packet(sockets[0],&rooms[0],1002,packet);
    send_packet(sockets[0],jc_room_relay_port(&rooms[0],slot),packet);
    recv_packet(sockets[1],&rooms[1],1003,packet);
    packet_for(1,2,JC_POSE_ROLE_FLAGS,2,packet); send_packet(sockets[0],ports[2],packet);
    recv_packet(sockets[2],&rooms[2],1004,packet);
    packet_for(2,2,JC_POSE_ROOM,2,packet); send_packet(sockets[1],ports[0],packet);
    slot=recv_packet(sockets[0],&rooms[0],1005,packet);
    send_packet(sockets[0],jc_room_relay_port(&rooms[0],slot),packet);
    recv_packet(sockets[2],&rooms[2],1006,packet);
    for (unsigned i=0;i<3;++i) {
        assert(rooms[i].peers[0].paired && rooms[i].peers[1].paired);
        assert(jc_pose_fresh(&rooms[i].peers[0],2,1007) && jc_pose_fresh(&rooms[i].peers[1],2,1007));
        struct jc_pose_receiver first,next;
        assert(jc_room_selected(&rooms[i],&first) && jc_room_next(&rooms[i]) && jc_room_selected(&rooms[i],&next));
        assert(memcmp(first.sample.identity,next.sample.identity,16));
    }
    packet_for(1,3,JC_POSE_ROLE_FLAGS,2,packet); send_packet(sockets[0],ports[1],packet);
    recv_packet(sockets[1],&rooms[1],7000,packet);
    assert(jc_pose_fresh(&rooms[1].peers[0],2,7000) && !jc_pose_fresh(&rooms[1].peers[1],2,7000));
    packet_for(3,2,JC_POSE_ROOM,2,packet); send_packet(sockets[2],ports[0],packet);
    slot=recv_packet(sockets[0],&rooms[0],7001,packet);
    send_packet(sockets[0],jc_room_relay_port(&rooms[0],slot),packet);
    recv_packet(sockets[1],&rooms[1],7002,packet);
    assert(jc_pose_fresh(&rooms[1].peers[1],2,7002));
    packet_for(3,3,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    send_packet(sockets[2],ports[0],packet);
    slot=recv_packet(sockets[0],&rooms[0],7100,packet);
    send_packet(sockets[0],jc_room_relay_port(&rooms[0],slot),packet);
    recv_packet(sockets[1],&rooms[1],7101,packet);
    assert(jc_pose_presence(&rooms[0].peers[1],2,7101)==3);
    assert(jc_pose_presence(&rooms[1].peers[1],2,7101)==3);
    assert(!jc_pose_fresh(&rooms[0].peers[1],2,7101));
    packet_for(2,3,JC_POSE_ROOM,2,packet); send_packet(sockets[1],ports[0],packet);
    slot=recv_packet(sockets[0],&rooms[0],7102,packet);
    assert(!jc_room_relay_port(&rooms[0],slot));
    packet_for(1,4,JC_POSE_ROLE_FLAGS,2,packet); send_packet(sockets[0],ports[1],packet);
    recv_packet(sockets[1],&rooms[1],7103,packet);
    assert(jc_pose_fresh(&rooms[0].peers[0],2,7103) && jc_pose_fresh(&rooms[1].peers[0],2,7103));
    for (unsigned i=0;i<3;++i) {
#ifdef _WIN32
        closesocket(sockets[i]);
#else
        close(sockets[i]);
#endif
    }
#ifdef _WIN32
    WSACleanup();
#endif
}
#else
static void three_sockets(void) {}
#endif
static void departure_policy(void) {
    struct jc_pose_room host; unsigned char own[16]={1},packet[JC_POSE_BYTES]; unsigned slot;
    jc_room_init(&host,1,own,0);
    packet_for(2,1,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    assert(receive(&host,packet,sizeof(packet),1000,2000,&slot)==5 && !host.peers[0].paired);
    packet_for(2,1,JC_POSE_ROOM,2,packet);
    assert(!receive(&host,packet,sizeof(packet),1001,2000,&slot));
    packet_for(3,1,JC_POSE_ROOM,2,packet);
    assert(!receive(&host,packet,sizeof(packet),1002,3000,&slot));
    packet_for(2,2,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    assert(receive(&host,packet,sizeof(packet),1003,2001,&slot)==5);
    packet[100]^=1;
    assert(receive(&host,packet,sizeof(packet),1004,2000,&slot)==4);
    assert(jc_pose_presence(&host.peers[0],2,1004)==1);
    packet_for(2,1,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    assert(receive(&host,packet,sizeof(packet),1004,2000,&slot)==6 && !host.peers[0].departed);
    packet_for(2,2,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    jc_session_put(packet+56,0x7fc00000,4);
    assert(mac(NULL,packet,JC_POSE_BODY,packet+JC_POSE_BODY));
    assert(receive(&host,packet,sizeof(packet),1004,2000,&slot)==7 && !host.peers[0].departed);
    assert(host.peers[0].sample.sequence==1 && host.peers[0].last_seen==1001);
    packet_for(2,2,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    memcpy(packet,"JCP3",4); jc_session_put(packet+4,3,2);
    assert(mac(NULL,packet,JC_POSE_BODY,packet+JC_POSE_BODY));
    assert(receive(&host,packet,sizeof(packet),1004,2000,&slot)==3 && !host.peers[0].departed);
    packet_for(2,2,JC_POSE_ROOM|JC_POSE_LEAVE,2,packet);
    assert(!receive(&host,packet,sizeof(packet),1005,2000,&slot) && slot==0);
    assert(jc_pose_presence(&host.peers[0],2,1005)==3);
    assert(jc_room_relay_port(&host,slot)==3000);
    struct jc_pose_receiver departed=host.peers[0];
    assert(receive(&host,packet,sizeof(packet),1006,2000,&slot)==11 && slot==2);
    packet_for(2,3,JC_POSE_ROOM,2,packet);
    assert(receive(&host,packet,sizeof(packet),1007,2000,&slot)==11);
    assert(host.peers[0].last_seen==departed.last_seen && host.peers[0].sample.sequence==2);
    packet_for(4,1,JC_POSE_ROOM,2,packet);
    assert(receive(&host,packet,sizeof(packet),1008,4000,&slot)==10);
    packet_for(3,2,JC_POSE_ROOM,3,packet);
    assert(receive(&host,packet,sizeof(packet),1009,3000,&slot)==8);
    assert(jc_pose_presence(&host.peers[1],2,1009)==4);
    packet_for(3,3,JC_POSE_ROOM,2,packet);
    assert(!receive(&host,packet,sizeof(packet),1010,3000,&slot));
    assert(jc_pose_presence(&host.peers[1],2,6010)==2);
    packet_for(3,4,JC_POSE_ROOM,2,packet);
    assert(!receive(&host,packet,sizeof(packet),6011,3000,&slot));
    assert(jc_pose_presence(&host.peers[1],2,6011)==1);
    packet_for(3,5,JC_POSE_ROOM|JC_POSE_LEAVE,3,packet);
    assert(!receive(&host,packet,sizeof(packet),6012,3000,&slot));
    assert(jc_pose_presence(&host.peers[1],2,6012)==3);
    for (unsigned mode=1;mode<=5;++mode) {
        assert(jc_room_io_defer(mode,10035));
        assert(jc_room_io_defer(mode,10054)==(mode>=4));
        assert(jc_room_io_defer(mode,10061)==(mode>=4));
        assert(!jc_room_io_defer(mode,10038) && !jc_room_io_defer(mode,10050) && !jc_room_io_defer(mode,0));
    }
}
static void appearance_relay(void) {
    struct jc_pose_room host,join; unsigned char host_id[16]={1},join_id[16]={3},packet[JC_POSE_BYTES]; unsigned slot;
    jc_room_init(&host,1,host_id,0); jc_room_init(&join,0,join_id,1000);
    struct jc_pose_sample s={.identity={1},.sequence=1,.chapter=2,.flags=JC_POSE_ROLE_FLAGS|JC_POSE_APPEARANCE,
        .transform={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},.appearance={3,4,0,1}};
    assert(jc_pose_encode(build,&s,mac,NULL,packet));
    assert(!receive(&join,packet,sizeof(packet),1000,1000,&slot));
    s.identity[0]=2;s.flags=JC_POSE_ROOM|JC_POSE_APPEARANCE;s.appearance[0]=20;s.appearance[1]=32;s.appearance[2]=6;s.appearance[3]=17;
    assert(jc_pose_encode(build,&s,mac,NULL,packet));
    assert(!receive(&host,packet,sizeof(packet),1001,2000,&slot));
    /* Forward the original authenticated packet, not a host-generated appearance. */
    assert(!receive(&join,packet,sizeof(packet),1002,1000,&slot));
    assert(join.peers[0].sample.appearance[2]==0 && join.peers[1].sample.appearance[2]==6);
    assert(!memcmp(host.peers[0].sample.appearance,join.peers[1].sample.appearance,sizeof(s.appearance)));
}
int main(void) {
    struct jc_pose_room host; unsigned char own[16]={1},packet[JC_POSE_BYTES]; unsigned slot;
    jc_room_init(&host,1,own,0);
    for (unsigned tick=0;tick<100;++tick) for (unsigned peer=0;peer<2;++peer) {
        packet_for(peer+2,tick+1,JC_POSE_ROOM,2,packet);
        assert(!receive(&host,packet,sizeof(packet),1000+tick*100,(uint16_t)(2000+peer),&slot));
        assert(slot==peer);
    }
    assert(host.received[0]==100 && host.received[1]==100);
    policy(); departure_policy(); appearance_relay(); three_sockets();
#ifdef JC_NO_SOCKET_TESTS
    puts("Three-endpoint UDP exercise omitted; room admission/relay policies ran without sockets.");
#endif
    puts("companion room: per-player appearance relay, authentication, admission cap, identity/port pinning, replay, chapter isolation, stale recovery, authenticated departure relay, surviving peer traffic, terminal identity and bounded UDP error policy passed");
    return 0;
}
