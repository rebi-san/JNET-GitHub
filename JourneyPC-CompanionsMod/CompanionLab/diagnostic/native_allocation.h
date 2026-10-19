#ifndef JC_NATIVE_ALLOCATION_H
#define JC_NATIVE_ALLOCATION_H
#include "native_layout.h"
/* Tests only a factory-created, unregistered Dude. Never initialize or tick it. */
enum { JC_ALLOC_OK=2, JC_ALLOC_NULL=-2, JC_ALLOC_UNTRUSTED=-3 };
struct jc_allocation_ops {
    jc_layout_reader read;
    uint64_t (*create)(void *);
    void (*destroy)(void *, uint64_t);
    void *context;
};
static inline int jc_allocation_validate(const struct jc_allocation_ops *ops,
                                        uint64_t image, uint64_t barn, uint64_t dude) {
    uint64_t barn_end, expected_dude, expected_mesh, expected_anim;
    if (!jc_layout_address(barn,0x28dd0,&barn_end) ||
        !jc_layout_address(image,0x5d0380,&expected_dude) ||
        !jc_layout_address(image,0x67f3f8,&expected_mesh) ||
        !jc_layout_address(image,0x5d0360,&expected_anim)) return JC_ALLOC_UNTRUSTED;
    uint64_t end;
    if (!dude) return JC_ALLOC_NULL;
    /* An unexpected pointer is quarantined until process exit, never blindly freed. */
    if ((dude & 15) || !jc_layout_address(dude,0x14640,&end) ||
        (dude < barn_end && barn < end)) return JC_ALLOC_UNTRUSTED;
    const uint64_t offsets[]={0,0x160,0x7b0,0xe50,0xa390};
    const uint64_t types[]={expected_dude,expected_mesh,expected_mesh,expected_mesh,expected_anim};
    for (size_t i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i) {
        uint64_t value;
        if (!jc_layout_field(ops->read,ops->context,dude,offsets[i],&value,8) || value!=types[i])
            return JC_ALLOC_UNTRUSTED;
    }
    /* These must still be unregistered: primary mesh lists/entries, cloth, effect nodes. */
    const uint64_t empty[]={0x168,0x190,0x6d8,0x6e0,0x2240,0x9b40,0x9b48,0x9bb0};
    for (size_t i=0;i<sizeof(empty)/sizeof(empty[0]);++i) {
        uint64_t value;
        if (!jc_layout_field(ops->read,ops->context,dude,empty[i],&value,8) || value)
            return JC_ALLOC_UNTRUSTED;
    }
    return JC_ALLOC_OK;
}
static inline int jc_allocation_create_checked(const struct jc_allocation_ops *ops,
                                               uint64_t image, uint64_t barn, uint64_t *out) {
    uint64_t end;
    *out=0;
    if (!jc_layout_address(barn,0x28dd0,&end) || !jc_layout_address(image,0x67f3f8,&end)) return JC_ALLOC_UNTRUSTED;
    uint64_t dude=ops->create(ops->context);
    int result=jc_allocation_validate(ops,image,barn,dude);
    if (result==JC_ALLOC_OK) *out=dude;
    return result;
}
static inline int jc_allocation_rehearse(const struct jc_allocation_ops *ops,
                                       uint64_t image, uint64_t barn) {
    uint64_t dude;
    int result=jc_allocation_create_checked(ops,image,barn,&dude);
    if (result==JC_ALLOC_OK) ops->destroy(ops->context,dude);
    return result;
}
#endif
