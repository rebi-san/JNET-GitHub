#include <assert.h>
#include <stdint.h>
#include <string.h>
struct binding { unsigned creature; };
struct jnet_character_instance { unsigned initialized,failed; uintptr_t game,barn; struct binding binding; };
static struct jnet_character_instance jnet_character_instance={1,0,0x1000,0x2000,{1}};
static unsigned jnet_character_generation=1,writes,calls,valid=1;
static unsigned char memory[0x3000];
static int own_read(void *a,uint64_t b,void*c,size_t d){(void)a;(void)b;(void)c;(void)d;return 1;}
static int jnet_character_binding_valid(int(*r)(void*,uint64_t,void*,size_t),void*c,struct binding*b,unsigned g){(void)r;(void)c;(void)b;assert(g==1);return valid;}
static int jc_local_windows_write(uintptr_t a,const void*p,size_t n){assert(a>=0x20b0 && a+n<=0x50b0);memcpy(memory+(a-0x20b0),p,n);++writes;return 1;}
static void jnet_character_fail(struct jnet_character_instance*i){i->failed=1;}
static void original(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,unsigned char f){assert(a && b && c==3 && d==4 && e==5 && f==6);++calls;}
static void (*jnet_character_body_input_original)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,unsigned char);
static void jnet_character_body_input(uintptr_t dude,uintptr_t game,uintptr_t axes,uintptr_t direction,uintptr_t peer,unsigned char enabled) {
    struct jnet_character_instance *instance=&jnet_character_instance;
    if(instance->initialized && !instance->failed && instance->game==game &&
       dude==instance->barn+0xb0 &&
       jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation)){
        float zero[4]={0};
        if(!jc_local_windows_write(dude+0x20,zero,16) ||
           !jc_local_windows_write(dude+0x34,zero,4) ||
           !jc_local_windows_write(dude+0x3c,zero,4) ||
           !jc_local_windows_write(dude+0x1c00,zero,16))jnet_character_fail(instance);
        return;
    }
    jnet_character_body_input_original(dude,game,axes,direction,peer,enabled);
}
int main(void){memset(memory,0xff,sizeof(memory));jnet_character_body_input_original=original;
jnet_character_body_input(0x20b0,0x1000,3,4,5,6);assert(writes==4 && !calls);
assert(memory[0x20]==0 && memory[0x30]==0xff && memory[0x34]==0 && memory[0x1c00]==0);
jnet_character_body_input(0x3000,0x1000,3,4,5,6);assert(calls==1 && writes==4);
valid=0;jnet_character_body_input(0x20b0,0x1000,3,4,5,6);assert(calls==2 && writes==4);
jnet_character_instance.initialized=0;jnet_character_body_input(0x20b0,0x1000,3,4,5,6);assert(calls==3 && writes==4);}
