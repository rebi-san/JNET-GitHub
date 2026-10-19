#ifndef JC_NATIVE_CLOTH_PRODUCER_H
#define JC_NATIVE_CLOTH_PRODUCER_H
#include "native_cloth_frame.h"
#include "native_companion_cloth.h"

struct jc_cloth_producer_participant {
    uint64_t game,barn,dude,peer,generation;
    struct jc_cloth_material materials[2];
};
struct jc_cloth_producer_ops {
    void *context;
    int (*prepare)(void *,const struct jc_cloth_partition *,struct jc_cloth_stream[7]);
    void (*abandon)(void *,struct jc_cloth_stream[7]);
    int (*begin)(void *,uint64_t,const struct jc_cloth_partition_job *,size_t,const void *,
        const struct jc_cloth_stream[7],const struct jc_cloth_material *,unsigned,unsigned *);
    int (*configure)(void *,unsigned,const uint32_t[7]);
    int (*adopt)(void *,unsigned,uint64_t,unsigned);
    int (*cancel)(void *,unsigned);
};
struct jc_cloth_producer {
    struct jc_cloth_native_jobs jobs;
    struct jc_cloth_partition plan;
    struct jc_cloth_stream streams[7];
    struct jc_cloth_lease lease;
    uint64_t generation;
    uint32_t role_pass[7];
    unsigned index,pass,roles,stage,code,adopted,quarantined,detail;
};
static inline int jc_cloth_producer_owns_worker(jc_layout_reader read,void *ctx,
        const struct jc_cloth_producer_participant *p,const struct jc_cloth_lease *lease,
        const unsigned char entry[16],uint64_t args,uint64_t streams,uint64_t state) {
    uint64_t barn,players,type;unsigned char remote;
    if (!read || !p || !lease || !entry || !p->dude || !p->barn || !p->game ||
        (lease->state!=JC_CLOTH_LIVE && lease->state!=JC_CLOTH_QUARANTINED) ||
        lease->dude!=p->dude || lease->game!=p->game || lease->barn!=p->barn || !lease->collection ||
        args!=p->barn+0x669ac0 || streams!=p->barn+0x669960) return 0;
    unsigned slot=jc_cloth_u32(entry),role=jc_cloth_u32(entry+4);
    if (slot>=JC_CLOTH_PARTITION_SLOTS || (role!=4 && role!=5) ||
        state!=p->barn+0x270080+(uint64_t)slot*0xe0 || slot!=lease->slot[role==5 ? 0 : 1]) return 0;
    if (!jc_layout_field(read,ctx,p->game,0x108,&barn,8) || barn!=p->barn ||
        !jc_layout_field(read,ctx,p->game,0x1e0,&players,8) || !players ||
        p->dude==players+0xb0 || p->dude==players+0x146f0 ||
        !jc_layout_field(read,ctx,p->dude,0,&type,8) || type!=lease->image+0x5d0380 ||
        !jc_layout_field(read,ctx,p->dude,0x81,&remote,1) || remote!=1) return 0;
    struct jc_cloth_lease fresh=*lease;
    return !jc_cloth_capture(read,ctx,&fresh,1) && !memcmp(fresh.slot,lease->slot,sizeof(fresh.slot));
}
static inline int jc_cloth_producer_identity(jc_layout_reader read,void *ctx,uint64_t image,
        const struct jc_cloth_producer_participant *p) {
    uint64_t barn,players,type;uint32_t appearance[4];unsigned char remote;
    if (!p || !p->peer || !p->generation || !p->game || !p->barn || !p->dude ||
        !jc_layout_field(read,ctx,p->game,0x108,&barn,8) || barn!=p->barn ||
        !jc_layout_field(read,ctx,p->game,0x1e0,&players,8) || !players ||
        p->dude==players+0xb0 || p->dude==players+0x146f0 ||
        !jc_layout_field(read,ctx,p->dude,0,&type,8) || type!=image+0x5d0380 ||
        !jc_layout_field(read,ctx,p->dude,0x81,&remote,1) || remote!=1 ||
        !jc_layout_field(read,ctx,p->dude,0x68,appearance,sizeof(appearance))) return JC_FRAME_OWNERSHIP;
    for (unsigned i=0;i<2;++i) {
        const struct jc_cloth_material *m=p->materials+i;
        if (m->participant!=1 || m->role!=4+i || m->peer!=p->peer || m->generation!=p->generation ||
            m->scarf!=appearance[0] || m->scarf_capacity!=appearance[1] ||
            m->outfit!=appearance[2] || m->symbol!=appearance[3]) return JC_FRAME_MATERIAL;
    }
    return JC_FRAME_OK;
}
static inline int jc_cloth_producer_jobs_same(const struct jc_cloth_native_jobs *a,
        const struct jc_cloth_native_jobs *b) {
    return a->barn==b->barn && a->entries==b->entries && a->arguments==b->arguments &&
        a->streams==b->streams && a->bank==b->bank && a->pass==b->pass && a->roles==b->roles &&
        a->count==b->count && !memcmp(a->jobs,b->jobs,a->count*sizeof(*a->jobs)) &&
        !memcmp(a->entry,b->entry,a->count*sizeof(*a->entry)) && jc_cloth_worker_prefix_difference(a->prefix,b->prefix)==0x210;
}
static inline int jc_cloth_producer_prepare(jc_layout_reader read,void *ctx,uint64_t image,
        const struct jc_cloth_producer_participant *p,uint64_t generation,unsigned pass,
        const struct jc_cloth_producer_ops *ops,struct jc_cloth_producer *out) {
    if (!read || !p || !out || !ops || !ops->prepare || !ops->abandon || !ops->begin ||
        !ops->configure || !ops->adopt || !ops->cancel || !generation || pass>1) return JC_FRAME_ARGUMENT;
    memset(out,0,sizeof(*out));out->index=JC_CLOTH_FRAMES;out->pass=pass;out->generation=generation;
    out->stage=1;int code=jc_cloth_producer_identity(read,ctx,image,p);if (code) goto failed;
    if (!jc_layout_field(read,ctx,image,0x731f60,out->role_pass,sizeof(out->role_pass))) {
        code=JC_FRAME_OWNERSHIP;goto failed;
    }
    for (unsigned r=0;r<7;++r) if (out->role_pass[r]>1) { code=JC_FRAME_OWNERSHIP;goto failed; }
    if (out->role_pass[4]!=pass || out->role_pass[5]!=pass) { code=JC_FRAME_BUSY;goto failed; }
    out->roles=(1u<<4)|(1u<<5);
    out->lease=(struct jc_cloth_lease){.image=image,.game=p->game,.barn=p->barn,.dude=p->dude,
        .generation=p->generation};
    out->stage=2;
    code=jc_cloth_capture(read,ctx,&out->lease,1);
    if (code) { out->detail=out->lease.capture_detail;code=JC_FRAME_OWNERSHIP;goto failed; }
    out->lease.state=JC_CLOTH_LIVE;
    unsigned char owners[JC_CLOTH_PARTITION_SLOTS]={0};
    owners[out->lease.slot[0]]=owners[out->lease.slot[1]]=1;
    out->stage=3;
    out->detail=jc_cloth_native_jobs_read(read,ctx,image,p->barn,pass,out->roles,owners,&out->jobs);
    if (out->detail) {
        code=JC_FRAME_OWNERSHIP;goto failed;
    }
    unsigned owned=0;uint32_t capacity[7];
    for (unsigned r=0;r<7;++r) capacity[r]=INT32_MAX;
    for (unsigned i=0;i<out->jobs.count;++i) if (out->jobs.jobs[i].participant) {
        const struct jc_cloth_partition_job *j=out->jobs.jobs+i;
        if (j->participant!=1 || (j->slot!=out->lease.slot[0] && j->slot!=out->lease.slot[1])) {
            code=JC_FRAME_OWNERSHIP;goto failed;
        }
        owned|=1u<<j->role;
    }
    if (owned!=out->roles || jc_cloth_partition_make(out->jobs.jobs,out->jobs.count,capacity,capacity,&out->plan)) {
        code=JC_FRAME_OWNERSHIP;goto failed;
    }
    for (unsigned r=4;r<=5;++r)
        if (!out->plan.vertex_capacity[r] || !out->plan.index_capacity[r]) {
            code=JC_FRAME_BUSY;goto failed;
        }
    if (!out->plan.ranges[1][4].indices && !out->plan.ranges[1][5].indices) {
        code=JC_FRAME_BUSY;goto failed;
    }
    out->stage=4;code=ops->prepare(ops->context,&out->plan,out->streams);if (code) goto failed;
    out->stage=5;
    struct jc_cloth_lease fresh_lease=out->lease;struct jc_cloth_native_jobs fresh_jobs;
    if (jc_cloth_producer_identity(read,ctx,image,p) || jc_cloth_capture(read,ctx,&fresh_lease,1) ||
        memcmp(out->lease.slot,fresh_lease.slot,sizeof(fresh_lease.slot)) ||
        jc_cloth_native_jobs_read(read,ctx,image,p->barn,pass,out->roles,owners,&fresh_jobs) ||
        !jc_cloth_producer_jobs_same(&out->jobs,&fresh_jobs)) {
        code=JC_FRAME_OWNERSHIP;goto abandon;
    }
    out->stage=6;code=ops->begin(ops->context,generation,out->jobs.jobs,out->jobs.count,
        (void *)(uintptr_t)out->jobs.arguments,out->streams,p->materials,2,&out->index);
    if (code) goto abandon;
    if (out->index>=JC_CLOTH_FRAMES) { code=JC_FRAME_QUARANTINED;goto quarantine; }
    out->stage=7;code=ops->configure(ops->context,out->index,out->role_pass);if (code) goto cancel;
    out->stage=8;code=ops->adopt(ops->context,out->index,generation,pass);
    if (code==JC_FRAME_QUARANTINED) goto quarantine;
    if (code) goto cancel;
    out->adopted=1;out->stage=0;out->code=JC_FRAME_OK;return JC_FRAME_OK;
cancel:
    if (ops->cancel(ops->context,out->index)) goto quarantine;
abandon:
    ops->abandon(ops->context,out->streams);
failed:
    out->code=code;return code;
quarantine:
    out->quarantined=1;out->code=JC_FRAME_QUARANTINED;return JC_FRAME_QUARANTINED;
}

#ifdef _WIN32
#include "native_cloth_live.h"
struct jc_cloth_producer_live {
    struct jc_cloth_live *backend;
    void *context;
    int (*adopt)(void *,unsigned,uint64_t,unsigned);
    int (*pass_done)(void *,uint64_t,unsigned);
};
static inline int jc_cloth_producer_live_prepare(void *ctx,const struct jc_cloth_partition *plan,
        struct jc_cloth_stream streams[7]) {
    return jc_cloth_live_streams_prepare(((struct jc_cloth_producer_live *)ctx)->backend,plan,streams);
}
static inline void jc_cloth_producer_live_abandon(void *ctx,struct jc_cloth_stream streams[7]) {
    jc_cloth_live_streams_abandon(((struct jc_cloth_producer_live *)ctx)->backend,streams);
}
static inline int jc_cloth_producer_live_begin(void *ctx,uint64_t generation,
        const struct jc_cloth_partition_job *jobs,size_t count,const void *args,
        const struct jc_cloth_stream streams[7],const struct jc_cloth_material *materials,
        unsigned material_count,unsigned *index) {
    return jc_cloth_live_begin(((struct jc_cloth_producer_live *)ctx)->backend,generation,jobs,count,args,
        streams,materials,material_count,index);
}
static inline int jc_cloth_producer_live_configure(void *ctx,unsigned index,const uint32_t role_pass[7]) {
    struct jc_cloth_producer_live *s=ctx;
    return jc_cloth_frame_configure_passes(&s->backend->frames,index,role_pass,s->pass_done,s->context);
}
static inline int jc_cloth_producer_live_adopt(void *ctx,unsigned index,uint64_t generation,unsigned pass) {
    struct jc_cloth_producer_live *s=ctx;return s->adopt(s->context,index,generation,pass);
}
static inline int jc_cloth_producer_live_cancel(void *ctx,unsigned index) {
    struct jc_cloth_producer_live *s=ctx;
    if (!jc_cloth_live_owner(s->backend) || index>=JC_CLOTH_FRAMES) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->backend->frames.frames+index;
    if (f->phase!=JC_FRAME_WRITING || f->stock_committed || f->pass_committed ||
        atomic_load_explicit(&f->route.completed,memory_order_acquire)) return JC_FRAME_BUSY;
    for (unsigned slot=0;slot<JC_CLOTH_PARTITION_SLOTS;++slot)
        if (atomic_load_explicit(f->route.claimed+slot,memory_order_acquire)) return JC_FRAME_BUSY;
    jc_cloth_frame_release(&s->backend->frames,f);return JC_FRAME_OK;
}
static inline struct jc_cloth_producer_ops jc_cloth_producer_live_ops(struct jc_cloth_producer_live *s) {
    return (struct jc_cloth_producer_ops){s,jc_cloth_producer_live_prepare,jc_cloth_producer_live_abandon,
        jc_cloth_producer_live_begin,jc_cloth_producer_live_configure,jc_cloth_producer_live_adopt,
        jc_cloth_producer_live_cancel};
}
#endif
#endif
