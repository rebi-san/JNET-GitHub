#include "native_lifecycle_trace.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    const uint64_t g=0x100000,b=0x200000;
    struct jc_lifecycle_jobs jobs={0};
    uint64_t args[]={0x555000,b+0x14850,0,0};
    assert(jc_lifecycle_animation(&jobs,18,args,7,b)==0);
    assert(jc_lifecycle_animation(&jobs,17,args,7,b)==1);
    assert(jc_lifecycle_animation(&jobs,17,args,7,b)==-1);
    assert(jc_lifecycle_animation(&jobs,18,args,8,b)==0);
    assert(jc_lifecycle_animation(&jobs,18,args,7,b)==1);
    assert(jc_lifecycle_animation(&jobs,18,args,7,b)==0);
    args[1]=b+0x777;
    assert(jc_lifecycle_animation(&jobs,17,args,7,b)==0);
    args[1]=b+0x210;
    for (unsigned i=0;i<32;++i) { args[0]=0x5000+i; assert(jc_lifecycle_animation(&jobs,17,args,7,b)==1); }
    args[0]=0x9999;
    assert(jc_lifecycle_animation(&jobs,17,args,7,b)==-1);
    assert(jc_lifecycle_role(0,g,b)==0);
    assert(jc_lifecycle_role(g,g,b)==4);
    assert(jc_lifecycle_role(b,g,b)==3);
    assert(jc_lifecycle_role(b+0xb0,g,b)==1);
    assert(jc_lifecycle_role(b+0x146f0,g,b)==2);
    assert(jc_lifecycle_role(b+0x210,g,b)==5);
    assert(jc_lifecycle_role(b+0x14850,g,b)==6);
    assert(jc_lifecycle_role(b+0x40,g,b)==7);
    assert(jc_lifecycle_role(b+0x90,g,b)==8);
    assert(jc_lifecycle_role(b+0xb0,g,0)==0);
    assert(jc_lifecycle_first(0)==1);
    assert(jc_lifecycle_first(JC_LIFECYCLE_CAP)==1);
    assert(jc_lifecycle_first(JC_LIFECYCLE_CAP+7)==8);
    struct jc_lifecycle_event a={.sequence=42,.frame=9},c=a;
    assert(jc_lifecycle_stable(&a,&c,42));
    assert(!jc_lifecycle_stable(&a,&c,41));
    c.sequence=0; assert(!jc_lifecycle_stable(&a,&c,42));
    c=a; c.frame++; assert(!jc_lifecycle_stable(&a,&c,42));
    c=a; c.args[3]++; assert(!jc_lifecycle_stable(&a,&c,42));
    c=a; c.sequence+=JC_LIFECYCLE_CAP; assert(!jc_lifecycle_stable(&a,&c,42));
    puts("native lifecycle: role identities, wrap/overrun and torn/reused-slot rejection passed");
    return 0;
}
