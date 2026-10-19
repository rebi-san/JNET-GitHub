#ifndef JC_REMOTE_PE_H
#define JC_REMOTE_PE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef int (*jc_image_reader)(void *, uint32_t, void *, size_t);
enum { JC_PE_INVALID = 0, JC_PE_FUNCTION = 1, JC_PE_FORWARD = 2, JC_PE_NOT_FOUND = 3 };
struct jc_export_result { uint32_t rva; char forward[128]; };

static inline uint16_t jc_u16(const unsigned char *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t jc_u32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline int jc_image_read(jc_image_reader reader, void *context, uint32_t size,
                                uint32_t rva, void *out, size_t length) {
    return length && length <= size && rva <= size - length && reader(context, rva, out, length);
}

static inline int jc_image_string(jc_image_reader reader, void *context, uint32_t size,
                                  uint32_t rva, char out[128]) {
    if (rva >= size) return 0;
    size_t length = size - rva;
    if (length > 128) length = 128;
    if (!jc_image_read(reader, context, size, rva, out, length)) return 0;
    return memchr(out, 0, length) != NULL;
}

static inline int jc_pe_export(jc_image_reader reader, void *context, uint32_t module_size,
                               const char *symbol, struct jc_export_result *out) {
    unsigned char dos[64], nt[264], directory[40], value[4];
    memset(out, 0, sizeof(*out));
    if (!symbol || !*symbol || !jc_image_read(reader, context, module_size, 0, dos, sizeof(dos)) ||
        dos[0] != 'M' || dos[1] != 'Z') return JC_PE_INVALID;
    uint32_t nt_rva = jc_u32(dos + 60);
    if (nt_rva > 1048576 || !jc_image_read(reader, context, module_size, nt_rva, nt, sizeof(nt)) ||
        memcmp(nt, "PE\0\0", 4) || jc_u16(nt + 4) != 0x8664 || jc_u16(nt + 20) < 240 ||
        jc_u16(nt + 24) != 0x20b || jc_u32(nt + 24 + 108) < 1) return JC_PE_INVALID;
    uint32_t size = jc_u32(nt + 24 + 56);
    if (size > module_size || size < sizeof(dos)) return JC_PE_INVALID;
    uint32_t export_rva = jc_u32(nt + 24 + 112), export_size = jc_u32(nt + 24 + 116);
    if (export_size < 40 || export_size > 4 * 1024 * 1024 || export_size > size || export_rva > size - export_size ||
        !jc_image_read(reader, context, size, export_rva, directory, sizeof(directory))) return JC_PE_INVALID;
    uint32_t ordinal_base = jc_u32(directory + 16), functions = jc_u32(directory + 20), names = jc_u32(directory + 24);
    uint32_t function_rva = jc_u32(directory + 28), name_rva = jc_u32(directory + 32), ordinal_rva = jc_u32(directory + 36);
    if (!functions || functions > 16384 || names > 16384 ||
        functions * 4 > size || function_rva > size - functions * 4 ||
        names * 4 > size || name_rva > size - names * 4 || names * 2 > size || ordinal_rva > size - names * 2)
        return JC_PE_INVALID;
    uint32_t index = UINT32_MAX;
    if (symbol[0] == '#') {
        uint32_t ordinal = 0;
        if (!symbol[1]) return JC_PE_INVALID;
        for (size_t i = 1; symbol[i]; ++i) {
            if (symbol[i] < '0' || symbol[i] > '9' || ordinal > 6553) return JC_PE_INVALID;
            ordinal = ordinal * 10 + (unsigned)(symbol[i] - '0');
            if (ordinal > 65535) return JC_PE_INVALID;
        }
        if (ordinal < ordinal_base || ordinal - ordinal_base >= functions) return JC_PE_NOT_FOUND;
        index = ordinal - ordinal_base;
    } else {
        uint32_t lo = 0, hi = names;
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo) / 2;
            char name[128];
            if (!jc_image_read(reader, context, size, name_rva + mid * 4, value, 4) ||
                !jc_image_string(reader, context, size, jc_u32(value), name)) return JC_PE_INVALID;
            int comparison = strcmp(symbol, name);
            if (comparison < 0) hi = mid;
            else if (comparison > 0) lo = mid + 1;
            else {
                if (!jc_image_read(reader, context, size, ordinal_rva + mid * 2, value, 2)) return JC_PE_INVALID;
                index = jc_u16(value);
                break;
            }
        }
    }
    if (index == UINT32_MAX) return JC_PE_NOT_FOUND;
    if (index >= functions || !jc_image_read(reader, context, size, function_rva + index * 4, value, 4)) return JC_PE_INVALID;
    uint32_t rva = jc_u32(value);
    if (!rva || rva >= size) return JC_PE_INVALID;
    if (rva >= export_rva && rva - export_rva < export_size) {
        if (!jc_image_string(reader, context, export_rva + export_size, rva, out->forward)) return JC_PE_INVALID;
        return JC_PE_FORWARD;
    }
    uint16_t sections = jc_u16(nt + 6);
    uint32_t section_rva = nt_rva + 24 + jc_u16(nt + 20);
    if (!sections || sections > 96) return JC_PE_INVALID;
    for (uint16_t i = 0; i < sections; ++i) {
        unsigned char section[40];
        if (!jc_image_read(reader, context, size, section_rva + 40 * i, section, sizeof(section))) return JC_PE_INVALID;
        uint32_t start = jc_u32(section + 12), length = jc_u32(section + 8);
        if (rva >= start && rva - start < length && (jc_u32(section + 36) & 0x20000000)) {
            out->rva = rva;
            return JC_PE_FUNCTION;
        }
    }
    return JC_PE_INVALID;
}
#endif
