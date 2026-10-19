#ifndef JC_NATIVE_CHAPTER_GATE_H
#define JC_NATIVE_CHAPTER_GATE_H
#include <stdint.h>
#include <string.h>
struct jc_chapter_gate {
    uintptr_t game;
    uint32_t thread;
    unsigned pending,failed;
    char target[64];
};
static inline int jc_chapter_defer(struct jc_chapter_gate *g,uintptr_t game,uint32_t thread,
        const char target[64]) {
    if (!g || !game || !thread || !target || !target[0] || !memchr(target,0,64) ||
        g->failed || (g->pending && (g->game!=game || g->thread!=thread))) {
        if (g) g->failed=1;
        return 0;
    }
    g->game=game; g->thread=thread; memcpy(g->target,target,64); g->pending=1; return 1;
}
/* Consume once only after drain succeeds. Native GameUpdate clears its own
   pending name after da5d0 returns, so a deferred request needs private storage. */
static inline int jc_chapter_take(struct jc_chapter_gate *g,uintptr_t game,uint32_t thread,
        int drained,char target[64]) {
    if (!g || !g->pending || g->failed) return 0;
    if (g->game!=game || g->thread!=thread || drained<0) { g->failed=1; return 0; }
    if (!drained) return 0;
    memcpy(target,g->target,64); g->pending=0; memset(g->target,0,64); return 1;
}
#endif
