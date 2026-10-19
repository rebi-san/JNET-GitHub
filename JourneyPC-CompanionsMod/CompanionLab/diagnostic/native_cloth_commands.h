#ifndef JC_NATIVE_CLOTH_COMMANDS_H
#define JC_NATIVE_CLOTH_COMMANDS_H
#include "native_cloth_frame.h"
#include "native_layout.h"
#if defined(__x86_64__) && !defined(_WIN32)
#define JC_COMMAND_ABI __attribute__((ms_abi))
#else
#define JC_COMMAND_ABI
#endif
typedef uint32_t (JC_COMMAND_ABI *jc_native_command_build)(uintptr_t,uintptr_t,uintptr_t,
    uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uint32_t);
#define JC_CLOTH_COMMANDS 512u
#define JC_CLOTH_STAGE_DRAWS (JC_CLOTH_DRAWS+JC_CLOTH_ROLES)
struct jc_cloth_command_source {
    uintptr_t material,passes,optional,batch,state,storage;
    uint32_t index,instances;
};
struct jc_cloth_command_binding {
    _Alignas(16) unsigned char state[0x38],vertex[0x48],bindings[1024];
    struct jc_cloth_index_view index;
    uint32_t offsets[32];
};
struct jc_cloth_command_stage {
    _Alignas(16) unsigned char renderer[0xa0],commands[JC_CLOTH_COMMANDS*128];
    struct jc_cloth_command_binding bindings[JC_CLOTH_STAGE_DRAWS];
    unsigned count,draws,failed;
    unsigned role_first[7],role_count[7];
};
struct jc_cloth_command_patch {
    unsigned first,count,replacement_first,replacement_count;
    uintptr_t material;
};
struct jc_cloth_ring_checkpoint {
    uint64_t object;
    uint32_t count,next;
    unsigned index;
};
static inline int jc_cloth_ring_capture(jc_layout_reader read,void *context,uint64_t object,
        unsigned index,struct jc_cloth_ring_checkpoint *out) {
    if (!read || !out || index>1) return 1;
    unsigned offset=index ? 0x10 : 0x18;
    uint32_t current,count; unsigned char state;
    if (!jc_layout_field(read,context,object,offset,&current,4) ||
        !jc_layout_field(read,context,object,offset+8,&count,4) ||
        !jc_layout_field(read,context,object,index ? 0xe : 0x1c,&state,1)) return 1;
    count&=0x7fffffff;
    if (!count || count>4096 || current>=count || state!=2) return 1;
    *out=(struct jc_cloth_ring_checkpoint){object,count,current+1==count ? 0 : current+1,index};
    return 0;
}
static inline int jc_cloth_ring_finished(jc_layout_reader read,void *context,
        const struct jc_cloth_ring_checkpoint *s) {
    if (!read || !s || !s->object || !s->count || s->index>1) return 1;
    unsigned offset=s->index ? 0x10 : 0x18;
    uint32_t current,count; unsigned char state;
    return !jc_layout_field(read,context,s->object,offset,&current,4) ||
        !jc_layout_field(read,context,s->object,offset+8,&count,4) ||
        !jc_layout_field(read,context,s->object,s->index ? 0xe : 0x1c,&state,1) ||
        (count&0x7fffffff)!=s->count || current!=s->next || state!=0;
}
static inline int jc_cloth_command_splice(const unsigned char *original,unsigned count,
        const struct jc_cloth_command_stage *stage,const struct jc_cloth_command_patch *patches,
        unsigned patch_count,unsigned char *output,unsigned capacity,unsigned *written) {
    if (!original || !stage || stage->failed || !patches || !patch_count || patch_count>7 ||
        !output || !written || !count || count>65536 || stage->count>JC_CLOTH_COMMANDS ||
        !jc_cloth_span((uintptr_t)original,(uint64_t)count*128) ||
        !jc_cloth_span((uintptr_t)stage,sizeof(*stage)))
        return 1;
    unsigned previous=0,total=count;
    for (unsigned p=0;p<patch_count;++p) {
        const struct jc_cloth_command_patch *x=patches+p;
        if (!x->count || x->first<previous || x->first>count || x->count>count-x->first ||
            x->replacement_first>stage->count || x->replacement_count>stage->count-x->replacement_first ||
            !x->material) return 1;
        for (unsigned i=0;i<x->count;++i) {
            const unsigned char *c=original+(x->first+i)*128u;
            uintptr_t material; memcpy(&material,c+0x20,8);
            uint32_t kind=jc_cloth_u32(c);
            if ((kind!=1 && kind!=2) || material!=x->material) return 1;
        }
        previous=x->first+x->count;
        total=total-x->count+x->replacement_count;
    }
    if (!total || total>capacity || !jc_cloth_span((uintptr_t)output,(uint64_t)total*128) ||
        jc_cloth_overlap((uintptr_t)output,(uint64_t)total*128,(uintptr_t)original,(uint64_t)count*128) ||
        jc_cloth_overlap((uintptr_t)output,(uint64_t)total*128,(uintptr_t)stage,sizeof(*stage))) return 1;
    unsigned src=0,dst=0;
    for (unsigned p=0;p<patch_count;++p) {
        const struct jc_cloth_command_patch *x=patches+p;
        unsigned before=x->first-src;
        memcpy(output+dst*128u,original+src*128u,before*128u); dst+=before;
        memcpy(output+dst*128u,stage->commands+x->replacement_first*128u,x->replacement_count*128u);
        dst+=x->replacement_count; src=x->first+x->count;
    }
    memcpy(output+dst*128u,original+src*128u,(count-src)*128u);
    *written=total; return 0;
}
static inline void jc_command_ptr(void *p,uintptr_t v) { memcpy(p,&v,8); }
static inline int jc_cloth_command_stage_init(struct jc_cloth_command_stage *s) {
    if (!s) return 1;
    memset(s,0,sizeof(*s));
    jc_command_ptr(s->renderer+0x58,(uintptr_t)s->commands);
    jc_command_ptr(s->renderer+0x60,(uintptr_t)s->commands+sizeof(s->commands));
    jc_command_ptr(s->renderer+0x68,(uintptr_t)s->commands);
    return 0;
}
/* Source state/pass dependencies must be frozen by the frame owner. Only the
   non-owning vertex/index views and stream bindings are copied here. */
static inline int jc_cloth_command_binding_make(struct jc_cloth_command_binding *b,
        jc_layout_reader read,void *ctx,const struct jc_cloth_command_source *source,
        const struct jc_cloth_stream *stream,uint32_t first,uint32_t count,
        unsigned *pass_count,uintptr_t *pass_table) {
    if (!b || !read || !source || !stream || !pass_count || !pass_table ||
        !source->material || !source->passes || !source->batch || !source->state ||
        source->index>UINT16_MAX) return 1;
    uint32_t passes; uintptr_t table;
    if (!read(ctx,source->passes+8,&passes,4) || !read(ctx,source->passes+0x10,&table,8)) return 1;
    passes&=0x7fffffff;
    if (!passes || passes>8 || !table || table>UINTPTR_MAX-(size_t)passes*0x2c0) return 1;
    memset(b,0,sizeof(*b));
    if (jc_cloth_index_view_make(stream->index,stream->index_bytes,stream->index_offset,
            stream->indices,first,count,&b->index) || !stream->vertex ||
        (uint64_t)stream->vertex_offset+(uint64_t)stream->vertices*60>stream->vertex_bytes ||
        !read(ctx,source->state,b->state,sizeof(b->state))) return 1;
    uintptr_t original_bindings; memcpy(&original_bindings,b->state+0x18,8);
    if (!original_bindings) return 1;
    uint16_t used[32]; unsigned used_count=0;
    for (unsigned p=0;p<passes;++p) {
        uint32_t n; uintptr_t offsets;
        if (!read(ctx,table+p*0x2c0+0x20,&n,4) || !read(ctx,table+p*0x2c0+0x28,&offsets,8)) return 1;
        n&=0x7fffffff;
        if (!n || n>32 || !offsets) return 1;
        for (unsigned j=0;j<n;++j) {
            uint16_t offset;
            if (!read(ctx,offsets+j*2,&offset,2) || offset%8 || offset>sizeof(b->bindings)-24) return 1;
            unsigned k=0; while (k<used_count && used[k]!=offset) ++k;
            if (k<used_count) continue;
            if (used_count==32) return 1;
            for (unsigned q=0;q<used_count;++q)
                if (offset<used[q]+24u && used[q]<offset+24u) return 1;
            uintptr_t original[2]; uint32_t stride,attribute;
            if (!read(ctx,original_bindings+offset,original,16) || !original[0] || !original[1] ||
                !read(ctx,original[0],&stride,4) || stride!=60 ||
                !read(ctx,original[1],&attribute,4) || attribute>=60) return 1;
            used[used_count]=offset; b->offsets[used_count]=attribute;
            jc_command_ptr(b->bindings+offset,(uintptr_t)b->vertex);
            jc_command_ptr(b->bindings+offset+8,(uintptr_t)&b->offsets[used_count++]);
        }
    }
    uint32_t stride=60,one=1;
    memcpy(b->vertex,&stride,4); memcpy(b->vertex+0x20,&one,4);
    memcpy(b->vertex+0x28,&stream->vertex,8); memcpy(b->vertex+0x40,&stream->vertex_offset,4);
    b->vertex[0x44]=1;
    jc_command_ptr(b->state+0x18,(uintptr_t)b->bindings);
    jc_command_ptr(b->state+0x30,(uintptr_t)&b->index);
    *pass_count=passes; *pass_table=table; return 0;
}
static inline int jc_cloth_command_stage_add(struct jc_cloth_command_stage *s,
        jc_layout_reader read,void *ctx,jc_native_command_build build,
        const struct jc_cloth_command_source *source,const struct jc_cloth_stream *stream,
        uint32_t first,uint32_t count) {
    if (!s || s->failed || !build || s->draws>=JC_CLOTH_STAGE_DRAWS) return 1;
    struct jc_cloth_command_binding *b=s->bindings+s->draws;
    unsigned passes; uintptr_t table;
    if (jc_cloth_command_binding_make(b,read,ctx,source,stream,first,count,&passes,&table) ||
        passes>JC_CLOTH_COMMANDS-s->count) return 1;
    uintptr_t begin=(uintptr_t)s->commands+s->count*128u;
    /* 34e990 touches only renderer+58's arena and +90/+98. Its allocator
       3e1b50 wraps, so capacity is checked BEFORE calling the native builder. */
    uint32_t result=build((uintptr_t)s->renderer,source->material,source->passes,source->optional,
        source->batch,source->index,(uintptr_t)b->state,source->storage,source->instances);
    uintptr_t end,cursor,start;
    memcpy(&end,s->renderer+0x98,8); memcpy(&cursor,s->renderer+0x68,8); memcpy(&start,s->renderer+0x90,8);
    if (result || start!=(uintptr_t)s->commands || end!=begin+passes*128u || cursor!=end) {
        s->failed=1; return 1;
    }
    for (unsigned p=0;p<passes;++p) {
        unsigned char *command=(unsigned char *)begin+p*128u;
        uintptr_t indices,bindings,pass; uint32_t actual; uint16_t ring;
        memcpy(&indices,command+0x68,8); memcpy(&bindings,command+0x58,8);
        memcpy(&pass,command+0x28,8); memcpy(&actual,command+0x70,4); memcpy(&ring,command+0x3a,2);
        if (indices!=(uintptr_t)&b->index || bindings!=(uintptr_t)b->bindings ||
            pass!=table+p*0x2c0 || actual!=count || ring) { s->failed=1; return 1; }
    }
    s->count+=passes; ++s->draws; return 0;
}
#endif
