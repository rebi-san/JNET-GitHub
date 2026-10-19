#include "native_animation_playback.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x20000];
static const uint64_t base=0x100000,image=UINT64_C(0x140000000);
static unsigned writes,reads,mutate_hits,read_limit,first_write_reads;
static uint64_t fail_address,mutate_address;
static int write_fail;
static uint32_t global_frame;
static unsigned frame_reads,change_frame_read;
static void put(unsigned offset,uint64_t value,unsigned size) { memcpy(memory+offset,&value,size); }
static void number(unsigned offset,float value) { memcpy(memory+offset,&value,4); }
static int reader(void *ctx,uint64_t address,void *out,size_t size) {
    (void)ctx; assert(++reads<10000);
    if (read_limit && reads>read_limit) return 0;
    if (address==image+0x3c94294 && size==4) {
        if (fail_address==address) return 0;
        if (++frame_reads==change_frame_read) ++global_frame;
        memcpy(out,&global_frame,4); return 1;
    }
    if (address<base || size>sizeof(memory) || address-base>sizeof(memory)-size) return 0;
    if (fail_address>=address && fail_address-address<size) return 0;
    if (mutate_address>=address && mutate_address-address<size && ++mutate_hits==2) memory[mutate_address-base]^=1;
    memcpy(out,memory+(address-base),size); return 1;
}
static int writer(void *ctx,uint64_t address,const void *data,size_t size) {
    (void)ctx; assert(size==20 && address>=base+0x7000 && address<base+0x70a0);
    if (!writes) first_write_reads=reads;
    ++writes; if (write_fail) return 0;
    memcpy(memory+(address-base),data,size); return 1;
}
static int stamp_writer(void *ctx,uint64_t address,const void *data,size_t size) {
    (void)ctx; assert(size==4 && address==base+0x3010);
    if (!writes) first_write_reads=reads;
    ++writes; if (write_fail) return 0;
    memcpy(memory+(address-base),data,size); return 1;
}
static void fixture(void) {
    memset(memory,0,sizeof(memory)); reads=writes=mutate_hits=read_limit=first_write_reads=0; fail_address=mutate_address=0; write_fail=0;
    global_frame=42; frame_reads=change_frame_read=0;
    put(0,image+0x67f3f8,8);put(0x30,base+0x1000,8);
    put(0x1010,base+0x2000,8);put(0x2018,base+0x3000,8);
    put(0x3000,base+0x4000,8);put(0x3008,base+0x6000,8);put(0x6000,1,4);
    put(0x3010,42,4);
    put(0x4100,base+0x5000,8);
    put(0x578,base+0x7000,8);put(0x580,2,8);
    put(0x3030,base+0x8000,8);put(0x3038,2,8);
    put(0x3040,base+0x8100,8);put(0x3048,2,8);
    put(0x3050,base+0x8200,8);put(0x3058,2,8);
    put(0x3060,base+0x8300,8);put(0x3068,2,8);
    put(0x3020,base+0xa000,8);put(0x3028,base+0xa100,8);
    put(0xa028,2,8);put(0xa128,3,8);
    for (unsigned i=0;i<2;++i) {
        unsigned row=0x7000+i*0x50;
        put(row+20,i,4);number(row+4,(float)i+1);number(row+8,-1);number(row+12,1);number(row+16,0.5f);
        memcpy(memory+row+0x20,i ? "Walk" : "Idle",5);put(row+0x30,4,8);put(row+0x38,15,8);
        put(0x8000+i*8,base+0x9000+i*0x40,8);
        put(0x8100+i*8,base+0x9100+i*0x40,8);number(0x9124+i*0x40,10);
        put(0x8200+i*8,base+0xb000+i*0x40,8);put(0x8300+i*8,base+0xb100+i*0x40,8);
        put(0xb010+i*0x40,base+0xc000,8);put(0xb110+i*0x40,base+0xc000,8);
    }
    put(0xc000,2,8);put(0xc008,base+0x8000,8);
    for (unsigned i=0;i<3;++i) {
        unsigned mesh=0xd000+i*0x1000;
        put(mesh+0x578,base+0x10000+i*0x1000,8);put(mesh+0x580,2,8);
        put(mesh,image+0x67f3f8,8); put(mesh+0x30,base+0x16000+i*0x100,8);
        put(0x16010+i*0x100,base+0x17000+i*0x100,8); put(0x17018+i*0x100,base+0x18000+i*0x100,8);
        put(0x18000+i*0x100,base+0x19000+i*0x200,8); put(0x18008+i*0x100,base+0x1a000+i*0x100,8);
        put(0x18010+i*0x100,42,4);
    }
}
static int apply(struct jc_anim_layout *layout,const struct jc_anim_sample *sample,int *written) {
    const uint64_t others[]={base+0xd000,base+0xe000,base+0xf000};
    return jc_anim_apply(reader,writer,NULL,image,base,others,layout,sample,written);
}
static int arm(struct jc_anim_bind_detail *detail) {
    const uint64_t meshes[]={base+0xd000,base+0xe000,base,base+0xf000};
    return jc_anim_bind_arm(reader,stamp_writer,NULL,image,meshes,2,base+0x4000,base+0x6000,detail);
}
int main(void) {
    struct jc_anim_bind_detail binding;
    static unsigned char snapshot[sizeof(memory)];
    fixture(); memcpy(snapshot,memory,sizeof(memory)); assert(!arm(&binding));
    assert(binding.changed && binding.previous_stamp==42 && binding.armed_stamp==41 && writes==1 && global_frame==42);
    memcpy(snapshot+0x3010,&binding.armed_stamp,4); assert(!memcmp(snapshot,memory,sizeof(memory)));
    unsigned arm_prewrite_reads=first_write_reads;
    assert(jc_anim_bind_complete(reader,NULL,image,base,&binding)==8 && binding.field==9 && binding.actual==41);
    put(0x3010,42,4); assert(!jc_anim_bind_complete(reader,NULL,image,base,&binding) && binding.observed_stamp==42);
    fixture(); put(0x3010,41,4); assert(!arm(&binding) && !binding.changed && !writes);
    fixture(); global_frame=0; put(0x3010,0,4); assert(!arm(&binding) && binding.armed_stamp==UINT32_MAX && writes==1);
    fixture(); put(0x17018,base+0x3000,8); assert(arm(&binding)==6 && binding.field==3 && !writes);
    fixture(); put(0x3000,base+0x4100,8); assert(arm(&binding)==6 && binding.field==4 && !writes);
    fixture(); mutate_address=base+0x3010; assert(arm(&binding)==3 && binding.field==6 && !writes);
    fixture(); mutate_address=base+0x17018; assert(arm(&binding)==4 && binding.field==6 && !writes);
    fixture(); change_frame_read=2; assert(arm(&binding)==3 && binding.field==6 && !writes);
    fixture(); write_fail=1; assert(arm(&binding)==7 && binding.field==7 && writes==1);
    fixture(); change_frame_read=3; assert(arm(&binding)==3 && binding.field==8 && writes==1);
    for (unsigned limit=1;limit<arm_prewrite_reads;++limit) {
        fixture(); memcpy(snapshot,memory,sizeof(memory)); read_limit=limit;
        assert(arm(&binding) && !writes && !memcmp(snapshot,memory,sizeof(memory)));
    }
    const float glide_range[]={.04166667f,1},spin_range[]={.04166667f,.6666667f};
    assert(jc_anim_clock_valid(.004938841f,-1,glide_range));
    assert(jc_anim_clock_valid(0,-1,spin_range));
    assert(!jc_anim_clock_valid(-.1f,-1,glide_range));
    assert(!jc_anim_clock_valid(1,-1,glide_range));
    assert(jc_anim_clock_valid(4,4,glide_range));
    assert(!jc_anim_clock_valid(INFINITY,-1,glide_range));
    assert(!jc_anim_clock_valid(0,-1,(float[]){1,1}));
    assert(!jc_anim_clock_valid(0,-1,(float[]){NAN,1}));
    struct jc_anim_layout layout={0}; struct jc_anim_sample sample,zero={0}; int written;
    fixture();assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
    assert(sample.total==2 && sample.count==2 && sample.clips[1].frame==2 && reads==4);
    assert(sample.clips[0].duration==-1 && sample.clips[1].duration==-1 && !layout.field);
    uint64_t schema=sample.schema;
    sample.clips[1].additive=1; sample.clips[0].frame=4;
    assert(!apply(&layout,&sample,&written) && written && writes==2);
    assert(memory[0x7000]==1 && memory[0x7052]==1);
    float duration; memcpy(&duration,memory+0x7008,4); assert(duration==-1);
    assert(!jc_anim_consumed(reader,NULL,&layout,&sample));
    assert(!jc_anim_consumed_detail(reader,NULL,&layout,&sample,&binding) && binding.field==12 && !binding.row);
    memory[0x7000]=memory[0x7050]=0;
    assert(jc_anim_consumed(reader,NULL,&layout,&sample));
    number(0x7058,2); assert(!jc_anim_consumed_detail(reader,NULL,&layout,&sample,&binding) && binding.field==17 && binding.row==1);
    number(0x7058,-1);
    number(0x705c,1); assert(!jc_anim_consumed_detail(reader,NULL,&layout,&sample,&binding) && binding.field==18 && binding.row==1);
    number(0x705c,0);
    number(0x7004,5);assert(!jc_anim_consumed(reader,NULL,&layout,&sample));number(0x7004,4);
    float frame;memcpy(&frame,memory+0x7004,4);assert(frame==4);
    assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample) && sample.schema==schema);
    sample.count=0; assert(!apply(&layout,&sample,&written) && written);
    assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample) && !sample.count);
    for (unsigned mode=0;mode<9;++mode) {
        fixture();layout=(struct jc_anim_layout){0};assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
        switch (mode) {
            case 0:sample.schema^=1;break;
            case 1:put(0xd578,base+0x7000,8);break;
            case 2:put(0x3058,1,8);break;
            case 3:put(0xc008,base+0x8800,8);break;
            case 4:sample.clips[0].frame=11;break;
            case 5:put(0xa028,1,8);break;
            case 6:fail_address=base+0x9124;break;
            case 7:mutate_address=base+0x7004;break;
            case 8:write_fail=1;break;
        }
        int code=apply(&layout,&sample,&written);
        const int expected[]={6,6,5,6,4,4,1,3,7}; assert(code==expected[mode]);
        assert(mode==8 ? written && writes==1 : !written && !writes);
    }
    fixture();layout=(struct jc_anim_layout){0};put(0x580,513,8);
    assert(jc_anim_capture(reader,NULL,image,base,&layout,&sample)==4 && !memcmp(&sample,&zero,sizeof(zero)));
    assert(layout.field==1 && layout.value==513);
    const float bad_durations[]={-2,-.5f,-INFINITY,INFINITY,NAN,1000001};
    for (unsigned i=0;i<sizeof(bad_durations)/sizeof(bad_durations[0]);++i) {
        fixture();layout=(struct jc_anim_layout){0};number(0x7058,bad_durations[i]);
        assert(jc_anim_capture(reader,NULL,image,base,&layout,&sample)==4 && !sample.total);
        uint32_t bits; memcpy(&bits,bad_durations+i,4);
        assert(layout.field==8 && layout.row==1 && layout.value==bits);
    }
    const float valid_durations[]={-1,0,.5f,1000000};
    for (unsigned i=0;i<sizeof(valid_durations)/sizeof(valid_durations[0]);++i) {
        fixture();layout=(struct jc_anim_layout){0};number(0x7008,valid_durations[i]);
        assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
        assert(!layout.field && sample.clips[0].duration==valid_durations[i]);
        assert(!apply(&layout,&sample,&written));
    }
    fixture();layout=(struct jc_anim_layout){0};put(0x7014,666,4);
    assert(jc_anim_capture(reader,NULL,image,base,&layout,&sample)==4 && !sample.total);
    assert(layout.field==2 && layout.row==0 && layout.value==666 && !writes);
    fixture();layout=(struct jc_anim_layout){0};number(0x7004,NAN);
    assert(jc_anim_capture(reader,NULL,image,base,&layout,&sample)==4 && !sample.total);
    fixture();layout=(struct jc_anim_layout){0};put(0x7030,128,8);
    assert(jc_anim_capture(reader,NULL,image,base,&layout,&sample)==4 && !sample.total);
    fixture();layout=(struct jc_anim_layout){0};assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
    sample.count=2;sample.clips[1].index=0;assert(!jc_anim_valid(&sample));
    fixture();layout=(struct jc_anim_layout){0};assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
    put(0xc000,245,8);put(0xc008,base+0x12000,8);
    put(0x12000+243*8,base+0x9000,8);put(0x12000+244*8,base+0x9040,8);
    reads=0;assert(!apply(&layout,&sample,&written));
    unsigned batched_reads=reads,prewrite_reads=first_write_reads;
    assert(batched_reads<60 && writes==2);
    printf("Animation collection: 245 descriptors, two active clips: %u reads (scalar scan would use %u).\n",batched_reads,batched_reads+2*244);
    static unsigned char before[sizeof(memory)];
    for (unsigned limit=1;limit<prewrite_reads;++limit) {
        fixture();layout=(struct jc_anim_layout){0};assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
        assert(!apply(&layout,&sample,&written));
        memory[0x7000]=memory[0x7050]=0;
        sample.clips[0].frame=3;
        put(0xc000,245,8);put(0xc008,base+0x12000,8);
        put(0x12000+243*8,base+0x9000,8);put(0x12000+244*8,base+0x9040,8);
        memcpy(before,memory,sizeof(memory));reads=writes=0;read_limit=limit;
        assert(apply(&layout,&sample,&written) && !written && !writes);
        assert(!memcmp(before,memory,sizeof(memory)));
        read_limit=0;reads=0;assert(!apply(&layout,&sample,&written) && written);
    }
    fixture();layout=(struct jc_anim_layout){0};assert(!jc_anim_capture(reader,NULL,image,base,&layout,&sample));
    put(0xc000,245,8);put(0xc008,base+0x12000,8);
    put(0x12000,base+0x9000,8);put(0x12008,base+0x9040,8);
    fail_address=base+0x12000+244*8;
    assert(apply(&layout,&sample,&written)==1 && !written && !writes);
    puts("Animation batching: unreadable collection tail rejected; every prewrite read-budget interruption preserves active state and retries successfully.");
    puts("Binding stamp: same-frame rearm changes only owned payload stamp; fresh frames need no write; rollover, alias, mutation, read budget and write failure rejected; native consumption reports exact row/field.");
    puts("Native animation: named schema, exact clip times, additive routing, zero-weight clearing; alias, capacity, descriptor, time, unreadable/mutating storage and write failure checks passed. No native calls.");
    return 0;
}
