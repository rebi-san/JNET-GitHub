#include "native_peer_lifecycle.h"
#include <assert.h>
#include <stdio.h>

struct fixture {
    struct jc_peer_lifecycle lifecycle;
    uint64_t thread,now;
    unsigned admissions,stops,actions[5],polls[5],symbol,live_effects,leases;
    int observed[5],fail_action,fail_admit,fail_stop;
};
static uint64_t owner(void *ctx){return ((struct fixture *)ctx)->thread;}
static int admit(void *ctx,const struct jc_peer_identity *id) {
    struct fixture *f=ctx;
    assert(id->peer && id->generation && !f->symbol && !f->live_effects && !f->leases);
    assert(!jc_peer_lifecycle_accepts(&f->lifecycle,id->peer,id->generation));
    assert(jc_peer_lifecycle_depart(&f->lifecycle,id->peer,id->generation,f->now,1)==JC_PEER_BUSY);
    ++f->admissions;f->symbol=(unsigned)id->peer;
    return !f->fail_admit;
}
static int stop(void *ctx,const struct jc_peer_identity *id) {
    struct fixture *f=ctx;assert(id->peer && id->generation);
    assert(!jc_peer_lifecycle_accepts(&f->lifecycle,id->peer,id->generation));
    ++f->stops;return !f->fail_stop;
}
static int begin(void *ctx,const struct jc_peer_identity *id,unsigned action) {
    struct fixture *f=ctx;assert(action<5 && id->peer && f->symbol);
    assert(jc_peer_lifecycle_tick(&f->lifecycle,f->now)==JC_PEER_BUSY);
    ++f->actions[action];
    if(f->fail_action==(int)action)return 0;
    if(action==JC_PEER_CLEAR_IDENTITY) {
        assert(!f->live_effects && !f->leases);f->symbol=0;
    }
    return 1;
}
static int poll(void *ctx,const struct jc_peer_identity *id,unsigned query) {
    struct fixture *f=ctx;assert(query<5 && id->peer && f->symbol);
    ++f->polls[query];
    if(query==JC_PEER_EFFECTS_IDLE && f->live_effects)return 0;
    if(query==JC_PEER_CONSUMERS_RETIRED && f->leases)return 0;
    return f->observed[query];
}
static void init(struct fixture *f) {
    memset(f,0,sizeof(*f));f->thread=7;f->fail_action=-1;
    struct jc_peer_lifecycle_ops ops={owner,admit,stop,begin,poll,f};
    assert(!jc_peer_lifecycle_init(&f->lifecycle,&ops,30000));
}
static int tick(struct fixture *f,uint64_t elapsed) {
    f->now+=elapsed;return jc_peer_lifecycle_tick(&f->lifecycle,f->now);
}
static void drain(struct fixture *f) {
    struct jc_peer_lifecycle *s=&f->lifecycle;
    assert(s->phase==JC_PEER_FADING);
    f->live_effects=1;f->leases=1;
    f->observed[JC_PEER_EFFECTS_IDLE]=1;f->observed[JC_PEER_CONSUMERS_RETIRED]=1;
    assert(tick(f,60000)==JC_PEER_BUSY && f->symbol);
    assert(jc_peer_lifecycle_admit(s,99,s->identity.generation+1,f->now)==JC_PEER_BUSY);
    f->live_effects=0;assert(!tick(f,1) && s->phase==JC_PEER_DRAINING);
    assert(tick(f,60000)==JC_PEER_BUSY && f->symbol);
    f->leases=0;f->live_effects=1;
    assert(tick(f,1)==JC_PEER_BUSY && f->symbol);
    f->live_effects=0;assert(!tick(f,1) && s->phase==JC_PEER_VACANT && !f->symbol);
}
static void repeated_replacements(void) {
    struct fixture f;init(&f);
    struct jc_peer_lifecycle *s=&f.lifecycle;
    for(uint64_t generation=1;generation<=128;++generation) {
        uint64_t peer=generation%2+10;
        assert(!jc_peer_lifecycle_admit(s,peer,generation,f.now));
        assert(jc_peer_lifecycle_accepts(s,peer,generation));
        assert(!jc_peer_lifecycle_accepts(s,peer,generation-1));
        assert(jc_peer_lifecycle_depart(s,peer,generation-1,f.now,1)==JC_PEER_IDENTITY);
        assert(jc_peer_lifecycle_depart(s,peer+1,generation,f.now,1)==JC_PEER_IDENTITY);
        unsigned stops=f.stops;
        assert(!jc_peer_lifecycle_depart(s,peer,generation,f.now,0));
        assert(!jc_peer_lifecycle_accepts(s,peer,generation));
        assert(!jc_peer_lifecycle_depart(s,peer,generation,f.now,0) && f.stops==stops+1);
        if(generation%2) {
            f.observed[JC_PEER_READY_TO_MED_COMPLETE]=0;
            assert(tick(&f,1)==JC_PEER_BUSY && s->phase==JC_PEER_READY_TO_MED);
            f.observed[JC_PEER_READY_TO_MED_COMPLETE]=1;
            assert(!tick(&f,1) && s->phase==JC_PEER_MED);
            f.observed[JC_PEER_MED_COMPLETE]=1;
            assert(!tick(&f,1) && s->phase==JC_PEER_MED_WAIT);
            assert(tick(&f,1999)==JC_PEER_BUSY && s->phase==JC_PEER_MED_WAIT);
            assert(!tick(&f,1) && s->phase==JC_PEER_DUST);
            f.observed[JC_PEER_DUST_COMPLETE]=1;
            assert(!tick(&f,1) && s->phase==JC_PEER_FADING);
        } else {
            unsigned hides=f.actions[JC_PEER_HIDE_AND_STOP_DUST];
            assert(!jc_peer_lifecycle_depart(s,peer,generation,f.now,1));
            assert(!jc_peer_lifecycle_depart(s,peer,generation,f.now,1));
            assert(f.actions[JC_PEER_HIDE_AND_STOP_DUST]==hides+1 && f.stops==stops+1);
        }
        drain(&f);
        assert(jc_peer_lifecycle_admit(s,peer,generation,f.now)==JC_PEER_IDENTITY);
    }
    assert(s->admissions==128 && s->departures==128 && s->abrupt_departures==64);
    assert(f.admissions==128 && f.stops==128 && f.actions[JC_PEER_CLEAR_IDENTITY]==128);
    assert(f.actions[JC_PEER_BEGIN_READY_TO_MED]==128 && f.actions[JC_PEER_BEGIN_MED]==64);
    assert(f.actions[JC_PEER_BEGIN_DUST]==64 && !f.live_effects && !f.leases);
}
static void failures(void) {
    struct fixture f;init(&f);struct jc_peer_lifecycle *s=&f.lifecycle;
    assert(!jc_peer_lifecycle_admit(s,10,1,0));
    f.thread=8;
    assert(jc_peer_lifecycle_depart(s,10,1,0,1)==JC_PEER_THREAD);
    assert(jc_peer_lifecycle_tick(s,0)==JC_PEER_THREAD && !f.stops);
    f.thread=7;
    assert(!jc_peer_lifecycle_depart(s,10,1,0,0));
    assert(!tick(&f,30000) && s->phase==JC_PEER_FADING && s->abrupt_departures==1);
    assert(!f.actions[JC_PEER_BEGIN_MED] && !f.actions[JC_PEER_BEGIN_DUST]);
    f.observed[JC_PEER_EFFECTS_IDLE]=-1;
    assert(tick(&f,1)==JC_PEER_FAILED && s->phase==JC_PEER_QUARANTINED);
    assert(jc_peer_lifecycle_admit(s,11,2,f.now)==JC_PEER_BUSY && f.symbol);
    assert(!f.actions[JC_PEER_CLEAR_IDENTITY]);
    for(unsigned query=JC_PEER_EFFECTS_IDLE;query<=JC_PEER_CONSUMERS_RETIRED;++query) {
        init(&f);assert(!jc_peer_lifecycle_admit(s,10,1,0));
        assert(!jc_peer_lifecycle_depart(s,10,1,0,1));
        f.observed[JC_PEER_EFFECTS_IDLE]=1;
        if(query==JC_PEER_CONSUMERS_RETIRED)assert(!tick(&f,1));
        f.observed[query]=-1;
        assert(tick(&f,1)==JC_PEER_FAILED && f.symbol && !f.actions[JC_PEER_CLEAR_IDENTITY]);
    }
    init(&f);assert(!jc_peer_lifecycle_admit(s,10,1,10));f.now=10;
    assert(jc_peer_lifecycle_depart(s,10,1,9,1)==JC_PEER_CLOCK && !f.stops);
    init(&f);f.fail_admit=1;
    assert(jc_peer_lifecycle_admit(s,10,1,0)==JC_PEER_FAILED && s->phase==JC_PEER_QUARANTINED);
    init(&f);assert(!jc_peer_lifecycle_admit(s,10,1,0));f.fail_stop=1;
    assert(jc_peer_lifecycle_depart(s,10,1,0,1)==JC_PEER_FAILED && s->phase==JC_PEER_QUARANTINED);
    assert(!f.actions[JC_PEER_HIDE_AND_STOP_DUST]);
    init(&f);assert(!jc_peer_lifecycle_admit(s,10,1,0));f.fail_action=JC_PEER_HIDE_AND_STOP_DUST;
    assert(jc_peer_lifecycle_depart(s,10,1,0,1)==JC_PEER_FAILED && s->phase==JC_PEER_QUARANTINED);
}
int main(void) {
    repeated_replacements();failures();
    puts("Peer lifecycle policy: 128 replacements, ordered departure, two-second med hold, abrupt/timeout fallback, identity rejection, effect/GPU drain gates, reentrancy and quarantine passed; native services mocked, no Journey validation");
    return 0;
}
