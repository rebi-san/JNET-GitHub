#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "native_companion_chirp.h"
#include "native_chirp_audio.h"
#include "chirp_audio_code.h"
static int __attribute__((ms_abi)) compare(const char *a,const char *b){return strcmp(a,b);}
static void relative(unsigned char *image,unsigned at,unsigned target){
    int32_t displacement=(int32_t)(target-at-5);image[at]=0xe9;memcpy(image+at+1,&displacement,4);
}
int main(void){
    unsigned char *image=mmap(NULL,0x700000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_chirp_audio_code(image);
    const unsigned char entry[]={0x41,0x55,0x56,0x53,0x55,0x57,0x41,0x56,0x41,0x57,
        0x48,0x81,0xec,0x80,1,0,0,0x48,0x8d,0xac,0x24,0,1,0,0,
        0x49,0x89,0xce,0x48,0x89,0xd7,0x4c,0x89,0x45,0x30,0xc6,0x45,0x90,0,0x41,0xbf,1,0,0,0,0x45,0x31,0xed};
    memcpy(image+0x13e600,entry,sizeof(entry));relative(image,0x13e600+sizeof(entry),0x13e760);
    const unsigned char end[]={0x48,0x8d,0x4d,0x90,0x48,0x8b,0x55,0x30,0xe8,0,0,0,0,
        0x48,0x81,0xc4,0x80,1,0,0,0x41,0x5f,0x41,0x5e,0x5f,0x5d,0x5b,0x5e,0x41,0x5d,0xc3};
    memcpy(image+0x13e87e,end,sizeof(end));int32_t displacement=0x49bac4-(0x13e87e + 13);
    memcpy(image+0x13e87e + 9,&displacement,4);
    image[0x49bac4]=0x48;image[0x49bac5]=0xb8;uintptr_t fn=(uintptr_t)compare;
    memcpy(image+0x49bac6,&fn,8);image[0x49bace]=0xff;image[0x49bacf]=0xe0;
    int (__attribute__((ms_abi)) *select_sound)(void *,void *,const char *)=(void *)(image+0x13e600);
    unsigned char *sounds=calloc(1,0x70400),event[0x70]={0};assert(sounds);
    assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    for(unsigned i=1;i<=22;++i){
        strcpy((char *)sounds+0x70234,jc_chirp_sound_name(i,1));
        assert(!select_sound(sounds,event,""));
        assert(select_sound(sounds,event,jc_chirp_sound_name(i,1))!=0);
    }
    for(unsigned i=1;i<=4;++i){
        strcpy((char *)sounds+0x70234,jc_chirp_sound_name(i,0));
        assert(!select_sound(sounds,event,jc_chirp_sound_name(i,1)));
    }
    assert(!mprotect(image,0x700000,PROT_READ|PROT_WRITE));
    assert(!memcmp(image+JC_CHIRP_AUDIO_BRANCH_RVA,jc_chirp_audio_before,7));
    unsigned char patch[7];
    assert(!jc_chirp_audio_plan((uintptr_t)image,(uintptr_t)image+UINT64_C(0x800000000),patch,image+0x13e500));
    assert(jc_chirp_audio_plan((uintptr_t)image,(uintptr_t)(image+0x13e500),patch,image+0x13e500));
    memcpy(image+JC_CHIRP_AUDIO_BRANCH_RVA,patch,7);
    assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    for(unsigned i=1;i<=22;++i){
        strcpy((char *)sounds+0x70234,jc_chirp_sound_name(i,1));
        assert(!select_sound(sounds,event,jc_chirp_sound_name(i,1)));
    }
    for(unsigned i=1;i<=4;++i){
        strcpy((char *)sounds+0x70234,jc_chirp_sound_name(i,0));
        assert(!select_sound(sounds,event,jc_chirp_sound_name(i,1)));
    }
    free(sounds);assert(!munmap(image,0x700000));
    puts("Pinned native remote chirp selector: reproduced empty-name failure for all 22 transmitted variants; corrected branch preserves every companion/creature sound and all four original Player-to-Player2 mappings. Audio output not exercised.");
}
