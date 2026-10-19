#ifndef JC_NATIVE_CLOTH_LIVE_H
#define JC_NATIVE_CLOTH_LIVE_H
#include "native_cloth_frame.h"
#include "native_render_retirement.h"
#include "native_cloth_material_live.h"
#include "native_cloth_commands.h"
#include "native_environment_probe.h"
#if defined(_WIN32) || defined(JC_CLOTH_LIVE_TEST_PLATFORM)
#define JC_CLOTH_BUFFERS (JC_CLOTH_FRAMES*JC_CLOTH_ROLES*2u)
struct jc_cloth_live_buffer {
    ID3D11Buffer *object;
    void *mapped;
    unsigned bytes,kind,pins;
};
struct jc_cloth_live_commands {
    uintptr_t executor,context,fence,begin,end;
    void *owner;
    uint64_t ticket;
};
#ifdef _WIN32
struct jc_cloth_stock_batch {
    uintptr_t begin,end;
    unsigned char *snapshot,*commands;
    unsigned count;
};
#endif
/* prepare builds privately, without exposing commands to the renderer. Its
   owner retains every command dependency, including stock draws using these
   streams, until discard. A return of zero must leave no published work. */
struct jc_cloth_live_ops {
    void *context;
    int (*workers_done)(void *,uint64_t);
    int (*prepare)(void *,uint64_t,const struct jc_cloth_frame_draw *,unsigned,
                   struct jc_cloth_live_commands *);
    void (*discard)(void *,struct jc_cloth_live_commands *);
    uint32_t (*handoff)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
    int (*atlas_pin)(void *,uint64_t);
    void (*atlas_unpin)(void *,uint64_t);
};
struct jc_cloth_live {
    struct jc_cloth_frames frames;
    struct jc_cloth_live_buffer buffers[JC_CLOTH_BUFFERS];
    struct jc_cloth_live_material *materials[JC_CLOTH_FRAMES*JC_CLOTH_DRAWS];
    struct jc_cloth_live_commands commands[JC_CLOTH_FRAMES];
    unsigned automatic_pending[JC_CLOTH_FRAMES];
#ifdef _WIN32
    struct jc_cloth_command_stage *native_stages[JC_CLOTH_FRAMES];
    struct jc_cloth_stock_batch stock_batches[JC_CLOTH_FRAMES];
    struct jc_environment_probe probes[JC_CLOTH_FRAMES*(JC_CLOTH_PARTICIPANTS-1)];
    uint64_t probe_serial;
#endif
    struct jc_live_retirement *retirement;
    struct jc_cloth_live_ops ops;
    jc_layout_reader read;
    void *read_context;
    ID3D11DeviceContext *device_context;
    uint64_t graphics_image;
    struct jc_cloth_graphics_ops graphics;
    DWORD thread;
    unsigned failed,closed;
};
static inline void jc_cloth_live_discard(struct jc_cloth_live *s,struct jc_cloth_live_commands *c) {
#ifdef _WIN32
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) if (s->native_stages[i] && c->owner==s->native_stages[i]) {
        if (s->stock_batches[i].snapshot) HeapFree(GetProcessHeap(),0,s->stock_batches[i].snapshot);
        if (s->stock_batches[i].commands) HeapFree(GetProcessHeap(),0,s->stock_batches[i].commands);
        memset(s->stock_batches+i,0,sizeof(s->stock_batches[i]));
        HeapFree(GetProcessHeap(),0,s->native_stages[i]); s->native_stages[i]=NULL;
        memset(c,0,sizeof(*c)); return;
    }
#endif
    s->ops.discard(s->ops.context,c);
}
static inline int jc_cloth_live_owner(const struct jc_cloth_live *s) {
    return s && s->thread==GetCurrentThreadId() && !s->failed && !s->closed;
}
static inline int jc_cloth_live_graphics_enter(struct jc_cloth_live *s,struct jc_cloth_graphics_token *token) {
    return !s->graphics_image || !jc_cloth_graphics_lock(s->read,s->read_context,s->graphics_image,
        &s->graphics,(uintptr_t)s->device_context,token);
}
static inline int jc_cloth_live_graphics_leave(struct jc_cloth_live *s,struct jc_cloth_graphics_token *token) {
    if (!s->graphics_image || !jc_cloth_graphics_unlock(s->read,s->read_context,s->graphics_image,&s->graphics,token)) return 1;
    s->failed=1;return 0;
}
static inline struct jc_cloth_live_buffer *jc_cloth_live_buffer_find(struct jc_cloth_live *s,uint64_t object) {
    for (unsigned i=0;i<JC_CLOTH_BUFFERS;++i)
        if (object && (uintptr_t)s->buffers[i].object==object) return s->buffers+i;
    return NULL;
}
static inline struct jc_cloth_live_material *jc_cloth_live_material_find(struct jc_cloth_live *s,uint64_t object) {
    for (unsigned i=0;i<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++i) {
        struct jc_cloth_live_material *m=s->materials[i];
        if (m && (uintptr_t)m->subscriber+0x88==object) return m;
    }
    return NULL;
}
#ifdef _WIN32
static inline struct jc_environment_probe *jc_cloth_live_probe_find(struct jc_cloth_live *s,uint64_t texture) {
    for (unsigned i=0;i<JC_CLOTH_FRAMES*(JC_CLOTH_PARTICIPANTS-1);++i)
        if (texture && s->probes[i].pair[0]==texture) return s->probes+i;
    return NULL;
}
static inline struct jc_environment_probe *jc_cloth_live_probe_prepare(struct jc_cloth_live *s,
        uint64_t image,uint64_t barn,uint64_t peer,uint64_t generation) {
    if (!jc_cloth_live_owner(s) || s->frames.closing) return NULL;
    struct jc_environment_probe *empty=NULL;
    for (unsigned i=0;i<JC_CLOTH_FRAMES*(JC_CLOTH_PARTICIPANTS-1);++i) {
        struct jc_environment_probe *p=s->probes+i;
        if (p->peer==peer && p->generation==generation && !p->pins)
            return !jc_environment_probe_check(s->read,s->read_context,p) ? p : NULL;
        if (!p->peer && !empty) empty=p;
    }
    if (!empty || s->probe_serial==UINT64_MAX) return NULL;
    if (jc_environment_probe_create(s->read,s->read_context,image,barn,peer,generation,++s->probe_serial,empty)) {
        if (empty->quarantined) s->failed=1;
        return NULL;
    }
    for (unsigned i=0;i<JC_CLOTH_FRAMES*(JC_CLOTH_PARTICIPANTS-1);++i) {
        const struct jc_environment_probe *other=s->probes+i;
        if (other==empty || !other->peer) continue;
        if (other->identity.texture==empty->identity.texture || other->identity.handle==empty->identity.handle ||
            other->identity.resource==empty->identity.resource) {
            empty->quarantined=1;s->failed=1;return NULL;
        }
    }
    return empty;
}
static inline int jc_cloth_live_bindings_match(struct jc_cloth_live *s,
        const struct jc_cloth_material *d) {
    struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,d->material);
    if (!m || !m->ready || m->quarantined || m->peer!=d->peer ||
        m->generation!=d->generation || m->role!=d->role || m->atlas_outfit!=d->outfit)
        return 0;
    struct jc_environment_probe *p=jc_cloth_live_probe_find(s,d->environment);
    if (!p || p->peer!=d->peer || p->generation!=d->generation || !p->produced ||
        jc_environment_probe_check(s->read,s->read_context,p)) return 0;
    for (unsigned i=0;i<(d->role==5 ? 6u : 4u);++i) {
        uint64_t texture=0;
        if (jc_cloth_live_atlas_check(s->read,s->read_context,m,i,&texture) ||
            texture!=d->atlas[i] || texture!=m->atlas_textures[i]) return 0;
    }
    return !jc_cloth_texture_bundle_bound(s->read,s->read_context,d->material,d->role,d->atlas,d->environment);
}
#endif
static inline int jc_cloth_live_acquire(void *context,unsigned kind,uint64_t object) {
    struct jc_cloth_live *s=context;
    if (!jc_cloth_live_owner(s) || s->frames.closing) return 0;
    if (kind==JC_FRAME_VERTEX || kind==JC_FRAME_INDEX) {
        struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,object);
        if (!b || b->kind!=kind || b->pins || !b->mapped) return 0;
        b->pins=1; return 1;
    }
    if (kind==JC_FRAME_MATERIAL_REF) {
        struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,object);
        return m && !m->pins && !jc_cloth_live_material_pin(s->read,s->read_context,m,m->peer,m->generation);
    }
#ifdef _WIN32
    if (kind==JC_FRAME_TEXTURE) {
        for (unsigned m=0;m<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++m) if (s->materials[m])
            for (unsigned i=0;i<(s->materials[m]->role==5 ? 6u : 4u);++i)
                if (s->materials[m]->atlas_textures[i]==object)
                    return jc_cloth_atlas_snapshot_pin(&s->materials[m]->atlas_snapshots[i],object);
        struct jc_environment_probe *p=jc_cloth_live_probe_find(s,object);
        if (p) {
            if (!p->produced || p->pins==UINT32_MAX || jc_environment_probe_check(s->read,s->read_context,p)) return 0;
            ++p->pins;return 1;
        }
    }
#endif
    return kind==JC_FRAME_TEXTURE && s->ops.atlas_pin(s->ops.context,object)==1;
}
static inline void jc_cloth_live_release(void *context,unsigned kind,uint64_t object) {
    struct jc_cloth_live *s=context;
    if (!s || s->thread!=GetCurrentThreadId() || s->closed) { if (s) s->failed=1; return; }
    if (kind==JC_FRAME_VERTEX || kind==JC_FRAME_INDEX) {
        struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,object);
        if (!b || b->kind!=kind || b->pins!=1) { s->failed=1; return; }
        b->pins=0;
    } else if (kind==JC_FRAME_MATERIAL_REF) {
        struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,object);
        if (!m || jc_cloth_live_material_unpin(m)) s->failed=1;
    } else if (kind==JC_FRAME_TEXTURE) {
#ifdef _WIN32
        for (unsigned m=0;m<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++m) if (s->materials[m])
            for (unsigned i=0;i<(s->materials[m]->role==5 ? 6u : 4u);++i)
                if (s->materials[m]->atlas_textures[i]==object) {
                    if (!jc_cloth_atlas_snapshot_unpin(&s->materials[m]->atlas_snapshots[i],object)) s->failed=1;
                    return;
                }
        struct jc_environment_probe *p=jc_cloth_live_probe_find(s,object);
        if (p) { if (!p->pins) s->failed=1;else --p->pins;return; }
#endif
        s->ops.atlas_unpin(s->ops.context,object);
    }
    else s->failed=1;
}
static inline int jc_cloth_live_workers(void *context,uint64_t generation) {
    struct jc_cloth_live *s=context;
    return jc_cloth_live_owner(s) ? s->ops.workers_done(s->ops.context,generation) : -1;
}
static inline int jc_cloth_live_publish(void *context,uint64_t generation,
        const struct jc_cloth_frame_draw *draws,unsigned count,uint64_t *ticket) {
    struct jc_cloth_live *s=context;
    if (!jc_cloth_live_owner(s) || !ticket || *ticket) return -1;
    struct jc_cloth_frame *frame=NULL;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i)
        if (s->frames.frames[i].generation==generation && s->frames.frames[i].draws==draws)
            frame=s->frames.frames+i;
    if (!frame || frame->phase!=JC_FRAME_READY || !frame->stock_committed || count!=frame->draw_count) return -1;
#ifdef _WIN32
    for (unsigned i=0;i<count;++i) if (!jc_cloth_live_bindings_match(s,&draws[i].material)) return -1;
#endif
    for (unsigned i=0;i<frame->ref_count;++i) {
        const struct jc_cloth_frame_ref *r=frame->refs+i;
        if (r->kind!=JC_FRAME_VERTEX && r->kind!=JC_FRAME_INDEX) continue;
        struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,r->object);
        if (!b || b->pins!=1 || b->mapped) return 0;
    }
    unsigned slot=(unsigned)(frame-s->frames.frames);
    if (s->automatic_pending[slot]) return 0;
    if (s->commands[slot].ticket) return -1;
    struct jc_cloth_live_commands *c=s->commands+slot;
    int prepared;
#ifdef _WIN32
    if (s->stock_batches[slot].begin) return 0;
    if (s->native_stages[slot]) prepared=1;
    else
#endif
        prepared=s->ops.prepare(s->ops.context,generation,draws,count,c);
    if (prepared!=1) {
        if (!prepared) { jc_cloth_live_discard(s,c); memset(c,0,sizeof(*c)); return 0; }
        s->failed=1; return -1;
    }
    if (!c->context || !c->fence || !c->owner || (c->begin&7)) {
        jc_cloth_live_discard(s,c); memset(c,0,sizeof(*c)); return 0;
    }
    c->ticket=jc_live_retirement_arm(s->retirement,c->executor,c->begin,c->end);
    if (!c->ticket) {
#ifdef _WIN32
        if (s->native_stages[slot]) return 0;
#endif
        jc_cloth_live_discard(s,c); memset(c,0,sizeof(*c)); return 0;
    }
    *ticket=c->ticket;
    /* The native handoff may execute synchronously. Arm before calling it;
       any error afterward is uncertain, never an acquisition rollback. */
    if (s->ops.handoff(c->executor,c->context,c->fence,c->begin,c->end)) { s->failed=1; return -1; }
    return 1;
}
static inline int jc_cloth_live_retired(void *context,uint64_t ticket) {
    struct jc_cloth_live *s=context;
    if (!jc_cloth_live_owner(s)) return -1;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) if (ticket && s->commands[i].ticket==ticket) {
        int result=jc_live_retirement_result(s->retirement,ticket,1);
        if (result==1) {
            jc_cloth_live_discard(s,s->commands+i);
            memset(s->commands+i,0,sizeof(s->commands[i]));
        }
        return result;
    }
    return -1;
}
#ifdef _WIN32
/* At the owning render-frame boundary, with source dependencies frozen through
   its native fence. This stages stock and extra cloth into the same batch. */
static inline int jc_cloth_live_stage_native(struct jc_cloth_live *s,unsigned index,
        jc_native_command_build build,const struct jc_cloth_command_source stock[7],
        const struct jc_cloth_command_source *extras,uintptr_t executor,uintptr_t context,uintptr_t fence) {
    if (!jc_cloth_live_owner(s) || index>=JC_CLOTH_FRAMES || !stock || !build ||
        ((executor || context || fence) && (!executor || !context || !fence)) ||
        s->native_stages[index] || s->commands[index].owner)
        return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames.frames+index;
    if (f->phase!=JC_FRAME_READY || !f->stock_committed || (f->draw_count && !extras)) return JC_FRAME_BUSY;
    for (unsigned i=0;i<f->draw_count;++i)
        if (!jc_cloth_live_bindings_match(s,&f->draws[i].material)) return JC_FRAME_MATERIAL;
    struct jc_cloth_command_stage *stage=HeapAlloc(GetProcessHeap(),0,sizeof(*stage));
    if (!stage) return JC_FRAME_RETAIN;
    jc_cloth_command_stage_init(stage);
    for (unsigned r=0;r<7;++r) {
        stage->role_first[r]=stage->count;
        const struct jc_cloth_partition_range *range=&f->route.plan.ranges[0][r];
        if (range->indices && jc_cloth_command_stage_add(stage,s->read,s->read_context,build,
                stock+r,f->streams+r,range->index_begin,range->indices)) goto rejected;
        for (unsigned i=0;i<f->draw_count;++i) {
            const struct jc_cloth_frame_draw *d=f->draws+i;
            if (d->material.role!=r) continue;
            const struct jc_cloth_partition_range *part=&f->route.plan.ranges[d->material.participant][r];
            if (extras[i].material!=d->material.material ||
                jc_cloth_command_stage_add(stage,s->read,s->read_context,build,extras+i,
                    f->streams+r,part->index_begin,part->indices)) goto rejected;
        }
        stage->role_count[r]=stage->count-stage->role_first[r];
    }
    if (!stage->count) goto rejected;
    s->native_stages[index]=stage;
    s->commands[index]=(struct jc_cloth_live_commands){executor,context,fence,
        (uintptr_t)stage->commands,(uintptr_t)stage->commands+stage->count*128u,stage,0};
    return JC_FRAME_OK;
rejected:
    HeapFree(GetProcessHeap(),0,stage); return JC_FRAME_QUEUE;
}
static inline int jc_cloth_live_bind_stock_batch(struct jc_cloth_live *s,unsigned index,
        uintptr_t begin,uintptr_t end,const struct jc_cloth_command_source stock[7],
        const unsigned first[7],const unsigned counts[7]) {
    if (!jc_cloth_live_owner(s) || index>=JC_CLOTH_FRAMES || !stock || !first || !counts ||
        !begin || (begin&7) || end<=begin || (end-begin)%128 || end-begin>65536u*128u ||
        s->stock_batches[index].begin || !s->native_stages[index] || s->commands[index].ticket)
        return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames.frames+index;
    if (f->phase!=JC_FRAME_READY || !f->stock_committed) return JC_FRAME_BUSY;
    for (unsigned i=0;i<f->ref_count;++i) if (f->refs[i].kind==JC_FRAME_VERTEX || f->refs[i].kind==JC_FRAME_INDEX) {
        struct jc_cloth_live_buffer *v=jc_cloth_live_buffer_find(s,f->refs[i].object);
        if (!v || v->pins!=1 || v->mapped) return JC_FRAME_BUSY;
    }
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i)
        if (s->stock_batches[i].begin && begin<s->stock_batches[i].end && s->stock_batches[i].begin<end)
            return JC_FRAME_BUSY;
    struct jc_cloth_command_stage *stage=s->native_stages[index];
    struct jc_cloth_command_patch patches[7]; unsigned n=0,count=(unsigned)((end-begin)/128);
    uintptr_t tables[7]={0};
    for (unsigned r=0;r<7;++r) {
        if (!counts[r]) { if (stage->role_count[r]) return JC_FRAME_ARGUMENT; continue; }
        if (first[r]>count || counts[r]>count-first[r]) return JC_FRAME_ARGUMENT;
        uint32_t passes=0; uintptr_t table=0;
        if (!s->read(s->read_context,stock[r].passes+8,&passes,4) ||
            !s->read(s->read_context,stock[r].passes+0x10,&table,8) ||
            (passes&0x7fffffff)!=counts[r] || counts[r]>8 || !table) return JC_FRAME_ARGUMENT;
        if (!jc_cloth_span(table,(uint64_t)counts[r]*0x2c0)) return JC_FRAME_ARGUMENT;
        tables[r]=table;
        for (unsigned p=0;p<counts[r];++p) {
            uintptr_t pass=0;
            if (!s->read(s->read_context,begin+(first[r]+p)*128u+0x28,&pass,8) ||
                pass!=table+p*0x2c0) return JC_FRAME_ARGUMENT;
        }
        patches[n++]=(struct jc_cloth_command_patch){first[r],counts[r],stage->role_first[r],
            stage->role_count[r],stock[r].material};
    }
    for (unsigned i=1;i<n;++i) for (unsigned j=i;j && patches[j].first<patches[j-1].first;--j) {
        struct jc_cloth_command_patch p=patches[j]; patches[j]=patches[j-1]; patches[j-1]=p;
    }
    struct jc_cloth_stock_batch b={begin,end,NULL,NULL,0};
    b.snapshot=HeapAlloc(GetProcessHeap(),0,(SIZE_T)(end-begin));
    b.commands=HeapAlloc(GetProcessHeap(),0,(count+stage->count)*128u);
    if (!b.snapshot || !b.commands || !s->read(s->read_context,begin,b.snapshot,(size_t)(end-begin)) ||
        jc_cloth_command_splice(b.snapshot,count,stage,patches,n,b.commands,count+stage->count,&b.count)) {
        if (b.snapshot) HeapFree(GetProcessHeap(),0,b.snapshot);
        if (b.commands) HeapFree(GetProcessHeap(),0,b.commands);
        return JC_FRAME_QUEUE;
    }
    for (unsigned r=0;r<7;++r) for (unsigned p=0;p<counts[r];++p) {
        uintptr_t pass=0;
        memcpy(&pass,b.snapshot+(first[r]+p)*128u+0x28,8);
        if (pass!=tables[r]+p*0x2c0) {
            HeapFree(GetProcessHeap(),0,b.snapshot); HeapFree(GetProcessHeap(),0,b.commands);
            return JC_FRAME_QUEUE;
        }
    }
    s->stock_batches[index]=b; return JC_FRAME_OK;
}
static inline uint32_t jc_cloth_live_stock_handoff(struct jc_cloth_live *s,uintptr_t executor,
        uintptr_t context,uintptr_t fence,uintptr_t begin,uintptr_t end,int *handled) {
    *handled=0;
    if (!s) return 0;
    for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) {
        struct jc_cloth_stock_batch *b=s->stock_batches+i;
        if (!b->begin || end<=b->begin || begin>=b->end) continue;
        *handled=1;
        struct jc_cloth_frame *f=s->frames.frames+i;
        struct jc_cloth_live_commands *c=s->commands+i;
        if (!jc_cloth_live_owner(s) || b->begin!=begin || b->end!=end ||
            f->phase!=JC_FRAME_READY || c->ticket) goto uncertain;
        for (unsigned j=0;j<f->draw_count;++j)
            if (!jc_cloth_live_bindings_match(s,&f->draws[j].material)) goto uncertain;
        unsigned char chunk[4096];
        for (size_t offset=0;offset<end-begin;) {
            size_t bytes=(size_t)(end-begin)-offset;
            if (bytes>sizeof(chunk)) bytes=sizeof(chunk);
            if (!s->read(s->read_context,begin+offset,chunk,bytes) || memcmp(chunk,b->snapshot+offset,bytes))
                goto uncertain;
            offset+=bytes;
        }
        c->executor=executor; c->context=context; c->fence=fence;
        c->begin=(uintptr_t)b->commands; c->end=c->begin+b->count*128u;
        c->ticket=jc_live_retirement_arm(s->retirement,executor,c->begin,c->end);
        if (!c->ticket) return 5;
        f->ticket=c->ticket; f->phase=JC_FRAME_QUEUED;
        uint32_t result=s->ops.handoff(executor,context,fence,c->begin,c->end);
        if (!result) return 0;
uncertain:
        f->phase=JC_FRAME_UNKNOWN; s->failed=1; return 5;
    }
    return 0;
}
#endif
static inline int jc_cloth_live_init(struct jc_cloth_live *s,ID3D11DeviceContext *context,
        struct jc_live_retirement *retirement,jc_layout_reader read,void *read_context,
        struct jc_cloth_live_ops ops) {
    if (!s || !context || !retirement || !read || !ops.workers_done || !ops.prepare ||
        !ops.discard || !ops.handoff || !ops.atlas_pin || !ops.atlas_unpin ||
        context->lpVtbl->GetType(context)!=D3D11_DEVICE_CONTEXT_IMMEDIATE) return JC_FRAME_ARGUMENT;
    /* Fresh storage only. Reinitializing a live or quarantined session leaks
       ownership and is forbidden, just as for jc_cloth_frames_init. */
    memset(s,0,sizeof(*s)); s->ops=ops; s->read=read; s->read_context=read_context;
    s->retirement=retirement; s->device_context=context; s->thread=GetCurrentThreadId();
    context->lpVtbl->AddRef(context);
    return jc_cloth_frames_init(&s->frames,(struct jc_cloth_frame_backend){s,
        jc_cloth_live_acquire,jc_cloth_live_release,jc_cloth_live_workers,
        jc_cloth_live_publish,jc_cloth_live_retired});
}
static inline int jc_cloth_live_buffer_create_locked(struct jc_cloth_live *s,unsigned kind,unsigned bytes,
        struct jc_cloth_live_buffer **out) {
    if (!out) return JC_FRAME_ARGUMENT;
    *out=NULL;
    if (!jc_cloth_live_owner(s) || s->frames.closing ||
        (kind!=JC_FRAME_VERTEX && kind!=JC_FRAME_INDEX) || !bytes || bytes>64u*1024u*1024u)
        return JC_FRAME_ARGUMENT;
    unsigned i=0; while (i<JC_CLOTH_BUFFERS && s->buffers[i].object) ++i;
    if (i==JC_CLOTH_BUFFERS) return JC_FRAME_BUSY;
    ID3D11Device *device=NULL; ID3D11Buffer *buffer=NULL;
    s->device_context->lpVtbl->GetDevice(s->device_context,&device);
    if (!device) return JC_FRAME_RETAIN;
    D3D11_BUFFER_DESC desc={0}; desc.ByteWidth=bytes; desc.Usage=D3D11_USAGE_DYNAMIC;
    desc.BindFlags=kind==JC_FRAME_VERTEX ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    HRESULT hr=device->lpVtbl->CreateBuffer(device,&desc,NULL,&buffer);
    device->lpVtbl->Release(device);
    if (FAILED(hr) || !buffer) { if (buffer) buffer->lpVtbl->Release(buffer); return JC_FRAME_RETAIN; }
    s->buffers[i]=(struct jc_cloth_live_buffer){.object=buffer,.bytes=bytes,.kind=kind};
    *out=s->buffers+i; return JC_FRAME_OK;
}
static inline int jc_cloth_live_buffer_create(struct jc_cloth_live *s,unsigned kind,unsigned bytes,
        struct jc_cloth_live_buffer **out) {
    if (!jc_cloth_live_owner(s)) return JC_FRAME_ARGUMENT;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) return JC_FRAME_OWNERSHIP;
    int code=jc_cloth_live_buffer_create_locked(s,kind,bytes,out);
    return jc_cloth_live_graphics_leave(s,&token) ? code : JC_FRAME_QUARANTINED;
}
static inline int jc_cloth_live_buffer_map(struct jc_cloth_live *s,struct jc_cloth_live_buffer *b) {
    if (!jc_cloth_live_owner(s) || s->frames.closing || !b ||
        jc_cloth_live_buffer_find(s,(uintptr_t)b->object)!=b || b->pins || b->mapped) return JC_FRAME_BUSY;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) return JC_FRAME_OWNERSHIP;
    D3D11_MAPPED_SUBRESOURCE map={0};
    HRESULT hr=s->device_context->lpVtbl->Map(s->device_context,(ID3D11Resource *)b->object,
        0,D3D11_MAP_WRITE_DISCARD,0,&map);
    int code=JC_FRAME_OK;
    if (FAILED(hr)) code=JC_FRAME_RETAIN;
    else if (!map.pData) {
        s->device_context->lpVtbl->Unmap(s->device_context,(ID3D11Resource *)b->object,0);code=JC_FRAME_RETAIN;
    } else b->mapped=map.pData;
    return jc_cloth_live_graphics_leave(s,&token) ? code : JC_FRAME_QUARANTINED;
}
static inline void jc_cloth_live_streams_abandon(struct jc_cloth_live *s,
        struct jc_cloth_stream streams[7]) {
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) { s->failed=1;return; }
    for (unsigned r=0;r<7;++r) for (unsigned k=0;k<2;++k) {
        struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,k ? streams[r].index : streams[r].vertex);
        if (b && !b->pins && b->mapped) {
            s->device_context->lpVtbl->Unmap(s->device_context,(ID3D11Resource *)b->object,0);
            b->mapped=NULL;
        }
    }
    if (jc_cloth_live_graphics_leave(s,&token)) memset(streams,0,7*sizeof(*streams));
}
/* Size from the completed native job list, not last frame's counts. Reuse
   only retired buffers; all maps roll back before any native pointer change. */
static inline int jc_cloth_live_streams_prepare(struct jc_cloth_live *s,
        const struct jc_cloth_partition *plan,struct jc_cloth_stream streams[7]) {
    if (!jc_cloth_live_owner(s) || s->frames.closing || !plan || !plan->ready || !streams)
        return JC_FRAME_ARGUMENT;
    memset(streams,0,7*sizeof(*streams));
    int code=JC_FRAME_OK;
    for (unsigned r=0;r<7;++r) {
        if (!plan->vertex_capacity[r] && !plan->index_capacity[r]) continue;
        uint64_t bytes[2]={(uint64_t)plan->vertex_capacity[r]*60,(uint64_t)plan->index_capacity[r]*4};
        if (!bytes[0] || !bytes[1] || bytes[0]>64u*1024u*1024u || bytes[1]>64u*1024u*1024u) {
            code=JC_FRAME_ARGUMENT;goto failed;
        }
        for (unsigned k=0;k<2;++k) {
            unsigned kind=k ? JC_FRAME_INDEX : JC_FRAME_VERTEX;
            struct jc_cloth_live_buffer *best=NULL,*unused=NULL;
            for (unsigned i=0;i<JC_CLOTH_BUFFERS;++i) {
                struct jc_cloth_live_buffer *b=s->buffers+i;
                if (!b->object || b->kind!=kind || b->pins || b->mapped) continue;
                if (b->bytes>=bytes[k] && (!best || b->bytes<best->bytes)) best=b;
                if (!unused || b->bytes<unused->bytes) unused=b;
            }
            if (!best) {
                if (unused) {
                    unused->object->lpVtbl->Release(unused->object);
                    memset(unused,0,sizeof(*unused));
                }
                code=jc_cloth_live_buffer_create(s,kind,(unsigned)bytes[k],&best);
                if (code) goto failed;
            }
            code=jc_cloth_live_buffer_map(s,best);if (code) goto failed;
            if (k) {
                streams[r].index=(uintptr_t)best->object;streams[r].index_bytes=best->bytes;
                streams[r].mapped_indices=best->mapped;streams[r].indices=plan->index_capacity[r];
            } else {
                streams[r].vertex=(uintptr_t)best->object;streams[r].vertex_bytes=best->bytes;
                streams[r].mapped_vertices=best->mapped;streams[r].vertices=plan->vertex_capacity[r];
            }
        }
    }
    return JC_FRAME_OK;
failed:
    jc_cloth_live_streams_abandon(s,streams);return code;
}
/* Call after seal, stock count commit AND stock finalization. Finalization can
   still write fallback indices; unmapping immediately after workers is early. */
static inline int jc_cloth_live_unmap(struct jc_cloth_live *s,unsigned index) {
    if (!jc_cloth_live_owner(s)) return JC_FRAME_QUARANTINED;
    if (index>=JC_CLOTH_FRAMES) return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames.frames+index;
    if (f->phase!=JC_FRAME_READY || !f->stock_committed) return JC_FRAME_BUSY;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) return JC_FRAME_OWNERSHIP;
    for (unsigned i=0;i<f->ref_count;++i) {
        struct jc_cloth_frame_ref *r=f->refs+i;
        if (r->kind!=JC_FRAME_VERTEX && r->kind!=JC_FRAME_INDEX) continue;
        struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,r->object);
        if (!b || b->pins!=1 || !b->mapped) {
            s->failed=1; f->phase=JC_FRAME_UNKNOWN;
            jc_cloth_live_graphics_leave(s,&token);return JC_FRAME_QUARANTINED;
        }
        s->device_context->lpVtbl->Unmap(s->device_context,(ID3D11Resource *)b->object,0);
        b->mapped=NULL;
    }
    return jc_cloth_live_graphics_leave(s,&token) ? JC_FRAME_OK : JC_FRAME_QUARANTINED;
}
static inline int jc_cloth_live_begin(struct jc_cloth_live *s,uint64_t generation,
        const struct jc_cloth_partition_job *jobs,size_t count,const void *args,
        const struct jc_cloth_stream streams[7],const struct jc_cloth_material *materials,
        unsigned material_count,unsigned *index) {
    if (!jc_cloth_live_owner(s) || !streams || (!materials && material_count) ||
        material_count>JC_CLOTH_DRAWS) return JC_FRAME_ARGUMENT;
    for (unsigned r=0;r<7;++r) {
        const struct jc_cloth_stream *v=streams+r;
        if (!v->vertices && !v->indices) continue;
        struct jc_cloth_live_buffer *vb=jc_cloth_live_buffer_find(s,v->vertex);
        struct jc_cloth_live_buffer *ib=jc_cloth_live_buffer_find(s,v->index);
        if (!vb || !ib || vb->kind!=JC_FRAME_VERTEX || ib->kind!=JC_FRAME_INDEX ||
            !vb->mapped || !ib->mapped || vb->mapped!=v->mapped_vertices || ib->mapped!=v->mapped_indices ||
            v->vertex_offset || v->index_offset || vb->bytes!=v->vertex_bytes || ib->bytes!=v->index_bytes)
            return JC_FRAME_OWNERSHIP;
    }
    for (unsigned i=0;i<material_count;++i) {
        const struct jc_cloth_material *d=materials+i;
        struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,d->material);
        if (!m || m->peer!=d->peer || m->generation!=d->generation || m->role!=d->role)
            return JC_FRAME_MATERIAL;
#ifdef _WIN32
        if (!jc_cloth_live_bindings_match(s,d)) return JC_FRAME_MATERIAL;
#endif
        unsigned span=0;
        while (span<m->storage.count && (m->storage.spans[span].begin!=d->uniforms ||
                m->storage.spans[span].bytes!=d->uniform_bytes)) ++span;
        if (span==m->storage.count) return JC_FRAME_MATERIAL;
    }
    return jc_cloth_frame_begin(&s->frames,generation,jobs,count,args,streams,materials,material_count,index);
}
static inline int jc_cloth_live_register_material(struct jc_cloth_live *s,struct jc_cloth_live_material *m) {
    if (!jc_cloth_live_owner(s) || s->frames.closing || !m || !m->ready || m->quarantined ||
        m->pins || m->thread!=s->thread || !m->subscriber) return JC_FRAME_MATERIAL;
    unsigned slot=JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;
    for (unsigned i=0;i<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++i) {
        if (s->materials[i]==m) return JC_FRAME_MATERIAL;
        if (s->materials[i] && !jc_cloth_storage_disjoint(&s->materials[i]->storage,&m->storage))
            return JC_FRAME_MATERIAL;
        if (!s->materials[i] && slot==JC_CLOTH_FRAMES*JC_CLOTH_DRAWS) slot=i;
    }
    if (slot==JC_CLOTH_FRAMES*JC_CLOTH_DRAWS) return JC_FRAME_BUSY;
    s->materials[slot]=m; return JC_FRAME_OK;
}
static inline int jc_cloth_live_close_locked(struct jc_cloth_live *s) {
    if (!s || s->thread!=GetCurrentThreadId()) return JC_FRAME_ARGUMENT;
    if (s->closed) return JC_FRAME_OK;
    if (s->failed) return JC_FRAME_QUARANTINED;
    int result=jc_cloth_frames_drain(&s->frames);
    if (result) return result;
    if (s->failed) return JC_FRAME_QUARANTINED;
    /* Native geometry owners must still be alive here. BUSY is a request to
       defer outer chapter teardown, not permission to call DudeCleanup. */
    for (unsigned i=JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;i>0;--i) if (s->materials[i-1]) {
        if (jc_cloth_live_material_destroy(s->read,s->read_context,s->materials[i-1])) {
            s->failed=1; return JC_FRAME_QUARANTINED;
        }
        s->materials[i-1]=NULL;
    }
#ifdef _WIN32
    for (unsigned i=0;i<JC_CLOTH_FRAMES*(JC_CLOTH_PARTICIPANTS-1);++i) if (s->probes[i].peer) {
        if (jc_environment_probe_destroy(s->read,s->read_context,s->probes+i)) {
            s->failed=1;return JC_FRAME_QUARANTINED;
        }
    }
#endif
    for (unsigned i=0;i<JC_CLOTH_BUFFERS;++i) {
        struct jc_cloth_live_buffer *b=s->buffers+i;
        if (!b->object) continue;
        if (b->pins) { s->failed=1; return JC_FRAME_QUARANTINED; }
        if (b->mapped) s->device_context->lpVtbl->Unmap(s->device_context,(ID3D11Resource *)b->object,0);
        b->object->lpVtbl->Release(b->object); memset(b,0,sizeof(*b));
    }
    s->device_context->lpVtbl->Release(s->device_context); s->device_context=NULL;
    s->closed=1; return JC_FRAME_OK;
}
static inline int jc_cloth_live_close(struct jc_cloth_live *s) {
    if (!s || s->thread!=GetCurrentThreadId()) return JC_FRAME_ARGUMENT;
    if (s->closed) return JC_FRAME_OK;
    if (s->failed) return JC_FRAME_QUARANTINED;
    int code=jc_cloth_frames_drain(&s->frames);
    if (code) return code;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) return JC_FRAME_OWNERSHIP;
    code=jc_cloth_live_close_locked(s);
    return jc_cloth_live_graphics_leave(s,&token) ? code : JC_FRAME_QUARANTINED;
}
#endif
#endif
