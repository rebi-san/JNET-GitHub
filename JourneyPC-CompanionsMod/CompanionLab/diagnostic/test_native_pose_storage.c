#include "native_pose_storage.h"
#include <assert.h>
#include <stdio.h>
static const uint64_t image=UINT64_C(0x140000000), meshes[3]={0x100000,0x200000,0x300000};
static uint64_t models[3], roots[3];
static int reads, mutate, deny, wrong_type, missing;
static int reader(void *unused,uint64_t address,void *out,size_t size) {
    (void)unused; assert(++reads<=42);
    if (deny && size==64) return 0;
    for (int i=0;i<3;++i) {
        uint64_t value=0;
        uint64_t slot=0x400000+(uint64_t)i*0x1000;
        uint64_t resource=0x500000+(uint64_t)i*0x1000;
        uint64_t payload=0x600000+(uint64_t)i*0x1000;
        if (address==meshes[i]) value=wrong_type ? 0 : image+0x67f3f8;
        else if (address==meshes[i]+0x30) value=slot;
        else if (address==slot+0x10) value=resource;
        else if (address==resource+0x18) value=missing ? 0 : payload;
        else if (address==payload) value=models[i];
        else if (address==models[i]+0x100) value=roots[i]+(mutate && reads>21 ? 0x100 : 0);
        else if (address==roots[i]+0x20 || (mutate && address==roots[i]+0x120)) {
            assert(size==64); memset(out,0,size); return 1;
        } else continue;
        assert(size==8); memcpy(out,&value,8); return 1;
    }
    return 0;
}
static void fixture(void) {
    reads=mutate=deny=wrong_type=missing=0;
    for (int i=0;i<3;++i) { models[i]=0x700000+(uint64_t)i*0x1000; roots[i]=0x800000+(uint64_t)i*0x1000; }
}
static int check(int *a,int *b) { return jc_three_pose_storage(reader,NULL,image,meshes,a,b); }
int main(void) {
    int a,b;
    fixture(); assert(!check(&a,&b) && a==1 && b==1 && reads==42);
    fixture(); models[2]=models[0]; assert(!check(&a,&b) && a==0 && b==0);
    fixture(); models[2]=models[0]+8; assert(!check(&a,&b) && a==0);
    fixture(); roots[2]=roots[0]; assert(!check(&a,&b) && a==1 && b==0);
    fixture(); roots[2]=roots[0]+8; assert(!check(&a,&b) && b==0);
    fixture(); roots[2]=models[0]+0x80; assert(!check(&a,&b) && a==0 && b==0);
    fixture(); roots[2]=0; assert(check(&a,&b)==JC_LAYOUT_RANGE && a==-1 && b==-1);
    fixture(); models[2]=UINT64_MAX; assert(check(&a,&b)==JC_LAYOUT_RANGE && a==-1 && b==-1);
    fixture(); missing=1; assert(check(&a,&b)==JC_LAYOUT_TYPE && a==-1 && b==-1);
    fixture(); wrong_type=1; assert(check(&a,&b)==JC_LAYOUT_TYPE && a==-1 && b==-1);
    fixture(); deny=1; assert(check(&a,&b)==JC_LAYOUT_READ && a==-1 && b==-1);
    fixture(); mutate=1; assert(check(&a,&b)==JC_LAYOUT_CHANGED && a==-1 && b==-1);
    puts("Pose storage: 12 synthetic checks passed; no game calls or rendering.");
    return 0;
}
