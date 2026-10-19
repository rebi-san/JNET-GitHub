#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include "actor_code.h"
#define JC_SAND_ABI __attribute__((ms_abi))
static unsigned char *image;
static unsigned calls,invalid;
static uintptr_t rejected;
static int own_read(void *ctx,uint64_t address,void *out,size_t n) {
    (void)ctx;memcpy(out,(void *)(uintptr_t)address,n);return 1;
}
static int jc_local_windows_write(uint64_t at,const void *value,size_t n) {
    memcpy((void *)(uintptr_t)at,value,n);return 1;
}
static void message(const char *line) {assert(line && *line);}
#include "traveler_sand_guard.h"
static uintptr_t JC_SAND_ABI vector_set(uintptr_t material,uintptr_t name,uintptr_t value) {
    assert(material && value);++calls;
    const unsigned offsets[]={0x5defa8,0x5defb8,0x5df000,0x5df010,0x5defe0,0x5deff0};
    unsigned found=0;for(unsigned i=0;i<6;++i)found|=name==(uintptr_t)image+offsets[i];
    if(!found){++invalid;rejected=name;}
    return 0;
}
extern void reference_sand_consume(void *,void *,uint64_t);
__asm__(".text\n.globl _reference_sand_consume\n_reference_sand_consume:\n"
    "pushq %rbp\npushq %rbx\npushq %r12\npushq %r13\npushq %r14\npushq %r15\npushq %rsi\npushq %rdi\n"
    "subq $0x300,%rsp\nleaq 0xf8(%rsp),%rbp\nmovq %rdi,%r15\nmovq %rdx,0x98(%rsp)\n"
    "callq *%rsi\naddq $0x300,%rsp\n"
    "popq %rdi\npopq %rsi\npopq %r15\npopq %r14\npopq %r13\npopq %r12\npopq %rbx\npopq %rbp\nretq\n");
static uintptr_t JC_SAND_ABI consume_bridge(uintptr_t renderer,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    assert(a==11 && b==22 && c==33 && d==44);
    reference_sand_consume((void *)renderer,image+0x10e43b,UINT64_C(0x426c0db140410b8d));
    return 55;
}
extern void reference_sand_inline(void *,void *,unsigned);
__asm__(".text\n.globl _reference_sand_inline\n_reference_sand_inline:\n"
    "pushq %rbp\npushq %rbx\npushq %r12\npushq %r13\npushq %r14\npushq %r15\npushq %rsi\npushq %rdi\n"
    "subq $0x908,%rsp\nleaq 0x100(%rsp),%rbp\nmovq %rdi,%rbx\nmovq %rsp,%rdi\nmovb %dl,0x81(%rdi)\n"
    "xorq %r15,%r15\nmovl $0x3408,%r11d\nmovl $2,%r10d\n"
    "leaq 0x800(%rsp),%r8\nleaq 0x840(%rsp),%r9\n"
    "pxor %xmm0,%xmm0\nmovaps %xmm0,%xmm1\nmovaps %xmm0,%xmm2\nmovaps %xmm0,%xmm3\n"
    "movaps %xmm0,%xmm4\nmovaps %xmm0,%xmm5\nmovaps %xmm0,%xmm10\n"
    "movl $0x40000000,%eax\nmovd %eax,%xmm9\n"
    "movdqa %xmm0,0x6d0(%rbp)\nmovdqa %xmm0,0x7f0(%rsp)\nmovdqa %xmm0,0x800(%rsp)\n"
    "movdqa %xmm0,0x810(%rsp)\nmovdqa %xmm0,0x820(%rsp)\nmovdqa %xmm0,0x830(%rsp)\n"
    "movdqa %xmm0,0x840(%rsp)\nmovdqa %xmm0,0x850(%rsp)\nmovdqa %xmm0,0x860(%rsp)\nmovdqa %xmm0,0x870(%rsp)\n"
    "callq *%rsi\naddq $0x908,%rsp\n"
    "popq %rdi\npopq %rsi\npopq %r15\npopq %r14\npopq %r13\npopq %r12\npopq %rbx\npopq %rbp\nretq\n");
static void put(void *p,unsigned offset,uint64_t value) {memcpy((char *)p+offset,&value,8);}
int main(void) {
    image=mmap(NULL,0x700000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);assert(image!=MAP_FAILED);
    load_code(image);image[0x10e5a8]=0xc3;image[0x246125]=0xc3;
    image[0x273200]=0x48;image[0x273201]=0xb8;put(image,0x273202,(uintptr_t)vector_set);
    image[0x27320a]=0xff;image[0x27320b]=0xe0;assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    original_sand_append=(jc_sand_append_fn)(image+0x10f0a0);
    unsigned char *renderer=calloc(1,0xece0);assert(renderer);
    _Alignas(16) unsigned char items[2][8*64];
    _Alignas(16) float position[4]={3.36749f,61.69058f,2,1},effect[4]={0,1,0,0};
    uint64_t fault_value=UINT64_C(0x4276c328405784eb);
    for(unsigned mode=0;mode<3;++mode) {
        memset(renderer,0,0xece0);memset(items,0,sizeof(items));calls=invalid=0;rejected=0;
        for(unsigned kind=8;kind<=9;++kind) {
            put(renderer,0xb310+kind*0x5c8,(uintptr_t)items[kind-8]);
            for(unsigned i=0;i<7;++i) {
                if(mode==1)traveler_sand_append((uintptr_t)renderer,kind,position,effect,1,2,3,4,5,6);
                else original_sand_append((uintptr_t)renderer,kind,position,effect,1,2,3,4,5,6);
            }
        }
        if(mode==2) {
            original_sand_consume=consume_bridge;
            assert(traveler_sand_consume((uintptr_t)renderer,11,22,33,44)==55);
        } else reference_sand_consume(renderer,image+0x10e43b,fault_value);
        if(!mode)assert(calls==14 && invalid==2 && rejected==fault_value);
        else assert(calls==12 && !invalid && traveler_sand_dropped==2);
        if(mode==2)assert(traveler_sand_trimmed==2);
    }
    _Alignas(16) unsigned char inline_items[20*64];
    for(unsigned guarded=0;guarded<2;++guarded) {
        if(guarded) {
            assert(!mprotect(image,0x700000,PROT_READ|PROT_WRITE));
            memcpy(image+0x600000,image+0x245e10,17);
            const unsigned char jump[]={0xff,0x25,0,0,0,0};
            memcpy(image+0x600011,jump,6);put(image,0x600017,(uintptr_t)image+0x245e21);
            memcpy(image+0x245e10,jump,6);put(image,0x245e16,(uintptr_t)traveler_sand_inline);
            original_sand_inline=image+0x600000;sand_inline_resume=image+0x246125;
            assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
        }
        for(unsigned kind=8;kind<=9;++kind)for(unsigned initial=0;initial<=12;++initial) {
            memset(renderer,0,0xece0);memset(inline_items,0xa5,sizeof(inline_items));
            put(renderer,0xb310+kind*0x5c8,(uintptr_t)inline_items);
            memcpy(renderer+0xb318+kind*0x5c8,&initial,4);
            reference_sand_inline(renderer,image+0x245e10,kind-8);
            unsigned count=0;memcpy(&count,renderer+0xb318+kind*0x5c8,4);
            if(!guarded) {
                assert(count==initial+6);
                if(initial==6) {
                    uint64_t overwritten=0;memcpy(&overwritten,inline_items+6*64+0x30,8);
                    assert(overwritten==UINT64_C(0x400000003e000000));
                }
            } else {
                unsigned expected=initial;while(expected<=3)expected+=3;
                assert(count==expected);
                for(unsigned i=6*64;i<sizeof(inline_items);++i)assert(inline_items[i]==0xa5);
            }
        }
    }
    assert(traveler_sand_inline_dropped>0);
    free(renderer);assert(!munmap(image,0x700000));
    puts("Native inline writer reproduced the reported 0x400000003e000000 overwrite; production pre-write guard preserved the buffer boundary for both kinds and starting counts 0-12. Native sand consumer: seven entries reproduce the reported position-valued shader-name pointer; append and consumer guards submit six valid names each, including writers bypassing the append hook. Native consumer and append code executed; material writes intercepted, no Journey runtime.");
}
