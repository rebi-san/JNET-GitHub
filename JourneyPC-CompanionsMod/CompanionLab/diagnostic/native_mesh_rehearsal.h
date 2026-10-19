#ifndef JC_NATIVE_MESH_REHEARSAL_H
#define JC_NATIVE_MESH_REHEARSAL_H
#include "native_allocation.h"
#include "native_spawn_preflight.h"
struct jc_mesh_ops {
    struct jc_allocation_ops allocation;
    void (*initialize)(void *, uint64_t);
    void (*cleanup)(void *, uint64_t);
};
struct jc_mesh_result { uint32_t counts[3]; int independent; };
static inline int jc_mesh_rehearse_or_retain(const struct jc_mesh_ops *ops, uint64_t image,
    uint64_t barn, struct jc_mesh_result *out, int (*retain)(void *,uint64_t)) {
    const struct jc_allocation_ops *a=&ops->allocation;
    uint64_t object;
    memset(out,0,sizeof(*out));
    int status=jc_allocation_create_checked(a,image,barn,&object);
    if (status!=JC_ALLOC_OK) return status;
    ops->initialize(a->context,object);
    uint64_t dudes[3]={barn+0xb0,barn+0x146f0,object};
    uint64_t meshes[3]={dudes[0]+0x160,dudes[1]+0x160,dudes[2]+0x160};
    int distinct=0;
    int valid=jc_three_clone_check(a->read,a->context,image,meshes,&distinct)==JC_LAYOUT_OK && distinct;
    uint64_t entries[3]={0}, counts[3]={0};
    for (int i=0;i<3;++i) {
        if (!jc_layout_field(a->read,a->context,dudes[i],0x6d8,&entries[i],8) ||
            !jc_layout_field(a->read,a->context,dudes[i],0x6e0,&counts[i],8) ||
            counts[i]==0 || counts[i]>4096) valid=0;
        if (counts[i]<=4096) out->counts[i]=(uint32_t)counts[i];
    }
    for (int i=0;i<3;++i) {
        uint64_t end;
        if (!counts[i] || counts[i]>4096 || (entries[i]&7) ||
            !jc_layout_address(entries[i],counts[i]*0x50,&end)) { valid=0; continue; }
        for (int j=0;j<i;++j) {
            uint64_t other_end;
            if (!counts[j] || counts[j]>4096 ||
                !jc_layout_address(entries[j],counts[j]*0x50,&other_end) ||
                (entries[i]<other_end && entries[j]<end)) valid=0;
        }
    }
    for (int i=0;i<3;++i) {
        uint64_t pointer, count;
        if (!jc_layout_field(a->read,a->context,dudes[i],0x6d8,&pointer,8) ||
            !jc_layout_field(a->read,a->context,dudes[i],0x6e0,&count,8) ||
            pointer!=entries[i] || count!=counts[i]) valid=0;
    }
    out->independent=valid;
    if (valid && retain && retain(a->context,object)) return 5;
    /* Known factory object only. Native cleanup owns unlinking; never edit a list. */
    ops->cleanup(a->context,object);
    if (jc_allocation_validate(a,image,barn,object)!=JC_ALLOC_OK) return JC_ALLOC_UNTRUSTED;
    a->destroy(a->context,object);
    out->independent=valid;
    return valid ? JC_ALLOC_OK : 4; /* Calls returned/cleaned, but independence not established. */
}
static inline int jc_mesh_rehearse(const struct jc_mesh_ops *ops,uint64_t image,
                                  uint64_t barn,struct jc_mesh_result *out) {
    return jc_mesh_rehearse_or_retain(ops,image,barn,out,NULL);
}
#endif
