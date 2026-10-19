#ifndef JC_NATIVE_LAYOUT_H
#define JC_NATIVE_LAYOUT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Only for the hash-pinned build; evidence is documented in CompanionLab/README.md. */
typedef int (*jc_layout_reader)(void *, uint64_t, void *, size_t);
enum { JC_LAYOUT_OK = 0, JC_LAYOUT_READ = 1, JC_LAYOUT_TYPE = 2,
       JC_LAYOUT_CHANGED = 3, JC_LAYOUT_RANGE = 4 };
struct jc_layout_result {
    int remote_gate, local_position_finite, remote_position_finite;
    unsigned char gates[3], blocked;
    uint32_t local_chapter, remote_chapter;
};
static inline int jc_layout_address(uint64_t base, uint64_t offset, uint64_t *out) {
    if (offset > UINT64_C(0x00007fffffffffff) || base < 65536 || base > UINT64_C(0x00007fffffffffff) - offset) return 0;
    *out = base + offset;
    return 1;
}
static inline int jc_layout_field(jc_layout_reader reader, void *context, uint64_t base,
                                  uint64_t offset, void *out, size_t size) {
    uint64_t address;
    return jc_layout_address(base, offset, &address) && size &&
        address <= UINT64_C(0x00007fffffffffff) - size && reader(context, address, out, size);
}
static inline int jc_layout_finite(const uint32_t position[4]) {
    for (int i=0; i<3; ++i) if ((position[i] & 0x7f800000u) == 0x7f800000u) return 0;
    return 1;
}
static inline int jc_native_layout(jc_layout_reader reader, void *context, uint64_t image,
                                    uint64_t game, struct jc_layout_result *out) {
    uint64_t barn, again, barn_type, local_type, remote_type, expected_barn, expected_dude;
    uint32_t chapter_a, chapter_b, local_pos[4], remote_pos[4];
    unsigned char gates[3], blocked, gates_after[3], blocked_after;
    memset(out, 0, sizeof(*out));
    if (!jc_layout_address(image, 0x5da340, &expected_barn) ||
        !jc_layout_address(image, 0x5d0380, &expected_dude)) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(reader, context, game, 0x1e0, &barn, 8) ||
        !jc_layout_field(reader, context, barn, 0, &barn_type, 8)) return JC_LAYOUT_READ;
    if (barn_type != expected_barn) return JC_LAYOUT_TYPE;
    if (!jc_layout_field(reader, context, barn, 0xb0, &local_type, 8) ||
        !jc_layout_field(reader, context, barn, 0x146f0, &remote_type, 8)) return JC_LAYOUT_READ;
    if (local_type != expected_dude || remote_type != expected_dude) return JC_LAYOUT_TYPE;
    if (!jc_layout_field(reader, context, barn, 0xa2, gates, 3) ||
        !jc_layout_field(reader, context, barn, 0x1670c, &blocked, 1) ||
        !jc_layout_field(reader, context, barn, 0x14, &chapter_a, 4) ||
        !jc_layout_field(reader, context, barn, 0x64, &chapter_b, 4) ||
        !jc_layout_field(reader, context, barn, 0xc0, local_pos, 16)) return JC_LAYOUT_READ;
    out->remote_gate = (gates[2] || gates[0]) && !blocked && chapter_a == chapter_b;
    memcpy(out->gates,gates,3); out->blocked=blocked;
    out->local_chapter=chapter_a; out->remote_chapter=chapter_b;
    out->local_position_finite = jc_layout_finite(local_pos);
    if (out->remote_gate) {
        if (!jc_layout_field(reader, context, barn, 0x14700, remote_pos, 16)) return JC_LAYOUT_READ;
        out->remote_position_finite = jc_layout_finite(remote_pos);
    }
    /* Detect some torn snapshots without claiming synchronization with the game. */
    if (!jc_layout_field(reader, context, game, 0x1e0, &again, 8)) return JC_LAYOUT_READ;
    if (again != barn) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(reader, context, barn, 0, &again, 8)) return JC_LAYOUT_READ;
    if (again != barn_type) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(reader, context, barn, 0xa2, gates_after, 3) ||
        !jc_layout_field(reader, context, barn, 0x1670c, &blocked_after, 1)) return JC_LAYOUT_READ;
    if (memcmp(gates, gates_after, 3) || blocked != blocked_after) return JC_LAYOUT_CHANGED;
    uint32_t value;
    if (!jc_layout_field(reader, context, barn, 0x14, &value, 4)) return JC_LAYOUT_READ;
    if (value != chapter_a) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(reader, context, barn, 0x64, &value, 4)) return JC_LAYOUT_READ;
    if (value != chapter_b) return JC_LAYOUT_CHANGED;
    return JC_LAYOUT_OK;
}
#endif
