#ifndef JC_NATIVE_TRIAL_SHADER_H
#define JC_NATIVE_TRIAL_SHADER_H
#include "native_shader_inputs.h"
#include "native_appearance.h"
#include <math.h>
static inline int jc_trial_affine(const float m[16]) {
    for (unsigned i=0;i<16;++i) if (!isfinite(m[i])) return 0;
    return fabsf(m[3])<1e-5f && fabsf(m[7])<1e-5f && fabsf(m[11])<1e-5f && fabsf(m[15]-1)<1e-5f;
}
static inline void jc_trial_multiply(const float a[16],const float b[16],float out[16]) {
    for (unsigned c=0;c<4;++c) for (unsigned r=0;r<4;++r) {
        double sum=0;
        for (unsigned k=0;k<4;++k) sum+=(double)a[k*4+r]*b[c*4+k];
        out[c*4+r]=(float)sum;
    }
}
static inline int jc_trial_inverse(const float m[16],float out[16]) {
    double a[4][8]; float result[16],product[16];
    if (!jc_trial_affine(m)) return 4;
    for (unsigned r=0;r<4;++r) for (unsigned c=0;c<8;++c) a[r][c]=c<4 ? m[c*4+r] : (double)(c-4==r);
    for (unsigned c=0;c<4;++c) {
        unsigned pivot=c;
        for (unsigned r=c+1;r<4;++r) if (fabs(a[r][c])>fabs(a[pivot][c])) pivot=r;
        if (fabs(a[pivot][c])<1e-8) return 4;
        for (unsigned k=0;k<8;++k) { double v=a[c][k]; a[c][k]=a[pivot][k]; a[pivot][k]=v; }
        double divisor=a[c][c]; for (unsigned k=0;k<8;++k) a[c][k]/=divisor;
        for (unsigned r=0;r<4;++r) if (r!=c) {
            double factor=a[r][c]; for (unsigned k=0;k<8;++k) a[r][k]-=factor*a[c][k];
        }
    }
    for (unsigned r=0;r<4;++r) for (unsigned c=0;c<4;++c) result[c*4+r]=(float)a[r][c+4];
    if (!jc_trial_affine(result)) return 4;
    for (unsigned order=0;order<2;++order) {
        jc_trial_multiply(order ? result : m,order ? m : result,product);
        for (unsigned i=0;i<16;++i) if (!isfinite(product[i]) || fabsf(product[i]-(float)(i%5==0))>0.01f) return 4;
    }
    memcpy(out,result,sizeof(result)); return 0;
}
static inline int jc_trial_spine_mode(jc_layout_reader read,void *ctx,uint64_t model,uint64_t root,float out[16],int lookup) {
    uint64_t head,node,again,tree_root,stack[256],seen[256],bone=0; unsigned pending=0,used=0;
    if (!jc_layout_field(read,ctx,model,0xa8,&head,8) || !jc_layout_field(read,ctx,head,8,&node,8)) return 1;
    tree_root=node; stack[pending++]=node;
    while (pending) {
        node=stack[--pending]; unsigned char entry[0x48],check[0x48];
        if (!jc_layout_field(read,ctx,node,0,entry,sizeof(entry))) return 1;
        if (entry[0x19]) { if (node!=head) return 2; continue; }
        if (used==256) return 5;
        for (unsigned i=0;i<used;++i) if (seen[i]==node) return 5;
        seen[used++]=node;
        uint64_t left,right,length,capacity;
        memcpy(&left,entry,8); memcpy(&right,entry+0x10,8);
        memcpy(&length,entry+0x30,8); memcpy(&capacity,entry+0x38,8);
        if (length>capacity || capacity>4096) return 4;
        if (lookup) {
            char name[7]={0},name_check[7]={0}; uint64_t address=node+0x20;
            unsigned bytes=length<6 ? (unsigned)length+1 : length==6 ? 7 : 6;
            if (capacity>=16) memcpy(&address,entry+0x20,8);
            if (!jc_layout_field(read,ctx,address,0,name,bytes)) return 1;
            if (length<=6 && name[length]) return 2;
            int order=memcmp(name,"spineB",length<6 ? (size_t)length : 6);
            if (!order) order=length<6 ? -1 : length>6 ? 1 : 0;
            if (!order) {
                if (bone) return 2;
                memcpy(&bone,entry+0x40,8);
                if (!bone) return 2;
            }
            if (!jc_layout_field(read,ctx,node,0,check,sizeof(check)) || memcmp(entry,check,sizeof(check)) ||
                !jc_layout_field(read,ctx,address,0,name_check,bytes) || memcmp(name,name_check,bytes)) return 3;
            stack[pending++]=order<0 ? right : left;
            continue;
        }
        if (length==6) {
            char name[7]; uint64_t address=node+0x20;
            if (capacity>=16) memcpy(&address,entry+0x20,8);
            if (!jc_layout_field(read,ctx,address,0,name,sizeof(name))) return 1;
            if (!memcmp(name,"spineB",7)) {
                if (bone) return 2;
                memcpy(&bone,entry+0x40,8);
                if (!bone) return 2;
            }
        }
        if (!jc_layout_field(read,ctx,node,0,check,sizeof(check)) || memcmp(entry,check,sizeof(check))) return 3;
        if (pending>254) return 5;
        stack[pending++]=left; stack[pending++]=right;
    }
    if (!bone) return 2;
    float world[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    used=0; node=bone; int reached_root=0;
    while (node) {
        unsigned char data[0x60],check[0x60]; float local[16],next[16];
        if (used==64) return 5;
        for (unsigned i=0;i<used;++i) if (seen[i]==node) return 5;
        seen[used++]=node;
        if (!jc_layout_field(read,ctx,node,0,data,sizeof(data))) return 1;
        memcpy(local,data+0x20,64);
        if (!jc_trial_affine(local)) return 4;
        jc_trial_multiply(local,world,next); memcpy(world,next,64);
        if (node==root) reached_root=1;
        if (!jc_layout_field(read,ctx,node,0,check,sizeof(check)) || memcmp(data,check,sizeof(data))) return 3;
        memcpy(&node,data+8,8);
        if (reached_root && node) return 2;
    }
    if (!reached_root || !jc_trial_affine(world)) return 2;
    if (!jc_layout_field(read,ctx,model,0xa8,&again,8) || again!=head ||
        !jc_layout_field(read,ctx,head,8,&again,8) || again!=tree_root) return 3;
    memcpy(out,world,64); return 0;
}
static inline int jc_trial_spine(jc_layout_reader read,void *ctx,uint64_t model,uint64_t root,float out[16]) {
    return jc_trial_spine_mode(read,ctx,model,root,out,0);
}
static inline int jc_trial_spine_lookup(jc_layout_reader read,void *ctx,uint64_t model,uint64_t root,float out[16]) {
    return jc_trial_spine_mode(read,ctx,model,root,out,1);
}
typedef int (*jc_shader_writer)(void *,uint64_t,const void *,size_t);
struct jc_trial_shader_result { int stage,originals_unchanged,trial_only; };
static inline int jc_trial_shader_same(const struct jc_shader_inputs *a,const struct jc_shader_inputs *b) {
    return a->buffer==b->buffer && a->bytes==b->bytes && a->payload==b->payload &&
        a->instance==b->instance && a->shader==b->shader && a->found==b->found &&
        !memcmp(a->offsets,b->offsets,sizeof(a->offsets)) && !memcmp(a->sizes,b->sizes,sizeof(a->sizes));
}
static inline int jc_trial_shader_update(jc_layout_reader read,jc_shader_writer write,void *ctx,
    uint64_t definition,const uint64_t materials[3],const float world[16],const float previous[16],struct jc_trial_shader_result *out) {
    struct jc_shader_inputs before[3],after;
    struct jc_appearance_plan plan; struct jc_appearance_bindings storage[3],current;
    unsigned char buffers[3][4096],check[4096]; float inverse[16],center[4];
    memset(out,0,sizeof(*out)); out->originals_unchanged=out->trial_only=-1;
    if (!write) return 2;
    int code=jc_trial_inverse(world,inverse); if (code) return code;
    code=jc_appearance_plan_read(read,ctx,definition,&plan); if (code) return code;
    memcpy(center,world+12,12); center[3]=1;
    for (unsigned i=0;i<3;++i) {
        code=jc_appearance_bindings_read(read,ctx,materials[i],&plan,&storage[i]); if (code) return code;
        for (unsigned j=0;j<i;++j) if (!jc_appearance_storage_disjoint(&storage[i],&storage[j])) return 6;
        code=jc_shader_inputs_read(read,ctx,materials[i],&before[i]); if (code) return code;
        if (before[i].bytes>4096) return 4;
        if (before[i].buffer!=storage[i].buffer || before[i].bytes!=storage[i].bytes ||
            before[i].payload!=storage[i].payload || before[i].instance!=storage[i].instances[0].instance ||
            before[i].shader!=storage[i].instances[0].shader) return 3;
        for (unsigned j=0;j<i;++j) if (before[i].buffer<before[j].buffer+before[j].bytes &&
            before[j].buffer<before[i].buffer+before[i].bytes) return 6;
        if (!jc_layout_field(read,ctx,before[i].buffer,0,buffers[i],before[i].bytes)) return 1;
    }
    struct jc_shader_inputs *trial=&before[2];
    unsigned center_offset=trial->offsets[1],inverse_offset=trial->offsets[2];
    if (trial->sizes[0]!=4 || trial->sizes[1]!=16 || trial->sizes[2]!=64) return 4;
    if ((trial->found&7)!=7 || (trial->finite&7)!=7 || (trial->zero&1)!=1 ||
        (center_offset<inverse_offset+64 && inverse_offset<center_offset+16)) return 6;
    if (previous) {
        float old_inverse[16],old_center[4];
        if (jc_trial_inverse(previous,old_inverse)) return 4;
        memcpy(old_center,previous+12,12); old_center[3]=1;
        if (memcmp(buffers[2]+center_offset,old_center,16) ||
            memcmp(buffers[2]+inverse_offset,old_inverse,64)) return 3;
    } else if ((trial->zero&7)!=7) return 6;
    for (unsigned i=0;i<3;++i) {
        code=jc_shader_inputs_read(read,ctx,materials[i],&after); if (code) return code;
        if (!jc_trial_shader_same(&after,&before[i])) return 3;
        code=jc_appearance_bindings_read(read,ctx,materials[i],&plan,&current); if (code) return code;
        if (!jc_appearance_storage_same(&current,&storage[i])) return 3;
        if (!jc_layout_field(read,ctx,after.buffer,0,check,after.bytes) || memcmp(check,buffers[i],after.bytes)) return 3;
    }
    out->stage=1;
    if (!write(ctx,trial->buffer+center_offset,center,16) || !write(ctx,trial->buffer+inverse_offset,inverse,64)) return 7;
    out->stage=2;
    memcpy(buffers[2]+center_offset,center,16); memcpy(buffers[2]+inverse_offset,inverse,64);
    for (unsigned i=0;i<3;++i) {
        code=jc_shader_inputs_read(read,ctx,materials[i],&after); if (code) return code;
        code=jc_appearance_bindings_read(read,ctx,materials[i],&plan,&current); if (code) return code;
        if (!jc_appearance_storage_same(&current,&storage[i]) || !jc_trial_shader_same(&after,&before[i]) ||
            !jc_layout_field(read,ctx,after.buffer,0,check,after.bytes) || memcmp(check,buffers[i],after.bytes)) return 8;
    }
    out->originals_unchanged=out->trial_only=1; out->stage=3; return 0;
}
static inline int jc_trial_shader_write(jc_layout_reader read,jc_shader_writer write,void *ctx,
    uint64_t definition,const uint64_t materials[3],const float world[16],struct jc_trial_shader_result *out) {
    return jc_trial_shader_update(read,write,ctx,definition,materials,world,NULL,out);
}
#endif
