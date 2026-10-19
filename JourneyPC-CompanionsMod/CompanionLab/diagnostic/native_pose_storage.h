#ifndef JC_NATIVE_POSE_STORAGE_H
#define JC_NATIVE_POSE_STORAGE_H
#include "native_spawn_preflight.h"
struct jc_pose_storage { struct jc_clone_snapshot clone; uint64_t model, root; };
static inline int jc_pose_storage_read(jc_layout_reader read, void *context,
                                       uint64_t image, uint64_t mesh,
                                       struct jc_pose_storage *out) {
    memset(out,0,sizeof(*out));
    int code=jc_clone_snapshot_read(read,context,image,mesh,&out->clone);
    if (code) return code;
    if (!out->clone.payload) return JC_LAYOUT_TYPE;
    uint64_t end;
    if (!jc_layout_field(read,context,out->clone.payload,0,&out->model,8)) return JC_LAYOUT_READ;
    if ((out->model&7) || !jc_layout_address(out->model,0x108,&end)) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,context,out->model,0x100,&out->root,8)) return JC_LAYOUT_READ;
    if ((out->root&7) || !jc_layout_address(out->root,0x60,&end)) return JC_LAYOUT_RANGE;
    unsigned char transform[64];
    if (!jc_layout_field(read,context,out->root,0x20,transform,sizeof(transform))) return JC_LAYOUT_READ;
    return JC_LAYOUT_OK;
}
static inline int jc_three_pose_storage(jc_layout_reader read, void *context,
                                        uint64_t image, const uint64_t meshes[3],
                                        int *models_distinct, int *roots_distinct) {
    struct jc_pose_storage before[3], after;
    *models_distinct=*roots_distinct=-1;
    for (int i=0;i<3;++i) {
        int code=jc_pose_storage_read(read,context,image,meshes[i],&before[i]);
        if (code) return code;
    }
    int models=1,roots=1;
    for (int i=0;i<3;++i) {
        for (int j=0;j<i;++j) {
            models &= jc_clone_prefix_disjoint(before[i].model,before[j].model,0x108);
            roots &= jc_clone_prefix_disjoint(before[i].root,before[j].root,0x60);
            /* Different travelers must not overlap across either role. */
            if ((before[i].model<before[j].root+0x60 && before[j].root<before[i].model+0x108) ||
                (before[j].model<before[i].root+0x60 && before[i].root<before[j].model+0x108))
                models=roots=0;
        }
        int code=jc_pose_storage_read(read,context,image,meshes[i],&after);
        if (code) return code;
        if (!jc_clone_snapshot_equal(&before[i].clone,&after.clone) ||
            before[i].model!=after.model || before[i].root!=after.root) return JC_LAYOUT_CHANGED;
    }
    *models_distinct=models; *roots_distinct=roots;
    return JC_LAYOUT_OK;
}
#endif
