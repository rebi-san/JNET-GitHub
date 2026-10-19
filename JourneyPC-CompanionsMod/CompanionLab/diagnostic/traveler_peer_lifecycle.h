#ifndef JC_TRAVELER_PEER_LIFECYCLE_H
#define JC_TRAVELER_PEER_LIFECYCLE_H
static struct jc_peer_lifecycle traveler_peer;
static struct jc_actor_departure traveler_departure;
static struct {
    uint64_t actor,glyph,generation;
    unsigned char enabled;
    unsigned valid;
} traveler_peer_glyph_resume;
static struct {
    LONG state,result;
    unsigned completed,target;
    uint64_t generation,abrupt,ready_at;
} traveler_repeat;
static void traveler_repeat_tick(void);
__declspec(dllexport) volatile LONG JNETPeerLifecycle[14];

static void traveler_peer_report(void) {
    LONG values[14]={(LONG)traveler_peer.phase,(LONG)traveler_peer.last_result,
        (LONG)traveler_peer.identity.generation,(LONG)traveler_peer.admissions,
        (LONG)traveler_peer.departures,(LONG)traveler_peer.abrupt_departures,
        (LONG)(traveler_departure.dissolve*1000),(LONG)traveler_peer_restart_requested,
        traveler_repeat.state,(LONG)traveler_repeat.completed,(LONG)traveler_repeat.target,traveler_repeat.result,
        traveler_repeat.target==7,(LONG)traveler_actor_trial_outfit};
    for(unsigned i=0;i<14;++i)InterlockedExchange(&JNETPeerLifecycle[i],values[i]);
}
static int traveler_peer_fault(int code) {
    if(traveler_repeat.state==1){traveler_repeat.state=-1;traveler_repeat.result=code;}
    traveler_peer.phase=JC_PEER_QUARANTINED;traveler_peer.last_result=(unsigned)code;
    traveler_peer_depart_requested=traveler_peer_restart_requested=0;
    traveler_actor_trial_draining=0;JNETActorTrial[0]=-1;JNETActorTrial[1]=code;
    if(traveler_actor && traveler_actor->state!=JC_ACTOR_RUNTIME_QUARANTINED)
        traveler_actor_report(jc_actor_runtime_quarantine(traveler_actor,code));
    traveler_peer_report();return 0;
}
static int traveler_peer_write(void *ctx,uint64_t address,const void *data,size_t bytes) {
    (void)ctx;
    if(!traveler_actor || !traveler_context() || !jc_actor_runtime_thread(traveler_actor) ||
        address<traveler_object || bytes>16)return 0;
    uint64_t offset=address-traveler_object;
    int allowed=(offset==0x50 && bytes==2) || (offset==0x20 && bytes==16) ||
        ((offset==0x56 || offset==0x1e1c || offset==0x1dfc || offset==0x13d2a) && bytes==1) ||
        ((offset==0x1e00 || offset==0x2000 || offset==0x10ac4) && bytes==4);
    for(unsigned i=0;i<14;++i)allowed|=offset==jc_actor_emitter(i)+0x84 && bytes==1;
    for(unsigned i=0;i<16;++i)allowed|=offset==0xa390+0x64cd+i*0x28 && bytes==1;
    return allowed && jc_local_windows_write(address,data,bytes);
}
static int traveler_peer_identity(const struct jc_peer_identity *id) {
    return traveler_actor && id->peer==traveler_actor->identity.peer &&
        id->generation==traveler_actor->identity.generation &&
        traveler_live_identity(&traveler_actor->identity) &&
        jc_actor_departure_check(own_read,NULL,&traveler_departure);
}
static int traveler_peer_admit_service(void *ctx,const struct jc_peer_identity *id) {
    (void)ctx;
    if(!traveler_peer_identity(id) || traveler_actor->state!=JC_ACTOR_RUNTIME_ACTIVE)return 0;
    if(traveler_peer_glyph_resume.valid) {
        const struct jc_actor_identity *actor=&traveler_actor->identity;
        unsigned char gate=1;
        if(actor->actor!=traveler_peer_glyph_resume.actor || actor->glyph!=traveler_peer_glyph_resume.glyph ||
            actor->generation<=traveler_peer_glyph_resume.generation ||
            jc_actor_material_independent(own_read,NULL,actor,&traveler_actor_materials) ||
            !own_read(NULL,actor->glyph+0x70c,&gate,1) || gate ||
            !jc_local_windows_write(actor->glyph+0x70c,&traveler_peer_glyph_resume.enabled,1) ||
            !own_read(NULL,actor->glyph+0x70c,&gate,1) || gate!=traveler_peer_glyph_resume.enabled)return 0;
        traveler_peer_glyph_resume.valid=0;
        message("[JC] rejoined actor restored its owned floating glyph gate");
    }
    return 1;
}
static int traveler_peer_stop_sources(void *ctx,const struct jc_peer_identity *id) {
    (void)ctx;
    if(!traveler_peer_identity(id) || traveler_actor->state!=JC_ACTOR_RUNTIME_ACTIVE)return 0;
    traveler_actor->departing=1;traveler_actor->input.inactive=1;traveler_actor->input.interaction_gate=0;
    memset(traveler_actor->input.axes,0,sizeof(traveler_actor->input.axes));
    if(jc_receiver_depart(&traveler_actor->receiver,id->peer,id->generation))return 0;
    return cloth_actor_growth_stop() && jc_actor_departure_emitters(own_read,traveler_peer_write,NULL,&traveler_departure,0) &&
        jc_actor_departure_sounds(own_read,NULL,&traveler_departure,1)==1;
}
static int traveler_peer_action(void *ctx,const struct jc_peer_identity *id,unsigned action) {
    (void)ctx;
    if(!traveler_peer_identity(id))return 0;
    if(action==JC_PEER_BEGIN_READY_TO_MED)
        return jc_actor_departure_animation(own_read,traveler_peer_write,NULL,&traveler_departure,1);
    if(action==JC_PEER_BEGIN_MED || action==JC_PEER_BEGIN_DUST)return 1;
    if(action==JC_PEER_HIDE_AND_STOP_DUST) {
        if(!jc_actor_departure_emitters(own_read,traveler_peer_write,NULL,&traveler_departure,0) ||
            jc_actor_departure_sounds(own_read,NULL,&traveler_departure,1)!=1)return 0;
        traveler_departure.hidden=1;
        int code=JNETActorStop();
        return !code || code==JC_ACTOR_RUNTIME_BUSY;
    }
    if(action==JC_PEER_CLEAR_IDENTITY) {
        if(traveler_actor->state!=JC_ACTOR_RUNTIME_RETIRED ||
            !jc_actor_departure_animation(own_read,traveler_peer_write,NULL,&traveler_departure,0))return 0;
        unsigned char zero=0,check=1,enabled=0;
        uint64_t glyph=traveler_actor->identity.glyph;
        if(jc_actor_material_independent(own_read,NULL,&traveler_actor->identity,&traveler_actor_materials) ||
            !own_read(NULL,glyph+0x70c,&enabled,1) || enabled>1 ||
            !jc_local_windows_write(glyph+0x70c,&zero,1) || !own_read(NULL,glyph+0x70c,&check,1) || check)return 0;
        traveler_peer_glyph_resume.actor=traveler_actor->identity.actor;
        traveler_peer_glyph_resume.glyph=glyph;
        traveler_peer_glyph_resume.generation=traveler_actor->identity.generation;
        traveler_peer_glyph_resume.enabled=enabled;traveler_peer_glyph_resume.valid=1;
        memset(&traveler_chirp_glyph,0,sizeof(traveler_chirp_glyph));
        memset(traveler_chirp_pending,0,sizeof(traveler_chirp_pending));
        JNETActorTrial[0]=3;traveler_actor_trial_draining=0;
        return 1;
    }
    return 0;
}
static int traveler_peer_poll(void *ctx,const struct jc_peer_identity *id,unsigned query) {
    (void)ctx;
    if(!traveler_peer_identity(id))return -1;
    if(query==JC_PEER_READY_TO_MED_COMPLETE || query==JC_PEER_MED_COMPLETE)
        return jc_actor_departure_med(own_read,NULL,&traveler_departure);
    if(query==JC_PEER_DUST_COMPLETE) {
        float rendered;
        if(!own_read(NULL,traveler_object+0x2000,&rendered,4) || !isfinite(rendered))return -1;
        return traveler_departure.dissolve>=1.0f && rendered>=0.999f;
    }
    if(query==JC_PEER_EFFECTS_IDLE) {
        int particles=jc_actor_departure_particles_idle(own_read,NULL,&traveler_departure);
        if(particles!=1)return particles;
        int sounds=jc_actor_departure_sounds(own_read,NULL,&traveler_departure,0);
        if(sounds!=1)return sounds;
        uint64_t barn;
        if(!own_read(NULL,traveler_game+0x1b8,&barn,8) || !barn)return -1;
        for(unsigned i=0;i<JC_CHIRP_SLOTS;++i) {
            if(!traveler_chirp_pending[i].event)continue;
            if(traveler_chirp_pending[i].peer!=id->peer || traveler_chirp_pending[i].generation!=id->generation ||
                traveler_chirp_pending[i].event!=barn+0x10+i*0x70)return -1;
            uint32_t counter;unsigned char active;
            if(!own_read(NULL,traveler_chirp_pending[i].event+0x40,&counter,4) ||
                !own_read(NULL,traveler_chirp_pending[i].event+0x58,&active,1) || active>1)return -1;
            if(counter==traveler_chirp_pending[i].counter && active)return 0;
        }
        return 1;
    }
    if(query==JC_PEER_CONSUMERS_RETIRED) {
        int code=JNETActorStop();
        return !code?1:code==JC_ACTOR_RUNTIME_BUSY?0:-1;
    }
    return -1;
}
static int traveler_peer_admit(uint64_t peer,uint64_t generation) {
    if(!traveler_actor || traveler_actor->identity.peer!=peer || traveler_actor->identity.generation!=generation)
        return JC_ACTOR_IDENTITY;
    if(!jc_actor_departure_capture(own_read,NULL,&traveler_actor->identity,&traveler_departure))
        return JC_ACTOR_IDENTITY;
    if(!traveler_peer.owner) {
        struct jc_peer_lifecycle_ops ops={traveler_actor_thread,traveler_peer_admit_service,
            traveler_peer_stop_sources,traveler_peer_action,traveler_peer_poll,NULL};
        int code=jc_peer_lifecycle_init(&traveler_peer,&ops,60000);
        if(code)return code;
    }
    int code=jc_peer_lifecycle_admit(&traveler_peer,peer,generation,GetTickCount64());
    traveler_peer_report();return code;
}
static int traveler_peer_vacant(void) {
    return traveler_peer.owner && traveler_peer.phase==JC_PEER_VACANT && traveler_actor &&
        traveler_actor->state==JC_ACTOR_RUNTIME_RETIRED && !traveler_peer_depart_requested;
}
static int traveler_peer_before_input(void) {
    if(!traveler_peer.owner) {
        if(traveler_peer_depart_requested)return traveler_peer_fault(JC_ACTOR_IDENTITY);
        return 1;
    }
    if(traveler_peer.phase==JC_PEER_QUARANTINED)return 0;
    if(!traveler_actor || !traveler_context())return traveler_peer_fault(JC_ACTOR_IDENTITY);
    if(traveler_actor->state==JC_ACTOR_RUNTIME_QUARANTINED)return traveler_peer_fault(JC_ACTOR_QUARANTINED);
    if(traveler_live_retired(NULL,&traveler_actor->identity)!=1)return 0;
    unsigned before=traveler_peer.phase;
    if(traveler_peer_depart_requested) {
        unsigned abrupt=traveler_peer_depart_requested==2;
        traveler_peer_depart_requested=0;
        int code=jc_peer_lifecycle_depart(&traveler_peer,traveler_actor->identity.peer,
            traveler_actor->identity.generation,GetTickCount64(),abrupt);
        if(code)return traveler_peer_fault(code);
    }
    int code=jc_peer_lifecycle_tick(&traveler_peer,GetTickCount64());
    if(code && code!=JC_PEER_BUSY)return traveler_peer_fault(code);
    if(before!=traveler_peer.phase) {
        char line[160];snprintf(line,sizeof(line),"[JC] peer lifecycle phase=%u generation=%llu admissions=%llu departures=%llu",
            traveler_peer.phase,(unsigned long long)traveler_peer.identity.generation,
            (unsigned long long)traveler_peer.admissions,(unsigned long long)traveler_peer.departures);message(line);
    }
    traveler_repeat_tick();traveler_peer_report();return 1;
}
static void traveler_peer_after_input(void) {
    if(!traveler_actor || !traveler_actor->departing || traveler_departure.hidden)return;
    uint64_t clock;float dt;
    if(!own_read(NULL,traveler_game+0x68,&clock,8) || !own_read(NULL,clock+0x10,&dt,4) ||
        !jc_actor_departure_input(own_read,traveler_peer_write,NULL,&traveler_departure,
            traveler_peer.phase==JC_PEER_DUST,dt))traveler_peer_fault(JC_ACTOR_WRITE);
}
static void traveler_peer_after_update(void) {
    if(!traveler_actor || !traveler_actor->departing || traveler_departure.hidden)return;
    if(!jc_actor_departure_emitters(own_read,traveler_peer_write,NULL,&traveler_departure,
            traveler_peer.phase==JC_PEER_DUST) ||
        jc_actor_departure_sounds(own_read,NULL,&traveler_departure,1)!=1)traveler_peer_fault(JC_ACTOR_WRITE);
}
static int traveler_peer_replace(uint64_t peer,uint64_t generation) {
    struct jc_peer_identity previous={traveler_actor?traveler_actor->identity.peer:0,
        traveler_actor?traveler_actor->identity.generation:0};
    if(!traveler_peer_vacant() || !peer || generation<=traveler_peer.identity.generation ||
        !traveler_context() || !traveler_live || !traveler_actor ||
        traveler_live_retired(NULL,&traveler_actor->identity)!=1 ||
        traveler_peer_poll(NULL,&previous,JC_PEER_EFFECTS_IDLE)!=1)return JC_ACTOR_RUNTIME_BUSY;
    struct traveler_live_storage *p=traveler_live;
    if(!jc_cloth_live_owner(&p->backend) || cloth_participant_ready || cloth_producer.quarantined)return JC_ACTOR_QUARANTINED;
    for(unsigned f=0;f<JC_CLOTH_FRAMES;++f)for(unsigned n=0;n<2;++n) {
        struct jc_cloth_live_material *m=&p->materials[f][n];
        if(!m->ready || m->quarantined || m->pins || m->peer!=p->peer || m->generation!=p->generation)
            return JC_ACTOR_QUARANTINED;
    }
    for(unsigned i=0;i<JC_CLOTH_BUFFERS;++i)
        if(p->backend.buffers[i].pins || p->backend.buffers[i].mapped)return JC_ACTOR_RUNTIME_BUSY;
    for(unsigned i=0;i<sizeof(p->backend.probes)/sizeof(p->backend.probes[0]);++i) {
        struct jc_environment_probe *probe=&p->backend.probes[i];
        if(probe->peer && (probe->peer!=p->peer || probe->generation!=p->generation || probe->pins ||
            probe->quarantined || jc_environment_probe_check(own_read,NULL,probe)))return JC_ACTOR_QUARANTINED;
    }
    if(traveler_actor_admission_outfit<=6) {
        for(unsigned f=0;f<JC_CLOTH_FRAMES;++f)for(unsigned n=0;n<2;++n) {
            struct jc_cloth_live_material *m=&p->materials[f][n];
            if(m->atlas_holders && m->atlas_outfit!=traveler_actor_admission_outfit &&
                traveler_live_material_reset(m)) {
                traveler_peer_fault(JC_ACTOR_QUARANTINED);return JC_ACTOR_QUARANTINED;
            }
        }
    }
    struct jc_actor_runtime *next=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*next));
    if(!next)return JC_ACTOR_RUNTIME_BUSY;
    unsigned char zero=0;float value=0,velocity[4]={0};
    int reset=jc_actor_departure_animation(own_read,traveler_peer_write,NULL,&traveler_departure,0) &&
        jc_actor_departure_store(own_read,traveler_peer_write,NULL,traveler_object,0x56,&zero,1) &&
        jc_actor_departure_store(own_read,traveler_peer_write,NULL,traveler_object,0x1e1c,&zero,1) &&
        jc_actor_departure_store(own_read,traveler_peer_write,NULL,traveler_object,0x1e00,&value,4) &&
        jc_actor_departure_store(own_read,traveler_peer_write,NULL,traveler_object,0x2000,&value,4) &&
        jc_actor_departure_store(own_read,traveler_peer_write,NULL,traveler_object,0x20,velocity,16);
    if(!reset){HeapFree(GetProcessHeap(),0,next);traveler_peer_fault(JC_ACTOR_WRITE);return JC_ACTOR_WRITE;}
    struct jc_actor_identity id=traveler_actor->identity;id.peer=peer;id.generation=generation;
    for(unsigned f=0;f<JC_CLOTH_FRAMES;++f)for(unsigned n=0;n<2;++n) {
        p->materials[f][n].peer=peer;p->materials[f][n].generation=generation;
    }
    for(unsigned i=0;i<sizeof(p->backend.probes)/sizeof(p->backend.probes[0]);++i)if(p->backend.probes[i].peer) {
        p->backend.probes[i].peer=peer;p->backend.probes[i].generation=generation;p->backend.probes[i].produced=0;
    }
    p->peer=peer;p->generation=generation;p->backend.frames.closing=0;
    int code=jc_actor_runtime_bind(next,&id,&traveler_actor->ops);
    if(code){p->backend.failed=1;HeapFree(GetProcessHeap(),0,next);traveler_peer_fault(code);return code;}
    struct jc_actor_runtime *old=traveler_actor;traveler_actor=next;HeapFree(GetProcessHeap(),0,old);
    memset(&cloth_participant,0,sizeof(cloth_participant));
    return 0;
}
static void traveler_repeat_cancel(void) {
    if(traveler_repeat.state==1) {
        traveler_repeat.state=3;traveler_peer_restart_requested=0;traveler_peer_report();
    }
}
static uint32_t traveler_repeat_outfit(void) {
    return traveler_repeat.state==1 && traveler_repeat.target==7 && traveler_repeat.completed<7 ?
        traveler_repeat.completed : UINT32_MAX;
}
static int traveler_repeat_start(unsigned outfits) {
    if(traveler_repeat.state==1 || (JNETActorTrial[0]!=0 && JNETActorTrial[0]!=3) ||
        (traveler_actor && !traveler_peer_vacant()))return JC_ACTOR_RUNTIME_BUSY;
    memset(&traveler_repeat,0,sizeof(traveler_repeat));
    traveler_repeat.state=1;traveler_repeat.target=outfits?7:10;
    traveler_repeat.generation=traveler_peer.identity.generation;
    traveler_repeat.abrupt=traveler_peer.abrupt_departures;
    int code=JNETActorTrialStart();
    if(code){traveler_repeat.state=-1;traveler_repeat.result=code;}
    traveler_peer_report();return code;
}
static void traveler_repeat_tick(void) {
    if(traveler_repeat.state!=1)return;
    if(JNETActorTrial[0]<0 || traveler_peer.phase==JC_PEER_QUARANTINED ||
        traveler_peer.abrupt_departures!=traveler_repeat.abrupt) {
        traveler_repeat.state=-1;traveler_repeat.result=JNETActorTrial[1]?JNETActorTrial[1]:JC_PEER_FAILED;
        traveler_peer_restart_requested=0;return;
    }
    uint64_t now=GetTickCount64();
    if(traveler_peer.phase==JC_PEER_ACTIVE) {
        if(now>=traveler_actor_trial_started && now-traveler_actor_trial_started>=15000)
            traveler_actor_trial_stop(0);
        return;
    }
    if(!traveler_peer_vacant())return;
    if(traveler_peer.identity.generation!=traveler_repeat.generation) {
        traveler_repeat.generation=traveler_peer.identity.generation;
        ++traveler_repeat.completed;traveler_repeat.ready_at=now;
        char line[160];snprintf(line,sizeof(line),"[JC] repeat clean cycles=%u/%u actor=%llx glyph=%llx",
            traveler_repeat.completed,traveler_repeat.target,(unsigned long long)traveler_object,
            (unsigned long long)traveler_actor->identity.glyph);message(line);
    }
    if(traveler_repeat.completed==traveler_repeat.target){traveler_repeat.state=2;return;}
    if(now<traveler_repeat.ready_at || traveler_peer_restart_requested)return;
    int code=JNETActorTrialStart();
    if(code){traveler_repeat.state=-1;traveler_repeat.result=code;traveler_peer_restart_requested=0;}
}
#endif
