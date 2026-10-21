#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "jnet_character.h"
static unsigned jnet_credits,jnet_control_character;
static int jnet_fps_limit,jnet_bindings[3][8],jnet_creature_bindings[JNET_CHARACTER_COUNT][3][7];
static unsigned reads,fallbacks,deny;
static int jnet_menu_read(uintptr_t p,void *out,size_t n){++reads;if(reads==deny)return 0;memcpy(out,(void *)p,n);return 1;}
static int jnet_controls_limit(unsigned d){return d==0?255:d==1?5:16;}
static uintptr_t jnet_setting_original(uintptr_t c,uintptr_t o,uintptr_t n){(void)c;(void)o;(void)n;++fallbacks;return 99;}
#include "setting_fixture.h"
static void check(const char *key,uintptr_t expected,int heap){
    unsigned char object[32]={0};uint64_t length=strlen(key),capacity=heap?63:15;uintptr_t data=(uintptr_t)key,out=0;
    if(heap)memcpy(object,&data,8);else {assert(length<16);memcpy(object,key,length);}
    memcpy(object+16,&length,8);memcpy(object+24,&capacity,8);
    reads=fallbacks=0;
    uintptr_t result=jnet_setting(0,(uintptr_t)&out,(uintptr_t)object);
    assert(reads<=2);
    if(expected){assert(result==(uintptr_t)&out && out==expected && !fallbacks);}
    else assert(result==99 && fallbacks==1);
}
int main(void){
    char key[64];
    for(unsigned k=0;k<JNET_CHARACTER_COUNT;++k)for(unsigned d=0;d<3;++d)for(unsigned a=0;a<7;++a){
        snprintf(key,sizeof(key),"%%JNET_CREATURE_%u_%u_%u%%",k,d,a);
        check(key,jnet_character_is_creature(k)?(uintptr_t)&jnet_creature_bindings[k==JNET_CHARACTER_FLOW_2?JNET_CHARACTER_FLOW_1:k][d][a]:0,1);
    }
    for(unsigned d=0;d<3;++d)for(unsigned a=0;a<8;++a){
        snprintf(key,sizeof(key),"%%JNET_BIND_%u_%u%%",d,a);
        jnet_bindings[d][a]=999;
        check(key,(uintptr_t)&jnet_bindings[d][a],0);assert(!jnet_bindings[d][a]);
        check(key,(uintptr_t)&jnet_bindings[d][a],1);
    }
    check("%JNETCredits%",(uintptr_t)&jnet_credits,0);
    check("%JNETFPS%",(uintptr_t)&jnet_fps_limit,0);
    jnet_control_character=99;check("%JNET_CONTROL_CHARACTER%",(uintptr_t)&jnet_control_character,1);assert(!jnet_control_character);
    const char *invalid[]={"%NativeSetting%","%JNET_BIND_3_0%","%JNET_BIND_0_8%","%JNET_CREATURE_9_0_0%","%JNET_CREATURE_1_3_0%","%JNET_CREATURE_1_0_7%","%JNET_CREATURE_1_0_0%x","%JNET_CREATURE_/_0_0%","abcdefghijklmnopqrstuvwxyz0123456789"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)check(invalid[i],0,1);
    deny=1;check("%JNETCredits%",0,0);deny=2;check("%JNET_CONTROL_CHARACTER%",0,1);
    puts("Setting lookup: all bindings resolve with at most two protected reads; native/invalid/read-failure fallback and value bounds passed.");
    return 0;
}
