#include "native_mesh_guard.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(jc_mesh_guard(0,0x100000,0,0,0,0,0,0)==0);
    assert(jc_mesh_guard(0x100000,0x200000,0,0,0,0,0,0)==0);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x260eb2,0,0,0,0)==1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263076,0x300000,245,1,0)==2);
    assert(jc_mesh_guard(0x100000,0x100000,0,0x260eb2,0,0,0,0)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x260eb2,0,0,1,0)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x260eb2,0x300000,0,0,0)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263076,0x300000,245,0,0)==2);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263076,0x300000,245,1,1)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263076,0,245,1,0)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263076,0x300001,245,1,0)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263076,0x300000,4097,1,0)==-1);
    assert(jc_mesh_guard(0x100000,0x100000,1,0x263077,0x300000,245,1,0)==-1);
    for (int growth=0;growth<2;++growth) {
        int deferred=0,forwarded=0,failure=-1;
        if (growth) {
            assert(jc_mesh_guard_detail(0x100000,0x100000,1,0x260eb2,0,0,
                deferred,forwarded,&failure)==1 && !failure);
            deferred=1;
        }
        assert(!jc_mesh_guard_complete(deferred,forwarded));
        assert(jc_mesh_guard_detail(0x100000,0x100000,1,0x263076,0x300000,245,
            deferred,forwarded,&failure)==2 && !failure);
        forwarded=1;
        assert(jc_mesh_guard_complete(deferred,forwarded));
        assert(jc_mesh_guard_detail(0x100000,0x100000,1,0x263076,0x300000,245,
            deferred,forwarded,&failure)==-1 && failure==JC_MESH_GUARD_STATE);
        assert(jc_mesh_guard_detail(0x100000,0x100000,1,0x260eb2,0,0,
            deferred,forwarded,&failure)==-1 && failure==JC_MESH_GUARD_STATE);
    }
    int failure;
    assert(jc_mesh_guard_detail(1,1,0,0x263076,0x300000,245,0,0,&failure)==-1 && failure==JC_MESH_GUARD_CONTEXT);
    assert(jc_mesh_guard_detail(1,1,1,0x263076,0,245,0,0,&failure)==-1 && failure==JC_MESH_GUARD_READY);
    assert(jc_mesh_guard_detail(1,1,1,0x263076,0x300001,245,0,0,&failure)==-1 && failure==JC_MESH_GUARD_READY);
    assert(jc_mesh_guard_detail(1,1,1,0x263076,0x300000,0,0,0,&failure)==-1 && failure==JC_MESH_GUARD_READY);
    assert(jc_mesh_guard_detail(1,1,1,0x263076,0x300000,4097,0,0,&failure)==-1 && failure==JC_MESH_GUARD_READY);
    assert(jc_mesh_guard_detail(1,1,1,0x263077,0x300000,245,0,0,&failure)==-1 && failure==JC_MESH_GUARD_CALLER);
    assert(jc_mesh_guard_detail(1,1,1,0x260eb2,0,0,1,0,&failure)==-1 && failure==JC_MESH_GUARD_EARLY);
    assert(jc_mesh_guard_detail(1,1,1,0x263076,0x300000,245,-1,0,&failure)==-1 && failure==JC_MESH_GUARD_STATE);
    assert(!jc_mesh_guard_complete(2,1) && !jc_mesh_guard_complete(-1,1) && !jc_mesh_guard_complete(0,2));
    puts("Mesh guard: growth and free-slot reuse lifecycles, duplicate/context/caller/range rejection passed; no native game code executed.");
}
