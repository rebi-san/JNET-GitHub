#ifndef JC_NATIVE_LIFECYCLE_TRACE_H
#define JC_NATIVE_LIFECYCLE_TRACE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define JC_LIFECYCLE_CAP 8192u
/* Stable wire layout for helper reads, not the room protocol. */
struct jc_lifecycle_event {
    uint32_t sequence,frame,tick,thread,event,caller,roles,xmm1;
    uint64_t args[4];
};
struct jc_lifecycle_ring {
    uint32_t enabled,head,dropped,lock;
    struct jc_lifecycle_event events[JC_LIFECYCLE_CAP];
};
struct jc_lifecycle_jobs { uint64_t job[32]; uint32_t thread[32]; };
/* Called under the observer lock. Preserve raw job arguments; never relabel them as meshes. */
static inline int jc_lifecycle_animation(struct jc_lifecycle_jobs *jobs,unsigned event,
        const uint64_t args[4],uint32_t thread,uint64_t barn) {
    if (event==17) {
        if (!barn || !args[0] || (args[1]!=barn+0x210 && args[1]!=barn+0x14850)) return 0;
        unsigned slot=32;
        for (unsigned i=0;i<32;++i) {
            if (jobs->job[i]==args[0] && jobs->thread[i]==thread) return -1;
            if (!jobs->job[i] && slot==32) slot=i;
        }
        if (slot==32) return -1;
        jobs->job[slot]=args[0]; jobs->thread[slot]=thread; return 1;
    }
    if (event==18) {
        for (unsigned i=0;i<32;++i) if (jobs->job[i] && jobs->job[i]==args[0] && jobs->thread[i]==thread) {
            jobs->job[i]=0; return 1;
        }
        return 0;
    }
    return 1;
}
_Static_assert(sizeof(struct jc_lifecycle_event)==64,"lifecycle event layout");
_Static_assert(offsetof(struct jc_lifecycle_ring,events)==16,"lifecycle ring layout");
static inline unsigned jc_lifecycle_role(uint64_t p,uint64_t game,uint64_t barn) {
    if (!p) return 0;
    if (game && p==game) return 4;
    if (!barn) return 0;
    if (p==barn) return 3;
    if (p==barn+0xb0) return 1;
    if (p==barn+0x146f0) return 2;
    if (p==barn+0x210) return 5;
    if (p==barn+0x14850) return 6;
    if (p==barn+0x40) return 7;
    if (p==barn+0x90) return 8;
    return 0;
}
static inline uint32_t jc_lifecycle_first(uint32_t head) {
    return head>=JC_LIFECYCLE_CAP ? head-JC_LIFECYCLE_CAP+1 : 1;
}
/* Two independently read copies must agree; writers clear sequence before reuse. */
static inline int jc_lifecycle_stable(const struct jc_lifecycle_event *a,
        const struct jc_lifecycle_event *b,uint32_t sequence) {
    return sequence && a->sequence==sequence && b->sequence==sequence && !memcmp(a,b,sizeof(*a));
}
#endif
