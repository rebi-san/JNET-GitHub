#ifndef JC_NATIVE_CLOTH_PRELAUNCH_H
#define JC_NATIVE_CLOTH_PRELAUNCH_H
#include "native_cloth_commands.h"

typedef int (*jc_cloth_prelaunch_write)(void *,uint64_t,const void *,size_t);
enum { JC_PRELAUNCH_OK,JC_PRELAUNCH_ARGUMENT,JC_PRELAUNCH_CHANGED,
       JC_PRELAUNCH_RESTORED,JC_PRELAUNCH_QUARANTINED };
struct jc_cloth_map_patch {
    uint64_t address;
    unsigned size;
    unsigned char before[12],after[12];
};
struct jc_cloth_prelaunch {
    uint64_t barn;
    unsigned roles,count,applied;
    uint64_t objects[7][2];
    struct jc_cloth_ring_checkpoint rings[7][2];
    struct jc_cloth_map_patch patches[28];
};
/* 89c95..cbA maps before 8a0b0 builds this frame's jobs. At 8a38f,
   both output records and stream pointers exist, but the owning dependency
   has not been released. Redirect both together, retaining native map/unmap. */
static inline int jc_cloth_prelaunch_capture(jc_layout_reader read,void *ctx,uint64_t barn,
        unsigned roles,const struct jc_cloth_stream streams[7],struct jc_cloth_prelaunch *out) {
    if (!read || !barn || !roles || (roles&~0x7fu) || !streams || !out)
        return JC_PRELAUNCH_ARGUMENT;
    struct jc_cloth_prelaunch v={.barn=barn,.roles=roles};
    for (unsigned r=0;r<7;++r) if (roles&(1u<<r)) {
        uint64_t slot,geometry;
        if (!jc_layout_field(read,ctx,barn,0x666e40+r*0x630,&slot,8) || !slot ||
            !jc_layout_field(read,ctx,slot,8,&geometry,8) || !geometry ||
            !jc_layout_field(read,ctx,geometry,0x28,&v.objects[r][0],8) || !v.objects[r][0] ||
            geometry>UINT64_MAX-0x30) return JC_PRELAUNCH_CHANGED;
        v.objects[r][1]=geometry+0x30;
        for (unsigned k=0;k<2;++k) {
            if (jc_cloth_ring_capture(read,ctx,v.objects[r][k],k,&v.rings[r][k]))
                return JC_PRELAUNCH_CHANGED;
            struct jc_cloth_map_patch *map=&v.patches[v.count++],*base=&v.patches[v.count++];
            map->address=barn+0x666e00+r*0x630+k*0x10;map->size=12;
            base->address=barn+0x669970+r*8+k*0x38;base->size=8;
            uint64_t pointer=(uintptr_t)(k ? streams[r].mapped_indices : streams[r].mapped_vertices);
            uint64_t bytes=(uint64_t)(k ? streams[r].indices : streams[r].vertices)*(k ? 4u : 60u);
            uint64_t original_pointer,original_base;uint32_t original_bytes;
            if (!pointer || !bytes || bytes>UINT32_MAX || pointer>UINT64_MAX-bytes ||
                !jc_layout_field(read,ctx,map->address,0,map->before,12) ||
                !jc_layout_field(read,ctx,base->address,0,base->before,8)) return JC_PRELAUNCH_CHANGED;
            memcpy(&original_pointer,map->before,8);memcpy(&original_bytes,map->before+8,4);
            memcpy(&original_base,base->before,8);
            if (!original_pointer || !original_bytes || original_base!=original_pointer ||
                original_pointer>UINT64_MAX-original_bytes ||
                jc_cloth_overlap(pointer,bytes,original_pointer,original_bytes)) return JC_PRELAUNCH_CHANGED;
            uint32_t capacity=(uint32_t)bytes;
            memcpy(map->after,&pointer,8);memcpy(map->after+8,&capacity,4);
            memcpy(base->after,&pointer,8);
        }
    }
    *out=v;return JC_PRELAUNCH_OK;
}
static inline int jc_cloth_prelaunch_restore(jc_layout_reader read,jc_cloth_prelaunch_write write,
        void *ctx,struct jc_cloth_prelaunch *s,unsigned touched) {
    int failed=0;unsigned char check[12];
    while (touched) {
        const struct jc_cloth_map_patch *p=&s->patches[--touched];
        if (!write(ctx,p->address,p->before,p->size) ||
            !jc_layout_field(read,ctx,p->address,0,check,p->size) || memcmp(check,p->before,p->size)) failed=1;
    }
    s->applied=failed ? 2u : 0u;
    return failed ? JC_PRELAUNCH_QUARANTINED : JC_PRELAUNCH_RESTORED;
}
static inline int jc_cloth_prelaunch_apply(jc_layout_reader read,jc_cloth_prelaunch_write write,
        void *ctx,struct jc_cloth_prelaunch *s) {
    if (!read || !write || !s || !s->count || s->count>28 || s->applied)
        return JC_PRELAUNCH_ARGUMENT;
    unsigned char check[12];
    for (unsigned r=0;r<7;++r) if (s->roles&(1u<<r)) {
        uint64_t slot,geometry,vertex;
        if (!jc_layout_field(read,ctx,s->barn,0x666e40+r*0x630,&slot,8) || !slot ||
            !jc_layout_field(read,ctx,slot,8,&geometry,8) || !geometry ||
            geometry>UINT64_MAX-0x30 || geometry+0x30!=s->objects[r][1] ||
            !jc_layout_field(read,ctx,geometry,0x28,&vertex,8) || vertex!=s->objects[r][0])
            return JC_PRELAUNCH_CHANGED;
        for (unsigned k=0;k<2;++k) {
            struct jc_cloth_ring_checkpoint now;
            const struct jc_cloth_ring_checkpoint *old=&s->rings[r][k];
            if (jc_cloth_ring_capture(read,ctx,s->objects[r][k],k,&now) ||
                now.object!=old->object || now.count!=old->count || now.next!=old->next || now.index!=old->index)
                return JC_PRELAUNCH_CHANGED;
        }
    }
    for (unsigned i=0;i<s->count;++i) {
        const struct jc_cloth_map_patch *p=&s->patches[i];
        if ((p->size!=8 && p->size!=12) || !jc_layout_field(read,ctx,p->address,0,check,p->size) ||
            memcmp(check,p->before,p->size)) return JC_PRELAUNCH_CHANGED;
    }
    for (unsigned i=0;i<s->count;++i) {
        const struct jc_cloth_map_patch *p=&s->patches[i];
        if (!write(ctx,p->address,p->after,p->size) ||
            !jc_layout_field(read,ctx,p->address,0,check,p->size) || memcmp(check,p->after,p->size))
            return jc_cloth_prelaunch_restore(read,write,ctx,s,i+1);
    }
    s->applied=1;return JC_PRELAUNCH_OK;
}
#endif
