#include "native_cloth_partition.h"
#include "native_cloth_draw.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls;
static void simulated_worker(void *ctx,void *worker,const void *entry,void *args,
        void *streams,void *descriptor,void *state) {
    (void)ctx; (void)worker; (void)streams; (void)descriptor;
    struct jc_cloth_partition_job j;
    assert(!jc_cloth_worker_counts(entry,state,0,&j));
    unsigned char *p=args;
    uint32_t v=jc_cloth_u32(p+0x210+4*j.role)+j.vertices;
    uint32_t i=jc_cloth_u32(p+0x22c+4*j.role)+j.indices;
    memcpy(p+0x210+4*j.role,&v,4); memcpy(p+0x22c+4*j.role,&i,4); ++calls;
}
static void routing_checks(void) {
    struct jc_cloth_route *r=calloc(1,sizeof(*r)); assert(r);
    unsigned char entry[10]={0},state[0xe0]={0},args[JC_CLOTH_WORKER_ARGS]={0};
    entry[4]=4; entry[9]=1; state[0x54]=3; state[0x5c]=3;
    struct jc_cloth_partition_job j;
    assert(!jc_cloth_worker_counts(entry,state,1,&j));
    uint32_t vc[7]={0,0,0,0,4,0,0},ic[7]={0,0,0,0,48,0,0};
    assert(!jc_cloth_route_prepare(r,1,&j,1,args,sizeof(args),vc,ic));
    assert(r->plan.ranges[1][4].index_begin==24 && !r->plan.ranges[0][4].indices);
    assert(jc_cloth_route_finish(r,1)==JC_PARTITION_INCOMPLETE);
    assert(jc_cloth_route_run(r,2,NULL,simulated_worker,NULL,entry,args,args,state)==JC_PARTITION_ARGUMENT);
    assert(!calls);
    assert(!jc_cloth_route_run(r,1,NULL,simulated_worker,NULL,entry,args,args,state));
    assert(!jc_cloth_route_finish(r,1) && calls==1);
    assert(jc_cloth_route_run(r,1,NULL,simulated_worker,NULL,entry,args,args,state)==JC_PARTITION_SLOT);
    assert(jc_cloth_route_finish(r,1)==JC_PARTITION_INCOMPLETE && calls==1);
    assert(!jc_cloth_route_prepare(r,2,&j,1,args,sizeof(args),vc,ic));
    state[0x54]=4;
    assert(jc_cloth_route_run(r,2,NULL,simulated_worker,NULL,entry,args,args,state)==JC_PARTITION_CAPACITY);
    assert(jc_cloth_route_finish(r,2)==JC_PARTITION_INCOMPLETE && calls==1);
    state[0x54]=3;
    assert(jc_cloth_route_run(r,2,NULL,simulated_worker,NULL,entry,args,args,state)==JC_PARTITION_INCOMPLETE);
    entry[8]=1;
    uint64_t begin=100,end=108; memcpy(state+0xa0,&begin,8); memcpy(state+0xa8,&end,8);
    assert(!jc_cloth_worker_counts(entry,state,1,&j) && j.vertices==5 && j.indices==12);
    end=99; memcpy(state+0xa8,&end,8);
    assert(jc_cloth_worker_counts(entry,state,1,&j)==JC_PARTITION_CAPACITY);
    end=105; memcpy(state+0xa8,&end,8);
    assert(jc_cloth_worker_counts(entry,state,1,&j)==JC_PARTITION_CAPACITY);
    end=UINT64_MAX-3; begin=0; memcpy(state+0xa0,&begin,8); memcpy(state+0xa8,&end,8);
    assert(jc_cloth_worker_counts(entry,state,1,&j)==JC_PARTITION_CAPACITY);
    entry[9]=0;
    assert(!jc_cloth_worker_counts(entry,state,1,&j) && !j.vertices && !j.indices);
    assert(!jc_cloth_route_prepare(r,3,&j,1,args,sizeof(args),vc,ic));
    assert(!jc_cloth_route_run(r,3,NULL,simulated_worker,NULL,entry,args,args,state));
    assert(!jc_cloth_route_finish(r,3));
    assert(jc_cloth_route_prepare(r,3,&j,1,args,sizeof(args),vc,ic)==JC_PARTITION_ARGUMENT);
    assert(jc_cloth_route_finish(r,3)==JC_PARTITION_ARGUMENT);
    assert(!jc_cloth_route_prepare(r,4,&j,1,args,sizeof(args),vc,ic));
    assert(jc_cloth_route_run(r,4,NULL,simulated_worker,NULL,entry,args,NULL,state)==JC_PARTITION_ARGUMENT);
    assert(jc_cloth_route_finish(r,4)==JC_PARTITION_INCOMPLETE);
    free(r);
}
static void draw_view_checks(void) {
    struct jc_cloth_index_view view,saved;
    assert(!jc_cloth_index_view_make(0x12340000,8192,64,1024,24,90,&view));
    assert(jc_cloth_u32(view.bytes+8)==90 && jc_cloth_u32(view.bytes+0x38)==160);
    assert(view.bytes[0xc]==8 && view.bytes[0x3c]==1 && jc_cloth_u32(view.bytes+0x18)==1);
    saved=view;
    assert(jc_cloth_index_view_make(0,8192,64,1024,24,90,&view)==JC_CLOTH_DRAW_ARGUMENT);
    assert(jc_cloth_index_view_make(1,8192,65,1024,24,90,&view)==JC_CLOTH_DRAW_RANGE);
    assert(jc_cloth_index_view_make(1,8192,64,1024,1020,6,&view)==JC_CLOTH_DRAW_RANGE);
    assert(jc_cloth_index_view_make(1,8192,64,1024,0,0,&view)==JC_CLOTH_DRAW_RANGE);
    assert(jc_cloth_index_view_make(1,8192,64,1024,0,4,&view)==JC_CLOTH_DRAW_RANGE);
    assert(jc_cloth_index_view_make(1,4096,64,1024,0,3,&view)==JC_CLOTH_DRAW_RANGE);
    assert(jc_cloth_index_view_make(1,UINT32_MAX,0,UINT32_MAX,0,3,&view)==JC_CLOTH_DRAW_RANGE);
    assert(jc_cloth_index_view_make(1,UINT32_MAX,UINT32_MAX-3,1,0,3,&view)==JC_CLOTH_DRAW_RANGE);
    assert(!memcmp(&view,&saved,sizeof(view)));
}

struct native_jobs_fixture { unsigned char *barn; unsigned char globals[0x28]; unsigned reads,change; };
static int jobs_read(void *context,uint64_t address,void *out,size_t size) {
    struct native_jobs_fixture *x=context;
    if (address>=0x10731f60 && address-0x10731f60<=sizeof(x->globals) &&
        size<=sizeof(x->globals)-(size_t)(address-0x10731f60)) {
        if (address==0x10731f60 && ++x->reads==2 && x->change) x->globals[16]=1;
        memcpy(out,x->globals+(size_t)(address-0x10731f60),size); return 1;
    }
    if (address>=0x20000000 && address-0x20000000<=0x6bd080 &&
        size<=0x6bd080-(size_t)(address-0x20000000)) {
        memcpy(out,x->barn+(size_t)(address-0x20000000),size); return 1;
    }
    return 0;
}
static void native_job_snapshots(void) {
    struct native_jobs_fixture x={0}; x.barn=calloc(1,0x6bd080); assert(x.barn);
    struct jc_cloth_native_jobs *out=calloc(1,sizeof(*out)); assert(out);
    unsigned char owners[768]={1,2}; uint32_t passes[7]={1,1,0,0,0,0,0},reserve=768;
    memcpy(x.globals,passes,sizeof(passes)); memcpy(x.globals+0x24,&reserve,4);
    uint64_t bases[2]={0x20000080,0x20270080}; memcpy(x.barn+0x669960,bases,sizeof(bases));
    uint32_t count=3; for (unsigned i=0;i<3;++i) memcpy(x.barn+0x6bca10+i*0x20,&count,4);
    unsigned char *entries=x.barn+0x669d10+reserve*16;
    for (unsigned i=0;i<3;++i) {
        uint32_t role=i<2 ? 4+i : 2,vertices=3,indices=6,active=1;
        memcpy(entries+i*16,&i,4); memcpy(entries+i*16+4,&role,4); entries[i*16+9]=1;
        memcpy(x.barn+0xd14+i*0xd00,&role,4);
        memcpy(x.barn+0x270080+i*0xe0+0x54,&vertices,4);
        memcpy(x.barn+0x270080+i*0xe0+0x5c,&indices,4);
        memcpy(x.barn+0x270080+i*0xe0+0x98,&active,4);
    }
    assert(!jc_cloth_native_jobs_read(jobs_read,&x,0x10000000,0x20000000,0,0x30,owners,out));
    assert(out->count==2 && out->jobs[0].participant==1 && out->jobs[1].participant==2);
    assert(out->jobs[0].vertices==3 && out->jobs[1].indices==6 && out->arguments==0x20669ac0);
    x.reads=0; x.change=1;
    assert(jc_cloth_native_jobs_read(jobs_read,&x,0x10000000,0x20000000,0,0x30,owners,out));
    x.change=0; x.globals[16]=0; entries[16]=0;
    assert(jc_cloth_native_jobs_read(jobs_read,&x,0x10000000,0x20000000,0,0x30,owners,out)==JC_PARTITION_SLOT);
    entries[16]=1; x.barn[0x270118]=3;
    assert(jc_cloth_native_jobs_read(jobs_read,&x,0x10000000,0x20000000,0,0x30,owners,out));
    x.barn[0x270118]=1; owners[1]=16;
    assert(jc_cloth_native_jobs_read(jobs_read,&x,0x10000000,0x20000000,0,0x30,owners,out)==JC_PARTITION_ROLE);
    owners[1]=2; x.barn[0x6bca30]=4;
    assert(jc_cloth_native_jobs_read(jobs_read,&x,0x10000000,0x20000000,0,0x30,owners,out)==JC_PARTITION_CAPACITY);
    free(out); free(x.barn);
}
int main(void) {
    native_job_snapshots();
    struct jc_cloth_partition_job jobs[]={
        {0,0,4,17,30},{1,0,5,29,60},{2,1,4,23,42},{3,1,5,37,90},
        {4,2,4,31,66},{5,2,5,43,108},{6,0,0,7,12},{7,1,4,13,18}
    };
    uint32_t vc[7],ic[7]; for (unsigned i=0;i<7;++i) { vc[i]=4096; ic[i]=8192; }
    struct jc_cloth_partition p,check;
    assert(jc_cloth_partition_make(jobs,8,vc,ic,&p)==0);
    assert(p.ranges[1][4].vertex_begin==20 && p.ranges[1][4].index_begin==48);
    assert(p.ranges[1][4].vertices==36 && p.ranges[1][4].indices==60);
    assert(p.ranges[1][4].vertex_capacity==40 && p.ranges[1][4].index_capacity==72);
    assert(p.ranges[2][4].vertex_begin==60 && p.ranges[2][4].index_begin==120);
    uint32_t seed=17;
    for (unsigned iteration=0;iteration<1000;++iteration) {
        for (unsigned i=7;i>0;--i) {
            seed=seed*1664525u+1013904223u; unsigned j=seed%(i+1);
            struct jc_cloth_partition_job swap=jobs[i]; jobs[i]=jobs[j]; jobs[j]=swap;
        }
        assert(jc_cloth_partition_make(jobs,8,vc,ic,&check)==0);
        assert(!memcmp(&p,&check,sizeof(p)));
        uint32_t v[16][7],ix[16][7];
        for (unsigned owner=0;owner<16;++owner) for (unsigned role=0;role<7;++role) {
            v[owner][role]=p.ranges[owner][role].vertex_begin;
            ix[owner][role]=p.ranges[owner][role].index_begin;
        }
        for (unsigned i=0;i<8;++i) {
            const struct jc_cloth_partition_job *j=jobs+i;
            const struct jc_cloth_partition_range *r=&p.ranges[j->participant][j->role];
            v[j->participant][j->role]+=j->vertices; ix[j->participant][j->role]+=j->indices;
            assert(v[j->participant][j->role]<=r->vertex_begin+r->vertex_capacity);
            assert(ix[j->participant][j->role]<=r->index_begin+r->index_capacity);
        }
        for (unsigned owner=0;owner<16;++owner) for (unsigned role=0;role<7;++role)
            assert(jc_cloth_partition_finished(&p,owner,role,v[owner][role],ix[owner][role])==0);
    }
    assert(jc_cloth_partition_finished(&p,1,4,20,48)==JC_PARTITION_INCOMPLETE);
    assert(jc_cloth_partition_finished(&p,1,4,57,108)==JC_PARTITION_INCOMPLETE);
    struct jc_cloth_partition_job bad[8]; memcpy(bad,jobs,sizeof(bad));
    bad[1].slot=bad[0].slot;
    assert(jc_cloth_partition_make(bad,8,vc,ic,&check)==JC_PARTITION_SLOT && !check.ready);
    memcpy(bad,jobs,sizeof(bad)); bad[0].slot=768;
    assert(jc_cloth_partition_make(bad,8,vc,ic,&check)==JC_PARTITION_SLOT);
    memcpy(bad,jobs,sizeof(bad)); bad[0].participant=16;
    assert(jc_cloth_partition_make(bad,8,vc,ic,&check)==JC_PARTITION_ROLE);
    bad[0].participant=1; bad[0].role=2;
    assert(jc_cloth_partition_make(bad,8,vc,ic,&check)==JC_PARTITION_ROLE);
    memcpy(bad,jobs,sizeof(bad)); bad[0].vertices=UINT32_MAX;
    assert(jc_cloth_partition_make(bad,8,vc,ic,&check)==JC_PARTITION_CAPACITY);
    vc[4]=p.vertex_capacity[4]-1;
    assert(jc_cloth_partition_make(jobs,8,vc,ic,&check)==JC_PARTITION_CAPACITY);
    vc[4]=4096; ic[5]=p.index_capacity[5]-1;
    assert(jc_cloth_partition_make(jobs,8,vc,ic,&check)==JC_PARTITION_CAPACITY);
    routing_checks();
    draw_view_checks();
    puts("cloth partition: stock plus two recipients, 1000 job orders, rounded capacity; worker routing rejects duplicate/stale/changed jobs and invalid subdivision; completion required; native worker execution checked separately");
    return 0;
}
