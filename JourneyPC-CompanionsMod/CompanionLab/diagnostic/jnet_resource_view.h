#ifndef JNET_RESOURCE_VIEW_H
#define JNET_RESOURCE_VIEW_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef int (*jnet_view_read)(void *,uint64_t,void *,size_t);
static inline uint64_t jnet_resource_view(jnet_view_read read,void *ctx,uint64_t object,
        uint32_t index,unsigned *failure) {
    unsigned char header[16],again[16];uint32_t count;uint64_t base,value=0;
    *failure=0;
    if(!object || object>UINT64_MAX-0x40 || !read(ctx,object+0x30,header,sizeof(header))){*failure=1;return 0;}
    memcpy(&count,header,4);count&=0x7fffffff;
    if(index>=count)return 0;
    if(count==1)base=object+0x38;else memcpy(&base,header+8,8);
    uint64_t offset=(uint64_t)index*48+8;
    if(!base || base>UINT64_MAX-offset-8 || !read(ctx,base+offset,&value,8)){*failure=2;return 0;}
    if(!read(ctx,object+0x30,again,sizeof(again)) || memcmp(header,again,sizeof(header))){*failure=3;return 0;}
    return value;
}
#endif
