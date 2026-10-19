#ifndef JC_NATIVE_EVALUATION_LAYOUT_H
#define JC_NATIVE_EVALUATION_LAYOUT_H
#include "native_skeleton_mirror.h"
#include <stdlib.h>
#define JC_EVAL_CAP 1024u
#define JC_EVAL_REGIONS 16384u
#define JC_EVAL_READS 8192u
#define JC_EVAL_BYTES (2u*1024u*1024u)
enum { JC_EVAL_META, JC_EVAL_STORAGE, JC_EVAL_DESTINATION, JC_EVAL_POSE, JC_EVAL_SOURCE };
struct jc_eval_owner_result {
    unsigned records,bindings,slots,scratch_bytes,execution_bytes,output_pairs,shape_mutations;
    int pending;
};
struct jc_eval_layout_result {
    struct jc_eval_owner_result owner[4];
    int disjoint;
    unsigned stage,owner_index,states;
};
struct jc_eval_region { uint64_t address; unsigned size,owner,kind; };
struct jc_eval_saved_read { uint64_t address; unsigned offset,size; };
struct jc_eval_layout_workspace {
    struct jc_skel_layout skeleton[4];
    struct jc_eval_region regions[JC_EVAL_REGIONS];
    struct jc_eval_saved_read saved[JC_EVAL_READS];
    unsigned char bytes[JC_EVAL_BYTES];
    unsigned region_count,read_count,byte_count;
};
static inline uint64_t jc_eval_u64(const unsigned char *p) { uint64_t n; memcpy(&n,p,8); return n; }
static inline unsigned jc_eval_u32(const unsigned char *p) { unsigned n; memcpy(&n,p,4); return n; }
static inline int jc_eval_region_add(struct jc_eval_layout_workspace *w,uint64_t at,unsigned size,unsigned owner,unsigned kind) {
    uint64_t end;
    if (!size) return 0;
    if (!jc_layout_address(at,size,&end)) return 4;
    if (w->region_count==JC_EVAL_REGIONS) return 5;
    w->regions[w->region_count++]=(struct jc_eval_region){at,size,owner,kind};
    return 0;
}
static inline int jc_eval_save(jc_layout_reader read,void *ctx,struct jc_eval_layout_workspace *w,
        uint64_t at,unsigned size,const unsigned char **out) {
    uint64_t end;
    if (!size || !jc_layout_address(at,size,&end)) return 4;
    if (w->read_count==JC_EVAL_READS || size>JC_EVAL_BYTES-w->byte_count) return 5;
    *out=w->bytes+w->byte_count;
    if (!read(ctx,at,w->bytes+w->byte_count,size)) return 1;
    w->saved[w->read_count++]=(struct jc_eval_saved_read){at,w->byte_count,size};
    w->byte_count+=size;
    return 0;
}
static inline int jc_eval_readable(jc_layout_reader read,void *ctx,uint64_t at,unsigned size) {
    uint64_t end; unsigned char scratch[4096];
    if (!size || !jc_layout_address(at,size,&end)) return 4;
    for (unsigned offset=0;offset<size;) {
        unsigned n=size-offset; if (n>sizeof(scratch)) n=sizeof(scratch);
        if (!read(ctx,at+offset,scratch,n)) return 1;
        offset+=n;
    }
    return 0;
}
static inline int jc_eval_saved_compare(void *ctx,unsigned index,const unsigned char *bytes,size_t size) {
    const struct jc_eval_layout_workspace *w=ctx;
    if (index>=w->read_count || size!=w->saved[index].size) return 4;
    return memcmp(bytes,w->bytes+w->saved[index].offset,size) ? 3 : 0;
}
static inline int jc_eval_saved_recheck(jc_layout_reader read,void *ctx,struct jc_eval_layout_workspace *w) {
    struct jc_pose_span spans[JC_EVAL_CAP]; unsigned count=0;
    unsigned char buffer[4096];
    for (unsigned i=0;i<=w->read_count;++i) {
        const struct jc_eval_saved_read *r=i<w->read_count ? w->saved+i : NULL;
        if (count && (!r || count==JC_EVAL_CAP || r->size>sizeof(buffer))) {
            int code=jc_read_spans(read,ctx,spans,count,jc_eval_saved_compare,w); if (code) return code;
            count=0;
        }
        if (!r) break;
        if (r->size<=sizeof(buffer)) {
            spans[count++]=(struct jc_pose_span){r->address,r->size,i};
        } else for (unsigned offset=0;offset<r->size;) {
            unsigned n=r->size-offset; if (n>sizeof(buffer)) n=sizeof(buffer);
            if (!read(ctx,r->address+offset,buffer,n)) return 1;
            if (memcmp(buffer,w->bytes+r->offset+offset,n)) return 3;
            offset+=n;
        }
    }
    return 0;
}
static inline int jc_eval_skeleton_regions(struct jc_eval_layout_workspace *w,unsigned owner) {
    const struct jc_skel_layout *s=w->skeleton+owner;
    const uint64_t at[]={s->mesh,s->storage.clone.slot,s->storage.clone.resource,s->storage.clone.payload,
        s->storage.model,s->head,s->table,s->skin};
    const unsigned sizes[]={0x590,0x20,0x20,0x70,0x108,0x48,s->count*32,s->skin_count*16};
    int code;
    for (unsigned i=0;i<8;++i) if ((code=jc_eval_region_add(w,at[i],sizes[i],owner,JC_EVAL_META))) return code;
    for (unsigned i=0;i<s->names;++i) {
        const struct jc_skel_bone *b=s->bones+i;
        if ((code=jc_eval_region_add(w,b->node,0x48,owner,JC_EVAL_META)) ||
            (code=jc_eval_region_add(w,b->name_address,(unsigned)strlen(b->name)+1,owner,JC_EVAL_META)) ||
            (code=jc_eval_region_add(w,b->bone,0x20,owner,JC_EVAL_META)) ||
            (code=jc_eval_region_add(w,b->bone+0x20,64,owner,JC_EVAL_POSE))) return code;
    }
    if ((code=jc_eval_region_add(w,s->storage.root,0x20,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,s->storage.root+0x20,64,owner,JC_EVAL_POSE))) return code;
    for (unsigned i=0;i<s->count;++i)
        if ((code=jc_eval_region_add(w,s->rows[i][3],64,owner,JC_EVAL_POSE))) return code;
    for (unsigned i=0;i<s->skin_count;++i)
        if ((code=jc_eval_region_add(w,s->skins[i][0],48,owner,JC_EVAL_POSE))) return code;
    return 0;
}
static inline int jc_eval_owner_collect(jc_layout_reader read,void *ctx,struct jc_eval_layout_workspace *w,
        unsigned owner,struct jc_eval_layout_result *out) {
    struct jc_eval_owner_result *r=out->owner+owner;
    const struct jc_skel_layout *s=w->skeleton+owner;
    const unsigned char *p,*header; int code;
    code=jc_eval_save(read,ctx,w,s->storage.clone.payload+0x18,8,&p); if (code) return code;
    uint64_t job=jc_eval_u64(p);
    if (!job) return 2;
    if (job&7) return 4;
    code=jc_eval_save(read,ctx,w,job,0x78,&header); if (code) return code;
    r->pending=jc_eval_u64(header+0x70)!=0;
    if (r->pending) return 7;
    r->records=jc_eval_u32(header+8)&0x7fffffff;
    r->bindings=jc_eval_u32(header+0x18)&0x7fffffff;
    r->slots=jc_eval_u32(header+0x58)&0x7fffffff;
    unsigned scratch_count=jc_eval_u32(header+0x40)&0x7fffffff;
    if (!r->records || !r->bindings || !r->slots) return 2;
    if (r->records>JC_EVAL_CAP || r->bindings>JC_EVAL_CAP || r->slots>JC_EVAL_CAP ||
        scratch_count>65536 || jc_eval_u32(header+0x50)>16u*1024u*1024u) return 5;
    r->scratch_bytes=scratch_count*16;
    r->execution_bytes=0x48+r->records*0xa8;
    const uint64_t tables[]={jc_eval_u64(header+0x10),jc_eval_u64(header+0x20),
        jc_eval_u64(header+0x48),jc_eval_u64(header+0x60),jc_eval_u64(header+0x68)};
    const unsigned sizes[]={r->records*0x98,r->bindings*0x30,r->scratch_bytes,r->slots*8,r->execution_bytes};
    if (!r->scratch_bytes) return 2;
    if ((code=jc_eval_region_add(w,job,0x70,owner,JC_EVAL_META)) ||
        (code=jc_eval_region_add(w,job+0x70,8,owner,JC_EVAL_STORAGE))) return code;
    for (unsigned i=0;i<5;++i) {
        if (tables[i]&((i==2 || i==4) ? 15u : 7u)) return 4;
        code=jc_eval_region_add(w,tables[i],sizes[i],owner,i<2 ? JC_EVAL_META : JC_EVAL_STORAGE);
        if (code) return code;
        if (i<2) { code=jc_eval_save(read,ctx,w,tables[i],sizes[i],&p); if (code) return code; }
        else { code=jc_eval_readable(read,ctx,tables[i],sizes[i]); if (code) return code; }
        if (!i) for (unsigned row=0;row<r->records;++row) {
            const unsigned char *entry=p+row*0x98;
            uint64_t node=jc_eval_u64(entry);
            if ((node&7) || jc_eval_u32(entry+8)>32 || jc_eval_u32(entry+0x8c)>=r->slots ||
                jc_eval_u32(entry+0x90)>r->scratch_bytes ||
                jc_eval_u32(entry+0x94)>jc_eval_u32(header+0x50)) return 4;
            const unsigned char *node_header;
            if ((code=jc_eval_save(read,ctx,w,node,8,&node_header)) ||
                (code=jc_eval_region_add(w,node,8,owner,JC_EVAL_META))) return code;
            for (unsigned dependency=0;dependency<jc_eval_u32(entry+8);++dependency)
                if (jc_eval_u32(entry+0xc+dependency*4)>=r->slots) return 4;
        }
    }
    code=jc_eval_save(read,ctx,w,tables[4]+0x40,4,&p); if (code) return code;
    if (jc_eval_u32(p)!=r->records) return 2;
    code=jc_eval_save(read,ctx,w,s->storage.clone.payload+0x40,16,&p); if (code) return code;
    uint64_t states=jc_eval_u64(p),state_count=jc_eval_u64(p+8);
    if (!state_count || state_count>JC_EVAL_CAP) return 5;
    if (states&7) return 4;
    code=jc_eval_save(read,ctx,w,states,(unsigned)state_count*8,&p); if (code) return code;
    const unsigned char *state_table=p;
    if ((code=jc_eval_region_add(w,states,(unsigned)state_count*8,owner,JC_EVAL_META))) return code;
    for (unsigned i=0;i<state_count;++i) {
        uint64_t state=jc_eval_u64(state_table+i*8);
        if (state&7) return 4;
        code=jc_eval_save(read,ctx,w,state,0x28,&p); if (code) return code;
        float values[6]; memcpy(values,p+0x10,sizeof(values));
        for (unsigned j=0;j<6;++j) if (!isfinite(values[j])) return 4;
        if (values[5]<=values[4] || values[5]-values[4]>86400 || fabsf(values[0])>86400 ||
            fabsf(values[2])>1000 || (values[3]<0 && values[3]!=-1)) return 4;
        if ((code=jc_eval_region_add(w,state,0x10,owner,JC_EVAL_META)) ||
            (code=jc_eval_region_add(w,state+0x10,8,owner,JC_EVAL_DESTINATION)) ||
            (code=jc_eval_region_add(w,state+0x18,0x10,owner,JC_EVAL_META))) return code;
        ++out->states;
    }
    code=jc_eval_save(read,ctx,w,s->storage.model+0xb8,12,&p); if (code) return code;
    uint64_t pairs=jc_eval_u64(p); r->output_pairs=jc_eval_u32(p+8);
    if (r->output_pairs>JC_EVAL_CAP) return 5;
    if (r->output_pairs) {
        if (pairs&7) return 4;
        code=jc_eval_save(read,ctx,w,pairs,r->output_pairs*16,&p); if (code) return code;
        if ((code=jc_eval_region_add(w,pairs,r->output_pairs*16,owner,JC_EVAL_META))) return code;
        for (unsigned i=0;i<r->output_pairs;++i) {
            uint64_t dst=jc_eval_u64(p+i*16),src=jc_eval_u64(p+i*16+8);
            if ((dst|src)&15) return 4;
            if ((code=jc_eval_readable(read,ctx,dst,64)) || (code=jc_eval_readable(read,ctx,src,64)) ||
                (code=jc_eval_region_add(w,dst,64,owner,JC_EVAL_DESTINATION)) ||
                (code=jc_eval_region_add(w,src,64,owner,JC_EVAL_SOURCE))) return code;
        }
    }
    code=jc_eval_save(read,ctx,w,s->storage.model+0x60,16,&p); if (code) return code;
    uint64_t begin=jc_eval_u64(p),end=jc_eval_u64(p+8);
    if (end<begin || ((begin|end)&7) || (end-begin)/8>JC_EVAL_CAP) return 4;
    unsigned shapes=(unsigned)((end-begin)/8);
    if (shapes) {
        code=jc_eval_save(read,ctx,w,begin,shapes*8,&p); if (code) return code;
        const unsigned char *shape_table=p;
        if ((code=jc_eval_region_add(w,begin,shapes*8,owner,JC_EVAL_META))) return code;
        for (unsigned i=0;i<shapes;++i) {
            uint64_t entry=jc_eval_u64(shape_table+i*8);
            if (entry&7) return 4;
            code=jc_eval_save(read,ctx,w,entry,0x20,&p); if (code) return code;
            uint64_t type=jc_eval_u64(p),target=jc_eval_u64(p+0x18);
            if ((code=jc_eval_region_add(w,entry,0x20,owner,JC_EVAL_META))) return code;
            if (type&7) return 4;
            if ((code=jc_eval_region_add(w,type,0x14,owner,JC_EVAL_META))) return code;
            code=jc_eval_save(read,ctx,w,type+0x10,4,&p); if (code) return code;
            if ((jc_eval_u32(p)&0x7fffffff)!=0x46 || !target) continue;
            if (target&15) return 4;
            if ((code=jc_eval_region_add(w,target,0x100,owner,JC_EVAL_POSE)) ||
                (code=jc_eval_region_add(w,target,0x80,owner,JC_EVAL_META)) ||
                (code=jc_eval_region_add(w,target+0x80,128,owner,JC_EVAL_DESTINATION)) ||
                (code=jc_eval_readable(read,ctx,target+0x80,128))) return code;
            ++r->shape_mutations;
        }
    }
    return 0;
}
static inline int jc_eval_region_compare(const void *a,const void *b) {
    uint64_t x=((const struct jc_eval_region *)a)->address,y=((const struct jc_eval_region *)b)->address;
    return x<y ? -1 : x>y;
}
static inline int jc_eval_disjoint(struct jc_eval_layout_workspace *w) {
    uint64_t ends[4][5]={{0}};
    qsort(w->regions,w->region_count,sizeof(w->regions[0]),jc_eval_region_compare);
    for (unsigned i=0;i<w->region_count;++i) {
        struct jc_eval_region r=w->regions[i];
        for (unsigned owner=0;owner<4;++owner) for (unsigned kind=0;kind<5;++kind) {
            if (ends[owner][kind]<=r.address) continue;
            int writer=(r.owner>=2 && (r.kind==JC_EVAL_STORAGE || r.kind==JC_EVAL_DESTINATION)) ||
                (owner>=2 && (kind==JC_EVAL_STORAGE || kind==JC_EVAL_DESTINATION));
            if (!writer) continue;
            if (r.owner==owner && (r.kind==JC_EVAL_SOURCE || kind==JC_EVAL_SOURCE)) continue;
            if (r.owner!=owner || r.kind==JC_EVAL_META || kind==JC_EVAL_META ||
                r.kind==JC_EVAL_STORAGE || kind==JC_EVAL_STORAGE) return 6;
        }
        uint64_t end=r.address+r.size;
        if (end>ends[r.owner][r.kind]) ends[r.owner][r.kind]=end;
    }
    return 0;
}
static inline int jc_eval_layout_inspect(jc_layout_reader read,void *ctx,uint64_t image,
        const uint64_t meshes[4],struct jc_eval_layout_result *out,struct jc_eval_layout_workspace *w) {
    memset(out,0,sizeof(*out)); out->disjoint=-1; out->owner_index=4;
    for (unsigned i=0;i<4;++i) out->owner[i].pending=-1;
    if (!w) return 8;
    w->region_count=w->read_count=w->byte_count=0; out->stage=1;
    int code;
    for (unsigned i=0;i<4;++i) {
        out->owner_index=i;
        code=jc_skel_layout_read(read,ctx,image,meshes[i],w->skeleton+i); if (code) return code;
        code=jc_eval_skeleton_regions(w,i); if (code) return code;
    }
    out->stage=2;
    for (unsigned i=0;i<4;++i) {
        out->owner_index=i;
        code=jc_eval_owner_collect(read,ctx,w,i,out); if (code) return code;
    }
    out->owner_index=4; out->stage=3;
    code=jc_eval_disjoint(w); if (code) { out->disjoint=0; return code; }
    out->stage=4;
    for (unsigned i=0;i<4;++i) {
        out->owner_index=i;
        code=jc_skel_live(read,ctx,image,w->skeleton+i); if (code) return code;
    }
    out->owner_index=4;
    code=jc_eval_saved_recheck(read,ctx,w); if (code) return code;
    out->disjoint=1; out->stage=0; return 0;
}
#endif
