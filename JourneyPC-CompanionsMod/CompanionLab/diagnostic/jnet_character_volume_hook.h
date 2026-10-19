#ifndef JNET_CHARACTER_VOLUME_HOOK_H
#define JNET_CHARACTER_VOLUME_HOOK_H
#ifdef __APPLE__
#define JNET_VOLUME_SYMBOL(name) "_" #name
#else
#define JNET_VOLUME_SYMBOL(name) #name
#endif
__attribute__((used)) static void *jnet_character_volume_original;
__attribute__((naked,used)) static void jnet_character_volume(void) {
    __asm__ volatile(
        "pushfq\n push %rax\n push %rcx\n push %rdx\n push %r8\n push %r9\n push %r10\n push %r11\n"
        "sub $136,%rsp\n"
        "movdqu %xmm0,32(%rsp)\n movdqu %xmm1,48(%rsp)\n movdqu %xmm2,64(%rsp)\n"
        "movdqu %xmm3,80(%rsp)\n movdqu %xmm4,96(%rsp)\n movdqu %xmm5,112(%rsp)\n"
        "lea 64(%rsp),%r8\n lea 80(%rsp),%r9\n call " JNET_VOLUME_SYMBOL(jnet_character_volume_filter) "\n"
        "movdqu 32(%rsp),%xmm0\n movdqu 48(%rsp),%xmm1\n movdqu 64(%rsp),%xmm2\n"
        "movdqu 80(%rsp),%xmm3\n movdqu 96(%rsp),%xmm4\n movdqu 112(%rsp),%xmm5\n"
        "add $136,%rsp\n pop %r11\n pop %r10\n pop %r9\n pop %r8\n pop %rdx\n pop %rcx\n pop %rax\n popfq\n"
        "jmp *" JNET_VOLUME_SYMBOL(jnet_character_volume_original) "(%rip)\n");
}
#undef JNET_VOLUME_SYMBOL
#endif
