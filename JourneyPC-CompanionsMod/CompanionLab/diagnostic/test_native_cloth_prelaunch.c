#include "native_cloth_prelaunch.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct fixture { unsigned char *barn; unsigned writes,fail,persistent; };
static int read_memory(void *context,uint64_t address,void *out,size_t size) {
    struct fixture *f=context;
    if (address<0x10000000 || address-0x10000000>0x6bd080 ||
        size>0x6bd080-(address-0x10000000)) return 0;
    memcpy(out,f->barn+(size_t)(address-0x10000000),size);return 1;
}
static int write_memory(void *context,uint64_t address,const void *in,size_t size) {
    struct fixture *f=context;
    assert(address>=0x10000000 && address-0x10000000+size<=0x6bd080);
    ++f->writes;
    if (f->fail && (f->writes==f->fail || (f->persistent && f->writes>=f->fail))) {
        memcpy(f->barn+(size_t)(address-0x10000000),in,3);return 0;
    }
    memcpy(f->barn+(size_t)(address-0x10000000),in,size);return 1;
}
static void u64(unsigned char *p,uint64_t value) { memcpy(p,&value,8); }
static void u32(unsigned char *p,uint32_t value) { memcpy(p,&value,4); }
static void prepare(struct fixture *f,struct jc_cloth_stream streams[7]) {
    memset(f->barn,0,0x6bd080);f->writes=f->fail=f->persistent=0;
    memset(streams,0,7*sizeof(*streams));
    for (unsigned r=4;r<=5;++r) {
        uint64_t slot=0x10600000+r*0x100,geometry=0x10610000+r*0x100,vertex=0x10620000+r*0x100;
        u64(f->barn+0x666e40+r*0x630,slot);
        u64(f->barn+(slot-0x10000000)+8,geometry);
        u64(f->barn+(geometry-0x10000000)+0x28,vertex);
        f->barn[(vertex-0x10000000)+0x1c]=2;u32(f->barn+(vertex-0x10000000)+0x20,3);
        f->barn[(geometry-0x10000000)+0x30+0xe]=2;u32(f->barn+(geometry-0x10000000)+0x30+0x18,3);
        for (unsigned k=0;k<2;++k) {
            uint64_t pointer=0x40000000+r*0x10000+k*0x8000;
            u64(f->barn+0x666e00+r*0x630+k*0x10,pointer);
            u32(f->barn+0x666e08+r*0x630+k*0x10,16384);
            u64(f->barn+0x669970+r*8+k*0x38,pointer);
        }
        streams[r].mapped_vertices=(void *)(uintptr_t)(0x50000000+r*0x10000);
        streams[r].mapped_indices=(void *)(uintptr_t)(0x50008000+r*0x10000);
        streams[r].vertices=64;streams[r].indices=96;
    }
}
int main(void) {
    struct fixture f={.barn=calloc(1,0x6bd080)};assert(f.barn);
    unsigned char *saved=malloc(0x6bd080);assert(saved);
    struct jc_cloth_stream streams[7];struct jc_cloth_prelaunch s;
    prepare(&f,streams);memcpy(saved,f.barn,0x6bd080);
    assert(!jc_cloth_prelaunch_capture(read_memory,&f,0x10000000,0x30,streams,&s));
    assert(s.count==8 && !jc_cloth_prelaunch_apply(read_memory,write_memory,&f,&s));
    assert(s.applied==1 && f.writes==8);
    for (unsigned i=0;i<s.count;++i)
        assert(!memcmp(f.barn+s.patches[i].address-0x10000000,s.patches[i].after,s.patches[i].size));
    assert(jc_cloth_prelaunch_apply(read_memory,write_memory,&f,&s)==JC_PRELAUNCH_ARGUMENT);
    assert(jc_cloth_prelaunch_restore(read_memory,write_memory,&f,&s,s.count)==JC_PRELAUNCH_RESTORED);
    assert(!memcmp(saved,f.barn,0x6bd080));
    for (unsigned failure=1;failure<=8;++failure) {
        prepare(&f,streams);f.fail=failure;
        assert(!jc_cloth_prelaunch_capture(read_memory,&f,0x10000000,0x30,streams,&s));
        assert(jc_cloth_prelaunch_apply(read_memory,write_memory,&f,&s)==JC_PRELAUNCH_RESTORED);
        assert(!s.applied && !memcmp(saved,f.barn,0x6bd080));
    }
    prepare(&f,streams);f.fail=2;f.persistent=1;
    assert(!jc_cloth_prelaunch_capture(read_memory,&f,0x10000000,0x30,streams,&s));
    assert(jc_cloth_prelaunch_apply(read_memory,write_memory,&f,&s)==JC_PRELAUNCH_QUARANTINED && s.applied==2);
    prepare(&f,streams);
    assert(!jc_cloth_prelaunch_capture(read_memory,&f,0x10000000,0x30,streams,&s));
    f.barn[0x62041c]=0;
    assert(jc_cloth_prelaunch_apply(read_memory,write_memory,&f,&s)==JC_PRELAUNCH_CHANGED && !f.writes);
    prepare(&f,streams);
    assert(!jc_cloth_prelaunch_capture(read_memory,&f,0x10000000,0x30,streams,&s));
    f.barn[0x669990]^=1;
    assert(jc_cloth_prelaunch_apply(read_memory,write_memory,&f,&s)==JC_PRELAUNCH_CHANGED && !f.writes);
    assert(jc_cloth_prelaunch_capture(read_memory,&f,0x10000000,0x30,streams,&s)==JC_PRELAUNCH_CHANGED);
    free(saved);free(f.barn);
    puts("Cloth prelaunch: late output/base adoption, native ring checks, all eight partial-write rollback points, changed mappings and uncertain restoration; no game runtime");
    return 0;
}
