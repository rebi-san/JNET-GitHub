#ifndef JNET_TEXTURE_GUARD_H
#define JNET_TEXTURE_GUARD_H
static void *jnet_texture_original __attribute__((used));
static void *jnet_texture_missing __attribute__((used));
#ifdef __APPLE__
#define JNET_TEXTURE_SYMBOL(name) "_" #name
#else
#define JNET_TEXTURE_SYMBOL(name) #name
#endif
__attribute__((naked,used)) static void jnet_texture_guard(void) {
    __asm__("testq %r8,%r8\n"
        "jz 1f\n"
        "jmp *" JNET_TEXTURE_SYMBOL(jnet_texture_original) "(%rip)\n"
        "1: jmp *" JNET_TEXTURE_SYMBOL(jnet_texture_missing) "(%rip)\n");
}
#undef JNET_TEXTURE_SYMBOL
#endif
