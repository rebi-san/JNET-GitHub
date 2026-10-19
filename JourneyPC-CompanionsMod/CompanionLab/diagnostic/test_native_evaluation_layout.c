#include "native_evaluation_layout.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[4][0x10000],before[4][0x10000];
static const uint64_t image=0x140000000,base=0x100000;
static uint64_t meshes[4],mutate_start,mutate_at;
static unsigned reads,fail_read;
static struct jc_eval_layout_workspace work;
static struct jc_eval_layout_result result;
static void put(unsigned o,unsigned at,const void *value,size_t n) { memcpy(memory[o]+at,value,n); }
static void q(unsigned o,unsigned at,uint64_t value) { put(o,at,&value,8); }
static void d(unsigned o,unsigned at,unsigned value) { put(o,at,&value,4); }
static int read_memory(void *ctx,uint64_t at,void *out,size_t n) {
    (void)ctx; ++reads;
    if ((fail_read && reads>=fail_read) || at<base || n>sizeof(memory) || at-base>sizeof(memory)-n) return 0;
    memcpy(out,(unsigned char *)memory+(at-base),n);
    if (at==mutate_start && mutate_at>=at && mutate_at-at<n) {
        ((unsigned char *)memory)[mutate_at-base]^=1; mutate_at=0;
    }
    return 1;
}
static void fixture(void) {
    memset(memory,0,sizeof(memory)); reads=fail_read=0; mutate_start=mutate_at=0;
    for (unsigned o=0;o<4;++o) {
        uint64_t b=base+o*0x10000; meshes[o]=b+0x100;
        q(o,0x100,image+0x67f3f8); q(o,0x130,b+0x800); q(o,0x810,b+0x900); q(o,0x918,b+0x1000);
        q(o,0x1000,b+0x2000); q(o,0x2100,b+0x4000);
        q(o,0x20a8,b+0x3000); q(o,0x20c8,b+0x5000); q(o,0x20e0,b+0x5800); d(o,0x20e8,3); d(o,0x20f0,3);
        q(o,0x3008,b+0x3100); memory[o][0x3019]=1;
        const char *names[]={"root","spineB","leg"};
        float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        for (unsigned i=0;i<3;++i) {
            unsigned node=0x3100+i*0x100,bone=0x4000+i*0x100;
            q(o,node,b+0x3000); q(o,node+16,b+(i==2 ? 0x3000 : node+0x100));
            put(o,node+32,names[i],strlen(names[i])+1); q(o,node+48,strlen(names[i])); q(o,node+56,15); q(o,node+64,b+bone);
            q(o,bone+8,i ? b+0x4000 : 0); put(o,bone+32,matrix,64);
            q(o,0x5000+i*32,i ? b+0x5000 : 0); q(o,0x5008+i*32,b+bone+32); q(o,0x5018+i*32,b+0x6000+i*64);
            q(o,0x5800+i*16,b+0x7000+i*48); q(o,0x5808+i*16,b+0x6000+i*64);
        }
        q(o,0x1018,b+0x8000);
        d(o,0x8008,2); q(o,0x8010,b+0x8100); d(o,0x8018,2); q(o,0x8020,b+0x8400);
        d(o,0x8040,16); q(o,0x8048,b+0x8800); d(o,0x8050,256);
        d(o,0x8058,2); q(o,0x8060,b+0x8a00); q(o,0x8068,b+0x9000);
        q(o,0x8100,b+0xb000); q(o,0x8198,b+0xb100);
        d(o,0x8100+0x8c,0); d(o,0x8198+0x8c,1); d(o,0x8198+0x90,128); d(o,0x8198+0x94,128);
        d(o,0x9040,2);
        q(o,0x1040,b+0xa700); q(o,0x1048,2); q(o,0xa700,b+0xa900); q(o,0xa708,b+0xaa00);
        float state[6]={0,0,0,-1,0,1}; put(o,0xa910,state,sizeof(state)); put(o,0xaa10,state,sizeof(state));
        q(o,0x20b8,b+0xa000); d(o,0x20c0,1);
        q(o,0xa000,b+0x4120); q(o,0xa008,b+0xa100); put(o,0xa100,matrix,64);
        q(o,0x2060,b+0xa200); q(o,0x2068,b+0xa208); q(o,0xa200,b+0xa300);
        q(o,0xa300,b+0xa400); q(o,0xa318,b+0xa500); d(o,0xa410,0x46);
        put(o,0xa580,matrix,64); put(o,0xa5c0,matrix,64);
    }
}
static int inspect(void) { return jc_eval_layout_inspect(read_memory,NULL,image,meshes,&result,&work); }
static int exact_metadata_read(void *ctx,uint64_t at,void *out,size_t size) {
    if (size!=32) { ++reads; return 0; }
    return read_memory(ctx,at,out,size);
}
int main(void) {
    fixture(); memcpy(before,memory,sizeof(before)); assert(!inspect());
    assert(result.stage==0 && result.disjoint==1 && result.states==8 && result.owner_index==4);
    for (unsigned i=0;i<4;++i) {
        struct jc_eval_owner_result *r=result.owner+i;
        assert(r->records==2 && r->bindings==2 && r->slots==2 && r->scratch_bytes==256 &&
            r->execution_bytes==0x198 && r->output_pairs==1 && r->shape_mutations==1 && !r->pending);
    }
    assert(!memcmp(before,memory,sizeof(before))); unsigned complete_reads=reads;
    fixture(); q(2,0x1018,0); assert(inspect()==2 && result.owner_index==2 && result.owner[2].pending==-1);
    fixture(); q(3,0x8070,base+0x100); assert(inspect()==7 && result.owner[3].pending==1);
    fixture(); d(2,0x8008,JC_EVAL_CAP+1); assert(inspect()==5);
    fixture(); d(2,0x8018,0); assert(inspect()==2);
    fixture(); d(2,0x8058,0); assert(inspect()==2);
    fixture(); d(2,0x8108,33); assert(inspect()==4);
    fixture(); d(2,0x8108,1); d(2,0x810c,2); assert(inspect()==4);
    fixture(); d(2,0x8198+0x8c,2); assert(inspect()==4);
    fixture(); d(2,0x8198+0x90,256); assert(!inspect());
    fixture(); d(2,0x8198+0x90,257); assert(inspect()==4);
    fixture(); d(2,0x9040,1); assert(inspect()==2);
    fixture(); q(2,0x8048,UINT64_MAX-15); assert(inspect()==4);
    fixture(); q(2,0xa008,0); assert(inspect()==4);
    fixture(); q(2,0x2068,base+0x20000+0xa207); assert(inspect()==4);
    fixture(); q(2,0xa000,base+0x4120); assert(inspect()==6 && result.disjoint==0);
    fixture(); q(2,0xa000,base+0x30000+0x4120); assert(inspect()==6);
    fixture(); q(2,0xa000,base+0x20000+0x3100); assert(inspect()==6);
    fixture(); q(2,0x8060,base+0x20000+0x4120); assert(inspect()==6);
    fixture(); q(2,0xa318,base+0xa500); assert(inspect()==6);
    fixture(); q(2,0x8068,base+0x30000+0x9000); assert(inspect()==6);
    fixture(); q(2,0xa008,base+0x20000+0x8800); assert(!inspect());
    fixture(); q(2,0xa008,base+0x30000+0x8800); assert(inspect()==6);
    fixture(); d(2,0xa410,0x45); assert(!inspect() && !result.owner[2].shape_mutations);
    fixture(); q(2,0xa318,0); assert(!inspect() && !result.owner[2].shape_mutations);
    fixture(); d(2,0x20c0,0); assert(!inspect() && !result.owner[2].output_pairs);
    fixture(); float invalid=NAN; put(2,0xa910,&invalid,4); assert(inspect()==4);
    fixture(); float zero=0; put(2,0xaa24,&zero,4); assert(inspect()==4);
    fixture(); float duration=-2; put(2,0xaa1c,&duration,4); assert(inspect()==4);
    fixture(); mutate_start=base+0x20000+0x8000; mutate_at=mutate_start+0x18;
    assert(inspect()==3 && result.stage==4 && result.disjoint==-1);
    fixture(); mutate_start=base+0x20000+0xa000; mutate_at=mutate_start; assert(inspect()==3);
    for (unsigned i=1;i<=complete_reads;++i) { fixture(); fail_read=i; assert(inspect()==1); }
    fixture(); assert(jc_eval_layout_inspect(read_memory,NULL,image,meshes,&result,NULL)==8);
    assert(result.disjoint==-1 && result.owner[0].pending==-1);
    fixture(); work.read_count=work.byte_count=0;
    for (unsigned i=0;i<200;++i) {
        const unsigned char *saved; d(0,0xc000+i*64,i);
        assert(!jc_eval_save(read_memory,NULL,&work,base+0xc000+i*64,32,&saved));
    }
    reads=0; assert(!jc_eval_saved_recheck(read_memory,NULL,&work)); unsigned batched=reads;
    assert(batched==4);
    reads=0; assert(!jc_eval_saved_recheck(exact_metadata_read,NULL,&work) && reads==204);
    d(0,0xc000+199*64,201); reads=0;
    assert(jc_eval_saved_recheck(read_memory,NULL,&work)==3);
    d(0,0xc000+199*64,199); reads=0; fail_read=3;
    assert(jc_eval_saved_recheck(read_memory,NULL,&work)==1);
    fixture(); work.read_count=work.byte_count=0; const unsigned char *large;
    assert(!jc_eval_save(read_memory,NULL,&work,base,8192,&large)); reads=0;
    assert(!jc_eval_saved_recheck(read_memory,NULL,&work) && reads==2);
    memory[0][8191]^=1; assert(jc_eval_saved_recheck(read_memory,NULL,&work)==3);
    printf("evaluation metadata recheck: 200 scalar reads reduced to %u fresh bounded reads; unreadable-gap fallback, late mutation and large-range checks passed\n",batched);
    printf("evaluation layout: read-only ownership, pending jobs, bounds, original/sibling preservation and metadata rechecks passed; %u reads\n",complete_reads);
    return 0;
}
