#ifndef JC_NATIVE_ACTOR_MATERIAL_H
#define JC_NATIVE_ACTOR_MATERIAL_H
#include "native_actor_live.h"
#include "native_cloth_material_live.h"
struct jc_actor_material_proof {
    struct jc_cloth_storage storage[12];
    unsigned result,row,other;
};
enum { JC_ACTOR_MATERIAL_READ=60,JC_ACTOR_MATERIAL_ALIAS,JC_ACTOR_MATERIAL_CHANGED,
    JC_ACTOR_MATERIAL_EMPTY_SETTER };
static inline int jc_actor_material_storage(jc_layout_reader read,void *context,uint64_t material,
        int allow_empty,struct jc_cloth_storage *out) {
    uint64_t slot,link,payload;
    memset(out,0,sizeof(*out));
    if(!jc_layout_field(read,context,material,0x30,&slot,8))return 1;
    if(!slot)return allow_empty?0:1;
    if(!jc_layout_field(read,context,slot,0x10,&link,8))return 1;
    if(!link)return allow_empty?0:1;
    if(!jc_layout_field(read,context,link,0x18,&payload,8))return 1;
    if(!payload)return allow_empty?0:1;
    return jc_cloth_storage_read(read,context,material,out);
}
/* Rows 0..2 are owned body/chest/chirp; 3..5 Local; 6..8 stock Remote;
   9..11 are their floating glyph materials (glyph+90).
   Native24ac12 and117790 write shader values before committing274d70.
   Resource identity alone is therefore insufficient: both writable buffer
   variants must be disjoint, including cross-material comparisons. */
static inline int jc_actor_material_independent(jc_layout_reader read,void *context,
        const struct jc_actor_identity *id,struct jc_actor_material_proof *out) {
    static const unsigned offset[3]={0x1e0,0x830,0xed0};
    if(!read || !id || !out)return JC_ACTOR_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!jc_actor_identity_check(read,context,id))return out->result=JC_ACTOR_IDENTITY;
    uint64_t actors[3]={id->actor,id->barn+0xb0,id->barn+0x146f0},materials[12];
    for(unsigned i=0;i<9;++i)materials[i]=actors[i/3]+offset[i%3];
    for(unsigned i=0;i<3;++i) {
        uint64_t glyph=0;
        if(!jc_layout_field(read,context,actors[i],0x2240,&glyph,8) || !glyph)
            return out->result=JC_ACTOR_MATERIAL_READ;
        materials[9+i]=glyph+0x90;
    }
    for(unsigned row=0;row<12;++row) {
        out->row=row;
        if(jc_actor_material_storage(read,context,materials[row],row>=9 || row%3!=0,out->storage+row))
            return out->result=JC_ACTOR_MATERIAL_READ;
    }
    for(unsigned own=0;own<12;++own) {
        if(own>2 && own!=9)continue;
        for(unsigned other=0;other<12;++other) {
            if(other==own)continue;
            if(out->storage[own].count && out->storage[other].count &&
                !jc_cloth_storage_disjoint(out->storage+own,out->storage+other)) {
                out->row=own;out->other=other;return out->result=JC_ACTOR_MATERIAL_ALIAS;
            }
        }
    }
    for(unsigned row=0;row<12;++row) {
        struct jc_cloth_storage fresh;
        if(jc_actor_material_storage(read,context,materials[row],row>=9 || row%3!=0,&fresh) ||
            memcmp(&fresh,out->storage+row,sizeof(fresh))) {
            out->row=row;return out->result=JC_ACTOR_MATERIAL_CHANGED;
        }
    }
    for(unsigned i=0;i<3;++i) {
        uint64_t glyph=0;
        if(!jc_layout_field(read,context,actors[i],0x2240,&glyph,8) || glyph+0x90!=materials[9+i])
            return out->result=JC_ACTOR_MATERIAL_CHANGED;
    }
    if(!jc_actor_identity_check(read,context,id))return out->result=JC_ACTOR_IDENTITY;
    out->row=out->other=0;return 0;
}
/* Called immediately before native setter/commit or scalar lookup. Empty
   symbols are allowed at admission because 7a4d0 initializes them lazily; an
   actual write requires a fully initialized, disjoint destination. */
static inline int jc_actor_material_write_check(jc_layout_reader read,void *context,
        const struct jc_actor_identity *id,uint64_t object,int payload,
        struct jc_actor_material_proof *proof,int *handled) {
    static const unsigned offsets[3]={0x1e0,0x830,0xed0};
    if(!read || !id || !proof || !handled)return JC_ACTOR_ARGUMENT;
    *handled=0;unsigned selected=12;
    for(unsigned i=0;i<4;++i) {
        uint64_t material=i==3?id->glyph+0x90:id->actor+offsets[i];
        unsigned row=i==3?9:i;
        if(payload!=1) {if(material==object){selected=row;break;}continue;}
        uint64_t slot,link,value;
        if(!jc_layout_field(read,context,material,0x30,&slot,8))return JC_ACTOR_MATERIAL_READ;
        if(!slot)continue;
        if(!jc_layout_field(read,context,slot,0x10,&link,8))return JC_ACTOR_MATERIAL_READ;
        if(!link)continue;
        if(!jc_layout_field(read,context,link,0x18,&value,8))return JC_ACTOR_MATERIAL_READ;
        if(value && value==object){selected=row;break;}
    }
    if(selected==12)return 0;
    *handled=1;
    if(payload==0) {
        uint64_t slot,link;
        if(!jc_layout_field(read,context,object,0x30,&slot,8))return JC_ACTOR_MATERIAL_READ;
        if(!slot)return JC_ACTOR_MATERIAL_EMPTY_SETTER;
        if(!jc_layout_field(read,context,slot,0x10,&link,8))return JC_ACTOR_MATERIAL_READ;
        if(!link)return JC_ACTOR_MATERIAL_EMPTY_SETTER;
    }
    int code=jc_actor_material_independent(read,context,id,proof);
    if(code)return code;
    if(!proof->storage[selected].count){proof->row=selected;return proof->result=JC_ACTOR_MATERIAL_READ;}
    return 0;
}
#endif
