#ifndef JNET_STEAM_GAME_H
#define JNET_STEAM_GAME_H
#include "companion_actor_packet.h"
#include "jnet_character.h"
#include "native_companion_chirp.h"
#define JNET_GAME_RECORD 272u
_Static_assert(JC_RECEIVER_STATE_BYTES==108,"Steam gameplay state wire layout");
#define JNET_GAME_BYTES (64u+JNET_STEAM_CAPACITY*JNET_GAME_RECORD)
struct jnet_game_record {
    uint64_t id,lease,sequence,seen,chirp_seen,processed;
    unsigned character,character_known,spotlight,rotation_known;float rotation[4];
    struct jc_actor_sender sample;
    struct jc_chirp_event chirp;
};
static struct {
    uint64_t room,identity,generation,sequence,received,next_send,last_send,lease_counter;
    struct jnet_game_record local,peers[7];
    uint32_t handles[7],sent_handles[7];
    uint64_t sent_sequences[7],roster_sent[7],roster_keys[7][JNET_STEAM_CAPACITY][3];
} jnet_sg;
static struct {
    uint64_t room,identity,generation,received;
    struct jnet_game_record outgoing,records[JNET_STEAM_CAPACITY],events[32];
    struct {struct jc_chirp_event event;uint64_t seen;} outgoing_events[32];
    unsigned event_count,outgoing_count;
} jnet_sg_mail;
static SRWLOCK jnet_sg_lock=SRWLOCK_INIT;
__declspec(dllexport) volatile LONG JNETSteamGame[99];
static void jnet_sg_reset(void) {
    uint64_t lease=jnet_sg.lease_counter;memset(&jnet_sg,0,sizeof(jnet_sg));jnet_sg.lease_counter=lease;
    AcquireSRWLockExclusive(&jnet_sg_lock);memset(&jnet_sg_mail,0,sizeof(jnet_sg_mail));ReleaseSRWLockExclusive(&jnet_sg_lock);
    JNETSteamGame[0]=0;
}
static void jnet_sg_floats(unsigned char *p,const float *values,unsigned count) {
    for(unsigned i=0;i<count;++i){uint32_t bits;memcpy(&bits,values+i,4);jc_packet_put(p+i*4,bits,4);}
}
static void jnet_sg_read_floats(float *values,const unsigned char *p,unsigned count) {
    for(unsigned i=0;i<count;++i){uint32_t bits=(uint32_t)jc_packet_get(p+i*4,4);memcpy(values+i,&bits,4);}
}
static void jnet_sg_encode_record(unsigned char *p,const struct jnet_game_record *r,uint64_t now) {
    memset(p,0,JNET_GAME_RECORD);if(!r->id || !r->lease)return;
    jc_packet_put(p,r->id,8);jc_packet_put(p+8,r->lease,8);
    if(!r->sequence || now<r->seen || now-r->seen>=2000)return;
    jc_packet_put(p+16,r->sequence,8);jc_packet_put(p+24,now-r->seen,4);jc_packet_put(p+28,r->sample.chapter,4);
    for(unsigned i=0;i<JC_RECEIVER_STATE_BYTES;i+=4){uint32_t bits;memcpy(&bits,r->sample.state.bytes+i,4);jc_packet_put(p+32+i,bits,4);}
    float values[16];memcpy(values,r->sample.axes,16);memcpy(values+4,r->sample.direction,16);memcpy(values+8,r->sample.scalars,8);
    values[10]=r->sample.body_scale;memcpy(values+11,r->sample.glow_targets,12);memcpy(values+14,r->sample.chirp_shape,8);jnet_sg_floats(p+140,values,16);
    if(r->character_known && r->character<JNET_CHARACTER_COUNT)jc_packet_put(p+252,(r->character+1)|((r->character==JNET_CHARACTER_WAR_MACHINE && r->spotlight)?0x100u:0)|(r->rotation_known?0x200u:0),4);
    if(r->rotation_known)jnet_sg_floats(p+256,r->rotation,4);
    if(r->chirp.sequence && now>=r->chirp_seen && now-r->chirp_seen<=500) {
        jc_packet_put(p+204,r->chirp.sequence,8);jc_packet_put(p+212,r->chirp.sound,4);
        float chirp[8];memcpy(chirp,r->chirp.position,12);memcpy(chirp+3,r->chirp.velocity,12);chirp[6]=r->chirp.radius;chirp[7]=r->chirp.duration;
        jnet_sg_floats(p+216,chirp,8);jc_packet_put(p+248,now-r->chirp_seen,4);
    }
}
static int jnet_sg_decode_record(struct jnet_game_record *r,const unsigned char *p,uint64_t now) {
    memset(r,0,sizeof(*r));r->processed=now;r->id=jc_packet_get(p,8);r->lease=jc_packet_get(p+8,8);r->sequence=jc_packet_get(p+16,8);
    unsigned character=(unsigned)jc_packet_get(p+252,4);
    if((character&~0x3ffu) || (character&255)>JNET_CHARACTER_COUNT || ((character&0x100u) && (character&255)!=JNET_CHARACTER_WAR_MACHINE+1))return 0;
    r->spotlight=!!(character&0x100u);r->rotation_known=!!(character&0x200u);character&=255;
    if(r->rotation_known){
        if(!character || !jnet_character_is_creature(character-1))return 0;
        jnet_sg_read_floats(r->rotation,p+256,4);float norm=0;
        for(unsigned i=0;i<4;++i){if(!isfinite(r->rotation[i]))return 0;norm+=r->rotation[i]*r->rotation[i];}
        if(!isfinite(norm) || norm<.99f || norm>1.01f)return 0;
    }
    if(character && (!r->id || !r->lease || !r->sequence))return 0;
    if(character){r->character=character-1;r->character_known=1;}
    if(!r->id)return !r->lease && !r->sequence;
    if(!r->lease)return 0;if(!r->sequence)return 1;
    uint64_t age=jc_packet_get(p+24,4);if(age>=2000 || age>now)return 0;r->seen=now-age;r->sample.chapter=(uint32_t)jc_packet_get(p+28,4);
    for(unsigned i=0;i<JC_RECEIVER_STATE_BYTES;i+=4){uint32_t bits=(uint32_t)jc_packet_get(p+32+i,4);memcpy(r->sample.state.bytes+i,&bits,4);}
    float values[16];jnet_sg_read_floats(values,p+140,16);memcpy(r->sample.axes,values,16);memcpy(r->sample.direction,values+4,16);memcpy(r->sample.scalars,values+8,8);
    r->sample.body_scale=values[10];memcpy(r->sample.glow_targets,values+11,12);memcpy(r->sample.chirp_shape,values+14,8);
    if(r->sample.chapter>=12 || !jc_actor_sender_valid(&r->sample))return 0;
    r->chirp.sequence=jc_packet_get(p+204,8);
    if(r->chirp.sequence) {
        age=jc_packet_get(p+248,4);if(age>500 || age>now)return 0;r->chirp_seen=now-age;
        r->chirp.participant=r->id;r->chirp.epoch=r->lease;r->chirp.sound=(uint32_t)jc_packet_get(p+212,4);
        float chirp[8];jnet_sg_read_floats(chirp,p+216,8);memcpy(r->chirp.position,chirp,12);memcpy(r->chirp.velocity,chirp+3,12);r->chirp.radius=chirp[6];r->chirp.duration=chirp[7];
        if(!jc_chirp_event_valid(&r->chirp))return 0;
    }
    return 1;
}
static inline int jnet_sg_war_target(const struct jnet_game_record *target,
        uint64_t attacker,unsigned chapter,uint64_t now,uint64_t max_age) {
    return target && attacker && chapter<12 && max_age && max_age<=2000 &&
        target->id && target->id!=attacker && target->lease && target->sequence &&
        target->character_known && target->character==JNET_CHARACTER_WAYFARER &&
        target->sample.chapter==chapter && now>=target->seen && now-target->seen<max_age;
}
static void jnet_sg_publish(const struct jnet_game_record *records,uint64_t now) {
    AcquireSRWLockExclusive(&jnet_sg_lock);
    memcpy(jnet_sg_mail.records,records,sizeof(jnet_sg_mail.records));jnet_sg_mail.received=now;
    ReleaseSRWLockExclusive(&jnet_sg_lock);
}
static void jnet_sg_event_send(const struct jnet_game_record *r,int host) {
    unsigned char packet[JNET_GAME_BYTES]={0};memcpy(packet,"JNG1",4);jc_packet_put(packet+56,jnet_travel_epoch,8);jc_packet_put(packet+4,host?4:3,4);
    jc_packet_put(packet+8,jnet_sl.room,8);memcpy(packet+16,jnet_sl.nonce,32);jc_packet_put(packet+48,++jnet_sg.sequence,8);
    jnet_sg_encode_record(packet+64,r,GetTickCount64());
    for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].admitted && jnet_sn.peers[i].connected && jnet_sn.peers[i].identity!=r->id) {
        int result=jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),9);
        if(result==1)++JNETSteamGame[1];else JNETSteamGame[4]=result;
    }
}
static void jnet_sg_event_publish(const struct jnet_game_record *r) {
    AcquireSRWLockExclusive(&jnet_sg_lock);
    if(jnet_sg_mail.event_count<32)jnet_sg_mail.events[jnet_sg_mail.event_count++]=*r;else ++JNETSteamGame[16];
    ReleaseSRWLockExclusive(&jnet_sg_lock);
}
static int jnet_sg_receive(unsigned slot,const unsigned char *p,size_t bytes) {
    if(bytes<4 || memcmp(p,"JNG1",4))return 0;
    unsigned kind=bytes>=8?(unsigned)jc_packet_get(p+4,4):0;int event=kind==3 || kind==4;
    int host=jnet_sl.host==jnet_sl.identity;uint64_t now=GetTickCount64(),seq=bytes>=64?jc_packet_get(p+48,8):0;
    unsigned reason=1;
    if(bytes!=JNET_GAME_BYTES || slot>=7)goto reject;
    reason=2;
    if(!jnet_sn.peers[slot].connected || !jnet_sn.peers[slot].admitted ||
        !jnet_sn_member(jnet_sn.peers[slot].identity))goto reject;
    reason=1;
    if(!jnet_sg.room || jnet_sg.room!=jnet_sl.room ||
        kind!=(host?(event?3u:1u):(event?4u:2u)) || jc_packet_get(p+8,8)!=jnet_sl.room || memcmp(p+16,jnet_sl.nonce,32) ||
        !seq)goto reject;
    if(jc_packet_get(p+56,8)!=jnet_travel_epoch || jnet_travel_busy){++JNETSteamGame[98];return 1;}
    reason=5;if(!host && !event && seq<=jnet_sg.received)goto reject;
    reason=3;
    struct jnet_game_record records[JNET_STEAM_CAPACITY];
    for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)if(!jnet_sg_decode_record(records+i,p+64+i*JNET_GAME_RECORD,now))goto reject;
    if(event) {
        struct jnet_game_record *r=records;
        if(!r->id || !r->sequence || !r->chirp.sequence || records[1].id || records[2].id || !jnet_sn_member(r->id))goto reject;
        reason=4;
        if(host) {
            if(r->id!=jnet_sn.peers[slot].identity || jnet_sg.handles[slot]!=jnet_sn.peers[slot].handle)goto reject;
            reason=6;if(r->chirp.sequence<=jnet_sg.peers[slot].chirp.sequence)goto reject;
            r->lease=jnet_sg.peers[slot].lease;r->chirp.epoch=r->lease;jnet_sg.peers[slot].chirp=r->chirp;
            jnet_sg_event_send(r,1);
        } else if(jnet_sn.peers[slot].identity!=jnet_sl.host || r->id==jnet_sl.identity)goto reject;
        jnet_sg_event_publish(r);++JNETSteamGame[2];return 1;
    }
    for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)if(records[i].chirp.sequence)goto reject;
    reason=4;
    if(host) {
        if(records[0].id!=jnet_sn.peers[slot].identity || records[1].id || records[2].id ||
            !records[0].sequence || jnet_sg.handles[slot]!=jnet_sn.peers[slot].handle)goto reject;
        reason=5;if(records[0].sequence<=jnet_sg.peers[slot].sequence)goto reject;
        jnet_sg.next_send=0;
        records[0].lease=jnet_sg.peers[slot].lease;records[0].chirp.epoch=records[0].lease;
        if(records[0].chirp.sequence<jnet_sg.peers[slot].chirp.sequence)records[0].chirp=jnet_sg.peers[slot].chirp;
        jnet_sg.peers[slot]=records[0];
    } else {
        if(jnet_sn.peers[slot].identity!=jnet_sl.host || records[0].id!=jnet_sl.host)goto reject;
        for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)if(records[i].id){
            if(!jnet_sn_member(records[i].id))goto reject;
            for(unsigned j=0;j<i;++j)if(records[i].id==records[j].id)goto reject;
        }
        jnet_sg.received=seq;jnet_sg_publish(records,now);
    }
    ++JNETSteamGame[2];return 1;
reject:
    ++JNETSteamGame[3];++JNETSteamGame[17+reason];return 1;
}
static void jnet_sg_tick(void) {
    if(jnet_travel_busy)return;
    uint64_t now=GetTickCount64();int host=jnet_sl.host==jnet_sl.identity;
    if(!jnet_sl.room)return;
    if(jnet_sg.room!=jnet_sl.room || jnet_sg.generation!=jnet_sl.generation) {
        jnet_sg_reset();jnet_sg.room=jnet_sl.room;jnet_sg.generation=jnet_sl.generation;jnet_sg.identity=jnet_sl.identity;
        AcquireSRWLockExclusive(&jnet_sg_lock);jnet_sg_mail.room=jnet_sl.room;jnet_sg_mail.identity=jnet_sl.identity;jnet_sg_mail.generation=jnet_sl.generation;ReleaseSRWLockExclusive(&jnet_sg_lock);
        jnet_sg.local.id=jnet_sl.identity;jnet_sg.local.lease=++jnet_sg.lease_counter;
    }
    struct jnet_game_record outgoing;
    AcquireSRWLockExclusive(&jnet_sg_lock);outgoing=jnet_sg_mail.outgoing;ReleaseSRWLockExclusive(&jnet_sg_lock);
    if(outgoing.id==jnet_sl.identity && outgoing.sequence>jnet_sg.local.sequence) {
        jnet_sg.next_send=0;outgoing.lease=jnet_sg.local.lease;outgoing.chirp.epoch=outgoing.lease;jnet_sg.local=outgoing;
    }
    struct jnet_game_record events[32];unsigned event_count=0;
    AcquireSRWLockExclusive(&jnet_sg_lock);
    for(unsigned i=0;i<jnet_sg_mail.outgoing_count;++i) {
        events[event_count]=jnet_sg.local;events[event_count].chirp=jnet_sg_mail.outgoing_events[i].event;
        events[event_count].chirp.epoch=events[event_count].lease;events[event_count].chirp_seen=jnet_sg_mail.outgoing_events[i].seen;++event_count;
    }
    jnet_sg_mail.outgoing_count=0;ReleaseSRWLockExclusive(&jnet_sg_lock);
    for(unsigned i=0;i<event_count;++i)if(events[i].sequence && now>=events[i].chirp_seen && now-events[i].chirp_seen<=500)jnet_sg_event_send(events+i,host);else ++JNETSteamGame[17];
    struct jnet_game_record records[JNET_STEAM_CAPACITY]={0};records[0]=jnet_sg.local;unsigned n=1;
    if(host)for(unsigned i=0;i<7;++i) {
        if(!jnet_sn.peers[i].admitted || !jnet_sn_member(jnet_sn.peers[i].identity)) {memset(jnet_sg.peers+i,0,sizeof(jnet_sg.peers[i]));jnet_sg.handles[i]=0;continue;}
        if(jnet_sg.handles[i]!=jnet_sn.peers[i].handle) {
            memset(jnet_sg.peers+i,0,sizeof(jnet_sg.peers[i]));jnet_sg.handles[i]=jnet_sn.peers[i].handle;
            jnet_sg.peers[i].id=jnet_sn.peers[i].identity;jnet_sg.peers[i].lease=++jnet_sg.lease_counter;
        }
        if(n<JNET_STEAM_CAPACITY)records[n++]=jnet_sg.peers[i];
    }
    if(host)jnet_sg_publish(records,now);
    if(now<jnet_sg.next_send)return;jnet_sg.next_send=now+33;
    unsigned targets=0,connected=0;
    for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].admitted && jnet_sn.peers[i].connected) {
        ++connected;
        if(!host && (!jnet_sg.local.sequence || now<jnet_sg.local.seen || now-jnet_sg.local.seen>=2000))continue;
        if(!host && jnet_sg.sent_handles[i]==jnet_sn.peers[i].handle && jnet_sg.sent_sequences[i]==jnet_sg.local.sequence){++JNETSteamGame[40];continue;}
        if(host && jnet_sg.sent_handles[i]==jnet_sn.peers[i].handle && now-jnet_sg.roster_sent[i]<100) {
            int changed=0;for(unsigned j=0;j<JNET_STEAM_CAPACITY;++j) {
                uint64_t *key=jnet_sg.roster_keys[i][j];
                if(key[0]!=records[j].id || key[1]!=records[j].lease || key[2]!=records[j].sequence)changed=1;
            }
            if(!changed){++JNETSteamGame[61];continue;}
        }
        targets|=1u<<i;
    }
    JNETSteamGame[0]=1;if(!targets){if(!connected)jnet_sg.last_send=0;return;}
    if(jnet_sg.last_send && now>=jnet_sg.last_send){JNETSteamGame[10]=(LONG)(now-jnet_sg.last_send);if(JNETSteamGame[10]>JNETSteamGame[11])JNETSteamGame[11]=JNETSteamGame[10];}
    jnet_sg.last_send=now;
    if(jnet_sg.local.sequence && now>=jnet_sg.local.seen){JNETSteamGame[12]=(LONG)(now-jnet_sg.local.seen);if(JNETSteamGame[12]>JNETSteamGame[13])JNETSteamGame[13]=JNETSteamGame[12];}
    unsigned char packet[JNET_GAME_BYTES]={0};memcpy(packet,"JNG1",4);jc_packet_put(packet+56,jnet_travel_epoch,8);jc_packet_put(packet+4,host?2:1,4);jc_packet_put(packet+8,jnet_sl.room,8);memcpy(packet+16,jnet_sl.nonce,32);jc_packet_put(packet+48,++jnet_sg.sequence,8);
    for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i){struct jnet_game_record state=records[i];memset(&state.chirp,0,sizeof(state.chirp));jnet_sg_encode_record(packet+64+i*JNET_GAME_RECORD,&state,now);}
    for(unsigned i=0;i<7;++i)if(targets&(1u<<i)) {
        int result=jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),1);
        if(result==1){++JNETSteamGame[1];jnet_sg.sent_handles[i]=jnet_sn.peers[i].handle;jnet_sg.sent_sequences[i]=jnet_sg.local.sequence;
            jnet_sg.roster_sent[i]=now;for(unsigned j=0;j<JNET_STEAM_CAPACITY;++j){jnet_sg.roster_keys[i][j][0]=records[j].id;jnet_sg.roster_keys[i][j][1]=records[j].lease;jnet_sg.roster_keys[i][j][2]=records[j].sequence;}
        }
        else JNETSteamGame[4]=result;
    }
    JNETSteamGame[0]=1;
}
#endif
