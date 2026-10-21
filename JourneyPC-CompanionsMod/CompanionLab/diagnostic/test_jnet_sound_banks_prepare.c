#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef size_t SIZE_T;
typedef unsigned long DWORD;
#define _WIN32 1
#define JC_IMAGE_SIZE 0x37d0000u
#define MEM_RESERVE 1
#define MEM_COMMIT 2
#define MEM_RELEASE 4
#define PAGE_READWRITE 4
#define PAGE_EXECUTE_READWRITE 0x40
static unsigned char *image;
static unsigned failure,allocations,frees,protects;
static char last_message[200];
static void message(const char *text){snprintf(last_message,sizeof(last_message),"%s",text);}
static DWORD GetLastError(void){return 0;}
static void *GetCurrentProcess(void){return image;}
static int ReadProcessMemory(void *process,const void *source,void *dest,SIZE_T size,SIZE_T *got){
    assert(process==image);if(failure==1)return 0;memcpy(dest,source,size);*got=size;return 1;
}
static void *VirtualAlloc(void *address,SIZE_T size,DWORD type,DWORD access){
    (void)address;assert(size==0x1000 && type==(MEM_RESERVE|MEM_COMMIT) && access==PAGE_READWRITE);
    ++allocations;return image+0x37e0000;
}
static int VirtualFree(void *address,SIZE_T size,DWORD type){
    assert(address==image+0x37e0000 && !size && type==MEM_RELEASE);++frees;return 1;
}
static int VirtualProtect(void *address,SIZE_T size,DWORD access,DWORD *previous){
    assert(address==image+0x2b6000 && size==0x2000);++protects;*previous=0x20;
    return !(failure==7 || (failure==9 && access==0x20 && protects==2));
}
static int FlushInstructionCache(void *process,const void *address,SIZE_T size){
    assert(process==image && address==image+0x2b6000 && size==0x2000);return failure!=8;
}
#include "jnet_sound_banks.h"
#include "sound_bank_code.h"
int main(void){
    image=calloc(1,0x3800000);assert(image);
    for(unsigned trial=0;trial<8;++trial){
        memset(image,0,0x3800000);load_sound_bank_code(image);
        jnet_sound_bank_table=NULL;failure=0;allocations=frees=protects=0;
        if(trial==1)image[JNET_SOUND_BANK_TABLE_RVA]=1;
        if(trial==2)image[JNET_SOUND_BANK_TABLE_RVA+0x28]^=1;
        if(trial==3)image[0x2b77ce]^=1;
        if(trial==4)failure=1;
        if(trial==5)failure=7;
        if(trial==6)failure=8;
        if(trial==7)failure=9;
        unsigned char before[0x2000];memcpy(before,image+0x2b6000,sizeof(before));
        int result=jnet_sound_banks_prepare((uintptr_t)image);
        if(!trial){
            assert(result && allocations==1 && !frees);
            assert(jnet_sound_bank_empty(image+0x37e0000,(uintptr_t)(image+0x37e0000),8));
            assert(jnet_sound_bank_empty(image+JNET_SOUND_BANK_TABLE_RVA,(uintptr_t)(image+JNET_SOUND_BANK_TABLE_RVA),5));
            struct jnet_sound_bank_patch patches[5];
            assert(jnet_sound_bank_plan((uintptr_t)image,(uintptr_t)(image+0x37e0000),patches));
            for(unsigned i=0;i<5;++i)assert(!memcmp(image+patches[i].rva,patches[i].after,patches[i].size));
            assert(jnet_sound_banks_prepare((uintptr_t)image) && allocations==1);
        }else{
            assert(!result && !jnet_sound_bank_table && !memcmp(before,image+0x2b6000,sizeof(before)));
            assert(allocations==frees);
            assert(strstr(last_message,"declined stage="));
        }
    }
    free(image);
    puts("Production sound-bank prepare: original PE slot data accepted, eight self-pointers initialized, idempotence, occupied/corrupt/changed/read-failure refusal and protection/cache rollback passed with mocked Windows APIs.");
}
