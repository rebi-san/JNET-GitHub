#ifndef JC_NATIVE_EVALUATION_RUN_H
#define JC_NATIVE_EVALUATION_RUN_H
#include "native_evaluation_layout.h"
typedef int (*jc_eval_writable)(void *,uint64_t,size_t);
enum { JC_EVAL_RUN_ARGUMENT=10,JC_EVAL_RUN_SCHEDULER=11,JC_EVAL_RUN_BUSY=12,
    JC_EVAL_RUN_SPACE=13,JC_EVAL_RUN_CLOCK=14,JC_EVAL_RUN_GRAPH=15,JC_EVAL_RUN_WRITE=16,JC_EVAL_RUN_WAIT=17 };
struct jc_eval_run_result {
    uint64_t mesh,job,model,control,scheduler;
    unsigned stage,detail,nodes,bindings,clocks,scheduler_kind,scratch_bytes,temp_bytes;
    unsigned output[8];
    int ready;
};
static inline int jc_eval_run_save(jc_layout_reader read,void *ctx,struct jc_eval_layout_workspace *w,
        uint64_t at,unsigned size,const unsigned char **p) {
    return jc_eval_save(read,ctx,w,at,size,p);
}
static inline int jc_eval_run_write(jc_layout_reader read,jc_eval_writable writable,void *ctx,
        struct jc_eval_layout_workspace *w,uint64_t at,unsigned size,unsigned owner) {
    int code=jc_eval_readable(read,ctx,at,size); if (code) return code;
    if (!writable(ctx,at,size)) return JC_EVAL_RUN_WRITE;
    return jc_eval_region_add(w,at,size,owner,JC_EVAL_STORAGE);
}
static inline int jc_eval_run_environment(jc_layout_reader read,jc_eval_writable writable,void *ctx,
        uint64_t image,unsigned owner,unsigned items,const unsigned char job[0x78],
        struct jc_eval_run_result *out,struct jc_eval_layout_workspace *w) {
    const unsigned char *p,*scheduler,*ring; int code;
    out->stage=2; out->detail=1;
    code=jc_eval_run_save(read,ctx,w,image+0x3c0aee0,8,&p); if (code) return code;
    out->scheduler=jc_eval_u64(p);
    if (!out->scheduler || (out->scheduler&7)) return JC_EVAL_RUN_SCHEDULER;
    if ((code=jc_eval_region_add(w,image+0x3c0aee0,8,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,out->scheduler,0x30,owner,JC_EVAL_META))) return code;
    code=jc_eval_run_save(read,ctx,w,out->scheduler,0x30,&scheduler); if (code) return code;
    uint64_t vtable=jc_eval_u64(scheduler),scratch=jc_eval_u64(scheduler+0x18);
    out->scheduler_kind=vtable==image+0x599a68 ? 1 : vtable==image+0x599ba0 ? 2 : 0;
    if (!out->scheduler_kind) return JC_EVAL_RUN_SCHEDULER;
    code=jc_eval_region_add(w,vtable+0x20,0x20,owner,JC_EVAL_META); if (code) return code;
    code=jc_eval_run_save(read,ctx,w,vtable+0x20,0x20,&p); if (code) return code;
    const unsigned single[]={0x411780,0x4117b0,0x4117d0,0x4117e0};
    const unsigned threaded[]={0x412020,0x412040,0x412060,0x412130};
    const unsigned *expected=out->scheduler_kind==1 ? single : threaded;
    for (unsigned i=0;i<4;++i) if (jc_eval_u64(p+i*8)!=image+expected[i]) return JC_EVAL_RUN_SCHEDULER;
    if (out->scheduler_kind==2) {
        code=jc_eval_run_save(read,ctx,w,out->scheduler+0x78,0x20,&p); if (code) return code;
        if (jc_eval_u32(p+0x10) || p[0x18]!=1 || p[0x19]!=1) return JC_EVAL_RUN_BUSY;
        code=jc_eval_region_add(w,out->scheduler+0x78,0x20,owner,JC_EVAL_META); if (code) return code;
    }
    out->detail=2;
    unsigned slots=jc_eval_u32(job+0x58)&0x7fffffff;
    uint64_t needed=(uint64_t)items*slots*48;
    out->scratch_bytes=jc_eval_u32(scheduler+0x20);
    if (!items || items>JC_EVAL_CAP || !slots || slots>JC_EVAL_CAP || (scratch&127) ||
        needed>out->scratch_bytes || out->scratch_bytes>64u*1024u*1024u) return JC_EVAL_RUN_SPACE;
    code=jc_eval_run_write(read,writable,ctx,w,scratch,(unsigned)needed,owner); if (code) return code;
    out->detail=3;
    code=jc_eval_run_save(read,ctx,w,image+0x3c03e98,8,&p); if (code) return code;
    if (jc_eval_u32(p+4)) return JC_EVAL_RUN_BUSY;
    if (jc_eval_u32(job+0x38)>jc_eval_u32(p)) return JC_EVAL_RUN_SPACE;
    if ((code=jc_eval_region_add(w,image+0x3c03e98,4,owner,JC_EVAL_META)) ||
        (code=jc_eval_run_write(read,writable,ctx,w,image+0x3c03e9c,4,owner))) return code;
    out->detail=4;
    code=jc_eval_run_save(read,ctx,w,image+0x3c214d0+0x18,0x10,&p); if (code) return code;
    if (jc_eval_u32(p) || jc_eval_u32(p+8) || jc_eval_u32(p+12)) return JC_EVAL_RUN_BUSY;
    if (!writable(ctx,image+0x3c214d0+0x18,0x10)) return JC_EVAL_RUN_WRITE;
    if ((code=jc_eval_region_add(w,image+0x3c214d0,0x18,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,image+0x3c214d0+0x18,0x10,owner,JC_EVAL_STORAGE)) ||
        (code=jc_eval_region_add(w,image+0x3c214d0+0x28,0x18,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,image+0x3c214d0+0x40,0x18,owner,JC_EVAL_STORAGE))) return code;
    code=jc_eval_run_save(read,ctx,w,image+0x3c214d0+0x28,0x30,&ring); if (code) return code;
    uint64_t begin=jc_eval_u64(ring),end=jc_eval_u64(ring+8),head=jc_eval_u64(ring+0x18),tail=jc_eval_u64(ring+0x20);
    unsigned capacity=jc_eval_u32(ring+0x10),temporary=jc_eval_u32(job+0x50);
    out->detail=5;
    if (temporary>16u*1024u*1024u || end<=begin || end-begin!=capacity ||
        ((begin|end|head|tail)&15) || head<begin || head>=end || tail<begin || tail>=end) return JC_EVAL_RUN_SPACE;
    out->temp_bytes=((temporary+15u)&~15u)+16u;
    out->detail=6;
    if (out->temp_bytes>=capacity) return JC_EVAL_RUN_SPACE;
    /* 3de3c0: never enter native waiting/reclamation. 3de355 also permits
       exact-end allocations and writes a 16-byte padding header on wrap. */
    if ((head<tail && out->temp_bytes>=tail-head) ||
        (out->temp_bytes>=end-head && tail==begin)) return JC_EVAL_RUN_WAIT;
    if (out->temp_bytes>end-head) {
        if (out->temp_bytes>=tail-begin) return JC_EVAL_RUN_WAIT;
        code=jc_eval_run_write(read,writable,ctx,w,head,16,owner); if (code) return code;
        code=jc_eval_run_write(read,writable,ctx,w,begin,out->temp_bytes,owner); if (code) return code;
    } else {
        code=jc_eval_run_write(read,writable,ctx,w,head,out->temp_bytes,owner); if (code) return code;
    }
    if (!writable(ctx,image+0x3c214d0+0x40,0x18)) return JC_EVAL_RUN_WRITE;
    out->detail=7;
    code=jc_eval_run_save(read,ctx,w,image+0x3c03ea8,8,&p); if (code) return code;
    code=jc_eval_region_add(w,image+0x3c03ea8,8,owner,JC_EVAL_META); if (code) return code;
    uint64_t pool=jc_eval_u64(p);
    if (!pool || (pool&7)) return JC_EVAL_RUN_SPACE;
    code=jc_eval_run_save(read,ctx,w,pool,8,&p); if (code) return code;
    uint64_t task=jc_eval_u64(p);
    if (!task || (task&7)) return JC_EVAL_RUN_SPACE;
    if ((code=jc_eval_run_save(read,ctx,w,pool+8,0x18,&p)) ||
        (code=jc_eval_region_add(w,pool+8,0x18,owner,JC_EVAL_META)) ||
        (code=jc_eval_run_save(read,ctx,w,task,8,&p)) ||
        (code=jc_eval_run_write(read,writable,ctx,w,pool,8,owner)) ||
        (code=jc_eval_run_write(read,writable,ctx,w,task,0x20,owner))) return code;
    return 0;
}
static inline int jc_eval_run_clocks(jc_layout_reader read,void *ctx,
        struct jc_eval_layout_workspace *w,uint64_t payload,struct jc_eval_run_result *out) {
    const unsigned char *p,*table; int code;
    out->stage=3; out->detail=0;
    code=jc_eval_run_save(read,ctx,w,payload+0x40,16,&p); if (code) return code;
    uint64_t states=jc_eval_u64(p),count=jc_eval_u64(p+8);
    if (!count || count>JC_EVAL_CAP) return JC_EVAL_RUN_CLOCK;
    code=jc_eval_run_save(read,ctx,w,states,(unsigned)count*8,&table); if (code) return code;
    for (unsigned i=0;i<count;++i) {
        out->detail=i;
        code=jc_eval_run_save(read,ctx,w,jc_eval_u64(table+i*8),0x28,&p); if (code) return code;
        float values[6]; memcpy(values,p+0x10,sizeof(values));
        for (unsigned j=0;j<6;++j) if (!isfinite(values[j])) return JC_EVAL_RUN_CLOCK;
        float span=values[5]-values[4];
        if (!isfinite(span) || span<0.0001f || span>86400 || fabsf(values[0])>2*span ||
            fabsf(values[2])>1000 || (values[3]<0 && values[3]!=-1)) return JC_EVAL_RUN_CLOCK;
        ++out->clocks;
    }
    return 0;
}
static inline int jc_eval_run_outputs(jc_layout_reader read,jc_eval_writable writable,void *ctx,
        uint64_t image,unsigned owner,const unsigned char job[0x78],struct jc_eval_run_result *out,
        struct jc_eval_layout_workspace *w,unsigned *items_out) {
    const unsigned char *p,*root,*definition,*values,*bindings,*pairs[4]={0}; int code;
    unsigned pair_count[4]={0};
    unsigned char referenced[JC_EVAL_CAP]={0};
    out->stage=4; out->detail=1;
    uint64_t root_at=jc_eval_u64(job+0x30),definition_at=jc_eval_u64(job+0x28);
    if ((root_at|definition_at)&7) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_run_save(read,ctx,w,root_at,0x20,&root); if (code) return code;
    if (jc_eval_u64(root)!=image+0x577b80 || jc_eval_u64(root+0x10)!=definition_at ||
        !jc_eval_u64(root+0x18)) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_run_save(read,ctx,w,image+0x577b80+0x60,8,&p); if (code) return code;
    if (jc_eval_u64(p)!=image+0x395d10) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_region_add(w,image+0x577b80+0x60,8,owner,JC_EVAL_META); if (code) return code;
    code=jc_eval_run_save(read,ctx,w,definition_at,0x30,&definition); if (code) return code;
    unsigned items=jc_eval_u32(definition+0x20)&0x7fffffff;
    unsigned count=jc_eval_u32(job+0x18)&0x7fffffff;
    out->output[0]=items; out->output[1]=count;
    if (!items || items>JC_EVAL_CAP || !count || count>JC_EVAL_CAP) return JC_EVAL_RUN_GRAPH;
    uint64_t values_at=jc_eval_u64(definition+0x28),bindings_at=jc_eval_u64(job+0x20);
    if ((values_at|bindings_at)&7) return JC_EVAL_RUN_GRAPH;
    if ((code=jc_eval_run_save(read,ctx,w,values_at,items*24,&values)) ||
        (code=jc_eval_run_save(read,ctx,w,bindings_at,count*0x30,&bindings)) ||
        (code=jc_eval_region_add(w,root_at,0x20,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,definition_at,0x30,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,values_at,items*24,owner,JC_EVAL_META))) return code;
    unsigned linked=0;
    for (unsigned original=0;original<2;++original) {
        uint64_t original_job,original_root,original_definition;
        code=jc_eval_run_save(read,ctx,w,w->skeleton[original].storage.clone.payload+0x18,8,&p); if (code) return code;
        original_job=jc_eval_u64(p);
        code=jc_eval_run_save(read,ctx,w,original_job+0x30,8,&p); if (code) return code;
        original_root=jc_eval_u64(p);
        code=jc_eval_region_add(w,original_root,0x20,original,JC_EVAL_META); if (code) return code;
        code=jc_eval_run_save(read,ctx,w,original_root+0x10,8,&p); if (code) return code;
        original_definition=jc_eval_u64(p);
        linked+=original_definition==definition_at;
    }
    if (!linked) return JC_EVAL_RUN_GRAPH;
    for (unsigned o=0;o<4;++o) {
        out->detail=28*JC_EVAL_CAP+o;
        code=jc_eval_run_save(read,ctx,w,w->skeleton[o].storage.model+0xb8,12,&p); if (code) return code;
        uint64_t at=jc_eval_u64(p); pair_count[o]=jc_eval_u32(p+8);
        if (pair_count[o]>JC_EVAL_CAP || (at&7)) return JC_EVAL_RUN_GRAPH;
        if (pair_count[o]) {
            code=jc_eval_run_save(read,ctx,w,at,pair_count[o]*16,pairs+o); if (code) return code;
            code=jc_eval_region_add(w,at,pair_count[o]*16,o,JC_EVAL_META); if (code) return code;
        }
    }
    for (unsigned i=0;i<items;++i) {
        for (unsigned j=2;j<8;++j) out->output[j]=UINT32_MAX;
        out->detail=20*JC_EVAL_CAP+i;
        unsigned index=jc_eval_u32(values+i*24+0x10);
        out->output[2]=index;
        if (index>=items) return JC_EVAL_RUN_GRAPH;
        out->detail=21*JC_EVAL_CAP+i;
        unsigned binding=jc_eval_u32(values+index*24+0xc);
        out->output[3]=binding;
        if (binding>=count) return JC_EVAL_RUN_GRAPH;
        referenced[binding]=1;
        out->output[4]=jc_eval_u32(bindings+binding*0x30);
        out->output[6]=jc_eval_u32(bindings+binding*0x30+0x10);
        if (jc_eval_u32(bindings+binding*0x30)) {
            out->detail=22*JC_EVAL_CAP+i;
            uint64_t type=jc_eval_u64(values+index*24);
            out->output[5]=!type ? 0 : type>=image && type-image<=UINT32_MAX ? (unsigned)(type-image) : UINT32_MAX;
            if (type && type!=image+0x72c6e8 && type!=image+0x72c700 && type!=image+0x72c718) return JC_EVAL_RUN_GRAPH;
        }
    }
    for (unsigned i=0;i<count;++i) {
        const unsigned char *b=bindings+i*0x30;
        if (!referenced[i]) continue;
        unsigned kind=jc_eval_u32(b); if (!kind) continue;
        out->output[2]=out->output[5]=UINT32_MAX;
        out->output[7]=UINT32_MAX;
        out->output[3]=i; out->output[4]=kind; out->output[6]=jc_eval_u32(b+0x10);
        out->detail=23*JC_EVAL_CAP+i;
        if (kind!=1) return JC_EVAL_RUN_GRAPH;
        out->detail=24*JC_EVAL_CAP+i;
        uint64_t destination=jc_eval_u64(b+8);
        out->output[7]=0;
        for (unsigned o=0;o<4;++o) {
            unsigned match=destination==w->skeleton[o].storage.root+0x20;
            for (unsigned j=0;j<w->skeleton[o].names;++j)
                match|=destination==w->skeleton[o].bones[j].bone+0x20;
            if (match) out->output[7]|=1u<<o;
            for (unsigned j=0;j<pair_count[o];++j) {
                const unsigned char *pair=pairs[o]+j*16;
                if (destination!=jc_eval_u64(pair+8)) continue;
                uint64_t target=jc_eval_u64(pair);
                unsigned owned=target==w->skeleton[o].storage.root+0x20;
                for (unsigned k=0;k<w->skeleton[o].names;++k)
                    owned|=target==w->skeleton[o].bones[k].bone+0x20;
                if (owned) out->output[7]|=1u<<(o+4);
            }
        }
        unsigned found=out->output[7]&((1u<<owner)|(1u<<(owner+4)));
        if (!found) return JC_EVAL_RUN_GRAPH;
        out->detail=25*JC_EVAL_CAP+i;
        if (destination&15) return JC_EVAL_RUN_GRAPH;
        out->detail=26*JC_EVAL_CAP+i;
        if (!writable(ctx,destination,64)) return JC_EVAL_RUN_WRITE;
        code=jc_eval_region_add(w,destination,64,owner,JC_EVAL_DESTINATION); if (code) return code;
        ++out->bindings;
    }
    out->detail=27*JC_EVAL_CAP;
    if (!out->bindings) return JC_EVAL_RUN_GRAPH;
    *items_out=items;
    out->detail=0;
    return 0;
}
static inline int jc_eval_run_execution(jc_layout_reader read,void *ctx,const unsigned char job[0x78],
        unsigned items,struct jc_eval_run_result *out,struct jc_eval_layout_workspace *w) {
    const unsigned char *execution,*records; int code;
    out->stage=5; out->detail=1;
    unsigned nodes=jc_eval_u32(job+8)&0x7fffffff,slots=jc_eval_u32(job+0x58)&0x7fffffff;
    unsigned scratch=(jc_eval_u32(job+0x40)&0x7fffffff)*16;
    if (!nodes || nodes>JC_EVAL_CAP || !slots || slots>JC_EVAL_CAP ||
        jc_eval_u32(job+0x38)!=(uint64_t)items*slots) return JC_EVAL_RUN_GRAPH;
    if ((code=jc_eval_run_save(read,ctx,w,jc_eval_u64(job+0x68),0x48+nodes*0xa8,&execution)) ||
        (code=jc_eval_run_save(read,ctx,w,jc_eval_u64(job+0x10),nodes*0x98,&records))) return code;
    if (jc_eval_u64(execution+8) || jc_eval_u64(job+0x70)) return JC_EVAL_RUN_BUSY;
    if (jc_eval_u64(execution+0x10)!=jc_eval_u64(job+0x48) || jc_eval_u32(execution+0x18)!=scratch ||
        jc_eval_u32(execution+0x28)!=items*48 || jc_eval_u32(execution+0x2c)!=items ||
        jc_eval_u32(execution+0x30)!=slots || jc_eval_u32(execution+0x40)!=nodes) return JC_EVAL_RUN_GRAPH;
    const unsigned char *definition;
    code=jc_eval_run_save(read,ctx,w,jc_eval_u64(job+0x28)+0x28,8,&definition); if (code) return code;
    if (jc_eval_u64(execution+0x38) && jc_eval_u64(execution+0x38)!=jc_eval_u64(definition)) return JC_EVAL_RUN_GRAPH;
    for (unsigned i=0;i<nodes;++i) {
        const unsigned char *r=records+i*0x98,*e=execution+0x48+(nodes-1-i)*0xa8;
        unsigned dependencies=jc_eval_u32(r+8);
        if (dependencies>32 || jc_eval_u32(e+0x10)!=dependencies ||
            memcmp(e+0x14,r+0xc,dependencies*4) || jc_eval_u32(e+0x94)!=jc_eval_u32(r+0x8c) ||
            jc_eval_u32(e+0xa4)!=jc_eval_u32(r+0x90)) return JC_EVAL_RUN_GRAPH;
    }
    out->nodes=nodes;
    return 0;
}
static inline int jc_eval_run_controls(jc_layout_reader read,jc_eval_writable writable,void *ctx,
        unsigned owner,struct jc_eval_run_result *out,struct jc_eval_layout_workspace *w) {
    const unsigned char *p; int code;
    out->stage=9;
    for (unsigned i=0;i<4;++i) {
        out->detail=4+i;
        code=jc_eval_run_save(read,ctx,w,w->skeleton[i].storage.clone.payload+8,8,&p); if (code) return code;
        uint64_t control=jc_eval_u64(p);
        if (!control) return JC_EVAL_RUN_GRAPH;
        out->detail=8+i;
        if (control&3) return JC_EVAL_RUN_GRAPH;
        out->detail=12+i;
        code=jc_eval_run_save(read,ctx,w,control,4,&p); if (code) return code;
        unsigned references=jc_eval_u32(p);
        out->detail=16+i;
        if (!references || references>=0x7ffffffe) return JC_EVAL_RUN_GRAPH;
        out->detail=20+i;
        if (i==owner) {
            out->control=control;
            code=jc_eval_run_write(read,writable,ctx,w,control,4,owner);
        } else code=jc_eval_region_add(w,control,4,i,JC_EVAL_META);
        if (code) return code;
    }
    return 0;
}
static inline int jc_eval_run_recheck(jc_layout_reader read,void *ctx,uint64_t image,
        struct jc_eval_layout_workspace *w) {
    for (unsigned i=0;i<4;++i) {
        int code=jc_skel_live(read,ctx,image,w->skeleton+i); if (code) return code;
    }
    return jc_eval_saved_recheck(read,ctx,w);
}
static inline unsigned jc_eval_run_u16(const unsigned char *p) { uint16_t n; memcpy(&n,p,2); return n; }
static inline int jc_eval_run_private(const struct jc_eval_layout_workspace *w,uint64_t at,unsigned size,unsigned owner) {
    for (unsigned i=0;i<w->region_count;++i) {
        const struct jc_eval_region *r=w->regions+i;
        if (r->owner!=owner && jc_skel_overlap(at,size,r->address,r->size)) return JC_EVAL_RUN_GRAPH;
    }
    return 0;
}
static inline int jc_eval_run_sampler(jc_layout_reader read,void *ctx,uint64_t image,unsigned owner,
        const unsigned char *node,unsigned items,struct jc_eval_layout_workspace *w,unsigned *temporary) {
    const unsigned char *p,*descriptors,*states,*descriptor,*packed; int code;
    uint64_t clip=jc_eval_u64(node+0x18),state=jc_eval_u64(node+0x20);
    if (!clip || !state || ((clip|state)&7)) return JC_EVAL_RUN_GRAPH;
    uint64_t payload=w->skeleton[owner].storage.clone.payload;
    code=jc_eval_run_save(read,ctx,w,payload+0x30,0x20,&p); if (code) return code;
    uint64_t count=jc_eval_u64(p+8),states_count=jc_eval_u64(p+24);
    if (!count || count>JC_EVAL_CAP || count!=states_count) return JC_EVAL_RUN_GRAPH;
    uint64_t descriptor_table=jc_eval_u64(p),state_table=jc_eval_u64(p+16);
    if ((code=jc_eval_run_save(read,ctx,w,descriptor_table,(unsigned)count*8,&descriptors)) ||
        (code=jc_eval_run_save(read,ctx,w,state_table,(unsigned)count*8,&states)) ||
        (code=jc_eval_region_add(w,descriptor_table,(unsigned)count*8,owner,JC_EVAL_META))) return code;
    unsigned paired=0;
    for (unsigned i=0;i<count;++i) paired+=jc_eval_u64(descriptors+i*8)==clip && jc_eval_u64(states+i*8)==state;
    if (paired!=1) return JC_EVAL_RUN_GRAPH;
    unsigned original_match=0;
    for (unsigned original=0;original<2;++original) {
        code=jc_eval_run_save(read,ctx,w,w->skeleton[original].storage.clone.payload+0x30,16,&p); if (code) return code;
        uint64_t table=jc_eval_u64(p),n=jc_eval_u64(p+8);
        if (!n || n>JC_EVAL_CAP) return JC_EVAL_RUN_GRAPH;
        code=jc_eval_region_add(w,table,(unsigned)n*8,original,JC_EVAL_META); if (code) return code;
        code=jc_eval_run_save(read,ctx,w,table,(unsigned)n*8,&p); if (code) return code;
        for (unsigned i=0;i<n;++i) original_match+=jc_eval_u64(p+i*8)==clip;
    }
    if (!original_match) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_run_save(read,ctx,w,state,8,&p); if (code) return code;
    if (jc_eval_u64(p)!=image+0x67f3b0) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_run_save(read,ctx,w,image+0x67f3b0+0x20,8,&p); if (code) return code;
    if (jc_eval_u64(p)!=image+0x25fab0) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_region_add(w,image+0x67f3b0+0x20,8,owner,JC_EVAL_META); if (code) return code;
    code=jc_eval_run_save(read,ctx,w,clip,0x28,&descriptor); if (code) return code;
    unsigned curves=jc_eval_u32(descriptor+8)&0x7fffffff,constants=jc_eval_u32(descriptor+0x18)&0x7fffffff;
    uint64_t packed_at=jc_eval_u64(descriptor),constant_at=jc_eval_u64(descriptor+0x20);
    if (curves>items || constants>JC_EVAL_CAP || (packed_at&7)) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_run_save(read,ctx,w,packed_at,8,&p); if (code) return code;
    unsigned bytes=jc_eval_run_u16(p),groups=jc_eval_run_u16(p+6);
    if (jc_eval_run_u16(p+2)!=curves || jc_eval_run_u16(p+4)!=constants ||
        groups>JC_EVAL_CAP || (curves && !groups)) return JC_EVAL_RUN_GRAPH;
    unsigned mapping=8+((groups+3)&~3u)*2;
    unsigned streams=(mapping+(curves+constants)*4+7)&~7u;
    if (bytes<mapping+(curves+constants)*4) return JC_EVAL_RUN_GRAPH;
    unsigned extent=streams+curves*24; if (extent<bytes) extent=bytes;
    if ((code=jc_eval_run_save(read,ctx,w,packed_at,extent,&packed)) ||
        (code=jc_eval_region_add(w,clip,0x28,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,packed_at,extent,owner,JC_EVAL_META))) return code;
    unsigned sum=0;
    for (unsigned i=0;i<groups;++i) {
        unsigned group=jc_eval_run_u16(packed+8+i*2);
        if (sum>=curves || group>curves-sum) return JC_EVAL_RUN_GRAPH;
        unsigned stride=jc_eval_u32(packed+streams+sum*24+16)>>24;
        for (unsigned j=1;j<group;++j)
            if (jc_eval_u32(packed+streams+(sum+j)*24+16)>>24!=stride) return JC_EVAL_RUN_GRAPH;
        sum+=group;
    }
    if (sum!=curves) return JC_EVAL_RUN_GRAPH;
    for (unsigned i=0;i<curves+constants;++i)
        if (jc_eval_run_u16(packed+mapping+i*4)>=items || jc_eval_run_u16(packed+mapping+i*4+2)>2) return JC_EVAL_RUN_GRAPH;
    if (constants) {
        if ((code=jc_eval_readable(read,ctx,constant_at,constants*0x58)) ||
            (code=jc_eval_region_add(w,constant_at,constants*0x58,owner,JC_EVAL_META))) return code;
    }
    for (unsigned i=0;i<curves;++i) {
        const unsigned char *stream=packed+streams+i*24;
        uint64_t times=jc_eval_u64(stream),values=jc_eval_u64(stream+8);
        unsigned field=jc_eval_u32(stream+16),keys=field&0xffffff,stride=field>>24;
        if (keys<2 || keys>65535 || !stride || stride>16 || ((times|values)&3)) return JC_EVAL_RUN_GRAPH;
        unsigned value_bytes=(keys-1)*stride*4+16;
        if ((code=jc_eval_region_add(w,times,keys*4,owner,JC_EVAL_META)) ||
            (code=jc_eval_region_add(w,values,value_bytes,owner,JC_EVAL_META))) return code;
        float first,last;
        if (!read(ctx,times,&first,4) || !read(ctx,times+(keys-1)*4,&last,4) ||
            !isfinite(first) || !isfinite(last) || last<=first) return JC_EVAL_RUN_GRAPH;
        unsigned char ends[16];
        if (!read(ctx,values,ends,16) || !read(ctx,values+value_bytes-16,ends,16)) return 1;
    }
    *temporary=(((2*curves+2)*16+31)&~15u)+((6*curves+25)&~15u);
    return 0;
}
static inline int jc_eval_run_nodes(jc_layout_reader read,void *ctx,uint64_t image,unsigned owner,
        const unsigned char job[0x78],unsigned items,struct jc_eval_run_result *out,struct jc_eval_layout_workspace *w) {
    const unsigned char *records,*p; int code;
    out->stage=6; out->detail=0;
    unsigned count=jc_eval_u32(job+8)&0x7fffffff;
    out->detail=2;
    unsigned slots=jc_eval_u32(job+0x58)&0x7fffffff;
    unsigned scratch=(jc_eval_u32(job+0x40)&0x7fffffff)*16,total_temp=jc_eval_u32(job+0x50);
    unsigned char active_slots[JC_EVAL_CAP]={0}; active_slots[0]=1;
    unsigned node_size[JC_EVAL_CAP],permanent_size[JC_EVAL_CAP],temporary_size[JC_EVAL_CAP];
    unsigned other_size[4][JC_EVAL_CAP];
    const unsigned char *other_records[4]={0}; unsigned other_count[4]={0};
    if (!count || count>JC_EVAL_CAP || !slots || slots>JC_EVAL_CAP) return JC_EVAL_RUN_GRAPH;
    code=jc_eval_run_save(read,ctx,w,jc_eval_u64(job+0x10),count*0x98,&records); if (code) return code;
    for (unsigned o=0;o<4;++o) if (o!=owner) {
        code=jc_eval_run_save(read,ctx,w,w->skeleton[o].storage.clone.payload+0x18,8,&p); if (code) return code;
        code=jc_eval_run_save(read,ctx,w,jc_eval_u64(p)+8,16,&p); if (code) return code;
        other_count[o]=jc_eval_u32(p)&0x7fffffff;
        if (!other_count[o] || other_count[o]>JC_EVAL_CAP) return JC_EVAL_RUN_GRAPH;
        code=jc_eval_run_save(read,ctx,w,jc_eval_u64(p+8),other_count[o]*0x98,other_records+o); if (code) return code;
        for (unsigned j=0;j<other_count[o];++j) {
            code=jc_eval_run_save(read,ctx,w,jc_eval_u64(other_records[o]+j*0x98),8,&p); if (code) return code;
            uint64_t vt=jc_eval_u64(p);
            if (vt!=image+0x575ab8 && vt!=image+0x576798 && vt!=image+0x5782a8) {
                out->detail=vt>=image && vt-image<=UINT32_MAX ? (unsigned)(vt-image) : UINT32_MAX;
                return JC_EVAL_RUN_GRAPH;
            }
            other_size[o][j]=vt==image+0x575ab8 || vt==image+0x576798 ? 0x38 : 0x28;
            code=jc_eval_region_add(w,jc_eval_u64(other_records[o]+j*0x98),other_size[o][j],o,JC_EVAL_META); if (code) return code;
            if (other_size[o][j]==0x38) {
                code=jc_eval_run_save(read,ctx,w,jc_eval_u64(other_records[o]+j*0x98)+0x18,0x20,&p); if (code) return code;
                for (unsigned k=0;k<2;++k) {
                    unsigned n=jc_eval_u32(p+k*0x10)&0x7fffffff;
                    if (n>32) return JC_EVAL_RUN_GRAPH;
                    if (n) {
                        code=jc_eval_region_add(w,jc_eval_u64(p+k*0x10+8),n*(k ? 4 : 8),o,JC_EVAL_META); if (code) return code;
                    }
                }
            }
        }
    }
    for (unsigned i=0;i<count;++i) {
        const unsigned char *r=records+i*0x98,*node,*callbacks;
        unsigned output_slot=jc_eval_u32(r+0x8c);
        if (output_slot>=slots) return JC_EVAL_RUN_GRAPH;
        unsigned active=active_slots[output_slot]; active_slots[output_slot]=0;
        uint64_t at=jc_eval_u64(r);
        code=jc_eval_run_save(read,ctx,w,at,0x28,&node); if (code) return code;
        uint64_t vtable=jc_eval_u64(node);
        out->detail=vtable>=image && vtable-image<=UINT32_MAX ? (unsigned)(vtable-image) : UINT32_MAX;
        int sampler=vtable==image+0x5782a8,additive=vtable==image+0x576798;
        if (!sampler && !additive && vtable!=image+0x575ab8) return JC_EVAL_RUN_GRAPH;
        if (jc_eval_u64(node+0x10)!=jc_eval_u64(job+0x28)) return JC_EVAL_RUN_GRAPH;
        node_size[i]=sampler ? 0x28 : 0x38;
        code=jc_eval_run_private(w,at,node_size[i],owner); if (code) return code;
        for (unsigned j=0;j<i;++j) if (jc_skel_overlap(at,node_size[i],jc_eval_u64(records+j*0x98),node_size[j])) return JC_EVAL_RUN_GRAPH;
        for (unsigned o=0;o<4;++o) if (o!=owner) for (unsigned j=0;j<other_count[o];++j) {
            uint64_t other=jc_eval_u64(other_records[o]+j*0x98);
            if (jc_skel_overlap(at,node_size[i],other,other_size[o][j])) return JC_EVAL_RUN_GRAPH;
        }
        code=jc_eval_run_save(read,ctx,w,vtable+0x28,0x38,&callbacks); if (code) return code;
        const unsigned normal[]={0x3f9840,0x38ea60,0x3f9850,0x3f9860,0x38e950,0x38ea80,0x38eb80};
        const unsigned add[]={0x3f9840,0x391260,0x3f9850,0x3f9860,0x391130,0x391280,0x391380};
        const unsigned sample[]={0x397f40,0x397f70,0x3f9850,0x398010,0x3f9870,0x398040,0x3981e0};
        const unsigned *expected=sampler ? sample : additive ? add : normal;
        for (unsigned j=0;j<7;++j) if (jc_eval_u64(callbacks+j*8)!=image+expected[j]) return JC_EVAL_RUN_GRAPH;
        if ((code=jc_eval_region_add(w,vtable+0x28,0x38,owner,JC_EVAL_META)) ||
            (code=jc_eval_region_add(w,at,sampler ? 0x28 : 0x38,owner,JC_EVAL_META))) return code;
        unsigned temporary=0,permanent=active && sampler ? items*4 : 0;
        if (sampler) {
            if (jc_eval_u32(r+8)) return JC_EVAL_RUN_GRAPH;
            if (active) {
                code=jc_eval_run_sampler(read,ctx,image,owner,node,items,w,&temporary); if (code) return code;
            }
        } else {
            code=jc_eval_run_save(read,ctx,w,at,0x38,&node); if (code) return code;
            unsigned children=jc_eval_u32(node+0x18)&0x7fffffff,weights=jc_eval_u32(node+0x28)&0x7fffffff;
            if (children>32 || children!=weights || children!=jc_eval_u32(r+8) || (active && !children)) return JC_EVAL_RUN_GRAPH;
            if (children) {
            uint64_t child_at=jc_eval_u64(node+0x20),weight_at=jc_eval_u64(node+0x30);
            if ((code=jc_eval_run_private(w,child_at,children*8,owner)) ||
                (code=jc_eval_run_private(w,weight_at,weights*4,owner))) return code;
            const unsigned char *child_table,*weight_table;
            if ((code=jc_eval_run_save(read,ctx,w,child_at,children*8,&child_table)) ||
                (code=jc_eval_run_save(read,ctx,w,weight_at,weights*4,&weight_table)) ||
                (code=jc_eval_region_add(w,child_at,children*8,owner,JC_EVAL_META)) ||
                (code=jc_eval_region_add(w,weight_at,weights*4,owner,JC_EVAL_META))) return code;
            for (unsigned j=0;j<children;++j) {
                unsigned found=0; float weight; memcpy(&weight,weight_table+j*4,4);
                if (!isfinite(weight) || fabsf(weight)>16) return JC_EVAL_RUN_GRAPH;
                for (unsigned k=i+1;k<count;++k)
                    found+=jc_eval_u64(child_table+j*8)==jc_eval_u64(records+k*0x98) &&
                        jc_eval_u32(r+0xc+j*4)==jc_eval_u32(records+k*0x98+0x8c);
                if (found!=1) return JC_EVAL_RUN_GRAPH;
                unsigned dependency=jc_eval_u32(r+0xc+j*4);
                if (dependency>=slots) return JC_EVAL_RUN_GRAPH;
                if (active && (weight!=0.0f || (additive && !j))) active_slots[dependency]=1;
            }
            }
            if (active) temporary=16+((weights*4+15)&~15u);
        }
        unsigned permanent_at=jc_eval_u32(r+0x90),temporary_at=jc_eval_u32(r+0x94);
        if ((permanent_at|temporary_at)&15 || permanent_at>scratch || permanent>scratch-permanent_at ||
            temporary_at>total_temp || temporary>total_temp-temporary_at) return JC_EVAL_RUN_GRAPH;
        for (unsigned j=0;j<i;++j) {
            const unsigned char *prior=records+j*0x98;
            if ((permanent && permanent_size[j] && jc_skel_overlap(permanent_at,permanent,jc_eval_u32(prior+0x90),permanent_size[j])) ||
                (temporary && temporary_size[j] && jc_skel_overlap(temporary_at,temporary,jc_eval_u32(prior+0x94),temporary_size[j]))) return JC_EVAL_RUN_GRAPH;
        }
        permanent_size[i]=permanent; temporary_size[i]=temporary;
    }
    code=jc_eval_run_save(read,ctx,w,jc_eval_u64(job+0x30)+0x18,8,&p); if (code) return code;
    if (jc_eval_u64(p)!=jc_eval_u64(records) || jc_eval_u32(records+0x8c)) return JC_EVAL_RUN_GRAPH;
    return 0;
}
static inline int jc_eval_run_prepare(jc_layout_reader read,jc_eval_writable writable,void *ctx,
        uint64_t image,const uint64_t meshes[4],unsigned owner,struct jc_eval_run_result *out,
        struct jc_eval_layout_workspace *w) {
    if (!out) return JC_EVAL_RUN_ARGUMENT;
    memset(out,0,sizeof(*out)); out->stage=1;
    if (!read || !writable || !w || !meshes || owner<2 || owner>3) return JC_EVAL_RUN_ARGUMENT;
    struct jc_eval_layout_result layout;
    int code=jc_eval_layout_inspect(read,ctx,image,meshes,&layout,w);
    if (code) { out->detail=layout.stage*10+layout.owner_index; return code; }
    out->mesh=meshes[owner]; out->model=w->skeleton[owner].storage.model;
    const unsigned char *p,*job;
    code=jc_eval_run_save(read,ctx,w,w->skeleton[owner].storage.clone.payload+0x18,8,&p); if (code) return code;
    out->job=jc_eval_u64(p);
    code=jc_eval_run_save(read,ctx,w,out->job,0x78,&job); if (code) return code;
    unsigned items=0;
    if ((code=jc_eval_run_outputs(read,writable,ctx,image,owner,job,out,w,&items)) ||
        (code=jc_eval_run_execution(read,ctx,job,items,out,w)) ||
        (code=jc_eval_run_environment(read,writable,ctx,image,owner,items,job,out,w)) ||
        (code=jc_eval_run_clocks(read,ctx,w,w->skeleton[owner].storage.clone.payload,out)) ||
        (code=jc_eval_run_controls(read,writable,ctx,owner,out,w)) ||
        (code=jc_eval_run_nodes(read,ctx,image,owner,job,items,out,w))) return code;
    out->stage=7; out->detail=0;
    code=jc_eval_disjoint(w); if (code) return code;
    for (unsigned i=0;i<w->region_count;++i) {
        const struct jc_eval_region *r=w->regions+i;
        if (r->owner==owner && (r->kind==JC_EVAL_STORAGE || r->kind==JC_EVAL_DESTINATION || r->kind==JC_EVAL_POSE) &&
            !writable(ctx,r->address,r->size)) return JC_EVAL_RUN_WRITE;
    }
    out->stage=8;
    code=jc_eval_run_recheck(read,ctx,image,w); if (code) return code;
    out->stage=0; out->ready=1;
    return 0;
}
#endif
