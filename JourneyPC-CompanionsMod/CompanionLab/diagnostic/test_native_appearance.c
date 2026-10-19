#include "native_appearance.h"
#include "native_trial_shader.h"
#include "native_read_batch.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x40000];
static const uint64_t base=0x100000;
static uint64_t fail_address,change_address;
static unsigned hits,reads,change_hit;
static void put(unsigned off,uint64_t value) { memcpy(memory+off,&value,8); }
static void put32(unsigned off,uint32_t value) { memcpy(memory+off,&value,4); }
static void put16(unsigned off,uint16_t value) { memcpy(memory+off,&value,2); }
static int read_memory(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx; ++reads;
    if ((fail_address>=address && fail_address-address<bytes) || address<base || address-base>sizeof(memory) ||
        bytes>sizeof(memory)-(address-base)) return 0;
    if (change_address>=address && change_address-address<bytes && hits++==change_hit) memory[change_address-base]^=1;
    memcpy(out,memory+(address-base),bytes); return 1;
}
static void fixture(void) {
    memset(memory,0,sizeof(memory)); fail_address=change_address=0; hits=reads=0; change_hit=1;
    put(0x30,base+0x100); put(0x110,base+0x200); put(0x218,base+0x300);
    put(0x380,base+0x400); put32(0x400,1); put(0x408,base+0x500);
    put(0x500,base+0x600); put(0x508,base+0x1000); put32(0x1000,512);
    put32(0x640,13); put(0x648,base+0x700); put32(0x3f20,13);
    for (unsigned i=0;i<13;++i) {
        unsigned row=0x3720+i*128,entry=0x700+i*24,name=0x900+i*32,offset=4+i*32;
        unsigned resource=0x5000+i*0x400,texture=resource+0x100,handle=resource+0x300;
        strcpy((char *)memory+row+0x20,jc_appearance_names[i]); put32(row+0x78,jc_appearance_kind(i));
        strcpy((char *)memory+name,jc_appearance_names[i]); put(entry+8,(base+name)|(i&1));
        put16(entry+16,(uint16_t)offset); put16(entry+18,(uint16_t)(jc_appearance_bytes(i)|0x8000));
        if (jc_appearance_kind(i)==2) {
            put(row+0x48,base+resource); put(resource+0x10,base+resource+0x20);
            put(resource+0x20+0xc8,base+texture); put(texture+0x70,base+handle);
            unsigned kind=i==12 ? 3 : 1;
            memory[entry+2]=(unsigned char)(0x41+kind);
            put32(texture+0x110,kind); put32(handle+0xc,0); put32(handle+0x30,1);
            put32(0x1000+offset,0x41+kind); put32(0x1000+offset+4,0);
            put(0x1000+offset+8,base+texture+8); put(0x1000+offset+16,base+handle);
        } else {
            float a[4]={1,2,3,4},b[4]={3,4,5,6},t=0.5f,value[4]={2,3,4,5};
            memcpy(memory+row,a,16); memcpy(memory+row+16,b,16); memcpy(memory+row+0x34,&t,4);
            memcpy(memory+0x1000+offset,value,jc_appearance_bytes(i));
        }
    }
}
static unsigned setter_calls,setter_fail,corrupt_original,corrupt_trial;
static int set_value(uint64_t material,const char *name,const void *value,unsigned bytes) {
    ++setter_calls; if (setter_calls==setter_fail) return 0;
    unsigned i=0; while (i<13 && strcmp(name,jc_appearance_names[i])) ++i;
    assert(i<13 && material==base+0x20000);
    memcpy(memory+0x21000+4+i*32,value,bytes);
    if (corrupt_original) memory[0x1008]^=1;
    if (corrupt_trial) memory[0x211ff]^=1;
    return 1;
}
static int set_scalar(void *ctx,uint64_t material,const char *name,float value) {
    (void)ctx; return set_value(material,name,&value,4);
}
static int set_vector(void *ctx,uint64_t material,const char *name,const float value[4]) {
    (void)ctx; return set_value(material,name,value,16);
}
static int set_texture(void *ctx,uint64_t material,const char *name,uint64_t texture) {
    (void)ctx; unsigned char value[24]; uint64_t handle,owner=texture+8; uint32_t kind,resource_index;
    assert(read_memory(NULL,texture+0x70,&handle,8)); assert(read_memory(NULL,texture+0x110,&kind,4));
    assert(read_memory(NULL,handle+0xc,&resource_index,4)); kind+=0x41;
    if (kind==0x42) { uint32_t flags; assert(read_memory(NULL,handle+0x94,&flags,4)); kind|=flags&0x80000000u; }
    memcpy(value,&kind,4); memcpy(value+4,&resource_index,4); memcpy(value+8,&owner,8); memcpy(value+16,&handle,8);
    return set_value(material,name,value,24);
}
static const struct jc_appearance_setters setters={set_scalar,set_vector,set_texture};
static const uint64_t materials[3]={0x100000,0x110000,0x120000};
static void preparation_fixture(void) {
    fixture(); setter_calls=setter_fail=corrupt_original=corrupt_trial=0;
    for (unsigned i=1;i<3;++i) {
        unsigned o=i*0x10000; memcpy(memory+o,memory,0x2000);
        put(o+0x30,base+o+0x100); put(o+0x110,base+o+0x200); put(o+0x218,base+o+0x300);
        put(o+0x380,base+o+0x400); put(o+0x408,base+o+0x500); put(o+0x508,base+o+0x1000);
    }
    memset(memory+0x21004,0,508);
}
static void multiple_instances(unsigned material_offset,unsigned count) {
    unsigned o=material_offset;
    put32(o+0x400,0x80000000u|count); put(o+0x408,base+o+0x2000);
    put(o+0x2000,base+o+0x500);
    for (unsigned i=1;i<count;++i) {
        unsigned instance=o+0x2200+32*i,buffer=o+0x2600+64*i;
        put(o+0x2000+8*i,base+instance); put(instance,base+0x600);
        put(instance+8,base+buffer); put32(buffer,32);
    }
}
static void alternate_source_texture(unsigned index,uint32_t flags) {
    unsigned texture=0x30000+index*0x200,handle=texture+0x120,offset=0x1004+index*32;
    unsigned kind=index==12 ? 3 : 1;
    put(texture+0x70,base+handle); put32(texture+0x110,kind); put32(handle+0xc,1); put32(handle+0x30,2); put(handle+0x38,base+handle+0x40);
    put32(handle+0x94,flags);
    put32(offset,(0x41+kind)|(kind==1 ? flags&0x80000000u : 0)); put32(offset+4,1);
    put(offset+8,base+texture+8); put(offset+16,base+handle);
}
static void plan_rejected(int expected) {
    struct jc_appearance_plan p,zero={0}; memset(&p,0xff,sizeof(p));
    assert(jc_appearance_plan_read(read_memory,NULL,base+0x3000,&p)==expected);
    assert(!memcmp(&p,&zero,sizeof(p))); assert(reads<300);
}
static void bindings_rejected(const struct jc_appearance_plan *p,int expected) {
    struct jc_appearance_bindings b,zero={0}; memset(&b,0xff,sizeof(b));
    int code=jc_appearance_bindings_read(read_memory,NULL,base,p,&b);
    if (code!=expected) fprintf(stderr,"bindings expected %d, got %d; reads %u; changed offset %llx\n",
        expected,code,reads,(unsigned long long)(change_address ? change_address-base : 0));
    assert(code==expected);
    assert(!memcmp(&b,&zero,sizeof(b))); assert(reads<1200);
}
static unsigned shader_writes,shader_write_fail,shader_corrupt;
static int shader_write(void *ctx,uint64_t address,const void *value,size_t bytes) {
    (void)ctx; ++shader_writes;
    assert((address==base+0x21000+608 && bytes==16) || (address==base+0x21000+640 && bytes==64));
    if (shader_writes==shader_write_fail) return 0;
    memcpy(memory+(address-base),value,bytes);
    if (shader_corrupt && shader_writes==1) memory[0x1008]^=1;
    return 1;
}
static void shader_fixture(void) {
    preparation_fixture(); shader_writes=shader_write_fail=shader_corrupt=0;
    const char *names[]={"dudeDissolve","dudeCenter","dudeInvTorso"};
    unsigned offsets[]={600,608,640},sizes[]={4,16,64};
    put32(0x640,16);
    for (unsigned i=0;i<3;++i) {
        put32(0x1000+0x10000*i,1024);
        unsigned entry=0x700+(13+i)*24,name=0xe00+i*32;
        strcpy((char *)memory+name,names[i]); put(entry+8,base+name);
        put16(entry+16,(uint16_t)offsets[i]); put16(entry+18,(uint16_t)sizes[i]);
    }
    put(0x340a8,base+0x34120); put(0x34128,base+0x34200); memory[0x34139]=1;
    put(0x34200,base+0x34120); put(0x34210,base+0x34120);
    strcpy((char *)memory+0x34220,"spineB"); put(0x34230,6); put(0x34238,15); put(0x34240,base+0x34300);
    float child[16]={1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1};
    float root[16]={0,0,-1,0,0,1,0,0,1,0,0,0,10,20,30,1};
    put(0x34308,base+0x34400); memcpy(memory+0x34320,child,64); memcpy(memory+0x34420,root,64);
}
static uint64_t spine_tree(unsigned first,unsigned count) {
    if (!count) return base+0x34120;
    unsigned middle=first+count/2,offset=0x35000+middle*0x80;
    put(offset,spine_tree(first,count/2));
    put(offset+0x10,spine_tree(middle+1,count-count/2-1));
    char name[16];
    if (middle==31) strcpy(name,"spineB");
    else snprintf(name,sizeof(name),"%c%03u",middle<31 ? 'a' : 'z',middle);
    strcpy((char *)memory+offset+0x20,name); put(offset+0x30,strlen(name)); put(offset+0x38,15);
    if (middle==31) put(offset+0x40,base+0x34300);
    return base+offset;
}
static void spine_lookup_tests(void) {
    float full[16],fast[16];
    shader_fixture(); put(0x34128,spine_tree(0,63)); reads=0;
    assert(!jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,full));
    unsigned full_reads=reads; reads=0;
    assert(!jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast));
    unsigned fast_reads=reads;
    assert(!memcmp(full,fast,64) && fast_reads*3<full_reads);
    printf("Spine lookup: identical transform, %u reads versus %u for full 63-node scan.\n",fast_reads,full_reads);
    shader_fixture(); put(0x34200,base+0x34200);
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==5);
    shader_fixture(); put(0x34308,base+0x34300);
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==5);
    shader_fixture(); strcpy((char *)memory+0x34220,"spineA");
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==2);
    shader_fixture(); put(0x34238,4097);
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==4);
    shader_fixture(); put(0x34220,base+0x34500); strcpy((char *)memory+0x34500,"spineB"); put(0x34238,16);
    assert(!jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast));
    change_address=base+0x34502;
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==3);
    shader_fixture(); fail_address=base+0x34222;
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==1);
    shader_fixture(); put(0x34308,0);
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==2);
    shader_fixture(); put32(0x34320,0x7fc00000);
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==4);
    shader_fixture(); change_address=base+0x34300;
    assert(jc_trial_spine_lookup(read_memory,NULL,base+0x34000,base+0x34400,fast)==3);
}
static void shader_tests(void) {
    spine_lookup_tests();
    float world[16],inverse[16]; struct jc_trial_shader_result result;
    shader_fixture();
    assert(!jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world));
    assert(world[12]==13 && world[13]==22 && world[14]==29 && world[0]==0 && world[2]==-1);
    assert(!jc_trial_inverse(world,inverse));
    assert(!jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result));
    assert(shader_writes==2 && result.stage==3 && result.originals_unchanged==1 && result.trial_only==1);
    float center[4]; memcpy(center,memory+0x21000+608,16);
    assert(center[0]==13 && center[1]==22 && center[2]==29 && center[3]==1);
    assert(!memcmp(memory+0x21000+640,inverse,64));
    struct jc_shader_inputs snapshot;
    assert(!jc_shader_inputs_read(read_memory,NULL,materials[2],&snapshot));
    assert(snapshot.found==7 && snapshot.finite==7 && snapshot.zero==1);
    float moved[16]; memcpy(moved,world,64); moved[12]+=5; moved[14]-=2;
    assert(!jc_trial_shader_update(read_memory,shader_write,NULL,base+0x3000,materials,moved,world,&result));
    assert(result.stage==3 && result.originals_unchanged==1 && result.trial_only==1);
    unsigned writes_before=shader_writes;
    assert(jc_trial_shader_update(read_memory,shader_write,NULL,base+0x3000,materials,world,world,&result)==3 && shader_writes==writes_before);
    for (unsigned i=0;i<120;++i) {
        float next[16]; memcpy(next,moved,64); next[12]+=.1f;
        assert(!jc_trial_shader_update(read_memory,shader_write,NULL,base+0x3000,materials,next,moved,&result));
        memcpy(moved,next,64);
    }
    shader_fixture(); put(0x34308,base+0x34300);
    assert(jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world)==5);
    shader_fixture(); put(0x34200,base+0x34200);
    assert(jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world)==5);
    shader_fixture(); put32(0x34320,0x7fc00000);
    assert(jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world)==4);
    shader_fixture(); put(0x34308,0);
    assert(jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world)==2);
    shader_fixture(); change_address=base+0x34300;
    assert(jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world)==3);
    shader_fixture(); assert(!jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world));
    world[0]=world[1]=world[2]=0;
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==4 && !shader_writes);
    shader_fixture(); assert(!jc_trial_spine(read_memory,NULL,base+0x34000,base+0x34400,world));
    put16(0x700+14*24+18,32);
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==4 && !shader_writes);
    shader_fixture(); put16(0x700+14*24+16,636);
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==2 && !shader_writes);
    shader_fixture(); put(0x700+15*24+8,base+0xe20);
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==2 && !shader_writes);
    shader_fixture(); put(0x20508,base+0x1000);
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==6 && !shader_writes);
    shader_fixture(); shader_write_fail=2;
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==7 && result.stage==1);
    shader_fixture(); shader_corrupt=1;
    assert(jc_trial_shader_write(read_memory,shader_write,NULL,base+0x3000,materials,world,&result)==8);
}
static int limited_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    unsigned *remaining=ctx;
    if (!*remaining) return 0;
    --*remaining;
    return read_memory(NULL,address,out,bytes);
}
static void material_batch_tests(void) {
    unsigned char entries[256*24]={0}; char names[256][32];
    fixture();
    assert(!jc_material_names_read(read_memory,NULL,entries,0,names) && !reads);
    assert(jc_material_names_read(read_memory,NULL,entries,257,names)==JC_LAYOUT_RANGE && !reads);
    for (unsigned i=0;i<256;++i) {
        unsigned slot=(i*73)%256;
        uint64_t address=(base+0x30000+slot*32)|(i&1);
        memcpy(entries+i*24+8,&address,8);
        memset(memory+0x30000+slot*32,(int)(slot%251),32);
    }
    assert(!jc_material_names_read(read_memory,NULL,entries,256,names) && reads==1);
    for (unsigned i=0;i<256;++i)
        assert(!memcmp(names[i],memory+0x30000+((i*73)%256)*32,32));
    fail_address=base+0x30000+255*32+17;
    assert(jc_material_names_read(read_memory,NULL,entries,256,names)==JC_LAYOUT_READ);
    fixture();
    for (unsigned i=0;i<256;++i) {
        uint64_t address=base+0x30000+i*64;
        memcpy(entries+i*24+8,&address,8);
        memset(memory+0x30000+i*64,(int)(i%251),32);
    }
    fail_address=base+0x30020;
    assert(!jc_material_names_read(read_memory,NULL,entries,256,names) && reads==256);
    for (unsigned i=0;i<256;++i) assert(!memcmp(names[i],memory+0x30000+i*64,32));
    unsigned remaining=1;
    assert(jc_material_names_read(limited_read,&remaining,entries,256,names)==JC_LAYOUT_READ && !remaining);
    fixture();
    for (unsigned i=0;i<256;++i) {
        uint64_t address=(base+0x30000)|(i&1);
        memcpy(entries+i*24+8,&address,8);
    }
    assert(!jc_material_names_read(read_memory,NULL,entries,256,names) && reads==1);
    uint64_t invalid=UINT64_C(0x7fffffffffe0);
    memcpy(entries+8,&invalid,8);
    assert(jc_material_names_read(read_memory,NULL,entries,256,names)==JC_LAYOUT_READ && reads==1);
    fixture();
    for (unsigned i=0;i<64;++i) memory[0x30000+i]=(unsigned char)i;
    for (unsigned i=0;i<3;++i) {
        uint64_t address=base+0x30000+(2-i)*16;
        memcpy(entries+i*24+8,&address,8);
    }
    assert(!jc_material_names_read(read_memory,NULL,entries,3,names) && reads==1);
    for (unsigned i=0;i<3;++i) assert(!memcmp(names[i],memory+0x30000+(2-i)*16,32));
    struct jc_appearance_plan plan;
    struct jc_appearance_bindings bindings,zero={0};
    fixture(); assert(!jc_appearance_plan_read(read_memory,NULL,base+0x3000,&plan)); reads=0;
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&plan,&bindings));
    assert(bindings.found==8191 && bindings.matching==8191 && reads<60);
    fixture(); change_address=base+0x900+3*32+2;
    bindings_rejected(&plan,JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x700+5*24+1;
    bindings_rejected(&plan,JC_LAYOUT_CHANGED);
    fixture(); fail_address=base+0x700+5*24+1;
    bindings_rejected(&plan,JC_LAYOUT_READ);
    fixture(); remaining=14; memset(&bindings,0xff,sizeof(bindings));
    assert(jc_appearance_bindings_read(limited_read,&remaining,base,&plan,&bindings)==JC_LAYOUT_READ);
    assert(!remaining && !memcmp(&bindings,&zero,sizeof(zero)));
    fixture(); put32(0x640,14); put(0x700+13*24+8,base+0xb00);
    strcpy((char *)memory+0xb00,"unrelated"); change_address=base+0x700+13*24;
    bindings_rejected(&plan,JC_LAYOUT_CHANGED);
    struct jc_shader_inputs inputs,empty={0};
    shader_fixture(); assert(!jc_shader_inputs_read(read_memory,NULL,base,&inputs));
    assert((inputs.found&7)==7);
    shader_fixture(); change_address=base+0xe00+32+2;
    assert(jc_shader_inputs_read(read_memory,NULL,base,&inputs)==JC_LAYOUT_CHANGED);
    assert(!memcmp(&inputs,&empty,sizeof(empty)));
    shader_fixture(); change_address=base+0x700+14*24+1;
    assert(jc_shader_inputs_read(read_memory,NULL,base,&inputs)==JC_LAYOUT_CHANGED);
    shader_fixture(); fail_address=base+0xe00+32+2;
    assert(jc_shader_inputs_read(read_memory,NULL,base,&inputs)==JC_LAYOUT_READ);
    puts("Material batches: dense, sparse, aliased and tagged names; bounds, fresh rereads, partial failures and read limits checked.");
}
static void texture_batch_tests(void) {
    struct jc_texture_metadata metadata,empty={0};
    const unsigned texture=0x6100,handle=0x6300;
    for (unsigned kind=1;kind<=3;++kind) {
        fixture(); put32(texture+0x110,kind);
        unsigned list=kind==2 ? 0x38 : 0x30;
        put32(handle+list,2); put(handle+list+8,base+0x38000);
        assert(!jc_texture_metadata_read(read_memory,NULL,base+texture,&metadata) && reads==2);
        assert(metadata.handle==base+handle && metadata.kind==kind && metadata.count==2 && metadata.table==base+0x38000);
        fail_address=base+handle+list+9;
        assert(jc_texture_metadata_read(read_memory,NULL,base+texture,&metadata)==JC_LAYOUT_READ);
        assert(!memcmp(&metadata,&empty,sizeof(empty)));
    }
    fixture(); fail_address=base+texture+0x90;
    assert(jc_texture_metadata_read(read_memory,NULL,base+texture,&metadata)==JC_LAYOUT_READ);
    assert(!memcmp(&metadata,&empty,sizeof(empty)));
    struct jc_appearance_plan plan;
    fixture(); assert(!jc_appearance_plan_read(read_memory,NULL,base+0x3000,&plan) && reads<=49);
    const unsigned fields[]={texture+0x70,texture+0x110,handle+0xc,handle+0x94};
    for (unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);++i) {
        fixture(); change_address=base+fields[i];
        struct jc_appearance_plan zero={0}; memset(&plan,0xff,sizeof(plan));
        assert(jc_appearance_plan_read(read_memory,NULL,base+0x3000,&plan)!=JC_LAYOUT_OK);
        assert(!memcmp(&plan,&zero,sizeof(plan)));
    }
    preparation_fixture(); struct jc_appearance_preparation prep; struct jc_appearance_borrow borrow;
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    reads=0; assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow) && reads<=170);
    printf("Full borrow check: %u reader calls (0.61: 204); all three texture types and fresh identity rereads checked.\n",reads);
}
static void binding_snapshot_tests(void) {
    struct jc_appearance_plan plan;
    struct jc_appearance_bindings bindings,zero={0};
    struct jc_appearance_detail detail;
    fixture(); assert(!jc_appearance_plan_read(read_memory,NULL,base+0x3000,&plan));
    reads=0;
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&plan,&bindings));
    unsigned single_reads=reads;
    for (unsigned count=2;count<=16;++count) {
        fixture(); multiple_instances(0,count);
        for (unsigned i=1;i<count;++i) put(0x2000+i*8,base+0x500);
        assert(!jc_appearance_bindings_read_detail(read_memory,NULL,base,&plan,&bindings,&detail));
        assert(detail.unique_instances==1 && bindings.instance_count==count && reads==single_reads+2);
        for (unsigned i=0;i<count;++i) assert(bindings.instances[i].instance==base+0x500);
    }
    const unsigned fields[]={0x30,0x110,0x218,0x380,0x390,0x400,0x408,0x500,0x508,0x1000};
    for (unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);++i) {
        fixture(); change_address=base+fields[i];
        bindings_rejected(&plan,JC_LAYOUT_CHANGED);
    }
    fixture(); put32(0x390,1); put(0x388,base+0x400);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&plan,&bindings));
    fixture(); change_address=base+0x388;
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&plan,&bindings));
    fixture(); change_address=base+0x404;
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&plan,&bindings));
    fixture(); multiple_instances(0,3); fail_address=base+0x2017;
    bindings_rejected(&plan,JC_LAYOUT_READ);
    fixture(); fail_address=base+0x38a;
    bindings_rejected(&plan,JC_LAYOUT_READ);
    for (unsigned limit=0;limit<single_reads;++limit) {
        fixture(); unsigned remaining=limit; memset(&bindings,0xff,sizeof(bindings));
        assert(jc_appearance_bindings_read(limited_read,&remaining,base,&plan,&bindings)!=JC_LAYOUT_OK);
        assert(!remaining && !memcmp(&bindings,&zero,sizeof(zero)));
    }
    printf("Material snapshots: %u reads single / %u repeated-list; 16 aliases, both variants, changed owners, interior faults and every truncated read budget checked.\n",single_reads,single_reads+2);
}
static uint64_t logical_change;
static unsigned logical_hits;
static int batched_read(void *ctx,uint64_t address,void *out,size_t size) {
    if (logical_change>=address && logical_change-address<size && ++logical_hits==2)
        memory[logical_change-base]^=1;
    return jc_read_batch_get(ctx,read_memory,NULL,address,out,size);
}
static int batched_shader_write(void *ctx,uint64_t address,const void *data,size_t size) {
    jc_read_batch_reset(ctx); return shader_write(NULL,address,data,size);
}
static void read_batch_tests(void) {
    struct jc_read_batch batch={0}; struct jc_appearance_preparation prep; struct jc_appearance_borrow borrow;
    preparation_fixture();
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    reads=0; assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow)); unsigned original=reads;
    reads=0; assert(!jc_appearance_borrow_check(batched_read,&batch,&borrow)); unsigned reduced=reads;
    assert(reduced<original);
    const unsigned mutations[]={0x3f20,0x6100+0x70,0x630c,0x30,0x500,0x640,0x700,0x108c,0x20030,0x20500};
    for (unsigned i=0;i<sizeof(mutations)/sizeof(mutations[0]);++i) {
        jc_read_batch_reset(&batch); logical_change=base+mutations[i];logical_hits=0;
        assert(jc_appearance_borrow_check(batched_read,&batch,&borrow)!=0);
        assert(logical_hits>=2);memory[mutations[i]]^=1;
    }
    logical_change=0;
    jc_read_batch_reset(&batch); fail_address=base+0x21000+132;
    assert(jc_appearance_borrow_check(batched_read,&batch,&borrow)==JC_LAYOUT_READ);
    shader_fixture();jc_read_batch_reset(&batch);
    float world[16];struct jc_trial_shader_result result;
    assert(!jc_trial_spine(batched_read,&batch,base+0x34000,base+0x34400,world));
    assert(!jc_trial_shader_write(batched_read,batched_shader_write,&batch,base+0x3000,materials,world,&result));
    assert(result.stage==3 && result.originals_unchanged==1 && result.trial_only==1);
    float moved[16];memcpy(moved,world,64);moved[12]+=5;
    jc_read_batch_reset(&batch);
    assert(!jc_trial_shader_update(batched_read,batched_shader_write,&batch,base+0x3000,materials,moved,world,&result));
    printf("Inspection batching: full borrow %u -> %u underlying reads; ten mid-check mutations, live texture fault, shader write invalidation and isolated results checked.\n",original,reduced);
}
static unsigned body_holds,body_drops,body_hold_fail;
static int body_hold(void *ctx,const uint64_t pair[2]) {
    (void)ctx;uint32_t refs=0;++body_holds;
    if(body_hold_fail==body_holds)return 0;
    assert(read_memory(NULL,pair[1],&refs,4) && refs==1);
    ++refs;memcpy(memory+(pair[1]-base),&refs,4);return 1;
}
static void body_drop(void *ctx,const uint64_t pair[2]) {
    (void)ctx;uint32_t refs=0;++body_drops;
    assert(read_memory(NULL,pair[1],&refs,4) && refs==2);
    --refs;memcpy(memory+(pair[1]-base),&refs,4);
}
static const struct jc_body_appearance_ops body_ops={{set_scalar,set_vector,set_texture},body_hold,body_drop};
static void body_fixture(void) {
    preparation_fixture();body_holds=body_drops=body_hold_fail=0;
    const unsigned indices[4]={4,5,6,8};
    for(unsigned t=0;t<4;++t) {
        unsigned resource=0x5000+indices[t]*0x400;
        put(resource+0xf0,base+0x35000+4*t);put32(0x35000+4*t,1);
    }
}
static unsigned outfit_holds,outfit_drops,outfit_sets,outfit_missing;
static uint64_t outfit_bound[2];
static int outfit_lookup(void *ctx,const char *name,uint64_t pair[2]) {
    (void)ctx;const char *names[]={"Ninja","NinjaGlow","NinjaWhite","NinjaWhiteGlow"};
    for(unsigned i=0;i<4;++i)if(!strcmp(name,names[i])) {
        if(outfit_missing==i+1)return 0;
        pair[0]=100+i;pair[1]=200+i;return 1;
    }
    return 0;
}
static int outfit_hold(void *ctx,const uint64_t pair[2]) {
    (void)ctx;assert(pair[1]==pair[0]+100);++outfit_holds;return 1;
}
static void outfit_drop(void *ctx,const uint64_t pair[2]) {
    (void)ctx;assert(pair[1]==pair[0]+100);++outfit_drops;
}
static int outfit_texture(void *ctx,uint64_t material,const char *name,uint64_t texture) {
    (void)ctx;assert(material==1234);++outfit_sets;
    assert(!strcmp(name,"textureAtlasA") || !strcmp(name,"textureAtlasB"));
    outfit_bound[!strcmp(name,"textureAtlasB")]=texture;return 1;
}
static void body_outfit_tests(void) {
    const struct jc_body_appearance_ops ops={{NULL,NULL,outfit_texture},outfit_hold,outfit_drop};
    struct jc_body_outfit_lease lease={0};
    for(unsigned outfit=0;outfit<7;++outfit) {
        assert(!jc_body_outfit_apply(NULL,&ops,outfit_lookup,1234,outfit,&lease));
        assert(outfit_bound[0]==(outfit>3?102u:100u) && outfit_bound[1]==outfit_bound[0]+1);
    }
    assert(outfit_holds==4 && outfit_sets==4 && !outfit_drops);
    assert(!jc_body_outfit_apply(NULL,&ops,outfit_lookup,1234,0,&lease));
    assert(outfit_holds==4 && outfit_sets==6 && outfit_bound[0]==100);
    assert(jc_body_outfit_release(NULL,&ops,0,&lease) && !outfit_drops);
    assert(!jc_body_outfit_release(NULL,&ops,1,&lease) && outfit_drops==4);
    outfit_missing=2;
    assert(jc_body_outfit_apply(NULL,&ops,outfit_lookup,1234,0,&lease) && !lease.selected);
    assert(outfit_holds==5 && outfit_sets==6);
    assert(!jc_body_outfit_release(NULL,&ops,1,&lease) && outfit_drops==5);
    outfit_missing=0;
    assert(jc_body_outfit_apply(NULL,&ops,outfit_lookup,1234,7,&lease));
    puts("Body outfit: all seven tiers select independent red/white base and glow textures; bounded leases survive color changes, missing resources fail closed, release requires native destruction.");
}
static void body_tests(void) {
    body_outfit_tests();
    struct jc_body_appearance_lease lease={0};
    body_fixture();
    uint64_t definition=0;uint32_t outfits[2];
    put(0x38000+0xb0+0x2238,base+0x3000);
    for(unsigned local=0;local<7;++local)for(unsigned retained=0;retained<7;++retained) {
        put32(0x38000+0xb0+0x70,local);put32(0x37000+0x70,retained);
        assert(!jc_body_appearance_source(read_memory,NULL,base+0x38000,base+0x37000,&definition,outfits));
        assert(definition==base+0x3000 && outfits[0]==local && outfits[1]==retained);
    }
    put32(0x37000+0x70,7);
    assert(jc_body_appearance_source(read_memory,NULL,base+0x38000,base+0x37000,&definition,outfits)==JC_LAYOUT_RANGE);
    put32(0x37000+0x70,0);fail_address=base+0x38000+0xb0+0x2238;
    assert(jc_body_appearance_source(read_memory,NULL,base+0x38000,base+0x37000,&definition,outfits)==JC_LAYOUT_READ);
    fail_address=0;
    puts("Body source: 49 local/retained outfit combinations accepted; invalid outfit and missing definition rejected.");
    assert(!jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease));
    assert(lease.applied && lease.held==4 && body_holds==4 && setter_calls==7);
    assert(!memcmp(memory+0x21004+12*32,(unsigned char[24]){0},24));
    assert(!memcmp(memory+0x21004+2*32,(unsigned char[4]){0},4));
    assert(jc_body_appearance_release(NULL,&body_ops,materials[2],0,&lease)==JC_LAYOUT_CHANGED && !body_drops);
    assert(jc_body_appearance_release(NULL,&body_ops,materials[0],1,&lease)==JC_LAYOUT_CHANGED && !body_drops);
    assert(!jc_body_appearance_release(NULL,&body_ops,materials[2],1,&lease) && body_drops==4 && !lease.material);
    assert(!jc_body_appearance_release(NULL,&body_ops,materials[2],1,&lease) && body_drops==4);
    body_fixture();body_hold_fail=3;
    assert(jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease) && !setter_calls && lease.held==2);
    assert(!jc_body_appearance_release(NULL,&body_ops,materials[2],1,&lease) && body_drops==2);
    body_fixture();setter_fail=3;
    assert(jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease) && lease.held==4 && !lease.applied);
    assert(!jc_body_appearance_release(NULL,&body_ops,materials[2],1,&lease) && body_drops==4);
    body_fixture();corrupt_original=1;
    assert(jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease)==JC_LAYOUT_CHANGED && !lease.applied);
    assert(!jc_body_appearance_release(NULL,&body_ops,materials[2],1,&lease));
    body_fixture();corrupt_trial=1;
    assert(jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease)==JC_LAYOUT_CHANGED && !lease.applied);
    assert(!jc_body_appearance_release(NULL,&body_ops,materials[2],1,&lease));
    body_fixture();alternate_source_texture(4,0);
    assert(jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease)==JC_LAYOUT_CHANGED && !body_holds && !setter_calls);
    body_fixture();put(0x20508,base+0x1000);
    assert(jc_body_appearance_prepare(read_memory,NULL,&body_ops,base+0x3000,materials,&lease)==JC_LAYOUT_CHANGED && !body_holds && !setter_calls);
    puts("Body appearance: four retained textures, static material values, original preservation, untouched environment/glow, partial failure retention and destruction-only release passed; native setters/refcounts simulated.");
}
int main(void) {
    body_tests();
    read_batch_tests();
    binding_snapshot_tests();
    texture_batch_tests();
    material_batch_tests();
    struct jc_appearance_plan p; struct jc_appearance_bindings b;
    fixture(); assert(!jc_appearance_plan_read(read_memory,NULL,base+0x3000,&p));
    assert(p.definition==base+0x3000 && p.entries[0].value[0]==2 && p.entries[0].value[1]==3);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b));
    assert(b.found==8191 && b.matching==8191 && !b.zero && b.bytes==512);
    put32(0x1000+4+4*32,0x80000042u);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b) && b.matching==(8191^16));
    memset(memory+0x1000+4+4*32,0,24);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b));
    assert(b.found==8191 && b.matching==(8191^16) && b.zero==16);
    fixture(); put32(0x1000+4+5*32+4,78);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b) && b.matching==8191);
    fixture(); put32(0x640,12);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b) && b.found==4095);
    fixture(); put32(0x3f20,17); plan_rejected(JC_LAYOUT_RANGE);
    fixture(); strcpy((char *)memory+0x3720+128+0x20,"specular"); plan_rejected(JC_LAYOUT_TYPE);
    fixture(); strcpy((char *)memory+0x3720+0x20,"unknown"); plan_rejected(JC_LAYOUT_TYPE);
    fixture(); memset(memory+0x3720+0x20,'X',20); plan_rejected(JC_LAYOUT_RANGE);
    fixture(); put32(0x3720+0x78,2); plan_rejected(JC_LAYOUT_TYPE);
    fixture(); put32(0x3720,0x7fc00000); plan_rejected(JC_LAYOUT_RANGE);
    fixture(); put32(0x3720+0x34,0x7f800000); plan_rejected(JC_LAYOUT_RANGE);
    fixture(); fail_address=base+0x3000+0x720; plan_rejected(JC_LAYOUT_READ);
    fixture(); put(0x3720+4*128+0x48,0); plan_rejected(JC_LAYOUT_READ);
    fixture(); put32(0x5000+4*0x400+0x100+0x110,0); plan_rejected(JC_LAYOUT_TYPE);
    fixture(); change_address=base+0x3f20; plan_rejected(JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x3720; plan_rejected(JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x5000+4*0x400+0x300+0xc; plan_rejected(JC_LAYOUT_CHANGED);
    fixture(); put32(0x400,17);
    bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put32(0x390,2); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put32(0x1000,4097); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put32(0x640,257); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put16(0x710,511); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put16(0x712,4); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put16(0x712,8); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put16(0x700+4*24+18,32); bindings_rejected(&p,JC_LAYOUT_RANGE);
    fixture(); put(0x700+24+8,base+0x900); bindings_rejected(&p,JC_LAYOUT_TYPE);
    fixture(); put16(0x700+24+16,4); bindings_rejected(&p,JC_LAYOUT_TYPE);
    fixture(); change_address=base+0x1000; bindings_rejected(&p,JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x700; bindings_rejected(&p,JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x508; bindings_rejected(&p,JC_LAYOUT_CHANGED);
    fixture(); fail_address=base+0x900; bindings_rejected(&p,JC_LAYOUT_READ);
    struct jc_appearance_preparation prep;
    preparation_fixture(); float style=0.8f; memcpy(memory+0x1000+4+11*32,&style,4);
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(prep.stage==3 && prep.calls==13 && prep.originals_unchanged==1 && prep.trial_after==8191);
    assert(!memcmp(memory+0x1000+4+11*32,memory+0x21000+4+11*32,4));
    preparation_fixture(); put(0x20508,base+0x1000);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==5 && !setter_calls);
    preparation_fixture(); memset(memory+0x1000+4+4*32,0,24);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    preparation_fixture(); put32(0x1000+4+11*32,0x7fc00000);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==4 && !setter_calls);
    preparation_fixture(); setter_fail=5;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==7 && prep.stage==1 && prep.calls==5);
    preparation_fixture(); corrupt_original=1;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==8 && !prep.originals_unchanged);
    preparation_fixture(); corrupt_trial=1;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==8);
    struct jc_appearance_borrow borrow;
    preparation_fixture();
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow));
    float changed_style=0.4f; memcpy(memory+0x1000+4+11*32,&changed_style,4);
    assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow));
    put32(0x5000+4*0x400+0x300+0xc,78);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_RANGE);
    put32(0x5000+4*0x400+0x300+0xc,0);
    put(0x21000+4+4*32+16,base+0x9990);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_CHANGED);
    assert(set_texture(NULL,materials[2],jc_appearance_names[4],base+0x6100));
    assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow));
    put(0x20508,base+0x1000);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_CHANGED);
    assert(jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow)==5);
    assert(!borrow.ready);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_TYPE);
    fixture(); multiple_instances(0,2);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b));
    assert(b.instance_count==2 && b.buffer==base+0x1000 && b.found==8191);
    assert(b.instances[1].buffer==base+0x2640);
    fixture(); multiple_instances(0,16);
    assert(!jc_appearance_bindings_read(read_memory,NULL,base,&p,&b) && b.instance_count==16);
    fixture(); multiple_instances(0,2); change_address=base+0x2000;
    bindings_rejected(&p,JC_LAYOUT_CHANGED);
    fixture(); multiple_instances(0,2); change_address=base+0x2008;
    bindings_rejected(&p,JC_LAYOUT_CHANGED);
    fixture(); multiple_instances(0,2); change_address=base+0x2228;
    bindings_rejected(&p,JC_LAYOUT_CHANGED);
    fixture(); multiple_instances(0,2); put(0x2228,base+0x1000);
    bindings_rejected(&p,5);
    preparation_fixture(); multiple_instances(0,2); multiple_instances(0x10000,2); multiple_instances(0x20000,2);
    unsigned char extras[3][32];
    for (unsigned i=0;i<3;++i) memcpy(extras[i],memory+i*0x10000+0x2640,32);
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(prep.stage==3 && setter_calls==13);
    for (unsigned i=0;i<3;++i) assert(!memcmp(extras[i],memory+i*0x10000+0x2640,32));
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    put(0x22228,base+0x22400); put32(0x22400,32);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_CHANGED);
    preparation_fixture(); multiple_instances(0,2); put(0x2228,base+0x21000);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==5 && !setter_calls);
    struct jc_appearance_detail detail;
    fixture(); put16(0x712,8);
    assert(jc_appearance_bindings_read_detail(read_memory,NULL,base,&p,&b,&detail)==JC_LAYOUT_RANGE);
    assert(detail.field==9 && detail.parameter==0 && detail.declared==8 && detail.required==16 && detail.offset==4);
    fixture(); put32(0x400,17);
    assert(jc_appearance_bindings_read_detail(read_memory,NULL,base,&p,&b,&detail)==JC_LAYOUT_RANGE);
    assert(detail.field==3 && detail.instances==17 && detail.parameter==-1);
    fixture(); multiple_instances(0,3);
    put(0x2008,base+0x500); put(0x2010,base+0x500);
    assert(!jc_appearance_bindings_read_detail(read_memory,NULL,base,&p,&b,&detail));
    assert(b.instance_count==3 && detail.unique_instances==1 && b.found==8191);
    fixture(); multiple_instances(0,3); put(0x2008,base+0x500);
    assert(!jc_appearance_bindings_read_detail(read_memory,NULL,base,&p,&b,&detail));
    assert(detail.unique_instances==2);
    fixture(); multiple_instances(0,3); put(0x2008,base+0x500);
    put(0x2248,base+0x1004); put32(0x1004,32);
    bindings_rejected(&p,5);
    fixture(); multiple_instances(0,3);
    put(0x2008,base+0x500); put(0x2010,base+0x500); change_address=base+0x2010;
    bindings_rejected(&p,JC_LAYOUT_CHANGED);
    preparation_fixture();
    for (unsigned i=0;i<3;++i) {
        unsigned o=i*0x10000; multiple_instances(o,3);
        put(o+0x2008,base+o+0x500); put(o+0x2010,base+o+0x500);
    }
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(prep.stage==3 && setter_calls==13 && prep.originals_unchanged==1);
    assert(prep.detail.instances==3 && prep.detail.unique_instances==1);
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    put(0x22010,base+0x22240); put(0x22240,base+0x600); put(0x22248,base+0x22680); put32(0x22680,32);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_CHANGED);
    preparation_fixture(); multiple_instances(0x20000,3);
    put(0x22008,base+0x20500); put(0x22010,base+0x20500); put(0x20508,base+0x1000);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==5 && !setter_calls);
    preparation_fixture(); alternate_source_texture(4,0x80000000u); alternate_source_texture(5,0);
    unsigned char original_buffers[2][512];
    memcpy(original_buffers[0],memory+0x1000,512); memcpy(original_buffers[1],memory+0x11000,512);
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(prep.stage==3 && setter_calls==13 && prep.definition_difference==48);
    assert(prep.source_found==8191 && prep.trial_found==8191 && !prep.source_reason);
    assert(!memcmp(original_buffers[0],memory+0x1000,512) && !memcmp(original_buffers[1],memory+0x11000,512));
    assert(!memcmp(memory+0x1084,memory+0x21084,24) && !memcmp(memory+0x10a4,memory+0x210a4,24));
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow));
    put32(0x30000+4*0x200+0x120+0xc,2);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_RANGE);
    preparation_fixture();
    for (unsigned i=0;i<13;++i) if (jc_appearance_kind(i)==2) alternate_source_texture(i,0);
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(prep.definition_difference==(16+32+64+256+4096) && setter_calls==13);
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    alternate_source_texture(4,0x80000000u);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_CHANGED);
    preparation_fixture(); alternate_source_texture(4,0); put(0x108c,7);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_reason==3 && prep.source_parameter==4 && prep.source_code==JC_LAYOUT_RANGE);
    preparation_fixture(); alternate_source_texture(4,0); put(0x1094,base+0x6100);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_reason==3 && prep.source_code==JC_LAYOUT_CHANGED);
    preparation_fixture(); alternate_source_texture(4,0); put32(0x1088,77);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    preparation_fixture(); alternate_source_texture(4,0); put32(0x30800+0x110,3);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_code==JC_LAYOUT_TYPE);
    preparation_fixture(); alternate_source_texture(4,0); fail_address=base+0x30870;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_code==JC_LAYOUT_READ);
    preparation_fixture(); alternate_source_texture(4,0); change_address=base+0x3092c;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_code==JC_LAYOUT_CHANGED);
    preparation_fixture(); alternate_source_texture(4,0); change_address=base+0x3092c; change_hit=3;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_reason==4 && prep.source_parameter==4 && prep.source_code==JC_LAYOUT_CHANGED);
    preparation_fixture(); put(0x20500,base+0x23000);
    memcpy(memory+0x23000,memory+0x600,0x60); put(0x23048,base+0x23100);
    memcpy(memory+0x23100,memory+0x700,13*24); put(0x23100+4*24+8,base+0x23a00);
    strcpy((char *)memory+0x23a00,"otherTexture");
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_reason==2 && prep.source_parameter==4 && prep.missing_trial==16);
    assert(prep.source_found==8191 && prep.trial_found==(8191^16));
    preparation_fixture(); put(0x700+4*24+8,base+0x23a00); strcpy((char *)memory+0x23a00,"otherTexture");
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    assert(prep.source_reason==1 && prep.source_parameter==4 && prep.missing_source==16);
    preparation_fixture(); alternate_source_texture(12,0);
    put32(0x1004+12*32+4,0); /* Native material retains slot 0, resource is on slot 1. */
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(prep.refreshed_indices==4096 && prep.cube_stored_index==0 && prep.cube_current_index==1 && prep.cube_slots==2);
    assert(prep.stage==3 && setter_calls==13 && prep.originals_unchanged==1);
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    for (unsigned frame=0;frame<600;++frame) {
        put32(0x3192c,frame%2);
        assert(!jc_appearance_borrow_check(read_memory,NULL,&borrow));
    }
    put32(0x3192c,2);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_RANGE);
    put32(0x3192c,1); put32(0x21004+12*32+4,2);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_RANGE);
    put32(0x21004+12*32+4,1); put(0x31958,base+0x38000);
    assert(jc_appearance_borrow_check(read_memory,NULL,&borrow)==JC_LAYOUT_CHANGED);
    preparation_fixture(); alternate_source_texture(12,0); put32(0x31950,0);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    preparation_fixture(); alternate_source_texture(12,0); put32(0x31950,17);
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    preparation_fixture(); alternate_source_texture(12,0); fail_address=base+0x31960;
    assert(jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep)==6 && !setter_calls);
    preparation_fixture();
    assert(!jc_appearance_prepare(read_memory,NULL,&setters,base+0x3000,materials,&prep));
    assert(!jc_appearance_borrow_begin(read_memory,NULL,base+0x3000,materials[0],materials[2],&borrow));
    struct jc_appearance_borrow_detail why;
    assert(!jc_appearance_borrow_check_detail(read_memory,NULL,&borrow,&why) && !why.stage && why.parameter==-1);
    struct jc_appearance_borrow altered=borrow;
    altered.ready=0;
    assert(jc_appearance_borrow_check_detail(read_memory,NULL,&altered,&why)==JC_LAYOUT_TYPE && why.stage==1);
    altered=borrow; altered.definition_plan.entries[4].owner++;
    assert(jc_appearance_borrow_check_detail(read_memory,NULL,&altered,&why)==JC_LAYOUT_CHANGED && why.stage==2 && why.parameter==4);
    altered=borrow; altered.plan.entries[4].owner++;
    assert(jc_appearance_borrow_check_detail(read_memory,NULL,&altered,&why)==JC_LAYOUT_CHANGED && why.stage==3 && why.parameter==4);
    altered=borrow; altered.bindings[1].buffer++;
    assert(jc_appearance_borrow_check_detail(read_memory,NULL,&altered,&why)==JC_LAYOUT_CHANGED && why.stage==5 && why.role==1);
    puts("Borrow diagnostics: accepted, definition, original texture, live source identity and trial storage failures distinguished without bypassing checks.");
    shader_tests();
    puts("Appearance: indexed texture slots and 600 frame rotations; trial spine transforms and isolated shader writes; invalid indices, table replacement, cycles, singular matrices, overlapping properties and partial writes rejected. Native setters simulated; no game calls.");
    puts("Appearance: repeated instances, validated live texture bindings, named source failures, complete preparation and borrowed-resource changes passed. Native setters simulated; no game calls.");
    return 0;
}
