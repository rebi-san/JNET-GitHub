#ifndef JC_NATIVE_CHIRP_AUDIO_H
#define JC_NATIVE_CHIRP_AUDIO_H
#include <stdint.h>
#include <string.h>
#include <limits.h>
#define JC_CHIRP_AUDIO_BRANCH_RVA 0x13e760u
static const unsigned char jc_chirp_audio_before[]={0x44,0x88,0x6d,0x90,0x45,0x84,0xff};
static const unsigned char jc_chirp_audio_stub[]={
    0x8b,0x47,0x38,0x48,0x69,0xc0,0x3c,1,0,0,
    0x4c,0x89,0xf2,0x48,0x01,0xc2,0x48,0x81,0xc2,0x34,2,7,0,
    0x48,0x8d,0x4d,0x90,
    0x0f,0xb6,0x02,0x88,0x01,0x48,0xff,0xc2,0x48,0xff,0xc1,0x84,0xc0,0x75,0xf1,
    0x45,0x84,0xff,0xe9,0,0,0,0};
static inline int jc_chirp_audio_plan(uintptr_t image,uintptr_t target,unsigned char site[7],unsigned char *stub) {
    int64_t forward=(int64_t)target-(int64_t)(image+JC_CHIRP_AUDIO_BRANCH_RVA+5);
    int64_t back=(int64_t)(image+JC_CHIRP_AUDIO_BRANCH_RVA+7)-(int64_t)(target+sizeof(jc_chirp_audio_stub));
    if(forward<INT32_MIN || forward>INT32_MAX || back<INT32_MIN || back>INT32_MAX)return 0;
    int32_t relative=(int32_t)forward;site[0]=0xe9;memcpy(site+1,&relative,4);site[5]=site[6]=0x90;
    memcpy(stub,jc_chirp_audio_stub,sizeof(jc_chirp_audio_stub));relative=(int32_t)back;
    memcpy(stub+sizeof(jc_chirp_audio_stub)-4,&relative,4);return 1;
}
#ifdef _WIN32
static int jc_chirp_audio_prepare(uintptr_t image) {
    unsigned char current[7],patch[7];SIZE_T got=0;
    void *site=(void *)(image+JC_CHIRP_AUDIO_BRANCH_RVA);
    if(!ReadProcessMemory(GetCurrentProcess(),site,current,sizeof(current),&got) ||
       got!=sizeof(current) || memcmp(current,jc_chirp_audio_before,sizeof(current)))return 0;
    uintptr_t start=(image+JC_IMAGE_SIZE+0xffffu)&~(uintptr_t)0xffffu;void *stub=NULL;
    for(uintptr_t at=start;at<start+0x40000000u;at+=0x10000u){
        stub=VirtualAlloc((void *)at,0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(stub)break;
    }
    if(!stub)return 0;
    DWORD previous=0,ignored=0;
    if(!jc_chirp_audio_plan(image,(uintptr_t)stub,patch,stub) ||
       !VirtualProtect(stub,0x1000,PAGE_EXECUTE_READ,&ignored) ||
       !FlushInstructionCache(GetCurrentProcess(),stub,sizeof(jc_chirp_audio_stub)) ||
       !VirtualProtect(site,sizeof(current),PAGE_EXECUTE_READWRITE,&previous)){
        VirtualFree(stub,0,MEM_RELEASE);return 0;
    }
    memcpy(site,patch,sizeof(patch));
    int ok=FlushInstructionCache(GetCurrentProcess(),site,sizeof(patch))!=0;
    if(ok)ok=VirtualProtect(site,sizeof(patch),previous,&ignored)!=0;
    if(!ok){
        memcpy(site,current,sizeof(current));FlushInstructionCache(GetCurrentProcess(),site,sizeof(current));
        VirtualProtect(site,sizeof(current),previous,&ignored);VirtualFree(stub,0,MEM_RELEASE);return 0;
    }
    message("[JNET] remote chirp audio preserves already-mapped and original creature sound names");
    return 1;
}
#endif
#endif
