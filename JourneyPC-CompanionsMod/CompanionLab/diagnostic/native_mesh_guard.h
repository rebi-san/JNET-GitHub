#ifndef JC_NATIVE_MESH_GUARD_H
#define JC_NATIVE_MESH_GUARD_H
#include "native_layout.h"
enum { JC_MESH_GUARD_CONTEXT=1, JC_MESH_GUARD_STATE=2, JC_MESH_GUARD_EARLY=3,
       JC_MESH_GUARD_READY=4, JC_MESH_GUARD_CALLER=5 };
/* 0 forward unrelated mesh, 1 defer early owned callback, 2 permit ready owned
   callback, -1 reject owned callback. Caller owns exact mesh/thread lifetime. */
static inline int jc_mesh_guard_detail(uint64_t owned, uint64_t mesh, int matching_context,
                                uint64_t caller_rva, uint64_t entries, uint64_t count,
                                int deferred, int forwarded, int *failure) {
    *failure=0;
    if (!owned || mesh!=owned) return 0;
    if (!matching_context) { *failure=JC_MESH_GUARD_CONTEXT; return -1; }
    if (forwarded || deferred<0 || deferred>1) { *failure=JC_MESH_GUARD_STATE; return -1; }
    if (caller_rva==0x260eb2) {
        if (!deferred && !entries && !count) return 1;
        *failure=JC_MESH_GUARD_EARLY; return -1;
    }
    uint64_t end;
    if (caller_rva==0x263076) {
        if (count && count<=4096 && !(entries&7) &&
            jc_layout_address(entries,count*0x50,&end)) return 2;
        *failure=JC_MESH_GUARD_READY; return -1;
    }
    *failure=JC_MESH_GUARD_CALLER;
    return -1;
}
static inline int jc_mesh_guard(uint64_t owned,uint64_t mesh,int matching_context,
    uint64_t caller_rva,uint64_t entries,uint64_t count,int deferred,int forwarded) {
    int failure;
    return jc_mesh_guard_detail(owned,mesh,matching_context,caller_rva,entries,count,deferred,forwarded,&failure);
}
static inline int jc_mesh_guard_complete(int deferred,int forwarded) {
    return (deferred==0 || deferred==1) && forwarded==1;
}
#endif
