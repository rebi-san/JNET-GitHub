#ifndef JNET_STEAM_GAME_LIVE_H
#define JNET_STEAM_GAME_LIVE_H
#include "jnet_motion.h"
static void jnet_sg_game_phase(uintptr_t game,int playback) {
    static uint64_t room,generation,epoch,sequence,next_capture,last_update;
    static struct {uint64_t id,lease,generation,sequence,updated;} timing[2];
    static struct {uint64_t id;} memory[2];static struct jc_chirp_capture capture;
    uint64_t now=GetTickCount64(),active_room,identity,active_generation,received;
    struct jnet_game_record records[JNET_STEAM_CAPACITY];
    AcquireSRWLockExclusive(&jnet_sg_lock);
    active_room=jnet_sg_mail.room;identity=jnet_sg_mail.identity;active_generation=jnet_sg_mail.generation;received=jnet_sg_mail.received;
    memcpy(records,jnet_sg_mail.records,sizeof(records));ReleaseSRWLockExclusive(&jnet_sg_lock);
    if(!active_room && !traveler_network_external)return;
    if(room!=active_room || generation!=active_generation)memset(memory,0,sizeof(memory));
    if(room!=active_room || generation!=active_generation || epoch!=jnet_travel_epoch) {
        uint64_t chirp_sequence=capture.sequence;
        room=active_room;generation=active_generation;epoch=jnet_travel_epoch;next_capture=0;last_update=0;memset(&capture,0,sizeof(capture));
        capture.sequence=chirp_sequence;
    }
    if(playback && last_update && active_room && room==active_room && generation==active_generation) {
        uint64_t gap=now-last_update;JNETSteamGame[49]=(LONG)(gap>60000?60000:gap);
        if(JNETSteamGame[49]>JNETSteamGame[50])JNETSteamGame[50]=JNETSteamGame[49];
        ++JNETSteamGame[51+(gap<50?0:gap<100?1:gap<250?2:3)];
    }
    if(playback)last_update=active_room?now:0;
    if(playback) {
    ++JNETSteamGame[55];
    traveler_network_external=1;traveler_network_active=1;traveler_stream_enabled=1;
    traveler_network.identity=identity;traveler_network.chapter=jnet_current_level;
    traveler_network.paused=(jnet_travel_busy && jnet_travel_target!=jnet_current_level) || !active_room || !JNETSteamLobby[9] || JNETSteamLobby[10] || JNETNetwork[23];
    JNETNetwork[0]=traveler_network.paused?2:1;
    JNETSteamGame[26]=JNETSteamGame[34]=0;
    struct jnet_lobby_view lobby;jnet_lobby_snapshot(&lobby);
    for(unsigned slot=0;slot<2;++slot) {
        struct jc_network_peer *p=&traveler_network.peers[slot];
        const struct jnet_game_record *r=NULL;
        if(!traveler_network.paused && received && now>=received && now-received<2000)
            for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)if(records[i].id && records[i].id==p->connection.peer && records[i].sample.chapter==jnet_current_level)r=records+i;
        int idle=slot?traveler_local_state==0 || traveler_local_state==4:
            (!traveler_actor || traveler_peer_vacant()) && !traveler_actor_trial_join_pending;
        if(memory[slot].id && lobby.room==active_room && lobby.member_count){
            int member=0;for(unsigned k=0;k<lobby.member_count;++k)member|=lobby.members[k]==memory[slot].id;
            if(!member)memset(memory+slot,0,sizeof(memory[slot]));
        }
        if(!r || !r->sequence || now<r->seen || now-r->seen>=2000) {
            p->connection.admitted=0;p->connection.sequence=0;p->wire_chirps.pending=0;
            memset(jnet_motion+slot,0,sizeof(jnet_motion[slot]));
            if(idle)p->connection.peer=memory[slot].id;
        }
    }
    if(!traveler_network.paused && received && now>=received && now-received<2000)for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i) {
        const struct jnet_game_record *r=records+i;if(r->sample.chapter!=jnet_current_level || !r->id || r->id==identity || !r->sequence || now<r->seen || now-r->seen>=2000)continue;
        unsigned slot=2;
        for(unsigned j=0;j<2;++j)if(traveler_network.peers[j].connection.peer==r->id)slot=j;
        if(slot==2)for(unsigned n=0;n<2;++n){unsigned j=1-n;if(!traveler_network.peers[j].connection.peer){slot=j;break;}}
        if(slot==2)continue;
        struct jc_network_peer *p=&traveler_network.peers[slot];
        if(p->connection.peer!=r->id || p->connection.remote_epoch!=r->lease) {
            p->connection.sequence=0;p->consumed=0;memset(&p->wire_chirps,0,sizeof(p->wire_chirps));
        }
        unsigned row=24+slot*8;
        if(timing[slot].id!=r->id || timing[slot].lease!=r->lease || timing[slot].generation!=generation) {
            memset(timing+slot,0,sizeof(timing[slot]));timing[slot].id=r->id;timing[slot].lease=r->lease;timing[slot].generation=generation;
            for(unsigned k=0;k<8;++k)JNETSteamGame[row+k]=0;
            JNETSteamGame[row]=(LONG)(uint32_t)r->id;JNETSteamGame[row+1]=(LONG)(uint32_t)(r->id>>32);
        }
        JNETSteamGame[row+2]=1;JNETSteamGame[row+4]=(LONG)(now-r->seen);
        if(JNETSteamGame[row+4]>JNETSteamGame[row+5])JNETSteamGame[row+5]=JNETSteamGame[row+4];
        if(timing[slot].sequence!=r->sequence) {
            if(timing[slot].updated && now>=timing[slot].updated){uint64_t gap=now-timing[slot].updated;JNETSteamGame[row+6]=(LONG)(gap>60000?60000:gap);
                if(JNETSteamGame[row+6]>JNETSteamGame[row+7])JNETSteamGame[row+7]=JNETSteamGame[row+6];}
            if(r->processed && now>=r->processed){unsigned at=57+slot*2;JNETSteamGame[at]=(LONG)(now-r->processed>60000?60000:now-r->processed);if(JNETSteamGame[at]>JNETSteamGame[at+1])JNETSteamGame[at+1]=JNETSteamGame[at];}
            timing[slot].sequence=r->sequence;timing[slot].updated=now;++JNETSteamGame[row+3];
        }
        JNETSteamGame[14]=(LONG)(now-r->seen);if(JNETSteamGame[14]>JNETSteamGame[15])JNETSteamGame[15]=JNETSteamGame[14];
        memory[slot].id=r->id;
        p->connection.local=identity;p->connection.peer=r->id;p->connection.remote_epoch=r->lease;p->connection.admitted=1;
        p->connection.sequence=jnet_motion_apply(slot,r,generation,now,&p->sample);p->connection.last_seen=r->seen;traveler_udp[slot].ready=1;
        traveler_network_chirp_take(slot,now);
    }
    AcquireSRWLockExclusive(&jnet_sg_lock);
    unsigned retained=0;
    for(unsigned i=0;i<jnet_sg_mail.event_count;++i) {
        const struct jnet_game_record *event=&jnet_sg_mail.events[i];unsigned consumed=0;
        if(event->sample.chapter!=jnet_current_level)continue;
        if(now<event->chirp_seen || now-event->chirp_seen>500){++JNETSteamGame[17];continue;}
        for(unsigned slot=0;slot<2;++slot) {
            struct jc_network_peer *p=&traveler_network.peers[slot];
            if(p->connection.peer!=event->id || p->connection.remote_epoch!=event->lease)continue;
            if(event->chirp.sequence<=p->wire_chirps.sequence){consumed=1;break;}
            if(!p->wire_chirps.pending){p->wire_chirps.event=event->chirp;p->wire_chirps.sequence=event->chirp.sequence;p->wire_chirps.received_at=event->chirp_seen;p->wire_chirps.pending=1;consumed=1;}
            break;
        }
        if(!consumed)jnet_sg_mail.events[retained++]=*event;
    }
    jnet_sg_mail.event_count=retained;ReleaseSRWLockExclusive(&jnet_sg_lock);
    for(unsigned slot=0;slot<2;++slot) {
        traveler_network_chirp_take(slot,now);
        struct jc_network_peer *p=&traveler_network.peers[slot];unsigned row=8+slot*3;
        JNETNetwork[row]=(LONG)(slot+1);JNETNetwork[row+1]=p->connection.admitted;
        JNETNetwork[row+2]=p->connection.sequence?(LONG)(now-p->connection.last_seen>60000?60000:now-p->connection.last_seen):-1;
    }
    JNETSteamGame[7]=(LONG)traveler_local_state;JNETSteamGame[8]=JNETActorTrial[0];JNETSteamGame[9]=JNETNetwork[23];
    return;
    }
    if(jnet_travel_busy || !active_room || traveler_network.paused)return;
    struct jc_chirp_batch batch;int chirp=jc_chirp_capture_local(own_read,NULL,game,(uint32_t)JNETFrames,identity,1,&capture,&batch);
    if(now<next_capture && (chirp || !batch.count))return;next_capture=now+33;
    struct jnet_game_record outgoing={.id=identity,.lease=1,.sequence=++sequence,.seen=now};
    int code=jc_actor_capture_sender(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),game,&outgoing.sample);JNETSteamGame[5]=code;
    static uint64_t next_capture_error;
    if(code==JC_LAYOUT_RANGE && now>=next_capture_error){
        next_capture_error=now+5000;
        const struct jc_actor_sender *bad=&jc_actor_capture_rejected;uint32_t appearance[4];
        memcpy(appearance,bad->state.bytes+0x58,sizeof(appearance));char line[384];
        snprintf(line,sizeof(line),"[JNET] capture rejected field=%u scarf=%u/%u robe=%u symbol=%u chapter=%u scale=%.9g chirp=%.9g/%.9g glow=%.9g/%.9g/%.9g",
            jc_actor_capture_rejected_field,appearance[0],appearance[1],appearance[2],appearance[3],bad->chapter,
            bad->body_scale,bad->chirp_shape[0],bad->chirp_shape[1],bad->glow_targets[0],bad->glow_targets[1],bad->glow_targets[2]);message(line);
    }
    int valid=!code && outgoing.sample.chapter==jnet_current_level;
    outgoing.character=jnet_menu_edit.applied.character;
    outgoing.spotlight=jnet_character_spotlight;
    outgoing.rotation_known=jnet_character_capture_rotation && jnet_character_capture_rotation(outgoing.rotation);
    outgoing.character_known=outgoing.character<JNET_CHARACTER_COUNT;
    AcquireSRWLockExclusive(&jnet_sg_lock);
    if(jnet_sg_mail.room==active_room && jnet_sg_mail.generation==active_generation) {
        outgoing.chirp=jnet_sg_mail.outgoing.chirp;outgoing.chirp_seen=jnet_sg_mail.outgoing.chirp_seen;
        const struct jnet_game_record *previous=&jnet_sg_mail.outgoing;
        int chirp_ready=valid || (previous->id==identity && previous->sequence && previous->sample.chapter==jnet_current_level &&
            now>=previous->seen && now-previous->seen<2000);
        if(!chirp && !chirp_ready)JNETSteamGame[17]+=(LONG)batch.count;
        if(!chirp && chirp_ready)for(unsigned i=0;i<batch.count;++i) {
            if(jnet_sg_mail.outgoing_count<32){unsigned n=jnet_sg_mail.outgoing_count++;jnet_sg_mail.outgoing_events[n].event=batch.source[i].event;jnet_sg_mail.outgoing_events[n].seen=now;}
            else ++JNETSteamGame[16];
        }
        if(valid){jnet_sg_mail.outgoing=outgoing;++JNETSteamGame[6];}
    }
    ReleaseSRWLockExclusive(&jnet_sg_lock);
}
#endif
