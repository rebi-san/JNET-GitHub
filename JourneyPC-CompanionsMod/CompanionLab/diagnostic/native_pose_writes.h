#ifndef JC_NATIVE_POSE_WRITES_H
#define JC_NATIVE_POSE_WRITES_H
#include "native_pose_storage.h"
#define JC_POSE_WRITE_CAP 1024
#define JC_POSE_SPANS (3*(JC_POSE_WRITE_CAP+1))
struct jc_pose_span { uint64_t address; uint32_t size, owner; };
struct jc_pose_tables { struct jc_pose_storage pose; uint64_t table[2]; uint32_t count[2]; };
struct jc_pose_write_workspace { struct jc_pose_span spans[2][JC_POSE_SPANS]; };
struct jc_pose_write_result { uint32_t counts[3][2]; int disjoint; };
static inline void jc_pose_sift(struct jc_pose_span *spans,unsigned root,unsigned count) {
    while (root<count/2) {
        unsigned child=2*root+1;
        if (child+1<count && spans[child].address<spans[child+1].address) ++child;
        if (spans[root].address>=spans[child].address) return;
        struct jc_pose_span swap=spans[root]; spans[root]=spans[child]; spans[child]=swap;
        root=child;
    }
}
static inline void jc_pose_sort(struct jc_pose_span *spans,unsigned count) {
    for (unsigned i=count/2;i>0;--i) jc_pose_sift(spans,i-1,count);
    for (unsigned i=count;i>1;--i) {
        struct jc_pose_span swap=spans[0]; spans[0]=spans[i-1]; spans[i-1]=swap;
        jc_pose_sift(spans,0,i-1);
    }
}
static inline int jc_pose_writes_collect(jc_layout_reader read,void *context,uint64_t image,
        const uint64_t meshes[3],struct jc_pose_tables meta[3],struct jc_pose_span *spans,unsigned *used) {
    *used=0;
    memset(meta,0,3*sizeof(*meta));
    for (unsigned owner=0;owner<3;++owner) {
        struct jc_pose_tables *m=&meta[owner];
        int code=jc_pose_storage_read(read,context,image,meshes[owner],&m->pose);
        if (code) return code;
        unsigned char fields[44];
        if (!jc_layout_field(read,context,m->pose.model,0xc8,fields,sizeof(fields))) return JC_LAYOUT_READ;
        memcpy(&m->table[0],fields,8); memcpy(&m->table[1],fields+24,8);
        memcpy(&m->count[0],fields+32,4); memcpy(&m->count[1],fields+40,4);
        if (m->count[0]>JC_POSE_WRITE_CAP || m->count[1]>JC_POSE_WRITE_CAP-m->count[0]) return 5;
        spans[(*used)++]=(struct jc_pose_span){m->pose.root+0x20,64,owner};
        for (unsigned kind=0;kind<2;++kind) {
            uint64_t end;
            unsigned stride=kind ? 16 : 32,offset=kind ? 0 : 24,size=kind ? 48 : 64;
            if (m->count[kind] && ((m->table[kind]&7) ||
                !jc_layout_address(m->table[kind],(uint64_t)m->count[kind]*stride,&end))) return JC_LAYOUT_RANGE;
            for (unsigned i=0;i<m->count[kind];++i) {
                uint64_t address;
                unsigned char scratch[64];
                if (!jc_layout_field(read,context,m->table[kind],(uint64_t)i*stride+offset,&address,8)) return JC_LAYOUT_READ;
                if (!jc_layout_address(address,size,&end)) return JC_LAYOUT_RANGE;
                if (!read(context,address,scratch,size)) return JC_LAYOUT_READ;
                spans[(*used)++]=(struct jc_pose_span){address,size,owner};
            }
        }
    }
    return JC_LAYOUT_OK;
}
/* This enumerates two explicit matrix-write loops, not every native side effect. */
static inline int jc_three_pose_writes(jc_layout_reader read,void *context,uint64_t image,
        const uint64_t meshes[3],struct jc_pose_write_result *out,struct jc_pose_write_workspace *work) {
    memset(out,0,sizeof(*out)); out->disjoint=-1;
    if (!work) return 8;
    struct jc_pose_tables before[3],after[3];
    unsigned count=0,again=0;
    int code=jc_pose_writes_collect(read,context,image,meshes,before,work->spans[0],&count);
    if (code) return code;
    code=jc_pose_writes_collect(read,context,image,meshes,after,work->spans[1],&again);
    if (code) return code;
    if (count!=again) return JC_LAYOUT_CHANGED;
    for (unsigned i=0;i<3;++i) {
        if (!jc_clone_snapshot_equal(&before[i].pose.clone,&after[i].pose.clone) ||
            before[i].pose.model!=after[i].pose.model || before[i].pose.root!=after[i].pose.root ||
            memcmp(before[i].table,after[i].table,sizeof(before[i].table)) ||
            memcmp(before[i].count,after[i].count,sizeof(before[i].count))) return JC_LAYOUT_CHANGED;
    }
    for (unsigned i=0;i<count;++i)
        if (work->spans[0][i].address!=work->spans[1][i].address ||
            work->spans[0][i].size!=work->spans[1][i].size ||
            work->spans[0][i].owner!=work->spans[1][i].owner) return JC_LAYOUT_CHANGED;
    jc_pose_sort(work->spans[0],count);
    uint64_t ends[3]={0};
    int disjoint=1;
    for (unsigned i=0;i<count;++i) {
        struct jc_pose_span span=work->spans[0][i];
        for (unsigned j=0;j<3;++j) if (j!=span.owner && ends[j]>span.address) disjoint=0;
        uint64_t end=span.address+span.size;
        if (end>ends[span.owner]) ends[span.owner]=end;
    }
    for (unsigned i=0;i<3;++i) memcpy(out->counts[i],before[i].count,sizeof(out->counts[i]));
    out->disjoint=disjoint;
    return JC_LAYOUT_OK;
}
struct jc_pose_quality { uint32_t count, nonfinite, singular; };
static inline int jc_pose_quality_read(jc_layout_reader read,void *context,uint64_t model,
                                       struct jc_pose_quality *out) {
    memset(out,0,sizeof(*out));
    uint64_t table,again,destination; uint32_t count,recount;
    if (!jc_layout_field(read,context,model,0xc8,&table,8) ||
        !jc_layout_field(read,context,model,0xe8,&count,4)) return 1;
    if (count>256) return 5;
    for (unsigned i=0;i<count;++i) {
        uint32_t bits[16]; float m[16];
        if (!jc_layout_field(read,context,table,(uint64_t)i*32+24,&destination,8) ||
            !jc_layout_field(read,context,destination,0,bits,sizeof(bits))) return 1;
        int finite=1;
        for (unsigned j=0;j<16;++j) if ((bits[j]&0x7f800000u)==0x7f800000u) finite=0;
        if (!finite) { ++out->nonfinite; continue; }
        memcpy(m,bits,sizeof(m));
        double determinant=(double)m[0]*((double)m[5]*m[10]-(double)m[6]*m[9])
            -(double)m[1]*((double)m[4]*m[10]-(double)m[6]*m[8])
            +(double)m[2]*((double)m[4]*m[9]-(double)m[5]*m[8]);
        if (determinant>-1e-8 && determinant<1e-8) ++out->singular;
    }
    if (!jc_layout_field(read,context,model,0xc8,&again,8) ||
        !jc_layout_field(read,context,model,0xe8,&recount,4)) return 1;
    if (again!=table || recount!=count) return 3;
    out->count=count; return 0;
}
#endif
