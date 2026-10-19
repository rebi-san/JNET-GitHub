/* Standalone Windows fixture. Never opens Journey or installs real hooks. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#undef NDEBUG
#include <assert.h>
#include "target_build.h"
#define MH_OK 0
static unsigned installs;
static int MH_CreateHook(void *at,void *hook,void **original) {
    (void)at; (void)hook; (void)original; ++installs; return MH_OK;
}
#include "native_lifecycle_hooks.h"
__attribute__((used)) static uint64_t abi_seen[24];
__attribute__((used)) static uint64_t abi_vectors[12]={11,12,21,22,31,32,41,42,51,52,61,62};
__attribute__((used)) static void *abi_hook;
__attribute__((naked)) static void abi_target(void) {
    __asm__ volatile(
        "mov %rcx,abi_seen(%rip)\n mov %rdx,abi_seen+8(%rip)\n mov %r8,abi_seen+16(%rip)\n mov %r9,abi_seen+24(%rip)\n"
        "mov %rax,abi_seen+32(%rip)\n mov %r10,abi_seen+40(%rip)\n mov %r11,abi_seen+48(%rip)\n"
        "pushfq\n pop %rax\n mov %rax,abi_seen+56(%rip)\n"
        "mov 40(%rsp),%rax\n mov %rax,abi_seen+64(%rip)\n mov 48(%rsp),%rax\n mov %rax,abi_seen+72(%rip)\n"
        "movdqu %xmm0,abi_seen+80(%rip)\n movdqu %xmm1,abi_seen+96(%rip)\n movdqu %xmm2,abi_seen+112(%rip)\n"
        "movdqu %xmm3,abi_seen+128(%rip)\n movdqu %xmm4,abi_seen+144(%rip)\n movdqu %xmm5,abi_seen+160(%rip)\n"
        "mov $0xface1234,%eax\n ret\n");
}
__attribute__((naked)) static uint64_t abi_invoke(void) {
    __asm__ volatile(
        "sub $56,%rsp\n movq $8,32(%rsp)\n movq $9,40(%rsp)\n"
        "movdqu abi_vectors(%rip),%xmm0\n movdqu abi_vectors+16(%rip),%xmm1\n movdqu abi_vectors+32(%rip),%xmm2\n"
        "movdqu abi_vectors+48(%rip),%xmm3\n movdqu abi_vectors+64(%rip),%xmm4\n movdqu abi_vectors+80(%rip),%xmm5\n"
        "mov $0x200210,%ecx\n mov $0x200210,%edx\n mov $3,%r8d\n mov $4,%r9d\n mov $5,%eax\n mov $6,%r10d\n mov $7,%r11d\n"
        "stc\n call *abi_hook(%rip)\n add $56,%rsp\n ret\n");
}
static void verify(void *hook,unsigned id,void **original) {
    *original=(void *)abi_target; abi_hook=hook;
    uint32_t before=JNETLifecycle.head;
    memset(abi_seen,0,sizeof(abi_seen));
    assert(abi_invoke()==0xface1234);
    assert(abi_seen[0]==0x200210 && abi_seen[1]==0x200210 && abi_seen[2]==3 && abi_seen[3]==4);
    assert(abi_seen[4]==5 && abi_seen[5]==6 && abi_seen[6]==7 && (abi_seen[7]&1));
    assert(abi_seen[8]==8 && abi_seen[9]==9);
    assert(!memcmp(abi_seen+10,abi_vectors,sizeof(abi_vectors)));
    assert(JNETLifecycle.head==before+1);
    const struct jc_lifecycle_event *e=&JNETLifecycle.events[before%JC_LIFECYCLE_CAP];
    assert(e->event==id && e->roles==0x55 && e->xmm1==21 && e->args[0]==0x200210 && e->args[3]==4);
}
int main(void) {
    assert(lifecycle_install((unsigned char *)(uintptr_t)0x140000000) && installs==20);
    lifecycle_game=0x100000; lifecycle_barn=0x200000; lifecycle_frame=12;
    JNETLifecycle.enabled=1;
#define VERIFY(name,id) verify((void *)lifecycle_hook_##name,id,&lifecycle_original_##name)
    VERIFY(DudeConstructor,1); VERIFY(DudeResourceInit,2); VERIFY(DudeCleanup,3); VERIFY(DudeDestructor,4);
    VERIFY(RemoteReceive,5); VERIFY(RemoteApply,6); VERIFY(PlayerUpdate,7); VERIFY(DudeUpdate,8);
    VERIFY(DudeSimulation,9); VERIFY(PlayerAppearance,10); VERIFY(ClothAppearance,11);
    VERIFY(ChestSymbol,12); VERIFY(ChirpSymbol,13); VERIFY(PlayerShout,14); VERIFY(ShoutUpdate,15);
    VERIFY(RemoteDeparture,16); VERIFY(AnimationSubmit,17); VERIFY(AnimationFinish,18); VERIFY(MeshUpdate,19); VERIFY(DudeClothInit,20);
    /* Disabled/full-contention observers must still pass every native argument. */
    JNETLifecycle.enabled=0; abi_hook=(void *)lifecycle_hook_DudeUpdate;
    assert(abi_invoke()==0xface1234 && JNETLifecycle.head==20);
    JNETLifecycle.enabled=1; JNETLifecycle.lock=1;
    assert(abi_invoke()==0xface1234 && JNETLifecycle.head==20 && JNETLifecycle.dropped==1);
    JNETLifecycle.lock=0;
    for (unsigned i=0;i<JC_LIFECYCLE_CAP+4;++i) VERIFY(DudeUpdate,8);
    JNETLifecycle.enabled=0;
    lifecycle_boundary(0,0,0); /* Disabled boundaries must never read the game. */
    puts("20 native entry shims: volatile GPR/XMM, flags, stack arguments, return value, capture, contention and ring wrap passed");
    return 0;
}
