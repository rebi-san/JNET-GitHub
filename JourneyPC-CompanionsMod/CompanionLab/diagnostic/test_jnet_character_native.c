#include <assert.h>
#include <stdio.h>
#include "jnet_character_native.h"
struct cell { uint64_t address,value; size_t bytes; };
struct fixture { struct cell cells[20]; unsigned count,allocations; int ready,mutate,fail_inert,fail_rollback; };
static const uint64_t image=0x140000000ULL,game=0x1000,creatures=0x2000,decos=0x100000,markers=0x600000,clumps=0x700000,events=0x800000;
static void put(struct fixture *f,uint64_t address,uint64_t value,size_t bytes) {
    for(unsigned i=0;i<f->count;++i)if(f->cells[i].address==address){f->cells[i].value=value;return;}
    assert(f->count<20);f->cells[f->count++]=(struct cell){address,value,bytes};
}
static int read_cell(void *ctx,uint64_t address,void *out,size_t bytes) {
    struct fixture *f=ctx;
    for(unsigned i=0;i<f->count;++i)if(f->cells[i].address==address && f->cells[i].bytes==bytes){memcpy(out,&f->cells[i].value,bytes);return 1;}
    return 0;
}
static int write_cell(void *ctx,uint64_t address,const void *in,size_t bytes) {
    struct fixture *f=ctx;uint64_t value=0;
    if((f->fail_inert && address==creatures+0x18) ||
       (f->fail_rollback && address==creatures+0x81810))return 0;
    memcpy(&value,in,bytes);put(f,address,value,bytes);return 1;
}
static void setup(struct fixture *f) {
    memset(f,0,sizeof(*f));f->ready=1;
    put(f,game+0x1c8,creatures,8);put(f,game+0x138,decos,8);
    put(f,game+0x1d0,markers,8);put(f,game+0x148,clumps,8);put(f,game+0x250,events,8);
    put(f,creatures,image+0x5ce648,8);put(f,decos,image+0x5cef40,8);
    put(f,creatures+0x81810,0,4);put(f,decos+0x434010,0,4);
    put(f,markers+0x14070,0,8);put(f,clumps+0x2c080,0,8);put(f,events+0x1008,0,4);
    put(f,creatures+0x10,image+0x5ce668,8);
}
static int ready(void *ctx) {
    struct fixture *f=ctx;
    if(f->mutate)put(f,markers+0x14070,1,8);
    return f->ready;
}
static uint64_t allocate(void *ctx,uint64_t barn) {
    struct fixture *f=ctx;assert(barn==creatures);++f->allocations;
    put(f,creatures+0x81810,1,4);return creatures+0x10;
}
static int reserve(struct fixture *f,uint64_t *object) {
    const struct jnet_character_budget budget={1,2,3,1};struct jnet_character_pools before;
    return jnet_character_native_reserve(read_cell,write_cell,f,image,game,&budget,ready,allocate,&before,object);
}
struct motor_fixture { unsigned char bytes[256]; unsigned writes,fail_at; int fail_rest; };
struct input_fixture { unsigned char bytes[0x3000]; };
static int input_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    struct input_fixture *f=ctx;
    if(address>sizeof(f->bytes) || bytes>sizeof(f->bytes)-address)return 0;
    memcpy(out,f->bytes+address,bytes);return 1;
}
struct visual_fixture { unsigned char bytes[0xc000]; uint64_t denied; };
static int visual_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    struct visual_fixture *f=ctx;
    if(address==f->denied || address>sizeof(f->bytes) || bytes>sizeof(f->bytes)-address)return 0;
    memcpy(out,f->bytes+address,bytes);return 1;
}
static int visual_write(void *ctx,uint64_t address,const void *in,size_t bytes) {
    struct visual_fixture *f=ctx;
    if(address==f->denied || address>sizeof(f->bytes) || bytes>sizeof(f->bytes)-address)return 0;
    memcpy(f->bytes+address,in,bytes);return 1;
}
static void test_vortex_and_teleport(void) {
    struct visual_fixture f={0};uint64_t dude=0x100,event=0x6000,barn=0x8000;
    float remaining=1,rate=0.5f;
    memcpy(f.bytes+dude+0xf8,&remaining,4);memcpy(f.bytes+dude+0x1c44,&rate,4);
    for(unsigned i=0;i<120;++i)assert(jnet_character_vortex_decay(visual_read,visual_write,&f,dude,1.f/60));
    memcpy(&remaining,f.bytes+dude+0xf8,4);assert(remaining<0.00001f);
    remaining=1;memcpy(f.bytes+dude+0xf8,&remaining,4);
    assert(jnet_character_vortex_decay(visual_read,visual_write,&f,dude,0));
    memcpy(&remaining,f.bytes+dude+0xf8,4);assert(remaining==1);
    assert(jnet_character_vortex_decay(visual_read,visual_write,&f,dude,10));
    memcpy(&remaining,f.bytes+dude+0xf8,4);assert(remaining==0);
    assert(!jnet_character_vortex_decay(visual_read,visual_write,&f,dude,NAN));
    remaining=1;memcpy(f.bytes+dude+0xf8,&remaining,4);f.denied=dude+0x1c44;
    assert(!jnet_character_vortex_decay(visual_read,visual_write,&f,dude,1));f.denied=0;
    uint32_t count=1,id=42;uint64_t type=image+0x66fd40;
    memcpy(f.bytes+barn+0x1008,&count,4);memcpy(f.bytes+barn+8,&event,8);
    memcpy(f.bytes+event,&type,8);memcpy(f.bytes+event+0x10,&id,4);
    f.bytes[event+8]=f.bytes[event+9]=1;
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==0);
    f.bytes[event+8]=0;
    memcpy(f.bytes+dude+0x1250,&id,4);
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==0);
    uint32_t consumed=0;
    memcpy(f.bytes+dude+0x1250,&consumed,4);
    assert(f.bytes[event+9]==1);
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==1);
    f.denied=dude+0x1250;
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==-1);
    f.denied=0;
    uint32_t other=id+1;memcpy(f.bytes+dude+0x1250,&other,4);
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==-1);
    memcpy(f.bytes+dude+0x1250,&consumed,4);
    f.bytes[event+9]=0;
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==1);
    f.bytes[event+8]=f.bytes[event+9]=1;
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id+1,dude)==1);
    f.denied=event+8;
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==-1);
    f.denied=0;count=0;memcpy(f.bytes+barn+0x1008,&count,4);
    assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==1);
}
static void test_teleport_reuse(void) {
    struct visual_fixture f={0};uint64_t barn=0x8000,dude=0x100;
    unsigned resets=0;
    for(unsigned round=0;round<6;++round){
        uint64_t event=0x6000+round*0x100,type=image+0x66fd40;
        uint32_t count=round+1,id=0;
        memcpy(f.bytes+barn+0x1008,&count,4);
        memcpy(f.bytes+barn+8+round*8,&event,8);
        memcpy(f.bytes+event,&type,8);
        assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,0,dude)==-1);
        assert(jnet_character_teleport_identity(visual_read,visual_write,&f,image,barn,event,&id));
        assert(id==round+1);
        f.bytes[event+9]=1;
        memcpy(f.bytes+dude+0x1250,&id,4);
        for(unsigned frame=0;frame<8;++frame){
            for(unsigned i=0;i<count;++i){
                uint32_t request=0,event_id=0;
                memcpy(&request,f.bytes+dude+0x1250,4);
                memcpy(&event_id,f.bytes+0x6000+i*0x100+0x10,4);
                if(request==event_id){
                    ++resets;memset(f.bytes+dude+0x1250,0,4);
                }
            }
        }
        assert(resets==round+1);
        assert(jnet_character_teleport_complete(visual_read,&f,image,barn,event,id,dude)==1);
        assert(!jnet_character_teleport_identity(visual_read,visual_write,&f,image,barn,event,&id));
    }
    uint64_t event=0x6500;uint32_t id=99;
    f.bytes[event+8]=1;f.denied=event+0x10;
    assert(!jnet_character_teleport_identity(visual_read,visual_write,&f,image,barn,event,&id));
    assert(id==99);
}
static void test_texture_readiness(void) {
    struct visual_fixture f={0};uint64_t mesh=0x100,binding=0x3000,resource=0x3100,texture=0x3300,gpu=0x3400;
    uint32_t count=1,type=2;
    memcpy(f.bytes+mesh+0xf20,&count,4);memcpy(f.bytes+mesh+0x798,&type,4);
    assert(!jnet_character_textures_ready(visual_read,&f,mesh));
    memcpy(f.bytes+mesh+0x768,&binding,8);memcpy(f.bytes+binding+0x10,&resource,8);
    assert(!jnet_character_textures_ready(visual_read,&f,mesh));
    memcpy(f.bytes+resource+0xc8,&texture,8);
    assert(!jnet_character_textures_ready(visual_read,&f,mesh));
    memcpy(f.bytes+texture+0x70,&gpu,8);
    assert(jnet_character_textures_ready(visual_read,&f,mesh));
    memset(f.bytes+resource+0xc8,0,8);
    assert(!jnet_character_textures_ready(visual_read,&f,mesh));
    type=1;memcpy(f.bytes+mesh+0x798,&type,4);
    assert(jnet_character_textures_ready(visual_read,&f,mesh));
    count=17;memcpy(f.bytes+mesh+0xf20,&count,4);
    assert(!jnet_character_textures_ready(visual_read,&f,mesh));
}
static void test_visual_snapshot(void) {
    struct visual_fixture f={0};
    uint64_t d=0x100,definition=0x1800,collection=0x2000;
    uint32_t state=3,group=1,visits=77,expected=2,count=1;
    float matrix[16]={.4f,0,0,0,0,.4f,0,0,0,0,.4f,0,100,200,300,1};
    float fade[2]={200,220},cloth_fade=.75f;
    memcpy(f.bytes+d+0x540,matrix,sizeof(matrix));
    memcpy(f.bytes+d+0x6e4,&state,4);memcpy(f.bytes+d+0x704,&group,4);
    memcpy(f.bytes+d+0x598,&visits,4);memcpy(f.bytes+d+0x6f0,fade,8);
    f.bytes[d+0x70c]=1;f.bytes[d+0x70e]=1;
    memcpy(f.bytes+d+0xf58,&definition,8);memcpy(f.bytes+d+0xf40,&collection,8);
    memcpy(f.bytes+definition+0x10,&expected,4);memcpy(f.bytes+collection+0x91b0,&count,4);
    memcpy(f.bytes+d+0xf50,&cloth_fade,4);
    struct jnet_character_visual_snapshot out={0};
    assert(!jnet_character_visual_read(visual_read,&f,d,&out));
    assert(!memcmp(out.matrix,matrix,sizeof(matrix)) && out.render_state==3 && out.render_visits==77);
    assert(out.enabled[0]==1 && out.enabled[1]==0 && out.enabled[2]==1 && out.update_group==1);
    assert(out.fade[0]==200 && out.fade[1]==220 && out.cloth_fade==.75f);
    assert(out.cloth_count==1 && out.cloth_expected==2 && out.cloth_collection==collection);
    f.denied=definition+0x10;
    assert(jnet_character_visual_read(visual_read,&f,d,&out)==2 && out.cloth_count==1);
    f.denied=collection+0x91b0;
    assert(jnet_character_visual_read(visual_read,&f,d,&out)==3 && out.cloth_expected==2);
    f.denied=d+0x70c;
    struct jnet_character_visual_snapshot saved=out;
    assert(jnet_character_visual_read(visual_read,&f,d,&out)==1 && !memcmp(&saved,&out,sizeof(out)));
    assert(jnet_character_visual_read(visual_read,&f,UINT64_MAX-1,&out)==1);
    f.denied=0;memset(f.bytes+d+0xf58,0,8);memset(f.bytes+d+0xf40,0,8);
    assert(!jnet_character_visual_read(visual_read,&f,d,&out) && !out.cloth_count && !out.cloth_expected);
    uint64_t binding=0x3000,resource=0x3100,slots=0x3200,mesh_instance=0x3300;
    memcpy(f.bytes+d+0x18,&binding,8);memcpy(f.bytes+binding+0x10,&resource,8);
    memcpy(f.bytes+resource+0xa8,&slots,8);memcpy(f.bytes+slots+0x18,&mesh_instance,8);
    f.bytes[d+0x30]=1;f.bytes[d+0xf60]=1;
    assert(!jnet_character_visual_read(visual_read,&f,d,&out));
    assert(out.mesh_binding==binding && out.mesh_resource==resource && out.mesh_slots==slots && out.mesh_instance==mesh_instance);
    assert(out.mesh_changed==1 && out.cloth_pending==1);
    assert(jnet_character_mesh_ready(visual_read,&f,resource));
    memset(f.bytes+slots+0x18,0,8);
    assert(!jnet_character_mesh_ready(visual_read,&f,resource));
    memcpy(f.bytes+slots+0x18,&mesh_instance,8);
    assert(!jnet_character_mesh_ready(visual_read,&f,0));
    assert(!jnet_character_mesh_ready(visual_read,&f,UINT64_MAX-1));
    f.denied=resource+0xa8;assert(!jnet_character_mesh_ready(visual_read,&f,resource));
    f.denied=slots+0x18;assert(!jnet_character_mesh_ready(visual_read,&f,resource));
    f.denied=0;
    const uint64_t denied[]={d+0x18,binding+0x10,resource+0xa8,slots+0x18};
    for(unsigned i=0;i<4;++i){
        f.denied=denied[i];
        assert(jnet_character_visual_read(visual_read,&f,d,&out)==(int)(4+i));
    }
    f.denied=0;binding=UINT64_MAX-1;memcpy(f.bytes+d+0x18,&binding,8);
    assert(jnet_character_visual_read(visual_read,&f,d,&out)==5);
    assert(!memcmp(f.bytes+d+0x540,matrix,sizeof(matrix)));
}
static void test_terrain_binding(void) {
    struct input_fixture f={0};
    uint64_t g=0x100,terrain=0x800,resource=0x1000,payload=0x1800,collision=0x2000,out=99;
    memcpy(f.bytes+g+0xd0,&terrain,8);
    memcpy(f.bytes+terrain+0x10,&resource,8);
    memcpy(f.bytes+resource+0x10,&payload,8);
    memcpy(f.bytes+terrain+0x38,&collision,8);
    assert(jnet_character_terrain_binding(input_read,&f,g,&out) && out==terrain);
    uint64_t zero=0;
    memcpy(f.bytes+g+0xd0,&zero,8);
    memcpy(f.bytes+g+0xe0,&terrain,8);
    out=99;
    assert(!jnet_character_terrain_binding(input_read,&f,g,&out) && out==99);
    memcpy(f.bytes+g+0xd0,&terrain,8);
    memcpy(f.bytes+resource+0x10,&zero,8);
    assert(!jnet_character_terrain_binding(input_read,&f,g,&out) && out==99);
    memcpy(f.bytes+resource+0x10,&payload,8);
    memcpy(f.bytes+terrain+0x38,&zero,8);
    assert(!jnet_character_terrain_binding(input_read,&f,g,&out) && out==99);
}
static void test_input(void) {
    struct input_fixture f={0};
    const uint64_t g=0x100,b=0x400,c=0x800,n=0x1000,in=0x1600,p=0x1800,ch=0x2000;
    uint64_t type=image+0x5da340,zero=0;
    memcpy(f.bytes+g+0x1e0,&b,8);memcpy(f.bytes+b,&type,8);
    memcpy(f.bytes+g+0x60,&c,8);memcpy(f.bytes+c+0x30,&n,8);
    memcpy(f.bytes+g+0x40,&in,8);memcpy(f.bytes+in+8,&p,8);memcpy(f.bytes+p+0x260,&ch,8);
    float axes[4]={.25f,-.75f,0,0},right[4]={1,0,0,0},back[4]={0,-.6f,.8f,0};
    memcpy(f.bytes+ch+0x80,axes,16);memcpy(f.bytes+n+0x170,right,16);memcpy(f.bytes+n+0x190,back,16);
    struct jnet_character_input input={0};
    assert(jnet_character_input_sample(input_read,&f,image,g,b,1,1,&input));
    assert(input.forward==.75f && input.strafe==.25f && input.fly && input.enabled);
    assert(input.camera_forward[1]==.6f && input.camera_forward[2]==-.8f);
    struct jnet_character_input saved=input;
    memcpy(f.bytes+g+0x1e0,&zero,8);
    assert(!jnet_character_input_sample(input_read,&f,image,g,b,1,0,&input));
    assert(!memcmp(&input,&saved,sizeof(input)));
    memcpy(f.bytes+g+0x1e0,&b,8);
    axes[0]=NAN;memcpy(f.bytes+ch+0x80,axes,16);
    assert(!jnet_character_input_sample(input_read,&f,image,g,b,1,0,&input));
    memcpy(f.bytes+p+0x260,&zero,8);
    assert(jnet_character_input_sample(input_read,&f,image,g,b,0,0,&input));
    assert(input.forward==0 && input.strafe==0 && !input.enabled && !input.fly);
}
static int motor_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    struct motor_fixture *f=ctx;
    if(address>sizeof(f->bytes) || bytes>sizeof(f->bytes)-address)return 0;
    memcpy(out,f->bytes+address,bytes);return 1;
}
static int motor_write(void *ctx,uint64_t address,const void *in,size_t bytes) {
    struct motor_fixture *f=ctx;++f->writes;
    if(f->writes==f->fail_at || (f->fail_rest && f->writes>=f->fail_at))return 0;
    if(address>sizeof(f->bytes) || bytes>sizeof(f->bytes)-address)return 0;
    memcpy(f->bytes+address,in,bytes);return 1;
}
static int ground_height(void *ctx,const float position[3],float *height) {
    (void)ctx;*height=position[0]+position[2];return 1;
}
static int ground_unavailable(void *ctx,const float position[3],float *height) {
    (void)ctx;(void)position;(void)height;return 0;
}
static void test_grounded_motor(void) {
    struct motor_fixture f={0};
    const struct jnet_character_binding binding={0x140000000ULL,16,128,9};
    uint64_t type=binding.image+0x5ce668;
    memcpy(f.bytes+16,&type,8);memcpy(f.bytes+32,&binding.target,8);
    float pos[3]={1,3,2},quat[4]={0,0,0,1};
    memcpy(f.bytes+144,pos,12);memcpy(f.bytes+160,quat,16);
    struct jnet_character_intent intent={.direction={.6f,-.8f,0},.strength=1};
    assert(!jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,ground_height));
    memcpy(pos,f.bytes+144,12);
    assert(pos[0]>1 && pos[1]==pos[0]+pos[2]);
    intent.direction[0]=0;intent.direction[1]=1;
    assert(!jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,ground_height));
    memcpy(pos,f.bytes+144,12);assert(pos[1]>pos[0]+pos[2]);
    unsigned char saved[256];memcpy(saved,f.bytes,256);unsigned writes=f.writes;
    assert(jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,ground_unavailable)==-1);
    assert(f.writes==writes && !memcmp(saved,f.bytes,256));
}
static void test_motor(void) {
    struct motor_fixture f={0};
    const struct jnet_character_binding binding={0x140000000ULL,16,128,9};
    uint64_t type=binding.image+0x5ce668;
    memcpy(f.bytes+16,&type,8);memcpy(f.bytes+32,&binding.target,8);
    float pos[3]={1,5000,2},quat[4]={0,0,0,1};
    memcpy(f.bytes+144,pos,12);memcpy(f.bytes+160,quat,16);
    struct jnet_character_intent intent={.direction={0,1,0},.strength=1};
    assert(jnet_character_target_step(motor_read,motor_write,&f,&binding,8,&intent,10,.02f,NULL)==-1 && !f.writes);
    intent.strength=0;
    assert(!jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,NULL) && !f.writes);
    intent.strength=1;
    unsigned char original[256];memcpy(original,f.bytes,sizeof(original));
    f.fail_at=2;
    assert(jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,NULL)==-2);
    assert(!memcmp(original,f.bytes,sizeof(original)));
    f.writes=f.fail_at=0;
    assert(!jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,NULL));
    float moved[3];memcpy(moved,f.bytes+144,12);assert(moved[1]>5000);
    assert(!memcmp(f.bytes,original,144) && !memcmp(f.bytes+176,original+176,80));
    f.writes=0;f.fail_at=2;f.fail_rest=1;
    assert(jnet_character_target_step(motor_read,motor_write,&f,&binding,9,&intent,10,.02f,NULL)==-3);
}
static void test_chirp_head(void) {
    struct input_fixture f={0};
    struct jnet_character_binding binding={image,0x100,0x300,7};
    uint64_t value=image+0x5ce668;memcpy(f.bytes+0x100,&value,8);
    value=0x300;memcpy(f.bytes+0x110,&value,8);
    value=0x400;memcpy(f.bytes+0x118,&value,8);
    value=1;memcpy(f.bytes+0x410,&value,8);
    value=0x500;memcpy(f.bytes+0x408,&value,8);
    value=0x600;memcpy(f.bytes+0x500,&value,8);
    float position[3]={5,7,11},out[3]={0};memcpy(f.bytes+0x610,position,12);
    assert(jnet_character_head_position(input_read,&f,&binding,7,out));
    assert(!memcmp(out,position,12));
    assert(!jnet_character_head_position(input_read,&f,&binding,8,out));
    value=0;memcpy(f.bytes+0x500,&value,8);
    assert(!jnet_character_head_position(input_read,&f,&binding,7,out));
    assert(!memcmp(out,position,12));
}
static void test_animation_preparation(void) {
    struct fixture f={0};uint64_t event=0x1000,creature=0x2000,mesh=0x4000;
    put(&f,event,image+0x670678,8);put(&f,creature,image+0x5ce668,8);
    put(&f,event+0x50,0x3000,8);put(&f,0x3010,1,8);put(&f,0x3008,0x3100,8);
    put(&f,0x3100,creature,8);
    put(&f,event+0x48,0x3200,8);put(&f,0x3210,1,8);put(&f,0x3208,0x3300,8);
    put(&f,0x3300,mesh,8);put(&f,event+0xb0,0,8);put(&f,event+0xf8,0,4);
    assert(jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,0));
    assert(!jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,1));
    put(&f,event+0xb0,creature,8);put(&f,event+0xf8,1,4);put(&f,event+0xb8,mesh,8);
    assert(jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,1));
    put(&f,event+0xb0,creature+16,8);
    assert(!jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,1));
    put(&f,event+0xb0,creature,8);put(&f,event+0xb8,mesh+16,8);
    assert(!jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,1));
    put(&f,0x3300,mesh+0x10d0,8);
    assert(!jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,0));
    put(&f,0x3300,mesh,8);put(&f,0x3010,2,8);
    assert(!jnet_character_animation_ready(read_cell,&f,image,event,creature,mesh,1,0));
}
static void test_cloth_ride_preparation(void) {
    struct visual_fixture f={0};uint64_t event=0x100,mesh=0x3000;
    const uint64_t addresses[]={event,event+0x60,0x2010,0x2008,0x2100};
    const uint64_t values[]={image+0x679ed8,0x2000,1,0x2100,mesh};
    for(unsigned i=0;i<5;++i)memcpy(f.bytes+addresses[i],&values[i],8);
    assert(jnet_character_cloth_ride_ready(visual_read,&f,image,event,mesh,1));
    assert(!jnet_character_cloth_ride_ready(visual_read,&f,image,event,mesh,2));
    assert(!jnet_character_cloth_ride_ready(visual_read,&f,image,event,mesh+16,1));
    f.denied=0x2100;
    assert(!jnet_character_cloth_ride_ready(visual_read,&f,image,event,mesh,1));
}
static void test_glow_preparation(void) {
    struct visual_fixture f={0};uint64_t event=0x100,mesh=0x3000,value;
    const uint64_t addresses[]={event,event+0x48,0x2010,0x2008,0x2100};
    const uint64_t values[]={image+0x664168,0x2000,1,0x2100,mesh};
    for(unsigned i=0;i<5;++i)memcpy(f.bytes+addresses[i],&values[i],8);
    value=image+0x664160;memcpy(f.bytes+event,&value,8);
    assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,1,0));
    value=image+0x664168;memcpy(f.bytes+event,&value,8);
    assert(jnet_character_glow_ready(visual_read,&f,image,event,mesh,1,0));
    assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,1,1));
    value=0x2400;memcpy(f.bytes+event+0x58,&value,8);
    assert(jnet_character_glow_ready(visual_read,&f,image,event,mesh,1,1));
    assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,1,0));
    assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,2,1));
    assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh+16,1,1));
    f.denied=value;assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,1,1));
    f.denied=0;
    assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,65,1));
    for(unsigned count=1;count<=64;++count) {
        value=count;memcpy(f.bytes+0x2010,&value,8);
        for(unsigned i=0;i<count;++i){value=mesh+(uint64_t)i*0x10d0;memcpy(f.bytes+0x2100+i*8,&value,8);}
        assert(jnet_character_glow_ready(visual_read,&f,image,event,mesh,count,1));
        value=0;memcpy(f.bytes+event+0x58,&value,8);
        assert(!jnet_character_glow_ready(visual_read,&f,image,event,mesh,count,1));
        value=0x2400;memcpy(f.bytes+event+0x58,&value,8);
    }

}
static void test_color_preparation(void) {
    struct visual_fixture f={0};uint64_t event=0x100,mesh=0x3000,value;
    const uint64_t addresses[]={event,event+0x48,0x2010,0x2008,0x2100};
    const uint64_t values[]={image+0x65bca8,0x2000,1,0x2100,mesh};
    for(unsigned i=0;i<5;++i)memcpy(f.bytes+addresses[i],&values[i],8);
    assert(jnet_character_color_ready(visual_read,&f,image,event,mesh,1,0));
    assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh,1,1));
    value=0x2400;memcpy(f.bytes+event+0x80,&value,8);
    assert(jnet_character_color_ready(visual_read,&f,image,event,mesh,1,1));
    assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh,1,0));
    assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh,2,1));
    assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh+16,1,1));
    f.denied=value;assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh,1,1));
    f.denied=0;value=0x2401;memcpy(f.bytes+event+0x80,&value,8);
    assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh,1,1));
    value=image+0x664168;memcpy(f.bytes+event,&value,8);
    assert(!jnet_character_color_ready(visual_read,&f,image,event,mesh,1,1));
}
static void test_ride_preparation(void) {
    struct visual_fixture f={0};uint64_t event=0x100,creature=0x1000,value;
    const uint64_t addresses[]={event,creature,event+0x68,0x2010,0x2008,0x2100,creature+0xe8,0x2208};
    const uint64_t values[]={image+0x6718b0,image+0x5ce668,0x2000,1,0x2100,creature,0x2200,0x2300};
    for(unsigned i=0;i<8;++i)memcpy(f.bytes+addresses[i],&values[i],8);
    uint32_t count=2;memcpy(f.bytes+0x2210,&count,4);
    float strengths[3]={1.5f,2,2};memcpy(f.bytes+event+0x70,strengths,12);
    assert(jnet_character_ride_ready(visual_read,&f,image,event,creature,0));
    assert(!jnet_character_ride_ready(visual_read,&f,image,event,creature,1));
    value=0x2400;memcpy(f.bytes+event+0x48,&value,8);
    assert(jnet_character_ride_ready(visual_read,&f,image,event,creature,1));
    assert(!jnet_character_ride_ready(visual_read,&f,image,event,creature,0));
    f.denied=value;assert(!jnet_character_ride_ready(visual_read,&f,image,event,creature,1));f.denied=0;
    count=1;memcpy(f.bytes+0x2210,&count,4);
    assert(!jnet_character_ride_ready(visual_read,&f,image,event,creature,1));
    count=2;memcpy(f.bytes+0x2210,&count,4);
    strengths[1]=NAN;memcpy(f.bytes+event+0x70,strengths,12);
    assert(!jnet_character_ride_ready(visual_read,&f,image,event,creature,1));
}
static void test_attack_state_and_targets(void) {
    struct visual_fixture f={0};uint64_t event=0x100,creature=0x1000;
    const uint64_t addresses[]={event,event+0x48,event+0x60,0x2008,0x2010,0x2100,0x2108,0x2110};
    const uint64_t values[]={image+0x66ce70,creature,0x2000,0x2100,3,0x3000,0x3100,0x3200};
    for(unsigned i=0;i<8;++i)memcpy(f.bytes+addresses[i],&values[i],8);
    uint32_t phase=2;memcpy(f.bytes+event+0xac,&phase,4);
    assert(jnet_character_attack_locked(visual_read,&f,image,0,creature)==0);
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==0);
    f.bytes[event+8]=f.bytes[event+9]=1;
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==1);
    f.bytes[event+8]=0;
    for(phase=0;phase<3;++phase){
        memcpy(f.bytes+event+0xac,&phase,4);
        assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==(phase<2));
    }
    memcpy(f.bytes+event+0xac,&phase,4);
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==-1);
    phase=0;memcpy(f.bytes+event+0xac,&phase,4);f.bytes[event+9]=0;
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==0);
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature+16)==-1);
    f.denied=event+0xac;
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==0);
    f.bytes[event+9]=1;
    assert(jnet_character_attack_locked(visual_read,&f,image,event,creature)==-1);f.denied=0;
    uint64_t actors[3]={0x4000,0x4100,0};float position[3]={5,6,7};
    memcpy(f.bytes+actors[0]+0x10,position,sizeof(position));
    memcpy(f.bytes+actors[1]+0x10,position,sizeof(position));
    assert(jnet_character_attack_targets(visual_read,visual_write,&f,event,actors,1));
    assert(f.bytes[0x3040] && f.bytes[0x3140] && !f.bytes[0x3240]);
    assert(!memcmp(f.bytes+0x3010,position,sizeof(position)));
    position[1]=NAN;memcpy(f.bytes+actors[1]+0x10,position,sizeof(position));
    assert(jnet_character_attack_targets(visual_read,visual_write,&f,event,actors,1));
    assert(f.bytes[0x3040] && !f.bytes[0x3140]);
    assert(jnet_character_attack_targets(visual_read,visual_write,&f,event,actors,0));
    assert(!f.bytes[0x3040] && !f.bytes[0x3140]);
    f.denied=0x3040;
    assert(!jnet_character_attack_targets(visual_read,visual_write,&f,event,actors,1));f.denied=0;
    uint64_t handler=0x5000,eye=0x5100;
    const uint64_t ready_addresses[]={handler,handler+0x48,handler+0x50,event+0x58,eye,0x5208,0x5210,0x5300,event+0x88,event+0x90,0x5408,0x5508};
    const uint64_t ready_values[]={image+0x66cfe8,creature,0x5200,eye,image+0x672158,0x5300,1,event,0x5400,0x5500,0x5600,0x5700};
    for(unsigned i=0;i<12;++i)memcpy(f.bytes+ready_addresses[i],&ready_values[i],8);
    uint32_t n=2;memcpy(f.bytes+0x5410,&n,4);n=4;memcpy(f.bytes+0x5510,&n,4);
    assert(jnet_character_attack_ready(visual_read,&f,image,event,handler,creature,eye));
    assert(!jnet_character_attack_ready(visual_read,&f,image,event,handler,creature,eye+16));
    n=3;memcpy(f.bytes+0x5510,&n,4);
    assert(!jnet_character_attack_ready(visual_read,&f,image,event,handler,creature,eye));
    n=4;memcpy(f.bytes+0x5510,&n,4);float invalid=NAN;memcpy(f.bytes+0x5700,&invalid,4);
    assert(!jnet_character_attack_ready(visual_read,&f,image,event,handler,creature,eye));
}
int main(void) {
    test_attack_state_and_targets();
    {
        struct visual_fixture f={0};uint64_t dude=0x100;
        float before[3]={1,2,3},after[3]={1,2.1f,3},value=0;
        memcpy(f.bytes+dude+0x10,before,sizeof(before));
        assert(jnet_character_ride_glow(visual_read,visual_write,&f,dude,before));
        memcpy(&value,f.bytes+dude+0x1448,4);assert(value==0);
        memcpy(f.bytes+dude+0x10,after,sizeof(after));
        assert(jnet_character_ride_glow(visual_read,visual_write,&f,dude,before));
        memcpy(&value,f.bytes+dude+0x1448,4);assert(value==1);
        memcpy(&value,f.bytes+dude+0x1450,4);assert(value==1);
        after[1]=NAN;memcpy(f.bytes+dude+0x10,after,sizeof(after));
        assert(!jnet_character_ride_glow(visual_read,visual_write,&f,dude,before));
        f.denied=dude+0x10;
        assert(!jnet_character_ride_glow(visual_read,visual_write,&f,dude,before));
    }
    test_ride_preparation();test_cloth_ride_preparation();test_glow_preparation();test_color_preparation();
    test_animation_preparation();
    test_vortex_and_teleport();
    test_teleport_reuse();
    test_chirp_head();
    test_visual_snapshot();test_texture_readiness();
    test_terrain_binding();
    test_input();
    test_motor();
    test_grounded_motor();
    struct fixture f;uint64_t object=99;uint32_t count;unsigned char flags[2];
    setup(&f);assert(reserve(&f,&object)==0 && object==creatures+0x10 && f.allocations==1);
    assert(read_cell(&f,object+8,flags,2) && flags[0]==1 && flags[1]==1);
    setup(&f);f.ready=0;assert(reserve(&f,&object)==-2 && !object && !f.allocations);
    setup(&f);f.mutate=1;assert(reserve(&f,&object)==-3 && !object && !f.allocations);
    const uint64_t addresses[]={creatures+0x81810,decos+0x434010,markers+0x14070,clumps+0x2c080,events+0x1008};
    const uint64_t capacities[]={112,1024,1024,2048,512};
    for(unsigned i=0;i<5;++i){
        setup(&f);put(&f,addresses[i],capacities[i],i==2 || i==3 ? 8:4);
        assert(reserve(&f,&object)==-1 && !object && !f.allocations);
        put(&f,addresses[i],capacities[i]+1,i==2 || i==3 ? 8:4);
        assert(reserve(&f,&object)==-1 && !object && !f.allocations);
    }
    setup(&f);f.fail_inert=1;assert(reserve(&f,&object)==-4 && !object);
    assert(read_cell(&f,creatures+0x81810,&count,4) && !count);
    setup(&f);f.fail_inert=f.fail_rollback=1;assert(reserve(&f,&object)==-5 && !object);
    assert(read_cell(&f,creatures+0x81810,&count,4) && count==1);
    setup(&f);put(&f,creatures+0x10,0,8);assert(reserve(&f,&object)==-4 && !object);
    puts("Native character reservation: capacity, missing resources, stale snapshots, inert state and failed rollback passed; fixture only.");
}
