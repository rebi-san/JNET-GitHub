#ifndef JC_NATIVE_IDLE_POSE_H
#define JC_NATIVE_IDLE_POSE_H
#include "native_pose_writes.h"
#include "native_read_batch.h"
struct jc_idle_pose { uint64_t model, control, root; uint32_t references; };
static inline int jc_idle_context_valid(const unsigned char *context,const struct jc_idle_pose *inputs) {
    uint32_t count; memcpy(&count,context+0x10080,4);
    if (count>1) return 0;
    for (unsigned i=0;i<4096;++i) {
        uint64_t pair[2]; memcpy(pair,context+0x80+i*16,sizeof(pair));
        if (i<count) {
            if (pair[0]!=inputs->model || pair[1]!=inputs->control) return 0;
        } else if (pair[0] || pair[1]) return 0;
    }
    return 1;
}
static inline int jc_idle_floats(jc_layout_reader read,void *context,uint64_t address,unsigned count) {
    uint32_t values[16];
    if (!count || count>16 || !jc_layout_field(read,context,address,0,values,count*4)) return 0;
    for (unsigned i=0;i<count;++i) if ((values[i]&0x7f800000u)==0x7f800000u) return 0;
    return 1;
}
struct jc_idle_detail { int field,index; };
static inline int jc_idle_span_floats(void *ctx,unsigned id,const unsigned char *bytes,size_t size) {
    struct jc_idle_detail *detail=ctx; detail->field=12+(int)(id%4); detail->index=(int)(id/4);
    for (unsigned i=0;i<size/4;++i) {
        uint32_t value; memcpy(&value,bytes+i*4,4);
        if ((value&0x7f800000u)==0x7f800000u) return 8;
    }
    return 0;
}
static inline int jc_idle_matrix_inputs(jc_layout_reader read,void *ctx,uint64_t model,unsigned kind,
        struct jc_idle_detail *detail) {
    const unsigned offsets[]={0xc8,0xe0,0xd0},counts[]={0xe8,0xf0,0xec};
    uint64_t table,again,end; uint32_t count,count_again;
    unsigned char rows[1024*32],check[1024*32]; struct jc_pose_span spans[2048]; unsigned used=0;
    detail->field=11; detail->index=(int)kind;
    if (!jc_layout_field(read,ctx,model,offsets[kind],&table,8) ||
        !jc_layout_field(read,ctx,model,counts[kind],&count,4)) return 1;
    if (count>1024) return 5;
    unsigned stride=kind ? 16 : 32;
    if (count && !jc_layout_field(read,ctx,table,0,rows,count*stride)) return 1;
    for (unsigned i=0;i<count;++i) {
        uint64_t pair[2]; memcpy(pair,rows+i*stride,16);
        detail->field=12; detail->index=(int)i;
        if (!jc_layout_address(pair[1],64,&end)) return 8;
        spans[used++]=(struct jc_pose_span){pair[1],64,i*4};
        if (!kind && i) {
            detail->field=13;
            if (pair[0]<table || (pair[0]-table)%32 || (pair[0]-table)/32>=i) return 6;
            uint64_t matrix; memcpy(&matrix,rows+(size_t)(pair[0]-table)+24,8);
            if (!jc_layout_address(matrix,64,&end)) return 8;
            spans[used++]=(struct jc_pose_span){matrix,64,i*4+1};
        }
        if (kind==2) {
            detail->field=14;
            if (!jc_layout_address(pair[0],36,&end)) return 8;
            spans[used++]=(struct jc_pose_span){pair[0],36,i*4+2};
        }
    }
    int code=jc_read_spans(read,ctx,spans,used,jc_idle_span_floats,detail);
    if (code) return 8;
    if (!jc_layout_field(read,ctx,model,offsets[kind],&again,8) ||
        !jc_layout_field(read,ctx,model,counts[kind],&count_again,4) ||
        (count && !jc_layout_field(read,ctx,table,0,check,count*stride))) return 1;
    return again!=table || count_again!=count || memcmp(rows,check,count*stride) ? 3 : 0;
}
static inline int jc_idle_object_inputs(jc_layout_reader read,void *context,
        const uint64_t *descriptors,const uint64_t *states,unsigned count) {
    struct jc_pose_span spans[2048]; unsigned used=0;
    unsigned char scratch[4096]; uint64_t end;
    for (unsigned i=0;i<count;++i) {
        if (!jc_layout_address(descriptors[i],8,&end) || !jc_layout_address(states[i],0x28,&end)) return 0;
        spans[used++]=(struct jc_pose_span){descriptors[i],8,0};
        spans[used++]=(struct jc_pose_span){states[i],0x28,0};
    }
    jc_pose_sort(spans,used);
    for (unsigned i=0;i<used;) {
        unsigned last=i+1; uint64_t start=spans[i].address;
        end=start+spans[i].size;
        while (last<used && spans[last].address+spans[last].size-start<=sizeof(scratch)) {
            if (spans[last].address+spans[last].size>end) end=spans[last].address+spans[last].size;
            ++last;
        }
        if (!read(context,start,scratch,(size_t)(end-start)))
            for (unsigned j=i;j<last;++j) if (!read(context,spans[j].address,scratch,spans[j].size)) return 0;
        i=last;
    }
    return 1;
}
/* Necessary inputs for one inactive-weight CPU update, not a general animation adapter. */
static inline int jc_mesh_pose_ready_detail(jc_layout_reader read,void *context,uint64_t image,
                                    uint64_t mesh,struct jc_idle_pose *out,struct jc_idle_detail *detail,int active) {
    *detail=(struct jc_idle_detail){1,-1};
    memset(out,0,sizeof(*out));
    uint32_t threshold;
    if (!jc_layout_field(read,context,image,0x3763e7c,&threshold,4)) return 1;
    if (!threshold || (threshold&0x80000000u) || (threshold&0x7f800000u)==0x7f800000u) return 6;
    detail->field=2;
    struct jc_pose_storage pose,after;
    int code=jc_pose_storage_read(read,context,image,mesh,&pose);
    if (code) return code;
    detail->field=3;
    uint64_t entries,entry_count,descriptors,states,descriptor_count,state_count,control;
    uint32_t references,scale;
    unsigned char mesh_fields[24],payload_fields[0x50];
    if (!jc_layout_field(read,context,mesh,0x570,mesh_fields,sizeof(mesh_fields)) ||
        !jc_layout_field(read,context,pose.clone.payload,0,payload_fields,sizeof(payload_fields))) return 1;
    memcpy(&scale,mesh_fields,4); memcpy(&entries,mesh_fields+8,8); memcpy(&entry_count,mesh_fields+16,8);
    memcpy(&control,payload_fields+8,8); memcpy(&descriptors,payload_fields+0x30,8);
    memcpy(&descriptor_count,payload_fields+0x38,8); memcpy(&states,payload_fields+0x40,8);
    memcpy(&state_count,payload_fields+0x48,8);
    if (!jc_layout_field(read,context,control,0,&references,4)) return 1;
    if (!entry_count || entry_count>1024 || entry_count!=descriptor_count || state_count<descriptor_count || state_count>1024 ||
        !references || references>0x7ffffff0u) return 5;
    detail->field=4;
    if (scale&0x7fffffff) return 6;
    uint64_t descriptor_rows[1024],state_rows[1024],rows_check[1024];
    unsigned char entry_rows[16*0x50],entry_fields[1024][24];
    detail->field=9;
    if (!jc_layout_field(read,context,descriptors,0,descriptor_rows,(size_t)descriptor_count*8) ||
        !jc_layout_field(read,context,states,0,state_rows,(size_t)state_count*8)) return 1;
    for (unsigned i=0;i<entry_count;++i) {
        detail->field=5; detail->index=(int)i;
        if (!(i%16)) {
            unsigned n=entry_count-i>16 ? 16 : (unsigned)entry_count-i;
            if (!jc_layout_field(read,context,entries,(uint64_t)i*0x50,entry_rows,(n-1)*0x50+24)) return 1;
        }
        const unsigned char *entry=entry_rows+(i%16)*0x50;
        memcpy(entry_fields[i],entry,24);
        uint32_t index,weight;
        memcpy(&index,entry+20,4); memcpy(&weight,entry+16,4);
        detail->field=6;
        if (index>=descriptor_count || index>=state_count) return 7;
        detail->field=7;
        if (entry[1] || entry[0]>1 || entry[2]>1 || (!active && (entry[0] || (weight&0x7fffffff)))) return 6;
        detail->field=8;
        for (unsigned f=0;f<4;++f) {
            uint32_t bits; memcpy(&bits,entry+4+f*4,4);
            if ((bits&0x7f800000u)==0x7f800000u) return 8;
        }
    }
    detail->field=9; detail->index=-1;
    if (!jc_idle_object_inputs(read,context,descriptor_rows,state_rows,(unsigned)descriptor_count)) return 1;
    detail->field=10; detail->index=-1;
    if (!jc_idle_floats(read,context,mesh+0x530,16)) return 8;
    /* Sources used by the two matrix loops and the bounds-only math helper. */
    for (unsigned kind=0;kind<3;++kind) {
        code=jc_idle_matrix_inputs(read,context,pose.model,kind,detail);
        if (code) return code;
    }
    detail->field=15; detail->index=-1;
    unsigned char mesh_check[24],payload_check[0x50];
    if (!jc_layout_field(read,context,mesh,0x570,mesh_check,sizeof(mesh_check)) ||
        !jc_layout_field(read,context,pose.clone.payload,0,payload_check,sizeof(payload_check))) return 1;
    if (memcmp(mesh_fields,mesh_check,sizeof(mesh_fields)) || memcmp(payload_fields,payload_check,sizeof(payload_fields))) return 3;
    if (!jc_layout_field(read,context,descriptors,0,rows_check,(size_t)descriptor_count*8)) return 1;
    if (memcmp(descriptor_rows,rows_check,(size_t)descriptor_count*8)) return 3;
    if (!jc_layout_field(read,context,states,0,rows_check,(size_t)state_count*8)) return 1;
    if (memcmp(state_rows,rows_check,(size_t)state_count*8)) return 3;
    for (unsigned i=0;i<entry_count;i+=16) {
        unsigned n=entry_count-i>16 ? 16 : (unsigned)entry_count-i;
        if (!jc_layout_field(read,context,entries,(uint64_t)i*0x50,entry_rows,(n-1)*0x50+24)) return 1;
        for (unsigned j=0;j<n;++j) if (memcmp(entry_fields[i+j],entry_rows+j*0x50,24)) return 3;
    }
    code=jc_pose_storage_read(read,context,image,mesh,&after);
    if (code) return code;
    if (!jc_clone_snapshot_equal(&pose.clone,&after.clone) || pose.model!=after.model || pose.root!=after.root) return 3;
    detail->field=0;
    out->model=pose.model; out->control=control; out->root=pose.root; out->references=references;
    return 0;
}
static inline int jc_idle_pose_ready_detail(jc_layout_reader read,void *context,uint64_t image,
                                    uint64_t mesh,struct jc_idle_pose *out,struct jc_idle_detail *detail) {
    return jc_mesh_pose_ready_detail(read,context,image,mesh,out,detail,0);
}
static inline int jc_idle_pose_ready(jc_layout_reader read,void *context,uint64_t image,
                                    uint64_t mesh,struct jc_idle_pose *out) {
    struct jc_idle_detail detail;
    return jc_idle_pose_ready_detail(read,context,image,mesh,out,&detail);
}
#endif
