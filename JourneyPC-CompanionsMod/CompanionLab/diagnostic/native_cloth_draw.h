#ifndef JC_NATIVE_CLOTH_DRAW_H
#define JC_NATIVE_CLOTH_DRAW_H
#include <stdint.h>
#include <string.h>
#include <limits.h>

/* Non-owning, immutable command input for PC 36d5b0/36d6a0 ONLY.
   Never pass to buffer constructors, mapping, release or destruction routines.
   The caller retains the selected GPU buffer and vertex stream through the
   render fence; copying a handle here does not acquire that ownership. */
struct jc_cloth_index_view { _Alignas(16) unsigned char bytes[0x40]; };
enum { JC_CLOTH_DRAW_OK,JC_CLOTH_DRAW_ARGUMENT,JC_CLOTH_DRAW_RANGE };
static inline int jc_cloth_index_view_make(uint64_t retained_buffer,uint32_t buffer_bytes,
        uint32_t mapped_offset,uint32_t mapped_indices,uint32_t first,uint32_t count,
        struct jc_cloth_index_view *out) {
    if (!out || !retained_buffer) return JC_CLOTH_DRAW_ARGUMENT;
    if (!count || count%3 || count>INT32_MAX || (mapped_offset&3) ||
        (uint64_t)first+count>mapped_indices ||
        (uint64_t)mapped_offset+(uint64_t)mapped_indices*4>buffer_bytes)
        return JC_CLOTH_DRAW_RANGE;
    struct jc_cloth_index_view value={0};
    uint32_t offset=mapped_offset+first*4,one=1;
    memcpy(value.bytes+8,&count,4);
    value.bytes[0xc]=8; /* native format mapped to 0x2a by both draw routines */
    memcpy(value.bytes+0x18,&one,4); /* one inline selected buffer; ring index 0 */
    memcpy(value.bytes+0x20,&retained_buffer,8);
    memcpy(value.bytes+0x38,&offset,4);
    value.bytes[0x3c]=1;
    *out=value; return JC_CLOTH_DRAW_OK;
}
#endif
