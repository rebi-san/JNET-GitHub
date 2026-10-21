#ifndef JC_NATIVE_APPEARANCE_H
#define JC_NATIVE_APPEARANCE_H
#include "native_material_ready.h"
#include <math.h>

#define JC_APPEARANCE_COUNT 13u
#define JC_APPEARANCE_INSTANCES 16u
struct jc_appearance_detail {
    int field,parameter,table,row;
    uint32_t variant,instances,bytes,offset,declared,required,unique_instances;
};
struct jc_appearance_instance { uint64_t instance,shader,buffer; uint32_t bytes; };
struct jc_appearance_entry {
    float value[4];
    uint64_t texture,handle,owner;
    uint32_t resource_index,texture_type,binding_type,slot_count;
    uint64_t slot_table;
};
struct jc_appearance_plan { uint64_t definition; struct jc_appearance_entry entries[JC_APPEARANCE_COUNT]; };
struct jc_appearance_texture_binding { uint32_t type,resource_index; uint64_t owner,handle; };
struct jc_appearance_bindings {
    uint64_t payload,buffer;
    uint32_t bytes,found,matching,zero;
    uint16_t offsets[JC_APPEARANCE_COUNT];
    uint32_t instance_count;
    struct jc_appearance_instance instances[JC_APPEARANCE_INSTANCES];
    struct jc_appearance_texture_binding textures[JC_APPEARANCE_COUNT];
};
static const char *const jc_appearance_names[JC_APPEARANCE_COUNT]={
    "specular","alphaHardness","localDudeGlow","remoteDudeGlow","textureAtlasA",
    "textureAtlasB","textureDetail","dudeDecoGlow","textureMask","hsvHue",
    "hsvSaturation","hsvValue","envOccCube"
};
static inline unsigned jc_appearance_kind(unsigned i) {
    return i==4 || i==5 || i==6 || i==8 || i==12 ? 2 : i==0 ? 1 : 0;
}
static inline unsigned jc_appearance_bytes(unsigned i) {
    return jc_appearance_kind(i)==2 ? 24 : i==0 ? 16 : 4;
}
struct jc_texture_metadata {
    uint64_t handle,table;
    uint32_t kind,index,flags,count;
};
static inline int jc_texture_metadata_read(jc_layout_reader read,void *ctx,uint64_t texture,
    struct jc_texture_metadata *out) {
    struct jc_texture_metadata value={0};
    unsigned char object[0xa4],handle[0x8c];
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(read,ctx,texture,0x70,object,sizeof(object))) return JC_LAYOUT_READ;
    memcpy(&value.handle,object,8); memcpy(&value.kind,object+0xa0,4);
    if (value.kind<1 || value.kind>3) return JC_LAYOUT_TYPE;
    unsigned list=value.kind==2 ? 0x38 : 0x30;
    unsigned bytes=value.kind==1 ? sizeof(handle) : list+16-0xc;
    if (!jc_layout_field(read,ctx,value.handle,0xc,handle,bytes)) return JC_LAYOUT_READ;
    memcpy(&value.index,handle,4);
    memcpy(&value.count,handle+(list-0xc),4); memcpy(&value.table,handle+(list+8-0xc),8);
    if (value.kind==1) memcpy(&value.flags,handle+(0x94-0xc),4);
    *out=value; return JC_LAYOUT_OK;
}
/* PC 9d9a0 reads these definition rows; no game getters or setters run here. */
static inline int jc_appearance_plan_read(jc_layout_reader read,void *ctx,uint64_t definition,
                                          struct jc_appearance_plan *out) {
    struct jc_appearance_plan plan={0}; unsigned char rows[13*128],again[13*128];
    uint32_t count,check,seen=0;
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(read,ctx,definition,0xf20,&count,4)) return JC_LAYOUT_READ;
    if (count!=JC_APPEARANCE_COUNT) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,ctx,definition,0x720,rows,sizeof(rows))) return JC_LAYOUT_READ;
    for (unsigned row=0;row<count;++row) {
        const unsigned char *p=rows+row*128; unsigned index=JC_APPEARANCE_COUNT; uint32_t kind;
        if (!memchr(p+0x20,0,20)) return JC_LAYOUT_RANGE;
        for (unsigned n=0;n<JC_APPEARANCE_COUNT;++n)
            if (!strcmp((const char *)p+0x20,jc_appearance_names[n])) { index=n; break; }
        if (index==JC_APPEARANCE_COUNT || (seen&(1u<<index))) return JC_LAYOUT_TYPE;
        memcpy(&kind,p+0x78,4);
        if (kind!=jc_appearance_kind(index)) return JC_LAYOUT_TYPE;
        seen|=1u<<index;
        struct jc_appearance_entry *entry=&plan.entries[index];
        if (kind==2) {
            uint64_t resource,payload,resource_again,payload_again,texture_again;
            struct jc_texture_metadata metadata,check_metadata;
            memcpy(&resource,p+0x48,8);
            if (!jc_layout_field(read,ctx,resource,0x10,&payload,8) ||
                !jc_layout_field(read,ctx,payload,0xc8,&entry->texture,8)) return JC_LAYOUT_READ;
            int code=jc_texture_metadata_read(read,ctx,entry->texture,&metadata); if (code) return code;
            entry->handle=metadata.handle; entry->resource_index=metadata.index; entry->texture_type=metadata.kind;
            if (!jc_layout_address(entry->texture,8,&entry->owner)) return JC_LAYOUT_TYPE;
            entry->binding_type=0x41+entry->texture_type;
            if (entry->texture_type==1) entry->binding_type|=metadata.flags&0x80000000u;
            if (!jc_layout_field(read,ctx,definition,0x768+row*128,&resource_again,8) ||
                !jc_layout_field(read,ctx,resource,0x10,&payload_again,8) ||
                !jc_layout_field(read,ctx,payload,0xc8,&texture_again,8)) return JC_LAYOUT_READ;
            code=jc_texture_metadata_read(read,ctx,entry->texture,&check_metadata); if (code) return code;
            if (resource_again!=resource || payload_again!=payload || texture_again!=entry->texture ||
                check_metadata.handle!=entry->handle || check_metadata.index!=entry->resource_index ||
                check_metadata.kind!=entry->texture_type || check_metadata.flags!=metadata.flags) return JC_LAYOUT_CHANGED;
        } else {
            float start[4],end[4],t;
            memcpy(start,p,16); memcpy(end,p+16,16); memcpy(&t,p+0x34,4);
            if (!isfinite(t) || fabsf(t)>16) return JC_LAYOUT_RANGE;
            for (unsigned j=0;j<4;++j) {
                if (!isfinite(start[j]) || !isfinite(end[j])) return JC_LAYOUT_RANGE;
                entry->value[j]=start[j]+(end[j]-start[j])*t;
                if (!isfinite(entry->value[j])) return JC_LAYOUT_RANGE;
            }
        }
    }
    if (!jc_layout_field(read,ctx,definition,0xf20,&check,4) ||
        !jc_layout_field(read,ctx,definition,0x720,again,sizeof(again))) return JC_LAYOUT_READ;
    if (count!=check || memcmp(rows,again,sizeof(rows))) return JC_LAYOUT_CHANGED;
    plan.definition=definition; *out=plan; return JC_LAYOUT_OK;
}
/* Native setters select member zero for both inline and indirect collections.
   Other members are inventoried for ownership/alias checks, never added to the setter loop. */
static inline int jc_appearance_bindings_read_detail(jc_layout_reader read,void *ctx,uint64_t material,
                                              const struct jc_appearance_plan *plan,
                                              struct jc_appearance_bindings *out,struct jc_appearance_detail *detail) {
    struct jc_appearance_bindings b={0}; uint64_t collection,shader,first,again;
    uint64_t slot,link,members[JC_APPEARANCE_INSTANCES],check_members[JC_APPEARANCE_INSTANCES];
    unsigned char payload_header[20],collection_header[16];
    uint32_t encoded; int32_t variant; unsigned char headers[32],check_headers[32],buffer[4096],check[4096];
    uint16_t ranges[512][2]; int indices[512]; unsigned declarations=0;
    unsigned char entries_data[256*24],entries_check[256*24];
    char names[256][32],names_check[256][32];
    memset(out,0,sizeof(*out));
    memset(detail,0,sizeof(*detail)); detail->parameter=detail->table=detail->row=-1;
    detail->field=1;
    if (!jc_layout_field(read,ctx,material,0x30,&slot,8) ||
        !jc_layout_field(read,ctx,slot,0x10,&link,8) ||
        !jc_layout_field(read,ctx,link,0x18,&b.payload,8)) return JC_LAYOUT_READ;
    int code;
    detail->field=2;
    if (!jc_layout_field(read,ctx,b.payload,0x80,payload_header,sizeof(payload_header))) return JC_LAYOUT_READ;
    memcpy(&variant,payload_header+16,4);
    detail->variant=(uint32_t)variant;
    if (variant<0 || variant>1) return JC_LAYOUT_RANGE;
    detail->field=3;
    memcpy(&collection,payload_header+8*variant,8);
    if (!jc_layout_field(read,ctx,collection,0,collection_header,sizeof(collection_header))) return JC_LAYOUT_READ;
    memcpy(&encoded,collection_header,4); memcpy(&first,collection_header+8,8);
    b.instance_count=encoded&0x7fffffff; detail->instances=b.instance_count;
    if (!b.instance_count || b.instance_count>JC_APPEARANCE_INSTANCES) return JC_LAYOUT_RANGE;
    members[0]=first;
    if (b.instance_count>1 && !jc_layout_field(read,ctx,first,0,members,b.instance_count*8)) return JC_LAYOUT_READ;
    for (unsigned n=0;n<b.instance_count;++n) {
        struct jc_appearance_instance *member=&b.instances[n]; uint64_t end,member_header[2];
        detail->field=4; detail->row=(int)n;
        member->instance=members[n];
        unsigned prior_index=0;
        while (prior_index<n && b.instances[prior_index].instance!=member->instance) ++prior_index;
        if (prior_index<n) { *member=b.instances[prior_index]; continue; }
        if (!jc_layout_field(read,ctx,member->instance,0,member_header,sizeof(member_header))) return JC_LAYOUT_READ;
        member->shader=member_header[0]; member->buffer=member_header[1];
        if (!jc_layout_field(read,ctx,member->buffer,0,&member->bytes,4)) return JC_LAYOUT_READ;
        detail->field=5; detail->bytes=member->bytes;
        if (member->bytes<4 || member->bytes>sizeof(buffer) ||
            !jc_layout_address(member->buffer,member->bytes,&end)) return JC_LAYOUT_RANGE;
        for (unsigned j=0;j<n;++j) {
            const struct jc_appearance_instance *prior=&b.instances[j];
            if (member->buffer<prior->buffer+prior->bytes && prior->buffer<end) {
                detail->field=10; return 5;
            }
        }
        ++detail->unique_instances;
    }
    shader=b.instances[0].shader;
    b.buffer=b.instances[0].buffer; b.bytes=b.instances[0].bytes; detail->bytes=b.bytes;
    if (!jc_layout_field(read,ctx,shader,0x40,headers,sizeof(headers))) return JC_LAYOUT_READ;
    if (!jc_layout_field(read,ctx,b.buffer,0,buffer,b.bytes)) return JC_LAYOUT_READ;
    uint32_t captured_bytes; memcpy(&captured_bytes,buffer,4);
    if (captured_bytes!=b.bytes) return JC_LAYOUT_CHANGED;
    for (unsigned table=0;table<2;++table) {
        detail->field=6; detail->table=(int)table; detail->row=-1;
        uint32_t count; uint64_t entries; memcpy(&count,headers+table*16,4); count&=0x7fffffff;
        memcpy(&entries,headers+table*16+8,8); if (count>256) return JC_LAYOUT_RANGE;
        if (count && !jc_layout_field(read,ctx,entries,0,entries_data,count*24)) return JC_LAYOUT_READ;
        code=jc_material_names_read(read,ctx,entries_data,count,names); if (code) return code;
        for (unsigned row=0;row<count;++row) {
            detail->field=7; detail->row=(int)row; detail->parameter=-1;
            const unsigned char *entry=entries_data+row*24;
            const char *name=names[row];
            unsigned i=JC_APPEARANCE_COUNT;
            for (unsigned n=0;n<JC_APPEARANCE_COUNT;++n)
                if (!memcmp(name,jc_appearance_names[n],strlen(jc_appearance_names[n])+1)) { i=n; break; }
            uint16_t offset,size; memcpy(&offset,entry+16,2); memcpy(&size,entry+18,2); size&=0x1fff;
            ranges[declarations][0]=offset; ranges[declarations][1]=size;
            indices[declarations++]=(int)i;
            if (i==JC_APPEARANCE_COUNT) continue;
            detail->field=8; detail->parameter=(int)i;
            if ((b.found&(1u<<i)) || (jc_appearance_kind(i)==2 && table!=0)) return JC_LAYOUT_TYPE;
            unsigned width=jc_appearance_bytes(i);
            detail->field=9; detail->offset=offset; detail->declared=size; detail->required=width;
            if (offset<4 || size<width || (uint32_t)offset+size>b.bytes ||
                (jc_appearance_kind(i)==2 && size!=24)) return JC_LAYOUT_RANGE;
            if (jc_appearance_kind(i)==2) {
                detail->field=8;
                unsigned kind=plan->entries[i].texture_type,type=entry[2];
                if (kind<1 || kind>3 || (kind==1 ? !(type==0x42 || type==0x12 ||
                    (type>=0xd9 && type<=0xdc)) : type!=0x41+kind)) return JC_LAYOUT_TYPE;
            }
            b.found|=1u<<i;
            b.offsets[i]=offset;
            unsigned zero=1; for (unsigned j=0;j<width;++j) zero&=buffer[offset+j]==0;
            if (zero) b.zero|=1u<<i;
            if (jc_appearance_kind(i)==2) {
                uint32_t type,resource_index; uint64_t owner,handle;
                memcpy(&type,buffer+offset,4); memcpy(&resource_index,buffer+offset+4,4);
                memcpy(&owner,buffer+offset+8,8); memcpy(&handle,buffer+offset+16,8);
                b.textures[i]=(struct jc_appearance_texture_binding){type,resource_index,owner,handle};
                const struct jc_appearance_entry *expected=&plan->entries[i];
                if (type==expected->binding_type &&
                    owner==expected->owner && handle==expected->handle) b.matching|=1u<<i;
            } else if (!memcmp(buffer+offset,plan->entries[i].value,width)) b.matching|=1u<<i;
        }
        if (count && !jc_layout_field(read,ctx,entries,0,entries_check,count*24)) return JC_LAYOUT_READ;
        if (memcmp(entries_data,entries_check,count*24)) return JC_LAYOUT_CHANGED;
        code=jc_material_names_read(read,ctx,entries_data,count,names_check); if (code) return code;
        if (memcmp(names,names_check,count*32)) return JC_LAYOUT_CHANGED;
    }
    detail->field=10;
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (b.found&(1u<<i))
        for (unsigned j=0;j<declarations;++j) if (indices[j]!=(int)i && ranges[j][1] &&
            b.offsets[i]<(uint32_t)ranges[j][0]+ranges[j][1] &&
            ranges[j][0]<b.offsets[i]+jc_appearance_bytes(i)) return JC_LAYOUT_TYPE;
    uint32_t count_again; int32_t variant_again;
    detail->field=11;
    if (!jc_layout_field(read,ctx,shader,0x40,check_headers,32) ||
        !jc_layout_field(read,ctx,b.buffer,0,check,b.bytes)) return JC_LAYOUT_READ;
    if (memcmp(headers,check_headers,32) ||
        memcmp(buffer,check,b.bytes)) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,b.payload,0x80,payload_header,sizeof(payload_header)) ||
        !jc_layout_field(read,ctx,collection,0,collection_header,sizeof(collection_header))) return JC_LAYOUT_READ;
    memcpy(&variant_again,payload_header+16,4); memcpy(&again,payload_header+8*variant,8);
    memcpy(&count_again,collection_header,4);
    if (variant_again!=variant || again!=collection || count_again!=encoded) return JC_LAYOUT_CHANGED;
    memcpy(&again,collection_header+8,8);
    if (again!=first) return JC_LAYOUT_CHANGED;
    if (b.instance_count>1) {
        if (!jc_layout_field(read,ctx,first,0,check_members,b.instance_count*8)) return JC_LAYOUT_READ;
        if (memcmp(members,check_members,b.instance_count*8)) return JC_LAYOUT_CHANGED;
    }
    for (unsigned n=0;n<b.instance_count;++n) {
        const struct jc_appearance_instance *member=&b.instances[n]; uint32_t bytes; uint64_t member_header[2];
        unsigned prior_index=0;
        while (prior_index<n && b.instances[prior_index].instance!=member->instance) ++prior_index;
        if (prior_index<n) continue;
        if (!jc_layout_field(read,ctx,member->instance,0,member_header,sizeof(member_header))) return JC_LAYOUT_READ;
        if (member_header[0]!=member->shader || member_header[1]!=member->buffer) return JC_LAYOUT_CHANGED;
        if (!jc_layout_field(read,ctx,member->buffer,0,&bytes,4)) return JC_LAYOUT_READ;
        if (bytes!=member->bytes) return JC_LAYOUT_CHANGED;
    }
    if (!jc_layout_field(read,ctx,shader,0,&again,8)) return JC_LAYOUT_READ;
    if (!jc_layout_field(read,ctx,material,0x30,&again,8)) return JC_LAYOUT_READ;
    if (again!=slot) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,slot,0x10,&again,8)) return JC_LAYOUT_READ;
    if (again!=link) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,link,0x18,&again,8)) return JC_LAYOUT_READ;
    if (again!=b.payload) return JC_LAYOUT_CHANGED;
    detail->field=0;
    *out=b; return JC_LAYOUT_OK;
}
static inline int jc_appearance_bindings_read(jc_layout_reader read,void *ctx,uint64_t material,
    const struct jc_appearance_plan *plan,struct jc_appearance_bindings *out) {
    struct jc_appearance_detail detail;
    return jc_appearance_bindings_read_detail(read,ctx,material,plan,out,&detail);
}
static inline int jc_appearance_storage_same(const struct jc_appearance_bindings *a,
                                             const struct jc_appearance_bindings *b) {
    return a->payload==b->payload && a->buffer==b->buffer && a->bytes==b->bytes &&
        a->instance_count==b->instance_count && !memcmp(a->instances,b->instances,sizeof(a->instances)) &&
        a->found==b->found && !memcmp(a->offsets,b->offsets,sizeof(a->offsets));
}
static inline int jc_appearance_storage_disjoint(const struct jc_appearance_bindings *a,
                                                 const struct jc_appearance_bindings *b) {
    for (unsigned i=0;i<a->instance_count;++i) for (unsigned j=0;j<b->instance_count;++j)
        if (a->instances[i].buffer<b->instances[j].buffer+b->instances[j].bytes &&
            b->instances[j].buffer<a->instances[i].buffer+a->instances[i].bytes) return 0;
    return 1;
}
struct jc_appearance_setters {
    int (*scalar)(void *,uint64_t,const char *,float);
    int (*vector)(void *,uint64_t,const char *,const float[4]);
    int (*texture)(void *,uint64_t,const char *,uint64_t);
};
struct jc_appearance_preparation {
    unsigned stage,calls,check;
    int originals_unchanged;
    uint32_t source_found,trial_before,trial_after;
    struct jc_appearance_detail detail;
    int source_reason,source_parameter,source_code;
    uint32_t trial_found,source_matching,missing_source,missing_trial,definition_difference;
    uint32_t refreshed_indices,cube_stored_index,cube_current_index,cube_slots;
};

/* PC 34c7d0 refreshes binding+4 from handle+c in the render copy. 34ed50
   uses that index in a 48-byte (2D/cube) or 40-byte (volume) resource slot.
   A material's cached index may differ; its owner/type/handle may not. */
static inline int jc_appearance_texture_same(const struct jc_appearance_entry *a,
                                             const struct jc_appearance_entry *b) {
    return a->texture==b->texture && a->owner==b->owner && a->handle==b->handle &&
        a->texture_type==b->texture_type && a->binding_type==b->binding_type &&
        a->slot_count==b->slot_count && a->slot_table==b->slot_table;
}
static inline int jc_appearance_bound_texture(jc_layout_reader read,void *ctx,
    const struct jc_appearance_texture_binding *binding,unsigned expected_kind,
    struct jc_appearance_entry *out) {
    struct jc_appearance_entry value={0}; uint64_t end,end_slots;
    struct jc_texture_metadata metadata,again;
    memset(out,0,sizeof(*out));
    if (binding->owner<8 || !binding->handle ||
        !jc_layout_address(binding->owner-8,0x114,&end)) return JC_LAYOUT_RANGE;
    value.texture=binding->owner-8; value.owner=binding->owner;
    int code=jc_texture_metadata_read(read,ctx,value.texture,&metadata); if (code) return code;
    value.handle=metadata.handle; value.texture_type=metadata.kind; value.resource_index=metadata.index;
    if (value.texture_type!=expected_kind) return JC_LAYOUT_TYPE;
    value.binding_type=(0x41+value.texture_type)|(metadata.flags&0x80000000u);
    if (binding->type!=value.binding_type || binding->handle!=value.handle) return JC_LAYOUT_CHANGED;
    unsigned list=value.texture_type==2 ? 0x38 : 0x30;
    unsigned stride=value.texture_type==2 ? 40 : 48;
    value.slot_count=metadata.count&0x7fffffffu;
    if (!value.slot_count || value.slot_count>16 || value.resource_index>=value.slot_count ||
        binding->resource_index>=value.slot_count) return JC_LAYOUT_RANGE;
    if (!jc_layout_address(value.handle,list+8,&value.slot_table)) return JC_LAYOUT_RANGE;
    if (value.slot_count>1) value.slot_table=metadata.table;
    if (!jc_layout_address(value.slot_table,(uint64_t)stride*value.slot_count,&end_slots)) return JC_LAYOUT_RANGE;
    unsigned char slots[16*48];
    if (!jc_layout_field(read,ctx,value.slot_table,0,slots,stride*value.slot_count)) return JC_LAYOUT_READ;
    code=jc_texture_metadata_read(read,ctx,value.texture,&again); if (code) return code;
    if (again.handle!=value.handle || again.kind!=value.texture_type ||
        again.index!=value.resource_index || again.flags!=metadata.flags || again.count!=metadata.count ||
        (value.slot_count>1 && again.table!=value.slot_table)) return JC_LAYOUT_CHANGED;
    *out=value; return JC_LAYOUT_OK;
}

static inline int jc_appearance_source_plan(jc_layout_reader read,void *ctx,
    const struct jc_appearance_bindings *source,struct jc_appearance_plan *plan,int *parameter) {
    *parameter=-1;
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)==2 && (source->found&(1u<<i))) {
        struct jc_appearance_entry value;
        *parameter=(int)i;
        int code=jc_appearance_bound_texture(read,ctx,&source->textures[i],plan->entries[i].texture_type,&value);
        if (code) return code;
        plan->entries[i]=value;
    }
    *parameter=-1; return JC_LAYOUT_OK;
}
static inline int jc_appearance_first_parameter(uint32_t mask) {
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (mask&(1u<<i)) return (int)i;
    return -1;
}
/* Caller owns the game-thread/chapter lifetime guard and supplies pinned native setters.
   A failure after stage 1 is not rolled back by writing stale memory or freeing resources. */
static inline int jc_appearance_prepare(jc_layout_reader read,void *ctx,
                                         const struct jc_appearance_setters *setters,
                                         uint64_t definition,const uint64_t materials[3],
                                         struct jc_appearance_preparation *out) {
    struct jc_appearance_plan plan,definition_plan,check_plan;
    struct jc_appearance_bindings before[3],after;
    unsigned char buffers[3][4096],check[4096];
    const uint32_t required=(1u<<4)|(1u<<5)|(1u<<6)|(1u<<8);
    memset(out,0,sizeof(*out)); out->originals_unchanged=-1; out->source_parameter=-1;
    if (!setters || !setters->scalar || !setters->vector || !setters->texture) return JC_LAYOUT_TYPE;
    out->check=1;
    int code=jc_appearance_plan_read(read,ctx,definition,&plan); if (code) return code;
    definition_plan=plan;
    for (unsigned i=0;i<3;++i) {
        out->check=2+i;
        code=jc_appearance_bindings_read_detail(read,ctx,materials[i],&plan,&before[i],&out->detail); if (code) return code;
        for (unsigned j=0;j<i;++j)
            if (!jc_appearance_storage_disjoint(&before[i],&before[j])) { out->detail.field=10; return 5; }
        if (!jc_layout_field(read,ctx,before[i].buffer,0,buffers[i],before[i].bytes)) return JC_LAYOUT_READ;
    }
    out->check=5;
    out->source_found=before[0].found; out->trial_before=before[2].matching;
    out->trial_found=before[2].found; out->source_matching=before[0].matching;
    uint32_t textures=required|(1u<<12);
    out->missing_source=(required|(before[2].found&textures))&~before[0].found;
    out->missing_trial=required&~before[2].found;
    out->definition_difference=before[0].found&textures&~before[0].matching;
    if (out->missing_source || out->missing_trial) {
        out->source_reason=out->missing_source ? 1 : 2;
        out->source_parameter=jc_appearance_first_parameter(out->missing_source ? out->missing_source : out->missing_trial);
        return 6;
    }
    out->source_code=jc_appearance_source_plan(read,ctx,&before[0],&plan,&out->source_parameter);
    if (out->source_code) { out->source_reason=3; return 6; }
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)==2 &&
        (before[0].found&(1u<<i)) && before[0].textures[i].resource_index!=plan.entries[i].resource_index)
        out->refreshed_indices|=1u<<i;
    out->cube_stored_index=before[0].textures[12].resource_index;
    out->cube_current_index=plan.entries[12].resource_index;
    out->cube_slots=plan.entries[12].slot_count;
    /* Capture current color/glow values, since the style update supersedes definition defaults. */
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)!=2 && (before[0].found&(1u<<i))) {
        unsigned width=jc_appearance_bytes(i);
        memcpy(plan.entries[i].value,buffers[0]+before[0].offsets[i],width);
        for (unsigned j=0;j<width/4;++j) if (!isfinite(plan.entries[i].value[j])) return JC_LAYOUT_RANGE;
    }
    code=jc_appearance_plan_read(read,ctx,definition,&check_plan); if (code) return code;
    /* Compare immutable texture resolution again; scalar source values intentionally come from the material. */
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)==2 &&
        !jc_appearance_texture_same(&definition_plan.entries[i],&check_plan.entries[i])) return JC_LAYOUT_CHANGED;
    for (unsigned i=0;i<3;++i) {
        code=jc_appearance_bindings_read_detail(read,ctx,materials[i],&plan,&after,&out->detail); if (code) return code;
        if (!jc_appearance_storage_same(&after,&before[i]) ||
            !jc_layout_field(read,ctx,after.buffer,0,check,after.bytes) ||
            memcmp(check,buffers[i],after.bytes)) return JC_LAYOUT_CHANGED;
    }
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)==2 && (before[0].found&(1u<<i))) {
        struct jc_appearance_entry value;
        code=jc_appearance_bound_texture(read,ctx,&before[0].textures[i],plan.entries[i].texture_type,&value);
        if (code || memcmp(&value,&plan.entries[i],sizeof(value))) {
            out->source_reason=4; out->source_parameter=(int)i;
            out->source_code=code ? code : JC_LAYOUT_CHANGED; return 6;
        }
    }
    out->stage=1; out->check=6;
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (before[2].found&(1u<<i)) {
        unsigned kind=jc_appearance_kind(i); int ok;
        ++out->calls;
        if (kind==2) ok=setters->texture(ctx,materials[2],jc_appearance_names[i],plan.entries[i].texture);
        else if (kind==1) ok=setters->vector(ctx,materials[2],jc_appearance_names[i],plan.entries[i].value);
        else ok=setters->scalar(ctx,materials[2],jc_appearance_names[i],plan.entries[i].value[0]);
        if (!ok) return 7;
    }
    out->stage=2;
    for (unsigned i=0;i<3;++i) {
        out->check=i<2 ? 7 : 8;
        code=jc_appearance_bindings_read_detail(read,ctx,materials[i],&plan,&after,&out->detail); if (code) return code;
        if (!jc_appearance_storage_same(&after,&before[i])) return 8;
        if (i<2) {
            if (!jc_layout_field(read,ctx,after.buffer,0,check,after.bytes) ||
                memcmp(check,buffers[i],after.bytes)) { out->originals_unchanged=0; return 8; }
        } else {
            out->trial_after=after.matching;
            if (after.matching!=after.found) return 8;
            if (!jc_layout_field(read,ctx,after.buffer,0,check,after.bytes)) return JC_LAYOUT_READ;
            for (unsigned byte=0;byte<after.bytes;++byte) {
                int permitted=0;
                for (unsigned n=0;n<JC_APPEARANCE_COUNT;++n) if ((after.found&(1u<<n)) &&
                    byte>=after.offsets[n] && byte<after.offsets[n]+jc_appearance_bytes(n)) permitted=1;
                if (!permitted && check[byte]!=buffers[2][byte]) return 8;
            }
        }
    }
    out->originals_unchanged=1; out->stage=3; return JC_LAYOUT_OK;
}
struct jc_appearance_borrow {
    struct jc_appearance_plan plan,definition_plan;
    struct jc_appearance_bindings bindings[2];
    uint64_t materials[2];
    int ready;
};
/* Same-thread, same-chapter borrowed resources only. This does not acquire references.
   The caller must stop drawing on any failure, including before chapter teardown. */
struct jc_appearance_borrow_detail { int stage,parameter,role; };
static inline int jc_appearance_borrow_check_detail(jc_layout_reader read,void *ctx,
                                             const struct jc_appearance_borrow *borrow,struct jc_appearance_borrow_detail *detail) {
    *detail=(struct jc_appearance_borrow_detail){1,-1,-1};
    struct jc_appearance_plan plan;
    if (!borrow->ready) return JC_LAYOUT_TYPE;
    int code=jc_appearance_plan_read(read,ctx,borrow->plan.definition,&plan); if (code) return code;
    uint32_t textures=0;
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)==2) {
        detail->stage=2; detail->parameter=(int)i;
        textures|=1u<<i;
        if (!jc_appearance_texture_same(&plan.entries[i],&borrow->definition_plan.entries[i])) return JC_LAYOUT_CHANGED;
        if (borrow->bindings[0].found&(1u<<i)) {
            detail->stage=3;
            struct jc_appearance_entry value;
            code=jc_appearance_bound_texture(read,ctx,&borrow->bindings[0].textures[i],
                borrow->plan.entries[i].texture_type,&value);
            if (code) return code;
            if (!jc_appearance_texture_same(&value,&borrow->plan.entries[i])) return JC_LAYOUT_CHANGED;
        }
    }
    for (unsigned i=0;i<2;++i) {
        detail->stage=4; detail->role=(int)i; detail->parameter=-1;
        struct jc_appearance_bindings b;
        const struct jc_appearance_bindings *old=&borrow->bindings[i];
        code=jc_appearance_bindings_read(read,ctx,borrow->materials[i],&borrow->plan,&b); if (code) return code;
        detail->stage=5;
        if (!jc_appearance_storage_same(&b,old)) return JC_LAYOUT_CHANGED;
        detail->stage=6;
        if ((b.matching&textures)!=(b.found&textures)) {
            for (unsigned n=0;n<JC_APPEARANCE_COUNT;++n) if (((b.matching^b.found)&textures)&(1u<<n)) { detail->parameter=(int)n; break; }
            return JC_LAYOUT_CHANGED;
        }
        for (unsigned n=0;n<JC_APPEARANCE_COUNT;++n) if ((b.found&(1u<<n)) && jc_appearance_kind(n)==2) {
            detail->stage=7; detail->parameter=(int)n;
            struct jc_appearance_entry value;
            code=jc_appearance_bound_texture(read,ctx,&b.textures[n],borrow->plan.entries[n].texture_type,&value);
            if (code) return code;
            if (!jc_appearance_texture_same(&value,&borrow->plan.entries[n])) return JC_LAYOUT_CHANGED;
        }
    }
    *detail=(struct jc_appearance_borrow_detail){0,-1,-1};
    return JC_LAYOUT_OK;
}
static inline int jc_appearance_borrow_check(jc_layout_reader read,void *ctx,const struct jc_appearance_borrow *borrow) {
    struct jc_appearance_borrow_detail detail;
    return jc_appearance_borrow_check_detail(read,ctx,borrow,&detail);
}
static inline int jc_appearance_borrow_begin(jc_layout_reader read,void *ctx,uint64_t definition,
                                             uint64_t source,uint64_t trial,struct jc_appearance_borrow *out) {
    struct jc_appearance_borrow borrow={0};
    memset(out,0,sizeof(*out));
    int code=jc_appearance_plan_read(read,ctx,definition,&borrow.plan); if (code) return code;
    borrow.definition_plan=borrow.plan;
    borrow.materials[0]=source; borrow.materials[1]=trial;
    struct jc_appearance_bindings source_bindings; int parameter;
    code=jc_appearance_bindings_read(read,ctx,source,&borrow.plan,&source_bindings); if (code) return code;
    code=jc_appearance_source_plan(read,ctx,&source_bindings,&borrow.plan,&parameter); if (code) return code;
    for (unsigned i=0;i<2;++i) {
        code=jc_appearance_bindings_read(read,ctx,borrow.materials[i],&borrow.plan,&borrow.bindings[i]);
        if (code) return code;
    }
    if (!jc_appearance_storage_disjoint(&borrow.bindings[0],&borrow.bindings[1])) return 5;
    if (!jc_appearance_storage_same(&borrow.bindings[0],&source_bindings)) return JC_LAYOUT_CHANGED;
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (jc_appearance_kind(i)==2 &&
        (borrow.bindings[1].found&(1u<<i)) && !(borrow.bindings[0].found&(1u<<i))) return JC_LAYOUT_TYPE;
    borrow.ready=1;
    code=jc_appearance_borrow_check(read,ctx,&borrow); if (code) return code;
    *out=borrow; return JC_LAYOUT_OK;
}
static inline int jc_body_appearance_source(jc_layout_reader read,void *ctx,uint64_t barn,
        uint64_t actor,uint64_t *definition,uint32_t outfits[2]) {
    if(!definition || !outfits)return JC_LAYOUT_RANGE;
    if(!jc_layout_field(read,ctx,barn,0xb0+0x2238,definition,8) || !*definition ||
        !jc_layout_field(read,ctx,barn,0xb0+0x70,outfits,4) ||
        !jc_layout_field(read,ctx,actor,0x70,outfits+1,4))return JC_LAYOUT_READ;
    return outfits[0]<=6 && outfits[1]<=6 ? JC_LAYOUT_OK : JC_LAYOUT_RANGE;
}
struct jc_body_appearance_lease {
    uint64_t material,pairs[4][2];
    unsigned held,applied,stage;
};
struct jc_body_appearance_ops {
    struct jc_appearance_setters setters;
    int (*hold)(void *,const uint64_t[2]);
    void (*drop)(void *,const uint64_t[2]);
};
struct jc_body_outfit_lease {
    uint64_t pairs[2][2][2];
    unsigned held[2][2],selected;
};
typedef int (*jc_body_texture_lookup)(void *,const char *,uint64_t[2]);
static inline int jc_body_outfit_apply(void *ctx,const struct jc_body_appearance_ops *ops,
        jc_body_texture_lookup lookup,uint64_t material,unsigned outfit,struct jc_body_outfit_lease *lease) {
    static const char *const resources[2][2]={{"Ninja","NinjaGlow"},{"NinjaWhite","NinjaWhiteGlow"}};
    static const char *const parameters[2]={"textureAtlasA","textureAtlasB"};
    if(!ops || !lookup || !lease || !material || outfit>6)return JC_LAYOUT_RANGE;
    unsigned color=outfit>3;
    if(lease->selected==color+1)return 0;
    for(unsigned i=0;i<2;++i)if(!lease->held[color][i]) {
        uint64_t pair[2],again[2];
        if(!lookup(ctx,resources[color][i],pair) || !pair[0] || !pair[1] || !ops->hold(ctx,pair))return JC_LAYOUT_READ;
        memcpy(lease->pairs[color][i],pair,16);lease->held[color][i]=1;
        if(!lookup(ctx,resources[color][i],again) || memcmp(pair,again,16))return JC_LAYOUT_CHANGED;
    }
    for(unsigned i=0;i<2;++i)
        if(!ops->setters.texture(ctx,material,parameters[i],lease->pairs[color][i][0]))return JC_LAYOUT_CHANGED;
    lease->selected=color+1;return 0;
}
static inline int jc_body_outfit_release(void *ctx,const struct jc_body_appearance_ops *ops,
        int destroyed,struct jc_body_outfit_lease *lease) {
    if(!ops || !lease || !destroyed)return JC_LAYOUT_CHANGED;
    for(unsigned color=0;color<2;++color)for(unsigned i=0;i<2;++i)if(lease->held[color][i])
        ops->drop(ctx,lease->pairs[color][i]);
    memset(lease,0,sizeof(*lease));return 0;
}
static inline int jc_body_appearance_prepare(jc_layout_reader read,void *ctx,
        const struct jc_body_appearance_ops *ops,uint64_t definition,const uint64_t materials[3],
        struct jc_body_appearance_lease *lease) {
    static const unsigned textures[4]={4,5,6,8};
    static const unsigned numeric[3]={0,1,7};
    struct jc_appearance_plan plan;
    struct jc_appearance_bindings before[3],after;
    unsigned char buffers[3][4096],check[4096],rows[13*128];
    uint64_t pairs[4][2]={{0}};
    if (!lease || lease->material || !ops || !ops->hold || !ops->drop ||
        !ops->setters.scalar || !ops->setters.vector || !ops->setters.texture) return JC_LAYOUT_TYPE;
    lease->stage=1;
    int code=jc_appearance_plan_read(read,ctx,definition,&plan);if(code)return code;
    for(unsigned i=0;i<3;++i) {
        code=jc_appearance_bindings_read(read,ctx,materials[i],&plan,&before[i]);if(code)return code;
        if(before[i].bytes>4096 || !jc_layout_field(read,ctx,before[i].buffer,0,buffers[i],before[i].bytes))return JC_LAYOUT_READ;
        for(unsigned j=0;j<i;++j)if(!jc_appearance_storage_disjoint(before+i,before+j))return JC_LAYOUT_CHANGED;
    }
    lease->stage=2;
    if(!jc_layout_field(read,ctx,definition,0x720,rows,sizeof(rows)))return JC_LAYOUT_READ;
    for(unsigned t=0;t<4;++t) {
        unsigned i=textures[t],matches=0;
        if(!(before[0].found&(1u<<i)) || !(before[2].found&(1u<<i)))return JC_LAYOUT_CHANGED;
        for(unsigned r=0;r<13;++r) {
            const unsigned char *row=rows+128*r;uint64_t resource=0,payload=0;
            if(!memchr(row+0x20,0,20))return JC_LAYOUT_RANGE;
            if(strcmp((const char *)row+0x20,jc_appearance_names[i]))continue;
            ++matches;memcpy(&resource,row+0x48,8);
            if(!jc_layout_field(read,ctx,resource,0x10,&payload,8) ||
                !jc_layout_field(read,ctx,payload,0xc8,pairs[t],16))return JC_LAYOUT_READ;
        }
        if(matches!=1 || pairs[t][0]!=plan.entries[i].texture || !pairs[t][1] || (pairs[t][1]&3))return JC_LAYOUT_CHANGED;
    }
    for(unsigned n=0;n<3;++n) {
        unsigned i=numeric[n];
        if(!(before[0].found&(1u<<i)) || !(before[2].found&(1u<<i)))return JC_LAYOUT_TYPE;
        memcpy(plan.entries[i].value,buffers[0]+before[0].offsets[i],jc_appearance_bytes(i));
        for(unsigned j=0;j<jc_appearance_bytes(i)/4;++j)if(!isfinite(plan.entries[i].value[j]))return JC_LAYOUT_RANGE;
    }
    lease->stage=3;lease->material=materials[2];
    for(unsigned t=0;t<4;++t) {
        if(!ops->hold(ctx,pairs[t]))return JC_LAYOUT_CHANGED;
        memcpy(lease->pairs[t],pairs[t],16);++lease->held;
    }
    for(unsigned i=0;i<3;++i) {
        code=jc_appearance_bindings_read(read,ctx,materials[i],&plan,&after);if(code)return code;
        if(!jc_appearance_storage_same(&after,before+i) ||
            !jc_layout_field(read,ctx,after.buffer,0,check,after.bytes) ||
            memcmp(check,buffers[i],after.bytes))return JC_LAYOUT_CHANGED;
    }
    lease->stage=4;
    for(unsigned t=0;t<4;++t)if(!ops->setters.texture(ctx,materials[2],jc_appearance_names[textures[t]],pairs[t][0]))return JC_LAYOUT_CHANGED;
    for(unsigned n=0;n<3;++n) {
        unsigned i=numeric[n];
        int ok=i==0?ops->setters.vector(ctx,materials[2],jc_appearance_names[i],plan.entries[i].value):
            ops->setters.scalar(ctx,materials[2],jc_appearance_names[i],plan.entries[i].value[0]);
        if(!ok)return JC_LAYOUT_CHANGED;
    }
    lease->stage=5;
    for(unsigned i=0;i<3;++i) {
        code=jc_appearance_bindings_read(read,ctx,materials[i],&plan,&after);if(code)return code;
        if(!jc_appearance_storage_same(&after,before+i) ||
            !jc_layout_field(read,ctx,after.buffer,0,check,after.bytes))return JC_LAYOUT_CHANGED;
        if(i<2) {if(memcmp(check,buffers[i],after.bytes))return JC_LAYOUT_CHANGED;continue;}
        unsigned char expected[4096];memcpy(expected,buffers[i],after.bytes);
        for(unsigned t=0;t<4;++t) {
            unsigned param=textures[t];
            if(after.textures[param].owner!=plan.entries[param].owner ||
                after.textures[param].handle!=plan.entries[param].handle ||
                after.textures[param].type!=plan.entries[param].binding_type)return JC_LAYOUT_CHANGED;
            memcpy(expected+after.offsets[param],check+after.offsets[param],24);
        }
        for(unsigned n=0;n<3;++n) {
            unsigned param=numeric[n];memcpy(expected+after.offsets[param],plan.entries[param].value,jc_appearance_bytes(param));
        }
        if(memcmp(expected,check,after.bytes))return JC_LAYOUT_CHANGED;
    }
    lease->applied=1;lease->stage=6;return 0;
}
static inline int jc_body_appearance_release(void *ctx,const struct jc_body_appearance_ops *ops,
        uint64_t material,int destroyed,struct jc_body_appearance_lease *lease) {
    if(!lease || !ops || !ops->drop)return JC_LAYOUT_TYPE;
    if(!lease->material)return 0;
    if(!destroyed || material!=lease->material || lease->held>4)return JC_LAYOUT_CHANGED;
    while(lease->held)ops->drop(ctx,lease->pairs[--lease->held]);
    memset(lease,0,sizeof(*lease));return 0;
}
#endif
