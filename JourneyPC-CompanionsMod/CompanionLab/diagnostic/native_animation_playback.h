#ifndef JC_NATIVE_ANIMATION_PLAYBACK_H
#define JC_NATIVE_ANIMATION_PLAYBACK_H
#include "companion_animation.h"
#include "native_pose_storage.h"
struct jc_anim_layout {
    uint64_t mesh,entries,schema;
    unsigned count;
    unsigned field,row;
    uint64_t value;
    uint64_t names[JC_ANIM_ENTRIES];
    unsigned char rows[JC_ANIM_ENTRIES][0x50];
};
static inline int jc_anim_reject(struct jc_anim_layout *layout,unsigned field,unsigned row,uint64_t value,int code) {
    layout->field=field; layout->row=row; layout->value=value; return code;
}
static inline uint64_t jc_anim_hash(uint64_t hash,const void *data,size_t size) {
    const unsigned char *p=data;
    for (size_t i=0;i<size;++i) { hash^=p[i]; hash*=UINT64_C(1099511628211); }
    return hash;
}
static inline int jc_anim_name(jc_layout_reader read,void *ctx,uint64_t address,
        const unsigned char row[0x50],uint64_t *hash) {
    uint64_t length,capacity,pointer=address+0x20; char text[128];
    memcpy(&length,row+0x30,8); memcpy(&capacity,row+0x38,8);
    if (length>=sizeof(text) || length>capacity || capacity>4096) return 4;
    if (capacity>=16) {
        memcpy(&pointer,row+0x20,8);
        if (!jc_layout_field(read,ctx,pointer,0,text,(size_t)length+1)) return 1;
    } else memcpy(text,row+0x20,(size_t)length+1);
    if (text[length] || memchr(text,0,(size_t)length)) return 2;
    *hash=jc_anim_hash(UINT64_C(14695981039346656037),text,(size_t)length+1);
    return 0;
}
static inline int jc_anim_layout_read(jc_layout_reader read,void *ctx,uint64_t image,uint64_t mesh,
        struct jc_anim_layout *layout) {
    uint64_t type,fields[2],check[2]; unsigned char rows[JC_ANIM_ENTRIES][0x50];
    layout->field=layout->row=0; layout->value=0;
    if (!jc_layout_field(read,ctx,mesh,0,&type,8) || type!=image+0x67f3f8 ||
        !jc_layout_field(read,ctx,mesh,0x578,fields,sizeof(fields))) return 1;
    if (!fields[1] || fields[1]>JC_ANIM_ENTRIES) return jc_anim_reject(layout,1,0,fields[1],4);
    unsigned count=(unsigned)fields[1];
    if (!jc_layout_field(read,ctx,fields[0],0,rows,count*0x50)) return 1;
    int rebuild=layout->mesh!=mesh || layout->entries!=fields[0] || layout->count!=count || !layout->schema;
    if (!rebuild) for (unsigned i=0;i<count;++i)
        if (memcmp(rows[i]+0x14,layout->rows[i]+0x14,4) ||
            memcmp(rows[i]+0x20,layout->rows[i]+0x20,32)) rebuild=1;
    uint64_t schema=UINT64_C(14695981039346656037);
    for (unsigned i=0;i<count;++i) {
        uint32_t index; memcpy(&index,rows[i]+0x14,4);
        if (index>=count) return jc_anim_reject(layout,2,i,index,4);
        if (rows[i][2]>1) return jc_anim_reject(layout,3,i,rows[i][2],4);
        float weight; memcpy(&weight,rows[i]+0x10,4);
        if (!isfinite(weight) || fabsf(weight)>8) {
            uint32_t bits; memcpy(&bits,&weight,4); return jc_anim_reject(layout,4,i,bits,4);
        }
        if (rebuild || weight!=0) {
            uint64_t length,capacity; memcpy(&length,rows[i]+0x30,8); memcpy(&capacity,rows[i]+0x38,8);
            if (length>=128 || length>capacity) return jc_anim_reject(layout,5,i,length,4);
            if (capacity>4096) return jc_anim_reject(layout,6,i,capacity,4);
            uint64_t name; int code=jc_anim_name(read,ctx,fields[0]+i*0x50,rows[i],&name);
            if (code) return jc_anim_reject(layout,5,i,length,code);
            if (!rebuild && name!=layout->names[i]) { layout->schema=0; return 3; }
            layout->names[i]=name;
        }
        schema=jc_anim_hash(schema,&layout->names[i],8);
        schema=jc_anim_hash(schema,&index,4);
    }
    if (!jc_layout_field(read,ctx,mesh,0x578,check,sizeof(check)) || memcmp(fields,check,sizeof(fields))) return 3;
    layout->mesh=mesh; layout->entries=fields[0]; layout->count=count; layout->schema=schema;
    memcpy(layout->rows,rows,count*0x50); return 0;
}
static inline int jc_anim_capture(jc_layout_reader read,void *ctx,uint64_t image,uint64_t mesh,
        struct jc_anim_layout *layout,struct jc_anim_sample *out) {
    memset(out,0,sizeof(*out));
    int code=jc_anim_layout_read(read,ctx,image,mesh,layout); if (code) return code;
    struct jc_anim_sample next={.schema=layout->schema,.total=layout->count};
    for (unsigned i=0;i<layout->count;++i) {
        float weight; memcpy(&weight,layout->rows[i]+0x10,4);
        if (weight==0) continue;
        if (next.count==JC_ANIM_ACTIVE) return jc_anim_reject(layout,10,i,next.count+1,5);
        struct jc_anim_clip *c=next.clips+next.count++;
        c->index=i; c->additive=layout->rows[i][2];
        memcpy(&c->frame,layout->rows[i]+4,4); memcpy(&c->duration,layout->rows[i]+8,4);
        memcpy(&c->speed,layout->rows[i]+12,4); c->weight=weight;
        unsigned field=jc_anim_clip_field(c,next.total);
        if (field) {
            uint32_t bits=0;
            if (field>=7 && field<=9) memcpy(&bits,layout->rows[i]+4*(field-6),4);
            return jc_anim_reject(layout,field,i,bits,4);
        }
    }
    if (!jc_anim_valid(&next)) return 4;
    *out=next; return 0;
}
typedef int (*jc_anim_writer)(void *,uint64_t,const void *,size_t);
struct jc_anim_bind_detail {
    uint64_t payload,expected,actual;
    uint32_t global_stamp,previous_stamp,armed_stamp,observed_stamp;
    unsigned field,row,changed;
};
static inline int jc_anim_bind_reject(struct jc_anim_bind_detail *out,unsigned field,unsigned row,
        uint64_t expected,uint64_t actual,int code) {
    out->field=field; out->row=row; out->expected=expected; out->actual=actual; return code;
}
static inline int jc_anim_bind_separate(uint64_t at,unsigned bytes,uint64_t other,unsigned size) {
    uint64_t end,other_end;
    return jc_layout_address(at,bytes,&end) && jc_layout_address(other,size,&other_end) &&
        (end<=other || other_end<=at);
}
static inline int jc_anim_bind_arm(jc_layout_reader read,jc_anim_writer write,void *ctx,uint64_t image,
        const uint64_t meshes[4],unsigned owner,uint64_t model,uint64_t control,struct jc_anim_bind_detail *out) {
    if (!out) return 4;
    memset(out,0,sizeof(*out));
    if (!read || !write || !meshes || owner<2 || owner>3 || !model || !control)
        return jc_anim_bind_reject(out,1,owner,0,owner,4);
    struct jc_clone_snapshot clones[4],again;
    unsigned char headers[4][0x20],check[0x20];
    for (unsigned i=0;i<4;++i) {
        out->field=2; out->row=i;
        int code=jc_clone_snapshot_read(read,ctx,image,meshes[i],clones+i); if (code) return code;
        if (!clones[i].payload || !jc_layout_field(read,ctx,clones[i].payload,0,headers[i],sizeof(headers[i]))) return 1;
    }
    out->payload=clones[owner].payload;
    uint64_t observed_model,observed_control;
    memcpy(&observed_model,headers[owner],8); memcpy(&observed_control,headers[owner]+8,8);
    if (observed_model!=model) return jc_anim_bind_reject(out,4,owner,model,observed_model,6);
    if (observed_control!=control) return jc_anim_bind_reject(out,4,owner,control,observed_control,6);
    for (unsigned i=0;i<4;++i) {
        uint64_t other_model,other_control;
        memcpy(&other_model,headers[i],8); memcpy(&other_control,headers[i]+8,8);
        const uint64_t at[]={meshes[i],clones[i].slot,clones[i].resource,other_model,other_control};
        const unsigned sizes[]={0x590,0x20,0x20,0x108,4};
        for (unsigned j=0;j<5;++j)
            if (!jc_anim_bind_separate(out->payload+0x10,4,at[j],sizes[j]))
                return jc_anim_bind_reject(out,3,i,out->payload+0x10,at[j],6);
        if (i!=owner && !jc_anim_bind_separate(out->payload,0x70,clones[i].payload,0x70))
            return jc_anim_bind_reject(out,3,i,out->payload,clones[i].payload,6);
    }
    out->field=5; out->row=owner;
    if (!jc_layout_field(read,ctx,image,0x3c94294,&out->global_stamp,4)) return 1;
    memcpy(&out->previous_stamp,headers[owner]+0x10,4);
    out->armed_stamp=out->previous_stamp==out->global_stamp ? out->global_stamp-1u : out->previous_stamp;
    for (unsigned i=0;i<4;++i) {
        out->field=6; out->row=i;
        int code=jc_clone_snapshot_read(read,ctx,image,meshes[i],&again); if (code) return code;
        if (!jc_clone_snapshot_equal(clones+i,&again)) return 3;
        if (!jc_layout_field(read,ctx,again.payload,0,check,sizeof(check))) return 1;
        if (memcmp(check,headers[i],sizeof(check))) return 3;
    }
    uint32_t frame;
    if (!jc_layout_field(read,ctx,image,0x3c94294,&frame,4)) return 1;
    if (frame!=out->global_stamp) return jc_anim_bind_reject(out,6,owner,out->global_stamp,frame,3);
    if (out->armed_stamp!=out->previous_stamp) {
        out->field=7; out->row=owner;
        if (!write(ctx,out->payload+0x10,&out->armed_stamp,4)) return 7;
        out->changed=1;
        memcpy(headers[owner]+0x10,&out->armed_stamp,4);
    }
    for (unsigned i=0;i<4;++i) {
        out->field=8; out->row=i;
        if (!jc_layout_field(read,ctx,clones[i].payload,0,check,sizeof(check))) return 1;
        if (memcmp(check,headers[i],sizeof(check))) return 8;
    }
    if (!jc_layout_field(read,ctx,image,0x3c94294,&frame,4)) return 1;
    if (frame!=out->global_stamp) return jc_anim_bind_reject(out,8,owner,out->global_stamp,frame,3);
    out->field=out->row=0; return 0;
}
static inline int jc_anim_bind_complete(jc_layout_reader read,void *ctx,uint64_t image,uint64_t mesh,
        struct jc_anim_bind_detail *out) {
    struct jc_clone_snapshot clone;
    out->field=9;
    int code=jc_clone_snapshot_read(read,ctx,image,mesh,&clone); if (code) return code;
    if (clone.payload!=out->payload) return jc_anim_bind_reject(out,9,0,out->payload,clone.payload,3);
    uint32_t frame;
    if (!jc_layout_field(read,ctx,image,0x3c94294,&frame,4) ||
        !jc_layout_field(read,ctx,out->payload,0x10,&out->observed_stamp,4)) return 1;
    if (frame!=out->global_stamp) return jc_anim_bind_reject(out,9,0,out->global_stamp,frame,3);
    if (out->observed_stamp!=frame) return jc_anim_bind_reject(out,9,0,frame,out->observed_stamp,8);
    out->field=out->row=0; return 0;
}
static inline int jc_anim_clock_valid(float frame,float duration,const float range[2]) {
    if (!isfinite(frame) || !isfinite(duration) || !isfinite(range[0]) || !isfinite(range[1]) ||
        fabsf(frame)>1000000 || duration>1000000 || (duration<0 && duration!=-1)) return 0;
    double length=(double)range[1]-range[0];
    if (!(length>0) || length>1000000) return 0;
    double limit=duration>length ? duration : length;
    return frame>=-.01f && (double)frame<=limit+.01;
}
static inline int jc_anim_consumed_detail(jc_layout_reader read,void *ctx,const struct jc_anim_layout *layout,
        const struct jc_anim_sample *sample,struct jc_anim_bind_detail *out) {
    unsigned char rows[JC_ANIM_ENTRIES][0x50];
    out->field=10; out->row=0; out->expected=out->actual=0;
    if (!sample->total || sample->total!=layout->count || !jc_anim_valid(sample)) return 0;
    if (!jc_layout_field(read,ctx,layout->entries,0,rows,layout->count*0x50)) { out->field=11; return 0; }
    for (unsigned i=0;i<layout->count;++i) {
        const unsigned char *row=rows[i];
        if (row[0]) return jc_anim_bind_reject(out,12,i,0,row[0],0);
        if (row[1]) return jc_anim_bind_reject(out,13,i,0,row[1],0);
        const struct jc_anim_clip *c=NULL;
        for (unsigned j=0;j<sample->count;++j) if (sample->clips[j].index==i) c=sample->clips+j;
        uint32_t actual,expected=0; memcpy(&actual,row+16,4);
        if (c) memcpy(&expected,&c->weight,4);
        if (actual!=expected) return jc_anim_bind_reject(out,14,i,expected,actual,0);
        if (!c) continue;
        if (row[2]!=c->additive) return jc_anim_bind_reject(out,15,i,c->additive,row[2],0);
        memcpy(&actual,row+4,4); memcpy(&expected,&c->frame,4);
        if (actual!=expected) return jc_anim_bind_reject(out,16,i,expected,actual,0);
        memcpy(&actual,row+8,4); memcpy(&expected,&c->duration,4);
        if (actual!=expected) return jc_anim_bind_reject(out,17,i,expected,actual,0);
        memcpy(&actual,row+12,4);
        if (actual) return jc_anim_bind_reject(out,18,i,0,actual,0);
    }
    out->field=out->row=0;
    return 1;
}
static inline int jc_anim_consumed(jc_layout_reader read,void *ctx,const struct jc_anim_layout *layout,
        const struct jc_anim_sample *sample) {
    struct jc_anim_bind_detail detail={0};
    return jc_anim_consumed_detail(read,ctx,layout,sample,&detail);
}
static inline int jc_anim_apply(jc_layout_reader read,jc_anim_writer write,void *ctx,uint64_t image,
        uint64_t mesh,const uint64_t other_meshes[3],struct jc_anim_layout *layout,
        const struct jc_anim_sample *sample,int *written) {
    *written=0;
    if (!write || !sample->total || !jc_anim_valid(sample)) return 4;
    int code=jc_anim_layout_read(read,ctx,image,mesh,layout); if (code) return code;
    if (sample->total!=layout->count || sample->schema!=layout->schema) return 6;
    uint64_t end=0;
    if (!jc_layout_address(layout->entries,layout->count*0x50,&end)) return 4;
    for (unsigned j=0;j<3;++j) {
        uint64_t fields[2],other_end;
        if (!other_meshes[j] || !jc_layout_field(read,ctx,other_meshes[j],0x578,fields,sizeof(fields))) return 1;
        if (!fields[1] || fields[1]>JC_ANIM_ENTRIES || !jc_layout_address(fields[0],fields[1]*0x50,&other_end)) return 4;
        if (layout->entries<other_end && fields[0]<end) return 6;
    }
    struct jc_pose_storage pose;
    code=jc_pose_storage_read(read,ctx,image,mesh,&pose); if (code) return code;
    uint64_t payload[14];
    if (!jc_layout_field(read,ctx,pose.clone.payload,0,payload,sizeof(payload))) return 1;
    if (payload[7]!=layout->count || payload[9]<layout->count || payload[9]>JC_ANIM_ENTRIES ||
        payload[11]>JC_ANIM_ENTRIES || payload[13]>JC_ANIM_ENTRIES) return 4;
    unsigned used[2]={0}; unsigned char proposed[JC_ANIM_ENTRIES][0x50],check[JC_ANIM_ENTRIES][0x50];
    memcpy(proposed,layout->rows,layout->count*0x50);
    for (unsigned i=0;i<layout->count;++i) {
        proposed[i][0]=proposed[i][1]=0;
        memset(proposed[i]+0x10,0,4);
        for (unsigned j=0;j<sample->count;++j) if (sample->clips[j].index==i) {
            const struct jc_anim_clip *c=sample->clips+j;
            proposed[i][2]=(unsigned char)c->additive;
            uint32_t index; memcpy(&index,proposed[i]+0x14,4);
            uint64_t descriptor,state,sampler,collection,header[2],candidates[JC_ANIM_ENTRIES];
            unsigned kind=c->additive,slot=used[kind]++;
            if (slot>=payload[11+kind*2]) return 5;
            if (!jc_layout_field(read,ctx,payload[6],index*8,&descriptor,8) ||
                !jc_layout_field(read,ctx,payload[8],index*8,&state,8) ||
                !jc_layout_field(read,ctx,payload[10+kind*2],slot*8,&sampler,8) ||
                !jc_layout_field(read,ctx,sampler,0x10,&collection,8) ||
                !jc_layout_field(read,ctx,collection,0,header,sizeof(header))) return 1;
            unsigned count=(unsigned)(header[0]&0x7fffffff);
            if (!count || count>JC_ANIM_ENTRIES) return 4;
            if (count==1) candidates[0]=header[1];
            else if (!jc_layout_field(read,ctx,header[1],0,candidates,count*sizeof(*candidates))) return 1;
            int found=0;
            for (unsigned k=0;k<count;++k) if (candidates[k]==descriptor) found=1;
            if (!found) return 6;
            float range[2];
            if (!jc_layout_field(read,ctx,state,0x20,range,sizeof(range))) return 1;
            if (!jc_anim_clock_valid(c->frame,c->duration,range)) return 4;
            proposed[i][0]=1;
            memcpy(proposed[i]+4,&c->frame,4); memcpy(proposed[i]+8,&c->duration,4);
            memset(proposed[i]+12,0,4); memcpy(proposed[i]+16,&c->weight,4);
        }
    }
    for (unsigned kind=0;kind<2;++kind) {
        uint64_t fields[2];
        if (!jc_layout_field(read,ctx,payload[4+kind],0x28,fields,sizeof(fields))) return 1;
        if ((fields[0]&0x7fffffff)<payload[11+2*kind]+kind) return 4;
    }
    if (!jc_layout_field(read,ctx,layout->entries,0,check,layout->count*0x50) ||
        memcmp(check,layout->rows,layout->count*0x50)) return 3;
    for (unsigned i=0;i<layout->count;++i) if (memcmp(proposed[i],layout->rows[i],20)) {
        *written=1;
        if (!write(ctx,layout->entries+i*0x50,proposed[i],20)) return 7;
    }
    if (!jc_layout_field(read,ctx,layout->entries,0,check,layout->count*0x50) ||
        memcmp(check,proposed,layout->count*0x50)) return 8;
    memcpy(layout->rows,proposed,layout->count*0x50); return 0;
}
#endif
