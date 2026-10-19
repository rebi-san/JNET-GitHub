#ifndef JC_NATIVE_LIFECYCLE_HOOKS_H
#define JC_NATIVE_LIFECYCLE_HOOKS_H
#include "native_lifecycle_trace.h"
__declspec(dllexport) struct jc_lifecycle_ring JNETLifecycle;
static uint64_t lifecycle_game,lifecycle_barn,lifecycle_image;
static uint32_t lifecycle_frame,lifecycle_started;
static int lifecycle_started_valid;
static struct jc_lifecycle_jobs lifecycle_jobs;
/* No game-memory reads, allocation, logging, or native invocations in entry observers. */
static void lifecycle_emit(unsigned event,const uint64_t args[4],uint64_t caller,uint32_t xmm1) {
    struct jc_lifecycle_ring *r=&JNETLifecycle;
    if (!r->enabled) return;
    if (InterlockedCompareExchange((volatile LONG *)&r->lock,1,0)) {
        InterlockedIncrement((volatile LONG *)&r->dropped); return;
    }
    DWORD thread=GetCurrentThreadId();
    if (event==17 || event==18) {
        int accepted=jc_lifecycle_animation(&lifecycle_jobs,event,args,thread,lifecycle_barn);
        if (accepted<0) InterlockedIncrement((volatile LONG *)&r->dropped);
        if (accepted<=0) goto done;
    }
    if (event==19) {
        unsigned role=jc_lifecycle_role(args[0],lifecycle_game,lifecycle_barn);
        if (role!=5 && role!=6) goto done;
    }
    DWORD tick=GetTickCount();
    if (!lifecycle_started_valid) { lifecycle_started=tick; lifecycle_started_valid=1; }
    if ((DWORD)(tick-lifecycle_started)>=1200000) { InterlockedExchange((volatile LONG *)&r->enabled,0); goto done; }
    uint32_t n=r->head+1;
    if (!n) { InterlockedExchange((volatile LONG *)&r->enabled,0); goto done; }
    struct jc_lifecycle_event *e=&r->events[(n-1)%JC_LIFECYCLE_CAP];
    InterlockedExchange((volatile LONG *)&e->sequence,0);
    e->frame=lifecycle_frame; e->tick=tick; e->thread=thread;
    e->event=event; e->caller=caller>=lifecycle_image && caller-lifecycle_image<JC_IMAGE_SIZE ? (uint32_t)(caller-lifecycle_image) : UINT32_MAX;
    e->roles=0; e->xmm1=xmm1;
    for (unsigned i=0;i<4;++i) {
        e->args[i]=args[i];
        e->roles|=jc_lifecycle_role(args[i],lifecycle_game,lifecycle_barn)<<(4*i);
    }
    InterlockedExchange((volatile LONG *)&e->sequence,(LONG)n);
    InterlockedExchange((volatile LONG *)&r->head,(LONG)n);
 done:
    InterlockedExchange((volatile LONG *)&r->lock,0);
}
__attribute__((used,noinline)) static void lifecycle_entry(const uint64_t *s,unsigned event,uint32_t xmm1) {
    const uint64_t args[]={s[5],s[4],s[3],s[2]};
    lifecycle_emit(event,args,s[8],xmm1);
}
/* Entry RSP%16=8. Save flags, all volatile integer/XMM registers, then reserve
   aligned shadow space. Tail-jump restores the exact original stack, including
   fifth/later arguments; R10/R11 are not scratch across the observer. */
#define JC_LIFECYCLE_HOOK(name,id) \
__attribute__((used)) static void *lifecycle_original_##name; \
__attribute__((naked)) static void lifecycle_hook_##name(void) { \
    __asm__ volatile( \
        "pushfq\n push %rax\n push %rcx\n push %rdx\n push %r8\n push %r9\n push %r10\n push %r11\n" \
        "sub $136,%rsp\n" \
        "movdqu %xmm0,32(%rsp)\n movdqu %xmm1,48(%rsp)\n movdqu %xmm2,64(%rsp)\n" \
        "movdqu %xmm3,80(%rsp)\n movdqu %xmm4,96(%rsp)\n movdqu %xmm5,112(%rsp)\n" \
        "lea 136(%rsp),%rcx\n mov $" #id ",%edx\n mov 48(%rsp),%r8d\n call lifecycle_entry\n" \
        "movdqu 32(%rsp),%xmm0\n movdqu 48(%rsp),%xmm1\n movdqu 64(%rsp),%xmm2\n" \
        "movdqu 80(%rsp),%xmm3\n movdqu 96(%rsp),%xmm4\n movdqu 112(%rsp),%xmm5\n" \
        "add $136,%rsp\n pop %r11\n pop %r10\n pop %r9\n pop %r8\n pop %rdx\n pop %rcx\n pop %rax\n popfq\n" \
        "jmp *lifecycle_original_" #name "(%rip)\n"); \
}
JC_LIFECYCLE_HOOK(DudeConstructor,1)
JC_LIFECYCLE_HOOK(DudeResourceInit,2)
JC_LIFECYCLE_HOOK(DudeCleanup,3)
JC_LIFECYCLE_HOOK(DudeDestructor,4)
JC_LIFECYCLE_HOOK(RemoteReceive,5)
JC_LIFECYCLE_HOOK(RemoteApply,6)
JC_LIFECYCLE_HOOK(PlayerUpdate,7)
JC_LIFECYCLE_HOOK(DudeUpdate,8)
JC_LIFECYCLE_HOOK(DudeSimulation,9)
JC_LIFECYCLE_HOOK(PlayerAppearance,10)
JC_LIFECYCLE_HOOK(ClothAppearance,11)
JC_LIFECYCLE_HOOK(ChestSymbol,12)
JC_LIFECYCLE_HOOK(ChirpSymbol,13)
JC_LIFECYCLE_HOOK(PlayerShout,14)
JC_LIFECYCLE_HOOK(ShoutUpdate,15)
JC_LIFECYCLE_HOOK(RemoteDeparture,16)
JC_LIFECYCLE_HOOK(AnimationSubmit,17)
JC_LIFECYCLE_HOOK(AnimationFinish,18)
JC_LIFECYCLE_HOOK(MeshUpdate,19)
JC_LIFECYCLE_HOOK(DudeClothInit,20)
static int lifecycle_install(unsigned char *image) {
    lifecycle_image=(uintptr_t)image;
#define JC_INSTALL(name) if (MH_CreateHook(image+JC_RVA_##name,(void *)lifecycle_hook_##name,&lifecycle_original_##name)!=MH_OK) return 0;
    JC_INSTALL(DudeConstructor) JC_INSTALL(DudeResourceInit) JC_INSTALL(DudeCleanup) JC_INSTALL(DudeDestructor)
    JC_INSTALL(RemoteReceive) JC_INSTALL(RemoteApply) JC_INSTALL(PlayerUpdate) JC_INSTALL(DudeUpdate)
    JC_INSTALL(DudeSimulation) JC_INSTALL(PlayerAppearance) JC_INSTALL(ClothAppearance)
    JC_INSTALL(ChestSymbol) JC_INSTALL(ChirpSymbol) JC_INSTALL(PlayerShout) JC_INSTALL(ShoutUpdate)
    JC_INSTALL(RemoteDeparture) JC_INSTALL(AnimationSubmit) JC_INSTALL(AnimationFinish) JC_INSTALL(MeshUpdate)
    JC_INSTALL(DudeClothInit)
#undef JC_INSTALL
    return 1;
}
static void lifecycle_boundary(uintptr_t game,unsigned frame,int end) {
    if (!JNETLifecycle.enabled) return;
    if (!end) {
        DWORD error=GetLastError();
        uint64_t barn=0; SIZE_T got=0;
        if (!ReadProcessMemory(GetCurrentProcess(),(void *)(game+0x1e0),&barn,8,&got) || got!=8) barn=0;
        SetLastError(error);
        /* Never wait for another observer while the engine is updating. */
        if (!InterlockedCompareExchange((volatile LONG *)&JNETLifecycle.lock,1,0)) {
            lifecycle_game=game; lifecycle_barn=barn; lifecycle_frame=frame;
            InterlockedExchange((volatile LONG *)&JNETLifecycle.lock,0);
        } else InterlockedIncrement((volatile LONG *)&JNETLifecycle.dropped);
    }
    const uint64_t args[]={game,0,0,0};
    lifecycle_emit(end ? 22 : 21,args,0,0);
}
#endif
