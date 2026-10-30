#ifndef JNET_CANYON_LIVE_H
#define JNET_CANYON_LIVE_H
#include "jnet_canyon_collision.h"
#include "jnet_canyon_payload.h"
struct jnet_canyon_level {
    struct jnet_canyon_node *nodes;
    struct jnet_canyon_hull *hulls;
    unsigned node_count,hull_count;
    volatile LONG seen,full;
};
static struct jnet_canyon_level jnet_canyon_levels[sizeof(jnet_collision_resources)/sizeof(jnet_collision_resources[0])];
static unsigned (*jnet_canyon_query_original)(const float *,float,uintptr_t,uintptr_t *);

static int jnet_canyon_load(uintptr_t image,const struct jnet_collision_resource *resource,struct jnet_canyon_level *level) {
    wchar_t path[MAX_PATH];DWORD length=GetModuleFileNameW(self,path,MAX_PATH);
    wchar_t *slash=length && length<MAX_PATH?wcsrchr(path,L'\\'):NULL;
    if(!slash || (size_t)(slash+1-path)+wcslen(resource->filename)>=MAX_PATH)return 0;
    wcscpy(slash+1,resource->filename);
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    LARGE_INTEGER size;DWORD read=0;unsigned char *bytes=NULL,hash[32];int ok=0;
    struct jnet_canyon_hull *hulls=NULL;struct jnet_canyon_node *nodes=NULL;
    if(resource->bytes<24 || !GetFileSizeEx(file,&size) || size.QuadPart!=resource->bytes)goto done;
    bytes=HeapAlloc(GetProcessHeap(),0,resource->bytes);
    if(!bytes || !ReadFile(file,bytes,resource->bytes,&read,NULL) || read!=resource->bytes ||
       !startup_hash_bytes(bytes,read,hash) || memcmp(hash,resource->hash,32) || memcmp(bytes,"JNCANY1\0",8))goto done;
    uint32_t counts[4];memcpy(counts,bytes+8,sizeof(counts));
    if(!counts[0] || counts[0]!=resource->nodes || !counts[1] || counts[1]!=resource->triangles ||
       !counts[2] || counts[2]!=resource->instances || counts[3] ||
       24ull+counts[0]*sizeof(struct jnet_canyon_node)+(uint64_t)counts[1]*sizeof(struct jnet_canyon_triangle)!=read)goto done;
    hulls=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(SIZE_T)counts[1]*sizeof(*hulls));
    if(!hulls)goto done;
    const struct jnet_canyon_triangle *triangles=(const void *)(bytes+24+counts[0]*sizeof(struct jnet_canyon_node));
    for(unsigned i=0;i<counts[1];++i){
        if(triangles[i].owner>=resource->instances || !jnet_canyon_make_hull(&hulls[i],&triangles[i],image+0x5db310,i))goto done;
        snprintf(hulls[i].name,sizeof(hulls[i].name),"JNET%s%08u",resource->name,i);
    }
    nodes=HeapAlloc(GetProcessHeap(),0,(SIZE_T)counts[0]*sizeof(*nodes));
    if(!nodes)goto done;
    memcpy(nodes,bytes+24,(SIZE_T)counts[0]*sizeof(*nodes));
    level->nodes=nodes;level->hulls=hulls;level->node_count=counts[0];level->hull_count=counts[1];ok=1;
    char line[192];snprintf(line,sizeof(line),"[JNET] %s surface collision ready: %u source instances, %u triangle prisms; original level hulls retained",resource->name,counts[2],counts[1]);message(line);
done:
    CloseHandle(file);
    if(bytes)HeapFree(GetProcessHeap(),0,bytes);
    if(!ok){if(hulls)HeapFree(GetProcessHeap(),0,hulls);if(nodes)HeapFree(GetProcessHeap(),0,nodes);}
    return ok;
}

static int jnet_canyon_prepare(uintptr_t image) {
    int ready=0;
    for(unsigned i=0;i<sizeof(jnet_collision_resources)/sizeof(jnet_collision_resources[0]);++i){
        if(jnet_canyon_load(image,&jnet_collision_resources[i],&jnet_canyon_levels[i]))ready=1;
        else{
            char line[160];snprintf(line,sizeof(line),"[JNET] %s surface collision unavailable; original level collision retained",jnet_collision_resources[i].name);message(line);
        }
    }
    return ready;
}

static unsigned jnet_canyon_query_live(const float *point,float radius,uintptr_t grid,uintptr_t *output) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    unsigned used=jnet_canyon_query_original(point,radius,grid,output);
    if(!JNETGame || used>=4096 ||
       (caller!=fault_image+0xea15b && caller!=fault_image+0xea3c5 && caller!=fault_image+0xea98e))return used;
    int level=-1;uintptr_t barn=0,game=(uintptr_t)JNETGame;
    if(jc_render_level(own_read,NULL,game,&level) ||
       !own_read(NULL,game+0x110,&barn,sizeof(barn)) || !barn || grid!=barn+0x20)return used;
    for(unsigned i=0;i<sizeof(jnet_collision_resources)/sizeof(jnet_collision_resources[0]);++i){
        const struct jnet_collision_resource *resource=&jnet_collision_resources[i];
        struct jnet_canyon_level *collision=&jnet_canyon_levels[i];
        if(resource->level!=level || !collision->hulls)continue;
        unsigned overflow=0;
        unsigned count=jnet_canyon_query(collision->nodes,collision->node_count,collision->hulls,collision->hull_count,
                                       point,radius,barn,output,used,4096,&overflow);
        char line[192];
        if(count>used && InterlockedCompareExchange(&collision->seen,1,0)==0){
            snprintf(line,sizeof(line),"[JNET] %s surface collision active in native hull queries",resource->name);message(line);
        }
        if(overflow && InterlockedIncrement(&collision->full)<=4){
            snprintf(line,sizeof(line),"[JNET] %s surface query reached the 4096-entry native buffer; nearest added candidates retained",resource->name);message(line);
        }
        return count;
    }
    return used;
}
#endif
