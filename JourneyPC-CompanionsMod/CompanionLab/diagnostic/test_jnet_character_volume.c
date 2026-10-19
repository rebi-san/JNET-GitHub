#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "menu_code.h"
static unsigned mute,calls;
__attribute__((used,noinline,ms_abi)) static void jnet_character_volume_filter(uintptr_t sounds,uint64_t handle,float *volume,float *fade) {
    assert(sounds && (uint32_t)handle==4);
    ++calls;
    if(mute){*volume=0;*fade=0;}
    __asm__ volatile("xor %%ecx,%%ecx\n xor %%edx,%%edx\n xor %%r8d,%%r8d\n xor %%r9d,%%r9d\n xor %%r10d,%%r10d\n xor %%r11d,%%r11d\n pxor %%xmm5,%%xmm5"
        :::"rcx","rdx","r8","r9","r10","r11","xmm5");
}
#include "jnet_character_volume_hook.h"
__attribute__((naked,ms_abi)) static int invoke(uintptr_t sounds,uint64_t handle,float volume,float fade) {
    __asm__ volatile(
        "sub $56,%rsp\n mov %rcx,32(%rsp)\n"
        "mov $0x81,%r8d\n mov $0x91,%r9d\n mov $0xa1,%r10d\n mov $0xb1,%r11d\n"
        "mov $0x1234,%eax\n movd %eax,%xmm5\n call _jnet_character_volume\n"
        "cmp 32(%rsp),%rcx\n jne 1f\n cmp $0x81,%r8\n jne 1f\n cmp $0x91,%r9\n jne 1f\n"
        "cmp $0xa1,%r10\n jne 1f\n cmp $0xb1,%r11\n jne 1f\n"
        "movd %xmm5,%eax\n cmp $0x1234,%eax\n jne 1f\n mov $1,%eax\n jmp 2f\n"
        "1: xor %eax,%eax\n 2: add $56,%rsp\n ret\n");
}
int main(void) {
    size_t size=0x1500000;
    unsigned char *image=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_menu_code(image);
    assert(!mprotect(image,size,PROT_READ|PROT_EXEC));
    jnet_character_volume_original=image+0x1468e0;
    unsigned char sounds[0x1200]={0};
    const size_t slot=4*512;
    uint32_t generation=3;memcpy(sounds+slot+0x214,&generation,4);sounds[slot+0x218]=1;
    for(mute=0;mute<2;++mute){
        assert(invoke((uintptr_t)sounds,((uint64_t)generation<<32)|4,.75f,.25f));
        float volume,fade;memcpy(&volume,sounds+slot+0x1ec,4);memcpy(&fade,sounds+slot+0x1f4,4);
        assert(volume==(mute?0:.75f) && fade==(mute?0:.25f));
    }
    unsigned char saved[sizeof(sounds)];memcpy(saved,sounds,sizeof(sounds));
    assert(invoke((uintptr_t)sounds,((uint64_t)(generation+1)<<32)|4,.5f,.5f));
    assert(!memcmp(saved,sounds,sizeof(sounds)) && calls==3);
    assert(!munmap(image,size));
    puts("Sound-volume native ABI: caller-reused RCX/R8/R9/R10/R11/XMM5 preserved despite filter clobbers; native volume/fade and stale-handle rejection verified.");
    return 0;
}
