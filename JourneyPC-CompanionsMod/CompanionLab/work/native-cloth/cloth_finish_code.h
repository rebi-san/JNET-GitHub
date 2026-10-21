struct cloth_finish_call { uintptr_t barn,result,fourth,returned; unsigned roles,detail,called; };
static int cloth_finish_native(void *context,uint64_t generation,unsigned pass) {
    struct cloth_finish_call *call=context;
    if (generation!=cloth_maps.generation) { call->detail=1;return -1; }
    call->called=1;
    DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
    cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=1;
    call->returned=original_cloth_finish(call->barn,(void *)call->result,pass,call->fourth);
    cloth_production_thread=old_thread; cloth_production_pass=old_pass;
    unsigned char completed=0;
    if (call->returned!=call->result || !own_read(NULL,call->result,&completed,1) || completed!=1) {
        call->detail=2;return -1;
    }
    for (unsigned r=0;r<7;++r) if (call->roles&(1u<<r)) {
        uintptr_t objects[2],vertices=0,indices=0;
        unsigned detail=0;
        if (!cloth_map_objects(call->barn,r,objects) ||
            memcmp(objects,cloth_maps.objects[r],sizeof(objects))) detail=1;
        else if (jc_cloth_ring_finished(own_read,NULL,&cloth_maps.rings[r][0])) detail=2;
        else if (jc_cloth_ring_finished(own_read,NULL,&cloth_maps.rings[r][1])) detail=3;
        else if (!own_read(NULL,call->barn+0x669970+r*8,&vertices,8) || vertices) detail=4;
        else if (!own_read(NULL,call->barn+0x6699a8+r*8,&indices,8) || indices) detail=5;
        if (detail) { call->detail=16+r*8+detail;return -1; }
    }
    return 1;
}
static uintptr_t cloth_finish_observer(uintptr_t barn,void *result,unsigned pass,uintptr_t fourth) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_maps.roles || barn!=cloth_maps.barn || pass>1) {
        DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
        if (s && cloth_participant_ready && barn==cloth_participant.barn && pass==0) {
            cloth_production_thread=GetCurrentThreadId();cloth_production_pass=1;
        }
        uintptr_t returned=original_cloth_finish(barn,result,pass,fourth);
        cloth_production_thread=old_thread;cloth_production_pass=old_pass;
        return returned;
    }
    struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
    if (cloth_maps.finish_failed ||
        !(f->pass_required&(1u<<pass)) || (f->pass_finalized&(1u<<pass))) {
        DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
        cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=1;
        uintptr_t returned=original_cloth_finish(barn,result,pass,fourth);
        cloth_production_thread=old_thread; cloth_production_pass=old_pass;
        return returned;
    }
    unsigned char pending=0,done=0,busy=0; uintptr_t job=0;
    unsigned guard=0;
    if (!jc_cloth_live_owner(s)) guard=1;
    else if (f->phase!=JC_FRAME_WRITING) guard=2;
    else if (f->generation!=cloth_maps.generation) guard=3;
    else if (!own_read(NULL,barn+0x6bcf84,&pending,1)) guard=4;
    else if (pending!=1) guard=5;
    else if (!own_read(NULL,barn+0x6bcad0+pass,&done,1)) guard=6;
    else if (done) guard=7;
    else if (!own_read(NULL,barn+0x6bcef0,&job,8) || !job) guard=8;
    else if (!own_read(NULL,job+0x58,&busy,1)) guard=9;
    int valid=!guard;
    if (valid && busy) {
        unsigned char waiting[16]={0};
        memcpy(waiting+8,&job,8);
        if (jc_local_windows_write((uintptr_t)result,waiting,sizeof(waiting))) return (uintptr_t)result;
        valid=0;guard=10;
    }
    unsigned stage=100,phase=f->phase;
    int code=valid ? JC_FRAME_OK : JC_FRAME_OWNERSHIP;
    if (!code) { stage=101;code=jc_cloth_frame_seal_pass(&s->frames,cloth_maps.index,pass); }
    if (!code) { stage=102;code=jc_cloth_frame_commit_pass(f,f->generation,pass,(void *)f->source_args,JC_CLOTH_WORKER_ARGS); }
    struct cloth_finish_call call={.barn=barn,.result=(uintptr_t)result,.fourth=fourth,.roles=f->pass_roles[pass]};
    if (!code) { stage=103;code=jc_cloth_frame_finalize_pass(&s->frames,cloth_maps.index,pass,cloth_finish_native,&call); }
    if (!code && f->phase==JC_FRAME_READY) { stage=104;code=jc_cloth_live_unmap(s,cloth_maps.index); }
    if (code) {
        unsigned route=atomic_load_explicit(&f->route.failed,memory_order_acquire);
        unsigned rejection=atomic_load_explicit(&f->route.rejection,memory_order_acquire);
        unsigned detail=stage==100 ? guard : call.detail ? call.detail : rejection ? rejection : route;
        InterlockedExchange(&JCProbeCompanionFeatures[32],code);
        InterlockedExchange(&JCProbeCompanionFeatures[33],(LONG)stage);
        InterlockedExchange(&JCProbeCompanionFeatures[34],(LONG)detail);
        char line[384];
        snprintf(line,sizeof(line),"[JC] cloth finish failure code=%d stage=%u detail=%u pass=%u phase=%u pending=%u done=%u busy=%u route=%u completed=%u jobs=%u sealed=%u committed=%u finalized=%u native-called=%u",
            code,stage,detail,pass,phase,pending,done,busy,route,
            atomic_load_explicit(&f->route.completed,memory_order_acquire),(unsigned)f->route.jobs,
            f->pass_sealed,f->pass_committed,f->pass_finalized,call.called);
        message(line);
        for (unsigned owner=0;owner<JC_CLOTH_PARTICIPANTS;++owner)
            for (unsigned role=0;role<7;++role) if (f->pass_roles[pass]&(1u<<role)) {
                const struct jc_cloth_partition_range *range=&f->route.plan.ranges[owner][role];
                if (!range->jobs) continue;
                snprintf(line,sizeof(line),"[JC] cloth finish counters owner=%u role=%u vertices=%u/%u indices=%u/%u jobs=%u",
                    owner,role,jc_cloth_u32(f->route.args[owner].bytes+0x210+4*role),
                    range->vertex_begin+range->vertices,jc_cloth_u32(f->route.args[owner].bytes+0x22c+4*role),
                    range->index_begin+range->indices,range->jobs);
                message(line);
            }
        cloth_maps.finish_failed=1;
        s->failed=1; f->phase=JC_FRAME_UNKNOWN;cloth_participant_ready=0;
        atomic_store_explicit(&f->route.active_roles,0,memory_order_release);
        if (traveler_actor)
            traveler_actor_report(jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE));
        JCProbeActorTrial[0]=-1;JCProbeActorTrial[1]=JC_ACTOR_RUNTIME_SCOPE;
        /* Native completion and private publication are separate contracts.
           Keep all private storage pinned; never replace the native result with
           an incomplete result that its caller will poll forever. */
        return call.called ? call.returned : original_cloth_finish(barn,result,pass,fourth);
    }
    return call.returned;
}
