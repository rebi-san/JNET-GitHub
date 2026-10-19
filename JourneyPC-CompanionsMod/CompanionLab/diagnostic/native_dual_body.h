#ifndef JC_NATIVE_DUAL_BODY_H
#define JC_NATIVE_DUAL_BODY_H
#include "native_initialized_lifetime.h"
#include "native_pose_storage.h"
struct jc_dual_pose_snapshot {
    struct jc_pose_storage storage;
    uint64_t control;
    float world[16],root[16];
};
static inline int jc_dual_pose_snapshot_read(jc_layout_reader read,void *ctx,uint64_t image,
    uint64_t object,struct jc_dual_pose_snapshot *out) {
    memset(out,0,sizeof(*out));
    int code=jc_pose_storage_read(read,ctx,image,object+0x160,&out->storage);
    if (code) return code;
    if (!jc_layout_field(read,ctx,out->storage.clone.payload,8,&out->control,8) ||
        !jc_layout_field(read,ctx,object+0x160,0x530,out->world,sizeof(out->world)) ||
        !jc_layout_field(read,ctx,out->storage.root,0x20,out->root,sizeof(out->root))) return JC_LAYOUT_READ;
    return out->control ? 0 : JC_LAYOUT_TYPE;
}
static inline int jc_dual_pose_same(const struct jc_dual_pose_snapshot *a,const struct jc_dual_pose_snapshot *b) {
    return jc_clone_snapshot_equal(&a->storage.clone,&b->storage.clone) &&
        a->storage.model==b->storage.model && a->storage.root==b->storage.root && a->control==b->control &&
        !memcmp(a->world,b->world,sizeof(a->world)) && !memcmp(a->root,b->root,sizeof(a->root));
}
static inline int jc_dual_owner_pair(const struct jc_initialized_lifetime life[2],
    const struct jc_initialized_owner now[2]) {
    if (life[0].state!=1 || life[1].state!=1 || !life[0].object || !life[1].object ||
        life[0].game!=life[1].game || life[0].barn!=life[1].barn || life[0].thread!=life[1].thread ||
        !jc_clone_prefix_disjoint(life[0].object,life[1].object,0x14640)) return 1;
    for (unsigned i=0;i<2;++i) {
        if (now[i].expected_slots!=4 || now[i].expected_occupied!=4 ||
            life[i].owner.expected_slots!=4 || life[i].owner.expected_occupied!=3+i ||
            jc_initialized_compare(&now[i],&life[i].owner) ||
            now[i].pool_table!=life[i].owner.pool_table || !now[i].model || !now[i].control) return 2;
    }
    if (!now[0].source || now[0].source!=now[1].source || !now[0].pool_table ||
        now[0].pool_table!=now[1].pool_table || now[0].slot==now[1].slot || now[0].control==now[1].control ||
        !jc_clone_prefix_disjoint(now[0].model,now[1].model,0x108) ||
        !jc_clone_prefix_disjoint(now[0].clone.slot,now[1].clone.slot,0x20) ||
        !jc_clone_prefix_disjoint(now[0].clone.resource,now[1].clone.resource,0x20) ||
        !jc_clone_prefix_disjoint(now[0].clone.payload,now[1].clone.payload,0x48)) return 3;
    unsigned added=0;
    for (unsigned slot=0;slot<4;++slot) {
        uint64_t address=now[0].pool_table+(uint64_t)slot*0x30;
        uint64_t head=now[0].pool_heads[slot];
        if (!head || head!=now[1].pool_heads[slot] || head!=life[1].owner.pool_heads[slot]) return 4;
        if (!life[0].owner.pool_heads[slot]) {
            if (address!=now[1].slot || head!=life[1].object+0x210) return 5;
            ++added;
        } else if (head!=life[0].owner.pool_heads[slot]) return 5;
    }
    if (added!=1) return 5;
    for (unsigned i=0;i<2;++i) {
        if (now[i].slot<now[i].pool_table || now[i].slot-now[i].pool_table>=4*0x30 ||
            (now[i].slot-now[i].pool_table)%0x30 ||
            now[i].pool_heads[(now[i].slot-now[i].pool_table)/0x30]!=life[i].object+0x210) return 6;
    }
    return 0;
}
#endif
