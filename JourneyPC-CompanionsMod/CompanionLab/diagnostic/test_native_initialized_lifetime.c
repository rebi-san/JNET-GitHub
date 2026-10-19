#include "native_dual_body.h"
#include "native_render_queue.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x800000];
static const uint64_t image=UINT64_C(0x140000000),object=0x200000,barn=0x100000,game=0x11000;
static int cleaned,destroyed,bad_cleanup,reenter,denied,damage_pool,damage_destroy;
static uint64_t fail_at;
static unsigned read_count;
static struct jc_initialized_lifetime life;
static struct jc_mesh_ops ops;
static void put(uint64_t at,uint64_t value) { assert(at+8<=sizeof(memory)); memcpy(memory+at,&value,8); }
static int reader(void *ctx,uint64_t at,void *out,size_t size) {
    (void)ctx;
    ++read_count;
    if (denied || (fail_at>=at && fail_at-at<size) || at>sizeof(memory) || size>sizeof(memory)-at) return 0;
    memcpy(out,memory+at,size); return 1;
}
static void cleanup(void *ctx,uint64_t obj) {
    (void)ctx; assert(obj==object && life.state==3 && !life.object && life.cleanup_stage==2); ++cleaned;
    if (reenter) assert(jc_initialized_cleanup(&life,&ops,image,game,7,barn+0xb0)==3);
    put(obj+0x168,0); put(obj+0x190,0); put(obj+0x6d8,0); put(obj+0x6e0,0);
    put(0x630060,0);
    if (damage_pool) put(0x630000,0x765000);
    if (bad_cleanup) put(obj+0x190,0x300000);
}
static void destroy(void *ctx,uint64_t obj) { (void)ctx; assert(obj==object && cleaned==1 && life.state==3 && life.cleanup_stage==4); ++destroyed; if (damage_destroy) put(0x630000,0x765000); }
static void fixture(void) {
    memset(memory,0,sizeof(memory)); cleaned=destroyed=bad_cleanup=reenter=denied=damage_pool=damage_destroy=0;
    fail_at=0; read_count=0;
    ops=(struct jc_mesh_ops){{reader,NULL,destroy,NULL},NULL,cleanup};
    put(game+0x1e0,barn);
    put(object,image+0x5d0380); put(object+0x160,image+0x67f3f8);
    put(object+0x7b0,image+0x67f3f8); put(object+0xe50,image+0x67f3f8); put(object+0xa390,image+0x5d0360);
    put(object+0x190,0x300000); put(0x300010,0x400000); put(0x400018,0x500000);
    put(0x500000,0x700000); put(0x500008,0x710000);
    put(object+0x160+0x518,0x700000); put(object+0x160+0x520,0x710000);
    put(object+0x6d8,0x600000); put(object+0x6e0,245);
    put(object+0x160+0x528,0x720000); put(object+0x160+0x4e8,0x730000); put(object+0x168,0x740000);
    put(object+0x160+0x90,0x610000); put(object+0x160+0xb0,0x630060);
    put(0x610010,0x620000); put(0x620180,0x630000); put(0x620188,3);
    put(0x630000,0x750000); put(0x630030,barn+0x14900); put(0x630060,object+0x160+0xb0);
    put(barn+0x14900-0x20,0x610000); put(barn+0x14900,0x630030);
    life=(struct jc_initialized_lifetime){.object=object,.game=game,.barn=barn,.thread=7,.state=1};
    assert(!jc_initialized_owner_read(reader,NULL,image,object,&life.owner));
}
static void boundary_fixture(void) {
    fixture(); life.head_kinds[0]=2; put(0x630000,0);
}
static int finish(void) { return jc_initialized_cleanup(&life,&ops,image,game,7,barn+0xb0); }
struct test_budget { unsigned remaining; int expired,read_failed; };
static int budget_reader(void *ctx,uint64_t at,void *out,size_t size) {
    struct test_budget *budget=ctx;
    if (!budget->remaining) { budget->expired=1; return 0; }
    --budget->remaining;
    int ok=reader(NULL,at,out,size);
    if (!ok) budget->read_failed=1;
    return ok;
}
static int budget_check(struct test_budget *budget,struct jc_initialized_detail *detail) {
    int code=jc_initialized_validate_detail(&life,budget_reader,budget,image,game,7,detail);
    return jc_render_budget_result(code,budget->expired,budget->read_failed,1);
}
static void retry_tests(void) {
    fixture(); read_count=0; assert(!jc_initialized_validate(&life,reader,NULL,image,game,7));
    unsigned full_reads=read_count;
    for (unsigned limit=0;limit<full_reads;++limit) {
        fixture(); struct jc_initialized_lifetime prior=life;
        struct test_budget budget={.remaining=limit}; struct jc_initialized_detail detail;
        struct jc_render_budget_retry retry={0};
        int code=budget_check(&budget,&detail);
        assert(code==9 && budget.expired && !budget.read_failed && detail.field!=JC_IF_NONE);
        assert(jc_render_budget_defer(&retry,code,1279,0,0));
        assert(!memcmp(&life,&prior,sizeof(life)) && !cleaned && !destroyed);
        assert(!jc_initialized_validate_detail(&life,reader,NULL,image,game,7,&detail));
        jc_render_budget_drawn(&retry); assert(retry.recovered==1 && !retry.consecutive);
    }
    fixture(); struct test_budget budget={.remaining=2}; struct jc_initialized_detail detail;
    struct jc_render_budget_retry retry={0};
    assert(jc_render_budget_defer(&retry,budget_check(&budget,&detail),1279,0,0));
    put(object+0x6d8,0x600010); budget=(struct test_budget){.remaining=100};
    int code=budget_check(&budget,&detail);
    assert(code==JC_LAYOUT_CHANGED && detail.field==JC_IF_ENTRIES);
    assert(!jc_render_budget_defer(&retry,code,1279,0,0) && !cleaned && !destroyed);
    fixture(); put(0x630030,0); budget=(struct test_budget){.remaining=100};
    code=budget_check(&budget,&detail);
    assert(code==JC_LAYOUT_CHANGED && detail.field==JC_IF_POOL && detail.changed_mask==2);
    assert(!jc_render_budget_defer(&retry,code,1279,0,0));
    fixture(); fail_at=object+0x160+0x520+3; budget=(struct test_budget){.remaining=100};
    code=budget_check(&budget,&detail);
    assert(code==JC_LAYOUT_READ && budget.read_failed && !budget.expired);
    assert(!jc_render_budget_defer(&retry,code,1279,0,0));
    puts("Ownership retry: every truncated read budget recovers with fresh validation; changed entries/pool and real read failures remain fatal.");
}
int main(void) {
    retry_tests();
    fixture(); read_count=0; assert(!jc_initialized_validate(&life,reader,NULL,image,game,7));
    printf("Ownership batches: %u reads for the three-slot fixture.\n",read_count);
    fixture(); fail_at=object+0x160+0x520+3;
    assert(jc_initialized_validate(&life,reader,NULL,image,game,7)==JC_LAYOUT_READ);
    fixture(); fail_at=object+0x160+0x580+3;
    assert(jc_initialized_validate(&life,reader,NULL,image,game,7)==JC_LAYOUT_READ);
    fixture(); assert(!jc_initialized_validate(&life,reader,NULL,image,game,7));
    assert(finish()==2 && cleaned==1 && destroyed==1); assert(finish()==2 && cleaned==1 && destroyed==1);
    fixture(); reenter=1; assert(finish()==2 && cleaned==1 && destroyed==1);
    fixture(); assert(jc_initialized_cleanup(&life,&ops,image,game,7,barn+0x146f0)==1 && !cleaned);
    fixture(); assert(jc_initialized_cleanup(&life,&ops,image,game,8,barn+0xb0)==-1 && !cleaned && !destroyed);
    fixture(); assert(jc_initialized_cleanup(&life,&ops,image,game+16,7,barn+0xb0)==-1 && !cleaned);
    fixture(); put(game+0x1e0,barn+16); assert(finish()==-1 && !cleaned);
    fixture(); put(object+0x6d8,0x600010); assert(finish()==-1 && !cleaned);
    fixture(); put(object+0x160+0x520,0x710010); assert(finish()==-1 && !cleaned);
    fixture(); put(0x630060,0x750000); assert(finish()==-1 && !cleaned);
    fixture(); put(object+0x6e0,4097); assert(finish()==-1 && !cleaned);
    fixture(); denied=1; assert(finish()==-1 && !cleaned);
    fixture(); bad_cleanup=1; assert(finish()==-1 && cleaned==1 && !destroyed);
    fixture(); put(object,0); assert(finish()==-1 && !cleaned);
    fixture(); put(0x630030,0); assert(finish()==-1 && !cleaned);
    fixture(); assert(finish()==2 && life.cleanup_stage==5 && !life.failure_code && life.detail.field==JC_IF_NONE);
    assert(life.detail.pool_valid && life.detail.slots==3 && life.detail.occupied==3 && !life.detail.free_slots);
    fixture(); put(object+0x6d8,0x600010); assert(finish()==-1 && life.cleanup_stage==1 && life.failure_code==JC_LAYOUT_CHANGED && life.detail.field==JC_IF_ENTRIES && !cleaned && !destroyed);
    fixture(); put(object+0x160+0x520,0x710010); assert(finish()==-1 && life.failure_code==JC_LAYOUT_TYPE && life.detail.field==JC_IF_CONTROL && !cleaned);
    fixture(); put(object+0x6e0,4097); assert(finish()==-1 && life.failure_code==JC_LAYOUT_RANGE && life.detail.field==JC_IF_COUNT && !cleaned);
    fixture(); denied=1; assert(finish()==-1 && life.failure_code==JC_LAYOUT_READ && life.detail.field==JC_IF_BARN && !life.detail.pool_valid && !cleaned);
    fixture(); bad_cleanup=1; assert(finish()==-1 && life.cleanup_stage==3 && life.detail.field==JC_IF_POST_CLEANUP && cleaned==1 && !destroyed);
    fixture(); put(0x630030,0); assert(finish()==-1 && life.detail.field==JC_IF_POOL && life.detail.pool_valid && life.detail.slots==3 && life.detail.occupied==2 && life.detail.free_slots==1 && !cleaned);
    fixture(); put(object+0x160+0x4e8,0x730010); assert(finish()==-1 && life.detail.field==JC_IF_CACHE && !cleaned);
    fixture(); put(object+0x160+0x528,0x720010); assert(finish()==-1 && life.detail.field==JC_IF_MANAGER && !cleaned);
    fixture(); put(object+0x168,0x740010); assert(finish()==-1 && life.detail.field==JC_IF_HEAD && !cleaned);
    fixture(); assert(jc_initialized_cleanup(&life,&ops,image,game,8,barn+0xb0)==-1 && life.detail.field==JC_IF_THREAD && life.cleanup_stage==1 && !cleaned);
    fixture(); assert(jc_initialized_cleanup(&life,&ops,image,game+16,7,barn+0xb0)==-1 && life.detail.field==JC_IF_GAME && !cleaned);
    fixture(); put(0x630000,barn+0x2c0); put(0x630030,barn+0x14900);
    assert(!jc_initialized_owner_read(reader,NULL,image,object,&life.owner));
    put(0x630030,0); assert(finish()==-1 && !cleaned && !destroyed);
    assert(life.detail.comparison_valid && life.detail.same_pool && life.detail.changed_mask==2);
    assert(life.detail.before[0]==1 && life.detail.before[1]==2 && life.detail.before[2]==3);
    assert(life.detail.after[0]==1 && life.detail.after[1]==0 && life.detail.after[2]==3);
    fixture(); put(0x630000,0); assert(finish()==-1 && life.detail.changed_mask==1 && life.detail.before[0]==4 && life.detail.after[0]==0);
    fixture(); put(0x630060,0); assert(finish()==-1 && !life.detail.comparison_valid && !cleaned);
    fixture(); put(0x620180,0x640000); put(object+0x160+0xb0,0x640060);
    put(0x640000,0x750000); put(0x640030,0x760000); put(0x640060,object+0x210);
    assert(finish()==-1 && life.detail.comparison_valid && !life.detail.same_pool && !cleaned);
    fixture(); denied=1; assert(finish()==-1 && !life.detail.comparison_valid);
    fixture(); assert(finish()==2 && life.detail.comparison_valid && life.detail.same_pool && !life.detail.changed_mask);
    fixture(); put(0x630000,0); assert(finish()==-1 && !cleaned && life.detail.remaining_valid && !life.detail.remaining_code);
    fixture(); put(0x630000,0); put(object+0x6d8,0x600010);
    assert(finish()==-1 && !cleaned && life.detail.field==JC_IF_POOL && life.detail.remaining_valid && life.detail.remaining_field==JC_IF_ENTRIES);
    fixture(); denied=1; assert(finish()==-1 && !life.detail.remaining_valid);
    assert(jc_initialized_head_label(barn+0x910,barn,object)==5);
    assert(jc_initialized_head_label(barn+0xfb0,barn,object)==6);
    assert(jc_initialized_head_label(barn+0x14f50,barn,object)==7);
    assert(jc_initialized_head_label(barn+0x155f0,barn,object)==8);
    fixture(); uint64_t prop=life.owner.pool_heads[0]-0x30;
    put(prop,image+0x680970); put(prop+8,image+0x680858);
    put(prop+0x10,life.owner.source); put(prop+0x30,life.owner.pool_table);
    assert(jc_initialized_head_kind(reader,NULL,image,&life.owner,0)==1);
    put(prop-0x80,image+0x67f3f8);
    assert(jc_initialized_head_kind(reader,NULL,image,&life.owner,0)==2);
    put(prop+0x30,life.owner.pool_table+0x30);
    assert(jc_initialized_head_kind(reader,NULL,image,&life.owner,0)==4);
    put(prop,0); assert(jc_initialized_head_kind(reader,NULL,image,&life.owner,0)==3);
    denied=1; assert(jc_initialized_head_kind(reader,NULL,image,&life.owner,0)==5);
    boundary_fixture(); assert(finish()==2 && cleaned==1 && destroyed==1 && life.pool_released && life.released_free==2);
    assert(finish()==2 && destroyed==1);
    boundary_fixture(); assert(jc_initialized_validate(&life,reader,NULL,image,game,7)==JC_LAYOUT_CHANGED && !cleaned);
    boundary_fixture(); put(object+0x6d8,0x600010); assert(finish()==-1 && !cleaned && !destroyed);
    boundary_fixture(); life.head_kinds[0]=1; assert(finish()==-1 && !cleaned);
    boundary_fixture(); put(0x630030,0); assert(finish()==-1 && !cleaned);
    boundary_fixture(); put(0x630060,0); assert(finish()==-1 && !cleaned);
    boundary_fixture(); put(0x630000,0x765000); assert(finish()==-1 && !cleaned);
    boundary_fixture(); damage_pool=1; assert(finish()==-1 && cleaned==1 && !destroyed && life.failure_code==12);
    boundary_fixture(); damage_destroy=1; assert(finish()==-1 && cleaned==1 && destroyed==1 && life.failure_code==12);
    boundary_fixture(); put(0x620180,0x640000); put(object+0x210,0x640060);
    put(0x640030,barn+0x14900); put(0x640060,object+0x210);
    assert(finish()==-1 && !cleaned);
    fixture(); put(0x620188,4); put(0x630090,0);
    struct jc_initialized_owner dual_owner; struct jc_initialized_detail dual_detail;
    assert(!jc_initialized_owner_read_group(reader,NULL,image,object,&dual_owner,&dual_detail,4,3));
    assert(dual_detail.slots==4 && dual_detail.occupied==3 && dual_detail.free_slots==1);
    assert(dual_owner.expected_slots==4 && dual_owner.expected_occupied==3 && !dual_owner.pool_heads[3]);
    assert(jc_initialized_owner_read_group(reader,NULL,image,object,&dual_owner,&dual_detail,4,4)==JC_LAYOUT_CHANGED);
    put(0x630090,0x780000);
    assert(!jc_initialized_owner_read_group(reader,NULL,image,object,&dual_owner,&dual_detail,4,4));
    life.owner=dual_owner;
    assert(!jc_initialized_validate_detail(&life,reader,NULL,image,game,7,&dual_detail));
    assert(dual_detail.comparison_valid && !dual_detail.changed_mask && dual_detail.occupied==4);
    put(0x630090,0); assert(jc_initialized_validate(&life,reader,NULL,image,game,7)==JC_LAYOUT_CHANGED);
    put(0x630090,0x780000); fail_at=0x630090;
    assert(jc_initialized_validate(&life,reader,NULL,image,game,7)==JC_LAYOUT_READ); fail_at=0;
    assert(jc_initialized_owner_read_group(reader,NULL,image,object,&dual_owner,&dual_detail,5,4)==JC_LAYOUT_RANGE);
    assert(jc_initialized_owner_read_group(reader,NULL,image,object,&dual_owner,&dual_detail,4,5)==JC_LAYOUT_RANGE);
    fixture(); put(0x700100,0x760000);
    struct jc_dual_pose_snapshot sibling_before,sibling_after;
    assert(!jc_dual_pose_snapshot_read(reader,NULL,image,object,&sibling_before));
    assert(!jc_dual_pose_snapshot_read(reader,NULL,image,object,&sibling_after));
    assert(jc_dual_pose_same(&sibling_before,&sibling_after));
    put(0x760020,1);
    assert(!jc_dual_pose_snapshot_read(reader,NULL,image,object,&sibling_after));
    assert(!jc_dual_pose_same(&sibling_before,&sibling_after));
    fail_at=0x760020;
    assert(jc_dual_pose_snapshot_read(reader,NULL,image,object,&sibling_after)==JC_LAYOUT_READ);
    puts("Initialized lifetime: legacy cleanup and four-slot partial/full ownership, missing fourth slot and read faults passed; native calls simulated.");
}
