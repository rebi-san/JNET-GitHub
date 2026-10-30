#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define JNET_CHARACTER_WAR_MACHINE 2
#define JNET_CHARACTER_COUNT 7
struct jnet_character_state {
    uint64_t eye_event,game;unsigned initialized,failed,kind,eye_ready,eye_warned,attack_busy,owner;float remote_anger;
    struct {unsigned spotlight;} control;
    struct {uint64_t decorations;unsigned decoration_count;} pools;
    unsigned binding;
};
static struct jnet_character_state jnet_character_instances[3],jnet_character_caches[3][JNET_CHARACTER_COUNT*3];
static struct jnet_character_spec {unsigned mesh_count;struct {const char *mesh;} meshes[2];} jnet_character_specs[8];
static uintptr_t fault_image;static unsigned jnet_character_generation=1,valid=1,draws,mutes,expect_dim;
static unsigned char game[0x200],event[0x200],decorations[0x2200],animation[80];
static int own_read(void *ctx,uint64_t at,void *out,size_t bytes){(void)ctx;memcpy(out,(void *)(uintptr_t)at,bytes);return 1;}
static int jc_local_windows_write(uint64_t at,const void *in,size_t bytes){memcpy((void *)(uintptr_t)at,in,bytes);return 1;}
static int jnet_character_binding_valid(void *read,void *ctx,unsigned *binding,unsigned generation){(void)read;(void)ctx;(void)binding;assert(generation==1);return valid;}
static void message(const char *text){(void)text;}
static void jnet_character_fail(struct jnet_character_state *s){s->failed=1;}
static void volume(uintptr_t sounds,uint64_t handle,float value,float fade){assert(sounds==123 && handle==456 && value==0 && fade==0);++mutes;}
static void draw(uintptr_t e,uintptr_t g){assert(e==(uintptr_t)event && g==(uintptr_t)game);if(expect_dim){
    const unsigned offsets[]={0xcc,0xdc,0x10c,0x11c};
    for(unsigned i=0;i<4;++i){float v=1;memcpy(&v,event+offsets[i],4);assert(v==0);}
    unsigned request=0;memcpy(&request,event+0x7c,4);assert(request==0);
}++draws;}
#include "character_eye_code.h"
int main(void){
    fault_image=(uintptr_t)volume-0x1468e0;jnet_character_eye_original=draw;
    uint64_t sounds=123,handle=456;memcpy(game+0x150,&sounds,8);memcpy(event+0xa4,&handle,8);
    jnet_character_specs[JNET_CHARACTER_WAR_MACHINE]=(struct jnet_character_spec){2,{{"JNETCharacter_P_MechHead"},{"JNETCharacter_P_MechHeadGlowMask"}}};
    for(unsigned i=0;i<2;++i){uint64_t states=(uintptr_t)animation;uint32_t count=1;
        memcpy(decorations+0x10+i*0x10d0+0x588,&states,8);memcpy(decorations+0x10+i*0x10d0+0x590,&count,4);}
    for(unsigned owner=0;owner<3;++owner){
        struct jnet_character_state state={.eye_event=(uintptr_t)event,.game=(uintptr_t)game,.initialized=1,.kind=JNET_CHARACTER_WAR_MACHINE,.control={2},.pools={(uintptr_t)decorations,0}};
        jnet_character_instances[owner]=state;unsigned before=draws;
        jnet_character_eye((uintptr_t)event,(uintptr_t)game);assert(draws==before+1);
        jnet_character_instances[owner].attack_busy=1;
        unsigned native_request=2;memcpy(event+0x7c,&native_request,4);
        jnet_character_eye((uintptr_t)event,(uintptr_t)game);assert(draws==before+2);
        memcpy(&native_request,event+0x7c,4);assert(native_request==2);
        ++before;
        jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*owner]=state;memset(&jnet_character_instances[owner],0,sizeof(state));
        unsigned muted=mutes;jnet_character_eye((uintptr_t)event,(uintptr_t)game);assert(draws==before+1 && mutes==muted+1);
        valid=0;jnet_character_eye((uintptr_t)event,(uintptr_t)game);assert(mutes==muted+1);valid=1;
        memset(&jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*owner],0,sizeof(state));
    }
    jnet_character_eye((uintptr_t)event,(uintptr_t)game);assert(draws==7 && mutes==3);
    jnet_character_instances[0]=(struct jnet_character_state){.eye_event=(uintptr_t)event,.game=(uintptr_t)game,.initialized=1,.kind=JNET_CHARACTER_WAR_MACHINE,.control={1},.pools={(uintptr_t)decorations,0}};
    const unsigned offsets[]={0xcc,0xdc,0x10c,0x11c};float intensity=7;
    for(unsigned i=0;i<4;++i)memcpy(event+offsets[i],&intensity,4);
    expect_dim=1;jnet_character_eye((uintptr_t)event,(uintptr_t)game);expect_dim=0;
    for(unsigned i=0;i<4;++i){float restored=0;memcpy(&restored,event+offsets[i],4);assert(restored==intensity);}
    assert(mutes==4 && !jnet_character_instances[0].failed);
    assert(decorations[0x10+0x10d0+0x70c]==1);
    jnet_character_instances[0].control.spotlight=0;
    jnet_character_eye((uintptr_t)event,(uintptr_t)game);
    assert(decorations[0x10+0x10d0+0x70c]==0);
    unsigned closed=1;memcpy(&closed,event+0x7c,4);assert(closed==0);

    jnet_character_instances[0].owner=1;jnet_character_instances[0].control.spotlight=2;
    jnet_character_instances[0].remote_anger=.8f;
    jnet_character_eye((uintptr_t)event,(uintptr_t)game);
    float anger=0;memcpy(&anger,event+0x88,4);assert(anger==.8f && event[0xa0]==1);
    jnet_character_instances[0].remote_anger=0;
    jnet_character_eye((uintptr_t)event,(uintptr_t)game);
    memcpy(&anger,event+0x88,4);assert(anger==0 && event[0xa0]==0);
    puts("Eye controller: all three active owners draw; cached controllers mute without light submission; invalid bindings refuse; stock events preserved.");
}
