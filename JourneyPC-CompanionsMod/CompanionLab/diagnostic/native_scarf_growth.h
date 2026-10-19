#ifndef JC_NATIVE_SCARF_GROWTH_H
#define JC_NATIVE_SCARF_GROWTH_H
#include "native_actor_live.h"
#if defined(__x86_64__) && !defined(_WIN32)
#define JC_GROWTH_ABI __attribute__((ms_abi))
#else
#define JC_GROWTH_ABI
#endif
struct jc_scarf_growth {
    uint64_t image,game,actor,pool,sounds,binding,payload;
    unsigned slot,active;
    _Alignas(16) unsigned char native[0x20];
};
static inline int jc_scarf_growth_check(jc_layout_reader read,void *ctx,const struct jc_scarf_growth *g) {
    uint64_t pool,sounds,effect,binding,payload;uint32_t count;
    memcpy(&effect,g->native+0x10,8);
    return g->pool && g->slot<512 && effect==g->pool+0x10+(uint64_t)g->slot*0x100 &&
        jc_layout_field(read,ctx,g->game,0x140,&pool,8) && pool==g->pool &&
        jc_layout_field(read,ctx,g->game,0x150,&sounds,8) && sounds==g->sounds &&
        jc_layout_field(read,ctx,pool,0x20010,&count,4) && count>g->slot && count<=512 &&
        jc_layout_field(read,ctx,effect,0x10,&binding,8) && binding && binding==g->binding &&
        jc_layout_field(read,ctx,binding,0x10,&payload,8) && payload && payload==g->payload;
}
struct jc_scarf_arrival {uint64_t actor,generation,applied;};
static inline int jc_scarf_arrival_apply(jc_layout_reader read,jc_actor_writer write,void *ctx,
        uint64_t image,uint64_t game,uint64_t actor,uint64_t state,struct jc_scarf_arrival *a) {
    if(!a || !a->generation || a->actor!=actor)return 0;
    if(a->applied==a->generation)return 1;
    struct jc_scarf_growth g={.image=image,.game=game,.actor=actor};
    float values[2];uint64_t effect;
    if(!read(ctx,state,g.native,sizeof(g.native)) || !read(ctx,state,values,sizeof(values)) ||
        !isfinite(values[0]) || !isfinite(values[1]) || values[0]<0 || values[0]>1 || values[1]<0 || values[1]>1 ||
        !jc_layout_field(read,ctx,game,0x140,&g.pool,8) || !g.pool ||
        !jc_layout_field(read,ctx,game,0x150,&g.sounds,8) || !g.sounds)return 0;
    memcpy(&effect,g.native+0x10,8);
    if(effect<g.pool+0x10 || effect-g.pool-0x10>=512*0x100 || (effect-g.pool-0x10)%0x100)return 0;
    g.slot=(unsigned)((effect-g.pool-0x10)/0x100);
    if(!read(ctx,effect+0x10,&g.binding,8) || !g.binding ||
        !read(ctx,g.binding+0x10,&g.payload,8) || !jc_scarf_growth_check(read,ctx,&g))return 0;
    float zero[2]={0},check[2];
    if(!write(ctx,state,zero,sizeof(zero)) || !read(ctx,state,check,sizeof(check)) || memcmp(check,zero,sizeof(check))) {
        write(ctx,state,values,sizeof(values));return 0;
    }
    a->applied=a->generation;return 1;
}
static inline int jc_scarf_growth_prepare(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t game,uint64_t actor,struct jc_scarf_growth *g) {
    if(g->pool)return g->image==image && g->game==game && g->actor==actor && jc_scarf_growth_check(read,ctx,g);
    uint64_t pool,resources,sounds;uint32_t count,after;
    if(!jc_layout_field(read,ctx,game,0x140,&pool,8) || !pool ||
        !jc_layout_field(read,ctx,game,0xb0,&resources,8) || !resources ||
        !jc_layout_field(read,ctx,game,0x150,&sounds,8) || !sounds ||
        !jc_layout_field(read,ctx,pool,0x20010,&count,4) || count>=512)return 0;
    *g=(struct jc_scarf_growth){.image=image,.game=game,.actor=actor,.pool=pool,.sounds=sounds,.slot=count};
    uint64_t invalid=UINT64_MAX;memcpy(g->native+0x18,&invalid,8);
    uint64_t effect=((uint64_t (JC_GROWTH_ABI *)(uint64_t,uint64_t,const char *))(uintptr_t)(image+0xc4d10))
        (pool,resources,"CapeGrow");
    memcpy(g->native+0x10,&effect,8);
    if(effect!=pool+0x10+(uint64_t)count*0x100 || !read(ctx,pool+0x20010,&after,4) || after!=count+1 ||
        !read(ctx,effect+0x10,&g->binding,8) || !g->binding ||
        !read(ctx,g->binding+0x10,&g->payload,8) || !g->payload)return 0;
    return jc_scarf_growth_check(read,ctx,g);
}
static inline int jc_scarf_growth_stop(jc_layout_reader read,jc_actor_writer write,void *ctx,struct jc_scarf_growth *g) {
    if(!g->pool || !g->active)return 1;
    if(!jc_scarf_growth_check(read,ctx,g))return 0;
    uint64_t effect,handle;memcpy(&effect,g->native+0x10,8);memcpy(&handle,g->native+0x18,8);
    unsigned char off=0;float zero=0;
    if(!write(ctx,effect+0x94,&off,1) || !write(ctx,g->actor+0x143c,&zero,4))return 0;
    if((uint32_t)handle!=UINT32_MAX) {
        if((uint32_t)handle>=512)return 0;
        ((void (JC_GROWTH_ABI *)(uint64_t,uint64_t))(uintptr_t)(g->image+0x146870))(g->sounds,handle);
    }
    handle=UINT64_MAX;memcpy(g->native+0x18,&handle,8);g->active=0;return 1;
}
static inline int jc_scarf_growth_step(jc_layout_reader read,jc_actor_writer write,void *ctx,
        struct jc_scarf_growth *g,float delta) {
    if(!isfinite(delta) || delta< -1 || delta>1 || !jc_scarf_growth_check(read,ctx,g))return 0;
    float vectors[8];uint64_t handle;memcpy(&handle,g->native+0x18,8);
    if((uint32_t)handle!=UINT32_MAX && (uint32_t)handle>=512)return 0;
    if(!read(ctx,g->actor+0x2290,vectors,16) || !read(ctx,g->actor+0x14270,vectors+4,16))return 0;
    for(unsigned i=0;i<8;++i)if(!isfinite(vectors[i]))return 0;
    float glow=fminf(1.f,fmaxf(0,(delta-jc_actor_scalar(g->image,0x68afd8))*jc_actor_scalar(g->image,0x68b520)));
    if(!write(ctx,g->actor+0x143c,&glow,4))return 0;
    g->active=1;
    ((void (JC_GROWTH_ABI *)(uint64_t,uint64_t,void *,float,uint64_t))(uintptr_t)(g->image+0x24bdf0))
        (g->actor,0,g->native,delta,g->sounds);
    return jc_scarf_growth_check(read,ctx,g);
}
#endif
