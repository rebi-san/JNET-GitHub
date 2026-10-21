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
    char line[160];snprintf(line,sizeof(line),"[JC] retired cloth material rebuilt role=%u generation=%llu result=%d",
        role,(unsigned long long)generation,code);message(line);
    return code;
}
static int traveler_live_material_reset(struct jc_cloth_live_material *m) {
    if(!traveler_actor || !traveler_live_retired(NULL,&traveler_actor->identity))return JC_FRAME_OWNERSHIP;
    return traveler_live_material_rebuild(m);
}
