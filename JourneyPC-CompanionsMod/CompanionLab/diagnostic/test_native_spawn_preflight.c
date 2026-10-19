#include "native_spawn_preflight.h"
#include <assert.h>
#include <stdio.h>
static const uint64_t image=UINT64_C(0x140000000), pool=0x900000, dude=0x800000;
static const uint64_t meshes[3]={0x100000,0x200000,0x300000};
static uint64_t slots[3], resources[3], payloads[3], symbol;
static uint32_t count;
static int reads, mutate, deny, wrong_type;
static int reader(void *unused, uint64_t address, void *out, size_t size) {
    (void)unused; assert(++reads<=24);
    if (deny) return 0;
    uint64_t value=0; int found=0;
    if (address==pool+0x434010 && size==4) { memcpy(out,&count,4); return 1; }
    if (address==dude+0x2240) { value=symbol; found=1; }
    for (int i=0;i<3;++i) {
        if (address==meshes[i]) { value=wrong_type ? 0 : image+0x67f3f8; found=1; }
        if (address==meshes[i]+0x30) { value=slots[i]; if (mutate && reads>12) value=0; found=1; }
        if (address==slots[i]+0x10) { value=resources[i]; found=1; }
        if (address==resources[i]+0x18) { value=payloads[i]; found=1; }
    }
    if (!found) return 0;
    assert(size==8); memcpy(out,&value,8); return 1;
}
static void fixture(void) {
    reads=mutate=deny=wrong_type=0; count=10;
    for (int i=0;i<3;++i) {
        slots[i]=0x400000+(uint64_t)i*0x100;
        resources[i]=0x500000+(uint64_t)i*0x100;
        payloads[i]=0x600000+(uint64_t)i*0x100;
    }
    symbol=pool+0x10+(uint64_t)count*0x10d0;
}
int main(void) {
    int distinct; struct jc_symbol_reservation reservation;
    fixture(); assert(!jc_three_clone_check(reader,NULL,image,meshes,&distinct) && distinct);
    fixture(); payloads[2]=payloads[0]; assert(!jc_three_clone_check(reader,NULL,image,meshes,&distinct) && !distinct);
    fixture(); payloads[2]=payloads[0]+8; assert(!jc_three_clone_check(reader,NULL,image,meshes,&distinct) && !distinct);
    fixture(); slots[2]=slots[0]; assert(!jc_three_clone_check(reader,NULL,image,meshes,&distinct) && !distinct);
    fixture(); resources[2]=resources[0]; assert(!jc_three_clone_check(reader,NULL,image,meshes,&distinct) && !distinct);
    fixture(); slots[2]=0; assert(!jc_three_clone_check(reader,NULL,image,meshes,&distinct) && !distinct);
    fixture(); mutate=1; assert(jc_three_clone_check(reader,NULL,image,meshes,&distinct)==JC_LAYOUT_CHANGED && !distinct);
    fixture(); deny=1; assert(jc_three_clone_check(reader,NULL,image,meshes,&distinct)==JC_LAYOUT_READ && !distinct);
    fixture(); wrong_type=1; assert(jc_three_clone_check(reader,NULL,image,meshes,&distinct)==JC_LAYOUT_TYPE && !distinct);
    fixture(); payloads[2]=UINT64_C(0x7ffffffffff8); assert(jc_three_clone_check(reader,NULL,image,meshes,&distinct)==JC_LAYOUT_RANGE && !distinct);
    fixture(); assert(!jc_symbol_prepare(reader,NULL,pool,&reservation)); count++;
    assert(!jc_symbol_check_append(reader,NULL,&reservation,pool,dude));
    count++; assert(jc_symbol_check_append(reader,NULL,&reservation,pool,dude)==JC_LAYOUT_CHANGED);
    count--; symbol+=0x10d0; assert(jc_symbol_check_append(reader,NULL,&reservation,pool,dude)==JC_LAYOUT_CHANGED);
    assert(jc_symbol_check_append(reader,NULL,&reservation,pool+0x10,dude)==JC_LAYOUT_CHANGED);
    fixture(); count=1024; assert(jc_symbol_prepare(reader,NULL,pool,&reservation)==JC_LAYOUT_RANGE);
    fixture(); count=1023; assert(!jc_symbol_prepare(reader,NULL,pool,&reservation));
    fixture(); assert(jc_symbol_prepare(reader,NULL,UINT64_MAX,&reservation)==JC_LAYOUT_RANGE);
    puts("Spawn preflight: 17 synthetic checks passed; no native initialization, rendering or cleanup executed.");
    return 0;
}
