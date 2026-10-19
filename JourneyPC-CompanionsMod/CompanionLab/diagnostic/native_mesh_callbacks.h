#ifndef JC_NATIVE_MESH_CALLBACKS_H
#define JC_NATIVE_MESH_CALLBACKS_H
#include "native_layout.h"
enum { JC_CALLBACK_NODE_CAP=5, JC_CALLBACK_CYCLE=6, JC_CALLBACK_TYPE_CAP=7, JC_CALLBACK_STORAGE=8, JC_CALLBACK_TIME=9 };
struct jc_callback_set { uint64_t types[8]; uint32_t counts[8], used, total; };
enum { JC_CALLBACK_MAX_NODES=8192, JC_CALLBACK_HASH_SIZE=16384 };
struct jc_callback_row { uint64_t node,next,owner,callback,type; };
struct jc_callback_workspace {
    struct jc_callback_row rows[JC_CALLBACK_MAX_NODES];
    uint64_t seen[JC_CALLBACK_HASH_SIZE];
};
struct jc_callback_snapshot { struct jc_callback_set owners[3]; uint32_t visited; };
static inline int jc_callback_equal(const struct jc_callback_set *a,const struct jc_callback_set *b) {
    if (a->used!=b->used || a->total!=b->total) return 0;
    for (unsigned i=0;i<a->used;++i) {
        unsigned j=0;
        while (j<b->used && a->types[i]!=b->types[j]) ++j;
        if (j==b->used || a->counts[i]!=b->counts[j]) return 0;
    }
    return 1;
}
static inline int jc_mesh_callbacks(jc_layout_reader read,void *ctx,uint64_t manager,
                                    const uint64_t meshes[3],struct jc_callback_snapshot *out,
                                    struct jc_callback_workspace *work) {
    struct jc_callback_row *rows=work ? work->rows : NULL;
    struct jc_callback_snapshot result={0};
    uint64_t head,node,again; unsigned count=0;
    memset(out,0,sizeof(*out));
    if (!work) return JC_CALLBACK_STORAGE;
    memset(work->seen,0,sizeof(work->seen));
    if (!jc_layout_field(read,ctx,manager,0x80030,&head,8)) return JC_LAYOUT_READ;
    node=head;
    while (node) {
        if (count==JC_CALLBACK_MAX_NODES) return JC_CALLBACK_NODE_CAP;
        unsigned bucket=(unsigned)((node>>3)*UINT64_C(11400714819323198485) >> 50);
        unsigned probes=0;
        while (work->seen[bucket] && work->seen[bucket]!=node && probes<32) {
            bucket=(bucket+1)&(JC_CALLBACK_HASH_SIZE-1); ++probes;
        }
        if (work->seen[bucket]==node) return JC_CALLBACK_CYCLE;
        if (probes==32) return JC_CALLBACK_STORAGE;
        work->seen[bucket]=node;
        uint64_t fields[5];
        if (!jc_layout_field(read,ctx,node,0,fields,sizeof(fields))) return JC_LAYOUT_READ;
        rows[count]=(struct jc_callback_row){node,fields[0],fields[1],fields[4],0};
        int relevant=0;
        for (unsigned i=0;i<3;++i) if (meshes[i] && fields[1]==meshes[i]) relevant=1;
        if (relevant && !jc_layout_field(read,ctx,rows[count].callback,0,&rows[count].type,8)) return JC_LAYOUT_READ;
        if (relevant && !rows[count].type) return JC_LAYOUT_TYPE;
        out->visited=count+1;
        for (unsigned i=0;i<3;++i) if (meshes[i] && rows[count].owner==meshes[i]) {
            struct jc_callback_set *set=&result.owners[i]; unsigned j=0;
            while (j<set->used && set->types[j]!=rows[count].type) ++j;
            if (j==8) return JC_CALLBACK_TYPE_CAP;
            if (j==set->used) { set->types[j]=rows[count].type; ++set->used; }
            ++set->counts[j]; ++set->total;
        }
        node=rows[count++].next;
    }
    for (unsigned i=0;i<count;++i) {
        uint64_t fields[5];
        if (!jc_layout_field(read,ctx,rows[i].node,0,fields,sizeof(fields)) ||
            fields[0]!=rows[i].next || fields[1]!=rows[i].owner || fields[4]!=rows[i].callback ||
            (rows[i].type && (!jc_layout_field(read,ctx,rows[i].callback,0,&again,8) || again!=rows[i].type))) return JC_LAYOUT_CHANGED;
    }
    if (!jc_layout_field(read,ctx,manager,0x80030,&again,8) || again!=head) return JC_LAYOUT_CHANGED;
    result.visited=count; *out=result; return JC_LAYOUT_OK;
}
#endif
