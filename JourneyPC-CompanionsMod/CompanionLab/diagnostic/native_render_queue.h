#ifndef JC_NATIVE_RENDER_QUEUE_H
#define JC_NATIVE_RENDER_QUEUE_H
#include "native_layout.h"
struct jc_render_budget_retry { unsigned total,consecutive,recovered; };
static inline int jc_render_budget_result(int code,int expired,int read_failed,int late) {
    if (read_failed) return code ? code : JC_LAYOUT_READ;
    if (expired || (!code && late)) return 9;
    return code;
}
static inline int jc_render_budget_defer(struct jc_render_budget_retry *retry,int code,
    unsigned drawn,int pose_written,uint32_t queued) {
    if (code!=9 || !drawn || pose_written || queued) return 0;
    ++retry->total;
    return ++retry->consecutive<8;
}
static inline void jc_render_budget_drawn(struct jc_render_budget_retry *retry) {
    if (retry->consecutive) ++retry->recovered;
    retry->consecutive=0;
}
static inline int jc_render_trial_level(int level) { return level>=1 && level<=6; }
static inline int jc_render_trial_same_level(int prepared,int current) {
    return jc_render_trial_level(prepared) && current==prepared;
}
static inline int jc_render_level(jc_layout_reader read,void *ctx,uint64_t game,int *level) {
    static const char *names[]={"Level_Graveyard","Level_Barrens","Level_Desert","Level_Canyon",
        "Level_Cave","Level_Ruins","Level_Mountain","Level_Summit","Level_Credits",
        "Level_Bryan","Level_Matt","Level_Chris"};
    char name[32],again[32]; *level=-1;
    if (!jc_layout_field(read,ctx,game,0x1c02f0,name,sizeof(name)) ||
        !jc_layout_field(read,ctx,game,0x1c02f0,again,sizeof(again))) return JC_LAYOUT_READ;
    if (memcmp(name,again,sizeof(name))) return JC_LAYOUT_CHANGED;
    if (!memchr(name,0,sizeof(name))) return JC_LAYOUT_RANGE;
    for (unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i)
        if (!strcmp(name,names[i])) { *level=(int)i; return 0; }
    return JC_LAYOUT_TYPE;
}
/* Cull inputs use transposed rows: center = x*M[0..3]+y*M[4..7]+z*M[8..11]+M[12..15]. */
static inline int jc_render_bounds_relative(const float minimum[4],const float maximum[4],
    const float matrix[16],const float origin[3],int32_t milliths[6]) {
    double center[3],half[3],result[6];
    for (unsigned i=0;i<16;++i) {
        uint32_t bits; memcpy(&bits,matrix+i,4);
        if ((bits&0x7f800000u)==0x7f800000u) return JC_LAYOUT_RANGE;
    }
    for (unsigned i=0;i<3;++i) {
        uint32_t a,b,c; memcpy(&a,minimum+i,4); memcpy(&b,maximum+i,4); memcpy(&c,origin+i,4);
        if ((a&0x7f800000u)==0x7f800000u || (b&0x7f800000u)==0x7f800000u ||
            (c&0x7f800000u)==0x7f800000u || maximum[i]<minimum[i]) return JC_LAYOUT_RANGE;
        center[i]=((double)minimum[i]+maximum[i])*0.5;
        half[i]=((double)maximum[i]-minimum[i])*0.5;
    }
    for (unsigned i=0;i<3;++i) {
        result[i]=(double)matrix[12+i]-origin[i]; result[i+3]=0;
        for (unsigned j=0;j<3;++j) {
            double m=matrix[j*4+i];
            result[i]+=center[j]*m;
            result[i+3]+=half[j]*(m<0 ? -m : m)*2;
        }
        if (!(result[i]>=-1000000 && result[i]<=1000000 && result[i+3]<=1000000)) return JC_LAYOUT_RANGE;
    }
    for (unsigned i=0;i<6;++i) milliths[i]=(int32_t)(result[i]*1000);
    return 0;
}
static inline int jc_render_preview_transform(float transform[16],float origin[3]) {
    for (unsigned i=0;i<16;++i) {
        uint32_t bits; memcpy(&bits,&transform[i],4);
        if ((bits&0x7f800000u)==0x7f800000u) return 0;
    }
    if (transform[3]<-0.00001f || transform[3]>0.00001f ||
        transform[7]<-0.00001f || transform[7]>0.00001f ||
        transform[11]<-0.00001f || transform[11]>0.00001f ||
        transform[15]<0.99999f || transform[15]>1.00001f) return 0;
    float x=transform[12]+3.0f; uint32_t bits; memcpy(&bits,&x,4);
    if ((bits&0x7f800000u)==0x7f800000u) return 0;
    double delta=(double)x-transform[12];
    if (delta<2.999 || delta>3.001) return 0;
    memcpy(origin,transform+12,3*sizeof(float));
    transform[12]=x; return 1;
}
static inline int jc_render_preview_queue(const unsigned char *context,uint64_t model,uint64_t control) {
    uint32_t count; memcpy(&count,context+0x10080,4);
    if (count>1 || !model || !control) return 0;
    for (unsigned i=0;i<4096;++i) {
        uint64_t pair[2]; memcpy(pair,context+0x80+i*16,16);
        if (!i) { if (pair[0]!=model || pair[1]!=control) return 0; }
        else if (pair[0] || pair[1]) return 0;
    }
    return 1;
}
struct jc_render_visibility { uint32_t parts,nondegenerate; int mask_pass,culling; };
static inline int jc_render_visibility_read(jc_layout_reader read,void *ctx,uint64_t model,
    uint32_t mask,uint32_t expected,struct jc_render_visibility *out) {
    memset(out,0,sizeof(*out));
    uint64_t begin,end,again; uint32_t flags; unsigned char culling;
    if (!jc_layout_field(read,ctx,model,0x60,&begin,8) ||
        !jc_layout_field(read,ctx,model,0x68,&end,8) ||
        !jc_layout_field(read,ctx,model,0xf8,&flags,4) ||
        !jc_layout_field(read,ctx,model,0xfc,&culling,1)) return JC_LAYOUT_READ;
    if (end<begin || (end-begin)%8 || (begin&7) || (end-begin)/8>256) return JC_LAYOUT_RANGE;
    struct jc_render_visibility value={.parts=(uint32_t)((end-begin)/8),.mask_pass=(flags&mask)==expected,.culling=culling!=0};
    for (uint32_t i=0;i<value.parts;++i) {
        uint64_t part,bounds; float minimum[3],extent_size[3];
        if (!jc_layout_field(read,ctx,begin,(uint64_t)i*8,&part,8) ||
            !jc_layout_field(read,ctx,part,0x38,&bounds,8) ||
            !jc_layout_field(read,ctx,bounds,0,minimum,12) ||
            !jc_layout_field(read,ctx,bounds,0x18,extent_size,12)) return JC_LAYOUT_READ;
        int extent=0;
        for (unsigned j=0;j<3;++j) {
            uint32_t a,b; memcpy(&a,&minimum[j],4); memcpy(&b,&extent_size[j],4);
            if ((a&0x7f800000u)==0x7f800000u || (b&0x7f800000u)==0x7f800000u || extent_size[j]<0.0f) return JC_LAYOUT_RANGE;
            extent|=extent_size[j]>0.0f;
        }
        value.nondegenerate+=extent;
    }
    if (!jc_layout_field(read,ctx,model,0x60,&again,8)) return JC_LAYOUT_READ;
    if (again!=begin) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,model,0x68,&again,8)) return JC_LAYOUT_READ;
    if (again!=end) return JC_LAYOUT_CHANGED;
    *out=value; return JC_LAYOUT_OK;
}
struct jc_render_snapshot { uint32_t count, active, stored; };
struct jc_render_workspace { uint64_t pairs[4096][2]; };
/* Offline planning only. A future caller must track actual acquired references,
   invalidate records on replacement/release, and establish a safe native boundary.
   Matching pointer values alone must never create a ledger entry. */
struct jc_render_ledger {
    uint64_t queue, model, control, generation;
    unsigned char slots[512];
};
enum { JC_TRACK_OK=0, JC_TRACK_INPUT=1, JC_TRACK_CONTEXT=2,
       JC_TRACK_UNTRACKED=3, JC_TRACK_MISMATCH=4, JC_TRACK_FAILED=5 };
struct jc_render_tracking {
    struct jc_render_ledger ledger;
    uint32_t held,acquired,released,reused;
    int failed;
};
static inline int jc_render_track_fail(struct jc_render_tracking *track,int code) {
    if (track) track->failed=code;
    return code;
}
/* Events must come from observed native assignments/releases, not a pointer scan.
   Caller serializes events and supplies a new generation for each queue lifetime. */
static inline int jc_render_track_begin(struct jc_render_tracking *track,uint64_t queue,
    uint64_t generation,uint64_t model,uint64_t control,const struct jc_render_workspace *snapshot) {
    if (!track || !snapshot || !queue || !generation || !model || !control) return JC_TRACK_INPUT;
    if (track->failed) return JC_TRACK_FAILED;
    if (track->held || track->ledger.queue) return jc_render_track_fail(track,JC_TRACK_CONTEXT);
    for (unsigned i=0;i<4096;++i)
        if (snapshot->pairs[i][0]==model || snapshot->pairs[i][1]==control)
            return jc_render_track_fail(track,JC_TRACK_UNTRACKED);
    *track=(struct jc_render_tracking){.ledger={.queue=queue,.model=model,.control=control,.generation=generation}};
    return JC_TRACK_OK;
}
static inline int jc_render_track_assignment(struct jc_render_tracking *track,uint64_t queue,
    uint64_t generation,unsigned slot,uint64_t before_model,uint64_t before_control,
    uint64_t after_model,uint64_t after_control) {
    if (!track || slot>=4096) return jc_render_track_fail(track,JC_TRACK_INPUT);
    if (track->failed) return JC_TRACK_FAILED;
    struct jc_render_ledger *ledger=&track->ledger;
    if (!ledger->queue || ledger->queue!=queue || ledger->generation!=generation)
        return jc_render_track_fail(track,JC_TRACK_CONTEXT);
    int before=before_model==ledger->model && before_control==ledger->control;
    int after=after_model==ledger->model && after_control==ledger->control;
    if ((before_model==ledger->model || before_control==ledger->control) != before ||
        (after_model==ledger->model || after_control==ledger->control) != after)
        return jc_render_track_fail(track,JC_TRACK_MISMATCH);
    unsigned char mask=(unsigned char)(1u<<(slot%8));
    int recorded=(ledger->slots[slot/8]&mask)!=0;
    if (before!=recorded) return jc_render_track_fail(track,JC_TRACK_UNTRACKED);
    if (track->acquired==UINT32_MAX || track->released==UINT32_MAX || track->reused==UINT32_MAX)
        return jc_render_track_fail(track,JC_TRACK_INPUT);
    if (before && after) { ++track->reused; return JC_TRACK_OK; }
    if (before) { ledger->slots[slot/8]&=(unsigned char)~mask; --track->held; ++track->released; }
    if (after) { ledger->slots[slot/8]|=mask; ++track->held; ++track->acquired; }
    return JC_TRACK_OK;
}
/* Consume a known release before entering native code, preventing reentrant or
   duplicate release. Native completion must still be verified by the caller. */
static inline int jc_render_track_release(struct jc_render_tracking *track,uint64_t queue,
    uint64_t generation,unsigned slot,uint64_t model,uint64_t control) {
    if (!track || model!=track->ledger.model || control!=track->ledger.control)
        return jc_render_track_fail(track,JC_TRACK_MISMATCH);
    return jc_render_track_assignment(track,queue,generation,slot,model,control,0,0);
}
struct jc_render_retirement { uint16_t slots[4096]; uint32_t count; };
enum { JC_RETIRE_OK=0, JC_RETIRE_INPUT=1, JC_RETIRE_GENERATION=2,
       JC_RETIRE_ACTIVE=3, JC_RETIRE_REPLACED=4, JC_RETIRE_UNTRACKED=5 };
static inline int jc_render_plan_retirement(const struct jc_render_ledger *ledger,
    uint64_t queue,uint64_t generation,uint32_t active_count,
    const struct jc_render_workspace *snapshot,struct jc_render_retirement *out) {
    if (!out) return JC_RETIRE_INPUT;
    out->count=0;
    if (!ledger || !snapshot || !queue || !ledger->model || !ledger->control ||
        !generation || active_count>4096) return JC_RETIRE_INPUT;
    if (ledger->queue!=queue || ledger->generation!=generation) return JC_RETIRE_GENERATION;
    if (active_count) return JC_RETIRE_ACTIVE;
    uint32_t count=0;
    for (uint32_t i=0;i<4096;++i) {
        int recorded=(ledger->slots[i/8]>>(i%8))&1;
        int model_match=snapshot->pairs[i][0]==ledger->model;
        int control_match=snapshot->pairs[i][1]==ledger->control;
        if (recorded && (!model_match || !control_match)) return JC_RETIRE_REPLACED;
        if (!recorded && (model_match || control_match)) return JC_RETIRE_UNTRACKED;
        if (recorded) out->slots[count++]=(uint16_t)i;
    }
    out->count=count;
    return JC_RETIRE_OK;
}
static inline int jc_render_tracking_plan(const struct jc_render_tracking *track,
    uint64_t queue,uint64_t generation,uint32_t active_count,
    const struct jc_render_workspace *snapshot,struct jc_render_retirement *out) {
    if (!out) return JC_RETIRE_INPUT;
    out->count=0;
    if (!track || track->failed) return JC_RETIRE_INPUT;
    int code=jc_render_plan_retirement(&track->ledger,queue,generation,active_count,snapshot,out);
    if (!code && out->count!=track->held) { out->count=0; return JC_RETIRE_UNTRACKED; }
    return code;
}
static inline int jc_render_queue(jc_layout_reader read,void *context,uint64_t image,
                                  uint64_t queue,uint64_t model,uint64_t control,
                                  struct jc_render_workspace *work,struct jc_render_snapshot *out) {
    uint64_t type,after_type; uint32_t count,after_count;
    struct jc_render_snapshot result={0};
    memset(out,0,sizeof(*out));
    if (!work || !model || !control) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,context,queue,0,&type,8) ||
        !jc_layout_field(read,context,queue,0x10080,&count,4)) return JC_LAYOUT_READ;
    if (type!=image+0x6896e8) return JC_LAYOUT_TYPE;
    if (count>4096) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,context,queue,0x80,work->pairs,sizeof(work->pairs))) return JC_LAYOUT_READ;
    for (uint32_t i=0;i<4096;++i) {
        if (work->pairs[i][0]==model) {
            if (work->pairs[i][1]!=control) return JC_LAYOUT_TYPE;
            ++result.stored;
            if (i<count) ++result.active;
        }
    }
    if (!jc_layout_field(read,context,queue,0,&after_type,8) ||
        !jc_layout_field(read,context,queue,0x10080,&after_count,4)) return JC_LAYOUT_READ;
    if (type!=after_type || count!=after_count) return JC_LAYOUT_CHANGED;
    result.count=count; *out=result;
    return JC_LAYOUT_OK;
}
static inline int jc_render_command_range(uint64_t begin,uint64_t end,uint32_t *count) {
    *count=0;
    uint64_t limit;
    if (end<=begin || (begin&7) || (end-begin)%128 || (end-begin)/128>256 ||
        !jc_layout_address(begin,end-begin,&limit)) return 0;
    *count=(uint32_t)((end-begin)/128); return 1;
}
struct jc_render_command_input { uint32_t kind,count,instances,indexed; };
struct jc_render_draw_input {
    uint32_t kind,stored,indexed,index_count,instances,gate,context,topology;
};
static inline int jc_render_draw_input(jc_layout_reader read,void *ctx,uint64_t image,
    const unsigned char command[128],struct jc_render_draw_input *out) {
    memset(out,0,sizeof(*out));
    struct jc_render_draw_input value={0};
    uint64_t indices,pass,device; unsigned char gate; uint32_t again;
    memcpy(&value.kind,command,4); memcpy(&pass,command+0x28,8);
    memcpy(&indices,command+0x68,8); memcpy(&value.stored,command+0x70,4);
    memcpy(&value.instances,command+0x78,4);
    if (value.kind!=1 && value.kind!=2) return JC_LAYOUT_TYPE;
    if (value.kind==1) value.instances=1;
    value.indexed=indices!=0;
    if (!jc_layout_field(read,ctx,image,0x72c060,&gate,1) ||
        !jc_layout_field(read,ctx,image,0x40408b8,&device,8) ||
        !jc_layout_field(read,ctx,pass,0x18,&value.topology,4) ||
        (indices && !jc_layout_field(read,ctx,indices,8,&value.index_count,4))) return JC_LAYOUT_READ;
    value.gate=gate!=0; value.context=device!=0;
    if (!jc_layout_field(read,ctx,pass,0x18,&again,4) || again!=value.topology ||
        (indices && (!jc_layout_field(read,ctx,indices,8,&again,4) || again!=value.index_count)))
        return JC_LAYOUT_CHANGED;
    *out=value; return JC_LAYOUT_OK;
}
static inline int jc_render_first_item(jc_layout_reader read,void *ctx,uint64_t model,uint64_t *out) {
    uint64_t begin,end,part,bounds,item,again;
    *out=0;
    if (!jc_layout_field(read,ctx,model,0x60,&begin,8) ||
        !jc_layout_field(read,ctx,model,0x68,&end,8)) return JC_LAYOUT_READ;
    if (end<=begin || end-begin!=8 || (begin&7)) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,ctx,begin,0,&part,8) ||
        !jc_layout_field(read,ctx,part,0x38,&bounds,8) ||
        !jc_layout_field(read,ctx,bounds,0x28,&item,8)) return JC_LAYOUT_READ;
    if (!item) return JC_LAYOUT_TYPE;
    if (!jc_layout_field(read,ctx,model,0x60,&again,8) || again!=begin ||
        !jc_layout_field(read,ctx,model,0x68,&again,8) || again!=end ||
        !jc_layout_field(read,ctx,begin,0,&again,8) || again!=part ||
        !jc_layout_field(read,ctx,part,0x38,&again,8) || again!=bounds ||
        !jc_layout_field(read,ctx,bounds,0x28,&again,8) || again!=item) return JC_LAYOUT_CHANGED;
    *out=item; return JC_LAYOUT_OK;
}
static inline int jc_render_command_input(const unsigned char command[128],uint64_t material,
        uint64_t pass,uint64_t batch,struct jc_render_command_input *out) {
    memset(out,0,sizeof(*out));
    uint64_t m,p,b,indices; uint32_t kind,stored,instances;
    memcpy(&kind,command,4); memcpy(&m,command+0x20,8); memcpy(&p,command+0x28,8);
    memcpy(&b,command+0x30,8); memcpy(&indices,command+0x68,8);
    memcpy(&stored,command+0x70,4); memcpy(&instances,command+0x78,4);
    if ((kind!=1 && kind!=2) || m!=material || p!=pass || b!=batch) return 2;
    *out=(struct jc_render_command_input){kind,stored,kind==2 ? instances : 1,indices!=0};
    return 0;
}
#endif
