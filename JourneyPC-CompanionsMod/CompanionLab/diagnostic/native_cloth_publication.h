#ifndef JC_NATIVE_CLOTH_PUBLICATION_H
#define JC_NATIVE_CLOTH_PUBLICATION_H
#include "native_cloth_commands.h"

struct jc_cloth_publication_view {
    struct jc_cloth_command_binding binding;
    struct jc_cloth_command_source source;
    uintptr_t table;
    unsigned passes,indices;
};
static inline int jc_cloth_publication_view_prepare(struct jc_cloth_publication_view *v,
        jc_layout_reader read,void *ctx,const struct jc_cloth_command_source *source,
        const struct jc_cloth_stream *stream,unsigned first,unsigned count) {
    if (!v || jc_cloth_command_binding_make(&v->binding,read,ctx,source,stream,first,count,
            &v->passes,&v->table)) return 1;
    v->source=*source; v->source.state=(uintptr_t)v->binding.state; v->indices=count;
    return 0;
}
static inline int jc_cloth_publication_view_check(const struct jc_cloth_publication_view *v,
        jc_layout_reader read,void *ctx,uintptr_t begin,uintptr_t end) {
    if (!v || !read || !begin || end<begin || !v->passes || v->passes>8 ||
        end-begin!=v->passes*128u) return 1;
    for (unsigned n=0;n<v->passes;++n) {
        unsigned char command[128]; uintptr_t index,bindings,shader,pass;
        uint16_t ring;
        if (!read(ctx,begin+n*128u,command,sizeof(command))) return 1;
        memcpy(&index,command+0x68,8); memcpy(&bindings,command+0x58,8);
        memcpy(&shader,command+0x20,8); memcpy(&pass,command+0x28,8);
        memcpy(&ring,command+0x3a,2);
        if (index!=(uintptr_t)&v->binding.index || bindings!=(uintptr_t)v->binding.bindings ||
            shader!=v->source.material || pass!=v->table+n*0x2c0u || ring ||
            jc_cloth_u32(command+0x70)!=v->indices ||
            jc_cloth_u32(command)!=(v->source.instances ? 2u : 1u)) return 1;
    }
    return 0;
}

/* Literal queue operation 8a949..8a99c, with admission checks before mutation.
   Caller owns this render-queue phase and the model's native reference. Return
   2 means mutation began: retain resources and quarantine instead of retrying. */
static inline int jc_cloth_publication_append(jc_layout_reader read,void *ctx,
        uintptr_t image,uintptr_t render_context,uintptr_t model,uintptr_t control) {
    uint64_t queue,base,entry,pair[2],again[2],fresh_queue; uint32_t count,fresh;
    int32_t refs,old_refs;
    if (!read || !image || !model || !control ||
        !jc_layout_field(read,ctx,render_context,0x18,&queue,8) || !queue ||
        !jc_layout_address(queue,0x80,&base) ||
        !jc_layout_field(read,ctx,base,0x10000,&count,4) || count>=4096 ||
        !jc_layout_address(base,(uint64_t)count*16,&entry) ||
        !read(ctx,entry,pair,sizeof(pair)) ||
        !read(ctx,control,&refs,4) || refs<1 || refs==INT32_MAX ||
        (pair[0]==model && pair[1]!=control) ||
        (pair[0]!=model && pair[1]==control) ||
        (pair[1] && (!read(ctx,pair[1],&old_refs,4) || old_refs<1)) ||
        !jc_layout_field(read,ctx,render_context,0x18,&fresh_queue,8) || fresh_queue!=queue ||
        !jc_layout_field(read,ctx,base,0x10000,&fresh,4) || fresh!=count ||
        !read(ctx,entry,again,sizeof(again)) || memcmp(pair,again,sizeof(pair))) return 1;
    if (pair[0]!=model) {
        ((void (JC_COMMAND_ABI *)(uintptr_t))(image+0x91820))(entry);
        uintptr_t owned[2]={model,control};
        memcpy((void *)entry,owned,sizeof(owned));
        __atomic_fetch_add((int32_t *)control,1,__ATOMIC_SEQ_CST);
    }
    ++count; memcpy((void *)(base+0x10000),&count,4);
    if (!read(ctx,entry,again,sizeof(again)) || again[0]!=model || again[1]!=control ||
        !jc_layout_field(read,ctx,base,0x10000,&fresh,4) || fresh!=count) return 2;
    return 0;
}

/* Observe 34e990 on its owning builder thread. The frame retains this object
   until GPU retirement; copied state/optional pointers refer into it. */
struct jc_cloth_publication_record {
    struct jc_cloth_command_source source;
    uintptr_t renderer,cursor,limit,begin,end,table;
    unsigned count,prepared,complete;
    unsigned char state[0x38],optional[0x10],commands[8*128];
};
struct jc_cloth_publication {
    uint64_t generation;
    uintptr_t materials[7],renderer;
    uintptr_t items[7],bindings[7];
    unsigned roles,seen,failed;
    struct jc_cloth_publication_record records[7];
};
/* 2696b0 creates the subscriber's model. 2d6370 assigns item+20 to
   the owner's active constant collection. 2d13b0 forwards that exact item
   and collection to 34ee80. A shared shader resource is not actor identity. */
static inline int jc_cloth_publication_model(jc_layout_reader read,void *ctx,
        uintptr_t model,uintptr_t *item,uintptr_t *bindings,uintptr_t *shader) {
    struct {
        uintptr_t span[2],object,geometry,batches,collection,table,binding,resource;
        uint32_t batch_count,count,selector;
    } snapshots[2];
    if (!read || !item || !bindings || !shader) return 1;
    memset(snapshots,0,sizeof(snapshots));
    for (unsigned n=0;n<2;++n) {
        uintptr_t slot;
        if (!jc_layout_field(read,ctx,model,0x60,snapshots[n].span,16) ||
            !snapshots[n].span[0] || snapshots[n].span[1]<snapshots[n].span[0] ||
            snapshots[n].span[1]-snapshots[n].span[0]!=8 ||
            !read(ctx,snapshots[n].span[0],&snapshots[n].object,8) || !snapshots[n].object ||
            !read(ctx,snapshots[n].object,&snapshots[n].geometry,8) || !snapshots[n].geometry ||
            !read(ctx,snapshots[n].geometry,&snapshots[n].batch_count,4) ||
            (snapshots[n].batch_count&0x7fffffff)!=1 ||
            !jc_layout_field(read,ctx,snapshots[n].geometry,8,&snapshots[n].batches,8) || !snapshots[n].batches ||
            !read(ctx,snapshots[n].batches,&snapshots[n].selector,4) ||
            !jc_layout_field(read,ctx,snapshots[n].object,0x20,&snapshots[n].collection,8) || !snapshots[n].collection ||
            !read(ctx,snapshots[n].collection,&snapshots[n].count,4)) return 1;
        uint32_t count=snapshots[n].count&0x7fffffff;
        if (!count || count>16 || snapshots[n].selector>=count) return 1;
        if (count==1) {
            if (snapshots[n].collection>UINTPTR_MAX-8) return 1;
            slot=snapshots[n].collection+8;
        } else {
            if (!jc_layout_field(read,ctx,snapshots[n].collection,8,&snapshots[n].table,8) ||
                !snapshots[n].table || snapshots[n].table>UINTPTR_MAX-8*snapshots[n].selector) return 1;
            slot=snapshots[n].table+8*snapshots[n].selector;
        }
        if (!read(ctx,slot,&snapshots[n].binding,8) || !snapshots[n].binding ||
            !read(ctx,snapshots[n].binding,&snapshots[n].resource,8) || !snapshots[n].resource) return 1;
    }
    if (memcmp(snapshots,snapshots+1,sizeof(snapshots[0]))) return 1;
    *item=snapshots[0].object; *bindings=snapshots[0].collection; *shader=snapshots[0].resource;
    return 0;
}
static inline int jc_cloth_publication_init_models(struct jc_cloth_publication *p,
        jc_layout_reader read,void *ctx,uint64_t generation,const uintptr_t models[7],unsigned roles) {
    if (!p || !read || !generation || !models || !roles || (roles&~127u)) return 1;
    memset(p,0,sizeof(*p)); p->generation=generation; p->roles=roles;
    for (unsigned r=0;r<7;++r) if (roles&(1u<<r)) {
        if (jc_cloth_publication_model(read,ctx,models[r],p->items+r,p->bindings+r,p->materials+r)) goto fail;
        for (unsigned q=0;q<r;++q) if ((roles&(1u<<q)) &&
            (p->items[r]==p->items[q] || p->bindings[r]==p->bindings[q])) goto fail;
    }
    return 0;
fail:
    p->failed=1; return 1;
}
static inline int jc_cloth_publication_dispatch_role(const struct jc_cloth_publication *p,
        uintptr_t item,uintptr_t bindings,unsigned *role) {
    if (!p || !role || p->failed || !item || !bindings) return -1;
    for (unsigned r=0;r<7;++r) if ((p->roles&(1u<<r)) && p->items[r]==item) {
        if (p->bindings[r]!=bindings) return -1;
        *role=r; return 1;
    }
    return 0;
}
static inline int jc_cloth_publication_init(struct jc_cloth_publication *p,
        uint64_t generation,const uintptr_t materials[7],unsigned roles) {
    if (!p || !generation || !materials || !roles || (roles&~127u)) return 1;
    for (unsigned r=0;r<7;++r) if (roles&(1u<<r)) {
        if (!materials[r]) return 1;
        for (unsigned q=0;q<r;++q)
            if ((roles&(1u<<q)) && materials[q]==materials[r]) return 1;
    }
    memset(p,0,sizeof(*p)); p->generation=generation; p->roles=roles;
    memcpy(p->materials,materials,sizeof(p->materials)); return 0;
}
/* 0 unrelated, 1 captured; -1 invalid owned command. Do not suppress the
   stock builder when observing a failure before output has been rerouted. */
static inline int jc_cloth_publication_before_role(struct jc_cloth_publication *p,
        jc_layout_reader read,void *ctx,uintptr_t renderer,
        const struct jc_cloth_command_source *source,unsigned r) {
    if (!p || !read || !source || !p->generation || r>=7 || !(p->roles&(1u<<r))) return -1;
    struct jc_cloth_publication_record *v=p->records+r;
    uint32_t count=0; uintptr_t arena=0;
    if (p->failed || v->prepared || p->materials[r]!=source->material || !renderer || !source->passes || !source->state ||
        !source->batch || source->index>UINT16_MAX ||
        (p->renderer && p->renderer!=renderer) ||
        !jc_layout_field(read,ctx,renderer,0x58,&arena,8) ||
        !jc_layout_field(read,ctx,renderer,0x60,&v->limit,8) ||
        !jc_layout_field(read,ctx,renderer,0x68,&v->cursor,8) ||
        !jc_layout_field(read,ctx,source->passes,8,&count,4) ||
        !jc_layout_field(read,ctx,source->passes,0x10,&v->table,8) ||
        !read(ctx,source->state,v->state,sizeof(v->state))) goto fail;
    count&=0x7fffffff;
    if (!count || count>8 || !v->table ||
        !jc_cloth_span(v->table,(uint64_t)count*0x2c0) ||
        !arena || v->cursor<arena || v->limit<v->cursor ||
        (v->cursor&7) || v->limit-v->cursor<(uint64_t)count*128) goto fail;
    if (source->optional && !read(ctx,source->optional,v->optional,sizeof(v->optional))) goto fail;
    v->source=*source; v->renderer=renderer; v->count=count; v->prepared=1;
    p->renderer=renderer; return 1;
fail:
    p->failed=1; return -1;
}
static inline int jc_cloth_publication_before(struct jc_cloth_publication *p,
        jc_layout_reader read,void *ctx,uintptr_t renderer,
        const struct jc_cloth_command_source *source,unsigned *role) {
    if (!p || !source || !role) return -1;
    unsigned found=7;
    for (unsigned r=0;r<7;++r) if ((p->roles&(1u<<r)) && p->materials[r]==source->material) {
        if (found!=7) return -1;
        found=r;
    }
    if (found==7) return 0;
    *role=found; return jc_cloth_publication_before_role(p,read,ctx,renderer,source,found);
}
static inline int jc_cloth_publication_after(struct jc_cloth_publication *p,
        jc_layout_reader read,void *ctx,unsigned role,uint32_t result) {
    if (!p || !read || role>=7) return 1;
    struct jc_cloth_publication_record *v=p->records+role;
    uintptr_t cursor=0,end=0,table=0; uint32_t count=0;
    if (p->failed || result || !v->prepared || v->complete ||
        !jc_layout_field(read,ctx,v->renderer,0x68,&cursor,8) ||
        !jc_layout_field(read,ctx,v->renderer,0x98,&end,8) ||
        cursor!=v->cursor+(uint64_t)v->count*128 || end!=cursor || cursor>v->limit ||
        !jc_layout_field(read,ctx,v->source.passes,8,&count,4) ||
        !jc_layout_field(read,ctx,v->source.passes,0x10,&table,8) ||
        (count&0x7fffffff)!=v->count || table!=v->table ||
        !read(ctx,v->cursor,v->commands,v->count*128u)) goto fail;
    for (unsigned n=0;n<v->count;++n) {
        const unsigned char *c=v->commands+n*128u;
        uintptr_t material,pass,batch,storage; uint16_t index;
        memcpy(&material,c+0x20,8); memcpy(&pass,c+0x28,8);
        memcpy(&batch,c+0x30,8); memcpy(&index,c+0x38,2);
        memcpy(&storage,c+0x60,8);
        if (jc_cloth_u32(c)!=(v->source.instances ? 2u : 1u) ||
            material!=v->source.material || pass!=v->table+n*0x2c0u ||
            batch!=v->source.batch || index!=v->source.index || storage!=v->source.storage ||
            memcmp(c+0x40,v->state,0x20) ||
            memcmp(c+0x10,v->state+0x28,8) || memcmp(c+0x18,v->state+0x20,8) ||
            memcmp(c+8,v->optional+8,8) ||
            (v->source.instances && jc_cloth_u32(c+0x78)!=v->source.instances)) goto fail;
    }
    v->begin=v->cursor; v->end=end; v->complete=1;
    v->source.state=(uintptr_t)v->state;
    v->source.optional=(uintptr_t)v->optional;
    p->seen|=1u<<role; return 0;
fail:
    p->failed=1; return 1;
}
/* Resolve against the actual 378810 batch, never a guessed renderer arena.
   The original batch owns all borrowed shader/pass/storage dependencies. */
static inline int jc_cloth_publication_resolve(struct jc_cloth_publication *p,
        jc_layout_reader read,void *ctx,uintptr_t begin,uintptr_t end,
        struct jc_cloth_command_source stock[7],unsigned first[7],unsigned counts[7]) {
    if (!p || !read || !stock || !first || !counts || p->failed || !p->generation ||
        p->seen!=p->roles || !begin || end<=begin || (begin&7) ||
        (end-begin)%128 || end-begin>65536u*128u) return 1;
    unsigned char current[8*128];
    for (unsigned r=0;r<7;++r) if (p->roles&(1u<<r)) {
        struct jc_cloth_publication_record *v=p->records+r;
        if (!v->complete || v->begin<begin || v->end>end || (v->begin-begin)%128 ||
            !read(ctx,v->begin,current,v->count*128u) ||
            memcmp(current,v->commands,v->count*128u)) return 1;
        for (unsigned q=0;q<r;++q) if ((p->roles&(1u<<q)) &&
            v->begin<p->records[q].end && p->records[q].begin<v->end) return 1;
    }
    memset(stock,0,7*sizeof(*stock)); memset(first,0,7*sizeof(*first)); memset(counts,0,7*sizeof(*counts));
    for (unsigned r=0;r<7;++r) if (p->roles&(1u<<r)) {
        stock[r]=p->records[r].source;
        first[r]=(unsigned)((p->records[r].begin-begin)/128);
        counts[r]=p->records[r].count;
    }
    return 0;
}
#endif
