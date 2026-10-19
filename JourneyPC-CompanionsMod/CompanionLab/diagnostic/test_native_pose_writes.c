#include "native_pose_writes.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x180000];
static const uint64_t image=UINT64_C(0x140000000), meshes[3]={0x100000,0x180000,0x200000};
static struct jc_pose_write_workspace work;
static unsigned reads,passes;
static int mutate,deny;
static void put(uint64_t address,uint64_t value,unsigned size) {
    assert(address>=meshes[0] && address-meshes[0]+size<=sizeof(memory));
    memcpy(memory+(address-meshes[0]),&value,size);
}
static int reader(void *unused,uint64_t address,void *out,size_t size) {
    (void)unused; assert(++reads<13000);
    if (address==meshes[0]) ++passes;
    if (deny && address==meshes[2]+0x30000) return 0;
    if (address<meshes[0] || address-meshes[0]+size>sizeof(memory)) return 0;
    memcpy(out,memory+(address-meshes[0]),size);
    if (mutate && passes==2 && address==meshes[0]+0x10000+24) {
        uint64_t changed=meshes[0]+0x30080; memcpy(out,&changed,8);
    }
    return 1;
}
static void fixture(unsigned a,unsigned b) {
    memset(memory,0,sizeof(memory)); reads=passes=0; mutate=deny=0;
    for (unsigned i=0;i<3;++i) {
        uint64_t base=meshes[i];
        put(base,image+0x67f3f8,8); put(base+0x30,base+0x1000,8);
        put(base+0x1010,base+0x2000,8); put(base+0x2018,base+0x3000,8);
        put(base+0x3000,base+0x4000,8); put(base+0x4100,base+0x5000,8);
        put(base+0x40c8,base+0x10000,8); put(base+0x40e0,base+0x20000,8);
        put(base+0x40e8,a,4); put(base+0x40f0,b,4);
        for (unsigned j=0;j<a && j<1024;++j) put(base+0x10000+j*32+24,base+0x30000+j*64,8);
        for (unsigned j=0;j<b && j<1024;++j) put(base+0x20000+j*16,base+0x50000+j*64,8);
    }
}
static int check(struct jc_pose_write_result *out) {
    return jc_three_pose_writes(reader,NULL,image,meshes,out,&work);
}
int main(void) {
    struct jc_pose_write_result out;
    fixture(2,3); assert(!check(&out) && out.disjoint==1 && out.counts[2][0]==2 && out.counts[2][1]==3);
    fixture(0,0); assert(!check(&out) && out.disjoint==1 && !out.counts[0][0]);
    fixture(1024,0); assert(!check(&out) && out.disjoint==1 && out.counts[2][0]==1024);
    fixture(1024,1); assert(check(&out)==5 && out.disjoint==-1 && !out.counts[0][0]);
    fixture(1,1); put(meshes[2]+0x10018,meshes[0]+0x30000,8); assert(!check(&out) && out.disjoint==0);
    fixture(1,1); put(meshes[2]+0x20000,meshes[0]+0x30020,8); assert(!check(&out) && out.disjoint==0);
    fixture(1,1); put(meshes[2]+0x10018,meshes[0]+0x5020,8); assert(!check(&out) && out.disjoint==0);
    fixture(1,1); put(meshes[2]+0x10018,meshes[0]+0x30040,8); assert(!check(&out) && out.disjoint==1);
    fixture(1,1); put(meshes[0]+0x20000,meshes[0]+0x30000,8); assert(!check(&out) && out.disjoint==1);
    fixture(1,1); put(meshes[2]+0x10018,0,8); assert(check(&out)==JC_LAYOUT_RANGE && out.disjoint==-1);
    fixture(1,1); put(meshes[2]+0x40c8,UINT64_MAX,8); assert(check(&out)==JC_LAYOUT_RANGE && out.disjoint==-1);
    fixture(1,1); deny=1; assert(check(&out)==JC_LAYOUT_READ && out.disjoint==-1);
    fixture(1,1); mutate=1; assert(check(&out)==JC_LAYOUT_CHANGED && out.disjoint==-1);
    fixture(1,1); assert(jc_three_pose_writes(reader,NULL,image,meshes,&out,NULL)==8 && out.disjoint==-1);
    struct jc_pose_quality quality;
    fixture(1,0); assert(!jc_pose_quality_read(reader,NULL,meshes[0]+0x4000,&quality) && quality.singular==1);
    for (unsigned i=0;i<4;++i) put(meshes[0]+0x30000+i*20,0x3f800000,4);
    assert(!jc_pose_quality_read(reader,NULL,meshes[0]+0x4000,&quality) && quality.count==1 && !quality.singular && !quality.nonfinite);
    put(meshes[0]+0x30000,0x7f800000,4);
    assert(!jc_pose_quality_read(reader,NULL,meshes[0]+0x4000,&quality) && quality.nonfinite==1);
    fixture(257,0); assert(jc_pose_quality_read(reader,NULL,meshes[0]+0x4000,&quality)==5);
    fixture(1,0); deny=1; assert(jc_pose_quality_read(reader,NULL,meshes[2]+0x4000,&quality)==1);
    fixture(0,0); assert(!jc_pose_quality_read(reader,NULL,meshes[0]+0x4000,&quality) && !quality.count);
    puts("Pose writes: 14 ownership and 6 matrix-quality cases passed; no native calls.");
}
