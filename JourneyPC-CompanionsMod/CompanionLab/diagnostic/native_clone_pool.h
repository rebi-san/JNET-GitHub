#ifndef JC_NATIVE_CLONE_POOL_H
#define JC_NATIVE_CLONE_POOL_H
#include "native_layout.h"
struct jc_pool_result { uint32_t slots, capacity, occupied, free_slots; };
/* Fixed metadata and at most 64 list heads, twice; never follows a list. */
static inline int jc_clone_pool(jc_layout_reader read, void *ctx, uint64_t payload,
                                const uint64_t meshes[2], struct jc_pool_result *out) {
    uint64_t table, count, end, own[2], heads[64], again;
    uint32_t capacity, cap_after;
    struct jc_pool_result result={0};
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(read,ctx,payload,0xa8,&table,8) ||
        !jc_layout_field(read,ctx,payload,0xb0,&count,8) ||
        !jc_layout_field(read,ctx,payload,0xd8,&capacity,4)) return JC_LAYOUT_READ;
    if (!count || count>64 || capacity<count || capacity>64 || (table&7) ||
        !jc_layout_address(table,count*0x20,&end)) return JC_LAYOUT_RANGE;
    for (int j=0;j<2;++j) {
        if (!jc_layout_field(read,ctx,meshes[j],0x30,&own[j],8)) return JC_LAYOUT_READ;
        if (own[j]<table || own[j]>=end || (own[j]-table)%0x20) return JC_LAYOUT_TYPE;
    }
    if (own[0]==own[1]) return JC_LAYOUT_TYPE;
    for (uint64_t i=0;i<count;++i) {
        if (!jc_layout_field(read,ctx,table,i*0x20,&heads[i],8)) return JC_LAYOUT_READ;
        if (heads[i]) ++result.occupied; else ++result.free_slots;
        for (int j=0;j<2;++j) {
            uint64_t node;
            if (!jc_layout_address(meshes[j],0x30,&node)) return JC_LAYOUT_RANGE;
            if (own[j]==table+i*0x20 && heads[i]!=node) return JC_LAYOUT_TYPE;
        }
    }
    for (uint64_t i=0;i<count;++i) {
        if (!jc_layout_field(read,ctx,table,i*0x20,&again,8)) return JC_LAYOUT_READ;
        if (again!=heads[i]) return JC_LAYOUT_CHANGED;
    }
    for (int j=0;j<2;++j) {
        if (!jc_layout_field(read,ctx,meshes[j],0x30,&again,8)) return JC_LAYOUT_READ;
        if (again!=own[j]) return JC_LAYOUT_CHANGED;
    }
    if (!jc_layout_field(read,ctx,payload,0xa8,&again,8)) return JC_LAYOUT_READ;
    if (again!=table) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,payload,0xb0,&again,8)) return JC_LAYOUT_READ;
    if (again!=count) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,payload,0xd8,&cap_after,4)) return JC_LAYOUT_READ;
    if (cap_after!=capacity) return JC_LAYOUT_CHANGED;
    result.slots=(uint32_t)count; result.capacity=capacity; *out=result;
    return JC_LAYOUT_OK;
}
#endif
