#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct binding {uintptr_t creature;};
struct jnet_character_state {
    uintptr_t barn,game;unsigned initialized,failed;
    struct binding binding;
    float saved_dissolve[2];unsigned char saved_visibility[2];
};
static struct jnet_character_state jnet_character_instances[3];
static uintptr_t fault_image=0x10000000;
static unsigned jnet_character_generation=1,valid=1,calls;
static int own_read(void *ctx,uint64_t at,void *out,size_t n){(void)ctx;memcpy(out,(void *)(uintptr_t)at,n);return 1;}
static int jc_local_windows_write(uint64_t at,const void *in,size_t n){memcpy((void *)(uintptr_t)at,in,n);return 1;}
static int jnet_character_binding_valid(void *read,void *ctx,struct binding *b,unsigned generation){(void)read;(void)ctx;assert(b->creature && generation==1);return valid;}
static int jnet_character_vortex_decay(void *read,void *write,void *ctx,uintptr_t dude,float dt){(void)read;(void)write;(void)ctx;assert(dude && dt==.02f);return 1;}
static void jnet_character_write_memory(void){}
static void jnet_character_fail(struct jnet_character_state *s){s->failed=1;}
static void original(uintptr_t dude,uintptr_t game,uintptr_t axes,uintptr_t direction,uintptr_t peer,unsigned char enabled){(void)dude;(void)game;(void)axes;(void)direction;(void)peer;(void)enabled;++calls;}
#include "character_body_code.h"
int main(void){
    static unsigned char barn[0xa000],game[0x100],clock[0x20];
    uintptr_t dude=(uintptr_t)barn+0xb0,type=fault_image+0x5d0380,timer=(uintptr_t)clock;
    float dt=.02f,position[3]={4,5,6};
    memcpy(barn+0xb0,&type,8);memcpy(game+0x68,&timer,8);memcpy(clock+0x10,&dt,4);
    jnet_character_body_input_original=original;
    for(unsigned owner=0;owner<3;++owner){
        struct jnet_character_state *s=&jnet_character_instances[owner];
        *s=(struct jnet_character_state){.barn=(uintptr_t)barn,.game=(uintptr_t)game,.initialized=1,.binding={1},.saved_dissolve={.25f,.75f},.saved_visibility={owner&1,(owner+1)&1}};
        memset(barn+0xb0+0x20,0x3f,16);memcpy(barn+0xc0,position,12);
        jnet_character_body_input(dude,(uintptr_t)game,0,0,0,1);
        assert(!calls && !memcmp(barn+0xb0+0x20,(float[4]){0},16) && !memcmp(barn+0xc0,position,12));
        assert(jnet_character_restore_wayfarer(s));
        assert(!memcmp(barn+0xb0+0x1e00,s->saved_dissolve,4) && !memcmp(barn+0xb0+0x2000,s->saved_dissolve+1,4));
        assert(barn[0xb0+0x9c34]==s->saved_visibility[0] && barn[0xb0+0x9cc4]==s->saved_visibility[1]);
        memset(s,0,sizeof(*s));
    }
    jnet_character_body_input(dude,(uintptr_t)game,0,0,0,1);assert(calls==1);
    puts("Creature body: all three owners suppress Wayfarer input, preserve position, restore exact visibility/dissolve, and retain ordinary Wayfarer input. Native movement mocked.");
}
