#ifndef JNET_LEVEL_FIXES_H
#define JNET_LEVEL_FIXES_H
#include "jnet_texture_guard.h"
#include "jnet_resource_view.h"
static void *jnet_resource_view_original;
static uint64_t (*jnet_resource_buffer_original)(uintptr_t,uint32_t,unsigned char);
static volatile LONG jnet_resource_view_rejected;
static uint64_t jnet_resource_view_guard(uintptr_t object,uint32_t index) {
    unsigned failure=0;
    uint64_t result=jnet_resource_view(own_read,NULL,object,index,&failure);
    if(failure && InterlockedIncrement(&jnet_resource_view_rejected)<=8){
        char line[224];
        snprintf(line,sizeof(line),"[JNET] renderer resource view rejected stage=%u object=%llx index=%u caller=%llx frame=%lu",
            failure,(unsigned long long)object,index,
            (unsigned long long)(uintptr_t)__builtin_return_address(0),(unsigned long)JNETFrames);
        message(line);
    }
    return result;
}
static uint64_t jnet_resource_buffer_guard(uintptr_t object,uint32_t index,unsigned char writable) {
    if(writable)return jnet_resource_buffer_original(object,index,writable);
    return jnet_resource_view_guard(object,index);
}
static uintptr_t jnet_bryan_mesh;
static unsigned jnet_bryan_visits;
static int jnet_bryan_ready=-1;
static size_t (*jnet_decoration_read_original)(void *,size_t,size_t,void *);
static size_t jnet_decoration_read(void *buffer,size_t size,size_t count,void *stream) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    size_t result=jnet_decoration_read_original(buffer,size,count,stream);
    if(caller==fault_image+0x9f27a && size==1 && count==0x840 && result==count){
        static const unsigned char expected[32]={0xad,0x09,0xbe,0x48,0x19,0x95,0x2e,0x49,0x7c,0x16,0x7f,0xff,0x02,0x0f,0x36,0x9f,0x5f,0xa4,0x8f,0x42,0xeb,0x21,0x97,0x80,0xa0,0x96,0x5b,0x62,0x10,0x98,0x73,0xb4};
        unsigned char hash[32];
        if(startup_hash_bytes(buffer,0x840,hash) && !memcmp(hash,expected,32)){
            uintptr_t barn=0;uint32_t used=0;
            jnet_bryan_mesh=0;jnet_bryan_visits=0;jnet_bryan_ready=-1;
            if(JNETGame && own_read(NULL,(uintptr_t)JNETGame+0x138,&barn,8) && barn &&
               own_read(NULL,barn+0x434010,&used,4) && used<1024)
                jnet_bryan_mesh=barn+0x10+(uintptr_t)used*0x10d0;
            message("[JNET] Bryan diagnostic: original P_Relief record verified; texColor/texCham=Tapestry12 retained; position=363.995,70.524,498.829");
        }
    }
    return result;
}
static void jnet_bryan_observe(uintptr_t mesh) {
    if(!jnet_bryan_mesh || mesh!=jnet_bryan_mesh)return;
    int ready=jnet_character_textures_ready(own_read,NULL,mesh);
    if(jnet_bryan_visits++>=3 && ready==jnet_bryan_ready)return;
    jnet_bryan_ready=ready;
    for(unsigned i=0;i<2;++i){
        unsigned slot=i?10:1;uint64_t binding=0,resource=0,texture=0,gpu=0;
        int read=own_read(NULL,mesh+0x768+(uint64_t)slot*0x80,&binding,8) && binding &&
            own_read(NULL,binding+0x10,&resource,8) && resource &&
            own_read(NULL,resource+0xc8,&texture,8) && texture &&
            own_read(NULL,texture+0x70,&gpu,8);
        char line[256];snprintf(line,sizeof(line),"[JNET] Bryan Tapestry12 %s before native update: read=%d ready=%d binding=%llx resource=%llx texture=%llx gpu=%llx",i?"texCham":"texColor",read,ready,(unsigned long long)binding,(unsigned long long)resource,(unsigned long long)texture,(unsigned long long)gpu);message(line);
    }
}
#endif
