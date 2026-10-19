#ifndef JC_NATIVE_CLOTH_MATERIAL_LIVE_H
#define JC_NATIVE_CLOTH_MATERIAL_LIVE_H
#include "native_material_ready.h"

#define JC_CLOTH_SUBSCRIBER_BYTES 0x538u
#define JC_CLOTH_SUBSCRIBERS 64u
struct jc_cloth_graphics_ops {
    void *context;
    uint64_t (*mutex)(void *);
    uint32_t (*thread)(void *);
    void (*enter)(void *,uint64_t);
    void (*leave)(void *,uint64_t);
};
struct jc_cloth_graphics_token {
    uint64_t mutex,device_context;
    uint32_t thread,prior_depth;
    unsigned held;
};
struct jc_cloth_atlas_copy_ops {
    void *context;
    int (*hold)(void *,const uint64_t[2]);
    void (*drop)(void *,const uint64_t[2]);
    int (*clone)(void *,const uint64_t[4],uint64_t,uint64_t[4]);
    void (*release)(void *,uint64_t);
};
struct jc_cloth_atlas_snapshot {
    uint64_t texture[0x120/8],handle[0x98/8];
    uint64_t expected_texture[0x120/8],expected_handle[0x98/8];
    uint64_t objects[4],source,entry;
    unsigned pins,ready;
};
static inline int jc_cloth_atlas_sampler_prepare(unsigned char *texture,unsigned role,unsigned index) {
    if(index>=3 || (role!=4 && role!=5))return 0;
    texture[0xa]=2;
    if(role==5)texture[0xb]=2;
    uint32_t flags;memcpy(&flags,texture+0x24,4);flags&=~0x80u;
    memcpy(texture+0x24,&flags,4);
    return 1;
}
static inline int jc_cloth_atlas_snapshot_check(const struct jc_cloth_atlas_snapshot *s) {
    return !s || !s->ready || !s->objects[0] || !s->objects[1] || !s->objects[3] ||
        s->texture[0x70/8]!=(uintptr_t)s->handle ||
        s->texture[0x28/8]!=s->objects[3] || memcmp((const unsigned char *)s->handle+0x38,s->objects,24) ||
        memcmp(s->texture,s->expected_texture,sizeof(s->texture)) ||
        memcmp(s->handle,s->expected_handle,sizeof(s->handle));
}
static inline int jc_cloth_atlas_snapshot_pin(struct jc_cloth_atlas_snapshot *s,uint64_t texture) {
    if (jc_cloth_atlas_snapshot_check(s) || texture!=(uintptr_t)s->texture || s->pins==UINT32_MAX) return 0;
    ++s->pins;return 1;
}
static inline int jc_cloth_atlas_snapshot_unpin(struct jc_cloth_atlas_snapshot *s,uint64_t texture) {
    if (!s || !s->ready || !s->pins || texture!=(uintptr_t)s->texture) return 0;
    --s->pins;return 1;
}
static inline int jc_cloth_atlas_snapshot_destroy(const struct jc_cloth_atlas_copy_ops *ops,
        struct jc_cloth_atlas_snapshot *s) {
    if (!s || s->pins || !ops || !ops->release) return 1;
    if (!s->ready) return 0;
    if (jc_cloth_atlas_snapshot_check(s)) return 2;
    for (unsigned i=4;i;--i) if (s->objects[i-1]) ops->release(ops->context,s->objects[i-1]);
    memset(s,0,sizeof(*s));return 0;
}
static inline int jc_cloth_atlas_snapshot_create(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t entry,uint64_t source,const struct jc_cloth_atlas_copy_ops *ops,
        struct jc_cloth_atlas_snapshot *out) {
    uint64_t payload,pair[2],again_pair[2],again_payload,handle,vt,payload_type;
    unsigned char texture[0x120],gpu[0x98],fresh_texture[0x120],fresh_gpu[0x98];
    uint64_t original[4],owned[4]={0};uint32_t kind,count,index,flags;int held=0,result=2;
    if (!out || out->ready || out->pins || !ops || !ops->hold || !ops->drop || !ops->clone || !ops->release ||
        !source || !jc_layout_field(read,ctx,entry,0x38,&payload,8) ||
        !jc_layout_field(read,ctx,payload,0,&payload_type,8) || payload_type!=image+0x680b18 ||
        !jc_layout_field(read,ctx,payload,0xc8,pair,sizeof(pair)) || pair[0]!=source || !pair[1] || (pair[1]&3) ||
        !jc_layout_field(read,ctx,source,0,&vt,8) || vt!=image+0x6894c0) return 1;
    if (!ops->hold(ops->context,pair)) return 2;
    held=1;
    if (!jc_layout_field(read,ctx,source,0,texture,sizeof(texture))) goto done;
    memcpy(&vt,texture,8);memcpy(&handle,texture+0x70,8);memcpy(&kind,texture+0x110,4);
    if (vt!=image+0x6894c0 || kind!=1 || texture[0x114] ||
        memcmp(texture+0x78,(unsigned char[16]){0},16) ||
        !jc_layout_field(read,ctx,handle,0,gpu,sizeof(gpu))) goto done;
    memcpy(&count,gpu+0x30,4);memcpy(&index,gpu+0xc,4);memcpy(&flags,gpu+0x94,4);
    memcpy(original,gpu+0x38,24);memcpy(original+3,texture+0x28,8);
    if ((count&0x7fffffff)!=1 || index || !(flags&1) || !original[0] || !original[1] ||
        ((flags&0x80000000u) && !original[2])) goto done;
    memset(out,0,sizeof(*out));
    unsigned char *dst=(unsigned char *)out->texture,*dst_gpu=(unsigned char *)out->handle;
    memcpy(dst,&vt,8);memcpy(dst+8,texture+8,0x20);
    uint64_t private_handle=(uintptr_t)out->handle;memcpy(dst+0x70,&private_handle,8);memcpy(dst+0x110,&kind,4);
    memcpy(dst_gpu,gpu,0x20);memcpy(dst_gpu+0x28,gpu+0x28,8);
    count=1;memcpy(dst_gpu+0x30,&count,4);memcpy(dst_gpu+0x94,&flags,4);
    if (!ops->clone(ops->context,original,(uintptr_t)out->texture,owned) ||
        !owned[0] || !owned[1] || !owned[3] || (original[2] && !owned[2])) { result=3;goto done; }
    for (unsigned i=0;i<3;++i) if (owned[i])
        for (unsigned j=0;j<4;++j) if (owned[i]==original[j]) goto done;
    if (!jc_layout_field(read,ctx,source,0,fresh_texture,sizeof(fresh_texture)) ||
        memcmp(texture,fresh_texture,sizeof(texture)) ||
        !jc_layout_field(read,ctx,handle,0,fresh_gpu,sizeof(fresh_gpu)) || memcmp(gpu,fresh_gpu,sizeof(gpu)) ||
        !jc_layout_field(read,ctx,entry,0x38,&again_payload,8) || again_payload!=payload ||
        !jc_layout_field(read,ctx,payload,0xc8,again_pair,sizeof(again_pair)) || memcmp(pair,again_pair,sizeof(pair))) goto done;
    memcpy(dst_gpu+0x38,owned,24);memcpy(dst+0x28,owned+3,8);
    uint32_t sampler_flags;memcpy(&sampler_flags,dst+0x24,4);sampler_flags|=0x80;
    memcpy(dst+0x24,&sampler_flags,4);
    memcpy(out->objects,owned,sizeof(owned));memcpy(out->expected_texture,out->texture,sizeof(out->texture));
    memcpy(out->expected_handle,out->handle,sizeof(out->handle));
    out->source=source;out->entry=entry;out->ready=1;result=0;
done:
    if (result) { for (unsigned i=4;i;--i) if (owned[i-1]) ops->release(ops->context,owned[i-1]);memset(out,0,sizeof(*out)); }
    if (held) ops->drop(ops->context,pair);
    return result;
}
static inline int jc_cloth_graphics_lock(jc_layout_reader read,void *ctx,uint64_t image,
        const struct jc_cloth_graphics_ops *ops,uint64_t expected_context,
        struct jc_cloth_graphics_token *token) {
    struct jc_cloth_graphics_token t={0};uint32_t before[2],after[2];
    if (!token || token->held || !ops || !ops->mutex || !ops->thread || !ops->enter || !ops->leave ||
        !(t.thread=ops->thread(ops->context)) || !(t.mutex=ops->mutex(ops->context)) ||
        t.mutex!=image+0x40407e0 || !jc_layout_field(read,ctx,t.mutex,0x20,before,sizeof(before))) return 1;
    t.prior_depth=before[0]==t.thread ? before[1] : 0;
    if ((before[0]==t.thread && !t.prior_depth) || t.prior_depth>=0x7ffffffeu) return 2;
    ops->enter(ops->context,t.mutex);
    if (!jc_layout_field(read,ctx,t.mutex,0x20,after,sizeof(after)) || after[0]!=t.thread ||
        after[1]!=t.prior_depth+1 ||
        !jc_layout_field(read,ctx,image,0x40408b8,&t.device_context,8) || !t.device_context ||
        (expected_context && expected_context!=t.device_context)) {
        ops->leave(ops->context,t.mutex);return 3;
    }
    t.held=1;*token=t;return 0;
}
static inline int jc_cloth_graphics_unlock(jc_layout_reader read,void *ctx,uint64_t image,
        const struct jc_cloth_graphics_ops *ops,struct jc_cloth_graphics_token *token) {
    uint32_t state[2];uint64_t current;int result=0;
    if (!token || !token->held || !ops || !ops->thread || !ops->leave ||
        token->mutex!=image+0x40407e0 || ops->thread(ops->context)!=token->thread) return 1;
    int state_read=jc_layout_field(read,ctx,token->mutex,0x20,state,sizeof(state));
    if (state_read && state[0]==token->thread && !state[1]) return 2;
    if (!state_read ||
        state[0]!=token->thread || state[1]!=token->prior_depth+1 ||
        !jc_layout_field(read,ctx,image,0x40408b8,&current,8) || current!=token->device_context) result=2;
    ops->leave(ops->context,token->mutex);token->held=0;
    if (token->prior_depth && (!jc_layout_field(read,ctx,token->mutex,0x20,state,sizeof(state)) ||
        state[0]!=token->thread || state[1]!=token->prior_depth)) result=2;
    return result;
}
struct jc_cloth_subscribers { uint64_t geometry; unsigned count; uint64_t nodes[JC_CLOTH_SUBSCRIBERS]; };
struct jc_cloth_storage {
    uint64_t payload;
    unsigned count;
    struct { uint64_t begin; uint32_t bytes; } spans[32];
};
/* Atlas list nodes begin at holder+8, not at the holder vtable. */
struct jc_cloth_atlas_list { unsigned count; uint64_t nodes[256]; };
static inline int jc_cloth_atlas_list_read(jc_layout_reader read,void *ctx,uint64_t entry,
        struct jc_cloth_atlas_list *out) {
    struct jc_cloth_atlas_list list={0}; uint64_t ends[2],again[2],node,previous=0;
    if (!out || !jc_layout_field(read,ctx,entry,0x28,ends,sizeof(ends))) return 1;
    node=ends[0];
    while (node) {
        uint64_t links[3];
        if (list.count==256 || !jc_layout_field(read,ctx,node,0,links,sizeof(links)) ||
            links[0]!=entry+0x28 || links[2]!=previous) return 1;
        for (unsigned i=0;i<list.count;++i) if (list.nodes[i]==node) return 1;
        list.nodes[list.count++]=node; previous=node; node=links[1];
    }
    if (previous!=ends[1] || !jc_layout_field(read,ctx,entry,0x28,again,sizeof(again)) ||
        memcmp(ends,again,sizeof(ends))) return 1;
    *out=list; return 0;
}
static inline int jc_cloth_atlas_list_changed(const struct jc_cloth_atlas_list *before,
        const struct jc_cloth_atlas_list *after,uint64_t node,int added) {
    if (!before || !after || !node || before->count>256 || after->count>256) return 1;
    if (added) {
        if (before->count==256 || after->count!=before->count+1 ||
            after->nodes[before->count]!=node ||
            memcmp(before->nodes,after->nodes,before->count*8)) return 1;
        for (unsigned i=0;i<before->count;++i) if (before->nodes[i]==node) return 1;
    } else {
        unsigned found=0,index=0;
        if (!before->count || after->count+1!=before->count) return 1;
        for (unsigned i=0;i<before->count;++i) {
            if (before->nodes[i]==node) ++found;
            else if (index>=after->count || before->nodes[i]!=after->nodes[index++]) return 1;
        }
        if (found!=1 || index!=after->count) return 1;
    }
    return 0;
}
/* Native list at geometry+8; subscribers have owner/next/previous at 0/8/10. */
static inline int jc_cloth_subscribers_read(jc_layout_reader read,void *ctx,uint64_t geometry,
        struct jc_cloth_subscribers *out) {
    struct jc_cloth_subscribers value={.geometry=geometry}; uint64_t head[3],again[3],node,previous=0;
    if (!jc_layout_field(read,ctx,geometry,8,head,sizeof(head)) || head[2]!=geometry) return 1;
    node=head[0];
    while (node) {
        uint64_t links[3];
        if (value.count==JC_CLOTH_SUBSCRIBERS || !jc_layout_field(read,ctx,node,0,links,sizeof(links)) ||
            links[0]!=geometry+8 || links[2]!=previous) return 1;
        for (unsigned i=0;i<value.count;++i) if (value.nodes[i]==node) return 1;
        value.nodes[value.count++]=node; previous=node; node=links[1];
    }
    if (previous!=head[1] || !jc_layout_field(read,ctx,geometry,8,again,sizeof(again)) ||
        memcmp(head,again,sizeof(head))) return 1;
    *out=value; return 0;
}
static inline int jc_cloth_storage_read(jc_layout_reader read,void *ctx,uint64_t material,
        struct jc_cloth_storage *out) {
    struct jc_cloth_storage s; uint64_t collections[2],again; int32_t active;
    memset(&s,0,sizeof(s));
    if (jc_material_ready(read,ctx,material,&s.payload) ||
        !jc_layout_field(read,ctx,s.payload,0x80,collections,sizeof(collections)) ||
        !jc_layout_field(read,ctx,s.payload,0x90,&active,4) || active<0 || active>1) return 1;
    for (unsigned variant=0;variant<2;++variant) {
        uint32_t count; uint64_t members[16],table;
        if (!collections[variant]) { if ((int)variant==active) return 1; else continue; }
        if (!jc_layout_field(read,ctx,collections[variant],0,&count,4) ||
            !jc_layout_field(read,ctx,collections[variant],8,&table,8)) return 1;
        count&=0x7fffffff;
        if (count>16 || (!count && (int)variant==active)) return 1;
        if (count==1) members[0]=table;
        else if (count && !jc_layout_field(read,ctx,table,0,members,count*8)) return 1;
        for (unsigned i=0;i<count;++i) {
            uint64_t buffer,end; uint32_t bytes;
            if (!jc_layout_field(read,ctx,members[i],8,&buffer,8) ||
                !jc_layout_field(read,ctx,buffer,0,&bytes,4) || bytes<4 || bytes>65536 ||
                !jc_layout_address(buffer,bytes,&end)) return 1;
            unsigned j=0; while (j<s.count && s.spans[j].begin!=buffer) ++j;
            if (j<s.count) { if (s.spans[j].bytes!=bytes) return 1; continue; }
            s.spans[s.count].begin=buffer; s.spans[s.count++].bytes=bytes;
        }
    }
    if (!s.count || jc_material_ready(read,ctx,material,&again) || again!=s.payload) return 1;
    *out=s; return 0;
}
static inline int jc_cloth_storage_disjoint(const struct jc_cloth_storage *a,const struct jc_cloth_storage *b) {
    if (a->payload==b->payload) return 0;
    for (unsigned i=0;i<a->count;++i) for (unsigned j=0;j<b->count;++j)
        if (a->spans[i].begin<b->spans[j].begin+b->spans[j].bytes &&
            b->spans[j].begin<a->spans[i].begin+a->spans[i].bytes) return 0;
    return 1;
}

struct jc_cloth_appearance_route {
    uint64_t material[2],destination[2],payload[2],target_payload[2];
    unsigned ready;
};
static inline int jc_cloth_appearance_route_make(const uint64_t source[2],const uint64_t target[2],
        const struct jc_cloth_storage stock[2],const struct jc_cloth_storage owned[2],
        struct jc_cloth_appearance_route *out) {
    if (!source || !target || !stock || !owned || !out) return 1;
    memset(out,0,sizeof(*out));
    for (unsigned i=0;i<2;++i) {
        if (!source[i] || !target[i] || !stock[i].payload || !owned[i].payload ||
            !stock[i].count || stock[i].count>32 || !owned[i].count || owned[i].count>32) return 1;
    }
    for (unsigned i=0;i<2;++i) {
        for (unsigned j=0;j<2;++j)
            if (source[i]==target[j] || !jc_cloth_storage_disjoint(stock+i,owned+j)) return 1;
    }
    if (source[0]==source[1] || target[0]==target[1] ||
        stock[0].payload==stock[1].payload || !jc_cloth_storage_disjoint(owned,owned+1)) return 1;
    for (unsigned i=0;i<2;++i) {
        out->material[i]=source[i];out->destination[i]=target[i];
        out->payload[i]=stock[i].payload;out->target_payload[i]=owned[i].payload;
    }
    out->ready=1;return 0;
}
static inline uint64_t jc_cloth_appearance_target(const struct jc_cloth_appearance_route *route,
        uint64_t object,unsigned lookup) {
    if (!route || !route->ready) return object;
    for (unsigned i=0;i<2;++i)
        if (object==(lookup ? route->payload[i] : route->material[i]))
            return lookup ? route->target_payload[i] : route->destination[i];
    return object;
}

/* Check the 24-byte binding written by 273640 -> 2db010 -> 270a80. */
static inline int jc_cloth_texture_check(jc_layout_reader read,void *ctx,uint64_t material,
        const char *name,uint64_t texture,int bound) {
    uint64_t payload,collection,member,definition,buffer,table,handle,again;
    uint32_t count,bytes,kind,index,flags=0;int32_t variant;
    if (!name || strlen(name)>31 || jc_material_ready(read,ctx,material,&payload) ||
        !jc_layout_field(read,ctx,payload,0x90,&variant,4) || variant<0 || variant>1 ||
        !jc_layout_field(read,ctx,payload,0x80+variant*8,&collection,8) ||
        !jc_layout_field(read,ctx,collection,0,&count,4) || !(count&0x7fffffff) ||
        !jc_layout_field(read,ctx,collection,8,&member,8)) return 1;
    if ((count&0x7fffffff)>1 && !jc_layout_field(read,ctx,member,0,&member,8)) return 1;
    if (!jc_layout_field(read,ctx,member,0,&definition,8) ||
        !jc_layout_field(read,ctx,member,8,&buffer,8) ||
        !jc_layout_field(read,ctx,buffer,0,&bytes,4) || bytes<28 || bytes>65536 ||
        !jc_layout_field(read,ctx,definition,0x40,&count,4) ||
        !jc_layout_field(read,ctx,definition,0x48,&table,8) ||
        !(count&0x7fffffff) || (count&0x7fffffff)>256 ||
        !jc_layout_field(read,ctx,texture,0x70,&handle,8) ||
        !jc_layout_field(read,ctx,texture,0x110,&kind,4) || kind<1 || kind>3 ||
        !jc_layout_field(read,ctx,handle,0xc,&index,4) ||
        (kind==1 && !jc_layout_field(read,ctx,handle,0x94,&flags,4))) return 1;
    for (unsigned i=0;i<(count&0x7fffffff);++i) {
        unsigned char field[24],check[24],binding[24],expected[24]={0};
        uint64_t address;uint16_t offset,width;char candidate[32];
        if (!jc_layout_field(read,ctx,table,i*24,field,sizeof(field))) return 1;
        memcpy(&address,field+8,8);address&=~UINT64_C(1);
        if (!jc_layout_field(read,ctx,address,0,candidate,strlen(name)+1)) return 1;
        if (memcmp(candidate,name,strlen(name)+1)) continue;
        memcpy(&offset,field+16,2);memcpy(&width,field+18,2);width&=0x1fff;
        if (offset<4 || width!=24 || (uint32_t)offset+width>bytes ||
            (kind!=1 && field[2]!=0x41+kind) ||
            (kind==1 && field[2]!=0x42 && field[2]!=0x12 && (field[2]<0xd9 || field[2]>0xdc))) return 1;
        uint32_t type=0x41+kind;
        if (kind==1) type|=flags&0x80000000u;
        uint64_t owner=texture+8;
        memcpy(expected,&type,4);memcpy(expected+4,&index,4);
        memcpy(expected+8,&owner,8);memcpy(expected+16,&handle,8);
        uint32_t fresh_kind,fresh_index,fresh_flags=0;int32_t fresh_variant;
        if (!jc_layout_field(read,ctx,buffer,offset,binding,24) || (bound && memcmp(binding,expected,24)) ||
            !jc_layout_field(read,ctx,table,i*24,check,24) || memcmp(field,check,24) ||
            !jc_layout_field(read,ctx,texture,0x70,&again,8) || again!=handle ||
            !jc_layout_field(read,ctx,texture,0x110,&fresh_kind,4) || fresh_kind!=kind ||
            !jc_layout_field(read,ctx,handle,0xc,&fresh_index,4) || fresh_index!=index ||
            (kind==1 && (!jc_layout_field(read,ctx,handle,0x94,&fresh_flags,4) || fresh_flags!=flags)) ||
            !jc_layout_field(read,ctx,payload,0x90,&fresh_variant,4) || fresh_variant!=variant ||
            !jc_layout_field(read,ctx,payload,0x80+variant*8,&again,8) || again!=collection ||
            jc_material_ready(read,ctx,material,&again) || again!=payload) return 1;
        return 0;
    }
    return 1;
}
static inline int jc_cloth_texture_bound(jc_layout_reader read,void *ctx,uint64_t material,
        const char *name,uint64_t texture) {
    return jc_cloth_texture_check(read,ctx,material,name,texture,1);
}

static inline int jc_cloth_texture_bundle_bound(jc_layout_reader read,void *ctx,uint64_t material,
        unsigned role,const uint64_t atlas[6],uint64_t environment) {
    static const char *const names[6]={"textureAtlasA","textureAtlasB","textureMask",
        "textureDetail","textureFilledA","textureFilledB"};
    if ((role!=4 && role!=5) || !atlas || !environment) return 1;
    for (unsigned i=0;i<(role==5 ? 6u : 4u);++i)
        if (atlas[i]==environment || jc_cloth_texture_bound(read,ctx,material,names[i],atlas[i])) return 1;
    return jc_cloth_texture_bound(read,ctx,material,"envOccCube",environment);
}
#ifdef _WIN32
#include <d3d11.h>
static inline uint64_t jc_cloth_graphics_native_mutex(void *ctx) {
    uintptr_t (*get)(void)=(void *)((uintptr_t)ctx+0xc0840);return get();
}
static inline uint32_t jc_cloth_graphics_native_thread(void *ctx) {
    uint32_t (*get)(void)=(void *)((uintptr_t)ctx+0x34bff0);return get();
}
static inline void jc_cloth_graphics_native_enter(void *ctx,uint64_t mutex) {
    void (*enter)(uintptr_t)=(void *)((uintptr_t)ctx+0x374b80);enter((uintptr_t)mutex);
}
static inline void jc_cloth_graphics_native_leave(void *ctx,uint64_t mutex) {
    void (*leave)(uintptr_t)=(void *)((uintptr_t)ctx+0x374c00);leave((uintptr_t)mutex);
}
static inline struct jc_cloth_graphics_ops jc_cloth_graphics_native_ops(uint64_t image) {
    return (struct jc_cloth_graphics_ops){(void *)(uintptr_t)image,jc_cloth_graphics_native_mutex,
        jc_cloth_graphics_native_thread,jc_cloth_graphics_native_enter,jc_cloth_graphics_native_leave};
}
struct jc_cloth_atlas_native {
    uint64_t image;
    jc_layout_reader read;
    void *read_context;
    ID3D11DeviceContext *device_context;
    unsigned role,index;
};
static inline int jc_cloth_atlas_native_hold(void *ctx,const uint64_t pair[2]) {
    struct jc_cloth_atlas_native *s=ctx;int32_t refs;
    if (!jc_layout_field(s->read,s->read_context,pair[1],0,&refs,4)) return 0;
    for (unsigned i=0;i<64;++i) {
        if (refs<=0 || refs>=0x7ffffffe) return 0;
        LONG old=InterlockedCompareExchange((volatile LONG *)(uintptr_t)pair[1],refs+1,refs);
        if (old==refs) return 1;refs=old;
    }
    return 0;
}
static inline void jc_cloth_atlas_native_drop(void *ctx,const uint64_t pair[2]) {
    struct jc_cloth_atlas_native *s=ctx;uint64_t own[2]={pair[0],pair[1]};
    void (*drop)(uint64_t *)=(void *)(uintptr_t)(s->image+0x8e9b0);drop(own);
}
static inline void jc_cloth_atlas_native_release(void *ctx,uint64_t object) {
    (void)ctx;IUnknown *p=(void *)(uintptr_t)object;p->lpVtbl->Release(p);
}
static inline int jc_cloth_atlas_native_clone(void *ctx,const uint64_t source[4],
        uint64_t shell,uint64_t out[4]) {
    struct jc_cloth_atlas_native *s=ctx;ID3D11Device *device=NULL;
    ID3D11Texture2D *original=(void *)(uintptr_t)source[0],*texture=NULL;
    ID3D11ShaderResourceView *views[2]={NULL,NULL};ID3D11SamplerState *sampler=NULL;
    D3D11_TEXTURE2D_DESC desc;D3D11_SHADER_RESOURCE_VIEW_DESC view_desc[2];int result=0;
    if (!s->device_context || !source[0] || !source[1] ||
        s->device_context->lpVtbl->GetType(s->device_context)!=D3D11_DEVICE_CONTEXT_IMMEDIATE) return 0;
    original->lpVtbl->GetDesc(original,&desc);
    if (!desc.Width || desc.Width>16384 || !desc.Height || desc.Height>16384 ||
        !desc.MipLevels || desc.MipLevels>15 || desc.ArraySize!=1 || desc.SampleDesc.Count!=1 ||
        desc.SampleDesc.Quality || desc.CPUAccessFlags ||
        (desc.Usage!=D3D11_USAGE_DEFAULT && desc.Usage!=D3D11_USAGE_IMMUTABLE)) return 0;
    for (unsigned i=0;i<2;++i) if (source[1+i]) {
        ID3D11ShaderResourceView *view=(void *)(uintptr_t)source[1+i];ID3D11Resource *resource=NULL;
        view->lpVtbl->GetResource(view,&resource);
        int matches=(uintptr_t)resource==source[0];if (resource) resource->lpVtbl->Release(resource);
        if (!matches) return 0;
        view->lpVtbl->GetDesc(view,view_desc+i);
        if (view_desc[i].ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D ||
            view_desc[i].Texture2D.MostDetailedMip>=desc.MipLevels ||
            (view_desc[i].Texture2D.MipLevels!=UINT32_MAX &&
             (!view_desc[i].Texture2D.MipLevels || view_desc[i].Texture2D.MipLevels>
              desc.MipLevels-view_desc[i].Texture2D.MostDetailedMip))) return 0;
    }
    s->device_context->lpVtbl->GetDevice(s->device_context,&device);if (!device) return 0;
    ID3D11Device *source_device=NULL;original->lpVtbl->GetDevice(original,&source_device);
    int same_device=source_device==device;if (source_device) source_device->lpVtbl->Release(source_device);
    if (!same_device) goto done;
    desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=0;desc.MiscFlags=0;
    HRESULT made=device->lpVtbl->CreateTexture2D(device,&desc,NULL,&texture);out[0]=(uintptr_t)texture;
    if (FAILED(made) || !texture) goto done;
    for (unsigned i=0;i<2;++i) if (source[i+1]) {
        made=device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)texture,view_desc+i,views+i);
        out[i+1]=(uintptr_t)views[i];if (FAILED(made) || !views[i]) goto done;
    }
    int configured=jc_cloth_atlas_sampler_prepare((unsigned char *)(uintptr_t)shell,s->role,s->index);
    if (source[3] && !configured) {
        D3D11_SAMPLER_DESC sampler_desc;ID3D11SamplerState *from=(void *)(uintptr_t)source[3];
        from->lpVtbl->GetDesc(from,&sampler_desc);
        made=device->lpVtbl->CreateSamplerState(device,&sampler_desc,&sampler);out[3]=(uintptr_t)sampler;
        if (FAILED(made) || !sampler) goto done;
    } else {
        uint32_t flags;memcpy(&flags,(void *)(uintptr_t)(shell+0x24),4);flags&=~0x80u;
        memcpy((void *)(uintptr_t)(shell+0x24),&flags,4);
        int (*create)(uintptr_t)=(void *)(uintptr_t)(s->image+0x362db0);
        int code=create((uintptr_t)shell+8);memcpy(out+3,(void *)(uintptr_t)(shell+0x28),8);
        if (code || !out[3]) goto done;
    }
    s->device_context->lpVtbl->CopyResource(s->device_context,(ID3D11Resource *)texture,(ID3D11Resource *)original);
    result=1;
done:
    device->lpVtbl->Release(device);return result;
}
static inline struct jc_cloth_atlas_copy_ops jc_cloth_atlas_native_ops(struct jc_cloth_atlas_native *s) {
    return (struct jc_cloth_atlas_copy_ops){s,jc_cloth_atlas_native_hold,jc_cloth_atlas_native_drop,
        jc_cloth_atlas_native_clone,jc_cloth_atlas_native_release};
}
struct jc_cloth_live_material {
    unsigned char *subscriber;
    unsigned char *atlas_holders;
    uint64_t atlas_entries[6],atlas_textures[6],atlas_original[6];
    struct jc_cloth_atlas_snapshot atlas_snapshots[6];
    unsigned atlas_outfit;
    uint64_t image,geometry,model,control,peer,generation;
    DWORD thread;
    unsigned role,pins;
    int ready,quarantined;
    struct jc_cloth_storage storage;
};
static inline int jc_cloth_live_atlas_check(jc_layout_reader read,void *ctx,
        const struct jc_cloth_live_material *m,unsigned i,uint64_t *texture) {
    uint64_t links[2];
    if (!m || i>=(m->role==5 ? 6u : 4u) || !m->atlas_holders || !m->atlas_entries[i] ||
        !jc_layout_field(read,ctx,(uintptr_t)m->atlas_holders+i*0x28,0,links,sizeof(links)) ||
        links[0]!=m->image+0x5cd680 || links[1]!=m->atlas_entries[i]+0x28 ||
        jc_cloth_atlas_snapshot_check(m->atlas_snapshots+i) ||
        m->atlas_snapshots[i].entry!=m->atlas_entries[i] ||
        m->atlas_snapshots[i].source!=m->atlas_original[i]) return 1;
    *texture=(uintptr_t)m->atlas_snapshots[i].texture; return 0;
}
/* Loaded resources only. These subscriptions are not GPU retirement fences. */
static inline int jc_cloth_live_atlas_select(jc_layout_reader read,void *ctx,
        struct jc_cloth_live_material *m,uint64_t manager,uint64_t barn,unsigned outfit,const char *const names[3]) {
    if (!m || !m->ready || m->quarantined || m->pins || m->thread!=GetCurrentThreadId() ||
        !manager || !barn || !names || outfit>6) return 1;
    if (m->atlas_holders) {
        if (m->atlas_outfit!=outfit) return 1;
        for (unsigned i=0;i<(m->role==5 ? 6u : 4u);++i) {
            uint64_t texture;
            if (jc_cloth_live_atlas_check(read,ctx,m,i,&texture) || texture!=m->atlas_textures[i]) return 2;
        }
        return 0;
    }
    uint64_t type,vtable,entries[6],payload,texture,manager_context;
    unsigned count=m->role==5 ? 6u : 4u;
    uintptr_t (*lookup)(uintptr_t,uintptr_t,const char *)=(void *)(uintptr_t)(m->image+0x2ce120);
    if (!jc_layout_field(read,ctx,manager,8,&manager_context,8) || !manager_context ||
        !jc_layout_field(read,ctx,m->image,0x403f050,&type,8) ||
        !jc_layout_field(read,ctx,type,0,&vtable,8) || vtable!=m->image+0x5cd4c8) return 2;
    for (unsigned i=0;i<count;++i) {
        struct jc_cloth_atlas_list list;
        if (i<3) {
            if (!names[i] || !(entries[i]=lookup((uintptr_t)manager,(uintptr_t)type,names[i]))) return 2;
        } else {
            uint64_t link;
            if (!jc_layout_field(read,ctx,barn,0x6698c0+(i-3)*0x28,&link,8) || link<0x10028) return 2;
            entries[i]=link-0x28;
        }
        if (jc_cloth_atlas_list_read(read,ctx,entries[i],&list) || list.count==256 ||
            !jc_layout_field(read,ctx,entries[i],0x38,&payload,8) ||
            !jc_layout_field(read,ctx,payload,0xc8,&texture,8) || !texture) return 2;
    }
    m->atlas_holders=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,count*0x28);
    if (!m->atlas_holders) return 3;
    m->atlas_outfit=outfit;
    void (*bind)(uintptr_t,uintptr_t,void *)=(void *)(uintptr_t)(m->image+0x2cca00);
    for (unsigned i=0;i<(m->role==5 ? 6u : 4u);++i) {
        struct jc_cloth_atlas_list before,after;
        unsigned char *holder=m->atlas_holders+i*0x28;
        vtable=m->image+0x5cd680; memcpy(holder,&vtable,8);
        m->atlas_entries[i]=entries[i];
        if (jc_cloth_atlas_list_read(read,ctx,entries[i],&before)) goto failed;
        bind((uintptr_t)entries[i],(uintptr_t)manager_context,holder);
        if (jc_cloth_atlas_list_read(read,ctx,entries[i],&after) ||
            jc_cloth_atlas_list_changed(&before,&after,(uintptr_t)holder+8,1) ||
            !jc_layout_field(read,ctx,entries[i],0x38,&payload,8) ||
            !jc_layout_field(read,ctx,payload,0xc8,m->atlas_original+i,8)) goto failed;
        struct jc_cloth_graphics_ops graphics=jc_cloth_graphics_native_ops(m->image);
        struct jc_cloth_graphics_token token={0};
        if (jc_cloth_graphics_lock(read,ctx,m->image,&graphics,0,&token)) goto failed;
        struct jc_cloth_atlas_native native={.image=m->image,.read=read,.read_context=ctx,
            .device_context=(void *)(uintptr_t)token.device_context};
        native.role=m->role;native.index=i;
        struct jc_cloth_atlas_copy_ops ops=jc_cloth_atlas_native_ops(&native);
        int copied=jc_cloth_atlas_snapshot_create(read,ctx,m->image,entries[i],m->atlas_original[i],&ops,
            m->atlas_snapshots+i);
        int unlocked=jc_cloth_graphics_unlock(read,ctx,m->image,&graphics,&token);
        if (copied || unlocked || jc_cloth_live_atlas_check(read,ctx,m,i,m->atlas_textures+i)) goto failed;
    }
    return 0;
failed:
    m->quarantined=1; return 4;
}
static inline int jc_cloth_live_atlas_destroy(jc_layout_reader read,void *ctx,
        struct jc_cloth_live_material *m) {
    if (!m || m->pins || m->quarantined || m->thread!=GetCurrentThreadId()) return 1;
    if (!m->atlas_holders) return 0;
    for (unsigned i=0;i<(m->role==5 ? 6u : 4u);++i) if (m->atlas_snapshots[i].pins) return 1;
    struct jc_cloth_graphics_ops graphics=jc_cloth_graphics_native_ops(m->image);
    struct jc_cloth_graphics_token token={0};
    if (jc_cloth_graphics_lock(read,ctx,m->image,&graphics,0,&token)) return 1;
    struct jc_cloth_atlas_native native={.image=m->image,.read=read,.read_context=ctx,
            .device_context=(void *)(uintptr_t)token.device_context};
    struct jc_cloth_atlas_copy_ops ops=jc_cloth_atlas_native_ops(&native);
    int destroyed=0;
    for (unsigned i=0;i<(m->role==5 ? 6u : 4u);++i)
        if (jc_cloth_atlas_snapshot_destroy(&ops,m->atlas_snapshots+i)) { destroyed=1;break; }
    if (jc_cloth_graphics_unlock(read,ctx,m->image,&graphics,&token) || destroyed) goto failed;
    void (*destroy)(void *)=(void *)(uintptr_t)(m->image+0x85b70);
    for (unsigned i=0;i<(m->role==5 ? 6u : 4u);++i) {
        struct jc_cloth_atlas_list before,after; uint64_t links[3];
        unsigned char *holder=m->atlas_holders+i*0x28;
        if (jc_cloth_atlas_list_read(read,ctx,m->atlas_entries[i],&before)) goto failed;
        unsigned found=0;
        for (unsigned j=0;j<before.count;++j) found+=before.nodes[j]==(uintptr_t)holder+8;
        if (found!=1) goto failed;
        destroy(holder);
        if (!jc_layout_field(read,ctx,(uintptr_t)holder,8,links,sizeof(links)) ||
            links[0] || links[1] || links[2] ||
            jc_cloth_atlas_list_read(read,ctx,m->atlas_entries[i],&after) ||
            jc_cloth_atlas_list_changed(&before,&after,(uintptr_t)holder+8,0)) goto failed;
    }
    HeapFree(GetProcessHeap(),0,m->atlas_holders); m->atlas_holders=NULL;
    memset(m->atlas_entries,0,sizeof(m->atlas_entries));memset(m->atlas_textures,0,sizeof(m->atlas_textures));
    memset(m->atlas_original,0,sizeof(m->atlas_original));
    return 0;
failed:
    m->quarantined=1; return 2;
}
/* Caller holds the game-thread lifetime lock, before any publication. Storage
   stays fixed until native destruction; never copy an initialized subscriber. */
static inline int jc_cloth_live_material_create(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t barn,uint64_t manager,unsigned role,uint64_t peer,uint64_t generation,
        struct jc_cloth_live_material *m) {
    if (!m || m->subscriber || !peer || !generation || (role!=4 && role!=5) || !manager) return 1;
    uint64_t base,geometry,type,original[2],shader_address; char shader_name[16];
    const char *expected=role==4 ? "ClothRemote" : "ClothRemoteScarf";
    if (!jc_layout_address(barn,0x666d68+(uint64_t)role*0x630,&base) ||
        !jc_layout_address(base,0x78,&geometry) || !jc_layout_field(read,ctx,geometry,0,&type,8) ||
        type!=image+0x5cd660 || !jc_layout_field(read,ctx,base,0x170,original,sizeof(original)) ||
        !original[0] || !original[1] ||
        !jc_layout_field(read,ctx,image,0x5c3840+role*8,&shader_address,8) ||
        !jc_layout_field(read,ctx,shader_address,0,shader_name,strlen(expected)+1) ||
        memcmp(shader_name,expected,strlen(expected)+1)) return 2;
    struct jc_cloth_subscribers before,after;
    struct jc_cloth_storage stock;
    if (jc_cloth_subscribers_read(read,ctx,geometry,&before) || before.count>=JC_CLOTH_SUBSCRIBERS ||
        jc_cloth_storage_read(read,ctx,base+0x180,&stock)) return 2;
    unsigned stock_found=0;
    for (unsigned i=0;i<before.count;++i) stock_found+=before.nodes[i]==base+0xf8;
    if (stock_found!=1) return 2;
    unsigned char *p=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,JC_CLOTH_SUBSCRIBER_BYTES);
    if (!p) return 3;
    *m=(struct jc_cloth_live_material){.subscriber=p,.image=image,.geometry=geometry,
        .peer=peer,.generation=generation,.thread=GetCurrentThreadId(),.role=role,.quarantined=1};
    uint64_t inline_buffer=(uintptr_t)(p+0x18); memcpy(p+0x58,&inline_buffer,8);
    void *(*construct)(void *)=(void *)(uintptr_t)(image+0x272000);
    void (*load)(uintptr_t,void *,uintptr_t,const char *)=(void *)(uintptr_t)(image+0x2696b0);
    construct(p+0x88); load((uintptr_t)geometry,p,(uintptr_t)manager,(const char *)(uintptr_t)shader_address);
    memcpy(&m->model,p+0x78,8); memcpy(&m->control,p+0x80,8);
    if (!m->model || !m->control || m->model==original[0] || m->control==original[1] ||
        jc_cloth_subscribers_read(read,ctx,geometry,&after) || after.count!=before.count+1 ||
        memcmp(after.nodes,before.nodes,before.count*8) || after.nodes[before.count]!=(uintptr_t)p ||
        jc_cloth_storage_read(read,ctx,(uintptr_t)p+0x88,&m->storage) ||
        !jc_cloth_storage_disjoint(&stock,&m->storage)) return 4;
    /* Every subscriber, including previously added peers, must own its shader data. */
    for (unsigned i=0;i<before.count;++i) {
        struct jc_cloth_storage other; uint64_t pair[2];
        if (!jc_layout_field(read,ctx,before.nodes[i],0x78,pair,sizeof(pair)) ||
            pair[0]==m->model || pair[1]==m->control ||
            jc_cloth_storage_read(read,ctx,before.nodes[i]+0x88,&other) ||
            !jc_cloth_storage_disjoint(&other,&m->storage)) return 4;
    }
    struct jc_cloth_storage stock_after,own_after;
    if (jc_cloth_storage_read(read,ctx,base+0x180,&stock_after) || memcmp(&stock,&stock_after,sizeof(stock)) ||
        jc_cloth_storage_read(read,ctx,(uintptr_t)p+0x88,&own_after) ||
        memcmp(&m->storage,&own_after,sizeof(own_after))) return 4;
    m->quarantined=0; m->ready=1; return 0;
}
static inline int jc_cloth_live_material_pin(jc_layout_reader read,void *ctx,struct jc_cloth_live_material *m,
        uint64_t peer,uint64_t generation) {
    struct jc_cloth_storage current;
    if (!m || !m->ready || m->quarantined || m->thread!=GetCurrentThreadId() ||
        m->peer!=peer || m->generation!=generation || m->pins==UINT32_MAX || !m->atlas_holders) return 1;
    for (unsigned i=0;i<(m->role==5 ? 6u : 4u);++i) {
        uint64_t texture;
        if (jc_cloth_live_atlas_check(read,ctx,m,i,&texture) || texture!=m->atlas_textures[i]) {
            m->quarantined=1;return 2;
        }
    }
    if (jc_cloth_storage_read(read,ctx,(uintptr_t)m->subscriber+0x88,&current) ||
        memcmp(&current,&m->storage,sizeof(current))) { m->quarantined=1; return 2; }
    ++m->pins; return 0;
}
/* Only after the corresponding frame retirement; this is not a GPU wait. */
static inline int jc_cloth_live_material_unpin(struct jc_cloth_live_material *m) {
    if (!m || !m->pins || m->thread!=GetCurrentThreadId()) return 1;
    --m->pins; return 0;
}
static inline int jc_cloth_live_material_destroy(jc_layout_reader read,void *ctx,struct jc_cloth_live_material *m) {
    if (!m || !m->subscriber || !m->ready || m->quarantined || m->pins || m->thread!=GetCurrentThreadId()) return 1;
    struct jc_cloth_subscribers before,after; uint64_t expected[JC_CLOTH_SUBSCRIBERS]; unsigned count=0,found=0;
    if (jc_cloth_subscribers_read(read,ctx,m->geometry,&before)) return 2;
    for (unsigned i=0;i<before.count;++i) {
        if (before.nodes[i]==(uintptr_t)m->subscriber) ++found;
        else expected[count++]=before.nodes[i];
    }
    if (found!=1) return 2;
    m->ready=0; m->quarantined=1;
    void (*destroy)(void *)=(void *)(uintptr_t)(m->image+0x269b10);
    destroy(m->subscriber);
    if (jc_cloth_subscribers_read(read,ctx,m->geometry,&after) || after.count!=count ||
        memcmp(after.nodes,expected,count*8)) return 3;
    m->quarantined=0;
    if (jc_cloth_live_atlas_destroy(read,ctx,m)) { m->quarantined=1; return 3; }
    HeapFree(GetProcessHeap(),0,m->subscriber); memset(m,0,sizeof(*m)); return 0;
}
#endif
#endif
