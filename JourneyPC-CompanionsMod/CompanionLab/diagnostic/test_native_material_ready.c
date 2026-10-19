#include "native_material_ready.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x2000];
static const uint64_t base=0x100000;
static int changed,slot_reads;
static void put(unsigned off,uint64_t value) { memcpy(memory+off,&value,8); }
static int read_memory(void *ctx,uint64_t address,void *out,size_t size) {
    (void)ctx;
    if (changed && address==base+0x30 && slot_reads++==1) put(0x30,base+0x200);
    if (address<base || address-base>sizeof(memory) || size>sizeof(memory)-(address-base)) return 0;
    memcpy(out,memory+(address-base),size); return 1;
}
static void fixture(void) {
    memset(memory,0,sizeof(memory)); changed=slot_reads=0;
    put(0x30,base+0x100); put(0x110,base+0x200); put(0x218,base+0x300);
    put(0x380,base+0x400); put(0x400,1); put(0x408,base+0x500); put(0x500,base+0x600);
}
int main(void) {
    uint64_t payload;
    fixture(); assert(!jc_material_ready(read_memory,NULL,base,&payload) && payload==base+0x300);
    fixture(); put(0x400,2); put(0x408,base+0x700); put(0x700,base+0x500);
    assert(!jc_material_ready(read_memory,NULL,base,&payload));
    fixture(); put(0x390,1); put(0x388,base+0x400); assert(!jc_material_ready(read_memory,NULL,base,&payload));
    fixture(); put(0x390,2); assert(jc_material_ready(read_memory,NULL,base,&payload)==JC_LAYOUT_RANGE && !payload);
    fixture(); put(0x390,UINT32_MAX); assert(jc_material_ready(read_memory,NULL,base,&payload)==JC_LAYOUT_RANGE);
    fixture(); put(0x400,0); assert(jc_material_ready(read_memory,NULL,base,&payload)==JC_LAYOUT_RANGE);
    fixture(); put(0x400,4097); assert(jc_material_ready(read_memory,NULL,base,&payload)==JC_LAYOUT_RANGE);
    fixture(); put(0x500,0); assert(jc_material_ready(read_memory,NULL,base,&payload)==JC_LAYOUT_READ);
    fixture(); changed=1; assert(jc_material_ready(read_memory,NULL,base,&payload)==JC_LAYOUT_CHANGED && !payload);
    puts("Material prefix: 9 synthetic cases passed; no native material function executed.");
}
