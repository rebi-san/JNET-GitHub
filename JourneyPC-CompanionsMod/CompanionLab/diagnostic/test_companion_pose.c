#include "companion_pose.h"
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#else
#include <CommonCrypto/CommonHMAC.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
static unsigned char key[32];
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
#ifndef JC_NO_SOCKET_TESTS
static void socket_roundtrip(const unsigned char packet[JC_POSE_BYTES],const unsigned char build[16]) {
#ifdef _WIN32
    WSADATA data; assert(!WSAStartup(MAKEWORD(2,2),&data));
    SOCKET receiver=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP),sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    assert(receiver!=INVALID_SOCKET && sender!=INVALID_SOCKET);
    DWORD timeout=1000; assert(!setsockopt(receiver,SOL_SOCKET,SO_RCVTIMEO,(const char *)&timeout,sizeof(timeout)));
    assert(!setsockopt(sender,SOL_SOCKET,SO_RCVTIMEO,(const char *)&timeout,sizeof(timeout)));
    int length;
#else
    int receiver=socket(AF_INET,SOCK_DGRAM,0),sender=socket(AF_INET,SOCK_DGRAM,0);
    assert(receiver>=0 && sender>=0);
    struct timeval timeout={1,0}; assert(!setsockopt(receiver,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)));
    assert(!setsockopt(sender,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)));
    socklen_t length;
#endif
    struct sockaddr_in address={0}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(!bind(receiver,(struct sockaddr *)&address,sizeof(address)));
    length=sizeof(address); assert(!getsockname(receiver,(struct sockaddr *)&address,&length));
    assert(sendto(sender,(const char *)packet,JC_POSE_BYTES,0,(struct sockaddr *)&address,sizeof(address))==JC_POSE_BYTES);
    unsigned char readback[JC_POSE_BYTES+1]; struct jc_pose_receiver state={.own_identity={9}};
    struct sockaddr_in source={0}; length=sizeof(source);
    int count=(int)recvfrom(receiver,(char *)readback,sizeof(readback),0,(struct sockaddr *)&source,&length);
    assert(count==JC_POSE_BYTES && source.sin_addr.s_addr==htonl(INADDR_LOOPBACK));
    assert(!jc_pose_receive_from(&state,build,readback,(size_t)count,2,1000,ntohs(source.sin_port),mac,NULL));
    if (state.sample.flags&JC_POSE_DUPLEX) {
        struct jc_pose_sample reply={.identity={9},.sequence=1,.chapter=2,.flags=JC_POSE_DUPLEX,
            .transform={1,0,0,0,0,1,0,0,0,0,1,0,100,20,30,1}};
        unsigned char response[JC_POSE_BYTES];
        assert(jc_pose_encode(build,&reply,mac,NULL,response));
        assert(sendto(receiver,(const char *)response,sizeof(response),0,(struct sockaddr *)&source,sizeof(source))==JC_POSE_BYTES);
        length=sizeof(source);
        count=(int)recvfrom(sender,(char *)readback,sizeof(readback),0,(struct sockaddr *)&source,&length);
        struct jc_pose_receiver joined={.required_flags=JC_POSE_DUPLEX,.peer_port=ntohs(address.sin_port)};
        memcpy(joined.own_identity,state.sample.identity,16);
        assert(count==JC_POSE_BYTES && source.sin_addr.s_addr==htonl(INADDR_LOOPBACK));
        assert(!jc_pose_receive_from(&joined,build,readback,(size_t)count,2,1001,ntohs(source.sin_port),mac,NULL));
        assert(joined.sample.transform[12]==100 && state.sample.transform[12]==30);
        assert(memcmp(joined.sample.identity,state.sample.identity,16));
    }
#ifdef _WIN32
    closesocket(receiver); closesocket(sender); WSACleanup();
#else
    close(receiver); close(sender);
#endif
}
#else
static void socket_roundtrip(const unsigned char *packet,const unsigned char *build) { (void)packet;(void)build; }
#endif
static void duplex_policy_tests(const unsigned char build[16]) {
    struct jc_pose_sample sample={.identity={2},.sequence=1,.chapter=2,.flags=JC_POSE_DUPLEX,
        .transform={1,0,0,0,0,1,0,0,0,0,1,0,30,40,50,1}};
    unsigned char packet[JC_POSE_BYTES];
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    socket_roundtrip(packet,build);
    struct jc_pose_receiver r={.own_identity={9},.required_flags=JC_POSE_DUPLEX},saved;
    packet[120]^=1;
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1000,12000,mac,NULL)==4 && !r.peer_port && !r.paired);
    packet[120]^=1;
    assert(!jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1001,12000,mac,NULL));
    assert(r.peer_port==12000 && r.sample.flags==JC_POSE_DUPLEX);
    saved=r; sample.sequence=2; sample.transform[12]=31;
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1002,12001,mac,NULL)==5);
    assert(!memcmp(&r,&saved,sizeof(r)));
    assert(!jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1003,12000,mac,NULL));
    sample.sequence=3; sample.flags=0; assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1004,12000,mac,NULL)==5 && r.sample.sequence==2);
    sample.flags=JC_POSE_DUPLEX; sample.identity[0]=9;
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1005,12000,mac,NULL)==5);
    sample.identity[0]=2; sample.chapter=3;
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,1006,12000,mac,NULL)==8 && !r.ready);
    sample.chapter=2; sample.sequence=4;
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(!jc_pose_receive_from(&r,build,packet,sizeof(packet),2,7007,12000,mac,NULL));
    assert(jc_pose_fresh(&r,2,7007));
    memcpy(packet,"JCP1",4); jc_session_put(packet+4,1,2);
    assert(mac(NULL,packet,JC_POSE_BODY,packet+JC_POSE_BODY));
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,7008,12000,mac,NULL)==3);
    sample.flags=4; assert(!jc_pose_encode(build,&sample,mac,NULL,packet));
    r=(struct jc_pose_receiver){.own_identity={2}}; sample.flags=JC_POSE_DUPLEX;
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(jc_pose_receive_from(&r,build,packet,sizeof(packet),2,8000,12000,mac,NULL)==5 && !r.peer_port);
    puts("Reciprocal pose policy: distinct transforms; authenticated endpoint pinning, reflection, capability changes, old protocol and chapter recovery checked.");
}
static void animation_packets(const unsigned char build[16]) {
    struct jc_pose_sample sample={.identity={2},.sequence=1,.chapter=2,.flags=JC_POSE_DUPLEX,
        .transform={1,0,0,0,0,1,0,0,0,0,1,0,30,40,50,1},
        .animation={.schema=UINT64_C(0x1020304050607080),.total=64,.count=2,
            .clips={{.index=3,.frame=2,.duration=-1,.speed=1,.weight=.75f},
                    {.index=50,.additive=1,.frame=1,.duration=3,.speed=.5f,.weight=.25f}}}};
    unsigned char packet[JC_POSE_BYTES]; struct jc_pose_receiver r={0};
    assert(jc_pose_encode(build,&sample,mac,NULL,packet));
    assert(!jc_pose_receive(&r,build,packet,sizeof(packet),2,1000,mac,NULL));
    assert(!memcmp(&r.sample.animation,&sample.animation,sizeof(sample.animation)));
    const unsigned offsets[]={128,132,136,140,144,148,152,156,160};
    const uint32_t values[]={513,17,64,2,0x7fc00000,0xc0000000,0x7f800000,0x41100000,3};
    for (unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i) {
        sample.sequence=2; assert(jc_pose_encode(build,&sample,mac,NULL,packet));
        jc_session_put(packet+offsets[i],values[i],4); assert(mac(NULL,packet,JC_POSE_BODY,packet+JC_POSE_BODY));
        struct jc_pose_sample saved=r.sample; uint64_t seen=r.last_seen;
        assert(jc_pose_receive(&r,build,packet,sizeof(packet),2,1001+i,mac,NULL)==7);
        assert(!memcmp(&r.sample,&saved,sizeof(saved)) && r.last_seen==seen);
    }
    puts("Animation protocol: numeric clips round-trip; authenticated bad counts, indices, additive flags, non-finite times/speeds, bounds and duplicates preserve last accepted state.");
}
static void appearance_packets(const unsigned char build[16]) {
    struct jc_pose_sample s={.identity={2},.sequence=1,.chapter=2,.flags=JC_POSE_APPEARANCE,
        .transform={1,0,0,0,0,1,0,0,0,0,1,0,30,40,50,1},.appearance={12,24,6,20}};
    unsigned char packet[JC_POSE_BYTES];
    for (unsigned outfit=0;outfit<7;++outfit) {
        struct jc_pose_receiver r={0};s.appearance[2]=outfit;
        assert(jc_pose_encode(build,&s,mac,NULL,packet));
        assert(!jc_pose_receive(&r,build,packet,sizeof(packet),2,1000,mac,NULL));
        assert(!memcmp(s.appearance,r.sample.appearance,sizeof(s.appearance)));
    }
    const uint32_t bad[]={25,4097,7,21};
    for (unsigned i=0;i<4;++i) {
        struct jc_pose_receiver r={0};assert(jc_pose_encode(build,&s,mac,NULL,packet));
        jc_session_put(packet+520+i*4,bad[i],4);
        assert(mac(NULL,packet,JC_POSE_BODY,packet+JC_POSE_BODY));
        assert(jc_pose_receive(&r,build,packet,sizeof(packet),2,1000,mac,NULL)==7 && !r.paired);
    }
    s.flags=0;assert(!jc_pose_encode(build,&s,mac,NULL,packet));
    memset(s.appearance,0,sizeof(s.appearance));assert(jc_pose_encode(build,&s,mac,NULL,packet));
    struct jc_pose_receiver r={0};assert(!jc_pose_receive(&r,build,packet,sizeof(packet),2,1000,mac,NULL));
    assert(!(r.sample.flags&JC_POSE_APPEARANCE));
    memcpy(packet,"JCP4",4);r=(struct jc_pose_receiver){0};
    assert(jc_pose_receive(&r,build,packet,sizeof(packet),2,1000,mac,NULL)==3);
}
int main(void) {
    unsigned char build[16]={1},packet[JC_POSE_BYTES],other[JC_POSE_BYTES],tag[32];
    /* Published HMAC-SHA256 quick-brown-fox vector; zero-padding a three-byte key is equivalent. */
    memcpy(key,"key",3);
    static const unsigned char expected[]={0xf7,0xbc,0x83,0xf4,0x30,0x53,0x84,0x24,0xb1,0x32,0x98,0xe6,0xaa,0x6f,0xb1,0x43,0xef,0x4d,0x59,0xa1,0x49,0x46,0x17,0x59,0x97,0x47,0x9d,0xbc,0x2d,0x1a,0x3c,0xd8};
    const unsigned char message[]="The quick brown fox jumps over the lazy dog";
    assert(mac(NULL,message,sizeof(message)-1,tag) && !memcmp(tag,expected,32));
    duplex_policy_tests(build);
    animation_packets(build); appearance_packets(build);
    struct jc_pose_sample sample={.identity={2},.sequence=1,.chapter=2,.transform={1,0,0,0,0,1,0,0,0,0,1,0,30,40,50,1}};
    assert(jc_pose_encode(build,&sample,mac,NULL,packet)); socket_roundtrip(packet,build);
    struct jc_pose_receiver receiver={0},saved;
    assert(!jc_pose_receive(&receiver,build,packet,sizeof(packet),2,1000,mac,NULL));
    assert(jc_pose_fresh(&receiver,2,5999) && !jc_pose_fresh(&receiver,2,6000));
    assert(!jc_pose_fresh(&receiver,3,1001) && !jc_pose_fresh(&receiver,2,999));
    assert(jc_pose_receive(&receiver,build,packet,sizeof(packet),2,1001,mac,NULL)==6);
    sample.sequence=2; sample.identity[0]=3; assert(jc_pose_encode(build,&sample,mac,NULL,other));
    assert(jc_pose_receive(&receiver,build,other,sizeof(other),2,1002,mac,NULL)==5);
    sample.identity[0]=2; sample.chapter=3; assert(jc_pose_encode(build,&sample,mac,NULL,other));
    assert(jc_pose_receive(&receiver,build,other,sizeof(other),2,1003,mac,NULL)==8 && !receiver.ready);
    sample.sequence=3; sample.chapter=2; assert(jc_pose_encode(build,&sample,mac,NULL,other));
    assert(!jc_pose_receive(&receiver,build,other,sizeof(other),2,1004,mac,NULL) && receiver.ready);
    saved=receiver; other[120]^=1;
    assert(jc_pose_receive(&receiver,build,other,sizeof(other),2,1005,mac,NULL)==4 && !memcmp(&receiver.sample,&saved.sample,sizeof(saved.sample)));
    assert(jc_pose_receive(&receiver,build,packet,sizeof(packet),2,900,mac,NULL)==1);
    for (unsigned n=0;n<JC_POSE_BYTES;++n) {
        struct jc_pose_receiver r={0}; assert(jc_pose_receive(&r,build,packet,n,2,1000,mac,NULL)==3 && !r.paired);
    }
    for (unsigned n=0;n<10000;++n) {
        struct jc_pose_receiver r={0}; memcpy(other,packet,sizeof(other)); other[n%JC_POSE_BYTES]^=(unsigned char)(1u<<(n%8));
        assert(jc_pose_receive(&r,build,other,sizeof(other),2,1000,mac,NULL)!=0 && !r.paired);
    }
    for (unsigned i=0;i<16;++i) {
        struct jc_pose_receiver r={0}; memcpy(other,packet,sizeof(other)); jc_session_put(other+56+4*i,0x7fc00000u,4);
        assert(mac(NULL,other,JC_POSE_BODY,other+JC_POSE_BODY));
        assert(jc_pose_receive(&r,build,other,sizeof(other),2,1000,mac,NULL)==7 && !r.paired);
    }
    memset(&receiver,0,sizeof(receiver));
    for (unsigned n=0;n<32;++n) assert(jc_pose_receive(&receiver,build,NULL,0,2,1000,mac,NULL)==3);
    assert(jc_pose_receive(&receiver,build,packet,sizeof(packet),2,1000,mac,NULL)==2);
    assert(!jc_pose_receive(&receiver,build,packet,sizeof(packet),2,2000,mac,NULL));
    sample.transform[15]=0; assert(!jc_pose_encode(build,&sample,mac,NULL,other));
#ifdef JC_NO_SOCKET_TESTS
    puts("UDP loopback omitted; authenticated codec and policy tests ran without sockets.");
#endif
    puts("Pose link: HMAC known answer, per-player appearance, packet validation, identity/replay/chapter/staleness/rate, truncation, 10000 mutations and authenticated non-finite rejection passed. Simulated endpoints, not Journey gameplay.");
    return 0;
}
