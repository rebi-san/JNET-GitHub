#include "native_idle_pose.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x10000];
static const uint64_t image=UINT64_C(0x140000000),base=0x100000;
static unsigned reads,hits;
static uint64_t fail_address,change_address;
static void put(unsigned offset,uint64_t value,unsigned size) { memcpy(memory+offset,&value,size); }
static int reader(void *unused,uint64_t address,void *out,size_t size) {
    (void)unused; assert(++reads<4096);
    if (fail_address>=address && fail_address-address<size) return 0;
    if (change_address>=address && change_address-address<size && hits++==1) memory[change_address-base]^=1;
    if (address==image+0x3763e7c && size==4) { uint32_t threshold=0x3f800000;memcpy(out,&threshold,4);return 1; }
    if (address<base || address-base+size>sizeof(memory)) return 0;
    memcpy(out,memory+(address-base),size); return 1;
}
static void fixture(void) {
    memset(memory,0,sizeof(memory)); reads=hits=0; fail_address=change_address=0;
    put(0,image+0x67f3f8,8);put(0x30,base+0x1000,8);
    put(0x1010,base+0x2000,8);put(0x2018,base+0x3000,8);
    put(0x3000,base+0x4000,8);put(0x3008,base+0x6000,8);put(0x6000,1,4);
    put(0x4100,base+0x5000,8);
    put(0x578,base+0x7000,8);put(0x580,1,8);
    put(0x3030,base+0x8000,8);put(0x3038,1,8);
    put(0x3040,base+0x8100,8);put(0x3048,1,8);
    put(0x8000,base+0x9000,8);put(0x8100,base+0x9100,8);
    put(0x40c8,base+0xa000,8);put(0x40e8,1,4);put(0xa008,base+0xb000,8);
    put(0x40e0,base+0xa100,8);put(0x40f0,1,4);put(0xa108,base+0xb000,8);
    put(0x40d0,base+0xa200,8);put(0x40ec,1,4);put(0xa200,base+0xb100,8);put(0xa208,base+0xb000,8);
}
static int check(void) { struct jc_idle_pose out;return jc_idle_pose_ready(reader,NULL,image,base,&out); }
static void detail_tests(void) {
    struct jc_idle_pose out; struct jc_idle_detail detail;
    fixture(); assert(!jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail));
    assert(!detail.field && detail.index==-1 && out.model==base+0x4000);
    const unsigned offsets[]={0x570,0x7014,0x7010,0x700c,0xa108,0xb100};
    const uint64_t values[]={0x3f800000,666,0x3f800000,0x7fc00000,0,0x7f800000};
    const unsigned sizes[]={4,4,4,4,8,4};
    const int codes[]={6,7,6,8,8,8},fields[]={4,6,7,8,12,14};
    for (unsigned i=0;i<6;++i) {
        fixture(); put(offsets[i],values[i],sizes[i]);
        assert(jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail)==codes[i]);
        assert(detail.field==fields[i] && detail.index==(i ? 0 : -1));
        assert(!out.model && !out.control && !out.root && !out.references);
    }
    puts("Idle detail: accepted input and six distinct rejection fields checked; rejected outputs remain empty.");
}
static void dense_fixture(unsigned count) {
    fixture(); put(0x578,base+0xc000,8); put(0x580,count,8);
    put(0x3030,base+0x9000,8); put(0x3038,count,8);
    put(0x3040,base+0x9400,8); put(0x3048,count,8);
    for (unsigned i=0;i<count;++i) {
        put(0xc000+i*0x50+20,i,4);
        put(0x9000+i*8,base+0x9800,8); put(0x9400+i*8,base+0x9900,8);
    }
}
static void batch_tests(void) {
    struct jc_idle_pose out,zero={0}; struct jc_idle_detail detail;
    dense_fixture(65); assert(!jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail));
    unsigned total=reads; assert(total<200);
    const unsigned mutations[]={0x570,0x580,0x3038,0x9008,0x9408,0xc050+4,0xc000+64*0x50+20};
    for (unsigned i=0;i<sizeof(mutations)/sizeof(mutations[0]);++i) {
        dense_fixture(65); change_address=base+mutations[i];
        assert(jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail)==3);
        assert(!memcmp(&out,&zero,sizeof(out)));
    }
    dense_fixture(65); fail_address=base+0xc000+64*0x50+8;
    assert(jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail)==1);
    assert(!memcmp(&out,&zero,sizeof(out)));
    dense_fixture(65); put(0xc000+64*0x50+4,0x7fc00000,4);
    assert(jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail)==8 && detail.index==64);
    dense_fixture(65); put(0xc000+64*0x50+20,65,4);
    assert(jc_idle_pose_ready_detail(reader,NULL,image,base,&out,&detail)==7 && detail.index==64);
    printf("Idle batch: 65 entries in %u reads; changed headers, pointer tables, entry fields, interior faults and final partial batch checked.\n",total);
}
static void matrix_fixture(void) {
    fixture(); put(0x40e8,142,4);
    for (unsigned i=0;i<142;++i) {
        put(0xa000+i*32,i ? base+0xa000+(i-1)*32 : 0,8);
        put(0xa008+i*32,base+0xc000+i*64,8);
        put(0xa018+i*32,base+0xc000+i*64,8);
    }
}
static void matrix_tests(void) {
    struct jc_idle_detail detail;
    matrix_fixture(); assert(!jc_idle_matrix_inputs(reader,NULL,base+0x4000,0,&detail));
    unsigned total=reads; assert(total<16);
    matrix_fixture(); put(0xc000+141*64,0x7fc00000,4);
    assert(jc_idle_matrix_inputs(reader,NULL,base+0x4000,0,&detail)==8 && detail.index==141);
    matrix_fixture(); put(0xa000+32,base+0xa000+32,8);
    assert(jc_idle_matrix_inputs(reader,NULL,base+0x4000,0,&detail)==6);
    matrix_fixture(); change_address=base+0xa018;
    assert(jc_idle_matrix_inputs(reader,NULL,base+0x4000,0,&detail)==3);
    matrix_fixture(); change_address=base+0x40e8;
    assert(jc_idle_matrix_inputs(reader,NULL,base+0x4000,0,&detail)==3);
    matrix_fixture(); fail_address=base+0xc000+141*64+7;
    assert(jc_idle_matrix_inputs(reader,NULL,base+0x4000,0,&detail)==8);
    printf("Idle matrix inputs: 142 hierarchy rows in %u reads; final-row NaN/fault, cyclic parent, changed table and count rejected.\n",total);
}
int main(void) {
    matrix_tests();
    batch_tests();
    detail_tests();
    fixture();assert(!check());
    struct jc_idle_pose active_out;struct jc_idle_detail active_detail;
    fixture();put(0x7010,0x3f800000,4);put(0x7000,1,1);
    assert(!jc_mesh_pose_ready_detail(reader,NULL,image,base,&active_out,&active_detail,1));
    assert(check()==6);
    put(0x7001,1,1);assert(jc_mesh_pose_ready_detail(reader,NULL,image,base,&active_out,&active_detail,1)==6);
    fixture();put(0x7014,666,4);assert(check()==7);
    fixture();put(0x7010,0x3f800000,4);assert(check()==6);
    fixture();put(0x7001,1,1);assert(check()==6);
    fixture();put(0x570,0x3f800000,4);assert(check()==6);
    fixture();put(0x700c,0x7fc00000,4);assert(check()==8);
    fixture();put(0x6000,0,4);assert(check()==5);
    fixture();put(0x3038,1025,8);assert(check()==5);
    fixture();put(0x40ec,1025,4);assert(check()==5);
    fixture();put(0xa108,0,8);assert(check()==8);
    fixture();put(0xb100,0x7f800000,4);assert(check()==8);
    fixture();put(0x8100,0,8);assert(check()==1);
    static unsigned char context[0x10090];
    struct jc_idle_pose inputs={base+0x4000,base+0x6000,base+0x5000,1};
    assert(jc_idle_context_valid(context,&inputs));
    uint32_t queued=1;memcpy(context+0x10080,&queued,4);
    memcpy(context+0x80,&inputs.model,8);memcpy(context+0x88,&inputs.control,8);
    assert(jc_idle_context_valid(context,&inputs));
    context[0x80]^=1;assert(!jc_idle_context_valid(context,&inputs));context[0x80]^=1;
    context[0x90]=1;assert(!jc_idle_context_valid(context,&inputs));context[0x90]=0;
    queued=2;memcpy(context+0x10080,&queued,4);assert(!jc_idle_context_valid(context,&inputs));
    puts("Idle pose inputs/context: 17 synthetic checks passed; no native calls.");
}
