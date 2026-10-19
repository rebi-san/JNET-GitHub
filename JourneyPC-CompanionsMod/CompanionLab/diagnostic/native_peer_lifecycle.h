#ifndef JC_NATIVE_PEER_LIFECYCLE_H
#define JC_NATIVE_PEER_LIFECYCLE_H
#include <stdint.h>
#include <string.h>

enum { JC_PEER_VACANT,JC_PEER_ACTIVE,JC_PEER_READY_TO_MED,JC_PEER_MED,JC_PEER_MED_WAIT,
    JC_PEER_DUST,JC_PEER_FADING,JC_PEER_DRAINING,JC_PEER_QUARANTINED };
enum { JC_PEER_OK,JC_PEER_ARGUMENT,JC_PEER_THREAD,JC_PEER_IDENTITY,
    JC_PEER_BUSY,JC_PEER_FAILED,JC_PEER_CLOCK };
enum { JC_PEER_BEGIN_READY_TO_MED,JC_PEER_BEGIN_MED,JC_PEER_BEGIN_DUST,
    JC_PEER_HIDE_AND_STOP_DUST,JC_PEER_CLEAR_IDENTITY };
enum { JC_PEER_READY_TO_MED_COMPLETE,JC_PEER_MED_COMPLETE,JC_PEER_DUST_COMPLETE,
    JC_PEER_EFFECTS_IDLE,JC_PEER_CONSUMERS_RETIRED };
struct jc_peer_identity { uint64_t peer,generation; };
struct jc_peer_lifecycle_ops {
    uint64_t (*thread)(void *);
    int (*admit)(void *,const struct jc_peer_identity *);
    int (*stop_footsteps_and_loops)(void *,const struct jc_peer_identity *);
    int (*begin)(void *,const struct jc_peer_identity *,unsigned);
    int (*poll)(void *,const struct jc_peer_identity *,unsigned);
    void *context;
};
struct jc_peer_lifecycle {
    struct jc_peer_identity identity;
    struct jc_peer_lifecycle_ops ops;
    uint64_t owner,last_time,departure_time,med_time,animation_timeout;
    uint64_t admissions,departures,abrupt_departures;
    unsigned phase,busy,last_result;
};
static inline int jc_peer_lifecycle_owner(const struct jc_peer_lifecycle *s) {
    return s && s->owner && s->ops.thread && s->ops.thread(s->ops.context)==s->owner;
}
static inline int jc_peer_lifecycle_fail(struct jc_peer_lifecycle *s,int code) {
    s->phase=JC_PEER_QUARANTINED;s->last_result=(unsigned)code;s->busy=0;return code;
}
static inline int jc_peer_lifecycle_init(struct jc_peer_lifecycle *s,
        const struct jc_peer_lifecycle_ops *ops,uint64_t animation_timeout) {
    if(!s || !ops || !ops->thread || !ops->admit || !ops->stop_footsteps_and_loops ||
        !ops->begin || !ops->poll || !animation_timeout || s->owner)return JC_PEER_ARGUMENT;
    uint64_t owner=ops->thread(ops->context);
    if(!owner)return JC_PEER_THREAD;
    memset(s,0,sizeof(*s));s->ops=*ops;s->owner=owner;s->animation_timeout=animation_timeout;
    return JC_PEER_OK;
}
static inline int jc_peer_lifecycle_accepts(const struct jc_peer_lifecycle *s,
        uint64_t peer,uint64_t generation) {
    return jc_peer_lifecycle_owner(s) && !s->busy && s->phase==JC_PEER_ACTIVE &&
        peer && peer==s->identity.peer && generation==s->identity.generation;
}
static inline int jc_peer_lifecycle_admit(struct jc_peer_lifecycle *s,
        uint64_t peer,uint64_t generation,uint64_t now) {
    if(!jc_peer_lifecycle_owner(s))return JC_PEER_THREAD;
    if(s->busy || s->phase!=JC_PEER_VACANT)return JC_PEER_BUSY;
    if(!peer || !generation || generation<=s->identity.generation)return JC_PEER_IDENTITY;
    if(now<s->last_time)return jc_peer_lifecycle_fail(s,JC_PEER_CLOCK);
    s->identity=(struct jc_peer_identity){peer,generation};s->last_time=now;s->busy=1;
    if(s->ops.admit(s->ops.context,&s->identity)!=1)return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
    s->phase=JC_PEER_ACTIVE;s->busy=0;s->last_result=0;++s->admissions;
    return JC_PEER_OK;
}
static inline int jc_peer_lifecycle_begin(struct jc_peer_lifecycle *s,unsigned phase,unsigned action) {
    s->phase=phase;
    if(s->ops.begin(s->ops.context,&s->identity,action)!=1)
        return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
    s->busy=0;return JC_PEER_OK;
}
static inline int jc_peer_lifecycle_depart(struct jc_peer_lifecycle *s,
        uint64_t peer,uint64_t generation,uint64_t now,int abrupt) {
    if(!jc_peer_lifecycle_owner(s))return JC_PEER_THREAD;
    if(s->busy)return JC_PEER_BUSY;
    if(!peer || peer!=s->identity.peer || generation!=s->identity.generation)return JC_PEER_IDENTITY;
    if(s->phase==JC_PEER_QUARANTINED)return JC_PEER_FAILED;
    if(s->phase==JC_PEER_VACANT)return JC_PEER_IDENTITY;
    if(now<s->last_time)return jc_peer_lifecycle_fail(s,JC_PEER_CLOCK);
    s->last_time=now;s->busy=1;
    if(s->phase==JC_PEER_ACTIVE) {
        s->departure_time=now;++s->departures;
        s->phase=JC_PEER_READY_TO_MED;
        if(s->ops.stop_footsteps_and_loops(s->ops.context,&s->identity)!=1)
            return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
        if(!abrupt)return jc_peer_lifecycle_begin(s,JC_PEER_READY_TO_MED,JC_PEER_BEGIN_READY_TO_MED);
    }
    if(abrupt && s->phase<JC_PEER_FADING) {
        ++s->abrupt_departures;
        return jc_peer_lifecycle_begin(s,JC_PEER_FADING,JC_PEER_HIDE_AND_STOP_DUST);
    }
    s->busy=0;return JC_PEER_OK;
}
static inline int jc_peer_lifecycle_tick(struct jc_peer_lifecycle *s,uint64_t now) {
    if(!jc_peer_lifecycle_owner(s))return JC_PEER_THREAD;
    if(s->busy)return JC_PEER_BUSY;
    if(s->phase==JC_PEER_QUARANTINED)return JC_PEER_FAILED;
    if(now<s->last_time)return jc_peer_lifecycle_fail(s,JC_PEER_CLOCK);
    s->last_time=now;
    if(s->phase==JC_PEER_VACANT || s->phase==JC_PEER_ACTIVE)return JC_PEER_OK;
    s->busy=1;
    if(s->phase<JC_PEER_FADING && now-s->departure_time>=s->animation_timeout) {
        ++s->abrupt_departures;
        return jc_peer_lifecycle_begin(s,JC_PEER_FADING,JC_PEER_HIDE_AND_STOP_DUST);
    }
    if(s->phase==JC_PEER_MED_WAIT) {
        if(now-s->med_time<2000){s->busy=0;return JC_PEER_BUSY;}
        return jc_peer_lifecycle_begin(s,JC_PEER_DUST,JC_PEER_BEGIN_DUST);
    }
    unsigned query=s->phase==JC_PEER_READY_TO_MED?JC_PEER_READY_TO_MED_COMPLETE:
        s->phase==JC_PEER_MED?JC_PEER_MED_COMPLETE:
        s->phase==JC_PEER_DUST?JC_PEER_DUST_COMPLETE:
        s->phase==JC_PEER_FADING?JC_PEER_EFFECTS_IDLE:JC_PEER_CONSUMERS_RETIRED;
    int result=s->ops.poll(s->ops.context,&s->identity,query);
    if(result<0 || result>1)return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
    if(!result){s->busy=0;return JC_PEER_BUSY;}
    switch(s->phase) {
    case JC_PEER_READY_TO_MED:return jc_peer_lifecycle_begin(s,JC_PEER_MED,JC_PEER_BEGIN_MED);
    case JC_PEER_MED:s->med_time=now;s->phase=JC_PEER_MED_WAIT;break;
    case JC_PEER_DUST:return jc_peer_lifecycle_begin(s,JC_PEER_FADING,JC_PEER_HIDE_AND_STOP_DUST);
    case JC_PEER_FADING:s->phase=JC_PEER_DRAINING;break;
    case JC_PEER_DRAINING:
        result=s->ops.poll(s->ops.context,&s->identity,JC_PEER_EFFECTS_IDLE);
        if(result<0 || result>1)return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
        if(!result){s->busy=0;return JC_PEER_BUSY;}
        if(s->ops.begin(s->ops.context,&s->identity,JC_PEER_CLEAR_IDENTITY)!=1)
            return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
        s->identity.peer=0;s->phase=JC_PEER_VACANT;break;
    default:return jc_peer_lifecycle_fail(s,JC_PEER_FAILED);
    }
    s->busy=0;return JC_PEER_OK;
}
#endif
