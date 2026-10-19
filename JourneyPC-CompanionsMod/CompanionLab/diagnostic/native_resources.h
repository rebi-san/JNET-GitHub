#ifndef JC_NATIVE_RESOURCES_H
#define JC_NATIVE_RESOURCES_H
#include "native_layout.h"
/* Hash-pinned mesh fields; these checks do not establish full cloth ownership. */
struct jc_resource_slot { uint64_t type, entries, count, controller_mesh; };
struct jc_resource_result {
    uint32_t counts[2];
    int controller_owns_mesh[2];
    int tables_available, tables_overlap;
};
static inline int jc_resource_slot_read(jc_layout_reader reader, void *context,
                                        uint64_t dude, struct jc_resource_slot *slot) {
    return jc_layout_field(reader, context, dude, 0x160, &slot->type, 8) &&
        jc_layout_field(reader, context, dude, 0x6d8, &slot->entries, 8) &&
        jc_layout_field(reader, context, dude, 0x6e0, &slot->count, 8) &&
        jc_layout_field(reader, context, dude, 0xa620, &slot->controller_mesh, 8);
}
static inline int jc_native_resources(jc_layout_reader reader, void *context, uint64_t image,
                                      uint64_t game, struct jc_resource_result *out) {
    uint64_t barn, barn_type, expected_barn, expected_dude, expected_mesh, again;
    uint64_t dudes[2], mesh[2], ends[2] = {0, 0};
    struct jc_resource_slot slots[2], after;
    memset(out, 0, sizeof(*out));
    if (!jc_layout_address(image, 0x5da340, &expected_barn) ||
        !jc_layout_address(image, 0x5d0380, &expected_dude) ||
        !jc_layout_address(image, 0x67f3f8, &expected_mesh)) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(reader, context, game, 0x1e0, &barn, 8) ||
        !jc_layout_field(reader, context, barn, 0, &barn_type, 8)) return JC_LAYOUT_READ;
    if (barn_type != expected_barn) return JC_LAYOUT_TYPE;
    if (!jc_layout_address(barn, 0xb0, &dudes[0]) ||
        !jc_layout_address(barn, 0x146f0, &dudes[1])) return JC_LAYOUT_RANGE;
    for (int i=0; i<2; ++i) {
        uint64_t type;
        if (!jc_layout_field(reader, context, dudes[i], 0, &type, 8) ||
            !jc_resource_slot_read(reader, context, dudes[i], &slots[i])) return JC_LAYOUT_READ;
        if (type != expected_dude || slots[i].type != expected_mesh) return JC_LAYOUT_TYPE;
        if (!jc_layout_address(dudes[i], 0x160, &mesh[i])) return JC_LAYOUT_RANGE;
        /* Diagnostic cap only, not a recovered engine capacity. Never traverse entries. */
        if (slots[i].count > 4096) return JC_LAYOUT_RANGE;
        if (slots[i].count && (!slots[i].entries || (slots[i].entries & 7) ||
            !jc_layout_address(slots[i].entries, slots[i].count * 0x50, &ends[i]))) return JC_LAYOUT_RANGE;
        out->counts[i] = (uint32_t)slots[i].count;
        out->controller_owns_mesh[i] = slots[i].controller_mesh == mesh[i];
    }
    out->tables_available = slots[0].count && slots[1].count;
    if (out->tables_available)
        out->tables_overlap = slots[0].entries < ends[1] && slots[1].entries < ends[0];
    /* Recheck metadata only; no allocation dereference or atomicity guarantee. */
    for (int i=0; i<2; ++i) {
        uint64_t type;
        if (!jc_resource_slot_read(reader, context, dudes[i], &after) ||
            !jc_layout_field(reader, context, dudes[i], 0, &type, 8)) return JC_LAYOUT_READ;
        if (slots[i].type != after.type || slots[i].entries != after.entries ||
            slots[i].count != after.count || slots[i].controller_mesh != after.controller_mesh || type != expected_dude) return JC_LAYOUT_CHANGED;
    }
    if (!jc_layout_field(reader, context, game, 0x1e0, &again, 8)) return JC_LAYOUT_READ;
    if (again != barn) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(reader, context, barn, 0, &again, 8)) return JC_LAYOUT_READ;
    if (again != barn_type) return JC_LAYOUT_CHANGED;
    return JC_LAYOUT_OK;
}
#endif
