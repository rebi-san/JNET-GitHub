#include "native_mesh_rehearsal.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x800000];
static const uint64_t image=0x140000000, barn=0x100000, object=0x200000;
static int stage, scenario, count_reads;
static void put(uint64_t p, uint64_t value) { assert(p+8<=sizeof(memory)); memcpy(memory+p,&value,8); }
static int read_memory(void *ctx, uint64_t p, void *out, size_t size) {
    (void)ctx; assert(stage<4);
    if (p>=sizeof(memory) || size>sizeof(memory)-p) return 0;
    if (scenario==8 && p==object+0x6e0 && stage==2 && count_reads++==1) put(p,246);
    memcpy(out,memory+p,size); return 1;
}
static uint64_t create(void *ctx) {
    (void)ctx; assert(stage==0); stage=1;
    if (scenario==1) return 0;
    if (scenario==2) return barn+0xb0;
    return object;
}
static void initialize(void *ctx, uint64_t p) {
    (void)ctx; assert(stage==1 && p==object); stage=2;
    const uint64_t dudes[]={barn+0xb0,barn+0x146f0,object};
    for (int i=0;i<3;++i) {
        uint64_t d=dudes[i],slot=0x300000+i*0x100,resource=0x400000+i*0x100,payload=0x500000+i*0x100;
        put(d+0x160,image+0x67f3f8); put(d+0x190,slot); put(slot+0x10,resource); put(resource+0x18,payload);
        put(d+0x6d8,0x600000+i*0x10000); put(d+0x6e0,245);
    }
    put(object+0x168,0x700000);
    if (scenario==3) put(object+0x190,0x300000);
    if (scenario==4) put(object+0x6d8,0x600000+8);
    if (scenario==5) put(object+0x6e0,4097);
    if (scenario==6) put(object+0x190,0);
}
static void cleanup(void *ctx,uint64_t p) {
    (void)ctx; assert(stage==2 && p==object); stage=3;
    put(p+0x168,0); put(p+0x190,0); put(p+0x6d8,0); put(p+0x6e0,0);
    if (scenario==7) put(p+0x168,0x700000);
}
static void destroy(void *ctx,uint64_t p) { (void)ctx; assert(stage==3 && p==object); stage=4; }
static int retain_calls,accept_retention;
static int retain(void *ctx,uint64_t p) { (void)ctx; assert(stage==2 && p==object); ++retain_calls; return accept_retention; }
int main(void) {
    const struct jc_mesh_ops ops={{read_memory,create,destroy,NULL},initialize,cleanup};
    for (scenario=0;scenario<9;++scenario) {
        memset(memory,0,sizeof(memory)); stage=0; count_reads=0;
        put(object,image+0x5d0380); put(object+0x160,image+0x67f3f8);
        put(object+0x7b0,image+0x67f3f8); put(object+0xe50,image+0x67f3f8); put(object+0xa390,image+0x5d0360);
        struct jc_mesh_result result;
        int status=jc_mesh_rehearse(&ops,image,barn,&result);
        if (scenario==0) { assert(status==2 && stage==4 && result.independent); assert(result.counts[2]==245); }
        else if (scenario==1) assert(status==-2 && stage==1);
        else if (scenario==2) assert(status==-3 && stage==1);
        else if (scenario==7) assert(status==-3 && stage==3);
        else assert(status==4 && stage==4 && !result.independent);
    }
    for (int mode=0;mode<3;++mode) {
        memset(memory,0,sizeof(memory)); stage=0; count_reads=retain_calls=0;
        scenario=mode==2 ? 3 : 0; accept_retention=mode!=0;
        put(object,image+0x5d0380); put(object+0x160,image+0x67f3f8);
        put(object+0x7b0,image+0x67f3f8); put(object+0xe50,image+0x67f3f8); put(object+0xa390,image+0x5d0360);
        struct jc_mesh_result result;
        int status=jc_mesh_rehearse_or_retain(&ops,image,barn,&result,retain);
        if (mode==1) {
            assert(status==5 && stage==2 && retain_calls==1 && result.independent);
            cleanup(NULL,object); destroy(NULL,object);
        } else if (mode==0) assert(status==2 && stage==4 && retain_calls==1);
        else assert(status==4 && stage==4 && !retain_calls);
    }
    puts("Mesh rehearsal: 12 synthetic lifecycle/failure checks passed; no native game code executed.");
}
