#ifndef JC_NATIVE_ACTOR_RUNTIME_H
#define JC_NATIVE_ACTOR_RUNTIME_H
#include "native_actor_live.h"

enum { JC_ACTOR_RUNTIME_EMPTY,JC_ACTOR_RUNTIME_WAITING,JC_ACTOR_RUNTIME_ACTIVE,
    JC_ACTOR_RUNTIME_DRAINING,JC_ACTOR_RUNTIME_RETIRED,JC_ACTOR_RUNTIME_QUARANTINED };
enum { JC_ACTOR_PHASE_INPUT=1,JC_ACTOR_PHASE_UPDATE,JC_ACTOR_PHASE_RENDER,JC_ACTOR_PHASE_FINISH };
enum { JC_ACTOR_RUNTIME_ARGUMENT=40,JC_ACTOR_RUNTIME_THREAD,JC_ACTOR_RUNTIME_ORDER,
    JC_ACTOR_RUNTIME_BUSY,JC_ACTOR_RUNTIME_SCOPE };
struct jc_actor_runtime_ops {
    jc_layout_reader read;
    jc_actor_writer write;
    jc_actor_retired retired;
    uint64_t (*thread)(void *);
    int (*ready)(void *,const struct jc_actor_identity *);
    int (*prepare)(void *,const struct jc_actor_identity *,uint64_t);
    int (*begin)(void *,const struct jc_actor_identity *,unsigned,uint64_t,uint64_t);
    int (*end)(void *,const struct jc_actor_identity *,unsigned,uint64_t,int);
    void (*drain)(void *,const struct jc_actor_identity *);
    void *context;
};
/* Stable caller-owned storage. All entry points run on the recorded game owner
   thread; transport threads enqueue packets instead of touching this object.
   begin/end own appearance routing in INPUT/UPDATE and render attribution in
   RENDER. prepare reserves participant buffers before any actor mutation. end
   closes that scope even on failure; it must preserve submitted resource leases.
   retired includes actor meshes, animation jobs, cloth jobs and GPU batches. */
struct jc_actor_runtime {
    struct jc_actor_identity identity;
    struct jc_actor_runtime_ops ops;
    struct jc_receiver receiver;
    struct jc_actor_input input;
    struct jc_actor_publication publication;
    struct jc_actor_submission submission;
    uint64_t owner,input_frame,update_frame,render_frame,finish_frame;
    unsigned state,busy,last_result,departing;
};
static inline int jc_actor_runtime_thread(const struct jc_actor_runtime *r) {
    return r && r->ops.thread && r->owner && r->ops.thread(r->ops.context)==r->owner;
}
static inline int jc_actor_runtime_quarantine(struct jc_actor_runtime *r,int result) {
    r->state=JC_ACTOR_RUNTIME_QUARANTINED;r->last_result=(unsigned)result;
    r->ops.drain(r->ops.context,&r->identity);return result;
}
static inline int jc_actor_runtime_bind(struct jc_actor_runtime *r,
        const struct jc_actor_identity *id,const struct jc_actor_runtime_ops *ops) {
    if (!r || !id || !ops || !ops->read || !ops->write || !ops->retired || !ops->thread ||
        !ops->ready || !ops->prepare || !ops->begin || !ops->end || !ops->drain ||
        r->state!=JC_ACTOR_RUNTIME_EMPTY) return JC_ACTOR_RUNTIME_ARGUMENT;
    uint64_t owner=ops->thread(ops->context);
    if (!owner || !jc_actor_identity_check(ops->read,ops->context,id)) return JC_ACTOR_IDENTITY;
    if (!ops->ready(ops->context,id) || ops->retired(ops->context,id)!=1) return JC_ACTOR_RUNTIME_BUSY;
    r->identity=*id;r->ops=*ops;r->owner=owner;
    int code=jc_receiver_replace(&r->receiver,id->peer,id->generation);
    if (code) return code;
    r->state=JC_ACTOR_RUNTIME_WAITING;return 0;
}
static inline int jc_actor_runtime_receive(struct jc_actor_runtime *r,
        const struct jc_actor_sender *sender,uint64_t peer,uint64_t generation,uint64_t sequence) {
    if (!jc_actor_runtime_thread(r)) return JC_ACTOR_RUNTIME_THREAD;
    if (r->busy || r->departing || (r->state!=JC_ACTOR_RUNTIME_WAITING && r->state!=JC_ACTOR_RUNTIME_ACTIVE) ||
        r->input_frame!=r->finish_frame) return JC_ACTOR_RUNTIME_ORDER;
    uint32_t chapter;
    if (!jc_actor_identity_check(r->ops.read,r->ops.context,&r->identity) ||
        !jc_layout_field(r->ops.read,r->ops.context,r->identity.barn,0x14,&chapter,4))
        return jc_actor_runtime_quarantine(r,JC_ACTOR_IDENTITY);
    int code=jc_actor_receive(&r->receiver,&r->input,sender,peer,generation,sequence,chapter);
    if (!code) r->state=JC_ACTOR_RUNTIME_ACTIVE;
    r->last_result=(unsigned)code;return code;
}
static inline int jc_actor_runtime_enter(struct jc_actor_runtime *r,uint64_t frame) {
    if (!jc_actor_runtime_thread(r)) return JC_ACTOR_RUNTIME_THREAD;
    if (!frame || r->busy || r->state!=JC_ACTOR_RUNTIME_ACTIVE) return JC_ACTOR_RUNTIME_ORDER;
    if (!jc_actor_publication_owner(r->ops.read,r->ops.context,&r->identity,r->input.chapter))
        return jc_actor_runtime_quarantine(r,JC_ACTOR_IDENTITY);
    if (!r->ops.ready(r->ops.context,&r->identity)) return JC_ACTOR_RUNTIME_BUSY;
    r->busy=1;return 0;
}
static inline int jc_actor_runtime_finish(struct jc_actor_runtime *r,unsigned phase,
        uint64_t frame,int result,int native_called) {
    int untouched=phase==JC_ACTOR_PHASE_INPUT && result==JC_ACTOR_RECEIVER && !native_called &&
        !r->publication.attempted_spans && !r->publication.quarantined;
    int closed=r->ops.end(r->ops.context,&r->identity,phase,frame,result==0 || untouched);
    r->busy=0;
    if (!closed || (result && native_called) || r->publication.quarantined ||
        !jc_actor_publication_owner(r->ops.read,r->ops.context,&r->identity,r->input.chapter))
        return jc_actor_runtime_quarantine(r,result?result:JC_ACTOR_RUNTIME_SCOPE);
    r->last_result=(unsigned)result;return result;
}
/* Hook after the original 118030 call, before 118690. frame is the caller's
   monotonically increasing GameUpdate serial, not a renderer invocation count. */
static inline int jc_actor_runtime_after_input(struct jc_actor_runtime *r,uint64_t frame) {
    int code=jc_actor_runtime_enter(r,frame);if(code)return code;
    if (frame<=r->input_frame || r->input_frame!=r->finish_frame) {
        r->busy=0;return JC_ACTOR_RUNTIME_ORDER;
    }
    uint64_t flags;unsigned char gates[2];
    if (!jc_layout_field(r->ops.read,r->ops.context,r->identity.game,0x178,&flags,8) ||
        !jc_layout_field(r->ops.read,r->ops.context,flags,0x4d,gates,2)) {
        r->busy=0;return JC_ACTOR_READ;
    }
    if (r->ops.retired(r->ops.context,&r->identity)!=1 ||
        !r->ops.prepare(r->ops.context,&r->identity,frame)) {
        r->busy=0;return JC_ACTOR_RUNTIME_BUSY;
    }
    if (!r->ops.begin(r->ops.context,&r->identity,JC_ACTOR_PHASE_INPUT,frame,0)) {
        r->busy=0;return JC_ACTOR_RUNTIME_SCOPE;
    }
    r->input.interaction_gate=(unsigned char)(!r->departing && !gates[0]&&!gates[1]);
    if(!r->departing)code=jc_actor_publish_numeric(r->ops.read,r->ops.write,r->ops.retired,r->ops.context,
        &r->identity,frame,&r->receiver,&r->input,&r->publication);
    int native_called=0;
    if (!code) {
        native_called=1;
        code=jc_actor_native_input(r->identity.image,r->identity.actor,r->identity.game,
            r->identity.barn+0xb0,&r->input);
    }
    code=jc_actor_runtime_finish(r,JC_ACTOR_PHASE_INPUT,frame,code,native_called);
    if (!code) r->input_frame=frame;
    return code;
}
/* GameUpdate calls 118df0 immediately after 117790. Its per-actor a9d20
   rotates interaction parameters and samples body bones for the next frame.
   This is independent of cloth/GPU retirement and must run exactly once. */
static inline int jc_actor_runtime_after_finish(struct jc_actor_runtime *r,uint64_t frame) {
    int code=jc_actor_runtime_enter(r,frame);if(code)return code;
    if (frame!=r->update_frame || frame!=r->render_frame || frame<=r->finish_frame) {
        r->busy=0;return JC_ACTOR_RUNTIME_ORDER;
    }
    if (!r->ops.begin(r->ops.context,&r->identity,JC_ACTOR_PHASE_FINISH,frame,0)) {
        r->busy=0;return JC_ACTOR_RUNTIME_SCOPE;
    }
    jc_actor_native_finish(r->identity.image,r->identity.actor);
    code=jc_actor_runtime_finish(r,JC_ACTOR_PHASE_FINISH,frame,0,1);
    if(!code)r->finish_frame=frame;
    return code;
}
/* Hook after original 118690. Internal a9470 and environment setters must be
   redirected by begin/end to this participant, throughout the original call. */
static inline int jc_actor_runtime_after_update(struct jc_actor_runtime *r,uint64_t frame) {
    int code=jc_actor_runtime_enter(r,frame);if(code)return code;
    if (frame!=r->input_frame || frame<=r->update_frame) {
        r->busy=0;return JC_ACTOR_RUNTIME_ORDER;
    }
    if (!r->ops.begin(r->ops.context,&r->identity,JC_ACTOR_PHASE_UPDATE,frame,0)) {
        r->busy=0;return JC_ACTOR_RUNTIME_SCOPE;
    }
    jc_actor_native_update(r->identity.image,r->identity.actor,r->identity.game,
        r->input.direction,(unsigned char)!r->input.inactive);
    code=jc_actor_runtime_finish(r,JC_ACTOR_PHASE_UPDATE,frame,0,1);
    if (!code)r->update_frame=frame;
    return code;
}
/* Hook after original 117790 on its verified owner thread. render is the real
   context; begin grants a participant command/matrix lease. Never call 117790
   with a Dude pointer or write the two stock matrix slots. */
static inline int jc_actor_runtime_after_render(struct jc_actor_runtime *r,uint64_t frame,uint64_t render) {
    int code=jc_actor_runtime_enter(r,frame);if(code)return code;
    if (!render || !r->update_frame || frame!=r->update_frame || frame<=r->render_frame ||
        r->input_frame!=r->update_frame) { r->busy=0;return JC_ACTOR_RUNTIME_ORDER; }
    if (!r->ops.begin(r->ops.context,&r->identity,JC_ACTOR_PHASE_RENDER,frame,render)) {
        r->busy=0;return JC_ACTOR_RUNTIME_SCOPE;
    }
    code=jc_actor_submit(r->ops.read,r->ops.context,r->identity.image,r->identity.actor,render,&r->submission);
    code=jc_actor_runtime_finish(r,JC_ACTOR_PHASE_RENDER,frame,code,1);
    if (!code)r->render_frame=frame;
    return code;
}
/* Invoke before native chapter resource teardown or process-exit destruction.
   drain prevents new leases; poll returns BUSY until every native user retires.
   It never frees a Dude and never converts quarantine into permission to free. */
static inline int jc_actor_runtime_stop(struct jc_actor_runtime *r) {
    if (!jc_actor_runtime_thread(r)) return JC_ACTOR_RUNTIME_THREAD;
    if (r->busy) return JC_ACTOR_RUNTIME_BUSY;
    if (r->state==JC_ACTOR_RUNTIME_QUARANTINED) return JC_ACTOR_QUARANTINED;
    if (r->state==JC_ACTOR_RUNTIME_RETIRED) return 0;
    if (r->state!=JC_ACTOR_RUNTIME_DRAINING) {
        r->state=JC_ACTOR_RUNTIME_DRAINING;
        jc_receiver_depart(&r->receiver,r->identity.peer,r->identity.generation);
        r->ops.drain(r->ops.context,&r->identity);
    }
    int retired=r->ops.retired(r->ops.context,&r->identity);
    if (retired<0) return jc_actor_runtime_quarantine(r,JC_ACTOR_RUNTIME_SCOPE);
    if (retired!=1) return JC_ACTOR_RUNTIME_BUSY;
    r->state=JC_ACTOR_RUNTIME_RETIRED;return 0;
}
#endif
