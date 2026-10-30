#include "companion_script_peer.h"
#include "companion_chirp_packet.h"
#include <CommonCrypto/CommonHMAC.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define SOCKET int
#define LONG int32_t
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define WSAEWOULDBLOCK EAGAIN
#define WSAEMSGSIZE EMSGSIZE
#define WSAECONNRESET ECONNRESET
#define __declspec(x)
static unsigned traveler_network_active=1;
static uint64_t ticks;
static unsigned current,chirp_pending[3];
static unsigned char jc_game_hash[32]={14,1};
static struct jc_actor_sender sources[3];
static struct jc_script_peer traveler_streams[2];
struct jc_stream_udp {unsigned char key[32];struct jc_chirp_inbox chirps;};
static struct jc_stream_udp traveler_udp[2];
typedef struct {int64_t QuadPart;} LARGE_INTEGER;
static int QueryPerformanceFrequency(LARGE_INTEGER *v){v->QuadPart=1000000;return 1;}
static int QueryPerformanceCounter(LARGE_INTEGER *v){v->QuadPart=(int64_t)ticks*1000;return 1;}
static uint64_t GetTickCount64(void){return ticks;}
static uintptr_t GetModuleHandleW(void *p){(void)p;return 0;}
static int WSAGetLastError(void){return errno;}
static int traveler_udp_mac(void *p,const unsigned char *data,size_t size,unsigned char tag[32]) {
    CCHmac(kCCHmacAlgSHA256,((struct jc_stream_udp *)p)->key,32,data,size,tag);return 1;
}
static int fixture_receive(int s,char *p,int cap,int flags,struct sockaddr *address,int *size) {
    socklen_t length=(socklen_t)*size;int result=(int)recvfrom(s,p,(size_t)cap,flags,address,&length);*size=(int)length;return result;
}
static unsigned sent_kinds[3];
static ssize_t fixture_sendto(int socket,const void *packet,size_t bytes,int flags,const struct sockaddr *address,socklen_t length) {
    unsigned kind=bytes==112?0:bytes==256?1:2;
    assert(bytes==112 || bytes==256 || bytes==120);
    assert(!memcmp(packet,kind==0?"JNH1":kind==1?"JNA1":"JNC1",4));++sent_kinds[kind];
    return sendto(socket,packet,bytes,flags,address,length);
}
static int capture(void *read,void *ctx,uintptr_t image,uintptr_t game,struct jc_actor_sender *sample) {
    (void)read;(void)ctx;(void)image;assert(game==current+1);*sample=sources[current];return 0;
}
static int capture_chirp(void *read,void *ctx,uintptr_t game,uint64_t frame,uint64_t peer,uint64_t epoch,
        struct jc_chirp_capture *state,struct jc_chirp_batch *batch) {
    (void)read;(void)ctx;(void)game;(void)frame;memset(batch,0,sizeof(*batch));
    if(chirp_pending[current]) {
        chirp_pending[current]=0;batch->count=1;
        batch->source[0].event=(struct jc_chirp_event){.participant=peer,.epoch=epoch,
            .sequence=++state->sequence,.sound=3,.radius=4,.duration=.5f};
    }
    return 0;
}
#define recvfrom fixture_receive
#define sendto fixture_sendto
#define own_read NULL
#define jc_actor_capture_sender capture
#define jc_chirp_capture_local capture_chirp
#define JNET_LOBBY_ENABLED 1
static unsigned membership[3]={14,14,14};
static int jnet_lobby_allows(uint64_t peer){return (membership[current]&(1u<<peer))!=0;}
static int (*jnet_travel_peer_wait)(uint64_t,uint64_t);
static uint64_t travel_hold_until;
static int travel_hold(uint64_t peer,uint64_t now){return peer==2 && now<travel_hold_until;}
#include "network_fixture.h"
static __typeof__(traveler_network) clients[3];
static struct jc_stream_udp cryptos[3][2];
static struct jc_script_peer streams[3][2];
static void enter(unsigned client) {
    current=client;traveler_network=clients[client];
    memcpy(traveler_udp,cryptos[client],sizeof(traveler_udp));
    memcpy(traveler_streams,streams[client],sizeof(traveler_streams));
}
static void leave(void) {
    clients[current]=traveler_network;
    memcpy(cryptos[current],traveler_udp,sizeof(traveler_udp));
    memcpy(streams[current],traveler_streams,sizeof(traveler_streams));
}
static void rounds(unsigned count,unsigned clients_count) {
    for(unsigned n=0;n<count;++n) {
        usleep(1000);ticks+=50;
        for(unsigned i=0;i<clients_count;++i){enter(i);traveler_network_tick(i+1,ticks);leave();}
    }
}
static unsigned slot_for(unsigned client,unsigned peer) {
    for(unsigned s=0;s<2;++s)if(clients[client].peers[s].connection.peer==peer+1)return s;
    assert(0);return 0;
}
static int sample(unsigned client,unsigned peer,int first,struct jc_actor_sender *out) {
    enter(client);int code=traveler_network_sample(slot_for(client,peer),first,out);leave();return code;
}
static void configurations(void) {
    unsigned char config[113],before[112],bad[112];
    for(unsigned id=1;id<=3;++id)for(unsigned swap=0;swap<2;++swap)for(unsigned legacy=0;legacy<2;++legacy) {
        memset(config,0,sizeof(config));memcpy(config,legacy?"JCN1":"JNN1",4);
        jc_packet_put(config+4,43840+id,4);jc_packet_put(config+8,id,8);
        for(unsigned peer=1,slot=0;peer<=3;++peer)if(peer!=id) {
            unsigned char *record=config+16+(slot++^swap)*48;
            jc_packet_put(record,peer,8);jc_packet_put(record+8,43840+peer,8);record[16]=(unsigned char)(id+peer);
        }
        memcpy(before,config,112);assert(traveler_network_config_valid(config,112));
        assert(!memcmp(config,before,112));
        assert(!traveler_network_config_valid(NULL,112));
        for(size_t bytes=0;bytes<112;++bytes)assert(!traveler_network_config_valid(config,bytes));
        assert(!traveler_network_config_valid(config,113));
        for(unsigned i=0;i<4;++i) {
            memcpy(bad,config,112);bad[i]='X';assert(!traveler_network_config_valid(bad,112));
        }
        memcpy(bad,config,112);jc_packet_put(bad+4,43840+id+1,4);assert(!traveler_network_config_valid(bad,112));
        memcpy(bad,config,112);jc_packet_put(bad+8,0,8);assert(!traveler_network_config_valid(bad,112));
        memcpy(bad,config,112);jc_packet_put(bad+8,4,8);assert(!traveler_network_config_valid(bad,112));
        for(unsigned slot=0;slot<2;++slot) {
            unsigned at=16+slot*48;
            memcpy(bad,config,112);jc_packet_put(bad+at,id,8);assert(!traveler_network_config_valid(bad,112));
            memcpy(bad,config,112);jc_packet_put(bad+at,0,8);assert(!traveler_network_config_valid(bad,112));
            memcpy(bad,config,112);jc_packet_put(bad+at,4,8);assert(!traveler_network_config_valid(bad,112));
            memcpy(bad,config,112);jc_packet_put(bad+at+8,0,8);assert(!traveler_network_config_valid(bad,112));
            memcpy(bad,config,112);memset(bad+at+16,0,32);assert(!traveler_network_config_valid(bad,112));
        }
        memcpy(bad,config,112);memcpy(bad+64,bad+16,48);assert(!traveler_network_config_valid(bad,112));
    }
    puts("Peer config: JNN1 and legacy files, all participant/order combinations, unchanged input, truncation, invalid identities/ports/keys and duplicate peers passed.");
}
int main(int argc,char **argv) {
    configurations();
    if(argc==2 && !strcmp(argv[1],"--config-only"))return 0;
    assert(argc==1);
    uint16_t ports[3];
    for(unsigned i=0;i<3;++i) {
        clients[i].socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);assert(clients[i].socket>=0);
        assert(!fcntl(clients[i].socket,F_SETFL,O_NONBLOCK));
        struct sockaddr_in address={.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
        assert(!bind(clients[i].socket,(const struct sockaddr *)&address,sizeof(address)));
        socklen_t length=sizeof(address);assert(!getsockname(clients[i].socket,(struct sockaddr *)&address,&length));ports[i]=ntohs(address.sin_port);
        clients[i].identity=i+1;clients[i].epoch=100+i;
        sources[i]=(struct jc_actor_sender){.body_scale=.1f,.chirp_shape={1,1},.chapter=2};
        sources[i].axes[0]=(float)i*.2f;sources[i].direction[2]=1;
        jc_receiver_store(sources[i].state.bytes,10.f+i);
    }
    for(unsigned i=0;i<3;++i)for(unsigned j=0,s=0;j<3;++j)if(i!=j) {
        struct jc_network_peer *p=&clients[i].peers[s++];
        p->connection=(struct jc_connection){.local=i+1,.peer=j+1,.epoch=100+i};
        p->connection.nonce[0]=(unsigned char)(10+i*3+j);p->port=ports[j];
        p->authentication.key[0]=(unsigned char)(1+i+j);
    }
    struct jc_actor_sender out;
    assert(sample(0,1,1,&out)==JC_STREAM_HELD);
    rounds(3,2);
    assert(clients[0].peers[slot_for(0,1)].connection.admitted);
    chirp_pending[1]=1;rounds(3,2);
    assert(clients[0].peers[slot_for(0,1)].wire_chirps.pending);
    assert(!sample(0,1,1,&out));assert(!memcmp(&out,&sources[1],sizeof(out)));
    assert(cryptos[0][slot_for(0,1)].chirps.pending);
    cryptos[0][slot_for(0,1)].chirps.pending=0;
    assert(!sample(1,0,1,&out));assert(!memcmp(&out,&sources[0],sizeof(out)));
    assert(sample(0,2,1,&out)==JC_STREAM_HELD);
    sources[1].axes[0]=.8f;sources[1].direction[0]=-.5f;jc_receiver_store(sources[1].state.bytes,44.f);
    rounds(3,2);assert(!sample(0,1,0,&out));assert(!memcmp(&out,&sources[1],sizeof(out)));
    rounds(30,3);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(i!=j) {
        unsigned slot=slot_for(i,j);
        assert(clients[i].peers[slot].connection.admitted);
        assert(!sample(i,j,streams[i][slot].stream.accepted==0,&out));
    }
    chirp_pending[1]=1;rounds(3,3);
    unsigned slot=slot_for(0,1);
    assert(cryptos[0][slot].chirps.pending && cryptos[0][slot].chirps.accepted==2);
    assert(clients[0].peers[slot].wire_chirps.rejected>=1);
    uint64_t appearance_generation=streams[0][slot].stream.generation;
    for(unsigned robe=0;robe<7;++robe){
        sources[1].state.bytes[0x60]=(unsigned char)robe;
        sources[1].state.bytes[0x58]=sources[1].state.bytes[0x5c]=30;
        rounds(2,3);assert(!sample(0,1,0,&out));
        assert(out.state.bytes[0x60]==robe && streams[0][slot].stream.generation==appearance_generation);
    }
    clients[1].paused=1;rounds(45,3);
    assert(sample(0,1,0,&out)==JC_STREAM_TIMEOUT);
    traveler_network_external=1;jnet_travel_peer_wait=travel_hold;travel_hold_until=ticks+100;
    assert(sample(0,1,0,&out)==JC_STREAM_HELD);
    JNETNetwork[23]=1;assert(sample(0,1,0,&out)==JC_STREAM_TIMEOUT);JNETNetwork[23]=0;
    ticks=travel_hold_until;assert(sample(0,1,0,&out)==JC_STREAM_TIMEOUT);
    traveler_network_external=0;jnet_travel_peer_wait=NULL;
    assert(!sample(0,2,0,&out));assert(!sample(2,0,0,&out));
    uint64_t generation=streams[0][slot].stream.generation;
    clients[1].paused=0;rounds(3,3);
    assert(!sample(0,1,1,&out));assert(streams[0][slot].stream.generation>generation);
    assert(!memcmp(&out,&sources[1],sizeof(out)));
    uint64_t prior_chirp=clients[0].peers[slot].wire_chirps.sequence;
    chirp_pending[1]=1;rounds(3,3);
    assert(cryptos[0][slot].chirps.pending);
    assert(clients[0].peers[slot].wire_chirps.sequence==prior_chirp+1);
    for(unsigned cycle=0;cycle<3;++cycle) {
        cryptos[0][slot].chirps.pending=0;
        clients[1].paused=1;rounds(45,3);
        assert(sample(0,1,0,&out)==JC_STREAM_TIMEOUT);
        streams[0][slot].stream.departing=1;
        prior_chirp=clients[0].peers[slot].wire_chirps.sequence;
        clients[1].paused=0;rounds(3,3);
        chirp_pending[1]=1;rounds(2,3);
        assert(clients[0].peers[slot].wire_chirps.pending);
        assert(!sample(0,1,1,&out));
        assert(cryptos[0][slot].chirps.pending && cryptos[0][slot].chirps.event.sequence==prior_chirp+1);
    }
    uint64_t other_sequence=clients[0].peers[slot_for(0,2)].connection.sequence;
    ++clients[1].epoch;clients[1].sequence=0;
    for(unsigned s=0;s<2;++s) {
        struct jc_network_peer *p=&clients[1].peers[s];uint64_t peer=p->connection.peer;
        p->connection=(struct jc_connection){.local=2,.peer=peer,.epoch=clients[1].epoch,.nonce={99}};
    }
    rounds(30,3);
    assert(sample(0,1,0,&out)==JC_STREAM_TIMEOUT);
    assert(!sample(0,1,1,&out));assert(!memcmp(&out,&sources[1],sizeof(out)));
    assert(clients[0].peers[slot_for(0,2)].connection.sequence>other_sequence);
    enter(0);traveler_network.next_send=ticks;
    uint64_t serial=traveler_network.sequence;
    traveler_network_tick(1,ticks);assert(traveler_network.sequence==serial+1);
    ticks+=17;traveler_network_tick(1,ticks);assert(traveler_network.sequence==serial+1);
    ticks+=16;traveler_network_tick(1,ticks);assert(traveler_network.sequence==serial+2);
    ticks+=1000;traveler_network_tick(1,ticks);assert(traveler_network.sequence==serial+3);
    leave();
    membership[0]&=~4u;
    uint64_t rejected_sequence=clients[0].peers[slot_for(0,1)].connection.sequence;
    rounds(4,3);
    assert(clients[0].peers[slot_for(0,1)].connection.sequence==rejected_sequence);
    assert(sample(0,1,0,&out)==JC_STREAM_TIMEOUT);
    membership[0]|=4u;rounds(4,3);
    assert(clients[0].peers[slot_for(0,1)].connection.sequence>rejected_sequence);
    for(unsigned i=0;i<3;++i)close(clients[i].socket);
    assert(sent_kinds[0] && sent_kinds[1] && sent_kinds[2]);
    puts("Production UDP adapter: two then three endpoints, actual sample fidelity, chirp deduplication, first-admission chirp retention, four immediate post-reconnect chirps, pause/timeout, independent reconnect and process epoch replacement passed. Native game capture and actor rendering remain separate runtime checks.");
    return 0;
}
