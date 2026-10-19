#include "native_resources.h"
#include <assert.h>
#include <stdio.h>
static unsigned char game[0x200], barn[0x28dd0];
static const uint64_t image=UINT64_C(0x140000000), ga=0x100000, ba=0x200000;
static int reads, entry_reads, mutate, deny;
static void put(uint64_t offset, uint64_t value) { memcpy(barn+offset,&value,8); }
static int reader(void *unused, uint64_t address, void *out, size_t size) {
    (void)unused;
    assert(++reads <= 40);
    if (deny) return 0;
    if (address==ba+0xb0+0x6d8 && ++entry_reads==2 && mutate) put(0xb0+0x6d8,0x500000);
    if (address>=ga && address-ga<=sizeof(game) && size<=sizeof(game)-(address-ga)) {
        memcpy(out,game+(size_t)(address-ga),size); return 1;
    }
    if (address>=ba && address-ba<=sizeof(barn) && size<=sizeof(barn)-(address-ba)) {
        memcpy(out,barn+(size_t)(address-ba),size); return 1;
    }
    assert(!"Unexpected pointer traversal"); return 0;
}
static void fixture(void) {
    memset(game,0,sizeof(game)); memset(barn,0,sizeof(barn));
    reads=entry_reads=mutate=deny=0;
    memcpy(game+0x1e0,&ba,8); put(0,image+0x5da340);
    const uint64_t offsets[]={0xb0,0x146f0};
    for (int i=0;i<2;++i) {
        uint64_t d=offsets[i];
        put(d,image+0x5d0380); put(d+0x160,image+0x67f3f8);
        put(d+0x6d8,0x400000+(uint64_t)i*0x1000); put(d+0x6e0,3);
        put(d+0xa620,ba+d+0x160);
    }
}
int main(void) {
    struct jc_resource_result r;
    fixture();
    assert(jc_native_resources(reader,NULL,image,ga,&r)==0 && r.tables_available && !r.tables_overlap &&
           r.controller_owns_mesh[0] && r.controller_owns_mesh[1] && r.counts[0]==3);
    fixture(); put(0x146f0+0x6d8,0x400000);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==0 && r.tables_overlap);
    fixture(); put(0x146f0+0x6d8,0x400010);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==0 && r.tables_overlap);
    fixture(); put(0x146f0+0x6d8,0x400000+3*0x50);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==0 && !r.tables_overlap);
    fixture(); put(0x146f0+0x6e0,0); put(0x146f0+0x6d8,0);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==0 && !r.tables_available);
    fixture(); put(0x146f0+0xa620,ba+0xb0+0x160);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==0 && !r.controller_owns_mesh[1]);
    fixture(); put(0xb0+0x160,0);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==JC_LAYOUT_TYPE);
    fixture(); put(0xb0+0x6e0,4097);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==JC_LAYOUT_RANGE);
    fixture(); put(0xb0+0x6d8,8);
    assert(jc_native_resources(reader,NULL,image,ga,&r)==JC_LAYOUT_RANGE);
    fixture(); put(0xb0+0x6d8,UINT64_C(0x7ffffffffff8));
    assert(jc_native_resources(reader,NULL,image,ga,&r)==JC_LAYOUT_RANGE);
    fixture(); mutate=1;
    assert(jc_native_resources(reader,NULL,image,ga,&r)==JC_LAYOUT_CHANGED);
    fixture(); deny=1;
    assert(jc_native_resources(reader,NULL,image,ga,&r)==JC_LAYOUT_READ);
    puts("Native resource reader: 12 synthetic checks passed; no live ownership proof.");
    return 0;
}
