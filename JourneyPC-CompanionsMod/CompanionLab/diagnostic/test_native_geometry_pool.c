#include "native_geometry_pool.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x6000];
static const uint64_t base=0x100000,material=0x100100,source=0x101000,payload=0x102000,table=0x103000;
static int reads,fail,change,head_reads;
static uint64_t fail_at,change_at;
static void put(uint64_t address,uint64_t value) { memcpy(memory+(address-base),&value,8); }
static int reader(void *ctx,uint64_t address,void *out,size_t n) {
    (void)ctx; assert(++reads<=150);
    if (fail || (fail_at>=address && fail_at-address<n) || address<base || address-base>sizeof(memory) || n>sizeof(memory)-(address-base)) return 0;
    if (change && address==table && head_reads++==1) put(table,0);
    if (change_at>=address && change_at-address<n && head_reads++==1) memory[change_at-base]^=1;
    memcpy(out,memory+(address-base),n); return 1;
}
static void fixture(uint32_t count) {
    memset(memory,0,sizeof(memory)); reads=fail=change=head_reads=0; fail_at=change_at=0;
    put(material+0x10,source); put(source+0x10,payload); put(payload+0x180,table);
    put(payload+0x188,count); put(material+0x30,table); put(table,material+0x30);
}
int main(void) {
    struct jc_geometry_pool r;
    fixture(1); assert(jc_geometry_pool_read(reader,NULL,material,&r)==0 && r.slots==1 && r.occupied==1 && !r.free_slots && r.source==source);
    fixture(3); assert(jc_geometry_pool_read(reader,NULL,material,&r)==0 && r.free_slots==2);
    fixture(64); assert(jc_geometry_pool_read(reader,NULL,material,&r)==0 && r.free_slots==63);
    fixture(65); assert(jc_geometry_pool_read(reader,NULL,material,&r)==4 && reads==4);
    struct jc_geometry_slots glyph_slots;
    fixture(136);
    assert(!jc_geometry_pool_read_slots_bounded(reader,NULL,material,136,&r,&glyph_slots));
    assert(r.slots==136 && r.free_slots==135);
    fixture(137);
    assert(jc_geometry_pool_read_slots_bounded(reader,NULL,material,136,&r,&glyph_slots)==JC_LAYOUT_RANGE);
    fixture(136);change_at=table+135*0x30;
    assert(jc_geometry_pool_read_slots_bounded(reader,NULL,material,136,&r,&glyph_slots)==JC_LAYOUT_CHANGED);
    fixture(0); assert(jc_geometry_pool_read(reader,NULL,material,&r)==4);
    fixture(1); put(payload+0x180,UINT64_MAX); assert(jc_geometry_pool_read(reader,NULL,material,&r)==4);
    fixture(2); put(material+0x30,table+0x20); assert(jc_geometry_pool_read(reader,NULL,material,&r)==2);
    fixture(1); put(table,0); assert(jc_geometry_pool_read(reader,NULL,material,&r)==2);
    fixture(1); fail=1; assert(jc_geometry_pool_read(reader,NULL,material,&r)==1);
    fixture(1); change=1; assert(jc_geometry_pool_read(reader,NULL,material,&r)==3 && !r.source);
    fixture(64); assert(!jc_geometry_pool_read(reader,NULL,material,&r) && reads==10);
    fixture(3); fail_at=table+0x30+1;
    assert(jc_geometry_pool_read(reader,NULL,material,&r)==JC_LAYOUT_READ && !r.source);
    fixture(3); change_at=table+0x30;
    assert(jc_geometry_pool_read(reader,NULL,material,&r)==JC_LAYOUT_CHANGED && !r.source);
    fixture(3); change_at=payload+0x188;
    assert(jc_geometry_pool_read(reader,NULL,material,&r)==JC_LAYOUT_CHANGED && !r.source);
    struct jc_geometry_build_state build;
    const uint64_t image=0x140000000;
    fixture(2); put(payload,image+0x680930); put(payload+0x110,2);
    assert(jc_geometry_build_read(reader,NULL,image,payload,&build)==0 && build.limit==2 && build.slots==2 && build.occupied==1);
    fixture(0); put(payload,image+0x680930); put(payload+0x110,2); put(payload+0x180,0);
    assert(jc_geometry_build_read(reader,NULL,image,payload,&build)==0 && !build.slots && !build.occupied);
    fixture(2); put(payload,image+0x680930); put(payload+0x110,2); change=1;
    assert(jc_geometry_build_read(reader,NULL,image,payload,&build)==3 && !build.limit);
    fixture(2); put(payload,image+0x680930); put(payload+0x110,65);
    assert(jc_geometry_build_read(reader,NULL,image,payload,&build)==4);
    fixture(2); put(payload+0x110,2);
    assert(jc_geometry_build_read(reader,NULL,image,payload,&build)==2);
    puts("Geometry pool/build: 19 bounded read-only synthetic checks passed; no native game code executed.");
}
