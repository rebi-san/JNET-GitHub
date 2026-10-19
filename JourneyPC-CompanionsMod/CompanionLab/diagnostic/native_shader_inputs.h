#ifndef JC_NATIVE_SHADER_INPUTS_H
#define JC_NATIVE_SHADER_INPUTS_H
#include "native_material_ready.h"
struct jc_shader_inputs {
    uint64_t buffer,payload,instance,shader;
    uint32_t bytes,found,finite,zero;
    int32_t dissolve_milli;
    uint16_t offsets[6],sizes[6];
    float values[6][16];
};
/* Storage identity for disjointness checks; no torso parameter schema required. */
static inline int jc_shader_storage_read(jc_layout_reader read,void *ctx,uint64_t material,
    struct jc_shader_inputs *out) {
    struct jc_shader_inputs value={0}; uint64_t collection,first,pair[2],check[2],end,payload;
    uint32_t encoded,again; int32_t variant;
    memset(out,0,sizeof(*out));
    int code=jc_material_ready(read,ctx,material,&value.payload); if (code) return code;
    if (!jc_layout_field(read,ctx,value.payload,0x90,&variant,4) || variant<0 || variant>1 ||
        !jc_layout_field(read,ctx,value.payload,0x80+8*(uint64_t)variant,&collection,8) ||
        !jc_layout_field(read,ctx,collection,0,&encoded,4) ||
        !jc_layout_field(read,ctx,collection,8,&first,8)) return JC_LAYOUT_READ;
    unsigned count=encoded&0x7fffffff; if (!count || count>4096) return JC_LAYOUT_RANGE;
    value.instance=first;
    if (count>1 && !jc_layout_field(read,ctx,first,0,&value.instance,8)) return JC_LAYOUT_READ;
    if (!jc_layout_field(read,ctx,value.instance,0,pair,sizeof(pair)) ||
        !jc_layout_field(read,ctx,pair[1],0,&value.bytes,4)) return JC_LAYOUT_READ;
    value.shader=pair[0]; value.buffer=pair[1];
    if (!value.shader || value.bytes<4 || value.bytes>1048576 ||
        !jc_layout_address(value.buffer,value.bytes,&end)) return JC_LAYOUT_RANGE;
    if (jc_material_ready(read,ctx,material,&payload) || payload!=value.payload ||
        !jc_layout_field(read,ctx,collection,0,&again,4) || again!=encoded ||
        !jc_layout_field(read,ctx,collection,8,&check[0],8) || check[0]!=first ||
        (count>1 && (!jc_layout_field(read,ctx,first,0,&check[0],8) || check[0]!=value.instance)) ||
        !jc_layout_field(read,ctx,value.instance,0,check,sizeof(check)) || memcmp(pair,check,sizeof(pair)) ||
        !jc_layout_field(read,ctx,value.buffer,0,&again,4) || again!=value.bytes) return JC_LAYOUT_CHANGED;
    int32_t check_variant; uint64_t check_collection;
    if (!jc_layout_field(read,ctx,value.payload,0x90,&check_variant,4) || check_variant!=variant ||
        !jc_layout_field(read,ctx,value.payload,0x80+8*(uint64_t)variant,&check_collection,8) || check_collection!=collection)
        return JC_LAYOUT_CHANGED;
    *out=value; return 0;
}
static inline int jc_shader_inputs_read(jc_layout_reader read,void *ctx,uint64_t material,
    struct jc_shader_inputs *out) {
    static const char *names[]={"dudeDissolve","dudeCenter","dudeInvTorso","model","modelViewProj","viewprojection"};
    static const unsigned sizes[]={4,16,64,64,64,64};
    struct jc_shader_inputs value={0};
    uint64_t payload,collection,first,instance,shader,again,end; uint32_t encoded,check_size; int32_t variant;
    uint16_t ranges[512][2]; int indices[512]; unsigned declarations=0;
    unsigned char table_headers[32],check_headers[32],entries[256*24],entries_check[256*24];
    unsigned char buffer_snapshot[4096],buffer_check[4096]; uint64_t instance_pair[2];
    char table_names[256][32],names_check[256][32];
    memset(out,0,sizeof(*out));
    int code=jc_material_ready(read,ctx,material,&payload);
    if (code) return code;
    if (!jc_layout_field(read,ctx,payload,0x90,&variant,4)) return JC_LAYOUT_READ;
    if (variant<0 || variant>1) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,ctx,payload,0x80+8*(uint64_t)variant,&collection,8) ||
        !jc_layout_field(read,ctx,collection,0,&encoded,4) ||
        !jc_layout_field(read,ctx,collection,8,&first,8)) return JC_LAYOUT_READ;
    uint32_t count=encoded&0x7fffffff;
    if (!count || count>4096) return JC_LAYOUT_RANGE;
    instance=first;
    if (count>1 && !jc_layout_field(read,ctx,first,0,&instance,8)) return JC_LAYOUT_READ;
    if (!jc_layout_field(read,ctx,instance,0,instance_pair,sizeof(instance_pair))) return JC_LAYOUT_READ;
    shader=instance_pair[0]; value.buffer=instance_pair[1];
    if (!jc_layout_field(read,ctx,value.buffer,0,&value.bytes,4) ||
        !jc_layout_field(read,ctx,shader,0x40,table_headers,sizeof(table_headers))) return JC_LAYOUT_READ;
    if (value.bytes<4 || value.bytes>1048576 || !jc_layout_address(value.buffer,value.bytes,&end)) return JC_LAYOUT_RANGE;
    int batched=value.bytes<=sizeof(buffer_snapshot);
    if (batched && !jc_layout_field(read,ctx,value.buffer,0,buffer_snapshot,value.bytes)) return JC_LAYOUT_READ;
    for (unsigned table=0;table<2;++table) {
        uint32_t count; uint64_t array;
        memcpy(&count,table_headers+table*16,4); count&=0x7fffffff;
        memcpy(&array,table_headers+table*16+8,8);
        if (count>256) return JC_LAYOUT_RANGE;
        if (count && !jc_layout_field(read,ctx,array,0,entries,count*24)) return JC_LAYOUT_READ;
        code=jc_material_names_read(read,ctx,entries,count,table_names); if (code) return code;
        for (unsigned i=0;i<count;++i) {
            const unsigned char *entry=entries+i*24;
            const char *name=table_names[i];
            uint16_t row_offset,row_size;
            memcpy(&row_offset,entry+0x10,2); memcpy(&row_size,entry+0x12,2); row_size&=0x1fff;
            ranges[declarations][0]=row_offset; ranges[declarations][1]=row_size;
            indices[declarations]=-1;
            for (unsigned n=0;n<6;++n) {
                unsigned bit=1u<<n;
                if (memcmp(name,names[n],strlen(names[n])+1)) continue;
                if (value.found&bit) return JC_LAYOUT_TYPE;
                indices[declarations]=(int)n;
                uint16_t offset,size; float data[16]={0},check[16];
                memcpy(&offset,entry+0x10,2); memcpy(&size,entry+0x12,2); size&=0x1fff;
                if (offset<4 || size<sizes[n] || (uint32_t)offset+size>value.bytes) return JC_LAYOUT_RANGE;
                if (batched) memcpy(data,buffer_snapshot+offset,sizes[n]);
                else {
                    if (!jc_layout_field(read,ctx,value.buffer,offset,data,sizes[n])) return JC_LAYOUT_READ;
                    if (!jc_layout_field(read,ctx,value.buffer,offset,check,sizes[n]) || memcmp(data,check,sizes[n]))
                        return JC_LAYOUT_CHANGED;
                }
                int finite=1,zero=1;
                for (unsigned f=0;f<sizes[n]/4;++f) {
                    uint32_t bits; memcpy(&bits,data+f,4);
                    finite&=(bits&0x7f800000u)!=0x7f800000u;
                    zero&=(bits&0x7fffffffu)==0;
                }
                value.found|=bit;
                value.offsets[n]=offset; value.sizes[n]=size;
                memcpy(value.values[n],data,sizes[n]);
                if (finite) value.finite|=bit;
                if (zero) value.zero|=bit;
                if (!n && finite) {
                    if (!(data[0]>=-1000 && data[0]<=1000)) return JC_LAYOUT_RANGE;
                    value.dissolve_milli=(int32_t)((double)data[0]*1000);
                }
            }
            ++declarations;
        }
        if (count && !jc_layout_field(read,ctx,array,0,entries_check,count*24)) return JC_LAYOUT_READ;
        if (memcmp(entries,entries_check,count*24)) return JC_LAYOUT_CHANGED;
        code=jc_material_names_read(read,ctx,entries,count,names_check); if (code) return code;
        if (memcmp(table_names,names_check,count*32)) return JC_LAYOUT_CHANGED;
    }
    for (unsigned n=0;n<6;++n) if (value.found&(1u<<n))
        for (unsigned j=0;j<declarations;++j) if (indices[j]!=(int)n && ranges[j][1] &&
            value.offsets[n]<(unsigned)ranges[j][0]+ranges[j][1] &&
            ranges[j][0]<(unsigned)value.offsets[n]+value.sizes[n]) return JC_LAYOUT_TYPE;
    if (batched && (!jc_layout_field(read,ctx,value.buffer,0,buffer_check,value.bytes) ||
        memcmp(buffer_snapshot,buffer_check,value.bytes))) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,instance,0,instance_pair,sizeof(instance_pair)) ||
        instance_pair[0]!=shader || instance_pair[1]!=value.buffer) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,shader,0x40,check_headers,32) || memcmp(table_headers,check_headers,32) ||
        !jc_layout_field(read,ctx,value.buffer,0,&check_size,4) || check_size!=value.bytes ||
        !jc_layout_field(read,ctx,payload,0x90,&check_size,4) || check_size!=(uint32_t)variant ||
        !jc_layout_field(read,ctx,payload,0x80+8*(uint64_t)variant,&again,8) || again!=collection ||
        !jc_layout_field(read,ctx,collection,0,&check_size,4) || check_size!=encoded ||
        !jc_layout_field(read,ctx,collection,8,&again,8) || again!=first ||
        (count>1 && (!jc_layout_field(read,ctx,first,0,&again,8) || again!=instance))) return JC_LAYOUT_CHANGED;
    code=jc_material_ready(read,ctx,material,&again);
    if (code || again!=payload) return code ? code : JC_LAYOUT_CHANGED;
    value.payload=payload; value.instance=instance; value.shader=shader;
    *out=value; return JC_LAYOUT_OK;
}
struct jc_scalar_observation { float value; unsigned stage,table,row,offset,size; };
/* Read-only native first-match lookup; not a replacement for write/alias guards. */
static inline int jc_material_scalar_observe(jc_layout_reader read,void *ctx,uint64_t material,
        const char *name,struct jc_scalar_observation *out) {
    uint64_t payload,collection,first,instance,pair[2],check_pair[2],check;
    uint32_t encoded,bytes,recount; int32_t variant;
    unsigned char tables[32],again[32],entries[256*24],entries_again[256*24];
    char names[256][32],names_again[256][32];
    memset(out,0,sizeof(*out)); out->stage=1;
    if (!name || strlen(name)>=32) return JC_LAYOUT_RANGE;
    int code=jc_material_ready(read,ctx,material,&payload); if (code) return code;
    out->stage=2;
    if (!jc_layout_field(read,ctx,payload,0x90,&variant,4)) return JC_LAYOUT_READ;
    if (variant<0 || variant>1) return JC_LAYOUT_RANGE;
    if (!jc_layout_field(read,ctx,payload,0x80+8*(unsigned)variant,&collection,8) ||
        !jc_layout_field(read,ctx,collection,0,&encoded,4) ||
        !jc_layout_field(read,ctx,collection,8,&first,8)) return JC_LAYOUT_READ;
    unsigned count=encoded&0x7fffffff;
    if (!count || count>4096) return JC_LAYOUT_RANGE;
    instance=first;
    if (count>1 && !jc_layout_field(read,ctx,first,0,&instance,8)) return JC_LAYOUT_READ;
    if (!jc_layout_field(read,ctx,instance,0,pair,sizeof(pair)) ||
        !jc_layout_field(read,ctx,pair[1],0,&bytes,4) ||
        !jc_layout_field(read,ctx,pair[0],0x40,tables,sizeof(tables))) return JC_LAYOUT_READ;
    if (bytes<4 || bytes>1048576) return JC_LAYOUT_RANGE;
    int found=0;
    for (unsigned table=0;table<2 && !found;++table) {
        uint32_t size; uint64_t array;
        memcpy(&size,tables+16*table,4); size&=0x7fffffff;
        memcpy(&array,tables+16*table+8,8); out->stage=3;
        if (size>256) return JC_LAYOUT_RANGE;
        if (size && !jc_layout_field(read,ctx,array,0,entries,size*24)) return JC_LAYOUT_READ;
        code=jc_material_names_read(read,ctx,entries,size,names); if (code) return code;
        out->stage=4;
        for (unsigned i=0;i<size;++i) if (!memcmp(names[i],name,strlen(name)+1)) {
            uint16_t offset,extent;
            memcpy(&offset,entries+i*24+16,2); memcpy(&extent,entries+i*24+18,2); extent&=0x1fff;
            if (offset<4 || extent<4 || (unsigned)offset+extent>bytes) return JC_LAYOUT_RANGE;
            out->offset=offset; out->size=extent; out->table=table; out->row=i;
            found=1; break;
        }
        if (size && !jc_layout_field(read,ctx,array,0,entries_again,size*24)) return JC_LAYOUT_READ;
        if (memcmp(entries,entries_again,size*24)) return JC_LAYOUT_CHANGED;
        code=jc_material_names_read(read,ctx,entries,size,names_again); if (code) return code;
        if (memcmp(names,names_again,size*32)) return JC_LAYOUT_CHANGED;
    }
    if (!found) return JC_LAYOUT_TYPE;
    out->stage=5;
    float value,value_again;
    if (!jc_layout_field(read,ctx,pair[1],out->offset,&value,4) ||
        !jc_layout_field(read,ctx,pair[1],out->offset,&value_again,4)) return JC_LAYOUT_READ;
    if (memcmp(&value,&value_again,4)) return JC_LAYOUT_CHANGED;
    uint32_t bits; memcpy(&bits,&value,4);
    if ((bits&0x7f800000u)==0x7f800000u) return JC_LAYOUT_RANGE;
    out->stage=6;
    if (!jc_layout_field(read,ctx,instance,0,check_pair,sizeof(check_pair)) ||
        !jc_layout_field(read,ctx,pair[0],0x40,again,sizeof(again)) ||
        !jc_layout_field(read,ctx,pair[1],0,&recount,4)) return JC_LAYOUT_READ;
    if (memcmp(pair,check_pair,sizeof(pair)) || memcmp(tables,again,sizeof(tables)) || recount!=bytes) return JC_LAYOUT_CHANGED;
    int32_t check_variant; uint32_t check_count;
    if (!jc_layout_field(read,ctx,payload,0x90,&check_variant,4) ||
        !jc_layout_field(read,ctx,payload,0x80+8*(unsigned)variant,&check,8)) return JC_LAYOUT_READ;
    if (check_variant!=variant || check!=collection) return JC_LAYOUT_CHANGED;
    if (!jc_layout_field(read,ctx,collection,0,&check_count,4) ||
        !jc_layout_field(read,ctx,collection,8,&check,8)) return JC_LAYOUT_READ;
    if (check_count!=encoded || check!=first) return JC_LAYOUT_CHANGED;
    if (count>1 && (!jc_layout_field(read,ctx,first,0,&check,8) || check!=instance)) return JC_LAYOUT_CHANGED;
    code=jc_material_ready(read,ctx,material,&check); if (code) return code;
    if (check!=payload) return JC_LAYOUT_CHANGED;
    out->value=value; out->stage=0; return JC_LAYOUT_OK;
}
#endif
