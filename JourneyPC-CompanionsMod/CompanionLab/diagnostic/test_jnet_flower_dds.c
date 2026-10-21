#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "jnet_flower_dds.h"

static void put(unsigned char *data,size_t offset,uint32_t value) {for(unsigned i=0;i<4;++i)data[offset+i]=(unsigned char)(value>>(8*i));}
static size_t make(unsigned char *data,unsigned width,unsigned height,unsigned mips) {
    memset(data,0,128);memcpy(data,"DDS ",4);
    put(data,4,124);put(data,8,0x20000|0x1007);put(data,12,height);put(data,16,width);put(data,28,mips);
    put(data,76,32);put(data,80,0x41);put(data,88,32);put(data,92,0xff0000);put(data,96,0xff00);put(data,100,0xff);put(data,104,0xff000000u);
    size_t at=128;
    for(unsigned level=0;level<mips;++level){
        unsigned w=width>>level?width>>level:1u,h=height>>level?height>>level:1u;
        memset(data+at,(int)(level+1),(size_t)w*h*4);at+=(size_t)w*h*4;
    }
    return at;
}
int main(void) {
    unsigned char *data=malloc(1<<20);
    struct jnet_flower_dds_level levels[JNET_FLOWER_DDS_MAX_LEVELS];
    size_t size=make(data,8,16,5);
    /* All five levels are found with their sizes and in order; 8x16 ends at 1x1. */
    assert(jnet_flower_dds_levels(data,size,8,16,levels)==5);
    assert(levels[0].width==8 && levels[0].height==16 && levels[0].pixels[0]==1 && levels[4].width==1 && levels[4].height==1 && levels[4].pixels[0]==5);
    assert(levels[1].pixels==levels[0].pixels+8*16*4 && levels[4].pixels+4==data+size);
    /* A single-surface file with no mip flag has one level. */
    size=make(data,8,16,1);put(data,8,0x1007);assert(jnet_flower_dds_levels(data,size,8,16,levels)==1);
    size=make(data,8,16,5);
    /* Truncated, wrong size, wrong format and wrong header are all refused. */
    assert(!jnet_flower_dds_levels(data,size-1,8,16,levels) && !jnet_flower_dds_levels(data,127,8,16,levels) && !jnet_flower_dds_levels(NULL,size,8,16,levels));
    assert(!jnet_flower_dds_levels(data,size,16,8,levels) && !jnet_flower_dds_levels(data,size,8,32,levels));
    unsigned char saved[128];memcpy(saved,data,128);
    put(data,92,0xff);assert(!jnet_flower_dds_levels(data,size,8,16,levels));memcpy(data,saved,128);
    put(data,88,24);assert(!jnet_flower_dds_levels(data,size,8,16,levels));memcpy(data,saved,128);
    put(data,80,0x4);assert(!jnet_flower_dds_levels(data,size,8,16,levels));memcpy(data,saved,128);
    put(data,4,100);assert(!jnet_flower_dds_levels(data,size,8,16,levels));memcpy(data,saved,128);
    put(data,28,0);assert(!jnet_flower_dds_levels(data,size,8,16,levels));memcpy(data,saved,128);
    put(data,28,15);assert(!jnet_flower_dds_levels(data,size,8,16,levels));memcpy(data,saved,128);
    data[0]='X';assert(!jnet_flower_dds_levels(data,size,8,16,levels));
    free(data);
    puts("Flower DDS: mip levels, single surface and corrupt/truncated/mismatched file rejection passed.");
    return 0;
}
