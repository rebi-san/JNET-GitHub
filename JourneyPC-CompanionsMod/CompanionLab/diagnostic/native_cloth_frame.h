#ifndef JC_NATIVE_CLOTH_FRAME_H
#define JC_NATIVE_CLOTH_FRAME_H
#include "native_cloth_partition.h"
#include "native_cloth_draw.h"

#define JC_CLOTH_FRAMES 4u
#define JC_CLOTH_DRAWS ((JC_CLOTH_PARTICIPANTS-1)*2u)
#define JC_CLOTH_REFS (JC_CLOTH_ROLES*2u+JC_CLOTH_DRAWS*8u)
enum { JC_FRAME_OK,JC_FRAME_ARGUMENT,JC_FRAME_BUSY,JC_FRAME_OWNERSHIP,
       JC_FRAME_MATERIAL,JC_FRAME_RETAIN,JC_FRAME_WORKERS,JC_FRAME_QUEUE,
       JC_FRAME_QUARANTINED };
enum { JC_FRAME_FREE,JC_FRAME_WRITING,JC_FRAME_READY,JC_FRAME_QUEUED,JC_FRAME_UNKNOWN };
enum { JC_FRAME_VERTEX,JC_FRAME_INDEX,JC_FRAME_MATERIAL_REF,JC_FRAME_TEXTURE };
struct jc_cloth_stream {
    uint64_t vertex,index;
    uint32_t vertex_bytes,index_bytes,vertex_offset,index_offset,vertices,indices;
    void *mapped_vertices,*mapped_indices;
};
struct jc_cloth_material {
    /* Atlas A/B/mask, detail, then scarf filled A/B (8a7a0). */
    uint64_t peer,generation,material,uniforms,atlas[6];
    uint64_t environment;
    uint32_t uniform_bytes,participant,role,outfit,symbol,scarf,scarf_capacity;
};
struct jc_cloth_frame_draw {
    struct jc_cloth_index_view index;
    struct jc_cloth_material material;
    uint64_t vertex;
    uint32_t vertex_offset,count;
};
struct jc_cloth_frame_ref { uint64_t object; unsigned kind; };
struct jc_cloth_frame {
    struct jc_cloth_route route;
    struct jc_cloth_stream streams[JC_CLOTH_ROLES];
    struct jc_cloth_frame_draw draws[JC_CLOTH_DRAWS];
    struct jc_cloth_frame_ref refs[JC_CLOTH_REFS];
    uint64_t generation,ticket;
    const void *source_args;
    unsigned phase,ref_count,draw_count,stock_committed;
    unsigned pass_mode,pass_roles[2],pass_required,pass_sealed,pass_committed,pass_finalized,pass_finalizing;
    int (*pass_done)(void *,uint64_t,unsigned);
    void *pass_context;
};
/* Control functions run on one owner thread; only route_run is concurrent.
   Acquire must retain the real resource AND lease its storage against native
   reuse. A COM AddRef alone is insufficient. Material acquisition freezes its
   bindings/uniforms through release. Worker completion covers canceled jobs as
   well. Publish: 1 = accepted with a ticket, 0 = nothing queued, -1 = uncertain.
   Retired means BOTH command consumption and GPU use have ended. The native
   backend is deliberately explicit: no stock job flag substitutes for it. */
struct jc_cloth_frame_backend {
    void *context;
    int (*acquire)(void *,unsigned,uint64_t);
    void (*release)(void *,unsigned,uint64_t);
    int (*workers_done)(void *,uint64_t);
    int (*publish)(void *,uint64_t,const struct jc_cloth_frame_draw *,unsigned,uint64_t *);
    int (*retired)(void *,uint64_t);
};
struct jc_cloth_frames {
    struct jc_cloth_frame frames[JC_CLOTH_FRAMES];
    struct jc_cloth_frame_backend backend;
    uint64_t last_generation;
    unsigned closing;
};
static inline int jc_cloth_span(uint64_t p,uint64_t n) { return p && n && n<=UINT64_MAX-p; }
static inline int jc_cloth_overlap(uint64_t a,uint64_t an,uint64_t b,uint64_t bn) {
    return a<b+bn && b<a+an;
}
static inline int jc_cloth_frames_init(struct jc_cloth_frames *s,struct jc_cloth_frame_backend b) {
    if (!s || !b.acquire || !b.release || !b.workers_done || !b.publish || !b.retired)
        return JC_FRAME_ARGUMENT;
    /* Only for fresh storage, never a reset of a live scheduler. */
    memset(s,0,sizeof(*s)); s->backend=b; return JC_FRAME_OK;
}
static inline void jc_cloth_frame_release(struct jc_cloth_frames *s,struct jc_cloth_frame *f) {
    while (f->ref_count) {
        const struct jc_cloth_frame_ref *r=&f->refs[--f->ref_count];
        s->backend.release(s->backend.context,r->kind,r->object);
    }
    f->phase=JC_FRAME_FREE; f->draw_count=0; f->ticket=0; f->stock_committed=0;
    f->pass_mode=f->pass_required=f->pass_sealed=f->pass_committed=f->pass_finalized=f->pass_finalizing=0;
    f->pass_done=NULL; f->pass_context=NULL;
}
static inline int jc_cloth_frame_acquire(struct jc_cloth_frames *s,struct jc_cloth_frame *f,
        unsigned kind,uint64_t object) {
    if (f->ref_count==JC_CLOTH_REFS || !s->backend.acquire(s->backend.context,kind,object)) return 0;
    f->refs[f->ref_count++]=(struct jc_cloth_frame_ref){object,kind}; return 1;
}
static inline int jc_cloth_frames_poll(struct jc_cloth_frames *s) {
    if (!s || !s->backend.retired) return JC_FRAME_ARGUMENT;
    int result=JC_FRAME_OK;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) {
        struct jc_cloth_frame *f=s->frames+i;
        if (f->phase==JC_FRAME_UNKNOWN) { result=JC_FRAME_QUARANTINED; continue; }
        if (f->phase!=JC_FRAME_QUEUED) continue;
        int retired=s->backend.retired(s->backend.context,f->ticket);
        if (retired==1) jc_cloth_frame_release(s,f);
        else if (retired<0) { f->phase=JC_FRAME_UNKNOWN; result=JC_FRAME_QUARANTINED; }
    }
    return result;
}
static inline int jc_cloth_frame_material_conflict(const struct jc_cloth_material *a,
        const struct jc_cloth_material *b) {
    return a->material==b->material ||
        jc_cloth_overlap(a->uniforms,a->uniform_bytes,b->uniforms,b->uniform_bytes);
}
static inline int jc_cloth_frame_probe_alias(const struct jc_cloth_material *a,
        const struct jc_cloth_material *b) {
    for (unsigned i=0;i<(b->role==5 ? 6u : 4u);++i)
        if (a->environment==b->atlas[i]) return 1;
    return 0;
}
static inline int jc_cloth_stream_conflict(const struct jc_cloth_stream *a,const struct jc_cloth_stream *b) {
    uint64_t av=(uintptr_t)a->mapped_vertices,ai=(uintptr_t)a->mapped_indices;
    uint64_t bv=(uintptr_t)b->mapped_vertices,bi=(uintptr_t)b->mapped_indices;
    return a->vertex==b->vertex || a->index==b->index || a->vertex==b->index || a->index==b->vertex ||
        jc_cloth_overlap(av,(uint64_t)a->vertices*60,bv,(uint64_t)b->vertices*60) ||
        jc_cloth_overlap(av,(uint64_t)a->vertices*60,bi,(uint64_t)b->indices*4) ||
        jc_cloth_overlap(ai,(uint64_t)a->indices*4,bv,(uint64_t)b->vertices*60) ||
        jc_cloth_overlap(ai,(uint64_t)a->indices*4,bi,(uint64_t)b->indices*4);
}
static inline int jc_cloth_frame_begin(struct jc_cloth_frames *s,uint64_t generation,
        const struct jc_cloth_partition_job *jobs,size_t count,const void *native_args,
        const struct jc_cloth_stream streams[7],const struct jc_cloth_material *materials,
        unsigned material_count,unsigned *frame_index) {
    if (!s || !s->backend.acquire || !generation || generation<=s->last_generation ||
        !streams || !native_args || !frame_index || material_count>JC_CLOTH_DRAWS ||
        (!materials && material_count)) return JC_FRAME_ARGUMENT;
    if (s->closing) return JC_FRAME_BUSY;
    if ((!jobs && count) || count>JC_CLOTH_PARTITION_SLOTS) return JC_FRAME_ARGUMENT;
    unsigned admitted_roles=0;
    for (size_t i=0;i<count;++i) {
        if (jobs[i].role>=7) return JC_FRAME_ARGUMENT;
        admitted_roles|=1u<<jobs[i].role;
    }
    for (unsigned r=0;r<7;++r) if (admitted_roles&(1u<<r))
        if (jc_cloth_u32((const unsigned char *)native_args+0x210+r*4) ||
            jc_cloth_u32((const unsigned char *)native_args+0x22c+r*4)) return JC_FRAME_WORKERS;
    if (jc_cloth_frames_poll(s)) return JC_FRAME_QUARANTINED;
    for (unsigned k=0;k<JC_CLOTH_FRAMES;++k)
        if (s->frames[k].phase==JC_FRAME_WRITING || s->frames[k].phase==JC_FRAME_READY)
            return JC_FRAME_BUSY;
    unsigned index=0;
    while (index<JC_CLOTH_FRAMES && s->frames[index].phase!=JC_FRAME_FREE) ++index;
    if (index==JC_CLOTH_FRAMES) return JC_FRAME_BUSY;
    struct jc_cloth_frame *f=s->frames+index;
    uint32_t vc[7],ic[7];
    for (unsigned r=0;r<7;++r) { vc[r]=streams[r].vertices; ic[r]=streams[r].indices; }
    f->route.last_generation=s->last_generation;
    if (jc_cloth_route_prepare(&f->route,generation,jobs,count,native_args,
            JC_CLOTH_WORKER_ARGS,vc,ic)) return JC_FRAME_ARGUMENT;
    f->draw_count=0;
    for (unsigned r=0;r<7;++r) {
        if (!f->route.plan.vertex_capacity[r] && !f->route.plan.index_capacity[r]) continue;
        const struct jc_cloth_stream *v=streams+r;
        if (!v->vertex || !v->index || !v->mapped_vertices || !v->mapped_indices ||
            !jc_cloth_span((uintptr_t)v->mapped_vertices,(uint64_t)v->vertices*60) ||
            !jc_cloth_span((uintptr_t)v->mapped_indices,(uint64_t)v->indices*4) ||
            jc_cloth_overlap((uintptr_t)v->mapped_vertices,(uint64_t)v->vertices*60,
                (uintptr_t)v->mapped_indices,(uint64_t)v->indices*4) ||
            (v->index_offset&3) ||
            (uint64_t)v->vertex_offset+(uint64_t)v->vertices*60>v->vertex_bytes ||
            (uint64_t)v->index_offset+(uint64_t)v->indices*4>v->index_bytes)
            return JC_FRAME_OWNERSHIP;
        /* Conservatively prohibit reuse of either selected buffer, including
           across roles. An independent material cannot make overlapping GPU
           writes safe. Unused role descriptors are ignored. */
        for (unsigned other=0;other<r;++other) {
            if (!f->route.plan.vertex_capacity[other] && !f->route.plan.index_capacity[other]) continue;
            if (jc_cloth_stream_conflict(v,streams+other))
                return JC_FRAME_OWNERSHIP;
        }
        if (v->vertex==v->index) return JC_FRAME_OWNERSHIP;
        for (unsigned k=0;k<JC_CLOTH_FRAMES;++k) if (s->frames[k].phase!=JC_FRAME_FREE)
            for (unsigned n=0;n<7;++n) {
                if (!s->frames[k].route.plan.vertex_capacity[n] &&
                    !s->frames[k].route.plan.index_capacity[n]) continue;
                if (jc_cloth_stream_conflict(v,s->frames[k].streams+n))
                    return JC_FRAME_BUSY;
            }
    }
    unsigned seen[JC_CLOTH_PARTICIPANTS]={0};
    for (unsigned i=0;i<material_count;++i) {
        const struct jc_cloth_material *m=materials+i;
        if (!m->participant || m->participant>=JC_CLOTH_PARTICIPANTS || (m->role!=4 && m->role!=5) ||
            (seen[m->participant]&(1u<<m->role)) || !m->peer || !m->generation || !m->material ||
            !jc_cloth_span(m->uniforms,m->uniform_bytes) || !m->atlas[0] || !m->atlas[1] || !m->atlas[2] ||
            !m->atlas[3] || (m->role==5 && (!m->atlas[4] || !m->atlas[5])) ||
            !m->environment ||
            m->outfit>6 || m->symbol>20 || m->scarf>m->scarf_capacity || m->scarf_capacity>4096)
            return JC_FRAME_MATERIAL;
        if (jc_cloth_frame_probe_alias(m,m)) return JC_FRAME_MATERIAL;
        seen[m->participant]|=1u<<m->role;
        for (unsigned j=0;j<i;++j) {
            const struct jc_cloth_material *p=materials+j;
            if (jc_cloth_frame_material_conflict(m,p)) return JC_FRAME_MATERIAL;
            if (jc_cloth_frame_probe_alias(m,p) || jc_cloth_frame_probe_alias(p,m) ||
                ((m->participant==p->participant)!=(m->environment==p->environment)))
                return JC_FRAME_MATERIAL;
            if (m->participant==p->participant && (m->peer!=p->peer || m->generation!=p->generation ||
                m->outfit!=p->outfit || m->symbol!=p->symbol || m->scarf!=p->scarf ||
                m->scarf_capacity!=p->scarf_capacity)) return JC_FRAME_MATERIAL;
            if (m->participant!=p->participant && m->peer==p->peer) return JC_FRAME_MATERIAL;
        }
        for (unsigned k=0;k<JC_CLOTH_FRAMES;++k) if (s->frames[k].phase!=JC_FRAME_FREE)
            for (unsigned j=0;j<s->frames[k].draw_count;++j)
                if (jc_cloth_frame_material_conflict(m,&s->frames[k].draws[j].material) ||
                    m->environment==s->frames[k].draws[j].material.environment ||
                    jc_cloth_frame_probe_alias(m,&s->frames[k].draws[j].material) ||
                    jc_cloth_frame_probe_alias(&s->frames[k].draws[j].material,m)) return JC_FRAME_BUSY;
        const struct jc_cloth_partition_range *p=&f->route.plan.ranges[m->participant][m->role];
        if (!p->indices) continue;
        struct jc_cloth_frame_draw *d=f->draws+f->draw_count;
        const struct jc_cloth_stream *v=streams+m->role;
        if (jc_cloth_index_view_make(v->index,v->index_bytes,v->index_offset,v->indices,
                p->index_begin,p->indices,&d->index)) return JC_FRAME_OWNERSHIP;
        d->material=*m; d->vertex=v->vertex; d->vertex_offset=v->vertex_offset; d->count=p->indices;
        ++f->draw_count;
    }
    for (unsigned p=1;p<JC_CLOTH_PARTICIPANTS;++p) for (unsigned r=4;r<=5;++r)
        if (f->route.plan.ranges[p][r].indices && !(seen[p]&(1u<<r))) return JC_FRAME_MATERIAL;
    memcpy(f->streams,streams,sizeof(f->streams)); f->generation=generation; f->source_args=native_args;
    for (unsigned r=0;r<7;++r) {
        if (!f->route.plan.vertex_capacity[r] && !f->route.plan.index_capacity[r]) continue;
        if (!jc_cloth_frame_acquire(s,f,JC_FRAME_VERTEX,streams[r].vertex) ||
            !jc_cloth_frame_acquire(s,f,JC_FRAME_INDEX,streams[r].index)) goto failed;
    }
    for (unsigned i=0;i<f->draw_count;++i) {
        const struct jc_cloth_material *m=&f->draws[i].material;
        if (!jc_cloth_frame_acquire(s,f,JC_FRAME_MATERIAL_REF,m->material)) goto failed;
        for (unsigned a=0;a<(m->role==5 ? 6u : 4u);++a)
            if (!jc_cloth_frame_acquire(s,f,JC_FRAME_TEXTURE,m->atlas[a])) goto failed;
        if (!jc_cloth_frame_acquire(s,f,JC_FRAME_TEXTURE,m->environment)) goto failed;
    }
    f->phase=JC_FRAME_WRITING; s->last_generation=generation; *frame_index=index;
    return JC_FRAME_OK;
failed:
    jc_cloth_frame_release(s,f); return JC_FRAME_RETAIN;
}
static inline int jc_cloth_frame_run(struct jc_cloth_frame *f,uint64_t generation,
        void *context,jc_cloth_worker_call call,void *worker,const unsigned char entry[10],
        void *streams,void *descriptor,unsigned char state[0xe0]) {
    if (!f || f->phase!=JC_FRAME_WRITING || f->generation!=generation || !entry || !streams)
        return JC_FRAME_ARGUMENT;
    unsigned role=jc_cloth_u32(entry+4);
    void *v=NULL,*i=NULL;
    if (role<7) {
        memcpy(&v,(unsigned char *)streams+0x10+role*8,8);
        memcpy(&i,(unsigned char *)streams+0x48+role*8,8);
    }
    if (role>=7 || v!=f->streams[role].mapped_vertices || i!=f->streams[role].mapped_indices) {
        atomic_store_explicit(&f->route.failed,JC_PARTITION_ARGUMENT,memory_order_release);
        return JC_FRAME_OWNERSHIP;
    }
    return jc_cloth_route_run(&f->route,generation,context,call,worker,entry,streams,descriptor,state)
        ? JC_FRAME_WORKERS : JC_FRAME_OK;
}
/* Foreign contexts/roles stay native. Once a role is redirected, refusal must
   suppress that worker rather than write through the stock counters. */
static inline int jc_cloth_frame_dispatch(struct jc_cloth_frame *f,uint64_t generation,
        unsigned roles,void *bound_streams,void *context,jc_cloth_worker_call call,
        void *worker,const unsigned char entry[10],void *args,void *streams,
        void *descriptor,unsigned char state[0xe0]) {
    if (!f || !roles || streams!=bound_streams) return 0;
    unsigned reason=0;
    if (!entry) { reason=1;goto declined; }
    unsigned role=jc_cloth_u32(entry+4);
    if (role>=JC_CLOTH_ROLES) { reason=2;goto declined; }
    if (!(roles&(1u<<role))) return 0;
    if (atomic_load_explicit(&f->route.failed,memory_order_acquire)) return -1;
    if (args!=f->source_args) { reason=3;goto declined; }
    if (f->generation!=generation) { reason=4;goto declined; }
    if (f->phase!=JC_FRAME_WRITING) { reason=5;goto declined; }
    unsigned difference=jc_cloth_worker_prefix_difference(args,f->route.args[0].bytes);
    if (difference!=0x210) { reason=6+(difference<<8);goto declined; }
    if (!jc_cloth_frame_run(f,generation,context,call,worker,entry,streams,descriptor,state)) return 1;
    {
        unsigned empty=0;
        atomic_compare_exchange_strong_explicit(&f->route.failed,&empty,JC_PARTITION_ARGUMENT,memory_order_release,memory_order_relaxed);
    }
    return -1;
declined:
    {
        unsigned empty=0;
        atomic_compare_exchange_strong_explicit(&f->route.rejection,&empty,reason,memory_order_release,memory_order_relaxed);
        empty=0;
        atomic_compare_exchange_strong_explicit(&f->route.failed,&empty,JC_PARTITION_ARGUMENT,memory_order_release,memory_order_relaxed);
    }
    return -1;
}
static inline int jc_cloth_frame_seal(struct jc_cloth_frames *s,unsigned index) {
    if (!s || index>=JC_CLOTH_FRAMES) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames+index;
    if (f->phase!=JC_FRAME_WRITING || f->pass_mode) return JC_FRAME_ARGUMENT;
    if (s->backend.workers_done(s->backend.context,f->generation)!=1) return JC_FRAME_BUSY;
    if (jc_cloth_route_finish(&f->route,f->generation)) return JC_FRAME_WORKERS;
    f->phase=JC_FRAME_READY; return JC_FRAME_OK;
}
static inline int jc_cloth_frame_configure_passes(struct jc_cloth_frames *s,unsigned index,
        const uint32_t role_pass[7],int (*done)(void *,uint64_t,unsigned),void *context) {
    if (!s || index>=JC_CLOTH_FRAMES || !role_pass || !done) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames+index;
    if (f->phase!=JC_FRAME_WRITING || f->pass_mode || s->closing) return JC_FRAME_ARGUMENT;
    unsigned roles[2]={0,0},used=0;
    for (unsigned slot=0;slot<JC_CLOTH_PARTITION_SLOTS;++slot)
        if (f->route.present[slot]) used|=1u<<f->route.slots[slot].role;
    if (!used) return JC_FRAME_ARGUMENT;
    for (unsigned r=0;r<7;++r) {
        if (role_pass[r]>1) return JC_FRAME_ARGUMENT;
        if (used&(1u<<r)) roles[role_pass[r]]|=1u<<r;
    }
    for (unsigned slot=0;slot<JC_CLOTH_PARTITION_SLOTS;++slot)
        if (atomic_load_explicit(f->route.claimed+slot,memory_order_acquire)) return JC_FRAME_BUSY;
    if (atomic_load_explicit(&f->route.failed,memory_order_acquire)) return JC_FRAME_WORKERS;
    f->pass_roles[0]=roles[0]; f->pass_roles[1]=roles[1];
    f->pass_required=(roles[0] ? 1u : 0u)|(roles[1] ? 2u : 0u);
    f->pass_done=done; f->pass_context=context; f->pass_mode=1;
    atomic_store_explicit(&f->route.active_roles,roles[0] ? roles[0] : roles[1],memory_order_release);
    return JC_FRAME_OK;
}
static inline int jc_cloth_frame_seal_pass(struct jc_cloth_frames *s,unsigned index,unsigned pass) {
    if (!s || index>=JC_CLOTH_FRAMES || pass>1) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames+index;
    if (!f->pass_mode || !(f->pass_required&(1u<<pass)) || f->phase!=JC_FRAME_WRITING ||
        (f->pass_sealed&(1u<<pass)) ||
        (pass && (f->pass_required&1) && !(f->pass_finalized&1))) return JC_FRAME_ARGUMENT;
    int done=f->pass_done(f->pass_context,f->generation,pass);
    if (done<0) { f->phase=JC_FRAME_UNKNOWN; return JC_FRAME_QUARANTINED; }
    if (done!=1) return JC_FRAME_BUSY;
    if (jc_cloth_route_finish_roles(&f->route,f->generation,f->pass_roles[pass])) return JC_FRAME_WORKERS;
    atomic_store_explicit(&f->route.active_roles,0,memory_order_release);
    f->pass_sealed|=1u<<pass; return JC_FRAME_OK;
}
static inline int jc_cloth_frame_commit_pass(struct jc_cloth_frame *f,uint64_t generation,
        unsigned pass,void *native_args,size_t bytes) {
    if (!f || pass>1 || !f->pass_mode || f->phase!=JC_FRAME_WRITING || f->generation!=generation ||
        !(f->pass_sealed&(1u<<pass)) || (f->pass_committed&(1u<<pass)) ||
        !native_args || native_args!=f->source_args || bytes<JC_CLOTH_WORKER_ARGS)
        return JC_FRAME_ARGUMENT;
    if (jc_cloth_worker_prefix_difference(native_args,f->route.args[0].bytes)!=0x210) return JC_FRAME_WORKERS;
    unsigned roles=f->pass_roles[pass];
    for (unsigned r=0;r<7;++r) if (roles&(1u<<r))
        if (jc_cloth_u32((unsigned char *)native_args+0x210+4*r) ||
            jc_cloth_u32((unsigned char *)native_args+0x22c+4*r)) return JC_FRAME_WORKERS;
    for (unsigned r=0;r<7;++r) if (roles&(1u<<r)) {
        memcpy((unsigned char *)native_args+0x210+4*r,f->route.args[0].bytes+0x210+4*r,4);
        memcpy((unsigned char *)native_args+0x22c+4*r,f->route.args[0].bytes+0x22c+4*r,4);
    }
    f->pass_committed|=1u<<pass; return JC_FRAME_OK;
}
static inline int jc_cloth_frame_finalize_pass(struct jc_cloth_frames *s,unsigned index,unsigned pass,
        int (*finalize)(void *,uint64_t,unsigned),void *context) {
    if (!s || index>=JC_CLOTH_FRAMES || pass>1 || !finalize) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames+index;
    if (!f->pass_mode || f->phase!=JC_FRAME_WRITING || f->pass_finalizing ||
        !(f->pass_committed&(1u<<pass)) ||
        (f->pass_finalized&(1u<<pass))) return JC_FRAME_ARGUMENT;
    f->pass_finalizing=1;
    /* Pass0's native finalizer can release pass1 jobs before returning. */
    if (!pass) atomic_store_explicit(&f->route.active_roles,f->pass_roles[1],memory_order_release);
    int finished=finalize(context,f->generation,pass);
    f->pass_finalizing=0;
    if (finished!=1 || atomic_load_explicit(&f->route.failed,memory_order_acquire)) {
        atomic_store_explicit(&f->route.active_roles,0,memory_order_release);
        f->phase=JC_FRAME_UNKNOWN; return JC_FRAME_QUARANTINED;
    }
    f->pass_finalized|=1u<<pass;
    if (f->pass_finalized==f->pass_required) {
        if (jc_cloth_route_finish(&f->route,f->generation)) {
            f->phase=JC_FRAME_UNKNOWN; return JC_FRAME_QUARANTINED;
        }
        f->stock_committed=1; f->phase=JC_FRAME_READY;
    }
    return JC_FRAME_OK;
}
static inline int jc_cloth_frame_submit(struct jc_cloth_frames *s,unsigned index) {
    if (!s || index>=JC_CLOTH_FRAMES) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames+index;
    if (f->phase!=JC_FRAME_READY || !f->stock_committed) return JC_FRAME_ARGUMENT;
    int result=s->backend.publish(s->backend.context,f->generation,f->draws,f->draw_count,&f->ticket);
    if (result==1 && f->ticket) { f->phase=JC_FRAME_QUEUED; return JC_FRAME_OK; }
    if (result==0 && !f->ticket) return JC_FRAME_QUEUE;
    f->phase=JC_FRAME_UNKNOWN; return JC_FRAME_QUARANTINED;
}
static inline int jc_cloth_frame_cancel(struct jc_cloth_frames *s,unsigned index) {
    if (!s || index>=JC_CLOTH_FRAMES) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames+index;
    if (f->stock_committed || f->pass_committed) return JC_FRAME_BUSY;
    if (f->phase==JC_FRAME_WRITING && s->backend.workers_done(s->backend.context,f->generation)!=1)
        return JC_FRAME_BUSY;
    if (f->phase!=JC_FRAME_WRITING && f->phase!=JC_FRAME_READY) return JC_FRAME_BUSY;
    jc_cloth_frame_release(s,f); return JC_FRAME_OK;
}
/* Owner-thread boundary after seal and BEFORE stock 8a3a0 finalization.
   All native jobs must have been routed. The caller supplies the same still
   writable R8 block; no original worker may race this publication. Only stock
   actual counts go back, leaving extra absolute ends private. Once committed,
   hold resources until publish supplies a render ticket, even for zero extras. */
static inline int jc_cloth_frame_commit_stock(struct jc_cloth_frame *f,uint64_t generation,
        void *native_args,size_t bytes) {
    if (!f || f->pass_mode || f->phase!=JC_FRAME_READY || f->generation!=generation || f->stock_committed ||
        !native_args || native_args!=f->source_args || bytes<JC_CLOTH_WORKER_ARGS) return JC_FRAME_ARGUMENT;
    if (jc_cloth_worker_prefix_difference(native_args,f->route.args[0].bytes)!=0x210) return JC_FRAME_WORKERS;
    for (unsigned i=0;i<14;++i)
        if (jc_cloth_u32((unsigned char *)native_args+0x210+i*4)) return JC_FRAME_WORKERS;
    memcpy((unsigned char *)native_args+0x210,f->route.args[0].bytes+0x210,14*4);
    f->stock_committed=1; return JC_FRAME_OK;
}
/* Call before native chapter teardown, repeatedly on the owner thread while
   rendering still runs. Never wait here while holding a renderer/worker lock. */
static inline int jc_cloth_frames_drain(struct jc_cloth_frames *s) {
    if (!s || !s->backend.workers_done) return JC_FRAME_ARGUMENT;
    s->closing=1;
    int result=jc_cloth_frames_poll(s);
    if (result) return result;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) {
        struct jc_cloth_frame *f=s->frames+i;
        if (f->phase==JC_FRAME_FREE) continue;
        if ((f->phase==JC_FRAME_WRITING || f->phase==JC_FRAME_READY) &&
                !f->stock_committed && !f->pass_committed) {
            int done=s->backend.workers_done(s->backend.context,f->generation);
            if (done<0) { f->phase=JC_FRAME_UNKNOWN; return JC_FRAME_QUARANTINED; }
            if (done==1) { jc_cloth_frame_release(s,f); continue; }
        } else if (f->phase==JC_FRAME_READY && f->stock_committed) {
            int code=jc_cloth_frame_submit(s,i);
            if (code==JC_FRAME_QUARANTINED) return code;
        }
        result=JC_FRAME_BUSY;
    }
    return result;
}
static inline int jc_cloth_frames_reopen(struct jc_cloth_frames *s) {
    if (!s || !s->closing) return JC_FRAME_ARGUMENT;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i)
        if (s->frames[i].phase!=JC_FRAME_FREE) return JC_FRAME_BUSY;
    /* Keep monotonically increasing generations across chapters. */
    s->closing=0; return JC_FRAME_OK;
}
#endif
