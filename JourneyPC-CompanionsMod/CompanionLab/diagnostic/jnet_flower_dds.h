#ifndef JNET_FLOWER_DDS_H
#define JNET_FLOWER_DDS_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Locates the mip levels of an uncompressed 32-bit BGRA DDS, the format of Flower's petal atlas. Everything is bounds-checked: the file
   is read from disk, so a short, corrupt or different image must be refused rather than trusted. */
#define JNET_FLOWER_DDS_MAX_LEVELS 14u
struct jnet_flower_dds_level { const unsigned char *pixels; unsigned width,height; };
static inline uint32_t jnet_flower_dds_word(const unsigned char *data,size_t offset) {
    return (uint32_t)data[offset]|((uint32_t)data[offset+1]<<8)|((uint32_t)data[offset+2]<<16)|((uint32_t)data[offset+3]<<24);
}
/* Returns the level count (at least 1) or 0 when the image is not the expected 32-bit BGRA surface of exactly width x height. */
static inline unsigned jnet_flower_dds_levels(const unsigned char *data,size_t size,unsigned width,unsigned height,
        struct jnet_flower_dds_level levels[JNET_FLOWER_DDS_MAX_LEVELS]) {
    if(!data || size<128 || memcmp(data,"DDS ",4) || jnet_flower_dds_word(data,4)!=124 || jnet_flower_dds_word(data,76)!=32)return 0;
    uint32_t flags=jnet_flower_dds_word(data,80),bits=jnet_flower_dds_word(data,88);
    /* DDPF_RGB|DDPF_ALPHAPIXELS, 32 bits, masks B=0x0000ff, G=0x00ff00, R=0xff0000 stored as BGRA, A=0xff000000. */
    if((flags&0x41)!=0x41 || bits!=32 || jnet_flower_dds_word(data,92)!=0xff0000 || jnet_flower_dds_word(data,96)!=0xff00 ||
       jnet_flower_dds_word(data,100)!=0xff || jnet_flower_dds_word(data,104)!=0xff000000u)return 0;
    if(jnet_flower_dds_word(data,12)!=height || jnet_flower_dds_word(data,16)!=width)return 0;
    unsigned count=(jnet_flower_dds_word(data,8)&0x20000u)?(unsigned)jnet_flower_dds_word(data,28):1u;
    if(!count || count>JNET_FLOWER_DDS_MAX_LEVELS)return 0;
    size_t at=128;
    for(unsigned level=0;level<count;++level){
        unsigned w=width>>level?width>>level:1u,h=height>>level?height>>level:1u;
        size_t bytes=(size_t)w*h*4;
        if(bytes>size-at || at>size)return 0;
        levels[level]=(struct jnet_flower_dds_level){data+at,w,h};
        at+=bytes;
    }
    return count;
}
#endif
