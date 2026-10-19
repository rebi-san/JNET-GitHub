#include "native_shader_inputs.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x3000];
static const uint64_t base=0x100000;
static uint64_t fail_address,change_address;
static unsigned change_reads,reads;
static void put(unsigned off,uint64_t value) { memcpy(memory+off,&value,8); }
static void put16(unsigned off,uint16_t value) { memcpy(memory+off,&value,2); }
static int read_memory(void *ctx,uint64_t address,void *out,size_t size) {
    (void)ctx; ++reads;
    if (fail_address>=address && fail_address-address<size) return 0;
    if (address<base || address-base>sizeof(memory) || size>sizeof(memory)-(address-base)) return 0;
    if (change_address>=address && change_address-address<size && change_reads++==1) memory[change_address-base]^=1;
    memcpy(out,memory+(address-base),size); return 1;
}
static void fixture(void) {
    static const char *names[]={"dudeDissolve","dudeCenter","dudeInvTorso","model","modelViewProj","viewprojection"};
    static const unsigned sizes[]={4,16,64,64,64,64};
    memset(memory,0,sizeof(memory)); fail_address=change_address=0; change_reads=reads=0;
    put(0x30,base+0x100); put(0x110,base+0x200); put(0x218,base+0x300);
    put(0x380,base+0x400); put(0x400,1); put(0x408,base+0x500);
    put(0x500,base+0x600); put(0x508,base+0x1000); put(0x1000,512);
    put(0x640,3); put(0x648,base+0x700); put(0x650,3); put(0x658,base+0x748);
    for (unsigned i=0;i<6;++i) {
        unsigned entry=0x700+i*24,name=0x900+i*32;
        strcpy((char *)memory+name,names[i]); put(entry+8,(base+name)|(i&1));
        put16(entry+16,4+i*64); put16(entry+18,sizes[i]|0x8000);
    }
    float dissolve=0.25f; memcpy(memory+0x1004,&dissolve,4);
    float one=1; memcpy(memory+0x1084,&one,4);
}
static void rejected(int expected) {
    struct jc_shader_inputs result,empty={0}; memset(&result,0xff,sizeof(result));
    assert(jc_shader_inputs_read(read_memory,NULL,base,&result)==expected);
    assert(!memcmp(&result,&empty,sizeof(result)));
    assert(reads<1100);
}
int main(void) {
    struct jc_shader_inputs result;
    fixture(); assert(!jc_shader_inputs_read(read_memory,NULL,base,&result));
    assert(result.buffer==base+0x1000 && result.bytes==512 && result.found==63 && result.finite==63);
    assert(result.zero==58 && result.dissolve_milli==250);
    struct jc_shader_inputs storage;
    assert(!jc_shader_storage_read(read_memory,NULL,base,&storage));
    assert(storage.buffer==result.buffer && storage.bytes==result.bytes && storage.instance==result.instance && storage.shader==result.shader);
    fixture();put16(0x700+18,1); /* An incompatible torso declaration does not change storage. */
    assert(!jc_shader_storage_read(read_memory,NULL,base,&storage));
    assert(jc_shader_inputs_read(read_memory,NULL,base,&storage)==JC_LAYOUT_RANGE);
    fixture();change_address=base+0x508;
    assert(jc_shader_storage_read(read_memory,NULL,base,&storage)==JC_LAYOUT_CHANGED && !storage.buffer);
    fixture();put(0x1000,0);assert(jc_shader_storage_read(read_memory,NULL,base,&storage)==JC_LAYOUT_RANGE);
    fixture();assert(!jc_shader_inputs_read(read_memory,NULL,base,&result));
    unsigned small_reads=reads;
    struct jc_shader_inputs expected=result;
    fixture(); uint32_t buffer_bytes=4096; memcpy(memory+0x1000,&buffer_bytes,4);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && result.bytes==4096);
    assert(!memcmp(result.values,expected.values,sizeof(result.values)));
    fixture(); buffer_bytes=4097; memcpy(memory+0x1000,&buffer_bytes,4);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && result.bytes==4097);
    assert(!memcmp(result.values,expected.values,sizeof(result.values)) && reads>small_reads);
    printf("Shader buffer batching: %u reads; larger-buffer fallback %u reads with identical parameter values.\n",small_reads,reads);
    fixture(); fail_address=base+0x11ff; rejected(JC_LAYOUT_READ);
    fixture(); change_address=base+0x11ff; rejected(JC_LAYOUT_CHANGED);

    fixture(); put(0x390,1); put(0x388,base+0x400);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && result.found==63);
    fixture(); put(0x650,0);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && result.found==7 && result.zero==2);
    fixture(); put(0x640,0); put(0x650,0);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && !result.found && !result.zero);
    fixture(); uint32_t nan=0x7fc00000; memcpy(memory+0x1004,&nan,4);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && result.found==63 && result.finite==62 && !(result.zero&1));
    fixture(); uint32_t negative_zero=0x80000000; memcpy(memory+0x1004,&negative_zero,4);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && (result.zero&1) && !result.dissolve_milli);
    fixture(); put(0x640,257); rejected(JC_LAYOUT_RANGE);
    fixture(); put(0x1000,1048577); rejected(JC_LAYOUT_RANGE);
    fixture(); put16(0x710,2); rejected(JC_LAYOUT_RANGE);
    fixture(); put16(0x712,3); rejected(JC_LAYOUT_RANGE);
    fixture(); put16(0x710,511); rejected(JC_LAYOUT_RANGE);
    fixture(); fail_address=base+0x1004; rejected(JC_LAYOUT_READ);
    fixture(); fail_address=base+0x900; rejected(JC_LAYOUT_READ);
    fixture(); change_address=base+0x1004; rejected(JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x640; rejected(JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x900; rejected(JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x508; rejected(JC_LAYOUT_CHANGED);
    fixture(); put(0x390,2); rejected(JC_LAYOUT_RANGE);
    fixture(); put(0x400,2); put(0x408,base+0x800); put(0x800,base+0x500);
    assert(!jc_shader_inputs_read(read_memory,NULL,base,&result) && result.found==63);
    fixture(); float huge=1001; memcpy(memory+0x1004,&huge,4); rejected(JC_LAYOUT_RANGE);
    struct jc_scalar_observation scalar;
    fixture(); assert(!jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar));
    assert(scalar.value==.25f && !scalar.stage && scalar.offset==4 && !scalar.table);
    unsigned scalar_reads=reads;
    fixture(); strcpy((char *)memory+0x960,"dudeDissolve");
    assert(jc_shader_inputs_read(read_memory,NULL,base,&result)==JC_LAYOUT_TYPE);
    assert(!jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar) && scalar.offset==4);
    fixture(); put16(0x728,4);
    assert(jc_shader_inputs_read(read_memory,NULL,base,&result)==JC_LAYOUT_TYPE);
    assert(!jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar) && scalar.value==.25f);
    fixture(); strcpy((char *)memory+0x900,"unused"); strcpy((char *)memory+0x960,"dudeDissolve");
    assert(!jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar) && scalar.table==1 && scalar.offset==196);
    fixture(); assert(jc_material_scalar_observe(read_memory,NULL,base,"missing",&scalar)==JC_LAYOUT_TYPE && scalar.stage==4);
    fixture(); change_address=base+0x900;
    assert(jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar)==JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x1004;
    assert(jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar)==JC_LAYOUT_CHANGED);
    fixture(); change_address=base+0x508;
    assert(jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar)==JC_LAYOUT_CHANGED);
    fixture(); fail_address=base+0x1004;
    assert(jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar)==JC_LAYOUT_READ && scalar.stage==5);
    fixture(); memcpy(memory+0x1004,&nan,4);
    assert(jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar)==JC_LAYOUT_RANGE);
    fixture(); put16(0x712,3);
    assert(jc_material_scalar_observe(read_memory,NULL,base,"dudeDissolve",&scalar)==JC_LAYOUT_RANGE);
    printf("Read-only native scalar lookup: %u reads; native first-match precedence, alias/duplicate declarations, missing names, partial reads, mutations and bounds checked. Write guards unchanged.\n",scalar_reads);
    puts("Shader input reader: 20 synthetic cases passed; no native shader calls or writes.");
}
