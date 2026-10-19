#include "native_render_queue.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x10090];
static struct jc_render_workspace work;
static const uint64_t image=UINT64_C(0x140000000), queue=0x100000, model=0x200000, control=0x300000;
static int reads,deny,change;
static int reader(void *ctx,uint64_t address,void *out,size_t size) {
    (void)ctx; ++reads; assert(reads<=5);
    if (deny || address<queue || address-queue>sizeof(memory) || size>sizeof(memory)-(address-queue)) return 0;
    memcpy(out,memory+(address-queue),size);
    if (change && reads==5) { uint32_t n=2; memcpy(out,&n,4); }
    return 1;
}
static void fixture(uint32_t count) {
    memset(memory,0,sizeof(memory)); reads=deny=change=0;
    uint64_t type=image+0x6896e8;
    memcpy(memory,&type,8); memcpy(memory+0x10080,&count,4);
    memcpy(memory+0x80,&model,8); memcpy(memory+0x88,&control,8);
}
static int check(struct jc_render_snapshot *s) { return jc_render_queue(reader,NULL,image,queue,model,control,&work,s); }
static void retirement_tests(void) {
    struct jc_render_ledger ledger={.queue=queue,.model=model,.control=control,.generation=7};
    struct jc_render_retirement plan;
    struct jc_render_workspace original;
    memset(&work,0,sizeof(work));
    work.pairs[0][0]=model; work.pairs[0][1]=control;
    work.pairs[4095][0]=model; work.pairs[4095][1]=control;
    work.pairs[31][0]=0x400000; work.pairs[31][1]=0x500000;
    ledger.slots[0]=1; ledger.slots[511]=128;
    original=work;
    assert(!jc_render_plan_retirement(&ledger,queue,7,0,&work,&plan));
    assert(plan.count==2 && plan.slots[0]==0 && plan.slots[1]==4095);
    assert(!memcmp(&original,&work,sizeof(work)));
    assert(jc_render_plan_retirement(&ledger,queue,8,0,&work,&plan)==JC_RETIRE_GENERATION && !plan.count);
    assert(jc_render_plan_retirement(&ledger,queue+16,7,0,&work,&plan)==JC_RETIRE_GENERATION);
    assert(jc_render_plan_retirement(&ledger,queue,7,1,&work,&plan)==JC_RETIRE_ACTIVE);
    assert(jc_render_plan_retirement(&ledger,queue,7,4097,&work,&plan)==JC_RETIRE_INPUT);
    work.pairs[4095][1]=0x600000;
    assert(jc_render_plan_retirement(&ledger,queue,7,0,&work,&plan)==JC_RETIRE_REPLACED && !plan.count);
    work=original; ledger.slots[511]=0;
    assert(jc_render_plan_retirement(&ledger,queue,7,0,&work,&plan)==JC_RETIRE_UNTRACKED);
    ledger.slots[511]=128; work.pairs[3][1]=control;
    assert(jc_render_plan_retirement(&ledger,queue,7,0,&work,&plan)==JC_RETIRE_UNTRACKED);
    work=original; memset(work.pairs[0],0,16);
    assert(jc_render_plan_retirement(&ledger,queue,7,0,&work,&plan)==JC_RETIRE_REPLACED);
    memset(&work,0,sizeof(work)); memset(ledger.slots,0,sizeof(ledger.slots));
    assert(!jc_render_plan_retirement(&ledger,queue,7,0,&work,&plan) && !plan.count);
    assert(jc_render_plan_retirement(NULL,queue,7,0,&work,&plan)==JC_RETIRE_INPUT);
    assert(jc_render_plan_retirement(&ledger,queue,0,0,&work,&plan)==JC_RETIRE_INPUT);
    puts("render retirement planning: 12 cases passed; no native calls or writes");
}
static void tracking_tests(void) {
    struct jc_render_tracking track={0},prior;
    struct jc_render_retirement plan;
    memset(&work,0,sizeof(work));
    assert(!jc_render_track_begin(&track,queue,1,model,control,&work));
    assert(!jc_render_track_assignment(&track,queue,1,0,0,0,model,control));
    assert(track.held==1 && track.acquired==1);
    assert(!jc_render_track_assignment(&track,queue,1,0,model,control,model,control));
    assert(track.held==1 && track.acquired==1 && track.reused==1);
    assert(!jc_render_track_assignment(&track,queue,1,4095,0x400000,0x500000,model,control));
    work.pairs[0][0]=work.pairs[4095][0]=model;
    work.pairs[0][1]=work.pairs[4095][1]=control;
    assert(!jc_render_tracking_plan(&track,queue,1,0,&work,&plan) && plan.count==2);
    assert(!jc_render_track_assignment(&track,queue,1,0,model,control,0x400000,0x500000));
    assert(track.held==1 && track.released==1);
    assert(!jc_render_track_release(&track,queue,1,4095,model,control));
    assert(!track.held && track.acquired==2 && track.released==2);
    assert(jc_render_track_release(&track,queue,1,4095,model,control)==JC_TRACK_UNTRACKED);
    assert(jc_render_track_assignment(&track,queue,1,0,0,0,model,control)==JC_TRACK_FAILED);
    assert(jc_render_tracking_plan(&track,queue,1,0,&work,&plan)==JC_RETIRE_INPUT && !plan.count);
    track=(struct jc_render_tracking){0};
    assert(jc_render_track_begin(&track,queue,1,model,control,&work)==JC_TRACK_UNTRACKED);
    memset(&work,0,sizeof(work)); track=(struct jc_render_tracking){0};
    assert(!jc_render_track_begin(&track,queue,2,model,control,&work)); prior=track;
    assert(jc_render_track_assignment(&track,queue,1,0,0,0,model,control)==JC_TRACK_CONTEXT);
    track=prior; assert(jc_render_track_assignment(&track,queue+16,2,0,0,0,model,control)==JC_TRACK_CONTEXT);
    track=prior; assert(jc_render_track_assignment(&track,queue,2,4096,0,0,model,control)==JC_TRACK_INPUT);
    track=prior; assert(jc_render_track_assignment(&track,queue,2,0,0,0,model,control+16)==JC_TRACK_MISMATCH);
    track=prior; assert(jc_render_track_assignment(&track,queue,2,0,model,control,model,control)==JC_TRACK_UNTRACKED);
    track=prior; assert(jc_render_track_begin(&track,queue,3,model,control,&work)==JC_TRACK_CONTEXT);
    puts("render reference tracking: acquisition, reuse, replacement, retirement and failure scenarios passed; simulated events only");
}
static void preview_tests(void) {
    fixture(1); assert(jc_render_preview_queue(memory,model,control));
    fixture(0); assert(jc_render_preview_queue(memory,model,control));
    fixture(2); assert(!jc_render_preview_queue(memory,model,control));
    fixture(1); memcpy(memory+0x90,&model,8); assert(!jc_render_preview_queue(memory,model,control));
    fixture(1); memset(memory+0x88,0,8); assert(!jc_render_preview_queue(memory,model,control));
    puts("private preview: 5 queue cases passed; no rendering executed");
}
static void budget_retry_tests(void) {
    struct jc_render_budget_retry retry={0};
    assert(jc_render_budget_result(0,0,0,0)==0);
    assert(jc_render_budget_result(0,0,0,1)==9);
    assert(jc_render_budget_result(JC_LAYOUT_READ,1,0,1)==9);
    assert(jc_render_budget_result(JC_LAYOUT_CHANGED,1,0,1)==9);
    for (int code=1;code<=5;++code) {
        assert(jc_render_budget_result(code,0,0,1)==code);
        assert(jc_render_budget_result(code,1,1,1)==code);
        assert(!jc_render_budget_defer(&retry,code,205,0,0));
    }
    assert(!jc_render_budget_defer(&retry,9,0,0,0));
    assert(!jc_render_budget_defer(&retry,9,205,1,0));
    assert(!jc_render_budget_defer(&retry,9,205,0,1));
    assert(!retry.total && !retry.consecutive && !retry.recovered);
    for (unsigned i=1;i<8;++i) {
        assert(jc_render_budget_defer(&retry,9,205,0,0));
        assert(retry.total==i && retry.consecutive==i && !retry.recovered);
    }
    jc_render_budget_drawn(&retry);
    assert(retry.total==7 && !retry.consecutive && retry.recovered==1);
    jc_render_budget_drawn(&retry);
    assert(retry.recovered==1);
    for (unsigned i=1;i<=8;++i)
        assert(jc_render_budget_defer(&retry,9,206,0,0)==(i<8));
    assert(retry.total==15 && retry.consecutive==8 && retry.recovered==1);
    assert(!jc_render_budget_defer(&retry,JC_LAYOUT_CHANGED,206,0,0));
    puts("render budget: transient retry, recovery, eight-timeout stop, real errors and pending native work checked");
}
static void transform_point(const float m[16],const float p[4],float out[4]) {
    for (unsigned i=0;i<4;++i)
        out[i]=p[0]*m[i]+p[1]*m[4+i]+p[2]*m[8+i]+p[3]*m[12+i];
}
static void transform_tests(void) {
    const float fixtures[][16]={
        {1,0,0,0,0,1,0,0,0,0,1,0,266,71,124,1},
        {0,2,0,0,-3,0,0,0,0,0,0.5f,0,-266,71,124,1},
        {-2,0,0,0,0,0,3,0,0,4,0,0,0,0,0,1}};
    const float points[][4]={{0,0,0,1},{1,2,3,1},{-4,5,-6,1},{1,2,3,0}};
    for (unsigned f=0;f<3;++f) {
        float m[16],origin[3]; memcpy(m,fixtures[f],sizeof(m));
        assert(jc_render_preview_transform(m,origin));
        assert(!memcmp(origin,fixtures[f]+12,sizeof(origin)));
        assert(!memcmp(m,fixtures[f],12*sizeof(float)));
        assert(!memcmp(m+13,fixtures[f]+13,3*sizeof(float)));
        for (unsigned p=0;p<4;++p) {
            float before[4],after[4];
            transform_point(fixtures[f],points[p],before);
            transform_point(m,points[p],after);
            assert(after[0]-before[0]==3*points[p][3]);
            for (unsigned i=1;i<4;++i) assert(after[i]==before[i]);
        }
        float lo[4]={0,0,0,0},hi[4]={0,0,0,0}; int32_t relative[6];
        assert(!jc_render_bounds_relative(lo,hi,m,origin,relative));
        assert(relative[0]==3000 && !relative[1] && !relative[2]);
    }
    for (unsigned test=0;test<8;++test) {
        float m[16],before[16],origin[3]={-7,-8,-9},sentinel[3]={-7,-8,-9};
        memcpy(m,fixtures[0],sizeof(m));
        if (test<3) m[3+4*test]=3;
        else if (test==3) m[15]=0;
        else if (test==4) { uint32_t nan=0x7fc00000; memcpy(m+1,&nan,4); }
        else if (test==5) { uint32_t inf=0x7f800000; memcpy(m+12,&inf,4); }
        else if (test==6) m[12]=1e20f;
        else { m[3]=10; m[7]=20; m[11]=30; m[12]=m[13]=m[14]=0; }
        memcpy(before,m,sizeof(m));
        assert(!jc_render_preview_transform(m,origin));
        assert(!memcmp(m,before,sizeof(m)) && !memcmp(origin,sentinel,sizeof(origin)));
    }
    puts("mesh transform: 12 native point/direction comparisons, 3 relative origins, 8 unchanged rejections passed");
}
static unsigned char visible_memory[1024];
static int visibility_reader(void *ctx,uint64_t at,void *out,size_t size) {
    (void)ctx;
    if (at<0x100000 || at-0x100000>sizeof(visible_memory) || size>sizeof(visible_memory)-(at-0x100000)) return 0;
    memcpy(out,visible_memory+(at-0x100000),size); return 1;
}
static void vput(unsigned at,uint64_t value) { memcpy(visible_memory+at,&value,8); }
static void visibility_tests(void) {
    struct jc_render_visibility v;
    memset(visible_memory,0,sizeof(visible_memory));
    vput(0x60,0x100100); vput(0x68,0x100108); vput(0x100,0x100180); vput(0x1b8,0x100200);
    float extent[3]={1,2,3}; memcpy(visible_memory+0x218,extent,12);
    assert(!jc_render_visibility_read(visibility_reader,NULL,0x100000,1,0,&v) && v.parts==1 && v.nondegenerate==1 && v.mask_pass);
    assert(!jc_render_visibility_read(visibility_reader,NULL,0x100000,1,1,&v) && !v.mask_pass);
    memset(visible_memory+0x218,0,12);
    assert(!jc_render_visibility_read(visibility_reader,NULL,0x100000,1,0,&v) && !v.nondegenerate);
    extent[0]=-1; memcpy(visible_memory+0x218,extent,12);
    assert(jc_render_visibility_read(visibility_reader,NULL,0x100000,1,0,&v)==JC_LAYOUT_RANGE);
    vput(0x68,0x100100); assert(!jc_render_visibility_read(visibility_reader,NULL,0x100000,1,0,&v) && !v.parts);
    vput(0x68,0x101000); assert(jc_render_visibility_read(visibility_reader,NULL,0x100000,1,0,&v)==JC_LAYOUT_RANGE);
    puts("visibility: 6 model-list/filter/bounds cases passed; no rendering executed");
}
static int draw_deny,draw_changed,draw_index_reads;
static uint32_t draw_index_count;
static unsigned char draw_gate;
static uint64_t draw_context;
static int draw_reader(void *ctx,uint64_t at,void *out,size_t size) {
    (void)ctx;
    if (draw_deny) return 0;
    if (at==image+0x72c060 && size==1) memcpy(out,&draw_gate,1);
    else if (at==image+0x40408b8 && size==8) memcpy(out,&draw_context,8);
    else if (at==0x300018 && size==4) { uint32_t topology=4; memcpy(out,&topology,4); }
    else if (at==0x400008 && size==4) {
        uint32_t count=draw_index_count;
        if (++draw_index_reads==2 && draw_changed) ++count;
        memcpy(out,&count,4);
    } else return 0;
    return 1;
}
static void draw_input_tests(void) {
    unsigned char command[128]={0}; struct jc_render_draw_input input;
    uint32_t kind=2,stored=0,instances=3; uint64_t pass=0x300000,indices=0x400000;
    memcpy(command,&kind,4); memcpy(command+0x28,&pass,8); memcpy(command+0x68,&indices,8);
    memcpy(command+0x70,&stored,4); memcpy(command+0x78,&instances,4);
    draw_gate=1; draw_context=0x500000; draw_index_count=240;
    assert(!jc_render_draw_input(draw_reader,NULL,image,command,&input));
    assert(input.kind==2 && !input.stored && input.indexed && input.index_count==240 && input.instances==3);
    assert(input.gate==1 && input.context==1 && input.topology==4);
    draw_gate=0; draw_context=0; draw_index_count=0;
    assert(!jc_render_draw_input(draw_reader,NULL,image,command,&input));
    assert(!input.gate && !input.context && !input.index_count);
    kind=1; memcpy(command,&kind,4); memset(command+0x68,0,8);
    assert(!jc_render_draw_input(draw_reader,NULL,image,command,&input) && input.instances==1 && !input.indexed);
    draw_deny=1;
    assert(jc_render_draw_input(draw_reader,NULL,image,command,&input)==JC_LAYOUT_READ && !input.kind);
    draw_deny=0; memcpy(command+0x68,&indices,8); draw_changed=1; draw_index_reads=0;
    assert(jc_render_draw_input(draw_reader,NULL,image,command,&input)==JC_LAYOUT_CHANGED && !input.kind);
    kind=44; memcpy(command,&kind,4);
    assert(jc_render_draw_input(draw_reader,NULL,image,command,&input)==JC_LAYOUT_TYPE && !input.kind);
    uint64_t item=99;
    memset(visible_memory,0,sizeof(visible_memory));
    vput(0x60,0x100100); vput(0x68,0x100108); vput(0x100,0x100180);
    vput(0x1b8,0x100200); vput(0x228,0x100300);
    assert(!jc_render_first_item(visibility_reader,NULL,0x100000,&item) && item==0x100300);
    vput(0x228,0); assert(jc_render_first_item(visibility_reader,NULL,0x100000,&item)==JC_LAYOUT_TYPE && !item);
    vput(0x68,0x100110); assert(jc_render_first_item(visibility_reader,NULL,0x100000,&item)==JC_LAYOUT_RANGE && !item);
    vput(0x68,0x100108); vput(0x1b8,0x200000);
    assert(jc_render_first_item(visibility_reader,NULL,0x100000,&item)==JC_LAYOUT_READ && !item);
    puts("draw input: 6 count/gate/context/rejection cases and 4 local-item attribution cases passed; no draw calls");
}
static char level_name[32];
static int level_reads,level_change,level_deny;
static int level_reader(void *ctx,uint64_t at,void *out,size_t size) {
    (void)ctx; ++level_reads;
    if (level_deny || at!=0x100000+0x1c02f0 || size!=sizeof(level_name)) return 0;
    memcpy(out,level_name,size);
    if (level_change && level_reads==2) ((char *)out)[0]='X';
    return 1;
}
static void location_tests(void) {
    int level; int32_t result[6];
    for (int prepared=-1;prepared<=12;++prepared) {
        assert(jc_render_trial_level(prepared)==(prepared>=1 && prepared<=6));
        for (int current=-1;current<=12;++current)
            assert(jc_render_trial_same_level(prepared,current)==
                (prepared>=1 && prepared<=6 && prepared==current));
    }
    strcpy(level_name,"Level_Graveyard");
    assert(!jc_render_level(level_reader,NULL,0x100000,&level) && level==0);
    strcpy(level_name,"Level_Desert");
    assert(!jc_render_level(level_reader,NULL,0x100000,&level) && level==2);
    strcpy(level_name,"Level_Desert_extra");
    assert(jc_render_level(level_reader,NULL,0x100000,&level)==JC_LAYOUT_TYPE && level==-1);
    memset(level_name,'A',sizeof(level_name));
    assert(jc_render_level(level_reader,NULL,0x100000,&level)==JC_LAYOUT_RANGE);
    strcpy(level_name,"Level_Desert"); level_reads=0; level_change=1;
    assert(jc_render_level(level_reader,NULL,0x100000,&level)==JC_LAYOUT_CHANGED);
    level_change=0; level_deny=1;
    assert(jc_render_level(level_reader,NULL,0x100000,&level)==JC_LAYOUT_READ);
    float lo[4]={-1,0,-2,0},hi[4]={1,4,2,0},origin[3]={10,20,30};
    float m[16]={1,0,0,0,0,1,0,0,0,0,1,0,13,20,30,1};
    assert(!jc_render_bounds_relative(lo,hi,m,origin,result));
    assert(result[0]==3000 && result[1]==2000 && result[2]==0);
    assert(result[3]==2000 && result[4]==4000 && result[5]==4000);
    m[0]=0; m[1]=2; m[4]=-3; m[5]=0;
    assert(!jc_render_bounds_relative(lo,hi,m,origin,result));
    assert(result[0]==-3000 && result[1]==0 && result[3]==12000 && result[4]==4000);
    m[12]=1000;
    assert(!jc_render_bounds_relative(lo,hi,m,origin,result) && result[0]==984000);
    hi[0]=-2;
    assert(jc_render_bounds_relative(lo,hi,m,origin,result)==JC_LAYOUT_RANGE); hi[0]=1;
    uint32_t nan=0x7fc00000; memcpy(m+1,&nan,4);
    assert(jc_render_bounds_relative(lo,hi,m,origin,result)==JC_LAYOUT_RANGE); m[1]=2;
    m[12]=1e20f;
    assert(jc_render_bounds_relative(lo,hi,m,origin,result)==JC_LAYOUT_RANGE);
    puts("location: 6 level identity and 6 relative bounds cases passed; no game execution");
    puts("chapter policy: hub, unknown, excluded and cross-chapter pairs rejected; eligible same-chapter pairs accepted");
}
int main(void) {
    transform_tests();
    location_tests();
    struct jc_render_snapshot s;
    fixture(1); assert(!check(&s) && s.count==1 && s.active==1 && s.stored==1);
    fixture(0); assert(!check(&s) && !s.active && s.stored==1);
    fixture(4096); memcpy(memory+0x10070,&model,8); memcpy(memory+0x10078,&control,8);
    assert(!check(&s) && s.active==2 && s.stored==2);
    fixture(4097); assert(check(&s)==JC_LAYOUT_RANGE && !s.count);
    fixture(1); memory[0]=0; assert(check(&s)==JC_LAYOUT_TYPE);
    fixture(1); memset(memory+0x88,0,8); assert(check(&s)==JC_LAYOUT_TYPE);
    fixture(1); deny=1; assert(check(&s)==JC_LAYOUT_READ);
    fixture(1); change=1; assert(check(&s)==JC_LAYOUT_CHANGED && !s.stored);
    fixture(0); memset(memory+0x80,0,16); assert(!check(&s) && !s.stored);
    fixture(1); assert(jc_render_queue(reader,NULL,image,queue,model,control,NULL,&s)==JC_LAYOUT_RANGE);
    puts("render queue: 10 cases passed");
    retirement_tests();
    tracking_tests();
    preview_tests();
    budget_retry_tests();
    visibility_tests();
    draw_input_tests();
    uint32_t records=99;
    assert(jc_render_command_range(0x100000,0x100080,&records) && records==1);
    assert(jc_render_command_range(0x100000,0x108000,&records) && records==256);
    assert(!jc_render_command_range(0x100000,0x108080,&records) && !records);
    assert(!jc_render_command_range(0x100000,0x100000,&records));
    assert(!jc_render_command_range(0x100080,0x100000,&records));
    assert(!jc_render_command_range(0x100000,0x100081,&records));
    assert(!jc_render_command_range(0x100001,0x100081,&records));
    assert(!jc_render_command_range(0,128,&records));
    puts("command range: 8 bounded range cases passed");
    unsigned char command[128]={0}; struct jc_render_command_input input;
    uint64_t m=0x200000,p=0x300000,b=0x400000; uint32_t kind=1,count=12,instances=0;
    memcpy(command,&kind,4); memcpy(command+0x20,&m,8); memcpy(command+0x28,&p,8);
    memcpy(command+0x30,&b,8); memcpy(command+0x70,&count,4);
    assert(!jc_render_command_input(command,m,p,b,&input) && input.count==12 && input.instances==1 && !input.indexed);
    kind=2; memcpy(command,&kind,4); memcpy(command+0x78,&instances,4);
    assert(!jc_render_command_input(command,m,p,b,&input) && !input.instances);
    instances=7; memcpy(command+0x78,&instances,4); memcpy(command+0x68,&b,8);
    assert(!jc_render_command_input(command,m,p,b,&input) && input.instances==7 && input.indexed);
    assert(jc_render_command_input(command,m+8,p,b,&input)==2 && !input.count);
    assert(jc_render_command_input(command,m,p+8,b,&input)==2);
    assert(jc_render_command_input(command,m,p,b+8,&input)==2);
    kind=3; memcpy(command,&kind,4); assert(jc_render_command_input(command,m,p,b,&input)==2);
    puts("command attribution: 7 header and geometry-input cases passed");
}
