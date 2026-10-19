#ifndef JNET_MOTION_H
#define JNET_MOTION_H
#include <math.h>
static unsigned jnet_motion_ms;
struct jnet_motion_state {
    uint64_t id,lease,generation,wire,started,updated;
    float from[3],target[3],previous[3],delivered[3];
    struct jc_actor_sender source;
};
static struct jnet_motion_state jnet_motion[2];
static uint64_t jnet_motion_sequence[2];
static void jnet_motion_position(const struct jnet_motion_state *s,uint64_t now,float out[3]) {
    float t=!jnet_motion_ms || now<s->started || now-s->started>=jnet_motion_ms?1.f:(float)(now-s->started)/jnet_motion_ms;
    for(unsigned i=0;i<3;++i)out[i]=s->from[i]+(s->target[i]-s->from[i])*t;
}
static uint64_t jnet_motion_apply(unsigned slot,const struct jnet_game_record *r,uint64_t generation,uint64_t now,struct jc_actor_sender *sample) {
    struct jnet_motion_state *s=jnet_motion+slot;float position[3],current[3];
    memcpy(position,r->sample.state.bytes,sizeof(position));*sample=r->sample;
    int reset=s->id!=r->id || s->lease!=r->lease || s->generation!=generation || !s->wire;
    if(reset) {
        memset(s,0,sizeof(*s));s->id=r->id;s->lease=r->lease;s->generation=generation;
        memcpy(s->from,position,sizeof(position));memcpy(s->target,position,sizeof(position));
        unsigned at=80+slot*5;for(unsigned i=0;i<5;++i)JNETSteamGame[at+i]=0;
    }
    jnet_motion_position(s,now,current);
    int fresh=s->wire!=r->sequence;
    if(fresh) {
        float distance=0,offset=0;
        for(unsigned i=0;i<3;++i){float d=position[i]-s->previous[i];distance+=d*d;d=position[i]-current[i];offset+=d*d;}
        unsigned at=80+slot*5;
        if(!reset) {
            float length=sqrtf(distance);LONG mm=isfinite(length)?(LONG)(length>60.f?60000.f:length*1000.f):60000;
            JNETSteamGame[at]=mm;if(mm>JNETSteamGame[at+1])JNETSteamGame[at+1]=mm;
            ++JNETSteamGame[at+2];if(distance>1.f)++JNETSteamGame[at+3];
        }
        int bypass=reset || !jnet_motion_ms || now<s->updated || now-s->updated>=250 ||
            !isfinite(offset) || offset>1.f ||
            memcmp(s->source.state.bytes+0x40,r->sample.state.bytes+0x40,12) ||
            memcmp(s->source.state.bytes+0x58,r->sample.state.bytes+0x58,JC_RECEIVER_STATE_BYTES-0x58) ||
            s->source.chapter!=r->sample.chapter;
        memcpy(s->from,bypass?position:current,sizeof(position));memcpy(s->target,position,sizeof(position));
        memcpy(s->previous,position,sizeof(position));s->source=r->sample;s->wire=r->sequence;s->started=now;s->updated=now;
        if(bypass && !reset && jnet_motion_ms)++JNETSteamGame[at+4];
    }
    if(!jnet_motion_ms)return r->sequence;
    jnet_motion_position(s,now,current);
    int changed=fresh || memcmp(s->delivered,current,sizeof(current));
    memcpy(sample->state.bytes,current,sizeof(current));
    memcpy(s->delivered,current,sizeof(current));
    if(changed)++jnet_motion_sequence[slot];
    return jnet_motion_sequence[slot];
}
#endif
