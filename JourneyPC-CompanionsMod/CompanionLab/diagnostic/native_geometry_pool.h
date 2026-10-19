#ifndef JC_NATIVE_GEOMETRY_POOL_H
#define JC_NATIVE_GEOMETRY_POOL_H
#include "native_layout.h"
struct jc_geometry_pool { uint64_t source; uint32_t slots, occupied, free_slots; };
enum { JC_GEOMETRY_GLYPH_SLOTS=136 };
struct jc_geometry_slots { uint64_t table,heads[JC_GEOMETRY_GLYPH_SLOTS]; };
static inline int jc_geometry_pool_read_slots_bounded(jc_layout_reader read,void *ctx,uint64_t material,unsigned limit,
                                        struct jc_geometry_pool *out,struct jc_geometry_slots *slots) {
    uint64_t source,payload,table,owned,end,again,heads[JC_GEOMETRY_GLYPH_SLOTS];
    uint32_t count,count_after;
    unsigned char header[12],header_again[12],rows[JC_GEOMETRY_GLYPH_SLOTS*0x30],rows_again[JC_GEOMETRY_GLYPH_SLOTS*0x30];
    struct jc_geometry_pool result={0};
    memset(out,0,sizeof(*out));
    if (slots) memset(slots,0,sizeof(*slots));
    if (!jc_layout_field(read,ctx,material,0x10,&source,8) ||
        !jc_layout_field(read,ctx,source,0x10,&payload,8) ||
        !jc_layout_field(read,ctx,payload,0x180,header,sizeof(header)) ||
        !jc_layout_field(read,ctx,material,0x30,&owned,8)) return JC_LAYOUT_READ;
    memcpy(&table,header,8); memcpy(&count,header+8,4);
    if (!limit || limit>JC_GEOMETRY_GLYPH_SLOTS || !count || count>limit || (table&7) || !jc_layout_address(table,(uint64_t)count*0x30,&end)) return JC_LAYOUT_RANGE;
    if (owned<table || owned>=end || (owned-table)%0x30) return JC_LAYOUT_TYPE;
    uint64_t node;
    if (!jc_layout_address(material,0x30,&node)) return JC_LAYOUT_RANGE;
    size_t span=(count-1)*0x30+8;
    if (!jc_layout_field(read,ctx,table,0,rows,span)) return JC_LAYOUT_READ;
    for (uint32_t i=0;i<count;++i) {
        memcpy(&heads[i],rows+i*0x30,8);
        if (heads[i]) ++result.occupied; else ++result.free_slots;
        if (owned==table+(uint64_t)i*0x30 && heads[i]!=node) return JC_LAYOUT_TYPE;
    }
    if (!jc_layout_field(read,ctx,table,0,rows_again,span)) return JC_LAYOUT_READ;
    for (uint32_t i=0;i<count;++i) {
        memcpy(&again,rows_again+i*0x30,8);
        if (again!=heads[i]) return JC_LAYOUT_CHANGED;
    }
    if (!jc_layout_field(read,ctx,material,0x10,&again,8)) return JC_LAYOUT_READ;
    if (again!=source) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,material,0x30,&again,8)) return JC_LAYOUT_READ;
    if (again!=owned) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,source,0x10,&again,8)) return JC_LAYOUT_READ;
    if (again!=payload) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,payload,0x180,header_again,sizeof(header_again))) return JC_LAYOUT_READ;
    memcpy(&again,header_again,8); memcpy(&count_after,header_again+8,4);
    if (again!=table) return JC_LAYOUT_CHANGED;
    if (count_after!=count) return JC_LAYOUT_CHANGED;
    result.source=source; result.slots=count; *out=result;
    if (slots) { slots->table=table; memcpy(slots->heads,heads,count*sizeof(heads[0])); }
    return JC_LAYOUT_OK;
}
static inline int jc_geometry_pool_read_slots(jc_layout_reader read,void *ctx,uint64_t material,
        struct jc_geometry_pool *out,struct jc_geometry_slots *slots) {
    return jc_geometry_pool_read_slots_bounded(read,ctx,material,64,out,slots);
}
static inline int jc_geometry_pool_read(jc_layout_reader read,void *ctx,uint64_t material,
                                        struct jc_geometry_pool *out) {
    return jc_geometry_pool_read_slots(read,ctx,material,out,NULL);
}
struct jc_geometry_build_state { uint32_t limit, slots, occupied; };
/* Check a detached material through its captured source; never require a live peer as an anchor. */
static inline int jc_geometry_released(jc_layout_reader read,void *ctx,
        const struct jc_geometry_pool *before,const struct jc_geometry_slots *slots,
        uint64_t material,uint32_t *occupied) {
    uint64_t source,payload,table,again,links[3],row[2]; uint32_t count,removed=0;
    *occupied=0;
    if (!jc_layout_field(read,ctx,material,0x10,&source,8) ||
        !jc_layout_field(read,ctx,material,0x30,links,sizeof(links))) return JC_LAYOUT_READ;
    if (source || links[0] || links[1] || links[2]) return JC_LAYOUT_TYPE;
    if (!jc_layout_field(read,ctx,before->source,0x10,&payload,8) ||
        !jc_layout_field(read,ctx,payload,0x180,&table,8) ||
        !jc_layout_field(read,ctx,payload,0x188,&count,4)) return JC_LAYOUT_READ;
    if (table!=slots->table || count!=before->slots || !count || count>64) return JC_LAYOUT_CHANGED;
    for (unsigned pass=0;pass<2;++pass) {
        for (uint32_t i=0;i<count;++i) {
            uint64_t expected=slots->heads[i];
            if (expected==material+0x30) { expected=0; if (!pass) ++removed; }
            if (!jc_layout_field(read,ctx,table,(uint64_t)i*0x30,row,sizeof(row))) return JC_LAYOUT_READ;
            if (row[0]!=expected || (slots->heads[i]==material+0x30 && row[1])) return JC_LAYOUT_CHANGED;
            if (!pass && row[0]) ++*occupied;
        }
    }
    if (removed!=1 || *occupied+1!=before->occupied) return JC_LAYOUT_TYPE;
    if (!jc_layout_field(read,ctx,before->source,0x10,&again,8) || again!=payload ||
        !jc_layout_field(read,ctx,payload,0x180,&again,8) || again!=table ||
        !jc_layout_field(read,ctx,payload,0x188,&count,4) || count!=before->slots) return JC_LAYOUT_CHANGED;
    return JC_LAYOUT_OK;
}
static inline int jc_geometry_build_read(jc_layout_reader read,void *ctx,uint64_t image,
                                         uint64_t object,struct jc_geometry_build_state *out) {
    uint64_t type,table,end,heads[64],again;
    uint32_t limit,count,check;
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(read,ctx,object,0,&type,8) ||
        !jc_layout_field(read,ctx,object,0x110,&limit,4) ||
        !jc_layout_field(read,ctx,object,0x180,&table,8) ||
        !jc_layout_field(read,ctx,object,0x188,&count,4)) return JC_LAYOUT_READ;
    if (type!=image+0x680930) return JC_LAYOUT_TYPE;
    if (!limit || limit>64 || count>64 || (!table && count) ||
        (table && ((table&7) || !jc_layout_address(table,(uint64_t)count*0x30,&end)))) return JC_LAYOUT_RANGE;
    struct jc_geometry_build_state result={limit,count,0};
    for (uint32_t i=0;i<count;++i) {
        if (!jc_layout_field(read,ctx,table,(uint64_t)i*0x30,&heads[i],8)) return JC_LAYOUT_READ;
        if (heads[i]) ++result.occupied;
    }
    for (uint32_t i=0;i<count;++i) {
        if (!jc_layout_field(read,ctx,table,(uint64_t)i*0x30,&again,8)) return JC_LAYOUT_READ;
        if (again!=heads[i]) return JC_LAYOUT_CHANGED;
    }
    if (!jc_layout_field(read,ctx,object,0,&again,8)) return JC_LAYOUT_READ;
    if (again!=type) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,object,0x180,&again,8)) return JC_LAYOUT_READ;
    if (again!=table) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,object,0x188,&check,4)) return JC_LAYOUT_READ;
    if (check!=count) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,object,0x110,&check,4)) return JC_LAYOUT_READ;
    if (check!=limit) return JC_LAYOUT_CHANGED;
    *out=result; return JC_LAYOUT_OK;
}
#endif
