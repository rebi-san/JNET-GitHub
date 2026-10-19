#ifndef JC_NATIVE_ANIMATION_STORAGE_H
#define JC_NATIVE_ANIMATION_STORAGE_H
#include "native_pose_writes.h"
#define JC_ANIMATION_CAP 1024
#define JC_ANIMATION_SPANS (3*(JC_ANIMATION_CAP+6))
struct jc_animation_meta {
    struct jc_clone_snapshot clone;
    uint64_t tables[3], counts[3], blends[2], weights[2];
    uint32_t weight_counts[2];
};
struct jc_animation_workspace { struct jc_pose_span spans[2][JC_ANIMATION_SPANS]; };
struct jc_animation_result { uint32_t counts[3][5]; int disjoint; };
static inline int jc_animation_collect(jc_layout_reader read,void *context,uint64_t image,
        const uint64_t meshes[3],struct jc_animation_meta meta[3],struct jc_pose_span *spans,unsigned *used) {
    *used=0;
    memset(meta,0,3*sizeof(*meta));
    for (unsigned owner=0;owner<3;++owner) {
        struct jc_animation_meta *m=&meta[owner];
        int code=jc_clone_snapshot_read(read,context,image,meshes[owner],&m->clone);
        if (code) return code;
        if (!m->clone.payload) return JC_LAYOUT_TYPE;
        unsigned char fields[0x70]; uint64_t end;
        if (!jc_layout_field(read,context,m->clone.payload,0,fields,sizeof(fields))) return JC_LAYOUT_READ;
        spans[(*used)++]=(struct jc_pose_span){m->clone.payload,sizeof(fields),owner};
        unsigned total=0;
        for (unsigned kind=0;kind<3;++kind) {
            memcpy(&m->tables[kind],fields+0x40+kind*16,8);
            memcpy(&m->counts[kind],fields+0x48+kind*16,8);
            if (m->counts[kind]>JC_ANIMATION_CAP-total) return 5;
            total+=(unsigned)m->counts[kind];
            if (!m->counts[kind]) continue;
            if ((m->tables[kind]&7) || !jc_layout_address(m->tables[kind],m->counts[kind]*8,&end)) return JC_LAYOUT_RANGE;
            spans[(*used)++]=(struct jc_pose_span){m->tables[kind],(uint32_t)m->counts[kind]*8,owner};
            for (unsigned i=0;i<m->counts[kind];++i) {
                uint64_t object; unsigned char scratch[0x28];
                if (!jc_layout_field(read,context,m->tables[kind],i*8,&object,8)) return JC_LAYOUT_READ;
                if ((object&7) || !jc_layout_address(object,sizeof(scratch),&end)) return JC_LAYOUT_RANGE;
                if (!read(context,object,scratch,sizeof(scratch))) return JC_LAYOUT_READ;
                spans[(*used)++]=(struct jc_pose_span){object,sizeof(scratch),owner};
            }
        }
        for (unsigned kind=0;kind<2;++kind) {
            memcpy(&m->blends[kind],fields+0x20+kind*8,8);
            unsigned char header[16];
            if (!jc_layout_field(read,context,m->blends[kind],0x28,header,sizeof(header))) return JC_LAYOUT_READ;
            memcpy(&m->weight_counts[kind],header,4); m->weight_counts[kind]&=0x7fffffff;
            memcpy(&m->weights[kind],header+8,8);
            if (m->weight_counts[kind]>JC_ANIMATION_CAP) return 5;
            if (!m->weight_counts[kind]) continue;
            unsigned bytes=m->weight_counts[kind]*4;
            if ((m->weights[kind]&3) || !jc_layout_address(m->weights[kind],bytes,&end)) return JC_LAYOUT_RANGE;
            unsigned char scratch[64];
            for (unsigned offset=0;offset<bytes;offset+=sizeof(scratch)) {
                unsigned size=bytes-offset<sizeof(scratch) ? bytes-offset : sizeof(scratch);
                if (!read(context,m->weights[kind]+offset,scratch,size)) return JC_LAYOUT_READ;
            }
            spans[(*used)++]=(struct jc_pose_span){m->weights[kind],bytes,owner};
        }
    }
    return JC_LAYOUT_OK;
}
static inline int jc_three_animation_storage(jc_layout_reader read,void *context,uint64_t image,
        const uint64_t meshes[3],struct jc_animation_result *out,struct jc_animation_workspace *work) {
    memset(out,0,sizeof(*out)); out->disjoint=-1;
    if (!work) return 8;
    struct jc_animation_meta before[3],after[3]; unsigned count,again;
    int code=jc_animation_collect(read,context,image,meshes,before,work->spans[0],&count);
    if (code) return code;
    code=jc_animation_collect(read,context,image,meshes,after,work->spans[1],&again);
    if (code) return code;
    if (count!=again || memcmp(before,after,sizeof(before))) return JC_LAYOUT_CHANGED;
    for (unsigned i=0;i<count;++i)
        if (work->spans[0][i].address!=work->spans[1][i].address ||
            work->spans[0][i].size!=work->spans[1][i].size ||
            work->spans[0][i].owner!=work->spans[1][i].owner) return JC_LAYOUT_CHANGED;
    jc_pose_sort(work->spans[0],count);
    uint64_t ends[3]={0}; int disjoint=1;
    for (unsigned i=0;i<count;++i) {
        struct jc_pose_span span=work->spans[0][i];
        for (unsigned j=0;j<3;++j) if (j!=span.owner && ends[j]>span.address) disjoint=0;
        if (span.address+span.size>ends[span.owner]) ends[span.owner]=span.address+span.size;
    }
    for (unsigned i=0;i<3;++i) {
        for (unsigned j=0;j<3;++j) out->counts[i][j]=(uint32_t)before[i].counts[j];
        for (unsigned j=0;j<2;++j) out->counts[i][j+3]=before[i].weight_counts[j];
    }
    out->disjoint=disjoint;
    return JC_LAYOUT_OK;
}
#endif
