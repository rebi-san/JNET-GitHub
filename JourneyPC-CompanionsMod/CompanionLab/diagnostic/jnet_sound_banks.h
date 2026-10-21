#ifndef JNET_SOUND_BANKS_H
#define JNET_SOUND_BANKS_H
#include <stdint.h>
#include <string.h>
#include <limits.h>
#define JNET_SOUND_BANK_CAPACITY 8u
#define JNET_SOUND_BANK_STRIDE 0x30u
#define JNET_SOUND_BANK_TABLE_RVA 0x37c32e0u
static inline int jnet_sound_bank_empty(const unsigned char *table,uintptr_t address,unsigned count) {
    for(unsigned i=0;i<count;++i){
        const unsigned char *slot=table+i*JNET_SOUND_BANK_STRIDE;
        for(unsigned j=0;j<0x28;++j)if(slot[j])return 0;
        uintptr_t owner=0;memcpy(&owner,slot+0x28,sizeof(owner));
        if(owner!=address+i*JNET_SOUND_BANK_STRIDE)return 0;
    }
    return 1;
}
static inline void jnet_sound_bank_initialize(unsigned char *table,unsigned count) {
    memset(table,0,count*JNET_SOUND_BANK_STRIDE);
    for(unsigned i=0;i<count;++i){
        uintptr_t owner=(uintptr_t)(table+i*JNET_SOUND_BANK_STRIDE);
        memcpy(table+i*JNET_SOUND_BANK_STRIDE+0x28,&owner,sizeof(owner));
    }
}
struct jnet_sound_bank_patch {uint32_t rva;unsigned size;unsigned char before[7],after[7];};
static inline int jnet_sound_bank_plan(uintptr_t image,uintptr_t table,struct jnet_sound_bank_patch out[5]) {
    static const struct jnet_sound_bank_patch sites[]={
        {0x2b6ff1,7,{0x48,0x8d,0x2d,0xe8,0xc2,0x50,0x03},{0}},
        {0x2b77ce,7,{0x48,0x8d,0x0d,0x0b,0xbb,0x50,0x03},{0}},
        {0x2b704e,4,{0x41,0x83,0xf9,5},{0}},
        {0x2b77fc,3,{0x83,0xf8,5},{0}},
        {0x2b7c58,3,{0x83,0xf8,5},{0}}
    };
    memcpy(out,sites,sizeof(sites));
    for(unsigned i=0;i<5;++i){
        memcpy(out[i].after,out[i].before,out[i].size);
        if(i<2){
            int64_t delta=(int64_t)table-(int64_t)(image+out[i].rva+7);
            if(delta<INT32_MIN || delta>INT32_MAX)return 0;
            int32_t relative=(int32_t)delta;memcpy(out[i].after+3,&relative,4);
        } else out[i].after[out[i].size-1]=JNET_SOUND_BANK_CAPACITY;
    }
    return 1;
}
#ifdef _WIN32
static void *jnet_sound_bank_table;
static int jnet_sound_bank_decline(unsigned stage,unsigned detail) {
    char line[160];snprintf(line,sizeof(line),"[JNET] sound-bank expansion declined stage=%u detail=%u error=%lu",stage,detail,(unsigned long)GetLastError());
    message(line);return 0;
}
static int jnet_sound_banks_prepare(uintptr_t image) {
    if(jnet_sound_bank_table)return 1;
    struct jnet_sound_bank_patch patches[5];unsigned char current[7],old_table[5*JNET_SOUND_BANK_STRIDE];
    SIZE_T got=0;
    if(!ReadProcessMemory(GetCurrentProcess(),(void *)(image+JNET_SOUND_BANK_TABLE_RVA),old_table,sizeof(old_table),&got) || got!=sizeof(old_table))return jnet_sound_bank_decline(1,0);
    if(!jnet_sound_bank_empty(old_table,image+JNET_SOUND_BANK_TABLE_RVA,5))return jnet_sound_bank_decline(2,0);
    if(!jnet_sound_bank_plan(image,image,patches))return jnet_sound_bank_decline(3,0);
    for(unsigned i=0;i<5;++i)
        if(!ReadProcessMemory(GetCurrentProcess(),(void *)(image+patches[i].rva),current,patches[i].size,&got) ||
           got!=patches[i].size || memcmp(current,patches[i].before,patches[i].size))return jnet_sound_bank_decline(4,i);
    uintptr_t start=(image+JC_IMAGE_SIZE+0xffffu)&~(uintptr_t)0xffffu;
    void *table=NULL;
    for(uintptr_t address=start;address<start+0x40000000u;address+=0x10000u){
        table=VirtualAlloc((void *)address,0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(table)break;
    }
    if(!table)return jnet_sound_bank_decline(5,0);
    jnet_sound_bank_initialize(table,JNET_SOUND_BANK_CAPACITY);
    if(!jnet_sound_bank_plan(image,(uintptr_t)table,patches)){VirtualFree(table,0,MEM_RELEASE);return jnet_sound_bank_decline(6,0);}
    DWORD previous=0,ignored=0;void *code=(void *)(image+0x2b6000);size_t bytes=0x2000;
    if(!VirtualProtect(code,bytes,PAGE_EXECUTE_READWRITE,&previous)){VirtualFree(table,0,MEM_RELEASE);return jnet_sound_bank_decline(7,0);}
    for(unsigned i=0;i<5;++i)memcpy((void *)(image+patches[i].rva),patches[i].after,patches[i].size);
    int ok=FlushInstructionCache(GetCurrentProcess(),code,bytes)!=0;
    if(ok)ok=VirtualProtect(code,bytes,previous,&ignored)!=0;
    if(!ok){
        for(unsigned i=0;i<5;++i)memcpy((void *)(image+patches[i].rva),patches[i].before,patches[i].size);
        FlushInstructionCache(GetCurrentProcess(),code,bytes);
        VirtualProtect(code,bytes,previous,&ignored);
        VirtualFree(table,0,MEM_RELEASE);return jnet_sound_bank_decline(8,0);
    }
    jnet_sound_bank_table=table;
    message("[JNET] sound-bank registry expanded from 5 to 8 before persistent loading; original creature banks retained");
    return 1;
}
#endif
#endif
