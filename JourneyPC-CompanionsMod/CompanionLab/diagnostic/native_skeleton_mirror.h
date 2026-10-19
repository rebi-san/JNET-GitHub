#ifndef JC_NATIVE_SKELETON_MIRROR_H
#define JC_NATIVE_SKELETON_MIRROR_H
#include "native_pose_storage.h"
#include "native_trial_shader.h"
#include "native_read_batch.h"
#define JC_SKEL_CAP 256u
struct jc_skel_bone {
    uint64_t node,bone,parent,name_address;
    unsigned char entry[0x48],header[0x20];
    char name[128];
};
struct jc_skel_layout {
    struct jc_pose_storage storage;
    uint64_t mesh,head,tree,table,skin;
    uint32_t count,skin_count,names;
    uint64_t rows[JC_SKEL_CAP][4],skins[JC_SKEL_CAP][2];
    struct jc_skel_bone bones[JC_SKEL_CAP];
};
struct jc_skel_mirror {
    struct jc_skel_layout layout[4];
    unsigned map[2][JC_SKEL_CAP],stage;
    int ready;
    float before[4][JC_SKEL_CAP][16],last[2][JC_SKEL_CAP][16];
    int seen[2];
};
typedef int (*jc_skel_writer)(void *,uint64_t,const void *,size_t);
static inline int jc_skel_overlap(uint64_t a,size_t an,uint64_t b,size_t bn) {
    return a<b+bn && b<a+an;
}
static inline int jc_skel_range(uint64_t address,size_t bytes) {
    uint64_t end; return !(address&7) && jc_layout_address(address,bytes,&end);
}
static inline int jc_skel_layout_read(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t mesh,struct jc_skel_layout *l) {
    memset(l,0,sizeof(*l)); l->mesh=mesh;
    int code=jc_pose_storage_read(read,ctx,image,mesh,&l->storage); if (code) return code;
    unsigned char fields[0x50];
    if (!jc_layout_field(read,ctx,l->storage.model,0xa8,fields,sizeof(fields))) return 1;
    memcpy(&l->head,fields,8); memcpy(&l->table,fields+0x20,8); memcpy(&l->skin,fields+0x38,8);
    memcpy(&l->count,fields+0x40,4); memcpy(&l->skin_count,fields+0x48,4);
    if (!l->count || l->count>JC_SKEL_CAP || l->skin_count>JC_SKEL_CAP) return 5;
    if (!jc_skel_range(l->table,l->count*32) ||
        (l->skin_count && !jc_skel_range(l->skin,l->skin_count*16))) return 4;
    if (!read(ctx,l->table,l->rows,l->count*32) ||
        (l->skin_count && !read(ctx,l->skin,l->skins,l->skin_count*16)) ||
        !jc_layout_field(read,ctx,l->head,8,&l->tree,8)) return 1;
    uint64_t stack[JC_SKEL_CAP*2+1]; unsigned pending=0; stack[pending++]=l->tree;
    while (pending) {
        uint64_t node=stack[--pending]; unsigned char entry[0x48];
        if (!jc_skel_range(node,sizeof(entry)) || !read(ctx,node,entry,sizeof(entry))) return 1;
        if (entry[0x19]) { if (node!=l->head) return 2; continue; }
        if (l->names==JC_SKEL_CAP) return 5;
        for (unsigned i=0;i<l->names;++i) if (l->bones[i].node==node) return 5;
        struct jc_skel_bone *b=&l->bones[l->names++];
        memcpy(b->entry,entry,sizeof(entry)); b->node=node;
        uint64_t length,capacity,left,right;
        memcpy(&left,entry,8); memcpy(&right,entry+16,8);
        memcpy(&length,entry+0x30,8); memcpy(&capacity,entry+0x38,8); memcpy(&b->bone,entry+0x40,8);
        if (!length || length>=sizeof(b->name) || length>capacity || capacity>4096 || !jc_skel_range(b->bone,0x60)) return 4;
        b->name_address=node+0x20; if (capacity>=16) memcpy(&b->name_address,entry+0x20,8);
        if (!jc_layout_field(read,ctx,b->name_address,0,b->name,(size_t)length+1) ||
            !read(ctx,b->bone,b->header,sizeof(b->header))) return 1;
        if (b->name[length] || memchr(b->name,0,(size_t)length)) return 2;
        memcpy(&b->parent,b->header+8,8);
        for (unsigned i=0;i+1<l->names;++i)
            if (!strcmp(b->name,l->bones[i].name) || b->bone==l->bones[i].bone) return 2;
        stack[pending++]=left; stack[pending++]=right;
    }
    for (unsigned i=0;i<l->names;++i) {
        uint64_t bone=l->bones[i].bone; unsigned steps=0;
        while (bone!=l->storage.root) {
            unsigned j=0; while (j<l->names && l->bones[j].bone!=bone) ++j;
            if (j==l->names || ++steps>l->names) return 6;
            bone=l->bones[j].parent;
        }
    }
    if (!l->names || l->rows[0][0] || l->rows[0][1]!=l->storage.root+0x20) return 6;
    for (unsigned i=0;i<l->count;++i) {
        if (!jc_skel_range(l->rows[i][1],64) || !jc_skel_range(l->rows[i][3],64)) return 4;
        if (!i) continue;
        unsigned named=0;
        for (unsigned j=0;j<l->names;++j) named+=l->rows[i][1]==l->bones[j].bone+0x20;
        if (named!=1 || l->rows[i][0]<l->table || (l->rows[i][0]-l->table)%32 ||
            (l->rows[i][0]-l->table)/32>=i) return 6;
    }
    for (unsigned i=0;i<l->skin_count;++i)
        if (!jc_skel_range(l->skins[i][0],48) || !jc_skel_range(l->skins[i][1],64)) return 4;
    return 0;
}
static inline int jc_skel_metadata_check(void *ctx,unsigned field,const unsigned char *bytes,size_t size) {
    const struct jc_skel_layout *l=ctx;
    const struct jc_skel_bone *b=l->bones+field/3;
    const void *expected=field%3==0 ? (const void *)b->entry : field%3==1 ? (const void *)b->header : (const void *)b->name;
    return memcmp(bytes,expected,size) ? 3 : 0;
}
static inline int jc_skel_live(jc_layout_reader read,void *ctx,uint64_t image,const struct jc_skel_layout *l) {
    struct jc_pose_storage pose; unsigned char fields[0x50]; uint64_t tree,rows[JC_SKEL_CAP][4],skins[JC_SKEL_CAP][2];
    int code=jc_pose_storage_read(read,ctx,image,l->mesh,&pose); if (code) return code;
    if (memcmp(&pose,&l->storage,sizeof(pose))) return 3;
    if (!jc_layout_field(read,ctx,pose.model,0xa8,fields,sizeof(fields)) || !jc_layout_field(read,ctx,l->head,8,&tree,8)) return 1;
    if (memcmp(fields,&l->head,8) || memcmp(fields+0x20,&l->table,8) || memcmp(fields+0x38,&l->skin,8) ||
        memcmp(fields+0x40,&l->count,4) || memcmp(fields+0x48,&l->skin_count,4) || tree!=l->tree) return 3;
    if (!read(ctx,l->table,rows,l->count*32) || (l->skin_count && !read(ctx,l->skin,skins,l->skin_count*16))) return 1;
    if (memcmp(rows,l->rows,l->count*32) || memcmp(skins,l->skins,l->skin_count*16)) return 3;
    struct jc_pose_span spans[JC_SKEL_CAP*3];
    for (unsigned i=0;i<l->names;++i) {
        const struct jc_skel_bone *b=l->bones+i;
        spans[i*3]=(struct jc_pose_span){b->node,sizeof(b->entry),i*3};
        spans[i*3+1]=(struct jc_pose_span){b->bone,sizeof(b->header),i*3+1};
        spans[i*3+2]=(struct jc_pose_span){b->name_address,(uint32_t)strlen(b->name)+1,i*3+2};
    }
    return jc_read_spans(read,ctx,spans,l->names*3,jc_skel_metadata_check,(void *)l);
}
/* Protect both original travelers and the sibling, including their matrix outputs. */
static inline int jc_skel_private(const struct jc_skel_layout l[4]) {
    for (unsigned owner=2;owner<4;++owner) for (unsigned i=0;i<l[owner].names;++i) {
        uint64_t at=l[owner].bones[i].bone+0x20;
        for (unsigned other=0;other<4;++other) {
            const struct jc_skel_layout *p=l+other;
            const uint64_t addresses[]={p->mesh,p->storage.clone.slot,p->storage.clone.resource,p->storage.clone.payload,
                p->storage.model,p->head,p->table,p->skin};
            const size_t sizes[]={0x590,0x20,0x20,0x70,0x108,0x48,p->count*32,p->skin_count*16};
            for (unsigned j=0;j<8;++j) if (sizes[j] && jc_skel_overlap(at,64,addresses[j],sizes[j])) return 6;
            for (unsigned j=0;j<p->names;++j) {
                const struct jc_skel_bone *b=p->bones+j;
                if (jc_skel_overlap(at,64,b->node,0x48) || jc_skel_overlap(at,64,b->name_address,strlen(b->name)+1) ||
                    jc_skel_overlap(at,64,b->bone,other==owner && i==j ? 0x20 : 0x60)) return 6;
            }
            if (other!=owner && jc_skel_overlap(at,64,p->storage.root,0x60)) return 6;
            for (unsigned j=0;j<p->count;++j) if (jc_skel_overlap(at,64,p->rows[j][3],64)) return 6;
            for (unsigned j=0;j<p->skin_count;++j) if (jc_skel_overlap(at,64,p->skins[j][0],48)) return 6;
        }
    }
    return 0;
}
static inline int jc_skel_prepare(jc_layout_reader read,void *ctx,uint64_t image,const uint64_t meshes[4],struct jc_skel_mirror *w) {
    w->ready=0; w->stage=1;
    for (unsigned i=0;i<4;++i) { int code=jc_skel_layout_read(read,ctx,image,meshes[i],w->layout+i); if (code) return code; }
    w->stage=2;
    int code=jc_skel_private(w->layout); if (code) return code;
    for (unsigned body=0;body<2;++body) {
        const struct jc_skel_layout *source=w->layout,*target=w->layout+2+body;
        if (source->names!=target->names || source->count!=target->count) return 6;
        for (unsigned i=0;i<target->names;++i) {
            unsigned j=0; while (j<source->names && strcmp(target->bones[i].name,source->bones[j].name)) ++j;
            if (j==source->names) return 6;
            w->map[body][i]=j;
        }
        for (unsigned i=0;i<target->names;++i) {
            const struct jc_skel_bone *a=target->bones+i,*b=source->bones+w->map[body][i];
            if ((a->bone==target->storage.root)!=(b->bone==source->storage.root)) return 6;
            if (a->parent==target->storage.root && b->parent==source->storage.root) continue;
            if (!a->parent && !b->parent) continue;
            unsigned j=0; while (j<target->names && a->parent!=target->bones[j].bone) ++j;
            if (j==target->names || b->parent!=source->bones[w->map[body][j]].bone) return 6;
        }
    }
    w->stage=3;
    for (unsigned i=0;i<4;++i) { code=jc_skel_live(read,ctx,image,w->layout+i); if (code) return code; }
    w->ready=1; w->stage=0; return 0;
}
static inline int jc_skel_matrix_copy(void *ctx,unsigned index,const unsigned char *bytes,size_t size) {
    float (*out)[16]=ctx;
    if (size!=64 || index>=JC_SKEL_CAP) return 4;
    memcpy(out[index],bytes,64);
    if (!jc_trial_affine(out[index])) return 4;
    for (unsigned j=0;j<16;++j) if (fabsf(out[index][j])>1000000) return 4;
    return 0;
}
static inline int jc_skel_matrices(jc_layout_reader read,void *ctx,const struct jc_skel_layout *l,float out[JC_SKEL_CAP][16]) {
    struct jc_pose_span spans[JC_SKEL_CAP];
    for (unsigned i=0;i<l->names;++i) spans[i]=(struct jc_pose_span){l->bones[i].bone+0x20,64,i};
    return jc_read_spans(read,ctx,spans,l->names,jc_skel_matrix_copy,out);
}
static inline int jc_skel_apply(jc_layout_reader read,jc_skel_writer write,void *ctx,uint64_t image,
        struct jc_skel_mirror *w,unsigned body,unsigned *writes,unsigned *changed) {
    *writes=*changed=0; w->stage=4;
    if (!w->ready || body>=2 || !write) return 6;
    for (unsigned i=0;i<4;++i) {
        int code=jc_skel_live(read,ctx,image,w->layout+i); if (code) return code;
        code=jc_skel_matrices(read,ctx,w->layout+i,w->before[i]); if (code) return code;
    }
    w->stage=5;
    float check[JC_SKEL_CAP][16];
    for (unsigned i=0;i<4;++i) {
        int code=jc_skel_matrices(read,ctx,w->layout+i,check); if (code) return code;
        if (memcmp(check,w->before[i],w->layout[i].names*64)) return 3;
    }
    const struct jc_skel_layout *target=w->layout+body+2;
    w->stage=6;
    for (unsigned i=0;i<target->names;++i) {
        if (target->bones[i].bone==target->storage.root) continue;
        const float *value=w->before[0][w->map[body][i]];
        if (w->seen[body] && memcmp(value,w->last[body][i],64)) ++*changed;
        if (!memcmp(w->before[body+2][i],value,64)) continue;
        ++*writes;
        if (!write(ctx,target->bones[i].bone+0x20,value,64)) return 7;
    }
    w->stage=7;
    for (unsigned i=0;i<4;++i) {
        int code=jc_skel_matrices(read,ctx,w->layout+i,check); if (code) return 8;
        for (unsigned j=0;j<w->layout[i].names;++j) {
            const float *expected=(i==body+2 && target->bones[j].bone!=target->storage.root) ?
                w->before[0][w->map[body][j]] : w->before[i][j];
            if (memcmp(check[j],expected,64)) return 8;
        }
    }
    for (unsigned i=0;i<target->names;++i) memcpy(w->last[body][i],w->before[0][w->map[body][i]],64);
    w->seen[body]=1; w->stage=0; return 0;
}
static inline int jc_skel_post_render(jc_layout_reader read,void *ctx,struct jc_skel_mirror *w,unsigned body) {
    w->stage=8;
    if (!w->ready || body>=2) return 6;
    float check[JC_SKEL_CAP][16];
    for (unsigned i=0;i<4;++i) {
        int code=jc_skel_matrices(read,ctx,w->layout+i,check); if (code) return 8;
        for (unsigned j=0;j<w->layout[i].names;++j) {
            if (i==body+2 && w->layout[i].bones[j].bone==w->layout[i].storage.root) continue;
            const float *expected=i==body+2 ? w->last[body][j] : w->before[i][j];
            if (memcmp(check[j],expected,64)) return 8;
        }
    }
    w->stage=0; return 0;
}
#endif
