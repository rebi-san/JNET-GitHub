#ifndef JC_NATIVE_CLOTH_PARTITION_H
#define JC_NATIVE_CLOTH_PARTITION_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdatomic.h>
#include "native_layout.h"

#define JC_CLOTH_PARTICIPANTS 16u
#define JC_CLOTH_ROLES 7u
#define JC_CLOTH_PARTITION_SLOTS 768u
/* Counts are the exact requests at 1e2e6b/1e2e7e (or 1e4acb/1e4ade),
   after the native optional subdivision adjustment. Participant zero is stock.
   Slot ownership and frame/worker fences must be established by the caller. */
struct jc_cloth_partition_job { uint32_t slot,participant,role,vertices,indices; };
struct jc_cloth_partition_range {
    uint32_t vertex_begin,vertex_capacity,index_begin,index_capacity;
    uint32_t vertices,indices,jobs;
};
struct jc_cloth_partition {
    struct jc_cloth_partition_range ranges[JC_CLOTH_PARTICIPANTS][JC_CLOTH_ROLES];
    uint32_t vertex_capacity[JC_CLOTH_ROLES],index_capacity[JC_CLOTH_ROLES];
    unsigned ready;
};
enum { JC_PARTITION_OK, JC_PARTITION_ARGUMENT, JC_PARTITION_SLOT,
       JC_PARTITION_ROLE, JC_PARTITION_CAPACITY, JC_PARTITION_INCOMPLETE };
/* Reserve disjoint ranges inside already sized role streams. This does not
   allocate buffers, redirect native workers, change stock counts, or submit draws. */
static inline int jc_cloth_partition_make(const struct jc_cloth_partition_job *jobs,size_t count,
        const uint32_t vertex_capacity[JC_CLOTH_ROLES],
        const uint32_t index_capacity[JC_CLOTH_ROLES],struct jc_cloth_partition *out) {
    if (!out) return JC_PARTITION_ARGUMENT;
    memset(out,0,sizeof(*out));
    if ((!jobs && count) || count>JC_CLOTH_PARTITION_SLOTS || !vertex_capacity || !index_capacity)
        return JC_PARTITION_ARGUMENT;
    struct jc_cloth_partition p={0}; uint32_t seen[JC_CLOTH_PARTITION_SLOTS/32]={0};
    for (size_t i=0;i<count;++i) {
        const struct jc_cloth_partition_job *j=jobs+i;
        if (j->slot>=JC_CLOTH_PARTITION_SLOTS || (seen[j->slot/32]&(1u<<(j->slot%32))))
            return JC_PARTITION_SLOT;
        seen[j->slot/32]|=1u<<(j->slot%32);
        if (j->participant>=JC_CLOTH_PARTICIPANTS || j->role>=JC_CLOTH_ROLES ||
            (j->participant && j->role!=4 && j->role!=5)) return JC_PARTITION_ROLE;
        if (j->vertices>INT32_MAX || j->indices>INT32_MAX) return JC_PARTITION_CAPACITY;
        struct jc_cloth_partition_range *r=&p.ranges[j->participant][j->role];
        /* Match the pre-worker capacity rounding at 8a190..8a1ba. Native
           counters reserve actual counts; padding stays outside each draw. */
        uint64_t vertices=((uint64_t)j->vertices+3)&~UINT64_C(3);
        uint64_t indices=((uint64_t)j->indices+23)/24*24;
        if (vertices+r->vertex_capacity>INT32_MAX || indices+r->index_capacity>INT32_MAX)
            return JC_PARTITION_CAPACITY;
        r->vertex_capacity+=(uint32_t)vertices; r->index_capacity+=(uint32_t)indices;
        r->vertices+=j->vertices; r->indices+=j->indices; ++r->jobs;
    }
    for (unsigned role=0;role<JC_CLOTH_ROLES;++role) {
        uint64_t v=0,i=0;
        if (vertex_capacity[role]>INT32_MAX || index_capacity[role]>INT32_MAX)
            return JC_PARTITION_CAPACITY;
        /* 8a482..8a490 publishes three zero indices when the stock counter is
           empty. Keep that native fallback outside every extra draw, even when
           there is no stock companion. The caller must provide this capacity. */
        if (!p.ranges[0][role].indices) {
            for (unsigned owner=1;owner<JC_CLOTH_PARTICIPANTS;++owner)
                if (p.ranges[owner][role].indices) { p.ranges[0][role].index_capacity=24; break; }
        }
        for (unsigned owner=0;owner<JC_CLOTH_PARTICIPANTS;++owner) {
            struct jc_cloth_partition_range *r=&p.ranges[owner][role];
            r->vertex_begin=(uint32_t)v; r->index_begin=(uint32_t)i;
            v+=r->vertex_capacity; i+=r->index_capacity;
            if (v>vertex_capacity[role] || i>index_capacity[role]) return JC_PARTITION_CAPACITY;
        }
        p.vertex_capacity[role]=(uint32_t)v; p.index_capacity[role]=(uint32_t)i;
    }
    p.ready=1; *out=p; return JC_PARTITION_OK;
}
/* Native xadd counters start at the range's absolute begins. Preserve the
   original stream bases, so the emitted vertex indices need no rebasing.
   This check is valid only AFTER native worker completion, never as a fence. */
static inline int jc_cloth_partition_finished(const struct jc_cloth_partition *p,
        unsigned participant,unsigned role,uint32_t vertex_counter,uint32_t index_counter) {
    if (!p || !p->ready || participant>=JC_CLOTH_PARTICIPANTS || role>=JC_CLOTH_ROLES)
        return JC_PARTITION_ARGUMENT;
    const struct jc_cloth_partition_range *r=&p->ranges[participant][role];
    return vertex_counter==r->vertex_begin+r->vertices &&
        index_counter==r->index_begin+r->indices ? JC_PARTITION_OK : JC_PARTITION_INCOMPLETE;
}

/* PC render-worker R8 prefix is read-only; +210/+22c are its seven xadd
   counters. Keep R9 unchanged: it contains shared streams and slot-owned
   simulation arrays. This adapter is not an engine job fence or a GPU owner. */
#define JC_CLOTH_WORKER_ARGS 0x248u
static inline unsigned jc_cloth_worker_prefix_difference(const void *left,const void *right) {
    const unsigned char *a=left,*b=right;
    for (unsigned i=0;i<0x210;++i) {
        if (i==0x150) i=0x160;
        if (a[i]!=b[i]) return i;
    }
    return 0x210;
}

struct jc_cloth_worker_args {
    _Alignas(16) unsigned char bytes[JC_CLOTH_WORKER_ARGS];
};
struct jc_cloth_route {
    struct jc_cloth_partition plan;
    struct jc_cloth_worker_args args[JC_CLOTH_PARTICIPANTS];
    struct jc_cloth_partition_job slots[JC_CLOTH_PARTITION_SLOTS];
    unsigned char present[JC_CLOTH_PARTITION_SLOTS];
    atomic_uint claimed[JC_CLOTH_PARTITION_SLOTS],completed,failed,active_roles,rejection;
    uint64_t generation,last_generation;
    size_t jobs;
};
typedef void (*jc_cloth_worker_call)(void *,void *,const void *,void *,void *,void *,void *);
static inline uint32_t jc_cloth_u32(const void *p) {
    uint32_t v; memcpy(&v,p,4); return v;
}
/* Inputs are readable snapshots, not arbitrary live addresses. Invisible entries
   reserve nothing, even if their state has stale subdivision pointers. */
static inline int jc_cloth_worker_counts(const unsigned char entry[10],
        const unsigned char state[0xe0],uint32_t participant,struct jc_cloth_partition_job *out) {
    if (!entry || !state || !out) return JC_PARTITION_ARGUMENT;
    struct jc_cloth_partition_job j={jc_cloth_u32(entry),participant,jc_cloth_u32(entry+4),0,0};
    if (j.slot>=JC_CLOTH_PARTITION_SLOTS) return JC_PARTITION_SLOT;
    if (participant>=JC_CLOTH_PARTICIPANTS || j.role>=JC_CLOTH_ROLES ||
        (participant && j.role!=4 && j.role!=5)) return JC_PARTITION_ROLE;
    if (entry[9]) {
        uint64_t v=jc_cloth_u32(state+0x54),i=jc_cloth_u32(state+0x5c);
        if (entry[8]) {
            uint64_t begin,end; memcpy(&begin,state+0xa0,8); memcpy(&end,state+0xa8,8);
            if (end<begin || (end-begin)%4) return JC_PARTITION_CAPACITY;
            v+=(end-begin)/4; i*=4;
        }
        if (v>INT32_MAX || i>INT32_MAX) return JC_PARTITION_CAPACITY;
        j.vertices=(uint32_t)v; j.indices=(uint32_t)i;
    }
    *out=j; return JC_PARTITION_OK;
}
struct jc_cloth_native_jobs {
    uint64_t barn,entries,arguments,streams;
    uint32_t bank,pass,roles,count;
    struct jc_cloth_partition_job jobs[JC_CLOTH_PARTITION_SLOTS];
    unsigned char entry[JC_CLOTH_PARTITION_SLOTS][16];
    unsigned char prefix[0x210];
};
/* Only at the native pre-launch dependency boundary. A repeated read detects
   changes; it is not a substitute for owning that scheduling boundary. */
static inline int jc_cloth_native_jobs_read(jc_layout_reader read,void *context,
        uint64_t image,uint64_t barn,unsigned pass,unsigned roles,
        const unsigned char owners[JC_CLOTH_PARTITION_SLOTS],struct jc_cloth_native_jobs *out) {
    if (!read || !owners || !out || pass>1 || !roles || (roles&~0x7fu)) return JC_PARTITION_ARGUMENT;
    uint32_t table[7],reserve,counts[3],bank;
    uint64_t bases[2];
    if (!jc_layout_field(read,context,image,0x731f60,table,sizeof(table)) ||
        !jc_layout_field(read,context,image,0x731f84,&reserve,4) ||
        !jc_layout_field(read,context,barn,0x666d60,&bank,4) || bank>1 ||
        !jc_layout_field(read,context,barn,0x669960,bases,sizeof(bases)) ||
        bases[0]!=barn+0x80 || bases[1]!=barn+0x270080) return JC_PARTITION_ARGUMENT;
    for (unsigned r=0;r<7;++r)
        if (table[r]>1 || ((roles&(1u<<r)) && table[r]!=pass)) return JC_PARTITION_ROLE;
    for (unsigned i=0;i<3;++i)
        if (!jc_layout_field(read,context,barn,0x6bca10+pass*0x60+i*0x20,&counts[i],4))
            return JC_PARTITION_ARGUMENT;
    if (counts[0]>JC_CLOTH_PARTITION_SLOTS || counts[1]!=counts[0] || counts[2]!=counts[0] ||
        reserve<JC_CLOTH_PARTITION_SLOTS || reserve>0x52d0 ||
        (!pass && reserve+counts[0]>0x52d0) || (pass && counts[0]>reserve)) return JC_PARTITION_CAPACITY;
    uint64_t entries=barn+0x669d10+(pass ? 0 : (uint64_t)reserve*16);
    out->barn=barn; out->entries=entries; out->arguments=barn+0x669ac0;
    out->streams=barn+0x669960; out->bank=bank; out->pass=pass; out->roles=roles; out->count=0;
    if (!jc_layout_field(read,context,out->arguments,0,out->prefix,sizeof(out->prefix)))
        return JC_PARTITION_ARGUMENT;
    unsigned char seen[JC_CLOTH_PARTITION_SLOTS]={0};
    for (unsigned i=0;i<counts[0];++i) {
        unsigned char entry[16],again[16],state[0xe0]; uint32_t descriptor_role,slot,role;
        if (!jc_layout_field(read,context,entries,i*16,entry,sizeof(entry))) return JC_PARTITION_ARGUMENT;
        slot=jc_cloth_u32(entry); role=jc_cloth_u32(entry+4);
        if (slot>=JC_CLOTH_PARTITION_SLOTS || seen[slot]++) return JC_PARTITION_SLOT;
        if (role>=7 || table[role]!=pass) return JC_PARTITION_ROLE;
        if (!(roles&(1u<<role))) continue;
        if (!jc_layout_field(read,context,barn,0x270080+(uint64_t)slot*0xe0,state,sizeof(state)) ||
            !jc_layout_field(read,context,barn,0xd14+(uint64_t)slot*0xd00,&descriptor_role,4) ||
            descriptor_role!=role || jc_cloth_u32(state+0x98)!=1 ||
            !jc_layout_field(read,context,entries,i*16,again,sizeof(again)) || memcmp(entry,again,16))
            return JC_PARTITION_ARGUMENT;
        int code=jc_cloth_worker_counts(entry,state,owners[slot],out->jobs+out->count);
        if (code) return code;
        memcpy(out->entry[out->count++],entry,16);
    }
    uint32_t after[7],value;
    unsigned char prefix[0x210];
    if (!jc_layout_field(read,context,image,0x731f60,after,sizeof(after)) || memcmp(table,after,sizeof(table)) ||
        !jc_layout_field(read,context,image,0x731f84,&value,4) || value!=reserve ||
        !jc_layout_field(read,context,barn,0x666d60,&value,4) || value!=bank ||
        !jc_layout_field(read,context,out->arguments,0,prefix,sizeof(prefix)) || jc_cloth_worker_prefix_difference(prefix,out->prefix)!=0x210)
        return JC_PARTITION_ARGUMENT;
    for (unsigned i=0;i<3;++i)
        if (!jc_layout_field(read,context,barn,0x6bca10+pass*0x60+i*0x20,&value,4) || value!=counts[i])
            return JC_PARTITION_ARGUMENT;
    return JC_PARTITION_OK;
}
/* Start with zero-initialized storage and strictly increasing generations.
   Caller must fence the previous frame BEFORE prepare and publish the immutable
   frame BEFORE any worker starts. Supplied capacities must be real mapped extents. */
static inline int jc_cloth_route_prepare(struct jc_cloth_route *r,uint64_t generation,
        const struct jc_cloth_partition_job *jobs,size_t count,const void *native_args,
        size_t args_bytes,const uint32_t vc[7],const uint32_t ic[7]) {
    if (!r) return JC_PARTITION_ARGUMENT;
    r->generation=0;
    if (!generation || generation<=r->last_generation || !native_args || args_bytes<JC_CLOTH_WORKER_ARGS)
        return JC_PARTITION_ARGUMENT;
    int code=jc_cloth_partition_make(jobs,count,vc,ic,&r->plan);
    if (code) return code;
    memset(r->present,0,sizeof(r->present));
    atomic_init(&r->completed,0); atomic_init(&r->failed,0); atomic_init(&r->rejection,0);
    atomic_init(&r->active_roles,0x7f);
    for (unsigned s=0;s<JC_CLOTH_PARTITION_SLOTS;++s) atomic_init(r->claimed+s,0);
    for (unsigned owner=0;owner<JC_CLOTH_PARTICIPANTS;++owner) {
        memcpy(r->args[owner].bytes,native_args,0x210);
        for (unsigned role=0;role<JC_CLOTH_ROLES;++role) {
            const struct jc_cloth_partition_range *p=&r->plan.ranges[owner][role];
            memcpy(r->args[owner].bytes+0x210+4*role,&p->vertex_begin,4);
            memcpy(r->args[owner].bytes+0x22c+4*role,&p->index_begin,4);
        }
    }
    for (size_t i=0;i<count;++i) {
        r->slots[jobs[i].slot]=jobs[i]; r->present[jobs[i].slot]=1;
    }
    r->jobs=count; r->generation=r->last_generation=generation;
    return JC_PARTITION_OK;
}
/* A rejected routed job must NOT fall through to the stock worker: doing so
   would write into another participant's range. Suppress frame publication. */
static inline int jc_cloth_route_run(struct jc_cloth_route *r,uint64_t generation,
        void *call_context,jc_cloth_worker_call call,void *worker_context,
        const unsigned char entry[10],void *streams,void *descriptor,unsigned char state[0xe0]) {
    if (!r || !generation || generation!=r->generation) return JC_PARTITION_ARGUMENT;
    if (!call || !entry || !streams || !descriptor || !state) {
        atomic_store_explicit(&r->failed,JC_PARTITION_ARGUMENT,memory_order_release);
        return JC_PARTITION_ARGUMENT;
    }
    int code=JC_PARTITION_SLOT;
    uint32_t slot=jc_cloth_u32(entry);
    if (atomic_load_explicit(&r->failed,memory_order_acquire)) return JC_PARTITION_INCOMPLETE;
    if (slot<JC_CLOTH_PARTITION_SLOTS && r->present[slot]) {
        const struct jc_cloth_partition_job *expected=&r->slots[slot];
        if (!(atomic_load_explicit(&r->active_roles,memory_order_acquire)&(1u<<expected->role))) {
            atomic_store_explicit(&r->failed,JC_PARTITION_ROLE,memory_order_release);
            return JC_PARTITION_ROLE;
        }
        struct jc_cloth_partition_job actual;
        code=jc_cloth_worker_counts(entry,state,expected->participant,&actual);
        if (!code && memcmp(&actual,expected,sizeof(actual))) code=JC_PARTITION_CAPACITY;
        unsigned unclaimed=0;
        if (!code && !atomic_compare_exchange_strong_explicit(r->claimed+slot,&unclaimed,1,
                memory_order_acq_rel,memory_order_acquire)) code=JC_PARTITION_SLOT;
        if (!code) {
            call(call_context,worker_context,entry,r->args[expected->participant].bytes,
                 streams,descriptor,state);
            atomic_store_explicit(r->claimed+slot,2,memory_order_release);
            atomic_fetch_add_explicit(&r->completed,1,memory_order_release);
            return JC_PARTITION_OK;
        }
    }
    atomic_store_explicit(&r->failed,(unsigned)code,memory_order_release);
    return code;
}
/* Only after the external native job fence. No polling of native non-C atomics. */
static inline int jc_cloth_route_finish_roles(const struct jc_cloth_route *r,uint64_t generation,
        unsigned roles) {
    if (!r || !generation || generation!=r->generation || (roles&~0x7fu))
        return JC_PARTITION_ARGUMENT;
    if (atomic_load_explicit(&r->failed,memory_order_acquire)) return JC_PARTITION_INCOMPLETE;
    for (unsigned slot=0;slot<JC_CLOTH_PARTITION_SLOTS;++slot)
        if (r->present[slot] && (roles&(1u<<r->slots[slot].role)) &&
            atomic_load_explicit(r->claimed+slot,memory_order_acquire)!=2)
            return JC_PARTITION_INCOMPLETE;
    for (unsigned owner=0;owner<JC_CLOTH_PARTICIPANTS;++owner)
        for (unsigned role=0;role<JC_CLOTH_ROLES;++role) if (roles&(1u<<role))
            if (jc_cloth_partition_finished(&r->plan,owner,role,
                    jc_cloth_u32(r->args[owner].bytes+0x210+4*role),
                    jc_cloth_u32(r->args[owner].bytes+0x22c+4*role)))
                return JC_PARTITION_INCOMPLETE;
    return JC_PARTITION_OK;
}
static inline int jc_cloth_route_finish(const struct jc_cloth_route *r,uint64_t generation) {
    if (!r || !generation || generation!=r->generation) return JC_PARTITION_ARGUMENT;
    if (atomic_load_explicit(&r->failed,memory_order_acquire) ||
        atomic_load_explicit(&r->completed,memory_order_acquire)!=r->jobs)
        return JC_PARTITION_INCOMPLETE;
    return jc_cloth_route_finish_roles(r,generation,0x7f);
}
#endif
