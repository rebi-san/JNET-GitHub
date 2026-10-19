#ifndef JC_COMPANION_ANIMATION_H
#define JC_COMPANION_ANIMATION_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#define JC_ANIM_ACTIVE 16u
#define JC_ANIM_ENTRIES 512u
struct jc_anim_clip { uint32_t index,additive; float frame,duration,speed,weight; };
struct jc_anim_sample { uint64_t schema; uint32_t total,count; struct jc_anim_clip clips[JC_ANIM_ACTIVE]; };
static inline unsigned jc_anim_clip_field(const struct jc_anim_clip *c,unsigned total) {
    if (c->index>=total) return 2;
    if (c->additive>1) return 3;
    if (!isfinite(c->weight) || fabsf(c->weight)>8) return 4;
    if (!isfinite(c->frame) || fabsf(c->frame)>1000000) return 7;
    if (!isfinite(c->duration) || (c->duration<0 && c->duration!=-1) || c->duration>1000000) return 8;
    if (!isfinite(c->speed) || fabsf(c->speed)>32) return 9;
    return 0;
}
static inline int jc_anim_valid(const struct jc_anim_sample *s) {
    if (!s->total) return !s->schema && !s->count;
    if (!s->schema || s->total>JC_ANIM_ENTRIES || s->count>JC_ANIM_ACTIVE || s->count>s->total) return 0;
    for (unsigned i=0;i<s->count;++i) {
        const struct jc_anim_clip *c=s->clips+i;
        if (jc_anim_clip_field(c,s->total)) return 0;
        for (unsigned j=0;j<i;++j) if (s->clips[j].index==c->index) return 0;
    }
    return 1;
}
#endif
