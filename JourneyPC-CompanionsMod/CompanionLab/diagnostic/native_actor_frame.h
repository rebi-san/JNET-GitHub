#ifndef JC_NATIVE_ACTOR_FRAME_H
#define JC_NATIVE_ACTOR_FRAME_H
#include "native_layout.h"
#include "native_cloth_commands.h"
#include "native_render_retirement.h"
#if defined(_WIN32) || defined(JC_RETIREMENT_TEST_PLATFORM)
struct jc_actor_frame {
    struct jc_live_retirement retirement;
    uint64_t ticket,generation;
    uintptr_t executor;
    DWORD thread;
    int failed;
    _Alignas(16) unsigned char commands[128];
};
typedef uint32_t (JC_COMMAND_ABI *jc_actor_frame_flush)(uintptr_t);
typedef uint32_t (JC_COMMAND_ABI *jc_actor_frame_handoff)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
struct jc_actor_frame_ops {
    uintptr_t (JC_COMMAND_ABI *executor)(void);
    uint32_t (JC_COMMAND_ABI *reserve)(uintptr_t,uintptr_t *);
    jc_actor_frame_flush flush;
    jc_actor_frame_handoff handoff;
};
static void JC_COMMAND_ABI jc_actor_frame_marker(const uintptr_t *payload) { (void)payload; }
/* Only the owning GameUpdate thread acknowledges completed GPU queries. */
static inline int jc_actor_frame_poll(struct jc_actor_frame *s) {
    if (!s || s->failed || (s->thread && s->thread!=GetCurrentThreadId())) return -1;
    if (!s->ticket) return 1;
    int result=jc_live_retirement_result(&s->retirement,s->ticket,1);
    if (result<0) { s->failed=1; return -1; }
    if (result) { s->ticket=0; s->executor=0; }
    return result;
}
/* dad40 returns after both scene2793b0 calls and its UI consumer. Native
   353c48 reserves every asynchronous task before353d60 launches it;352de0
   forwards that reservation without obtaining another. Flush the remaining
   main arena before reserving this marker, so FIFO execution covers every
   preceding body/glyph/cloth command even when task handoffs arrive late. */
static inline int jc_actor_frame_after_ops(struct jc_actor_frame *s,jc_layout_reader read,
        void *ctx,uintptr_t image,uintptr_t game,uint64_t generation,
        const struct jc_actor_frame_ops *ops) {
    uint64_t render,global,scene,body,glyph,vtable,flush,begin,end,again;
    uint32_t body_count,glyph_count,executor_state;unsigned char synchronous;
    if (!s || !read || !image || !game || !generation || !ops || !ops->executor ||
        !ops->reserve || !ops->flush || !ops->handoff || jc_actor_frame_poll(s)!=1 ||
        (s->generation && generation<=s->generation)) return -1;
    s->thread=GetCurrentThreadId();
    if (!jc_layout_field(read,ctx,game,0x10,&render,8) || !render ||
        !jc_layout_field(read,ctx,image,0x3c942d0,&global,8) || !global ||
        global>UINTPTR_MAX-0xd0 || render!=global+0xd0 ||
        !jc_layout_field(read,ctx,game,0x98,&scene,8) || !scene ||
        !jc_layout_field(read,ctx,scene,0x18,&body,8) || !body ||
        !jc_layout_field(read,ctx,scene,0x60,&glyph,8) || !glyph || body==glyph ||
        !jc_layout_field(read,ctx,body,0x10080,&body_count,4) || body_count ||
        !jc_layout_field(read,ctx,glyph,0x10080,&glyph_count,4) || glyph_count ||
        !read(ctx,render,&vtable,8) || !vtable ||
        !jc_layout_field(read,ctx,vtable,0x10,&flush,8) || flush!=image+0x352540 ||
        !jc_layout_field(read,ctx,render,0x90,&begin,8) ||
        !jc_layout_field(read,ctx,render,0x98,&end,8) ||
        (!begin!=!end) || end<begin || (end-begin)%128 || end-begin>64u*1024u*1024u ||
        !jc_layout_field(read,ctx,image,0x3c03cd0,&synchronous,1) || synchronous>1)
        goto failed;
    uintptr_t executor=ops->executor();
    if (!executor || !jc_layout_field(read,ctx,executor,0x20,&executor_state,4) ||
        (!synchronous && executor_state!=1)) goto failed;
    if (begin && ops->flush((uintptr_t)render)) goto failed;
    if (!jc_layout_field(read,ctx,render,0x90,&begin,8) || begin ||
        !jc_layout_field(read,ctx,render,0x98,&end,8) || end ||
        !jc_layout_field(read,ctx,game,0x10,&again,8) || again!=render ||
        ops->executor()!=executor ||
        !jc_layout_field(read,ctx,body,0x10080,&body_count,4) || body_count ||
        !jc_layout_field(read,ctx,glyph,0x10080,&glyph_count,4) || glyph_count)
        goto failed;
    memset(s->commands,0,sizeof(s->commands));uint32_t kind=0x2c;
    uintptr_t callback=(uintptr_t)jc_actor_frame_marker;
    memcpy(s->commands,&kind,4);memcpy(s->commands+8,&callback,8);
    s->ticket=jc_live_retirement_arm(&s->retirement,executor,
        (uintptr_t)s->commands,(uintptr_t)s->commands+sizeof(s->commands));
    if (!s->ticket) goto failed;
    s->executor=executor;s->generation=generation;
    uintptr_t reservation=0;
    if (ops->reserve(executor,&reservation) || !reservation ||
        ops->handoff(executor,(uintptr_t)&reservation,0,
            (uintptr_t)s->commands,(uintptr_t)s->commands+sizeof(s->commands))) goto failed;
    return 0;
failed:
    s->failed=1;return -1;
}
static inline int jc_actor_frame_after(struct jc_actor_frame *s,jc_layout_reader read,
        void *ctx,uintptr_t image,uintptr_t game,uint64_t generation,
        jc_actor_frame_flush flush,jc_actor_frame_handoff handoff) {
    struct jc_actor_frame_ops ops={
        (uintptr_t (JC_COMMAND_ABI *)(void))(image+0x378930),
        (uint32_t (JC_COMMAND_ABI *)(uintptr_t,uintptr_t *))(image+0x378790),flush,handoff};
    return jc_actor_frame_after_ops(s,read,ctx,image,game,generation,&ops);
}
#endif
#endif
