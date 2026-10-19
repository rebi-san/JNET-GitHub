#include "native_animation_storage.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x180000];
static const uint64_t image=UINT64_C(0x140000000), meshes[3]={0x100000,0x180000,0x200000};
static struct jc_animation_workspace work;
static unsigned reads,passes;
static int mutate,deny;
static void put(uint64_t address,uint64_t value,unsigned size) {
    assert(address>=meshes[0] && address-meshes[0]+size<=sizeof(memory));
    memcpy(memory+(address-meshes[0]),&value,size);
}
static int reader(void *unused,uint64_t address,void *out,size_t size) {
    (void)unused; assert(++reads<14000);
    if (address==meshes[0]) ++passes;
    if (deny && address==meshes[2]+0x30000) return 0;
    if (address<meshes[0] || address-meshes[0]+size>sizeof(memory)) return 0;
    memcpy(out,memory+(address-meshes[0]),size);
    if (mutate && passes==2 && address==meshes[0]+0x10000) {
        uint64_t changed=meshes[0]+0x30040; memcpy(out,&changed,8);
    }
    return 1;
}
static void fixture(unsigned count) {
    memset(memory,0,sizeof(memory)); reads=passes=0; mutate=deny=0;
    for (unsigned i=0;i<3;++i) {
        uint64_t base=meshes[i];
        put(base,image+0x67f3f8,8); put(base+0x30,base+0x1000,8);
        put(base+0x1010,base+0x2000,8); put(base+0x2018,base+0x3000,8);
        for (unsigned k=0;k<3;++k) {
            put(base+0x3040+k*16,base+0x10000+k*0x2000,8);
            put(base+0x3048+k*16,k ? 0 : count,8);
        }
        for (unsigned j=0;j<count && j<1024;++j) put(base+0x10000+j*8,base+0x30000+j*64,8);
        for (unsigned k=0;k<2;++k) {
            put(base+0x3020+k*8,base+0x4000+k*0x100,8);
            put(base+0x4028+k*0x100,0x80000008,4);
            put(base+0x4030+k*0x100,base+0x60000+k*0x100,8);
        }
    }
}
static int check(struct jc_animation_result *out) {
    return jc_three_animation_storage(reader,NULL,image,meshes,out,&work);
}
int main(void) {
    struct jc_animation_result out;
    fixture(245); assert(!check(&out) && out.disjoint==1 && out.counts[2][0]==245 && out.counts[2][3]==8);
    fixture(0); assert(!check(&out) && out.disjoint==1 && !out.counts[0][0]);
    fixture(1024); assert(!check(&out) && out.disjoint==1);
    fixture(1025); assert(check(&out)==5 && out.disjoint==-1);
    fixture(1); put(meshes[2]+0x10000,meshes[0]+0x30008,8); assert(!check(&out) && out.disjoint==0);
    fixture(1); put(meshes[2]+0x10000,meshes[0]+0x60000,8); assert(!check(&out) && out.disjoint==0);
    fixture(1); put(meshes[2]+0x4030,meshes[0]+0x60100,8); assert(!check(&out) && out.disjoint==0);
    fixture(1); put(meshes[2]+0x10000,0,8); assert(check(&out)==JC_LAYOUT_RANGE && out.disjoint==-1);
    fixture(1); put(meshes[2]+0x4028,1025,4); assert(check(&out)==5 && out.disjoint==-1);
    fixture(1); deny=1; assert(check(&out)==JC_LAYOUT_READ && out.disjoint==-1);
    fixture(1); mutate=1; assert(check(&out)==JC_LAYOUT_CHANGED && out.disjoint==-1);
    fixture(1); assert(jc_three_animation_storage(reader,NULL,image,meshes,&out,NULL)==8 && out.disjoint==-1);
    puts("Animation storage: 12 synthetic cases passed; no native animation or draw calls.");
}
