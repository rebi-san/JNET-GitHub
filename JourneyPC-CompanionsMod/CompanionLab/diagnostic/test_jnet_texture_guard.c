#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jnet_texture_guard.h"
__attribute__((naked,used)) static void present(void) {
    __asm__("movq 0x70(%r8),%r10\nmovq %r10,%rax\nretq\n");
}
__attribute__((naked,used)) static void missing(void) {
    __asm__("movq $0xfeed,%rax\nretq\n");
}
static uintptr_t invoke(uintptr_t texture) {
    uintptr_t result;
    register uintptr_t arg __asm__("r8")=texture;
#ifdef __APPLE__
    __asm__ volatile("callq _jnet_texture_guard" : "=a"(result) : "r"(arg) : "r10","cc","memory");
#else
    __asm__ volatile("callq jnet_texture_guard" : "=a"(result) : "r"(arg) : "r10","cc","memory");
#endif
    return result;
}
int main(void) {
    unsigned char texture[0x80]={0};uintptr_t gpu=0x1234;
    jnet_texture_original=(void *)present;jnet_texture_missing=(void *)missing;
    assert(invoke(0)==0xfeed);
    assert(invoke((uintptr_t)texture)==0);
    memcpy(texture+0x70,&gpu,8);
    assert(invoke((uintptr_t)texture)==gpu);
    puts("Texture guard: null object follows native missing-texture branch; valid objects retain native GPU lookup.");
}
