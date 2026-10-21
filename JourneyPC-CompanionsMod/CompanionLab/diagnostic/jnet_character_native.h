#ifndef JNET_CHARACTER_NATIVE_H
#define JNET_CHARACTER_NATIVE_H
#include <stdint.h>
#include <string.h>
#include "jnet_character.h"
#define JNET_CHARACTER_GLOW_VTABLE_RVA 0x664168u

typedef int (*jnet_character_read)(void *,uint64_t,void *,size_t);
typedef int (*jnet_character_write)(void *,uint64_t,const void *,size_t);
static inline int jnet_character_attack_locked(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t creature) {
    if(!event)return 0;
    uint64_t type=0,owner=0;unsigned char state[2]={0};uint32_t phase=0;
    if(!read || !read(ctx,event,&type,8) || type!=image+0x66ce70 ||
       !read(ctx,event+0x48,&owner,8) || owner!=creature ||
       !read(ctx,event+8,state,2))return -1;
    if(!state[1])return 0;
    if(!read(ctx,event+0xac,&phase,4) || phase>2)return -1;
    return state[1] && (state[0] || phase<2);
}
static inline int jnet_character_attack_targets(jnet_character_read read,jnet_character_write write,
        void *ctx,uint64_t event,const uint64_t actors[3],int enabled) {
    uint64_t clump=0,items=0,count=0,markers[3];
    if(!read || !write || !event || !actors ||
       !read(ctx,event+0x60,&clump,8) || !clump ||
       !read(ctx,clump+8,&items,8) || !items ||
       !read(ctx,clump+0x10,&count,8) || count!=3 ||
       !read(ctx,items,markers,sizeof(markers)))return 0;
    for(unsigned i=0;i<3;++i){
        float position[3];unsigned char active=0;
        if(!markers[i])return 0;
        if(enabled && actors[i] && read(ctx,actors[i]+0x10,position,sizeof(position))){
            active=1;
            for(unsigned axis=0;axis<3;++axis)if(!isfinite(position[axis]))active=0;
        }
        if(!write(ctx,markers[i]+0x40,&active,1) ||
           (active && !write(ctx,markers[i]+0x10,position,sizeof(position))))return 0;
    }
    return 1;
}
static inline int jnet_character_attack_ready(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t handler,uint64_t creature,uint64_t eye) {
    uint64_t type=0,value=0,clump=0,items=0,count=0;
    if(!handler || !eye || jnet_character_attack_locked(read,ctx,image,event,creature)<0 ||
       !read(ctx,handler,&type,8) || type!=image+0x66cfe8 ||
       !read(ctx,handler+0x48,&value,8) || value!=creature ||
       !read(ctx,event+0x58,&value,8) || value!=eye ||
       !read(ctx,eye,&type,8) || type!=image+0x672158 ||
       !read(ctx,handler+0x50,&clump,8) || !clump ||
       !read(ctx,clump+8,&items,8) || !items ||
       !read(ctx,clump+0x10,&count,8) || count!=1 ||
       !read(ctx,items,&value,8) || value!=event)return 0;
    for(unsigned i=0;i<2;++i){
        uint64_t rail=0,points=0;uint32_t n=0;float positions[16];
        if(!read(ctx,event+0x88+8*i,&rail,8) || !rail ||
           !read(ctx,rail+8,&points,8) || !points ||
           !read(ctx,rail+0x10,&n,4) || n!=(i?4u:2u) ||
           !read(ctx,points,positions,16*n))return 0;
        for(unsigned j=0;j<4*n;++j)if(!isfinite(positions[j]))return 0;
    }
    return 1;
}
static inline int jnet_character_ride_glow(jnet_character_read read,jnet_character_write write,
        void *ctx,uint64_t dude,const float before[3]) {
    float after[3],glow[2];
    if(!read || !write || !dude || !before || !read(ctx,dude+0x10,after,sizeof(after)) ||
       !read(ctx,dude+0x1448,glow,4) || !read(ctx,dude+0x1450,glow+1,4))return 0;
    int moved=0;
    for(unsigned i=0;i<3;++i){
        if(!isfinite(before[i]) || !isfinite(after[i]))return 0;
        moved|=before[i]!=after[i];
    }
    for(unsigned i=0;i<2;++i)if(!isfinite(glow[i]))return 0;
    if(!moved)return 1;
    for(unsigned i=0;i<2;++i){
        float value=fmaxf(glow[i],1.f);
        if(!write(ctx,dude+0x1448+8*i,&value,4))return 0;
    }
    return 1;
}
static inline int jnet_character_ride_ready(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t creature,int prepared) {
    uint64_t type=0,clump=0,items=0,count=0,value=0,path=0,points=0,storage=0;
    uint32_t point_count=0;float point[4],strength[3];
    if(!read || !event || !creature ||
       !read(ctx,event,&type,8) || type!=image+0x6718b0 ||
       !read(ctx,creature,&type,8) || type!=image+0x5ce668 ||
       !read(ctx,event+0x68,&clump,8) || !clump ||
       !read(ctx,clump+0x10,&count,8) || count!=1 ||
       !read(ctx,clump+8,&items,8) || !items ||
       !read(ctx,items,&value,8) || value!=creature ||
       !read(ctx,creature+0xe8,&path,8) || !path ||
       !read(ctx,path+8,&points,8) || !points ||
       !read(ctx,path+0x10,&point_count,4) || point_count<2 || point_count>1024 ||
       !read(ctx,points,point,sizeof(point)) ||
       !read(ctx,event+0x70,strength,sizeof(strength)))return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(point[i]))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(strength[i]) || strength[i]<=0 || strength[i]>4)return 0;
    if(!read(ctx,event+0x48,&storage,8))return 0;
    if(!prepared)return storage==0;
    unsigned char state[48];
    return storage && !(storage&15) && read(ctx,storage,state,sizeof(state));
}
static inline int jnet_character_cloth_ride_ready(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t first_mesh,unsigned meshes) {
    uint64_t type=0,clump=0,items=0,count=0,mesh=0;
    if(!read || !event || !first_mesh || !meshes || meshes>32 ||
       !read(ctx,event,&type,8) || type!=image+0x679ed8 ||
       !read(ctx,event+0x60,&clump,8) || !clump ||
       !read(ctx,clump+0x10,&count,8) || count!=meshes ||
       !read(ctx,clump+8,&items,8) || !items)return 0;
    for(unsigned i=0;i<meshes;++i)
        if(!read(ctx,items+8*i,&mesh,8) || mesh!=first_mesh+(uint64_t)i*0x10d0)return 0;
    return 1;
}
static inline int jnet_character_glow_ready(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t first_mesh,unsigned meshes,int prepared) {
    uint64_t type=0,clump=0,items=0,count=0,mesh=0,storage=0;
    if(!read || !event || !first_mesh || !meshes || meshes>64 ||
       !read(ctx,event,&type,8) || type!=image+JNET_CHARACTER_GLOW_VTABLE_RVA ||
       !read(ctx,event+0x48,&clump,8) || !clump ||
       !read(ctx,clump+0x10,&count,8) || count!=meshes ||
       !read(ctx,clump+8,&items,8) || !items ||
       !read(ctx,event+0x58,&storage,8))return 0;
    for(unsigned i=0;i<meshes;++i)
        if(!read(ctx,items+8*i,&mesh,8) || mesh!=first_mesh+(uint64_t)i*0x10d0)return 0;
    if(!prepared)return storage==0;
    unsigned char state[32*64];
    return storage && !(storage&15) && read(ctx,storage,state,32*meshes);
}
static inline int jnet_character_color_ready(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t first_mesh,unsigned meshes,int prepared) {
    uint64_t type=0,clump=0,items=0,count=0,mesh=0,storage=0;
    if(!read || !event || !first_mesh || !meshes || meshes>64 ||
       !read(ctx,event,&type,8) || type!=image+0x65bca8 ||
       !read(ctx,event+0x48,&clump,8) || !clump ||
       !read(ctx,clump+0x10,&count,8) || count!=meshes ||
       !read(ctx,clump+8,&items,8) || !items ||
       !read(ctx,event+0x80,&storage,8))return 0;
    for(unsigned i=0;i<meshes;++i)
        if(!read(ctx,items+8*i,&mesh,8) || mesh!=first_mesh+(uint64_t)i*0x10d0)return 0;
    if(!prepared)return storage==0;
    unsigned char state[28*64];
    return storage && !(storage&3) && read(ctx,storage,state,28*meshes);
}
static inline int jnet_character_ride_reset(jnet_character_read read,jnet_character_write write,
        void *ctx,uint64_t image,uint64_t event) {
    uint64_t storage=0,zero=0;unsigned char state[48]={0};
    if(!read || !write || !read(ctx,event+0x48,&storage,8) || !storage || (storage&15) ||
       !read(ctx,image+0x5c1e60,state+16,16) || !read(ctx,image+0x5c1e70,state+32,16))return 0;
    return write(ctx,storage,state,sizeof(state)) && write(ctx,event+0x50,&zero,4) &&
        write(ctx,event+0x58,&zero,8);
}
static inline int jnet_character_animation_ready(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t event,uint64_t creature,uint64_t first_mesh,unsigned meshes,int prepared) {
    uint64_t type=0,clump=0,items=0,count=0,value=0;
    if(!read || !event || !creature || !first_mesh || !meshes || meshes>8 ||
       !read(ctx,event,&type,8) || type!=image+0x670678 ||
       !read(ctx,creature,&type,8) || type!=image+0x5ce668 ||
       !read(ctx,event+0x50,&clump,8) || !clump ||
       !read(ctx,clump+0x10,&count,8) || count!=1 ||
       !read(ctx,clump+8,&items,8) || !items ||
       !read(ctx,items,&value,8) || value!=creature ||
       !read(ctx,event+0x48,&clump,8) || !clump ||
       !read(ctx,clump+0x10,&count,8) || !count || count>8 ||
       !read(ctx,clump+8,&items,8) || !items)return 0;
    for(unsigned i=0;i<count;++i){
        if(!read(ctx,items+8*i,&value,8) || value<first_mesh ||
           (value-first_mesh)%0x10d0 || (value-first_mesh)/0x10d0>=meshes)return 0;
    }
    if(prepared){
        uint32_t matched=0;
        if(!read(ctx,event+0xb0,&value,8) || value!=creature ||
           !read(ctx,event+0xf8,&matched,4) || matched!=count)return 0;
        for(unsigned i=0;i<matched;++i){
            uint64_t expected=0;
            if(!read(ctx,items+8*i,&expected,8) ||
               !read(ctx,event+0xb8+8*i,&value,8) || value!=expected)return 0;
        }
    }
    return 1;
}
static inline int jnet_character_vortex_decay(jnet_character_read read,jnet_character_write write,
        void *ctx,uint64_t dude,float dt) {
    float remaining=0,rate=0;
    if(!read || !write || !isfinite(dt) || dt<0 ||
       !read(ctx,dude+0xf8,&remaining,4) || !isfinite(remaining))return 0;
    if(remaining<=0)return 1;
    if(!read(ctx,dude+0x1c44,&rate,4) || !isfinite(rate) || rate<0)return 0;
    remaining=fmaxf(0,remaining-dt*rate);
    return write(ctx,dude+0xf8,&remaining,4);
}
static inline int jnet_character_teleport_identity(jnet_character_read read,jnet_character_write write,
        void *ctx,uint64_t image,uint64_t events,uint64_t pending,uint32_t *identity) {
    uint32_t count=0,ids[512],candidate=1;uint64_t type=0;unsigned found=0;
    unsigned char flags[2];
    if(!read || !write || !identity || !pending ||
       !read(ctx,events+0x1008,&count,4) || count>512 ||
       !read(ctx,pending,&type,8) || type!=image+0x66fd40 ||
       !read(ctx,pending+8,flags,2) || flags[0] || flags[1])return 0;
    for(unsigned i=0;i<count;++i){
        uint64_t event=0;
        if(!read(ctx,events+8+(uint64_t)i*8,&event,8) || !event ||
           !read(ctx,event+0x10,&ids[i],4))return 0;
        found+=event==pending;
    }
    if(found!=1)return 0;
    for(unsigned i=0;i<count;){
        if(ids[i]==candidate){++candidate;i=0;}else ++i;
    }
    if(!write(ctx,pending+0x10,&candidate,4))return 0;
    *identity=candidate;return 1;
}
static inline int jnet_character_teleport_complete(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t events,uint64_t pending,uint32_t identity,uint64_t creature) {
    uint32_t count=0;
    if(!identity || !read || !read(ctx,events+0x1008,&count,4) || count>512)return -1;
    for(unsigned i=0;i<count;++i){
        uint64_t event=0,type=0;uint32_t id=0;unsigned char flags[2];
        if(!read(ctx,events+8+(uint64_t)i*8,&event,8))return -1;
        if(event!=pending)continue;
        if(!read(ctx,event,&type,8) || !read(ctx,event+0x10,&id,4))return -1;
        if(type!=image+0x66fd40 || id!=identity)return 1;
        if(!read(ctx,event+8,flags,2))return -1;
        if(flags[0])return 0;
        uint32_t request=0;
        if(!creature || !read(ctx,creature+0x1250,&request,4))return -1;
        if(request && request!=identity)return -1;
        return request==0;
    }
    return 1;
}
struct jnet_character_binding {
    uint64_t image,creature,target,generation;
};
static inline int jnet_character_input_sample(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t game,uint64_t barn,unsigned enabled,unsigned fly,
        struct jnet_character_input *out) {
    uint64_t local=0,type=0,camera=0,node=0,input=0,pad=0,channel=0;
    float axes[4]={0},right[4],back[4];
    if(!read || !out || !image || !game || !barn ||
       !read(ctx,game+0x1e0,&local,8) || local!=barn ||
       !read(ctx,barn,&type,8) || type!=image+0x5da340 ||
       !read(ctx,game+0x60,&camera,8) || !camera ||
       !read(ctx,camera+0x30,&node,8) || !node ||
       !read(ctx,game+0x40,&input,8) || !input ||
       !read(ctx,input+8,&pad,8) || !pad ||
       !read(ctx,pad+0x260,&channel,8) ||
       (channel && !read(ctx,channel+0x80,axes,sizeof(axes))) ||
       !read(ctx,node+0x170,right,sizeof(right)) ||
       !read(ctx,node+0x190,back,sizeof(back)))return 0;
    struct jnet_character_input sample={.forward=-axes[1],.strafe=axes[0],
        .enabled=!!enabled,.fly=!!fly};
    if(!isfinite(sample.forward) || !isfinite(sample.strafe))return 0;
    for(unsigned i=0;i<3;++i){
        if(!isfinite(right[i]) || !isfinite(back[i]))return 0;
        sample.camera_right[i]=right[i];sample.camera_forward[i]=-back[i];
    }
    *out=sample;return 1;
}
static inline int jnet_character_binding_valid(jnet_character_read read,void *ctx,
        const struct jnet_character_binding *binding,uint64_t generation) {
    uint64_t type=0,target=0;
    return read && binding && binding->image && binding->creature && binding->target &&
        binding->generation==generation &&
        read(ctx,binding->creature,&type,8) && type==binding->image+0x5ce668 &&
        read(ctx,binding->creature+0x10,&target,8) && target==binding->target;
}
static inline int jnet_character_head_position(jnet_character_read read,void *ctx,
        const struct jnet_character_binding *binding,uint64_t generation,float out[3]) {
    uint64_t segments=0,objects=0,count=0,head=0;
    float position[3];
    if(!out || !jnet_character_binding_valid(read,ctx,binding,generation) ||
       !read(ctx,binding->creature+0x18,&segments,8) || !segments ||
       !read(ctx,segments+0x10,&count,8) || !count || count>16 ||
       !read(ctx,segments+8,&objects,8) || !objects ||
       !read(ctx,objects,&head,8) || !head ||
       !read(ctx,head+0x10,position,sizeof(position)))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i]) || fabsf(position[i])>1000000)return 0;
    memcpy(out,position,sizeof(position));return 1;
}
struct jnet_character_visual_snapshot {
    float matrix[16],fade[2],cloth_fade;
    uint32_t render_state,update_group,blend,render_visits,cloth_expected,cloth_count;
    uint64_t cloth_definition,cloth_collection;
    uint64_t mesh_binding,mesh_resource,mesh_slots,mesh_instance;
    unsigned char mesh_changed,cloth_pending;
    unsigned char enabled[3];
};
static inline int jnet_character_visual_read(jnet_character_read read,void *ctx,
        uint64_t decoration,struct jnet_character_visual_snapshot *out) {
    struct jnet_character_visual_snapshot s={0};
    if(!read || !out || !decoration || decoration>UINT64_MAX-0x10d0)return 1;
    if(!read(ctx,decoration+0x540,s.matrix,sizeof(s.matrix)) ||
       !read(ctx,decoration+0x6e4,&s.render_state,4) ||
       !read(ctx,decoration+0x6f0,s.fade,sizeof(s.fade)) ||
       !read(ctx,decoration+0x704,&s.update_group,4) ||
       !read(ctx,decoration+0x70c,s.enabled,sizeof(s.enabled)) ||
       !read(ctx,decoration+0xf30,&s.blend,4) ||
       !read(ctx,decoration+0x598,&s.render_visits,4) ||
       !read(ctx,decoration+0xf58,&s.cloth_definition,8) ||
       !read(ctx,decoration+0xf40,&s.cloth_collection,8) ||
       !read(ctx,decoration+0xf50,&s.cloth_fade,4))return 1;
    int code=0;
    if(!read(ctx,decoration+0x18,&s.mesh_binding,8) ||
       !read(ctx,decoration+0x30,&s.mesh_changed,1) ||
       !read(ctx,decoration+0xf60,&s.cloth_pending,1))code=4;
    else if(s.mesh_binding && (s.mesh_binding>UINT64_MAX-0x18 ||
       !read(ctx,s.mesh_binding+0x10,&s.mesh_resource,8)))code=5;
    else if(s.mesh_resource && (s.mesh_resource>UINT64_MAX-0xb0 ||
       !read(ctx,s.mesh_resource+0xa8,&s.mesh_slots,8)))code=6;
    else if(s.mesh_slots && (s.mesh_slots>UINT64_MAX-0x20 ||
       !read(ctx,s.mesh_slots+0x18,&s.mesh_instance,8)))code=7;
    if(s.cloth_definition && (s.cloth_definition>UINT64_MAX-0x14 ||
       !read(ctx,s.cloth_definition+0x10,&s.cloth_expected,4)))code=2;
    if(s.cloth_collection && (s.cloth_collection>UINT64_MAX-0x91b4 ||
       !read(ctx,s.cloth_collection+0x91b0,&s.cloth_count,4)))code=3;
    *out=s;return code;
}
static inline int jnet_character_textures_ready(jnet_character_read read,void *ctx,uint64_t mesh) {
    uint32_t count=0;
    if(!read || !mesh || !read(ctx,mesh+0xf20,&count,4) || count>16)return 0;
    for(unsigned i=0;i<count;++i){
        uint32_t type=0;uint64_t binding=0,resource=0,texture=0,gpu=0;
        uint64_t parameter=mesh+(uint64_t)i*0x80;
        if(!read(ctx,parameter+0x798,&type,4))return 0;
        if(type==2 && (!read(ctx,parameter+0x768,&binding,8) || !binding ||
           !read(ctx,binding+0x10,&resource,8) || !resource ||
           !read(ctx,resource+0xc8,&texture,8) || !texture ||
           !read(ctx,texture+0x70,&gpu,8) || !gpu))return 0;
    }
    return 1;
}
static inline int jnet_character_mesh_ready(jnet_character_read read,void *ctx,uint64_t resource) {
    uint64_t slots=0,instance=0;
    return read && resource && resource<=UINT64_MAX-0xb0 &&
        read(ctx,resource+0xa8,&slots,8) && slots && slots<=UINT64_MAX-0x20 &&
        read(ctx,slots+0x18,&instance,8) && instance;
}
static inline int jnet_character_terrain_binding(jnet_character_read read,void *ctx,
        uint64_t game,uint64_t *out) {
    uint64_t terrain=0,resource=0,payload=0,collision=0;
    if(!read || !out || !game ||
       !read(ctx,game+0xd0,&terrain,8) || !terrain ||
       !read(ctx,terrain+0x10,&resource,8) || !resource ||
       !read(ctx,resource+0x10,&payload,8) || !payload ||
       !read(ctx,terrain+0x38,&collision,8) || !collision)return 0;
    *out=terrain;return 1;
}
static inline int jnet_character_target_step(jnet_character_read read,jnet_character_write write,
        void *ctx,const struct jnet_character_binding *binding,uint64_t generation,
        const struct jnet_character_intent *intent,float speed,float dt,
        int (*ground)(void *,const float[3],float *)) {
    struct jnet_character_pose before,next,check;
    if(!write || !jnet_character_binding_valid(read,ctx,binding,generation) ||
       !read(ctx,binding->target+0x10,before.position,sizeof(before.position)) ||
       !read(ctx,binding->target+0x20,before.rotation,sizeof(before.rotation)) ||
       !jnet_character_motion_step(&before,intent,speed,dt,&next))return -1;
    if(ground){
        float height=0;
        if(!ground(ctx,next.position,&height) || !isfinite(height))return -1;
        if(next.position[1]<height)next.position[1]=height;
    }
    if(!memcmp(&before,&next,sizeof(before)))return 0;
    if(!jnet_character_binding_valid(read,ctx,binding,generation))return -1;
    if(write(ctx,binding->target+0x10,next.position,sizeof(next.position)) &&
       write(ctx,binding->target+0x20,next.rotation,sizeof(next.rotation)) &&
       read(ctx,binding->target+0x10,check.position,sizeof(check.position)) &&
       read(ctx,binding->target+0x20,check.rotation,sizeof(check.rotation)) &&
       !memcmp(&check,&next,sizeof(next)))return 0;
    if(!jnet_character_binding_valid(read,ctx,binding,generation) ||
       !write(ctx,binding->target+0x10,before.position,sizeof(before.position)) ||
       !write(ctx,binding->target+0x20,before.rotation,sizeof(before.rotation)) ||
       !read(ctx,binding->target+0x10,check.position,sizeof(check.position)) ||
       !read(ctx,binding->target+0x20,check.rotation,sizeof(check.rotation)) ||
       memcmp(&check,&before,sizeof(before)))return -3;
    return -2;
}
struct jnet_character_pools {
    uint64_t game,image,creatures,decorations,markers,clumps,events;
    uint64_t marker_count,clump_count;
    uint32_t creature_count,decoration_count,event_count;
};
struct jnet_character_budget {
    uint32_t decorations,markers,clumps,events;
};
static inline int jnet_character_pool_snapshot(jnet_character_read read,void *ctx,
        uint64_t image,uint64_t game,struct jnet_character_pools *out) {
    struct jnet_character_pools p={.game=game,.image=image};
    uint64_t type=0;
    if(!read || !out || !image || !game ||
       !read(ctx,game+0x1c8,&p.creatures,8) || !p.creatures ||
       !read(ctx,game+0x138,&p.decorations,8) || !p.decorations ||
       !read(ctx,game+0x1d0,&p.markers,8) || !p.markers ||
       !read(ctx,game+0x148,&p.clumps,8) || !p.clumps ||
       !read(ctx,game+0x250,&p.events,8) || !p.events ||
       !read(ctx,p.creatures,&type,8) || type!=image+0x5ce648 ||
       !read(ctx,p.decorations,&type,8) || type!=image+0x5cef40 ||
       !read(ctx,p.creatures+0x81810,&p.creature_count,4) || p.creature_count>112 ||
       !read(ctx,p.decorations+0x434010,&p.decoration_count,4) || p.decoration_count>1024 ||
       !read(ctx,p.markers+0x14070,&p.marker_count,8) || p.marker_count>1024 ||
       !read(ctx,p.clumps+0x2c080,&p.clump_count,8) || p.clump_count>2048 ||
       !read(ctx,p.events+0x1008,&p.event_count,4) || p.event_count>512)return 0;
    *out=p;return 1;
}
static inline int jnet_character_pool_fits(const struct jnet_character_pools *p,
        const struct jnet_character_budget *b) {
    return p && b && p->creature_count<112 && p->decoration_count<=1024 &&
        p->marker_count<=1024 && p->clump_count<=2048 && p->event_count<=512 &&
        b->decorations<=1024-p->decoration_count && b->markers<=1024-p->marker_count &&
        b->clumps<=2048-p->clump_count && b->events<=512-p->event_count;
}
static inline int jnet_character_pool_unchanged(jnet_character_read read,void *ctx,
        const struct jnet_character_pools *expected) {
    struct jnet_character_pools now;
    return expected && jnet_character_pool_snapshot(read,ctx,expected->image,expected->game,&now) &&
        now.creatures==expected->creatures && now.decorations==expected->decorations &&
        now.markers==expected->markers && now.clumps==expected->clumps && now.events==expected->events &&
        now.creature_count==expected->creature_count && now.decoration_count==expected->decoration_count &&
        now.marker_count==expected->marker_count && now.clump_count==expected->clump_count &&
        now.event_count==expected->event_count;
}
static inline int jnet_character_native_inert(jnet_character_read read,jnet_character_write write,
        void *ctx,const struct jnet_character_pools *before,uint64_t creature) {
    uint32_t count=0;uint64_t type=0;unsigned char check[2];
    const unsigned char inert[2]={1,1};
    if(!before || !read || !write || before->creature_count>=112 ||
       creature!=before->creatures+0x10+(uint64_t)before->creature_count*0x1280 ||
       !read(ctx,before->creatures+0x81810,&count,4) || count!=before->creature_count+1 ||
       !read(ctx,creature,&type,8) || type!=before->image+0x5ce668)return 0;
    return write(ctx,creature+8,inert,2) && read(ctx,creature+8,check,2) && !memcmp(check,inert,2);
}
typedef uint64_t (*jnet_character_allocate)(void *,uint64_t);
static inline int jnet_character_native_reserve(jnet_character_read read,jnet_character_write write,
        void *ctx,uint64_t image,uint64_t game,const struct jnet_character_budget *budget,
        int (*resources_ready)(void *),jnet_character_allocate allocate,
        struct jnet_character_pools *before,uint64_t *creature) {
    struct jnet_character_pools p;
    if(!write || !resources_ready || !allocate || !before || !creature)return -1;
    *creature=0;
    if(!jnet_character_pool_snapshot(read,ctx,image,game,&p) || !jnet_character_pool_fits(&p,budget))return -1;
    if(!resources_ready(ctx))return -2;
    if(!jnet_character_pool_unchanged(read,ctx,&p))return -3;
    uint64_t object=allocate(ctx,p.creatures);
    if(!jnet_character_native_inert(read,write,ctx,&p,object)){
        uint32_t count=0;
        if(object!=p.creatures+0x10+(uint64_t)p.creature_count*0x1280 ||
           !read(ctx,p.creatures+0x81810,&count,4) || count!=p.creature_count+1 ||
           !write(ctx,p.creatures+0x81810,&p.creature_count,4) ||
           !read(ctx,p.creatures+0x81810,&count,4) || count!=p.creature_count)return -5;
        return -4;
    }
    *before=p;*creature=object;return 0;
}
#endif
