#ifndef JC_TRAVELER_LIVE_BACKEND_H
#define JC_TRAVELER_LIVE_BACKEND_H

struct traveler_live_storage {
    struct jc_cloth_live backend;
    struct jc_cloth_live_material materials[JC_CLOTH_FRAMES][2];
    uint64_t peer,generation;
};
static struct traveler_live_storage *traveler_live;
static int traveler_frame_retired(void);
static int traveler_live_identity(const struct jc_actor_identity *id) {
    return traveler_live && id->peer==traveler_live->peer && id->generation==traveler_live->generation &&
        id->actor==traveler_object && id->game==traveler_game && id->barn==traveler_barn &&
        active_cloth_backend==&traveler_live->backend && traveler_context();
}
static int traveler_live_retired(void *context,const struct jc_actor_identity *id) {
    (void)context;
    if (!traveler_live_identity(id) || traveler_frame_retired()!=1) return 0;
    struct jc_cloth_live *s=&traveler_live->backend;
    if (!jc_cloth_live_owner(s) || jc_cloth_frames_poll(&s->frames)) return 0;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i)
        if (s->frames.frames[i].phase!=JC_FRAME_FREE || s->automatic_pending[i]) return 0;
    return !cloth_maps.roles && !cloth_producer.adopted;
}
static int traveler_live_detach_body_probe(struct jc_cloth_live *s) {
    if (!s || s->closed) return JC_FRAME_OK;
    unsigned owned=0;
    for (unsigned i=0;i<sizeof(s->probes)/sizeof(s->probes[0]);++i)
        owned|=s->probes[i].pair[0]!=0;
    if (!owned) return JC_FRAME_OK;
    if (!traveler_live || s!=&traveler_live->backend || !traveler_actor ||
        traveler_actor->state!=JC_ACTOR_RUNTIME_RETIRED || !original_cloth_setter[2])
        return JC_FRAME_OWNERSHIP;
    if (!traveler_live_retired(NULL,&traveler_actor->identity)) return JC_FRAME_BUSY;
    if (jc_actor_material_independent(own_read,NULL,&traveler_actor->identity,&traveler_actor_materials))
        return JC_FRAME_MATERIAL;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) return JC_FRAME_OWNERSHIP;
    int code=JC_FRAME_MATERIAL;
    uint64_t barn=0,pair[2]={0},again[2]={0};uint32_t refs=0;unsigned char remote=0;
    struct jc_environment_texture before,after;
    uint64_t material=traveler_object+0x1e0;
    if (!own_read(NULL,traveler_game+0x108,&barn,8) || !barn ||
        !own_read(NULL,traveler_object+0x81,&remote,1) || remote!=1 ||
        !own_read(NULL,barn+0x646d40,pair,sizeof(pair)) || !pair[0] || !pair[1] ||
        !own_read(NULL,pair[1],&refs,4) || !refs || refs>0x1000000 ||
        jc_environment_texture_read(own_read,NULL,pair[0],&before) ||
        !jc_environment_texture_ready(&before) ||
        jc_cloth_texture_check(own_read,NULL,material,"envOccCube",pair[0],0)) goto done;
    unsigned bound=!jc_cloth_texture_bound(own_read,NULL,material,"envOccCube",pair[0]);
    for (unsigned i=0;i<sizeof(s->probes)/sizeof(s->probes[0]);++i) {
        struct jc_environment_probe *p=&s->probes[i];
        if (!p->pair[0]) continue;
        if (p->pair[0]==pair[0] || p->pair[1]==pair[1] || p->pins ||
            jc_environment_probe_check(own_read,NULL,p)) goto done;
        bound|=!jc_cloth_texture_bound(own_read,NULL,material,"envOccCube",p->pair[0]);
    }
    if (!bound || !own_read(NULL,barn+0x646d40,again,sizeof(again)) ||
        memcmp(pair,again,sizeof(pair)) ||
        jc_environment_texture_read(own_read,NULL,pair[0],&after) ||
        !jc_environment_texture_same(&before,&after)) goto done;
    original_cloth_setter[2](material,(uintptr_t)"envOccCube",pair[0]);
    if (jc_cloth_texture_bound(own_read,NULL,material,"envOccCube",pair[0]) ||
        !own_read(NULL,barn+0x646d40,again,sizeof(again)) || memcmp(pair,again,sizeof(pair)) ||
        jc_environment_texture_read(own_read,NULL,pair[0],&after) ||
        !jc_environment_texture_same(&before,&after) ||
        jc_actor_material_independent(own_read,NULL,&traveler_actor->identity,&traveler_actor_materials)) goto done;
    code=JC_FRAME_OK;
done:
    if (!jc_cloth_live_graphics_leave(s,&token)) return JC_FRAME_OWNERSHIP;
    return code;
}
static int traveler_live_ready(void *context,const struct jc_actor_identity *id) {
    (void)context;
    if (!traveler_live_identity(id) || !jc_cloth_live_owner(&traveler_live->backend) ||
        traveler_live->backend.frames.closing) return 0;
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) for (unsigned r=0;r<2;++r) {
        struct jc_cloth_live_material *m=&traveler_live->materials[f][r];
        if (!m->ready || m->quarantined || m->peer!=id->peer || m->generation!=id->generation ||
            m->role!=r+4) return 0;
    }
    return 1;
}
static int traveler_live_prepare(void *context,const struct jc_actor_identity *id,uint64_t frame) {
    return frame && traveler_live_ready(context,id) && traveler_live_retired(context,id);
}
static struct jc_body_appearance_lease traveler_body_lease;
static struct jc_body_outfit_lease traveler_body_outfit;
static uint64_t traveler_body_image,traveler_body_thread;
static int traveler_body_hold(void *ctx,const uint64_t pair[2]) {
    struct jc_cloth_atlas_native native={.image=traveler_body_image,.read=own_read};
    (void)ctx;return jc_cloth_atlas_native_hold(&native,pair);
}
static void traveler_body_drop(void *ctx,const uint64_t pair[2]) {
    struct jc_cloth_atlas_native native={.image=traveler_body_image,.read=own_read};
    (void)ctx;jc_cloth_atlas_native_drop(&native,pair);
}
static int traveler_body_texture(void *ctx,uint64_t material,const char *name,uint64_t texture) {
    (void)ctx;
    if(material!=traveler_object+0x1e0 || !original_cloth_setter[2])return 0;
    original_cloth_setter[2](material,(uintptr_t)name,texture);return 1;
}
static int traveler_body_vector(void *ctx,uint64_t material,const char *name,const float value[4]) {
    (void)ctx;
    if(material!=traveler_object+0x1e0 || !original_cloth_setter[0])return 0;
    original_cloth_setter[0](material,(uintptr_t)name,(uintptr_t)value);return 1;
}
static int traveler_body_scalar(void *ctx,uint64_t material,const char *name,float value) {
    (void)ctx;uint64_t payload=0;
    if(material!=traveler_object+0x1e0 || jc_material_ready(own_read,NULL,material,&payload))return 0;
    float *(*lookup)(uintptr_t,const char *)=(void *)(uintptr_t)(traveler_body_image+JC_RVA_MaterialScalarLookup);
    float *destination=lookup(payload,name);
    return destination && jc_local_windows_write((uintptr_t)destination,&value,4);
}
static const struct jc_body_appearance_ops traveler_body_ops={
    {traveler_body_scalar,traveler_body_vector,traveler_body_texture},traveler_body_hold,traveler_body_drop};
static int traveler_body_prepare(const struct jc_actor_identity *id) {
    unsigned stage=1;int code=JC_LAYOUT_CHANGED;
    uint64_t definition=0;uint32_t outfits[2]={UINT32_MAX,UINT32_MAX};
    char line[224];
    if(traveler_body_lease.applied) {
        if(traveler_body_lease.material==id->actor+0x1e0 && traveler_body_thread==GetCurrentThreadId())return 1;
        goto declined;
    }
    if(traveler_body_lease.material || !traveler_live_identity(id))goto declined;
    stage=2;code=jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials);
    if(code)goto declined;
    stage=3;code=jc_body_appearance_source(own_read,NULL,id->barn,id->actor,&definition,outfits);
    if(code)goto declined;
    const uint64_t materials[3]={id->barn+0x290,id->barn+0x148d0,id->actor+0x1e0};
    traveler_body_image=id->image;traveler_body_thread=GetCurrentThreadId();
    struct jc_cloth_graphics_token token={0};
    stage=4;code=JC_FRAME_OWNERSHIP;
    if(!jc_cloth_live_graphics_enter(&traveler_live->backend,&token))goto declined;
    stage=5;code=jc_body_appearance_prepare(own_read,NULL,&traveler_body_ops,definition,materials,&traveler_body_lease);
    if(!code)code=jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials);
    if(!jc_cloth_live_graphics_leave(&traveler_live->backend,&token))code=JC_FRAME_OWNERSHIP;
    snprintf(line,sizeof(line),"[JNET] body appearance code=%d stage=%u held=%u applied=%u outfits=%u/%u",
        code,traveler_body_lease.stage,traveler_body_lease.held,traveler_body_lease.applied,outfits[0],outfits[1]);message(line);
    return !code;
declined:
    snprintf(line,sizeof(line),"[JNET] body preparation declined preflight=%u code=%d outfits=%u/%u lease-stage=%u",
        stage,code,outfits[0],outfits[1],traveler_body_lease.stage);message(line);
    return 0;
}
static int traveler_body_lookup(void *ctx,const char *name,uint64_t pair[2]) {
    (void)ctx;uint64_t manager=0,type=0,vtable=0,payload=0,entry=0,again[2];
    if(!own_read(NULL,traveler_game+0x28,&manager,8) || !manager ||
        !own_read(NULL,traveler_body_image+0x403f050,&type,8) || !type ||
        !own_read(NULL,type,&vtable,8) || vtable!=traveler_body_image+0x5cd4c8)return 0;
    uintptr_t (*lookup)(uintptr_t,uintptr_t,const char *)=(void *)(uintptr_t)(traveler_body_image+0x2ce120);
    entry=lookup(manager,type,name);
    if(!entry || !own_read(NULL,entry+0x38,&payload,8) || !payload ||
        !own_read(NULL,payload+0xc8,pair,16) || !pair[0] || !pair[1] ||
        !own_read(NULL,payload+0xc8,again,16) || memcmp(pair,again,16))return 0;
    return !jc_cloth_texture_check(own_read,NULL,traveler_object+0x1e0,"textureAtlasA",pair[0],0);
}
static int traveler_body_select(const struct jc_actor_identity *id) {
    uint32_t outfit=UINT32_MAX;
    if(!traveler_live_identity(id) || !own_read(NULL,id->actor+0x70,&outfit,4) || outfit>6)return 0;
    if(traveler_body_outfit.selected==(unsigned)(outfit>3)+1)return 1;
    struct jc_cloth_graphics_token token={0};
    if(!jc_cloth_live_graphics_enter(&traveler_live->backend,&token))return 0;
    int code=jc_body_outfit_apply(NULL,&traveler_body_ops,traveler_body_lookup,id->actor+0x1e0,outfit,&traveler_body_outfit);
    if(!code)for(unsigned i=0;i<2;++i)
        if(jc_cloth_texture_bound(own_read,NULL,id->actor+0x1e0,i?"textureAtlasB":"textureAtlasA",
                traveler_body_outfit.pairs[outfit>3][i][0]))code=JC_FRAME_MATERIAL;
    if(!jc_cloth_live_graphics_leave(&traveler_live->backend,&token))code=JC_FRAME_OWNERSHIP;
    char line[160];snprintf(line,sizeof(line),"[JNET] body outfit=%u color=%s result=%d",outfit,outfit>3?"white":"red",code);message(line);
    return !code;
}
static int traveler_body_release(uint64_t actor) {
    if(!traveler_body_lease.material)return 1;
    uint64_t model=1,material=1;
    if(traveler_body_thread!=GetCurrentThreadId() ||
        !own_read(NULL,actor+0x168,&model,8) || model ||
        !own_read(NULL,actor+0x210,&material,8) || material)return 0;
    int code=jc_body_appearance_release(NULL,&traveler_body_ops,actor+0x1e0,1,&traveler_body_lease);
    if(!code)code=jc_body_outfit_release(NULL,&traveler_body_ops,1,&traveler_body_outfit);
    if(!code)message("[JNET] body texture references released after native material destruction");
    return !code;
}
static int traveler_live_begin(void *context,const struct jc_actor_identity *id,
        unsigned phase,uint64_t frame,uint64_t render) {
    if (!traveler_live_ready(context,id) || !frame || phase<JC_ACTOR_PHASE_INPUT ||
        phase>JC_ACTOR_PHASE_FINISH) return 0;
    if (phase==JC_ACTOR_PHASE_UPDATE && (!traveler_body_prepare(id) || !traveler_body_select(id))) return 0;
    if (phase==JC_ACTOR_PHASE_RENDER) {
        uint64_t native_context=0;
        if (!render || !own_read(NULL,id->game+0x98,&native_context,8) || native_context!=render) return 0;
    }
    return 1;
}
static int traveler_live_end(void *context,const struct jc_actor_identity *id,
        unsigned phase,uint64_t frame,int success) {
    (void)context;(void)phase;(void)frame;
    if (!success || !traveler_live_identity(id)) {
        if (traveler_live) traveler_live->backend.failed=1;
        return 0;
    }
    return 1;
}
static void traveler_live_drain(void *context,const struct jc_actor_identity *id) {
    (void)context;
    if (traveler_live_identity(id)) {
        if(!cloth_actor_growth_stop())traveler_live->backend.failed=1;
        traveler_live->backend.frames.closing=1;
    }
}
static int traveler_live_material_rebuild(struct jc_cloth_live_material *m) {
    if(!traveler_live || !traveler_actor || !traveler_live_identity(&traveler_actor->identity) ||
        !m || m->pins || !m->ready || m->quarantined)return JC_FRAME_OWNERSHIP;
    for(unsigned i=0;i<6;++i)if(m->atlas_snapshots[i].pins)return JC_FRAME_OWNERSHIP;
    uint64_t barn=0,manager=0,image=m->image,peer=m->peer,generation=m->generation;
    unsigned role=m->role;
    if(!own_read(NULL,traveler_game+0x108,&barn,8) || !barn ||
        !own_read(NULL,traveler_game+0x28,&manager,8) || !manager)return JC_FRAME_OWNERSHIP;
    struct jc_cloth_graphics_token token={0};
    if(!jc_cloth_live_graphics_enter(&traveler_live->backend,&token))return JC_FRAME_OWNERSHIP;
    int code=jc_cloth_live_material_destroy(own_read,NULL,m);
    if(!code)code=jc_cloth_live_material_create(own_read,NULL,image,barn,manager,role,peer,generation,m);
    if(!jc_cloth_live_graphics_leave(&traveler_live->backend,&token))code=JC_FRAME_OWNERSHIP;
    if(code)traveler_live->backend.failed=1;
    char line[160];snprintf(line,sizeof(line),"[JNET] retired cloth material rebuilt role=%u generation=%llu result=%d",
        role,(unsigned long long)generation,code);message(line);
    return code;
}
static int traveler_live_material_reset(struct jc_cloth_live_material *m) {
    if(!traveler_actor || !traveler_live_retired(NULL,&traveler_actor->identity))return JC_FRAME_OWNERSHIP;
    return traveler_live_material_rebuild(m);
}
static int traveler_live_workers(void *context,uint64_t generation) {
    (void)context;
    return cloth_producer_pass_done(NULL,generation,cloth_producer.pass);
}
static int traveler_live_reject_borrowed_texture(void *context,uint64_t texture) {
    (void)context;(void)texture;return 0;
}
static void traveler_live_release_borrowed_texture(void *context,uint64_t texture) {
    (void)context;(void)texture;
    if (traveler_live) traveler_live->backend.failed=1;
}
/* Native object must already be retained. Only the temporary startup profile
   with four private Remote material instances can satisfy this allocation. */
__declspec(dllexport) int JNETActorPrepare(uint64_t peer,uint64_t generation) {
    if(traveler_live || traveler_actor)return traveler_peer_replace(peer,generation);
    if (!peer || !generation || traveler_live || traveler_actor || active_cloth_backend ||
        JNETTraveler[0]!=3 || !traveler_context()) {
        char line[240];snprintf(line,sizeof(line),"[JNET] extra traveler admission declined stage=prepare peer=%u generation=%u backend=%u lifecycle=%ld/%ld/%ld context=%u",
            peer!=0,generation!=0,active_cloth_backend!=NULL,(long)JNETTraveler[0],(long)JNETTraveler[1],(long)JNETTraveler[2],traveler_context()!=0);
        message(line);return JC_ACTOR_RUNTIME_ARGUMENT;
    }
    uint64_t image=(uintptr_t)GetModuleHandleW(NULL),context=0,barn=0,manager=0;
    if (!own_read(NULL,image+JC_D3D_CONTEXT_SLOT_RVA,&context,8) || !context ||
        !own_read(NULL,traveler_game+0x108,&barn,8) || !barn ||
        !own_read(NULL,traveler_game+0x28,&manager,8) || !manager) return JC_ACTOR_READ;
    struct traveler_live_storage *p=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*p));
    if (!p) return JC_ACTOR_RUNTIME_BUSY;
    p->peer=peer;p->generation=generation;
    struct jc_cloth_live_ops ops={.workers_done=traveler_live_workers,
        .atlas_pin=traveler_live_reject_borrowed_texture,.atlas_unpin=traveler_live_release_borrowed_texture};
    int code=JNETClothInitialize(&p->backend,(ID3D11DeviceContext *)(uintptr_t)context,&ops);
    if (code) {
        if (active_cloth_backend!=&p->backend) HeapFree(GetProcessHeap(),0,p);
        else traveler_live=p;
        return code;
    }
    traveler_live=p;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(&p->backend,&token)) {p->backend.failed=1;return JC_FRAME_OWNERSHIP;}
    for (unsigned f=0;f<JC_CLOTH_FRAMES && !code;++f) for (unsigned r=0;r<2 && !code;++r) {
        code=jc_cloth_live_material_create(own_read,NULL,image,barn,manager,r+4,peer,generation,
            &p->materials[f][r]);
        if (!code) code=jc_cloth_live_register_material(&p->backend,&p->materials[f][r]);
    }
    if (!jc_cloth_live_graphics_leave(&p->backend,&token) || code) {
        p->backend.failed=1;return JC_FRAME_QUARANTINED;
    }
    struct jc_actor_runtime_ops services={.ready=traveler_live_ready,.retired=traveler_live_retired,
        .prepare=traveler_live_prepare,.begin=traveler_live_begin,.end=traveler_live_end,.drain=traveler_live_drain};
    return JNETActorActivate(peer,generation,&services);
}
#endif
