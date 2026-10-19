#include "native_scarf_growth.h"
#include "actor_code.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
static unsigned char binding[32],payload[512];
static unsigned stops;
static int read_memory(void *ctx,uint64_t at,void *out,size_t n){(void)ctx;memcpy(out,(void *)(uintptr_t)at,n);return 1;}
static int write_memory(void *ctx,uint64_t at,const void *data,size_t n){(void)ctx;memcpy((void *)(uintptr_t)at,data,n);return 1;}
static void put(void *p,unsigned offset,uint64_t value){memcpy((unsigned char *)p+offset,&value,8);}
static void JC_GROWTH_ABI bind_effect(uint64_t resources,uint64_t holder,const char *name){
    assert(resources && !strcmp(name,"CapeGrow"));put((void *)(uintptr_t)holder,0,(uintptr_t)binding);
}
static unsigned JC_GROWTH_ABI sound_index(uint64_t sounds,const char *name){assert(sounds && name);return 0;}
static void JC_GROWTH_ABI stop_sound(uint64_t sounds,uint64_t handle){assert(sounds && handle==2);++stops;}
static void hook(unsigned char *image,unsigned rva,uintptr_t target){
    image[rva]=0x48;image[rva+1]=0xb8;memcpy(image+rva+2,&target,8);image[rva+10]=0xff;image[rva+11]=0xe0;
}
int main(void){
    unsigned char *image=mmap(NULL,0x740000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_code(image);
    uint32_t maximum=10;memcpy(image+0x731fb0,&maximum,4);
    hook(image,0x112fc0,(uintptr_t)bind_effect);hook(image,0x147fc0,(uintptr_t)sound_index);
    hook(image,0x146870,(uintptr_t)stop_sound);assert(!mprotect(image,0x690000,PROT_READ|PROT_EXEC));
    unsigned char game[0x200]={0};
    unsigned char *pool=calloc(1,0x20020),*sounds=calloc(1,0x7c7b8),*actor=calloc(1,0x14640);
    assert(pool && sounds && actor);put(game,0x140,(uintptr_t)pool);put(game,0x150,(uintptr_t)sounds);
    put(game,0xb0,(uintptr_t)binding);put(binding,0x10,(uintptr_t)payload);
    float position[4]={1,2,3,1},direction[4]={0,1,0,0};
    memcpy(actor+0x2290,position,16);memcpy(actor+0x14270,direction,16);
    struct jc_scarf_growth g={0};
    for(unsigned cycle=0;cycle<7;++cycle){
        assert(jc_scarf_growth_prepare(read_memory,NULL,(uintptr_t)image,(uintptr_t)game,(uintptr_t)actor,&g));
        uint32_t count;memcpy(&count,pool+0x20010,4);assert(count==1);
        assert(jc_scarf_growth_step(read_memory,write_memory,NULL,&g,.9f));
        assert(pool[0x10+0x94]==1 && !memcmp(pool+0x10+0x30,position,16));
        assert(!memcmp(pool+0x10+0x40,direction,16));
        assert(jc_scarf_growth_step(read_memory,write_memory,NULL,&g,0));assert(!pool[0x10+0x94]);
        assert(jc_scarf_growth_stop(read_memory,write_memory,NULL,&g));
    }
    assert(!jc_scarf_growth_step(read_memory,write_memory,NULL,&g,NAN));
    g.active=1;put(g.native,0x18,2);assert(jc_scarf_growth_stop(read_memory,write_memory,NULL,&g));assert(stops==1);
    _Alignas(16) unsigned char first[32],second[32];
    memcpy(first,g.native,32);memcpy(second,g.native,32);
    float charged[2]={1,1};memcpy(first,charged,8);memcpy(second,charged,8);
    struct jc_scarf_arrival a={.actor=(uintptr_t)actor,.generation=1},b=a;
    assert(jc_scarf_arrival_apply(read_memory,write_memory,NULL,(uintptr_t)image,(uintptr_t)game,(uintptr_t)actor,(uintptr_t)first,&a));
    assert(!memcmp(first,(float[2]){0},8) && !memcmp(first+8,g.native+8,24));
    assert(!memcmp(second,charged,8));
    assert(jc_scarf_arrival_apply(read_memory,write_memory,NULL,(uintptr_t)image,(uintptr_t)game,(uintptr_t)actor,(uintptr_t)second,&b));
    memcpy(first,charged,8);
    assert(jc_scarf_arrival_apply(read_memory,write_memory,NULL,(uintptr_t)image,(uintptr_t)game,(uintptr_t)actor,(uintptr_t)first,&a));
    assert(!memcmp(first,charged,8));++a.generation;
    assert(jc_scarf_arrival_apply(read_memory,write_memory,NULL,(uintptr_t)image,(uintptr_t)game,(uintptr_t)actor,(uintptr_t)first,&a));
    assert(!memcmp(first,(float[2]){0},8));
    memcpy(actor+0x68,&maximum,4);memcpy(actor+0x6c,&maximum,4);
    ((void (JC_GROWTH_ABI *)(uint64_t,float,void *,uint64_t,uint64_t,uint64_t))(image+0x24c010))
        ((uintptr_t)actor,1.f/60,first,0,0,(uintptr_t)sounds);
    float grown;memcpy(&grown,first+4,4);assert(grown>0 && grown<1 && pool[0x10+0x94]);
    puts("Native scarf update: preserved emitter and sound arguments, per-identity reset, gradual length growth and CapeGrow activation passed.");
    put(binding,0x10,0);assert(!jc_scarf_growth_step(read_memory,write_memory,NULL,&g,.5f));
    assert(jc_scarf_growth_stop(read_memory,write_memory,NULL,&g));
    put(binding,0x10,(uintptr_t)payload);uint32_t full=512;memcpy(pool+0x20010,&full,4);
    struct jc_scarf_growth other={0};
    assert(!jc_scarf_growth_prepare(read_memory,NULL,(uintptr_t)image,(uintptr_t)game,(uintptr_t)actor,&other));
    free(pool);free(sounds);free(actor);assert(!munmap(image,0x740000));
    puts("Native CapeGrow: seven admissions reuse one emitter; position, direction, enable, stop, capacity and ownership checks passed. Audio calls intercepted.");
    return 0;
}
