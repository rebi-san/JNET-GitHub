#ifndef JNET_FLOWER_NET_H
#define JNET_FLOWER_NET_H
/* Shares planted Flower Trail flowers with the other players in the lobby, so everyone sees every player's trail. Modelled on the rocket
   relay (jnet_rocket_live.h): lobby-room and nonce bound packets, a guest sends to the host and the host fans out, with sequence replay,
   epoch/chapter and finite-value checks. Only plantings travel; each player's own touches and chirps open the flowers on their screen.
   Needs jnet_flower_trail_state/jnet_flower_lock/jnet_flower_hidden from the including file (jnet_flower_render.h in the DLL). */
#include "jnet_flower_trail.h"
#define JNET_FLOWER_PACKET 280u
#define JNET_FLOWER_BATCH 8u
#define JNET_FLOWER_QUEUE 64u
#define JNET_FLOWER_FLUSH_MS 120u
#define JNET_FLOWER_MIN_GAP_MS 50u
struct jnet_flower_wire {float position[3],yaw,scale;unsigned type,chapter;uint64_t epoch;};
static SRWLOCK jnet_flower_net_lock=SRWLOCK_INIT;
static struct {struct jnet_flower_wire out[JNET_FLOWER_QUEUE],in[JNET_FLOWER_QUEUE];unsigned out_count,in_count;} jnet_flower_net;
static uint64_t jnet_flower_net_generation,jnet_flower_net_room,jnet_flower_net_sequence,jnet_flower_net_flush;
static uint64_t jnet_flower_net_received[7],jnet_flower_net_ids[7],jnet_flower_net_leases[7],jnet_flower_net_last[7];

static int jnet_flower_wire_valid(const struct jnet_flower_wire *w) {
    for(unsigned i=0;i<3;++i)if(!isfinite(w->position[i]) || fabsf(w->position[i])>100000.f)return 0;
    return isfinite(w->yaw) && fabsf(w->yaw)<=100.f && isfinite(w->scale) && w->scale>=.5f && w->scale<=2.f &&
        w->type<JNET_FLOWER_TYPES && w->chapter<12;
}
/* Game thread: a flower this player just planted, to be sent at the next flush. */
static void jnet_flower_net_queue(const struct jnet_flower_item *item) {
    if(!jnet_sl.room)return;
    struct jnet_flower_wire wire={.yaw=item->yaw,.scale=item->scale,.type=item->type,.chapter=jnet_current_level,.epoch=jnet_travel_epoch};
    memcpy(wire.position,item->position,sizeof(wire.position));
    AcquireSRWLockExclusive(&jnet_flower_net_lock);
    if(jnet_flower_net.out_count<JNET_FLOWER_QUEUE)jnet_flower_net.out[jnet_flower_net.out_count++]=wire;
    ReleaseSRWLockExclusive(&jnet_flower_net_lock);
}
static void jnet_flower_pack(unsigned char p[JNET_FLOWER_PACKET],const struct jnet_flower_wire *wires,unsigned count,
        uint64_t source,uint64_t lease,uint64_t sequence,uint64_t epoch,unsigned chapter) {
    memset(p,0,JNET_FLOWER_PACKET);memcpy(p,"JNF1",4);p[4]=(unsigned char)count;
    jc_packet_put(p+8,jnet_sl.room,8);memcpy(p+16,jnet_sl.nonce,32);
    jc_packet_put(p+48,source,8);jc_packet_put(p+56,lease,8);jc_packet_put(p+64,sequence,8);jc_packet_put(p+72,epoch,8);
    jc_packet_put(p+80,chapter,4);
    for(unsigned i=0;i<count;++i){
        float values[5]={wires[i].position[0],wires[i].position[1],wires[i].position[2],wires[i].yaw,wires[i].scale};
        jnet_sg_floats(p+88+24*i,values,5);jc_packet_put(p+88+24*i+20,wires[i].type,4);
    }
}
static void jnet_flower_net_reset(void) {
    memset(jnet_flower_net_received,0,sizeof(jnet_flower_net_received));memset(jnet_flower_net_ids,0,sizeof(jnet_flower_net_ids));
    memset(jnet_flower_net_leases,0,sizeof(jnet_flower_net_leases));memset(jnet_flower_net_last,0,sizeof(jnet_flower_net_last));
    AcquireSRWLockExclusive(&jnet_flower_net_lock);memset(&jnet_flower_net,0,sizeof(jnet_flower_net));ReleaseSRWLockExclusive(&jnet_flower_net_lock);
}
/* Transport poll: send queued plantings, up to eight to a packet, at most every JNET_FLOWER_FLUSH_MS. */
static void jnet_flower_poll_live(void) {
    if(jnet_flower_net_room!=jnet_sl.room || jnet_flower_net_generation!=jnet_sl.generation){
        jnet_flower_net_room=jnet_sl.room;jnet_flower_net_generation=jnet_sl.generation;jnet_flower_net_reset();
    }
    if(!jnet_sl.room || jnet_travel_busy)return;
    uint64_t now=GetTickCount64();
    if(now-jnet_flower_net_flush<JNET_FLOWER_FLUSH_MS)return;
    struct jnet_flower_wire wires[JNET_FLOWER_QUEUE];unsigned count;
    AcquireSRWLockExclusive(&jnet_flower_net_lock);count=jnet_flower_net.out_count;
    memcpy(wires,jnet_flower_net.out,count*sizeof(wires[0]));jnet_flower_net.out_count=0;ReleaseSRWLockExclusive(&jnet_flower_net_lock);
    if(!count)return;
    jnet_flower_net_flush=now;
    uint64_t lease=jnet_sg.local.lease;
    if(!lease)return;
    for(unsigned at=0;at<count;){
        struct jnet_flower_wire batch[JNET_FLOWER_BATCH];unsigned n=0;
        while(at<count && n<JNET_FLOWER_BATCH){
            if(wires[at].chapter==jnet_current_level && wires[at].epoch==jnet_travel_epoch)batch[n++]=wires[at];
            ++at;
        }
        if(!n)continue;
        unsigned char packet[JNET_FLOWER_PACKET];
        jnet_flower_pack(packet,batch,n,jnet_sl.identity,lease,++jnet_flower_net_sequence,jnet_travel_epoch,jnet_current_level);
        for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].connected && jnet_sn.peers[i].admitted)
            jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),9);
    }
}
/* Transport receive: returns 1 when the message was ours (valid or not), 0 to let the next handler look at it. */
static int jnet_flower_message(unsigned slot,const unsigned char *p,size_t bytes) {
    if(bytes<4 || memcmp(p,"JNF1",4))return 0;
    if(bytes!=JNET_FLOWER_PACKET || slot>=7 || !jnet_sl.room || jnet_travel_busy ||
       !jnet_sn.peers[slot].admitted || !jnet_sn.peers[slot].connected || !jnet_sn_member(jnet_sn.peers[slot].identity) ||
       jc_packet_get(p+8,8)!=jnet_sl.room || memcmp(p+16,jnet_sl.nonce,32))return 1;
    uint64_t source=jc_packet_get(p+48,8),lease=jc_packet_get(p+56,8),sequence=jc_packet_get(p+64,8),epoch=jc_packet_get(p+72,8);
    unsigned chapter=(unsigned)jc_packet_get(p+80,4),count=p[4];
    int host=jnet_sl.host==jnet_sl.identity;
    if(!source || !lease || !sequence || source==jnet_sl.identity || !jnet_sn_member(source) ||
       (host?source!=jnet_sn.peers[slot].identity:jnet_sn.peers[slot].identity!=jnet_sl.host) ||
       epoch!=jnet_travel_epoch || chapter!=jnet_current_level || !count || count>JNET_FLOWER_BATCH)return 1;
    struct jnet_flower_wire wires[JNET_FLOWER_BATCH];
    for(unsigned i=0;i<count;++i){
        float values[5];jnet_sg_read_floats(values,p+88+24*i,5);
        wires[i]=(struct jnet_flower_wire){.position={values[0],values[1],values[2]},.yaw=values[3],.scale=values[4],
            .type=(unsigned)jc_packet_get(p+88+24*i+20,4),.chapter=chapter,.epoch=epoch};
        if(!jnet_flower_wire_valid(&wires[i]))return 1;
    }
    if(host){
        if(jnet_sg.peers[slot].id!=source)return 1;
        lease=jnet_sg.peers[slot].lease;
    }
    unsigned at=0;
    for(;at<7;++at)if(jnet_flower_net_ids[at]==source)break;
    if(at==7)for(at=0;at<7;++at)if(!jnet_flower_net_ids[at] || !jnet_sn_member(jnet_flower_net_ids[at]))break;
    if(at==7)return 1;
    if(jnet_flower_net_ids[at]!=source || jnet_flower_net_leases[at]!=lease){
        jnet_flower_net_ids[at]=source;jnet_flower_net_leases[at]=lease;jnet_flower_net_received[at]=jnet_flower_net_last[at]=0;
    }
    if(sequence<=jnet_flower_net_received[at])return 1;
    jnet_flower_net_received[at]=sequence;
    uint64_t now=GetTickCount64();
    if(jnet_flower_net_last[at] && now-jnet_flower_net_last[at]<JNET_FLOWER_MIN_GAP_MS)return 1;
    jnet_flower_net_last[at]=now;
    if(host)for(unsigned i=0;i<7;++i)if(i!=slot && jnet_sn.peers[i].connected && jnet_sn.peers[i].admitted)
        jnet_sn_transmit(jnet_sn.peers[i].handle,p,JNET_FLOWER_PACKET,9);
    AcquireSRWLockExclusive(&jnet_flower_net_lock);
    for(unsigned i=0;i<count && jnet_flower_net.in_count<JNET_FLOWER_QUEUE;++i)jnet_flower_net.in[jnet_flower_net.in_count++]=wires[i];
    ReleaseSRWLockExclusive(&jnet_flower_net_lock);
    return 1;
}
/* Game thread: plants what the other players sent into this player's trail (the same list, so touches and chirps open them too). */
static unsigned jnet_flower_net_drain(uint32_t now) {
    struct jnet_flower_wire wires[JNET_FLOWER_QUEUE];unsigned count;
    AcquireSRWLockExclusive(&jnet_flower_net_lock);count=jnet_flower_net.in_count;
    memcpy(wires,jnet_flower_net.in,count*sizeof(wires[0]));jnet_flower_net.in_count=0;ReleaseSRWLockExclusive(&jnet_flower_net_lock);
    if(!count)return 0;
    unsigned added=0;
    AcquireSRWLockExclusive(&jnet_flower_lock);
    if(jnet_flower_trail_state.level!=jnet_current_level){
        jnet_flower_trail_clear(&jnet_flower_trail_state);jnet_flower_trail_state.level=jnet_current_level;
        memset(jnet_flower_hidden,0,sizeof(jnet_flower_hidden));
    }
    for(unsigned i=0;i<count;++i){
        if(wires[i].chapter!=jnet_current_level || wires[i].epoch!=jnet_travel_epoch)continue;
        unsigned slot=jnet_flower_trail_state.next;
        jnet_flower_trail_add(&jnet_flower_trail_state,wires[i].position,wires[i].type,wires[i].yaw,wires[i].scale,now);
        jnet_flower_hidden[slot]=0;++added;
    }
    ReleaseSRWLockExclusive(&jnet_flower_lock);
    return added;
}
#endif
