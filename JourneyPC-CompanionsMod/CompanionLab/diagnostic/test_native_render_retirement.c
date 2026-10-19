#include "native_render_retirement.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct jc_retirement r={0}; uint64_t a=0,b=0,unused=77;
    assert(!jc_retirement_arm(&r,1,0x1000,0x1001,&a));
    assert(jc_retirement_arm(&r,1,0x1000,0x1100,&a));
    assert(jc_retirement_result(&r,a)==0);
    assert(!jc_retirement_arm(&r,1,0x1080,0x1180,&unused) && unused==77);
    assert(jc_retirement_arm(&r,1,0x1100,0x1200,&b) && b>a);
    assert(!jc_retirement_enter(&r,2,0x1000,0x1200,9));
    assert(jc_retirement_enter(&r,1,0x1000,0x1200,9)==3);
    assert(!jc_retirement_enter(&r,1,0x1000,0x1200,9));
    assert(r.entries[0].thread==9 && jc_retirement_result(&r,a)==0);
    r.entries[0].phase=JC_RETIRE_GPU; assert(jc_retirement_result(&r,a)==0);
    uint64_t reused=0;
    assert(jc_retirement_arm(&r,1,0x1000,0x1100,&reused) && reused>b);
    assert(jc_retirement_enter(&r,1,0x1000,0x1100,9)==4);
    assert(r.entries[0].phase==JC_RETIRE_GPU && r.entries[2].phase==JC_RETIRE_EXECUTING);
    assert(!jc_retirement_arm(&r,1,0x1000,0x1100,&unused));
    r.entries[0].phase=JC_RETIRE_DONE; assert(jc_retirement_result(&r,a)==1);
    r.entries[1].phase=JC_RETIRE_FAILED; assert(jc_retirement_result(&r,b)==-1);
    assert(!jc_retirement_arm(&r,1,0x1100,0x1200,&unused));
    memset(&r,0,sizeof(r));
    assert(jc_retirement_arm(&r,1,0x1000,0x1200,&a));
    assert(!jc_retirement_enter(&r,1,0x1000,0x1100,9));
    assert(jc_retirement_result(&r,a)==-1);
    memset(&r,0,sizeof(r));
    for (unsigned i=0;i<4;++i) assert(jc_retirement_arm(&r,1,0x1000+i*256,0x1100+i*256,&a));
    assert(!jc_retirement_arm(&r,1,0x2000,0x2100,&a));
    assert(jc_retirement_result(&r,0)==-1 && jc_retirement_result(&r,100)==-1);
    puts("Render retirement: command-range ownership, containing execution, partial-range rejection, duplicate execution, pending GPU state, completion, quarantine and capacity passed; no GPU execution");
    return 0;
}
