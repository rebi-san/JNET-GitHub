#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "jnet_sound_banks.h"
#include "sound_bank_code.h"
static void pointer(unsigned char *p,void *v){memcpy(p,&v,8);}
static void entry(unsigned char *image,unsigned at,unsigned target){
    const unsigned char begin[]={0x41,0x54,0x41,0x55,0x53,0x56,0x55,0x48,0x81,0xec,0x60,1,0,0,0x48,0x89,0xe5,0x48,0x89,0xcb,0x31,0xf6,0xe9};
    memcpy(image+at,begin,sizeof(begin));int32_t relative=(int32_t)(target-at-sizeof(begin)-4);
    memcpy(image+at+sizeof(begin),&relative,4);
}
static void leave(unsigned char *image,unsigned at,const unsigned char *result,unsigned bytes){
    const unsigned char end[]={0x48,0x81,0xc4,0x60,1,0,0,0x5d,0x5e,0x5b,0x41,0x5d,0x41,0x5c,0xc3};
    memcpy(image+at,result,bytes);memcpy(image+at+bytes,end,sizeof(end));
}
int main(void){
    unsigned char *image=mmap(NULL,0x3800000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_sound_bank_code(image);
    unsigned char *original=image+JNET_SOUND_BANK_TABLE_RVA;
    assert(jnet_sound_bank_empty(original,(uintptr_t)original,5));
    unsigned char invalid[5*JNET_SOUND_BANK_STRIDE]={0};
    assert(!jnet_sound_bank_empty(invalid,(uintptr_t)original,5));
    memcpy(invalid,original,sizeof(invalid));invalid[0]=1;
    assert(!jnet_sound_bank_empty(invalid,(uintptr_t)original,5));
    memcpy(invalid,original,sizeof(invalid));invalid[0x18]=1;
    assert(!jnet_sound_bank_empty(invalid,(uintptr_t)original,5));
    memcpy(invalid,original,sizeof(invalid));invalid[0x28]^=1;
    assert(!jnet_sound_bank_empty(invalid,(uintptr_t)original,5));
    const unsigned char zero[]={0x31,0xc0},registered[]={0x48,0x8b,0x83,0x88,0,0,0},found[]={0x4c,0x89,0xd0};
    entry(image,0x2b77a0,0x2b77ce);leave(image,0x2b7801,zero,sizeof(zero));leave(image,0x2b7843,registered,sizeof(registered));
    entry(image,0x2b6fc0,0x2b6ff1);image[0x2b7054]=0xe9;int32_t exit_jump=0x2b7100-0x2b7059;memcpy(image+0x2b7055,&exit_jump,4);leave(image,0x2b7100,zero,sizeof(zero));leave(image,0x2b7064,found,sizeof(found));
    uintptr_t (__attribute__((ms_abi)) *register_bank)(void *)=(void *)(image+0x2b77a0);
    uintptr_t (__attribute__((ms_abi)) *find_bank)(void *)=(void *)(image+0x2b6fc0);
    void (__attribute__((ms_abi)) *release_bank)(void *,void *)=(void *)(image+0x8ed90);
    unsigned char banks[9][0xa0]={{0}},payloads[9][64]={{0}},query[64]={0};
    for(unsigned i=0;i<9;++i){snprintf((char *)payloads[i]+8,16,"Bank%u",i);pointer(banks[i]+0x98,payloads[i]);}
    assert(!mprotect(image,0x2b8000,PROT_READ|PROT_EXEC));
    for(unsigned i=0;i<5;++i)assert(register_bank(banks[i])==(uintptr_t)(image+JNET_SOUND_BANK_TABLE_RVA+i*JNET_SOUND_BANK_STRIDE));
    assert(!register_bank(banks[5]));
    for(unsigned i=0;i<5;++i)release_bank(banks[i]+0x88,(void *)(image+JNET_SOUND_BANK_TABLE_RVA+i*JNET_SOUND_BANK_STRIDE));
    assert(!mprotect(image,0x2b8000,PROT_READ|PROT_WRITE));
    unsigned char *table=image+0x37d0010;memset(table-16,0xa5,16);memset(table+8*JNET_SOUND_BANK_STRIDE,0x5a,16);
    jnet_sound_bank_initialize(table,JNET_SOUND_BANK_CAPACITY);
    assert(jnet_sound_bank_empty(table,(uintptr_t)table,8));
    struct jnet_sound_bank_patch patches[5];
    assert(!jnet_sound_bank_plan((uintptr_t)image,(uintptr_t)image+UINT64_C(0x800000000),patches));
    assert(jnet_sound_bank_plan((uintptr_t)image,(uintptr_t)table,patches));
    for(unsigned i=0;i<5;++i){assert(!memcmp(image+patches[i].rva,patches[i].before,patches[i].size));memcpy(image+patches[i].rva,patches[i].after,patches[i].size);}
    assert(!mprotect(image,0x2b8000,PROT_READ|PROT_EXEC));
    for(unsigned repeat=0;repeat<3;++repeat){
        for(unsigned i=0;i<8;++i){
            assert(register_bank(banks[i])==(uintptr_t)(table+i*JNET_SOUND_BANK_STRIDE));
            strcpy((char *)query+0x18,(char *)payloads[i]+8);
            assert(find_bank(query)==(uintptr_t)(table+i*JNET_SOUND_BANK_STRIDE));
        }
        assert(!register_bank(banks[8]));
        strcpy((char *)query+0x18,"Missing");assert(!find_bank(query));
        for(unsigned i=0;i<8;++i)release_bank(banks[i]+0x88,table+i*JNET_SOUND_BANK_STRIDE);
        assert(jnet_sound_bank_empty(table,(uintptr_t)table,8));
        for(unsigned i=0;i<16;++i){assert(table[-16+(int)i]==0xa5);assert(table[8*JNET_SOUND_BANK_STRIDE+i]==0x5a);}
    }
    assert(!munmap(image,0x3800000));
    puts("Pinned sound-bank initialized table including relocated self-pointers, occupied/corrupt table refusal, registration/lookup/release: stock fifth-slot limit, expanded sixth through eighth slots, capacity detection (overflow branch intercepted), all-bank lookup, repeated release/reuse and boundary canaries passed. Audio playback not exercised.");
}
