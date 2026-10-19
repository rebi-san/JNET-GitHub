#include "native_read_batch.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[12*JC_READ_PAGE];
static const uint64_t base=0x100000;
static unsigned reads;
static int exact_only;
static uint64_t fault;
static unsigned consumed;
static int consume(void *ctx,unsigned index,const unsigned char *bytes,size_t size) {
    (void)ctx;
    assert(index<3 && size==8 && !memcmp(bytes,memory+index*32,8));
    ++consumed; return 0;
}
static int reader(void *ctx,uint64_t address,void *out,size_t size) {
    (void)ctx; ++reads;
    if (exact_only && size==JC_READ_PAGE) return 0;
    if (address<base || size>sizeof(memory) || address-base>sizeof(memory)-size ||
        (fault>=address && fault-address<size)) return 0;
    memcpy(out,memory+(address-base),size); return 1;
}
int main(void) {
    struct jc_read_batch batch={0}; unsigned char out[8192];
    for (unsigned i=0;i<sizeof(memory);++i) memory[i]=(unsigned char)i;
    for (unsigned i=0;i<100;++i) {
        assert(jc_read_batch_get(&batch,reader,NULL,base+i*8,out,8));
        assert(!memcmp(out,memory+i*8,8));
    }
    assert(reads==1);
    memory[16]^=3;
    assert(jc_read_batch_get(&batch,reader,NULL,base+16,out,8) && out[0]==memory[16] && reads==2);
    memory[32]^=5;
    assert(jc_read_batch_get(&batch,reader,NULL,base+28,out,16) && !memcmp(out,memory+28,16));
    memory[32]^=7;
    assert(jc_read_batch_get(&batch,reader,NULL,base+32,out,1) && out[0]==memory[32]);
    memory[0]=90;
    assert(jc_read_batch_get(&batch,reader,NULL,base,out,8192));
    assert(!memcmp(out,memory,8192));
    memory[0]=91;
    assert(jc_read_batch_get(&batch,reader,NULL,base,out,1) && out[0]==91);
    memory[1]=92; jc_read_batch_reset(&batch);
    assert(jc_read_batch_get(&batch,reader,NULL,base+1,out,1) && out[0]==92);
    for (unsigned i=0;i<12;++i) assert(jc_read_batch_get(&batch,reader,NULL,base+i*4096,out,1));
    memory[0]=93;
    assert(jc_read_batch_get(&batch,reader,NULL,base,out,1) && out[0]==93);
    jc_read_batch_reset(&batch); exact_only=1;
    assert(jc_read_batch_get(&batch,reader,NULL,base+20,out,8) && !memcmp(out,memory+20,8));
    fault=base+22;
    assert(!jc_read_batch_get(&batch,reader,NULL,base+20,out,8));
    exact_only=0; fault=base+2000; jc_read_batch_reset(&batch);
    assert(jc_read_batch_get(&batch,reader,NULL,base+20,out,8));
    assert(!jc_read_batch_get(&batch,reader,NULL,base+1999,out,8));
    assert(!jc_read_batch_get(&batch,reader,NULL,0,out,8));
    assert(!jc_read_batch_get(&batch,reader,NULL,UINT64_MAX,out,8));
    assert(!jc_read_batch_get(&batch,reader,NULL,base,out,0));
    struct jc_pose_span spans[]={{base+64,8,2},{base,8,0},{base+32,8,1}};
    fault=0; reads=consumed=0;
    assert(!jc_read_spans(reader,NULL,spans,3,consume,NULL) && reads==1 && consumed==3);
    memory[32]^=1; reads=consumed=0;
    assert(!jc_read_spans(reader,NULL,spans,3,consume,NULL) && reads==1 && consumed==3);
    fault=base+20; reads=consumed=0;
    assert(!jc_read_spans(reader,NULL,spans,3,consume,NULL) && reads==4 && consumed==3);
    fault=base+33; consumed=0;
    assert(jc_read_spans(reader,NULL,spans,3,consume,NULL)==1 && consumed==1);
    spans[2].address=UINT64_MAX; consumed=0;
    assert(jc_read_spans(reader,NULL,spans,3,consume,NULL)==4 && !consumed);
    puts("Read batches: 100 neighboring requests in one read; repeated/overlapping ranges refresh, large reads and write resets invalidate, eviction and partial-page fallbacks preserve faults. No cross-inspection byte cache.");
    return 0;
}
