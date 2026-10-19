#ifndef JC_NATIVE_ENVIRONMENT_PROBE_H
#define JC_NATIVE_ENVIRONMENT_PROBE_H
#include "native_layout.h"
#include <stdio.h>

struct jc_environment_occluder {
    _Alignas(16) unsigned char node[0xd0];
    unsigned char triangles[255*3],faces[255],edges[4096*2];
    _Alignas(16) unsigned char vertices[256*16];
    unsigned bytes,planes;
};
static inline int jc_environment_occluder_read(jc_layout_reader read,void *ctx,uint64_t source,
        struct jc_environment_occluder *out) {
    unsigned char before[0xd0],again[0xd0];uint32_t counts[4];uint64_t pointers[4];
    if (!out || !jc_layout_field(read,ctx,source,0,before,sizeof(before))) return 1;
    memcpy(counts,before+0x70,sizeof(counts));memcpy(pointers,before+0x50,sizeof(pointers));
    if (!counts[0] || counts[0]>255 || !counts[1] || counts[1]>255 ||
        counts[2]>4096 || !counts[3] || counts[3]>256) return 1;
    unsigned char *dest[4]={out->triangles,out->faces,out->edges,out->vertices};
    size_t sizes[4]={counts[0]*3u,counts[1],counts[2]*2u,counts[3]*16u};
    unsigned bytes=0xd0;
    for (unsigned i=0;i<4;++i) {
        if (sizes[i] && !jc_layout_field(read,ctx,pointers[i],0,dest[i],sizes[i])) return 1;
        bytes+=(unsigned)((sizes[i]+15)&~(size_t)15);
    }
    unsigned sum=0;
    for (unsigned i=0;i<counts[1];++i) { if (!out->faces[i]) return 1;sum+=out->faces[i]; }
    if (sum!=counts[0]) return 1;
    for (unsigned i=0;i<counts[0]*3;++i) if (out->triangles[i]>=counts[3]) return 1;
    for (unsigned i=0;i<counts[3];++i) {
        uint32_t v[4];memcpy(v,out->vertices+i*16,sizeof(v));
        if (!jc_layout_finite(v)) return 1;
    }
    if (!jc_layout_field(read,ctx,source,0,again,sizeof(again)) || memcmp(before,again,sizeof(before))) return 1;
    memcpy(out->node,before,sizeof(before));
    for (unsigned i=0;i<4;++i) { uint64_t p=(uintptr_t)dest[i];memcpy(out->node+0x50+i*8,&p,8); }
    out->bytes=bytes;out->planes=counts[1];return 0;
}

/* Texture configuration follows ClothBarn initialization, 88970..88c54. */
struct jc_environment_texture {
    uint64_t texture,handle,resource,namespace_object;
    uint32_t kind,flags,rings,size;
    unsigned char sampler[5];
};
static inline int jc_environment_texture_read(jc_layout_reader read,void *ctx,uint64_t texture,
        struct jc_environment_texture *out) {
    struct jc_environment_texture v={.texture=texture};uint64_t handle_again=0;
    if (!out || !jc_layout_field(read,ctx,texture,0x70,&v.handle,8) || !v.handle ||
        !jc_layout_field(read,ctx,texture,0x110,&v.kind,4) || v.kind!=3 ||
        !jc_layout_field(read,ctx,texture,0x50,&v.namespace_object,8) || !v.namespace_object ||
        !jc_layout_field(read,ctx,texture,0x60,&v.resource,8) || !v.resource ||
        !jc_layout_field(read,ctx,texture,8,v.sampler,5) ||
        !jc_layout_field(read,ctx,v.handle,0x18,&v.flags,4) ||
        !jc_layout_field(read,ctx,v.handle,0x28,&v.size,4) ||
        !jc_layout_field(read,ctx,v.handle,0x30,&v.rings,4) ||
        !jc_layout_field(read,ctx,texture,0x70,&handle_again,8) || handle_again!=v.handle) return 1;
    *out=v;return 0;
}
static inline int jc_environment_texture_ready(const struct jc_environment_texture *v) {
    static const unsigned char sampler[5]={5,1,2,2,2};
    return v && v->texture && v->handle && v->resource && v->namespace_object &&
        v->kind==3 && v->size==8 && (v->rings&0x7fffffff)==3 &&
        (v->flags&0x23e)==0x1c && !memcmp(v->sampler,sampler,5);
}
static inline int jc_environment_texture_same(const struct jc_environment_texture *a,
        const struct jc_environment_texture *b) {
    return a->texture==b->texture && a->handle==b->handle && a->resource==b->resource &&
        a->namespace_object==b->namespace_object && a->kind==b->kind && a->flags==b->flags &&
        a->rings==b->rings && a->size==b->size && !memcmp(a->sampler,b->sampler,sizeof(a->sampler));
}
#if defined(__x86_64__) && !defined(_WIN32)
#define JC_ENV_ABI __attribute__((ms_abi))
#else
#define JC_ENV_ABI
#endif
static inline int jc_environment_texture_configure(jc_layout_reader read,void *ctx,
        uint64_t image,uint64_t texture,struct jc_environment_texture *out) {
    struct jc_environment_texture initial;uint32_t flags;
    if (!out || jc_environment_texture_read(read,ctx,texture,&initial) ||
        !jc_layout_field(read,ctx,texture,0x24,&flags,4) || (flags&0x80)) return 1;
    uint64_t handle=initial.handle;
    if (((unsigned (JC_ENV_ABI *)(uint64_t,unsigned,uint64_t,unsigned))(uintptr_t)(image+0x3b91e0))
        (handle,8,image+0x3bf4f80,0)) return 1;
    *(uint32_t *)(uintptr_t)(handle+0x18)&=~2u;
    ((void (JC_ENV_ABI *)(uint64_t))(uintptr_t)(image+0x37f7b0))(handle);
    *(uint32_t *)(uintptr_t)(handle+0x18)&=~0x200u;
    ((void (JC_ENV_ABI *)(uint64_t,unsigned))(uintptr_t)(image+0x2d8cb0))(texture,3);
    static const unsigned char sampler[5]={5,1,2,2,2};
    memcpy((void *)(uintptr_t)(texture+8),sampler,5);
    ((void (JC_ENV_ABI *)(uint64_t))(uintptr_t)(image+0x362db0))(texture+8);
    if (((unsigned (JC_ENV_ABI *)(uint64_t))(uintptr_t)(image+0x282dd0))(handle)) return 1;
    *(uint32_t *)(uintptr_t)(handle+0x18)=(*(uint32_t *)(uintptr_t)(handle+0x18)&~0x20u)|0x1c;
    struct jc_environment_texture after;
    if (jc_environment_texture_read(read,ctx,texture,&after) ||
        after.handle!=initial.handle || after.resource!=initial.resource ||
        after.namespace_object!=initial.namespace_object || !jc_environment_texture_ready(&after)) return 1;
    *out=after;return 0;
}
#if defined(_WIN32) || defined(JC_ENVIRONMENT_TEST_PLATFORM)
struct jc_environment_probe {
    uint64_t image,peer,generation,pair[2],resource[2],allocation,mapping[2];
    unsigned char *scene;
    struct jc_environment_texture identity;
    DWORD thread;
    unsigned pins,ready,produced,quarantined;
};
static inline int jc_environment_probe_check(jc_layout_reader read,void *ctx,
        const struct jc_environment_probe *p) {
    struct jc_environment_texture now;uint64_t mapping[2];uint32_t refs=0;
    return !p || !p->ready || p->quarantined || p->thread!=GetCurrentThreadId() ||
        p->mapping[0] || p->mapping[1] || p->scene ||
        !p->pair[1] || !jc_layout_field(read,ctx,p->pair[1],0,&refs,4) || !refs ||
        jc_environment_texture_read(read,ctx,p->pair[0],&now) ||
        !jc_environment_texture_ready(&now) || !jc_environment_texture_same(&now,&p->identity) ||
        !jc_layout_field(read,ctx,p->pair[0],0x78,mapping,sizeof(mapping)) || mapping[0] || mapping[1];
}
static inline int jc_environment_probe_create(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t barn,uint64_t peer,uint64_t generation,uint64_t serial,struct jc_environment_probe *p) {
    if (!p || p->pair[0] || p->pair[1] || p->ready || p->quarantined ||
        !image || !barn || !peer || !generation || !serial) return 1;
    uint64_t namespace_pair[2],stock[4],pool=0;
    struct jc_environment_texture stock_texture[2];
    if (!jc_layout_field(read,ctx,barn,0x646d10,namespace_pair,sizeof(namespace_pair)) ||
        !namespace_pair[0] || !namespace_pair[1] ||
        !jc_layout_field(read,ctx,barn,0x646d30,stock,sizeof(stock))) return 1;
    for (unsigned i=0;i<2;++i)
        if (jc_environment_texture_read(read,ctx,stock[i*2],stock_texture+i) ||
            !jc_environment_texture_ready(stock_texture+i)) return 1;
    char name[96];unsigned char native_name[32]={0};uint64_t capacity=15;
    int length=snprintf(name,sizeof(name),"JCEnv_%016llx_%016llx_%016llx",
        (unsigned long long)peer,(unsigned long long)generation,(unsigned long long)serial);
    if (length<0 || (size_t)length>=sizeof(name)) return 1;
    memcpy(native_name+24,&capacity,8);
    *p=(struct jc_environment_probe){.image=image,.peer=peer,.generation=generation,
        .thread=GetCurrentThreadId(),.quarantined=1};
    uint64_t *resource=p->resource;
    ((void (JC_ENV_ABI *)(void *,const char *,size_t))(uintptr_t)(image+0x7b920))(native_name,name,(size_t)length);
    ((void *(JC_ENV_ABI *)(uint64_t,void *,void *))(uintptr_t)(image+0x2d0330))(namespace_pair[0],resource,native_name);
    ((void (JC_ENV_ABI *)(void *))(uintptr_t)(image+0x7b7c0))(native_name);
    if (!resource[0] || !resource[1]) return 2;
    for (unsigned i=0;i<2;++i) if (resource[0]==stock_texture[i].resource) return 2;
    ((void (JC_ENV_ABI *)(uint64_t,unsigned,uint64_t,unsigned))(uintptr_t)(image+0x341e70))
        (resource[0],1,image+0x3c11060,0);
    p->allocation=((uint64_t (JC_ENV_ABI *)(size_t))(uintptr_t)(image+0x340f10))(0x120);
    if (p->allocation) p->pair[0]=((uint64_t (JC_ENV_ABI *)(uint64_t,const void *,const void *))
        (uintptr_t)(image+0x2d8480))(p->allocation,namespace_pair,resource);
    ((void (JC_ENV_ABI *)(void *))(uintptr_t)(image+0xc0d70))(resource);
    memset(resource,0,2*sizeof(*resource));
    if (!p->pair[0] || p->pair[0]!=p->allocation) return 2;
    pool=((uint64_t (JC_ENV_ABI *)(void))(uintptr_t)(image+0x91d90))();
    if (pool) p->pair[1]=((uint64_t (JC_ENV_ABI *)(uint64_t))(uintptr_t)(image+0x91c80))(pool);
    if (!p->pair[1]) return 2;
    uint32_t initial_refs;
    if (!jc_layout_field(read,ctx,p->pair[1],0,&initial_refs,4) || initial_refs) return 2;
    InterlockedIncrement((volatile LONG *)(uintptr_t)p->pair[1]);
    struct jc_environment_texture initial;
    if (jc_environment_texture_read(read,ctx,p->pair[0],&initial) ||
        initial.namespace_object!=namespace_pair[0]) return 3;
    for (unsigned i=0;i<2;++i) if (initial.texture==stock_texture[i].texture ||
        initial.handle==stock_texture[i].handle || initial.resource==stock_texture[i].resource) return 3;
    if (jc_environment_texture_configure(read,ctx,image,p->pair[0],&p->identity)) return 4;
    p->ready=1;p->quarantined=0;
    if (jc_environment_probe_check(read,ctx,p)) { p->quarantined=1;return 4; }
    return 0;
}
static inline int jc_environment_probe_update(jc_layout_reader read,void *ctx,uint64_t game,
        const float center[4],struct jc_environment_probe *p) {
    if (jc_environment_probe_check(read,ctx,p) || p->pins || !center) return 1;
    uint32_t position[4];memcpy(position,center,sizeof(position));
    if (!jc_layout_finite(position)) return 1;
    uint64_t world=0;float extent[4],threshold;
    if (!jc_layout_field(read,ctx,game,0x110,&world,8) || !world ||
        !jc_layout_field(read,ctx,p->image,0x68c7a0,extent,sizeof(extent)) ||
        !jc_layout_field(read,ctx,p->image,0x68b490,&threshold,4)) return 1;
    struct jc_environment_occluder *snapshot=HeapAlloc(GetProcessHeap(),0,sizeof(*snapshot));
    p->scene=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,0x20010);
    if (!snapshot || !p->scene) {
        if (snapshot) HeapFree(GetProcessHeap(),0,snapshot);
        if (p->scene) HeapFree(GetProcessHeap(),0,p->scene);
        p->scene=NULL;return 1;
    }
    p->produced=0;
    _Alignas(16) float low[4],high[4];
    for (unsigned i=0;i<4;++i) { low[i]=center[i]-extent[i];high[i]=center[i]+extent[i]; }
    uint64_t node=((uint64_t (JC_ENV_ABI *)(uint64_t,const void *,const void *,unsigned))
        (uintptr_t)(p->image+0xea040))(world,low,high,0x22);
    unsigned visited=0,count=0,planes=0;int failed=0;
    while (node) {
        unsigned char query[0x28];uint64_t object,next;float distance;uint32_t enabled,used;
        if (++visited>4096 || !jc_layout_field(read,ctx,node,0,query,sizeof(query))) { failed=1;break; }
        memcpy(&distance,query+0x10,4);memcpy(&object,query+0x18,8);memcpy(&next,query+0x20,8);
        uint32_t distance_bits;memcpy(&distance_bits,&distance,4);
        if ((distance_bits&0x7f800000u)==0x7f800000u) { failed=1;break; }
        if (!(distance<threshold)) break;
        if (!jc_layout_field(read,ctx,object,0x80,&enabled,4)) { failed=1;break; }
        if (enabled) {
            memcpy(&used,p->scene+0x20000,4);
            if (count==128 || jc_environment_occluder_read(read,ctx,object,snapshot) ||
                used>0x20000 || snapshot->bytes>0x20000-used || planes+snapshot->planes>4096) { failed=1;break; }
            ((void (JC_ENV_ABI *)(void *,const void *))(uintptr_t)(p->image+0xe94a0))(p->scene,snapshot->node);
            uint32_t after;memcpy(&after,p->scene+0x20000,4);
            if (after!=used+snapshot->bytes) { failed=1;break; }
            ++count;planes+=snapshot->planes;
        }
        node=next;
    }
    HeapFree(GetProcessHeap(),0,snapshot);
    uint64_t again=0;
    if (failed || !jc_layout_field(read,ctx,game,0x110,&again,8) || again!=world) {
        HeapFree(GetProcessHeap(),0,p->scene);p->scene=NULL;return 1;
    }
    p->quarantined=1;
    unsigned char dynamic=0;
    if (!jc_layout_field(read,ctx,p->pair[0],0x114,&dynamic,1)) return 2;
    ((void *(JC_ENV_ABI *)(uint64_t,void *,unsigned,unsigned char))(uintptr_t)(p->image+0x2d7fd0))
        (p->pair[0],p->mapping,2,dynamic);
    uint64_t table=0,map=0,unmap=0;unsigned char output[16]={0};
    if (!p->mapping[0] || !p->mapping[1] ||
        !jc_layout_field(read,ctx,p->mapping[0],0,&table,8) ||
        !jc_layout_field(read,ctx,table,8,&map,8) || !map ||
        !jc_layout_field(read,ctx,table,16,&unmap,8) || !unmap) return 2;
    ((void (JC_ENV_ABI *)(uint64_t,void *,unsigned))(uintptr_t)map)(p->mapping[0],output,0);
    uint64_t pixels;uint32_t row,face;
    memcpy(&pixels,output,8);memcpy(&row,output+8,4);memcpy(&face,output+12,4);
    MEMORY_BASIC_INFORMATION info;
    if (!pixels || row<32 || row>1048576 || face<(uint64_t)row*8 || face>16777216 ||
        !VirtualQuery((void *)(uintptr_t)pixels,&info,sizeof(info)) || info.State!=MEM_COMMIT ||
        (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
        !(info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) ||
        pixels<(uintptr_t)info.BaseAddress || pixels-(uintptr_t)info.BaseAddress>info.RegionSize ||
        (uint64_t)face*6>info.RegionSize-(pixels-(uintptr_t)info.BaseAddress)) return 2;
    _Alignas(16) unsigned char input[0x30]={0},streams[0x160]={0},job[0x48]={0},descriptor[0x88]={0};
    memcpy(input,center,16);memcpy(input+0x10,output,16);input[0x20]=1;
    uint64_t pointer=(uintptr_t)p->scene;memcpy(streams+0x138,&pointer,8);
    pointer=(uintptr_t)streams;memcpy(job+8,&pointer,8);
    pointer=(uintptr_t)input;descriptor[0x37]=1;memcpy(descriptor+0x38,&pointer,8);
    ((void (JC_ENV_ABI *)(void *,void *))(uintptr_t)(p->image+0x1db1f0))(job,descriptor);
    if (((unsigned (JC_ENV_ABI *)(uint64_t,unsigned))(uintptr_t)unmap)(p->mapping[0],0)) return 2;
    ((void (JC_ENV_ABI *)(void *))(uintptr_t)(p->image+0x8e9b0))(p->mapping);
    memset(p->mapping,0,sizeof(p->mapping));
    HeapFree(GetProcessHeap(),0,p->scene);p->scene=NULL;p->quarantined=0;
    if (jc_environment_probe_check(read,ctx,p)) { p->quarantined=1;return 2; }
    p->produced=1;return 0;
}
static inline int jc_environment_probe_destroy(jc_layout_reader read,void *ctx,struct jc_environment_probe *p) {
    if (jc_environment_probe_check(read,ctx,p) || p->pins) return 1;
    uint32_t refs=0;
    if (!jc_layout_field(read,ctx,p->pair[1],0,&refs,4) || refs!=1) return 1;
    ((void (JC_ENV_ABI *)(void *))(uintptr_t)(p->image+0x8e9b0))(p->pair);
    memset(p,0,sizeof(*p));return 0;
}
#endif
#undef JC_ENV_ABI
#endif
