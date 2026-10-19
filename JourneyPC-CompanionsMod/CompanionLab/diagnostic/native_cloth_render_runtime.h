#ifndef JC_NATIVE_CLOTH_RENDER_RUNTIME_H
#define JC_NATIVE_CLOTH_RENDER_RUNTIME_H
#include "native_cloth_publication.h"

#if defined(_WIN32) || defined(JC_CLOTH_RENDER_TEST_PLATFORM)
struct jc_cloth_render_draw {
    uintptr_t item,bindings,shader,begin,end;
    unsigned role,participant,first,count,building,built,forwarded,visited;
    struct jc_cloth_publication_view view;
    unsigned char snapshot[8*128];
};
struct jc_cloth_render_owner {
    struct jc_cloth_live *backend;
    uint64_t generation,native_frame;
    uintptr_t image,executor;
    unsigned index,count,building,handoffs,submitted,failed;
    uint64_t tickets[JC_RETIRE_SLOTS];
    unsigned ticket_count,ticket_done;
    int executor_mode;
    struct jc_cloth_render_draw draws[JC_CLOTH_STAGE_DRAWS];
};
struct jc_cloth_render_scope {
    struct jc_cloth_render_owner *owner;
    struct jc_cloth_render_draw *draw;
    struct jc_cloth_render_scope *previous;
    uintptr_t renderer,cursor;
};
static struct jc_cloth_render_owner *jc_cloth_render_owners[JC_CLOTH_FRAMES];
static SRWLOCK jc_cloth_render_lock=SRWLOCK_INIT;
static uint64_t jc_cloth_render_counts[2][7][4];
static DWORD jc_cloth_render_tls=TLS_OUT_OF_INDEXES;

static void jc_cloth_render_fail(struct jc_cloth_render_owner *o) {
    o->failed=1; o->backend->failed=1;
    o->backend->frames.frames[o->index].phase=JC_FRAME_UNKNOWN;
}
static int jc_cloth_render_add(struct jc_cloth_render_owner *o,uintptr_t model,
        unsigned role,unsigned participant,unsigned first,unsigned count) {
    if (o->count==JC_CLOTH_STAGE_DRAWS || role>=7 || !count || count%3) return 1;
    struct jc_cloth_render_draw *d=o->draws+o->count;
    struct jc_cloth_live *s=o->backend;
    uint64_t geometry,table; uint32_t batches;
    if (jc_cloth_publication_model(s->read,s->read_context,model,&d->item,&d->bindings,&d->shader)) return 2;
    if (!s->read(s->read_context,d->item,&geometry,8) || !geometry) return 3;
    if (!s->read(s->read_context,geometry,&batches,4) || (batches&0x7fffffff)!=1) return 4;
    if (!jc_layout_field(s->read,s->read_context,geometry,8,&table,8) || !table) return 5;
    for (unsigned i=0;i<o->count;++i)
        if (o->draws[i].item==d->item || o->draws[i].bindings==d->bindings) return 6;
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) if (jc_cloth_render_owners[f])
        for (unsigned i=0;i<jc_cloth_render_owners[f]->count;++i)
            if (jc_cloth_render_owners[f]->draws[i].item==d->item ||
                jc_cloth_render_owners[f]->draws[i].bindings==d->bindings) return 7;
    d->role=role; d->participant=participant; d->first=first; d->count=count;
    ++o->count; return 0;
}
/* Called after native finalization/private unmap, before render consumption.
   Root owns the GameUpdate/render lifetime boundary. No reservation is copied. */
static int jc_cloth_render_register(struct jc_cloth_live *s,unsigned index,
        uintptr_t image,uintptr_t barn,uintptr_t render_context,uint64_t native_frame,unsigned *detail) {
    if (!detail) return 1;
    *detail=1;
    if (!jc_cloth_live_owner(s) || index>=JC_CLOTH_FRAMES || !image || !barn || !render_context || !native_frame ||
        s->frames.closing || s->commands[index].owner || s->automatic_pending[index]) return 1;
    *detail=2;
    struct jc_cloth_frame *f=s->frames.frames+index;
    if (f->phase!=JC_FRAME_READY || !f->stock_committed || !f->draw_count) return 1;
    *detail=3;
    for (unsigned i=0;i<f->ref_count;++i)
        if (f->refs[i].kind==JC_FRAME_VERTEX || f->refs[i].kind==JC_FRAME_INDEX) {
            struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,f->refs[i].object);
            if (!b || b->mapped || b->pins!=1) return 1;
        }
    *detail=4;
    struct jc_cloth_render_owner *o=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*o));
    if (!o) return 1;
    o->backend=s; o->index=index; o->generation=f->generation; o->native_frame=native_frame; o->image=image; o->executor_mode=-1;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    *detail=5;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) if (jc_cloth_render_owners[i]) goto before_mutation;
    *detail=6;
    if (jc_cloth_render_tls==TLS_OUT_OF_INDEXES &&
        (jc_cloth_render_tls=TlsAlloc())==TLS_OUT_OF_INDEXES) goto before_mutation;
    for (unsigned r=0;r<7;++r) if (f->streams[r].indices) {
        uint64_t model;
        const struct jc_cloth_partition_range *range=&f->route.plan.ranges[0][r];
        *detail=0x100+r*16;
        if (!jc_layout_field(s->read,s->read_context,barn,0x666ed8+r*0x630,&model,8)) goto before_mutation;
        unsigned code=(unsigned)jc_cloth_render_add(o,(uintptr_t)model,r,0,range->index_begin,range->indices ? range->indices : 3);
        if (code) { *detail+=code;goto before_mutation; }
    }
    for (unsigned n=0;n<f->draw_count;++n) {
        const struct jc_cloth_material *d=&f->draws[n].material;
        struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,d->material);
        const struct jc_cloth_partition_range *range=&f->route.plan.ranges[d->participant][d->role];
        *detail=0x200+n*16;
        if (!m || m->pins!=1) goto before_mutation;
        if (!jc_cloth_live_bindings_match(s,d)) { *detail+=8;goto before_mutation; }
        unsigned code=(unsigned)jc_cloth_render_add(o,(uintptr_t)m->model,d->role,d->participant,range->index_begin,range->indices);
        if (code) { *detail+=code;goto before_mutation; }
    }
    *detail=7;
    if (o->count>JC_RETIRE_SLOTS) goto before_mutation;
    for (unsigned n=0;n<o->count;++n) if (o->draws[n].participant<2)
        ++jc_cloth_render_counts[o->draws[n].participant][o->draws[n].role][0];
    jc_cloth_render_owners[index]=o; s->commands[index].owner=o;
    s->automatic_pending[index]=1;
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned n=0;n<f->draw_count;++n) {
        struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,f->draws[n].material.material);
        /* 8a938 commits the role material before the original queue append. */
        ((void (JC_COMMAND_ABI *)(uintptr_t))(image+0x274d70))((uintptr_t)m->subscriber+0x88);
        *detail=0x300+n*16;
        uintptr_t item,bindings,shader;
        if (jc_cloth_publication_model(s->read,s->read_context,(uintptr_t)m->model,&item,&bindings,&shader) ||
            jc_cloth_publication_append(s->read,s->read_context,image,render_context,
                (uintptr_t)m->model,(uintptr_t)m->control)) {
            jc_cloth_render_fail(o); return 2;
        }
        unsigned found=0;
        for (unsigned j=0;j<o->count;++j) if (o->draws[j].item==item) {
            found=1;
            if (o->draws[j].bindings!=bindings || o->draws[j].shader!=shader) {
                *detail+=1; jc_cloth_render_fail(o); return 2;
            }
        }
        if (!found) { *detail+=2;jc_cloth_render_fail(o); return 2; }
    }
    *detail=0;return 0;
before_mutation:
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);
    HeapFree(GetProcessHeap(),0,o); return 1;
}
static int jc_cloth_render_dispatch_begin(uintptr_t renderer,uintptr_t item,uintptr_t bindings,
        struct jc_cloth_render_scope *scope) {
    memset(scope,0,sizeof(*scope));
    if (jc_cloth_render_tls==TLS_OUT_OF_INDEXES) return 0;
    scope->previous=TlsGetValue(jc_cloth_render_tls);
    if (!TlsSetValue(jc_cloth_render_tls,NULL)) return -1;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if (!o) continue;
        for (unsigned n=0;n<o->count;++n) {
            struct jc_cloth_render_draw *d=o->draws+n;
            if (d->item!=item) continue;
            if (o->backend->failed || o->backend->closed || o->failed || o->submitted ||
                o->backend->frames.frames[f].phase!=JC_FRAME_READY || d->visited || d->built ||
                bindings!=d->bindings || d->building) {
                jc_cloth_render_fail(o); ReleaseSRWLockExclusive(&jc_cloth_render_lock); return -1;
            }
            scope->owner=o; scope->draw=d; scope->renderer=renderer;
            if (!TlsSetValue(jc_cloth_render_tls,scope)) {
                jc_cloth_render_fail(o); scope->owner=NULL;
                ReleaseSRWLockExclusive(&jc_cloth_render_lock); return -1;
            }
            ++o->building; d->building=1;
            ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 1;
        }
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 0;
}
static void jc_cloth_render_dispatch_end(struct jc_cloth_render_scope *scope,uint32_t result) {
    if (jc_cloth_render_tls==TLS_OUT_OF_INDEXES) return;
    if (!scope->owner) { TlsSetValue(jc_cloth_render_tls,scope->previous); return; }
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    if (result || TlsGetValue(jc_cloth_render_tls)!=scope)
        jc_cloth_render_fail(scope->owner);
    --scope->owner->building; scope->draw->building=0; scope->draw->visited=1;
    if (scope->draw->participant<2) ++jc_cloth_render_counts[scope->draw->participant][scope->draw->role][1];
    if (!TlsSetValue(jc_cloth_render_tls,scope->previous)) jc_cloth_render_fail(scope->owner);
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);
}
/* Return 0 unrelated, 1 redirected, -1 owned failure. */
static int jc_cloth_render_build_before(uintptr_t renderer,const struct jc_cloth_command_source *source,
        const struct jc_cloth_command_source **redirected) {
    if (jc_cloth_render_tls==TLS_OUT_OF_INDEXES) return 0;
    struct jc_cloth_render_scope *scope=TlsGetValue(jc_cloth_render_tls);
    if (!scope) return 0;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    struct jc_cloth_render_owner *o=scope->owner;
    struct jc_cloth_live *s=o->backend; struct jc_cloth_render_draw *d=scope->draw;
    uint64_t limit; uint32_t passes;
    if (o->failed || d->built || scope->renderer!=renderer || source->material!=d->shader || source->index ||
        !jc_layout_field(s->read,s->read_context,renderer,0x68,&scope->cursor,8) ||
        !jc_layout_field(s->read,s->read_context,renderer,0x60,&limit,8) ||
        !jc_layout_field(s->read,s->read_context,source->passes,8,&passes,4) ||
        !(passes&0x7fffffff) || (passes&0x7fffffff)>8 || limit<scope->cursor ||
        limit-scope->cursor<(uint64_t)(passes&0x7fffffff)*128 ||
        jc_cloth_publication_view_prepare(&d->view,s->read,s->read_context,source,
            s->frames.frames[o->index].streams+d->role,d->first,d->count)) {
        jc_cloth_render_fail(o); ReleaseSRWLockExclusive(&jc_cloth_render_lock); return -1;
    }
    *redirected=&d->view.source;
    ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 1;
}
static void jc_cloth_render_build_after(uint32_t result) {
    struct jc_cloth_render_scope *scope=TlsGetValue(jc_cloth_render_tls);
    if (!scope) return;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    struct jc_cloth_render_owner *o=scope->owner; struct jc_cloth_live *s=o->backend;
    struct jc_cloth_render_draw *d=scope->draw; uintptr_t end,cursor;
    if (result || o->failed ||
        !jc_layout_field(s->read,s->read_context,scope->renderer,0x98,&end,8) ||
        !jc_layout_field(s->read,s->read_context,scope->renderer,0x68,&cursor,8) || cursor!=end ||
        jc_cloth_publication_view_check(&d->view,s->read,s->read_context,scope->cursor,end) ||
        !s->read(s->read_context,scope->cursor,d->snapshot,(size_t)(end-scope->cursor))) {
        jc_cloth_render_fail(o); ReleaseSRWLockExclusive(&jc_cloth_render_lock); return;
    }
    d->begin=scope->cursor; d->end=end; d->built=1;
    if (d->participant<2) ++jc_cloth_render_counts[d->participant][d->role][2];
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);
}
/* Native CPU tasks acquire queue reservations before they build commands.
   Handoff arrival order therefore need not be GPU execution order. Retain an
   EVENT for each containing batch, then require every EVENT to retire. */
typedef uint32_t (*jc_cloth_render_submit)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
typedef uint32_t (*jc_cloth_render_base_submit)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static uint32_t jc_cloth_render_forward(uintptr_t executor,uintptr_t context,uintptr_t fence,
        uintptr_t begin,uintptr_t end,jc_cloth_render_submit submit,
        jc_cloth_render_base_submit base_submit,int *handled) {
    *handled=0;
    struct jc_cloth_render_owner *active=NULL;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if (!o || o->submitted) continue;
        unsigned inside=0,remaining=0;
        for (unsigned n=0;n<o->count;++n) {
            struct jc_cloth_render_draw *d=o->draws+n;
            if (d->forwarded) continue;
            ++remaining;
            if (!d->built || end<=d->begin || begin>=d->end) continue;
            *handled=1; active=o;
            unsigned char bytes[8*128];
            if (begin>d->begin || end<d->end || o->backend->failed || o->backend->closed || o->failed ||
                !o->backend->read(o->backend->read_context,d->begin,bytes,(size_t)(d->end-d->begin)) ||
                memcmp(bytes,d->snapshot,(size_t)(d->end-d->begin))) goto failure;
            ++inside;
        }
        if (!inside) continue;
        unsigned char synchronous; uint32_t state;
        if ((!submit && !base_submit) || (!base_submit && !context) || o->ticket_count==JC_RETIRE_SLOTS ||
            !o->backend->read(o->backend->read_context,o->image+0x3c03cd0,&synchronous,1) || synchronous>1 ||
            !jc_layout_field(o->backend->read,o->backend->read_context,executor,0x20,&state,4) ||
            (!synchronous && state!=1) || (o->executor && o->executor!=executor) ||
            (o->executor_mode>=0 && o->executor_mode!=synchronous)) goto failure;
        o->executor=executor; o->executor_mode=synchronous;
        uint64_t ticket=jc_live_retirement_arm(o->backend->retirement,executor,begin,end);
        if (!ticket) goto failure;
        o->tickets[o->ticket_count++]=ticket;
        for (unsigned n=0;n<o->count;++n) if (o->draws[n].built && !o->draws[n].forwarded &&
            o->draws[n].begin>=begin && o->draws[n].end<=end) {
            o->draws[n].forwarded=1;
            if (o->draws[n].participant<2) ++jc_cloth_render_counts[o->draws[n].participant][o->draws[n].role][3];
        }
        ++o->handoffs;
        if (inside==remaining) {
            struct jc_cloth_live *s=o->backend; struct jc_cloth_live_commands *c=s->commands+f;
            c->executor=executor; c->context=context; c->fence=fence; c->begin=begin; c->end=end;
            c->ticket=o->tickets[0]; s->frames.frames[f].ticket=c->ticket;
            s->frames.frames[f].phase=JC_FRAME_QUEUED; o->submitted=1; s->automatic_pending[f]=0;
        }
        break;
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);
    if (!active) return 0;
    uint32_t result=base_submit ? base_submit(executor,begin,end,fence) :
        submit(executor,context,fence,begin,end);
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    --active->handoffs;
    if (result) jc_cloth_render_fail(active);
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);
    return result;
failure:
    *handled=1;
    if (active) jc_cloth_render_fail(active);
    ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 5;
}
static uint32_t jc_cloth_render_handoff(uintptr_t executor,uintptr_t context,uintptr_t fence,
        uintptr_t begin,uintptr_t end,jc_cloth_render_submit submit,int *handled) {
    return jc_cloth_render_forward(executor,context,fence,begin,end,submit,NULL,handled);
}
static uint32_t jc_cloth_render_base_handoff(uintptr_t executor,uintptr_t begin,uintptr_t end,
        uintptr_t fence,jc_cloth_render_base_submit submit,int *handled) {
    return jc_cloth_render_forward(executor,0,fence,begin,end,NULL,submit,handled);
}
static int jc_cloth_render_retired(struct jc_cloth_live *s,uint64_t ticket,int *handled) {
    *handled=0;
    if (!s || !ticket) return -1;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if (!o || o->backend!=s || !o->ticket_count || o->tickets[0]!=ticket) continue;
        *handled=1;
        if (!jc_cloth_live_owner(s) || o->failed) {
            ReleaseSRWLockExclusive(&jc_cloth_render_lock); return -1;
        }
        if (!o->submitted || o->building || o->handoffs) {
            ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 0;
        }
        for (unsigned i=0;i<o->ticket_count;++i) if (!(o->ticket_done&(1u<<i))) {
            int result=jc_live_retirement_result(s->retirement,o->tickets[i],1);
            if (result<0) {
                jc_cloth_render_fail(o); ReleaseSRWLockExclusive(&jc_cloth_render_lock); return -1;
            }
            if (result) o->ticket_done|=1u<<i;
        }
        if (o->ticket_done!=((1u<<o->ticket_count)-1)) {
            ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 0;
        }
        s->automatic_pending[f]=0; jc_cloth_render_owners[f]=NULL;
        HeapFree(GetProcessHeap(),0,o); memset(s->commands+f,0,sizeof(s->commands[f]));
        ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 1;
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock); return -1;
}
static inline int jc_cloth_render_frame_pending(struct jc_cloth_live *s,uint64_t native_frame) {
    int pending=0;
    if(!s || !native_frame)return 0;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for(unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        const struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if(o && o->backend==s && o->native_frame==native_frame) {pending=1;break;}
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);return pending;
}
/* A successfully retired whole-frame marker also covers models omitted by
   native culling. Never use an earlier frame's marker for a newer owner. */
static inline int jc_cloth_render_frame_complete(struct jc_cloth_live *s,uint64_t native_frame) {
    if (!jc_cloth_live_owner(s) || !native_frame) return -1;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if (!o || o->backend!=s) continue;
        if (o->native_frame!=native_frame) {
            ReleaseSRWLockExclusive(&jc_cloth_render_lock);return 0;
        }
        if (o->failed || o->building || o->handoffs || s->commands[f].owner!=o ||
            s->frames.frames[f].generation!=o->generation ||
            (s->frames.frames[f].phase!=JC_FRAME_READY && s->frames.frames[f].phase!=JC_FRAME_QUEUED)) {
            jc_cloth_render_fail(o);ReleaseSRWLockExclusive(&jc_cloth_render_lock);return -1;
        }
        for (unsigned n=0;n<o->count;++n) if(o->draws[n].built && !o->draws[n].forwarded) {
            jc_cloth_render_fail(o);ReleaseSRWLockExclusive(&jc_cloth_render_lock);return -1;
        }
        for(unsigned n=0;n<o->ticket_count;++n) if(!(o->ticket_done&(1u<<n))) {
            int result=jc_live_retirement_result(s->retirement,o->tickets[n],1);
            if(result<0) {
                jc_cloth_render_fail(o);ReleaseSRWLockExclusive(&jc_cloth_render_lock);return -1;
            }
            if(result) o->ticket_done|=1u<<n;
        }
        if(o->ticket_done!=((1u<<o->ticket_count)-1)) {
            ReleaseSRWLockExclusive(&jc_cloth_render_lock);return 0;
        }
        s->automatic_pending[f]=0;jc_cloth_render_owners[f]=NULL;
        memset(s->commands+f,0,sizeof(s->commands[f]));HeapFree(GetProcessHeap(),0,o);
        ReleaseSRWLockExclusive(&jc_cloth_render_lock);
        jc_cloth_frame_release(&s->frames,s->frames.frames+f);
        return s->failed ? -1 : 1;
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock);return 1;
}
static inline int jc_cloth_render_base_range(uintptr_t begin,uintptr_t end) {
    int owned=0;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if (!o) continue;
        for (unsigned n=0;n<o->count;++n) if (o->draws[n].built && !o->draws[n].forwarded &&
            begin<o->draws[n].end && o->draws[n].begin<end) { jc_cloth_render_fail(o); owned=1; }
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock); return owned;
}
static int jc_cloth_render_discard(struct jc_cloth_live_commands *commands) {
    if (!commands || !commands->owner) return 0;
    AcquireSRWLockExclusive(&jc_cloth_render_lock);
    for (unsigned f=0;f<JC_CLOTH_FRAMES;++f) {
        struct jc_cloth_render_owner *o=jc_cloth_render_owners[f];
        if (!o || commands->owner!=o) continue;
        if (o->failed || o->building || o->handoffs || !o->submitted || !commands->ticket ||
            o->ticket_done!=((1u<<o->ticket_count)-1)) {
            jc_cloth_render_fail(o); ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 1;
        }
        o->backend->automatic_pending[f]=0; jc_cloth_render_owners[f]=NULL;
        HeapFree(GetProcessHeap(),0,o); memset(commands,0,sizeof(*commands));
        ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 1;
    }
    ReleaseSRWLockExclusive(&jc_cloth_render_lock); return 0;
}
#endif
#endif
