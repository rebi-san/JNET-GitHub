#include "native_clone_pool.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x5000];
static const uint64_t base=0x100000, payload=0x100100, table=0x101000;
static const uint64_t meshes[2]={0x102000,0x103000};
static int reads, deny, change, head_reads;
static void put(uint64_t p,uint64_t value) { memcpy(memory+(p-base),&value,8); }
static int read_memory(void *ctx,uint64_t p,void *out,size_t n) {
    (void)ctx; ++reads; assert(reads<=140);
    if (deny || p<base || p-base>sizeof(memory) || n>sizeof(memory)-(p-base)) return 0;
    if (change && p==table && head_reads++==1) put(table,0);
    memcpy(out,memory+(p-base),n); return 1;
}
static void fixture(uint64_t count) {
    memset(memory,0,sizeof(memory)); reads=deny=change=head_reads=0;
    put(payload+0xa8,table); put(payload+0xb0,count); put(payload+0xd8,count);
    for (int j=0;j<2;++j) { put(meshes[j]+0x30,table+j*0x20); put(table+j*0x20,meshes[j]+0x30); }
}
int main(void) {
    struct jc_pool_result r;
    fixture(2); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==0 && r.slots==2 && r.capacity==2 && r.occupied==2 && r.free_slots==0);
    fixture(4); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==0 && r.occupied==2 && r.free_slots==2);
    fixture(64); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==0 && r.free_slots==62);
    fixture(65); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==4 && reads==3 && !r.slots);
    fixture(0); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==4);
    fixture(4); put(payload+0xd8,2); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==4);
    fixture(4); put(payload+0xa8,UINT64_MAX); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==4);
    fixture(4); put(meshes[1]+0x30,table); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==2);
    fixture(4); put(meshes[1]+0x30,table+1); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==2);
    fixture(4); put(table,0); assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==2);
    fixture(4); deny=1; assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==1 && !r.slots);
    fixture(4); change=1; assert(jc_clone_pool(read_memory,NULL,payload,meshes,&r)==3 && !r.slots);
    puts("Clone pool: 12 bounded read-only synthetic checks passed; no native game code executed.");
}
