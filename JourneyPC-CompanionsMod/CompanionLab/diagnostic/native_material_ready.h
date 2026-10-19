#ifndef JC_NATIVE_MATERIAL_READY_H
#define JC_NATIVE_MATERIAL_READY_H
#include "native_layout.h"
static inline int jc_material_names_read(jc_layout_reader read,void *ctx,const unsigned char *entries,
    unsigned count,char names[256][32]) {
    uint64_t addresses[256]; unsigned order[256]; unsigned char data[256*32];
    if (count>256) return JC_LAYOUT_RANGE;
    for (unsigned i=0;i<count;++i) {
        uint64_t end;
        memcpy(&addresses[i],entries+i*24+8,8); addresses[i]&=~UINT64_C(1);
        if (!jc_layout_address(addresses[i],32,&end)) return JC_LAYOUT_READ;
        unsigned j=i;
        while (j && addresses[order[j-1]]>addresses[i]) { order[j]=order[j-1]; --j; }
        order[j]=i;
    }
    for (unsigned first=0;first<count;) {
        uint64_t start=addresses[order[first]],end=start+32;
        unsigned next=first+1;
        while (next<count && addresses[order[next]]<=end) {
            uint64_t limit=addresses[order[next]]+32;
            if (limit-start>sizeof(data)) break;
            if (limit>end) end=limit;
            ++next;
        }
        if (!jc_layout_field(read,ctx,start,0,data,(size_t)(end-start))) return JC_LAYOUT_READ;
        for (unsigned i=first;i<next;++i)
            memcpy(names[order[i]],data+(size_t)(addresses[order[i]]-start),32);
        first=next;
    }
    return JC_LAYOUT_OK;
}
static inline int jc_material_ready(jc_layout_reader read, void *ctx, uint64_t material, uint64_t *payload_out) {
    uint64_t slot,link,payload,collection,first,descriptor,again;
    int32_t variant;
    uint32_t encoded,count;
    *payload_out=0;
    if (!jc_layout_field(read,ctx,material,0x30,&slot,8) ||
        !jc_layout_field(read,ctx,slot,0x10,&link,8) ||
        !jc_layout_field(read,ctx,link,0x18,&payload,8) ||
        !jc_layout_field(read,ctx,payload,0x90,&variant,4)) return JC_LAYOUT_READ;
    if (variant<0 || variant>1) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,ctx,payload,0x80+(uint64_t)variant*8,&collection,8) ||
        !jc_layout_field(read,ctx,collection,0,&encoded,4)) return JC_LAYOUT_READ;
    count=encoded&0x7fffffff;
    if (!count || count>4096) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,ctx,collection,8,&first,8)) return JC_LAYOUT_READ;
    if (count>1 && !jc_layout_field(read,ctx,first,0,&first,8)) return JC_LAYOUT_READ;
    if (!jc_layout_field(read,ctx,first,0,&descriptor,8) ||
        !jc_layout_field(read,ctx,descriptor,0,&again,8)) return JC_LAYOUT_READ;
    uint64_t check; int32_t check_variant; uint32_t check_count;
    if (!jc_layout_field(read,ctx,material,0x30,&check,8) || check!=slot ||
        !jc_layout_field(read,ctx,slot,0x10,&check,8) || check!=link ||
        !jc_layout_field(read,ctx,link,0x18,&check,8) || check!=payload ||
        !jc_layout_field(read,ctx,payload,0x90,&check_variant,4) || check_variant!=variant ||
        !jc_layout_field(read,ctx,payload,0x80+(uint64_t)variant*8,&check,8) || check!=collection ||
        !jc_layout_field(read,ctx,collection,0,&check_count,4) || check_count!=encoded) return JC_LAYOUT_CHANGED;
    *payload_out=payload;
    return JC_LAYOUT_OK;
}
#endif
