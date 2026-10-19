#ifndef JC_NATIVE_SPAWN_PREFLIGHT_H
#define JC_NATIVE_SPAWN_PREFLIGHT_H
#include "native_layout.h"
/* Necessary checks for a future adapter, not permission to initialize/render/free. */
struct jc_clone_snapshot { uint64_t type, slot, resource, payload; };
static inline int jc_clone_snapshot_read(jc_layout_reader read, void *context,
                                        uint64_t image, uint64_t mesh,
                                        struct jc_clone_snapshot *out) {
    uint64_t expected;
    memset(out,0,sizeof(*out));
    if (!jc_layout_address(image,0x67f3f8,&expected)) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,context,mesh,0,&out->type,8) ||
        !jc_layout_field(read,context,mesh,0x30,&out->slot,8)) return JC_LAYOUT_READ;
    if (out->type!=expected) return JC_LAYOUT_TYPE;
    if (!out->slot) return JC_LAYOUT_OK;
    if ((out->slot & 7) || !jc_layout_field(read,context,out->slot,0x10,&out->resource,8)) return JC_LAYOUT_READ;
    if (!out->resource) return JC_LAYOUT_OK;
    if ((out->resource & 7) || !jc_layout_field(read,context,out->resource,0x18,&out->payload,8)) return JC_LAYOUT_READ;
    uint64_t end;
    if (out->payload && ((out->payload & 7) || !jc_layout_address(out->payload,0x48,&end))) return JC_LAYOUT_RANGE;
    return JC_LAYOUT_OK;
}
static inline int jc_clone_snapshot_equal(const struct jc_clone_snapshot *a,
                                          const struct jc_clone_snapshot *b) {
    return a->type==b->type && a->slot==b->slot && a->resource==b->resource && a->payload==b->payload;
}
static inline int jc_clone_prefix_disjoint(uint64_t a, uint64_t b, uint64_t size) {
    uint64_t ae, be;
    return jc_layout_address(a,size,&ae) && jc_layout_address(b,size,&be) && (ae<=b || be<=a);
}
/* Fixed three objects only; never follows lists or dereferences animation arrays. */
static inline int jc_three_clone_check(jc_layout_reader read, void *context,
                                      uint64_t image, const uint64_t meshes[3], int *distinct) {
    struct jc_clone_snapshot before[3], after;
    *distinct=0;
    for (int i=0;i<3;++i) {
        int status=jc_clone_snapshot_read(read,context,image,meshes[i],&before[i]);
        if (status) return status;
    }
    int valid=1;
    for (int i=0;i<3;++i) {
        valid=valid && before[i].slot && before[i].resource && before[i].payload;
        for (int j=0;j<i;++j)
            valid=valid && meshes[i]!=meshes[j] && jc_clone_prefix_disjoint(before[i].slot,before[j].slot,0x20) &&
                  jc_clone_prefix_disjoint(before[i].resource,before[j].resource,0x20) &&
                  jc_clone_prefix_disjoint(before[i].payload,before[j].payload,0x48);
        int status=jc_clone_snapshot_read(read,context,image,meshes[i],&after);
        if (status) return status;
        if (!jc_clone_snapshot_equal(&before[i],&after)) return JC_LAYOUT_CHANGED;
    }
    *distinct=valid;
    return JC_LAYOUT_OK; /* Distinct top-level pointers are not full ownership proof. */
}
struct jc_symbol_reservation { uint64_t pool, entry; uint32_t count; };
static inline int jc_symbol_prepare(jc_layout_reader read, void *context, uint64_t pool,
                                    struct jc_symbol_reservation *out) {
    uint64_t end;
    memset(out,0,sizeof(*out));
    if (!jc_layout_address(pool,0x434014,&end)) return JC_LAYOUT_RANGE;
    uint32_t count;
    if (!jc_layout_field(read,context,pool,0x434010,&count,4)) return JC_LAYOUT_READ;
    if (count>=1024) return JC_LAYOUT_RANGE;
    out->pool=pool; out->count=count;
    return jc_layout_address(pool,0x10+(uint64_t)count*0x10d0,&out->entry) ? JC_LAYOUT_OK : JC_LAYOUT_RANGE;
}
/* Observation only: no count decrement, compaction, unlinking or freeing. */
static inline int jc_symbol_check_append(jc_layout_reader read, void *context,
                                        const struct jc_symbol_reservation *before,
                                        uint64_t current_pool, uint64_t dude) {
    uint64_t symbol, expected;
    uint32_t count;
    if (before->count>=1024 || !jc_layout_address(before->pool,0x10+(uint64_t)before->count*0x10d0,&expected) ||
        expected!=before->entry) return JC_LAYOUT_RANGE;
    if (current_pool!=before->pool) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,context,current_pool,0x434010,&count,4) ||
        !jc_layout_field(read,context,dude,0x2240,&symbol,8)) return JC_LAYOUT_READ;
    return count==before->count+1 && symbol==before->entry ? JC_LAYOUT_OK : JC_LAYOUT_CHANGED;
}
#endif
