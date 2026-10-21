#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "companion_actor_packet.h"
/* Just the lobby/transport state the flower sync touches, so this runs anywhere (the full lobby fixture needs a POSIX target). */
typedef struct {int locked;} SRWLOCK;
#define SRWLOCK_INIT {0}
static void AcquireSRWLockExclusive(SRWLOCK *l){assert(!l->locked);l->locked=1;}
static void ReleaseSRWLockExclusive(SRWLOCK *l){assert(l->locked);l->locked=0;}
static uint64_t clock_ms;
static uint64_t GetTickCount64(void){return clock_ms;}
static struct {uint64_t room,identity,host,generation;unsigned char nonce[32];} jnet_sl={.nonce={1,2,3}};
static struct {struct {unsigned connected,admitted;uint32_t handle;uint64_t identity;} peers[7];} jnet_sn;
static struct {struct {uint64_t lease;} local;struct {uint64_t id,lease;} peers[7];} jnet_sg;
static struct {unsigned count;uint64_t ids[8];} jnet_sn_members;
static unsigned jnet_travel_busy,jnet_current_level;static uint64_t jnet_travel_epoch;
static int jnet_sn_member(uint64_t id){for(unsigned i=0;i<jnet_sn_members.count;++i)if(jnet_sn_members.ids[i]==id)return 1;return 0;}
static unsigned flower_sends;static unsigned char flower_sent[280];static uint32_t flower_handles[16];
static int jnet_sn_transmit(uint32_t handle,const void *data,uint32_t bytes,int flags){
    assert(bytes==280 && flags==9 && !memcmp(data,"JNF1",4));memcpy(flower_sent,data,bytes);
    if(flower_sends<16)flower_handles[flower_sends]=handle;++flower_sends;return 1;
}
static void jnet_sg_floats(unsigned char *p,const float *v,unsigned n){for(unsigned i=0;i<n;++i){uint32_t b;memcpy(&b,v+i,4);jc_packet_put(p+4*i,b,4);}}
static void jnet_sg_read_floats(float *v,const unsigned char *p,unsigned n){for(unsigned i=0;i<n;++i){uint32_t b=(uint32_t)jc_packet_get(p+4*i,4);memcpy(v+i,&b,4);}}
#include "jnet_flower_trail.h"
static struct jnet_flower_trail jnet_flower_trail_state;
static SRWLOCK jnet_flower_lock;
static unsigned char jnet_flower_hidden[JNET_FLOWER_CAPACITY];
#include "jnet_flower_net.h"

static struct jnet_flower_wire wire(float x,unsigned type) {
    return (struct jnet_flower_wire){.position={x,2,3},.yaw=1.f,.scale=1.f,.type=type,.chapter=1,.epoch=3};
}
static unsigned in_queue(void) {return jnet_flower_net.in_count;}
int main(void) {
    _Static_assert(88+24*JNET_FLOWER_BATCH<=JNET_FLOWER_PACKET,"eight flowers fit in a packet");
    clock_ms=10000;jnet_sl.room=1000;jnet_sl.host=jnet_sl.identity=42;jnet_sl.generation=1;
    jnet_travel_epoch=3;jnet_current_level=1;
    jnet_sn_members.count=3;jnet_sn_members.ids[0]=42;jnet_sn_members.ids[1]=43;jnet_sn_members.ids[2]=44;
    for(unsigned i=0;i<2;++i){
        jnet_sn.peers[i].identity=43+i;jnet_sn.peers[i].admitted=jnet_sn.peers[i].connected=1;jnet_sn.peers[i].handle=100+i;
        jnet_sg.peers[i].id=43+i;jnet_sg.peers[i].lease=20+i;
    }
    jnet_sg.local.lease=5;
    jnet_flower_poll_live();
    /* Outgoing: a planting is queued only inside a lobby, sent after the flush interval to every admitted peer, eight to a packet. */
    struct jnet_flower_item item={{1,2,3},.5f,1.f,2,0,0};
    jnet_sl.room=0;jnet_flower_net_queue(&item);assert(!jnet_flower_net.out_count);jnet_sl.room=1000;
    for(unsigned i=0;i<10;++i){item.position[0]=(float)i;jnet_flower_net_queue(&item);}
    assert(jnet_flower_net.out_count==10);
    clock_ms+=JNET_FLOWER_FLUSH_MS;jnet_flower_poll_live();
    assert(flower_sends==4 && !jnet_flower_net.out_count);
    assert(!memcmp(flower_sent,"JNF1",4) && jc_packet_get(flower_sent+48,8)==42 && jc_packet_get(flower_sent+56,8)==5 && jc_packet_get(flower_sent+64,8)==2 && flower_sent[4]==2);
    assert(flower_handles[0]==100 && flower_handles[1]==101);
    /* No send before the interval, none while travelling, none without a lease, and stale-epoch plantings are dropped. */
    jnet_flower_net_queue(&item);jnet_flower_poll_live();assert(flower_sends==4);
    clock_ms+=JNET_FLOWER_FLUSH_MS;jnet_travel_busy=1;jnet_flower_poll_live();assert(flower_sends==4);jnet_travel_busy=0;
    jnet_travel_epoch=4;jnet_flower_poll_live();assert(flower_sends==4);jnet_travel_epoch=3;
    jnet_flower_net_queue(&item);jnet_sg.local.lease=0;clock_ms+=JNET_FLOWER_FLUSH_MS;jnet_flower_poll_live();assert(flower_sends==4);jnet_sg.local.lease=5;
    /* Incoming at the host: validated, bound to the sending peer, queued and fanned out to the other peer only. */
    jnet_flower_net_reset();flower_sends=0;
    struct jnet_flower_wire wires[3]={wire(1,0),wire(2,3),wire(3,7)};
    unsigned char p[JNET_FLOWER_PACKET];
    jnet_flower_pack(p,wires,3,43,7,1,3,1);
    assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==3 && flower_sends==1 && flower_handles[0]==101);
    assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==3 && flower_sends==1);                       /* replay */
    clock_ms+=100;jnet_flower_pack(p,wires,3,44,7,2,3,1);
    assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==3);                                          /* source is not this peer */
    assert(jnet_flower_message(1,p,sizeof(p)) && in_queue()==6 && flower_sends==2);
    clock_ms+=100;jnet_flower_pack(p,wires,3,43,7,3,4,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);   /* wrong epoch */
    jnet_flower_pack(p,wires,3,43,7,3,3,2);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);                  /* wrong chapter */
    wires[1].type=JNET_FLOWER_TYPES;jnet_flower_pack(p,wires,3,43,7,3,3,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);wires[1].type=3;
    wires[0].position[1]=NAN;jnet_flower_pack(p,wires,3,43,7,3,3,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);wires[0].position[1]=2;
    wires[2].scale=9;jnet_flower_pack(p,wires,3,43,7,3,3,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);wires[2].scale=1;
    jnet_flower_pack(p,wires,3,43,7,3,3,1);p[4]=0;assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);
    p[4]=JNET_FLOWER_BATCH+1;assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);
    jnet_flower_pack(p,wires,3,43,7,3,3,1);p[16]^=1;assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==6);p[16]^=1;     /* wrong nonce */
    assert(jnet_flower_message(0,p,sizeof(p)-1) && in_queue()==6);                                                         /* wrong length */
    assert(!jnet_flower_message(0,(const unsigned char *)"JNR1",4) && !jnet_flower_message(0,(const unsigned char *)"JN",2));
    clock_ms+=100;jnet_flower_pack(p,wires,3,43,7,4,3,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==9 && flower_sends==3);
    clock_ms+=10;jnet_flower_pack(p,wires,3,43,7,5,3,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==9 && flower_sends==3);   /* under the minimum gap */
    jnet_sn.peers[0].admitted=0;clock_ms+=100;jnet_flower_pack(p,wires,3,43,7,6,3,1);assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==9);jnet_sn.peers[0].admitted=1;
    /* A guest accepts only the host's packets and does not relay. */
    jnet_sl.identity=43;jnet_sl.host=42;jnet_sn.peers[0].identity=42;flower_sends=0;jnet_flower_net_reset();
    clock_ms+=100;jnet_flower_pack(p,wires,3,44,7,1,3,1);
    assert(jnet_flower_message(1,p,sizeof(p)) && in_queue()==0);
    assert(jnet_flower_message(0,p,sizeof(p)) && in_queue()==3 && !flower_sends);
    jnet_sl.identity=jnet_sl.host=42;jnet_sn.peers[0].identity=43;
    /* A new lobby session clears everything queued. */
    jnet_sl.generation++;jnet_flower_poll_live();assert(!jnet_flower_net.in_count && !jnet_flower_net.out_count);
    /* Draining plants the received flowers into the trail (same list as local ones), starting untouched, and drops other levels. */
    jnet_flower_net.in[0]=wire(1,0);jnet_flower_net.in[1]=wire(2,5);jnet_flower_net.in[2]=wire(3,2);jnet_flower_net.in[2].chapter=2;
    jnet_flower_net.in[3]=wire(4,1);jnet_flower_net.in[3].epoch=9;jnet_flower_net.in_count=4;
    jnet_flower_trail_state.level=7;jnet_flower_trail_add(&jnet_flower_trail_state,(float[]){9,9,9},1,0,1.f,1);
    assert(jnet_flower_net_drain(5000)==2 && jnet_flower_trail_state.level==1 && jnet_flower_trail_state.count==2);
    assert(jnet_flower_trail_state.items[0].born==5000 && !jnet_flower_trail_state.items[0].touched && jnet_flower_trail_state.items[1].type==5);
    assert(jnet_flower_net_drain(5001)==0 && jnet_flower_trail_state.count==2);
    puts("Flower sync: batching, flush interval, lease/epoch/travel gates, host fanout, source binding, replay, gap, finite and range checks, guest relay rules, session reset and drain passed; fixture only.");
    return 0;
}
