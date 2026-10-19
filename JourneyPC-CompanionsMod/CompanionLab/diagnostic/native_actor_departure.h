#ifndef JC_NATIVE_ACTOR_DEPARTURE_H
#define JC_NATIVE_ACTOR_DEPARTURE_H
#include "native_actor_live.h"
#if defined(__x86_64__) && !defined(_WIN32)
#define JC_DEPARTURE_ABI __attribute__((ms_abi))
#else
#define JC_DEPARTURE_ABI
#endif

struct jc_actor_departure {
    struct jc_actor_identity identity;
    uint64_t effects[14],sounds;
    float dissolve;
    unsigned captured,hidden;
};
static inline unsigned jc_actor_emitter(unsigned index) {return 0x9bb0+index*0x90;}
static inline int jc_actor_departure_capture(jc_layout_reader read,void *ctx,
        const struct jc_actor_identity *id,struct jc_actor_departure *out) {
    static const char *const names[14]={"FootSandDot","FootSandSmoke","AddStrandTail","UseStrandTail",
        "LoseScarf","LoseScarfGlowing","ContrailGlow","DudeSandFall","DudeSandFallDot",
        "DudeSandFallCrash","DudeSandFallCrashDot","DudeDissolveDust","DudeDissolveChunk","DudeRollingInDirt"};
    if(!out || !jc_actor_identity_check(read,ctx,id))return 0;
    struct jc_actor_departure s={.identity=*id};
    if(!jc_layout_field(read,ctx,id->game,0x150,&s.sounds,8) || !s.sounds)return 0;
    for(unsigned i=0;i<14;++i) {
        uint64_t node=id->actor+jc_actor_emitter(i),ends[2],other[2],again;
        char name[32];
        if(!read(ctx,node,&s.effects[i],8) || !s.effects[i] ||
            !read(ctx,s.effects[i],ends,16) || ends[0]!=node || ends[1]!=node ||
            !jc_layout_field(read,ctx,s.effects[i],0x18,name,sizeof(name)) ||
            !memchr(name,0,sizeof(name)) || strcmp(name,names[i]) ||
            !jc_layout_field(read,ctx,id->barn+0xb0,jc_actor_emitter(i),other,8) ||
            !jc_layout_field(read,ctx,id->barn+0x146f0,jc_actor_emitter(i),other+1,8) ||
            s.effects[i]==other[0] || s.effects[i]==other[1] ||
            !read(ctx,node,&again,8) || again!=s.effects[i])return 0;
        for(unsigned j=0;j<i;++j)if(s.effects[j]==s.effects[i])return 0;
    }
    s.captured=1;*out=s;return 1;
}
static inline int jc_actor_departure_check(jc_layout_reader read,void *ctx,
        const struct jc_actor_departure *s) {
    uint64_t sounds;
    if(!s || !s->captured || !jc_actor_identity_check(read,ctx,&s->identity) ||
        !jc_layout_field(read,ctx,s->identity.game,0x150,&sounds,8) || sounds!=s->sounds)return 0;
    for(unsigned i=0;i<14;++i) {
        uint64_t node=s->identity.actor+jc_actor_emitter(i),effect,ends[2];
        if(!read(ctx,node,&effect,8) || effect!=s->effects[i] ||
            !read(ctx,effect,ends,16) || ends[0]!=node || ends[1]!=node)return 0;
    }
    return 1;
}
static inline int jc_actor_departure_store(jc_layout_reader read,jc_actor_writer write,void *ctx,
        uint64_t actor,unsigned offset,const void *value,size_t bytes) {
    unsigned char check[16];
    return bytes<=sizeof(check) && write(ctx,actor+offset,value,bytes) &&
        read(ctx,actor+offset,check,bytes) && !memcmp(value,check,bytes);
}
static inline int jc_actor_departure_emitters(jc_layout_reader read,jc_actor_writer write,void *ctx,
        const struct jc_actor_departure *s,int keep_dust) {
    if(!jc_actor_departure_check(read,ctx,s))return 0;
    unsigned char zero=0;
    for(unsigned i=0;i<14;++i) {
        if(keep_dust && (i==11 || i==12))continue;
        if(!jc_actor_departure_store(read,write,ctx,s->identity.actor,jc_actor_emitter(i)+0x84,&zero,1))return 0;
    }
    return 1;
}
static inline int jc_actor_departure_sounds(jc_layout_reader read,void *ctx,
        const struct jc_actor_departure *s,int stop) {
    uint64_t handles[3][14];
    if(!jc_actor_departure_check(read,ctx,s) ||
        !jc_layout_field(read,ctx,s->identity.actor,0x1780,handles[0],sizeof(handles[0])) ||
        !jc_layout_field(read,ctx,s->identity.barn+0xb0,0x1780,handles[1],sizeof(handles[1])) ||
        !jc_layout_field(read,ctx,s->identity.barn+0x146f0,0x1780,handles[2],sizeof(handles[2])))return -1;
    for(unsigned i=0;i<14;++i) {
        if((uint32_t)handles[0][i]==UINT32_MAX)continue;
        if((uint32_t)handles[0][i]>=512)return -1;
        for(unsigned p=1;p<3;++p)for(unsigned j=0;j<14;++j)
            if(handles[0][i]==handles[p][j])return -1;
    }
    int idle=1;
    for(unsigned i=0;i<14;++i) {
        uint64_t handle=handles[0][i];uint32_t serial;unsigned char active;
        if((uint32_t)handle==UINT32_MAX)continue;
        uint64_t slot=s->sounds+(uint64_t)(uint32_t)handle*0x200;
        if(!read(ctx,slot+0x214,&serial,4) || !read(ctx,slot+0x218,&active,1) || active>1)return -1;
        if(!active || serial!=(uint32_t)(handle>>32))continue;
        if(!stop){idle=0;continue;}
        ((void (JC_DEPARTURE_ABI *)(uint64_t,uint64_t))(uintptr_t)(s->identity.image+0x146870))(s->sounds,handle);
        if(!read(ctx,slot+0x218,&active,1) || active)return -1;
    }
    return idle;
}
static inline int jc_actor_departure_particles_idle(jc_layout_reader read,void *ctx,
        const struct jc_actor_departure *s) {
    if(!jc_actor_departure_check(read,ctx,s))return -1;
    int idle=1;
    for(unsigned i=0;i<14;++i) {
        uint64_t particles;unsigned char enabled;
        if(!jc_layout_field(read,ctx,s->identity.actor,jc_actor_emitter(i)+0x84,&enabled,1) ||
            !jc_layout_field(read,ctx,s->effects[i],0xc0,&particles,8) || enabled>1)return -1;
        if(enabled || particles)idle=0;
    }
    return idle;
}
static inline int jc_actor_departure_med(jc_layout_reader read,void *ctx,
        const struct jc_actor_departure *s) {
    if(!jc_actor_departure_check(read,ctx,s))return -1;
    uint64_t controller=s->identity.actor+0xa390;uint32_t count;
    if(!read(ctx,controller+0x64a0,&count,4) || !count || count>196)return -1;
    for(unsigned i=0;i<16;++i) {
        uint64_t record;unsigned char active;float priority;char text[24];
        if(!read(ctx,controller+0x64cd+i*0x28,&active,1) || active>1)return -1;
        if(!active)continue;
        if(!read(ctx,controller+0x64a8+i*0x28,&record,8) || record<controller+0x33a0 ||
            record>=controller+0x33a0+count*0x40 || (record-controller-0x33a0)%0x40 ||
            !read(ctx,record+8,&priority,4) || !isfinite(priority) ||
            !read(ctx,record+0x28,text,sizeof(text)) || !memchr(text,0,sizeof(text)))return -1;
    }
    uint64_t name=((uint64_t (JC_DEPARTURE_ABI *)(uint64_t))(uintptr_t)
        (s->identity.image+0xaedd0))(controller);
    char value[4];
    if(!name)return 0;
    if(!read(ctx,name,value,sizeof(value)))return -1;
    return !memcmp(value,"Med",4);
}
static inline int jc_actor_departure_animation(jc_layout_reader read,jc_actor_writer write,void *ctx,
        const struct jc_actor_departure *s,int ready) {
    if(!jc_actor_departure_check(read,ctx,s))return 0;
    uint64_t controller=s->identity.actor+0xa390;uint32_t count,queued;
    if(!read(ctx,controller+0x64a0,&count,4) || !count || count>196 ||
        !read(ctx,controller+0x6734,&queued,4) || queued>=196)return 0;
    if(ready) {
        unsigned found=0;
        for(unsigned i=0;i<count;++i) {
            char name[24];
            if(!read(ctx,controller+0x33a0+i*0x40+0x28,name,sizeof(name)) || !memchr(name,0,sizeof(name)))return 0;
            if(!strcmp(name,"ReadyToMed"))found=1;
        }
        if(!found)return 0;
    }
    unsigned char zero=0;uint32_t empty=0;
    if(!jc_actor_departure_store(read,write,ctx,s->identity.actor,0xa390+0x999a,&zero,1) ||
        !jc_actor_departure_store(read,write,ctx,s->identity.actor,0xa390+0x6734,&empty,4))return 0;
    for(unsigned i=0;i<16;++i)
        if(!jc_actor_departure_store(read,write,ctx,s->identity.actor,0xa390+0x64cd+i*0x28,&zero,1))return 0;
    if(ready)((void (JC_DEPARTURE_ABI *)(uint64_t,const char *))(uintptr_t)
        (s->identity.image+0xaea60))(controller,"ReadyToMed");
    return 1;
}
static inline int jc_actor_departure_input(jc_layout_reader read,jc_actor_writer write,void *ctx,
        struct jc_actor_departure *s,int dust,float dt) {
    if(!jc_actor_departure_check(read,ctx,s) || !isfinite(dt) || dt<0 || dt>1)return 0;
    unsigned char zero=0,one=1;uint16_t off=0;
    if(dust)s->dissolve=fminf(1.0f,s->dissolve+jc_actor_scalar(s->identity.image,0x68b080)*dt);
    return jc_actor_departure_store(read,write,ctx,s->identity.actor,0x50,&off,2) &&
        jc_actor_departure_store(read,write,ctx,s->identity.actor,0x56,&one,1) &&
        jc_actor_departure_store(read,write,ctx,s->identity.actor,0x1e1c,&one,1) &&
        jc_actor_departure_store(read,write,ctx,s->identity.actor,0x1dfc,&zero,1) &&
        jc_actor_departure_store(read,write,ctx,s->identity.actor,0x1e00,&s->dissolve,4);
}
#undef JC_DEPARTURE_ABI
#endif
