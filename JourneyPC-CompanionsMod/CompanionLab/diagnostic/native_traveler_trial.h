#ifndef JC_NATIVE_TRAVELER_TRIAL_H
#define JC_NATIVE_TRAVELER_TRIAL_H
#include "native_resources.h"

/* A chapter-owned glyph is not an allocation owned by Dude's destructor. */
struct jc_traveler_identity {
    struct jc_resource_slot mesh;
    uint64_t glyph, sounds[14];
};
static inline int jc_traveler_identity_read(jc_layout_reader read,void *ctx,
        uint64_t image,uint64_t dude,struct jc_traveler_identity *out) {
    uint64_t type;
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(read,ctx,dude,0,&type,8) ||
        !jc_resource_slot_read(read,ctx,dude,&out->mesh) ||
        !jc_layout_field(read,ctx,dude,0x2240,&out->glyph,8) ||
        !jc_layout_field(read,ctx,dude,0x1780,out->sounds,sizeof(out->sounds))) return 1;
    if (type!=image+0x5d0380 || out->mesh.type!=image+0x67f3f8 ||
        out->mesh.controller_mesh!=dude+0x160) return 2;
    uint64_t end;
    if (!out->mesh.count || out->mesh.count>4096 || (out->mesh.entries&7) ||
        !jc_layout_address(out->mesh.entries,out->mesh.count*0x50,&end)) return 4;
    return 0;
}
static inline int jc_traveler_storage_same(const struct jc_traveler_identity *a,const struct jc_traveler_identity *b) {
    return a && b && a->glyph==b->glyph && !memcmp(&a->mesh,&b->mesh,sizeof(a->mesh));
}
static inline int jc_traveler_glyph_slot(uint64_t barn,uint32_t count,uint64_t glyph) {
    uint64_t begin,end;
    return count<=1024 && jc_layout_address(barn,0x10,&begin) &&
        jc_layout_address(begin,(uint64_t)count*0x10d0,&end) &&
        glyph>=begin && glyph<end && (glyph-begin)%0x10d0==0;
}
static inline int jc_traveler_added(const struct jc_traveler_identity originals[2],
        const struct jc_traveler_identity *added,uint64_t decorations,uint32_t before,
        uint32_t after,unsigned *live_sounds) {
    *live_sounds=0;
    if (before>=1024 || after!=before+1 ||
        !jc_traveler_glyph_slot(decorations,after,added->glyph) ||
        added->glyph!=decorations+0x10+(uint64_t)before*0x10d0) return 5;
    for (unsigned p=0;p<2;++p) {
        if (added->glyph==originals[p].glyph ||
            !jc_traveler_glyph_slot(decorations,before,originals[p].glyph)) return 5;
        uint64_t a_end,b_end;
        if (!jc_layout_address(added->mesh.entries,added->mesh.count*0x50,&a_end) ||
            !jc_layout_address(originals[p].mesh.entries,originals[p].mesh.count*0x50,&b_end) ||
            (added->mesh.entries<b_end && originals[p].mesh.entries<a_end)) return 6;
    }
    for (unsigned i=0;i<14;++i) {
        uint64_t handle=added->sounds[i];
        if ((uint32_t)handle==UINT32_MAX) continue;
        ++*live_sounds;
        for (unsigned p=0;p<2;++p) for (unsigned j=0;j<14;++j)
            if ((uint32_t)originals[p].sounds[j]!=UINT32_MAX && handle==originals[p].sounds[j]) return 7;
        for (unsigned j=0;j<i;++j) if (handle==added->sounds[j]) return 7;
    }
    return 0;
}
#endif
