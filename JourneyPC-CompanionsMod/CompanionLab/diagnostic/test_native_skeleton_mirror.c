#include "native_skeleton_mirror.h"
#include "native_read_batch.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[4][0x10000];
static const uint64_t image=0x140000000,base=0x100000;
static unsigned reads,writes,fail_read,fail_write; static uint64_t mutate_at,mutate_start;
static struct jc_skel_mirror work;
static void put(unsigned o,unsigned at,const void *value,size_t n) { memcpy(memory[o]+at,value,n); }
static void q(unsigned o,unsigned at,uint64_t value) { put(o,at,&value,8); }
static void d(unsigned o,unsigned at,uint32_t value) { put(o,at,&value,4); }
static int read_memory(void *ctx,uint64_t at,void *out,size_t n) {
    (void)ctx; ++reads;
    if ((fail_read && reads>=fail_read) || at<base || n>sizeof(memory) || at-base>sizeof(memory)-n) return 0;
    memcpy(out,(unsigned char *)memory+(at-base),n);
    if (at==mutate_start && mutate_at>=at && mutate_at-at<n) { ((unsigned char *)memory)[mutate_at-base]^=1; mutate_at=0; }
    return 1;
}
static int write_memory(void *ctx,uint64_t at,const void *value,size_t n) {
    (void)ctx; ++writes;
    assert(n==64 && at>=base+0x20000 && at<base+0x40000);
    if (writes==fail_write) return 0;
    memcpy((unsigned char *)memory+at-base,value,n); return 1;
}
static struct jc_read_batch batch;
static int batch_read(void *ctx,uint64_t at,void *out,size_t n) {
    (void)ctx; return jc_read_batch_get(&batch,read_memory,NULL,at,out,n);
}
static int batch_write(void *ctx,uint64_t at,const void *value,size_t n) {
    jc_read_batch_reset(&batch); return write_memory(ctx,at,value,n);
}
static uint64_t meshes[4];
static void fixture(void) {
    memset(memory,0,sizeof(memory)); memset(&work,0,sizeof(work)); reads=writes=fail_read=fail_write=0; mutate_at=0;
    for (unsigned o=0;o<4;++o) {
        uint64_t b=base+o*0x10000; meshes[o]=b+0x100;
        q(o,0x100,image+0x67f3f8); q(o,0x130,b+0x800); q(o,0x810,b+0x900); q(o,0x918,b+0x1000);
        q(o,0x1000,b+0x2000); q(o,0x2100,b+0x4000);
        q(o,0x20a8,b+0x3000); q(o,0x20c8,b+0x5000); q(o,0x20e0,b+0x5800); d(o,0x20e8,3); d(o,0x20f0,3);
        q(o,0x3008,b+0x3100); memory[o][0x3019]=1;
        const char *names[]={"root","spineB","leg"};
        for (unsigned i=0;i<3;++i) {
            unsigned node=0x3100+i*0x100,bone=0x4000+i*0x100;
            q(o,node,b+0x3000); q(o,node+16,b+(i==2 ? 0x3000 : node+0x100));
            put(o,node+32,names[i],strlen(names[i])+1); q(o,node+48,strlen(names[i])); q(o,node+56,15); q(o,node+64,b+bone);
            q(o,bone+8,i ? b+0x4000 : 0);
            float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; matrix[13]=(float)i+(float)o;
            put(o,bone+32,matrix,64);
            q(o,0x5000+i*32,i ? b+0x5000 : 0); q(o,0x5008+i*32,b+bone+32); q(o,0x5018+i*32,b+0x6000+i*64);
            q(o,0x5800+i*16,b+0x7000+i*48); q(o,0x5808+i*16,b+0x6000+i*64);
        }
    }
}
static void dense_fixture(void) {
    fixture();
    for (unsigned o=0;o<4;++o) {
        uint64_t b=base+o*0x10000;
        memset(memory[o]+0x3000,0,0xd000);
        q(o,0x2100,b+0x6000); q(o,0x20c8,b+0xb000); q(o,0x20e0,0);
        d(o,0x20e8,142); d(o,0x20f0,0); q(o,0x3008,b+0x3100); memory[o][0x3019]=1;
        for (unsigned i=0;i<142;++i) {
            unsigned node=0x3100+i*0x50,bone=0x6000+i*0x80; char name[16];
            snprintf(name,sizeof(name),"bone%03u",i);
            q(o,node,b+0x3000); q(o,node+16,b+(i==141 ? 0x3000 : node+0x50));
            put(o,node+32,name,strlen(name)+1); q(o,node+48,strlen(name)); q(o,node+56,15); q(o,node+64,b+bone);
            q(o,bone+8,i ? b+0x6000 : 0);
            float m[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; m[13]=(float)i+(float)o;
            put(o,bone+32,m,64); q(o,0xb000+i*32,i ? b+0xb000 : 0);
            q(o,0xb008+i*32,b+bone+32); q(o,0xb018+i*32,b+0xd000+i*64);
        }
    }
}
int main(void) {
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    unsigned n,c; unsigned char originals[2][0x10000]; memcpy(originals,memory,sizeof(originals));
    assert(!jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)); assert(n==2 && !c);
    assert(!memcmp(originals,memory,sizeof(originals))); assert(work.ready);
    assert(!jc_skel_post_render(read_memory,NULL,&work,0));
    memory[1][0x4120]^=1; assert(jc_skel_post_render(read_memory,NULL,&work,0)==8); memory[1][0x4120]^=1;
    assert(!memcmp(memory[2]+0x4120,memory[0]+0x4120,64));
    assert(memcmp(memory[2]+0x4020,memory[0]+0x4020,64)); /* root remains independent */
    float x=.7f; put(0,0x4120+48,&x,4);
    assert(!jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)); assert(n==1 && c==1);
    unsigned char sibling[0x10000]; memcpy(sibling,memory[2],sizeof(sibling));
    assert(!jc_skel_apply(read_memory,write_memory,NULL,image,&work,1,&n,&c)); assert(n==2);
    assert(!memcmp(sibling,memory[2],sizeof(sibling)));
    assert(!jc_skel_apply(read_memory,write_memory,NULL,image,&work,1,&n,&c)); assert(!n && !c);
    fixture(); q(2,0x3240,base+0x4100); assert(jc_skel_prepare(read_memory,NULL,image,meshes,&work)); assert(!writes);
    fixture(); memcpy(memory[2]+0x3220,"other",6); assert(jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    fixture(); q(2,0x4208,base+0x20000+0x4100); assert(jc_skel_prepare(read_memory,NULL,image,meshes,&work)==6);
    fixture(); d(0,0x20e8,JC_SKEL_CAP+1); assert(jc_skel_prepare(read_memory,NULL,image,meshes,&work)==5);
    fixture(); q(0,0x3110,base+0x3100); assert(jc_skel_prepare(read_memory,NULL,image,meshes,&work)==5);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    memory[2][0x3220]='X'; assert(jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)==3 && !n);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    float invalid=NAN; put(0,0x4120,&invalid,4); assert(jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)==4 && !n);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    fail_read=reads+1; assert(jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)==1 && !n);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    fail_write=2; assert(jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)==7 && n==2);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    mutate_at=base+0x4120; mutate_start=base+0x4020;
    assert(jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)==3 && !n);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    unsigned before=reads; assert(!jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c));
    printf("skeleton mirror: named mapping, independent roots/sibling/originals, mutation/bounds/partial-write checks passed; 3-bone fixture %u reads\n",reads-before);
    fixture(); jc_read_batch_reset(&batch);
    assert(!jc_skel_prepare(batch_read,NULL,image,meshes,&work));
    jc_read_batch_reset(&batch); before=reads;
    assert(!jc_skel_apply(batch_read,batch_write,NULL,image,&work,0,&n,&c));
    assert(!jc_skel_post_render(read_memory,NULL,&work,0));
    printf("skeleton mirror batching: %u underlying reads including native-render postcheck; originals preserved\n",reads-before);
    fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work)); unsigned maximum=reads;
    for (unsigned at=1;at<=maximum;++at) {
        fixture(); fail_read=at; assert(jc_skel_prepare(read_memory,NULL,image,meshes,&work)); assert(!work.ready && !writes);
    }
    dense_fixture(); assert(!jc_skel_prepare(read_memory,NULL,image,meshes,&work));
    before=reads;
    assert(!jc_skel_apply(read_memory,write_memory,NULL,image,&work,0,&n,&c)); assert(n==141);
    unsigned copy_reads=reads-before; before=reads;
    assert(!jc_skel_post_render(read_memory,NULL,&work,0));
    assert(copy_reads<200 && reads-before<40);
    printf("142-bone fixture: %u copy reads, %u post-render reads; fresh full metadata and all four skeletons checked\n",copy_reads,reads-before);
    /* Changing a protected bone inside a merged read must still stop before writes. */
    mutate_at=base+0x60a0; mutate_start=base+0x6020;
    assert(jc_skel_apply(read_memory,write_memory,NULL,image,&work,1,&n,&c)==3 && !n);
    return 0;
}
