#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include "jnet_resource_view.h"
#include "menu_code.h"
static unsigned char object[0x80],slots[48*8];
static unsigned reads,change;
static int read_memory(void *ctx,uint64_t at,void *out,size_t n){
    (void)ctx;++reads;
    if(change && reads==3)object[0x30]^=1;
    uint64_t o=(uintptr_t)object,s=(uintptr_t)slots;
    if((at>=o && at<=o+sizeof(object) && n<=o+sizeof(object)-at) ||
       (at>=s && at<=s+sizeof(slots) && n<=s+sizeof(slots)-at)){memcpy(out,(void *)(uintptr_t)at,n);return 1;}
    return 0;
}
int main(void){
    unsigned char *image=mmap(NULL,0x4000000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_menu_code(image);
    uint64_t (__attribute__((ms_abi)) *native)(void *,uint32_t)=(void *)(image+0x372ad0);
    uint64_t (__attribute__((ms_abi)) *buffer)(void *,uint32_t,unsigned char)=(void *)(image+0x372990);
    unsigned failure;
    for(unsigned flags=0;flags<2;++flags)for(unsigned count=0;count<=8;++count){
        memset(object,0,sizeof(object));uint32_t encoded=count|(flags?0x80000000u:0);memcpy(object+0x30,&encoded,4);
        uint64_t base=(uintptr_t)slots;memcpy(object+0x38,&base,8);
        for(unsigned i=0;i<8;++i){uint64_t value=0x12340000+i;memcpy(slots+i*48+8,&value,8);}
        uint64_t single=0x98765432;memcpy(object+0x40,&single,8);
        for(unsigned index=0;index<=count+1;++index){
            reads=0;uint64_t result=jnet_resource_view(read_memory,NULL,(uintptr_t)object,index,&failure);
            assert(result==native(object,index) && !failure);
            assert(result==buffer(object,index,0));
        }
    }
    uint32_t count=0x04000000;uint64_t bad=0x000006d6025006d5ULL;
    memcpy(object+0x30,&count,4);memcpy(object+0x38,&bad,8);
    assert(!jnet_resource_view(read_memory,NULL,(uintptr_t)object,0x030c06ca,&failure) && failure==2);
    count=0x18455356;bad=0x101;
    memcpy(object+0x30,&count,4);memcpy(object+0x38,&bad,8);
    assert(bad+(uint64_t)0x18455355*48+8==UINT64_C(0x48cffa0f9));
    assert(!jnet_resource_view(read_memory,NULL,(uintptr_t)object,0x18455355,&failure) && failure==2);
    bad=UINT64_MAX-4;memcpy(object+0x38,&bad,8);
    assert(!jnet_resource_view(read_memory,NULL,(uintptr_t)object,1,&failure) && failure==2);
    assert(!jnet_resource_view(read_memory,NULL,UINT64_MAX-8,0,&failure) && failure==1);
    bad=(uintptr_t)slots;memcpy(object+0x38,&bad,8);reads=0;change=1;
    assert(!jnet_resource_view(read_memory,NULL,(uintptr_t)object,0,&failure) && failure==3);
    munmap(image,0x4000000);
    puts("Resource view: original native lookup matched for inline/array/empty/out-of-range and high-bit counts; reported invalid backing/index, address overflow and changed descriptor rejected. Renderer/GPU not exercised.");
}
