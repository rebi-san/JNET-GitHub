#ifndef JC_NATIVE_LIFETIME_H
#define JC_NATIVE_LIFETIME_H
#include "native_allocation.h"
struct jc_lifetime {
    uint64_t object, game, barn, thread;
    int state; /* 0 idle, 1 retained, 2 released, 3 releasing, -1 quarantined */
};
static inline void jc_lifetime_quarantine(struct jc_lifetime *life) {
    life->object=0; life->state=-1;
}
/* Called only when the game's local Dude enters its normal resource cleanup. */
static inline int jc_lifetime_cleanup(struct jc_lifetime *life,
                                     const struct jc_allocation_ops *ops, uint64_t image,
                                     uint64_t game, uint64_t thread, uint64_t cleanup_dude) {
    if (life->state!=1 || cleanup_dude!=life->barn+0xb0) return life->state;
    uint64_t current_barn;
    if (game!=life->game || thread!=life->thread ||
        !jc_layout_field(ops->read,ops->context,game,0x1e0,&current_barn,8) ||
        current_barn!=life->barn ||
        jc_allocation_validate(ops,image,life->barn,life->object)!=JC_ALLOC_OK) {
        jc_lifetime_quarantine(life); return life->state;
    }
    uint64_t object=life->object;
    life->object=0; life->state=3; /* Consume ownership before a possibly reentrant call. */
    ops->destroy(ops->context,object);
    life->state=2;
    return life->state;
}
#endif
