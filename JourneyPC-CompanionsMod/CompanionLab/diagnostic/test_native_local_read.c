#include "native_local_read.h"
#include <assert.h>
#include <stdio.h>
static unsigned char bytes[24*64];
static unsigned queries,copies,denied,bad;
static const uint64_t base=0x100000;
static int query(void *ctx,uint64_t address,struct jc_local_region *out) {
    (void)ctx; ++queries;
    if (address<base || address>=base+sizeof(bytes) || (denied && address>=base+64 && address<base+128)) return 0;
    uint64_t start=base+((address-base)/64)*64;
    *out=(struct jc_local_region){bad ? address+1 : start,start+64}; return 1;
}
static int copy(void *ctx,uint64_t address,void *out,size_t n) {
    (void)ctx; ++copies;
    if (address<base || n>sizeof(bytes) || address-base>sizeof(bytes)-n ||
        (denied && address<base+128 && address+n>base+64)) return 0;
    memcpy(out,bytes+(address-base),n); return 1;
}
int main(void) {
    struct jc_local_read_cache cache={0}; unsigned char out[128];
    for (unsigned i=0;i<1000;++i) {
        bytes[0]=(unsigned char)i;
        assert(jc_local_read(&cache,1,query,copy,NULL,base,out,1) && out[0]==(unsigned char)i);
    }
    assert(queries==1 && copies==1000);
    assert(jc_local_read(&cache,2,query,copy,NULL,base,out,1) && queries==2);
    assert(jc_local_read(&cache,2,query,copy,NULL,base+60,out,8) && queries==3);
    denied=1;
    assert(!jc_local_read(&cache,2,query,copy,NULL,base+60,out,8) && !cache.used);
    unsigned before=copies;
    assert(!jc_local_read(&cache,2,query,copy,NULL,base+60,out,8) && copies==before);
    denied=0;
    for (unsigned i=0;i<24;++i) assert(jc_local_read(&cache,3,query,copy,NULL,base+i*64,out,1));
    assert(cache.used==16);
    for (unsigned i=0;i<24;++i) assert(jc_local_read(&cache,3,query,copy,NULL,base+i*64,out,1));
    assert(cache.used==16);
    assert(!jc_local_read(&cache,4,query,copy,NULL,0,out,1));
    assert(!jc_local_read(&cache,4,query,copy,NULL,UINT64_MAX-5,out,20));
    assert(!jc_local_read(&cache,4,query,copy,NULL,0x7fffffffffffULL,out,2));
    assert(!jc_local_read(&cache,4,query,copy,NULL,base,NULL,1));
    assert(!jc_local_read(&cache,4,query,copy,NULL,base,out,0));
    bad=1; assert(!jc_local_read(&cache,5,query,copy,NULL,base,out,1));
#ifdef _WIN32
    assert(jc_local_windows_test()==0);
    assert(jc_local_windows_test()==0);
    LARGE_INTEGER frequency,start,stop; QueryPerformanceFrequency(&frequency);
    struct jc_local_read_cache measured={0}; SIZE_T got;
    QueryPerformanceCounter(&start);
    for (unsigned i=0;i<10000;++i) assert(jc_local_read(&measured,1,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)bytes,out,16));
    QueryPerformanceCounter(&stop); double direct=(double)(stop.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart;
    QueryPerformanceCounter(&start);
    for (unsigned i=0;i<10000;++i) assert(ReadProcessMemory(GetCurrentProcess(),bytes,out,16,&got) && got==16);
    QueryPerformanceCounter(&stop); double compatibility=(double)(stop.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart;
    printf("10000 fresh 16-byte reads: protected %.3f ms, compatibility %.3f ms. Reader microbenchmark only.\n",direct,compatibility);
    puts("Windows protected-copy self-test passed on private pages.");
#endif
    puts("Inspection reader: 1000 fresh-value reads / one permission query; inspection resets, cross-region rejection, changed permissions, bounded eviction and invalid ranges passed. Host uses simulated mappings; Windows fault recovery requires runtime.");
}
