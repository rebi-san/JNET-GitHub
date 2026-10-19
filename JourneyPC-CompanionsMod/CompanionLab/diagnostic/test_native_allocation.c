#include "native_lifetime.h"
#include <assert.h>
#include <stdio.h>
static unsigned char object[0x14640];
static const uint64_t image=UINT64_C(0x140000000), barn=0x200000;
static uint64_t candidate;
static int created, destroyed, reads, deny, reenter;
static uint64_t current_barn;
static struct jc_lifetime life;
static void put(size_t offset, uint64_t value) { memcpy(object+offset,&value,8); }
static uint64_t create(void *unused) { (void)unused; assert(!created++); return candidate; }
static void destroy(void *unused, uint64_t address) {
    (void)unused; assert(address==candidate && created==1 && !destroyed++);
    if (reenter) { assert(life.object==0); assert(jc_lifetime_cleanup(&life,NULL,image,0x700000,42,barn+0xb0)==3); }
    memset(object,0xdd,sizeof(object));
}
static int read_object(void *unused, uint64_t address, void *out, size_t size) {
    (void)unused; assert(!destroyed && ++reads<=27);
    if (deny) return 0;
    if (address==0x7001e0 && size==8) { memcpy(out,&current_barn,8); return 1; }
    assert(address>=candidate && address-candidate<=sizeof(object) && size<=sizeof(object)-(address-candidate));
    memcpy(out,object+(size_t)(address-candidate),size); return 1;
}
static void fixture(void) {
    memset(object,0,sizeof(object)); candidate=0x400000;
    created=destroyed=reads=deny=reenter=0; current_barn=barn;
    life=(struct jc_lifetime){candidate,0x700000,barn,42,1};
    put(0,image+0x5d0380); put(0x160,image+0x67f3f8);
    put(0x7b0,image+0x67f3f8); put(0xe50,image+0x67f3f8); put(0xa390,image+0x5d0360);
}
int main(void) {
    const struct jc_allocation_ops ops={read_object,create,destroy,NULL};
    fixture(); assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_OK && created==1 && destroyed==1);
    fixture(); candidate=0; assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_NULL && !destroyed && !reads);
    fixture(); candidate=barn+0xb0; assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed && !reads);
    fixture(); candidate=barn-0x10; assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed && !reads);
    fixture(); candidate++; assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed && !reads);
    fixture(); candidate=UINT64_C(0x7ffffffffff0); assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed && !reads);
    fixture(); deny=1; assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed);
    fixture(); put(0xe50,0); assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed);
    fixture(); put(0x9b40,0x500000); assert(jc_allocation_rehearse(&ops,image,barn)==JC_ALLOC_UNTRUSTED && !destroyed);
    fixture(); assert(jc_allocation_rehearse(&ops,0,barn)==JC_ALLOC_UNTRUSTED && !created);
    fixture(); assert(jc_allocation_rehearse(&ops,image,UINT64_MAX)==JC_ALLOC_UNTRUSTED && !created);
    fixture(); created=1; reenter=1;
    assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,42,barn+0xb0)==2 && destroyed==1 && !life.object);
    assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,42,barn+0xb0)==2 && destroyed==1);
    fixture(); assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,42,barn+0x146f0)==1 && !destroyed && !reads);
    fixture(); assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,43,barn+0xb0)==-1 && !destroyed && !life.object);
    fixture(); current_barn+=0x100; assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,42,barn+0xb0)==-1 && !destroyed);
    fixture(); put(0x2240,0x900000); assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,42,barn+0xb0)==-1 && !destroyed);
    fixture(); deny=1; assert(jc_lifetime_cleanup(&life,&ops,image,0x700000,42,barn+0xb0)==-1 && !destroyed);
    fixture(); assert(jc_lifetime_cleanup(&life,&ops,image,0x710000,42,barn+0xb0)==-1 && !destroyed);
    puts("Allocation/lifetime: 19 synthetic checks passed; native constructors were not executed.");
    return 0;
}
