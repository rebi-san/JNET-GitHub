#ifndef JC_TRAVELER_CHIRP_CHARGE_H
#define JC_TRAVELER_CHIRP_CHARGE_H
#include "native_chirp_charge.h"
static struct jc_chirp_charge traveler_charge;
static struct {uint64_t actor;unsigned char prior;unsigned applied;} traveler_charge_scope[3];
static void traveler_charge_after_update(void) {
    for(unsigned i=0;i<3;++i)if(traveler_charge_scope[i].applied) {
        if(!jc_local_windows_write(traveler_charge_scope[i].actor+0x50,&traveler_charge_scope[i].prior,1))
            traveler_peer_fault(JC_ACTOR_RUNTIME_SCOPE);
        traveler_charge_scope[i].applied=0;
    }
}
static void traveler_charge_cleanup(uintptr_t actor) {
    if(actor!=traveler_barn+0xb0)return;
    traveler_charge_after_update();memset(&traveler_charge,0,sizeof(traveler_charge));
}
static void traveler_charge_before_update(uintptr_t barn,uintptr_t game) {
    if(!traveler_actor || traveler_actor->state==JC_ACTOR_RUNTIME_QUARANTINED ||
        barn!=traveler_barn || game!=traveler_game ||
        !jc_actor_runtime_thread(traveler_actor) || !traveler_context())return;
    const struct jc_actor_identity *id=&traveler_actor->identity;
    if(!jc_actor_identity_check(own_read,NULL,id))return;
    uint64_t pool=0,clock=0;float dt;
    unsigned char snapshot[0x718],again[0x718];unsigned owners[16]={0};uint32_t count;
    struct jc_layout_result layout;
    if(jc_native_layout(own_read,NULL,id->image,game,&layout) ||
        !own_read(NULL,game+0x1b8,&pool,8) || !pool ||
        !own_read(NULL,game+0x68,&clock,8) || !clock || !own_read(NULL,clock+0x10,&dt,4) ||
        !isfinite(dt) || dt<=0 || dt>1 || !own_read(NULL,pool,snapshot,sizeof(snapshot)) ||
        !own_read(NULL,pool,again,sizeof(again)) || memcmp(snapshot,again,sizeof(snapshot)))return;
    memcpy(&count,snapshot+0x710,4);if(count>16)return;
    for(unsigned i=0;i<count;++i) {
        const unsigned char *s=snapshot+i*0x70;uint32_t serial;memcpy(&serial,s+0x50,4);
        if(s[0x4c]==1 && !s[0x4d])owners[i]=1;
        else if(s[0x4d]==1 && !s[0x4c] && layout.remote_gate)owners[i]=2;
        else if(!s[0x4c] && !s[0x4d] && traveler_chirp_pending[i].event==pool+0x10+i*0x70 &&
            traveler_chirp_pending[i].counter==serial && traveler_chirp_pending[i].peer==id->peer &&
            traveler_chirp_pending[i].generation==id->generation)owners[i]=3;
    }
    for(unsigned i=0;i<3;++i) {
        uint64_t actor=i==0?barn+0xb0:i==1?barn+0x146f0:id->actor;
        int active=i==0 || (i==1?layout.remote_gate && traveler_local_state!=3:
            traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE && !traveler_actor->departing);
        float position[4];unsigned char prior=0;
        if(!active || !own_read(NULL,actor+0x10,position,16) || !own_read(NULL,actor+0x50,&prior,1) || prior>1) {
            jc_chirp_charge_tick(&traveler_charge,i,id->generation,0,0,dt);continue;
        }
        int hit=jc_chirp_charge_hit(id->image,snapshot,owners,i,position,dt);
        if(!jc_chirp_charge_tick(&traveler_charge,i,id->generation,1,hit,dt))continue;
        float energy,targets[2];
        if(!own_read(NULL,actor+0x60,&energy,4) || !isfinite(energy) || energy<0 || energy>1 ||
            !own_read(NULL,actor+0x1448,targets,4) || !own_read(NULL,actor+0x1450,targets+1,4) ||
            !isfinite(targets[0]) || !isfinite(targets[1]))continue;
        float glow=1.f-powf(energy,.125f);unsigned char one=1;
        if(!jc_local_windows_write(actor+0x50,&one,1)){traveler_peer_fault(JC_ACTOR_RUNTIME_SCOPE);return;}
        traveler_charge_scope[i].actor=actor;traveler_charge_scope[i].prior=prior;traveler_charge_scope[i].applied=1;
        for(unsigned j=0;j<2;++j) {
            targets[j]=fmaxf(targets[j],glow);
            if(!jc_local_windows_write(actor+0x1448+j*8,targets+j,4)) {
                traveler_charge_after_update();traveler_peer_fault(JC_ACTOR_RUNTIME_SCOPE);return;
            }
        }
    }
    if((uint32_t)JNETFrames%300==0) {
        char line[192];snprintf(line,sizeof(line),"[JC] extra chirp charging hits local/native/extra=%u/%u/%u timers=%.3f/%.3f/%.3f",
            traveler_charge.hits[0],traveler_charge.hits[1],traveler_charge.hits[2],
            traveler_charge.remaining[0],traveler_charge.remaining[1],traveler_charge.remaining[2]);message(line);
    }
}
#endif
