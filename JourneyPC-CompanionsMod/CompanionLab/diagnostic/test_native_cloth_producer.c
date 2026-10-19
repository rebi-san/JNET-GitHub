#ifdef _WIN32
#include <windows.h>
#endif
#include "native_cloth_producer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct fixture {
    unsigned char *barn,*dude,*collection;
    unsigned char game[0x200],owner[2][32],link[2][32];
    uint32_t passes[7],reserve;
    struct jc_cloth_frames frames;
    struct jc_cloth_producer_participant participant;
    unsigned prepare,abandon,begin,configure,adopt,cancel,acquired,released,fail,mutate;
    unsigned char vertices[2][4096],indices[2][4096];
};
static void put64(void *p,uint64_t v) { memcpy(p,&v,8); }
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static int read_memory(void *ctx,uint64_t address,void *out,size_t n) {
    struct fixture *f=ctx;
    if (address==UINT64_C(0x140731f60) && n==28) { memcpy(out,f->passes,n);return 1; }
    if (address==UINT64_C(0x140731f84) && n==4) { memcpy(out,&f->reserve,n);return 1; }
    const void *blocks[]={f->barn,f->dude,f->collection,f->game,f->owner,f->link};
    const size_t sizes[]={0x6bd000,0x9b80,0x9200,sizeof(f->game),sizeof(f->owner),sizeof(f->link)};
    for (unsigned i=0;i<6;++i) if (address>=(uintptr_t)blocks[i] && address-(uintptr_t)blocks[i]<=sizes[i] &&
        n<=sizes[i]-(address-(uintptr_t)blocks[i])) { memcpy(out,(void *)(uintptr_t)address,n);return 1; }
    return 0;
}
static int acquire(void *ctx,unsigned kind,uint64_t object) {
    struct fixture *f=ctx;(void)kind;assert(object);++f->acquired;return 1;
}
static void release(void *ctx,unsigned kind,uint64_t object) {
    struct fixture *f=ctx;(void)kind;assert(object);++f->released;
}
static int done(void *ctx,uint64_t generation) { (void)ctx;assert(generation);return 1; }
static int pass_done(void *ctx,uint64_t generation,unsigned pass) { assert(pass<=1);return done(ctx,generation); }
static int publish(void *ctx,uint64_t generation,const struct jc_cloth_frame_draw *draws,unsigned n,uint64_t *ticket) {
    (void)ctx;(void)generation;(void)draws;(void)n;(void)ticket;assert(0);return 0;
}
static int retired(void *ctx,uint64_t ticket) { (void)ctx;(void)ticket;return 1; }
static int prepare(void *ctx,const struct jc_cloth_partition *p,struct jc_cloth_stream streams[7]) {
    struct fixture *f=ctx;++f->prepare;if (f->fail==4) return JC_FRAME_BUSY;
    memset(streams,0,7*sizeof(*streams));
    for (unsigned r=4;r<=5;++r) streams[r]=(struct jc_cloth_stream){
        .vertex=100+r,.index=200+r,.vertex_bytes=sizeof(f->vertices[0]),.index_bytes=sizeof(f->indices[0]),
        .vertices=p->vertex_capacity[r],.indices=p->index_capacity[r],
        .mapped_vertices=f->vertices[r-4],.mapped_indices=f->indices[r-4]};
    if (f->mutate==1) f->passes[4]^=1;
    if (f->mutate==2) put32(f->barn+0x270080+2*0xe0+0x54,99);
    if (f->mutate==3) put64(f->dude+0x9b58,0);
    return 0;
}
static void abandon(void *ctx,struct jc_cloth_stream streams[7]) {
    struct fixture *f=ctx;++f->abandon;memset(streams,0,7*sizeof(*streams));
}
static int begin(void *ctx,uint64_t generation,const struct jc_cloth_partition_job *jobs,size_t count,
        const void *args,const struct jc_cloth_stream streams[7],const struct jc_cloth_material *materials,
        unsigned material_count,unsigned *index) {
    struct fixture *f=ctx;++f->begin;if (f->fail==6) return JC_FRAME_BUSY;
    return jc_cloth_frame_begin(&f->frames,generation,jobs,count,args,streams,materials,material_count,index);
}
static int configure(void *ctx,unsigned index,const uint32_t passes[7]) {
    struct fixture *f=ctx;++f->configure;if (f->fail==7) return JC_FRAME_ARGUMENT;
    return jc_cloth_frame_configure_passes(&f->frames,index,passes,pass_done,f);
}
static int adopt(void *ctx,unsigned index,uint64_t generation,unsigned pass) {
    struct fixture *f=ctx;++f->adopt;struct jc_cloth_frame *frame=f->frames.frames+index;
    assert(frame->phase==JC_FRAME_WRITING && frame->generation==generation && frame->pass_required==(1u<<pass));
    if (f->fail==8) return JC_FRAME_OWNERSHIP;
    if (f->fail==9) { frame->phase=JC_FRAME_UNKNOWN;return JC_FRAME_QUARANTINED; }
    return 0;
}
static int cancel(void *ctx,unsigned index) {
    struct fixture *f=ctx;++f->cancel;if (f->fail==10) return JC_FRAME_BUSY;
    assert(f->frames.frames[index].phase==JC_FRAME_WRITING);
    jc_cloth_frame_release(&f->frames,f->frames.frames+index);return 0;
}
static void setup(struct fixture *f,unsigned pass) {
    memset(f,0,sizeof(*f));
    f->barn=calloc(1,0x6bd000);f->dude=calloc(1,0x9b80);f->collection=calloc(1,0x9200);
    assert(f->barn && f->dude && f->collection);f->reserve=768;
    f->passes[4]=f->passes[5]=pass;
    put64(f->game+0x108,(uintptr_t)f->barn);put64(f->game+0x1e0,UINT64_C(0x200000000));
    put64(f->dude,UINT64_C(0x1405d0380));f->dude[0x81]=1;
    uint32_t appearance[4]={5,10,2,4};memcpy(f->dude+0x68,appearance,sizeof(appearance));
    put64(f->dude+0x9b40,(uintptr_t)f->collection);put32(f->collection+0x91b0,2);
    for (unsigned i=0;i<2;++i) {
        unsigned slot=i+1,role=5-i;
        put64(f->dude+0x9b58+i*16,(uintptr_t)f->collection+i*32);
        put64(f->dude+0x9b60+i*16,(uintptr_t)f->collection+0xc0+i*0x1828);
        put64(f->collection+i*32,(uintptr_t)f->owner[i]);put64(f->collection+i*32+0x18,(uintptr_t)f->barn);
        put64(f->owner[i]+0x10,(uintptr_t)f->link[i]);put64(f->link[i]+0x18,(uintptr_t)f->barn+0x80+slot*0xd00);
        put32(f->barn+0xd14+slot*0xd00,role);put32(f->barn+0x270118+slot*0xe0,1);
    }
    put64(f->barn+0x669960,(uintptr_t)f->barn+0x80);put64(f->barn+0x669968,(uintptr_t)f->barn+0x270080);
    for (unsigned k=0;k<3;++k) put32(f->barn+0x6bca10+pass*0x60+k*0x20,4);
    for (unsigned slot=1;slot<=4;++slot) {
        unsigned role=slot&1 ? 5 : 4;
        put32(f->barn+0xd14+slot*0xd00,role);put32(f->barn+0x270118+slot*0xe0,1);
        put32(f->barn+0x270080+slot*0xe0+0x54,5);put32(f->barn+0x270080+slot*0xe0+0x5c,9);
        unsigned char *entry=f->barn+0x669d10+(pass ? 0 : 768*16)+(slot-1)*16;
        put32(entry,slot);put32(entry+4,role);entry[9]=1;
    }
    f->participant=(struct jc_cloth_producer_participant){.game=(uintptr_t)f->game,.barn=(uintptr_t)f->barn,
        .dude=(uintptr_t)f->dude,.peer=7,.generation=9};
    for (unsigned i=0;i<2;++i) {
        struct jc_cloth_material *m=f->participant.materials+i;
        *m=(struct jc_cloth_material){.peer=7,.generation=9,.material=1000+i,.uniforms=2000+i*128,
            .uniform_bytes=64,.environment=5000,.participant=1,.role=4+i,.outfit=2,.symbol=4,.scarf=5,.scarf_capacity=10};
        for (unsigned j=0;j<6;++j) m->atlas[j]=3000+i*100+j;
    }
    assert(!jc_cloth_frames_init(&f->frames,(struct jc_cloth_frame_backend){f,acquire,release,done,publish,retired}));
}
static void cleanup(struct fixture *f) { free(f->barn);free(f->dude);free(f->collection); }
int main(void) {
    struct fixture *f=calloc(1,sizeof(*f));struct jc_cloth_producer *out=calloc(1,sizeof(*out));assert(f && out);
    struct jc_cloth_producer_ops ops={f,prepare,abandon,begin,configure,adopt,cancel};unsigned scenarios=0;
    for (unsigned pass=0;pass<2;++pass) {
        setup(f,pass);
        assert(!jc_cloth_producer_prepare(read_memory,f,UINT64_C(0x140000000),&f->participant,1,pass,&ops,out));
        assert(out->adopted && !out->stage && out->jobs.count==4 && out->lease.slot[0]==1 && out->lease.slot[1]==2);
        assert(out->plan.ranges[0][4].vertices==5 && out->plan.ranges[1][4].vertex_begin==8 &&
            out->plan.ranges[1][4].index_begin==24 && f->frames.frames[out->index].draw_count==2);
        assert(f->prepare==1 && f->begin==1 && f->configure==1 && f->adopt==1 && !f->released);
        jc_cloth_frame_release(&f->frames,f->frames.frames+out->index);assert(f->acquired==f->released);
        cleanup(f);++scenarios;
    }
    for (unsigned fail=4;fail<=9;++fail) {
        if (fail==5) continue;setup(f,0);f->fail=fail;
        assert(jc_cloth_producer_prepare(read_memory,f,UINT64_C(0x140000000),&f->participant,1,0,&ops,out));
        if (fail==9) assert(out->quarantined && !f->released && !f->abandon);
        else assert(f->acquired==f->released && f->abandon==(fail!=4));
        if (fail==7 || fail==8) assert(f->cancel==1);
        cleanup(f);++scenarios;
    }
    for (unsigned mutation=1;mutation<=3;++mutation) {
        setup(f,0);f->mutate=mutation;
        assert(jc_cloth_producer_prepare(read_memory,f,UINT64_C(0x140000000),&f->participant,1,0,&ops,out)==JC_FRAME_OWNERSHIP);
        assert(out->stage==5 && !f->begin && f->abandon==1 && !f->acquired);cleanup(f);++scenarios;
    }
    for (unsigned invalid=0;invalid<6;++invalid) {
        setup(f,0);
        if (invalid==0) f->passes[5]=1;
        if (invalid==1) put64(f->dude+0x9b60,0);
        if (invalid==2) f->participant.materials[0].peer=8;
        if (invalid==3) f->dude[0x81]=0;
        if (invalid==4) put32(f->barn+0x669d10+768*16,700);
        if (invalid==5) {
            f->barn[0x669d10+768*16+9]=0;f->barn[0x669d10+768*16+16+9]=0;
        }
        assert(jc_cloth_producer_prepare(read_memory,f,UINT64_C(0x140000000),&f->participant,1,0,&ops,out));
        assert(!f->prepare && !f->begin && !f->acquired);cleanup(f);++scenarios;
    }
    for (unsigned variant=0;variant<12;++variant) {
        setup(f,0);
        struct jc_cloth_lease lease={.image=UINT64_C(0x140000000),.game=f->participant.game,
            .barn=f->participant.barn,.dude=f->participant.dude,.state=JC_CLOTH_LIVE};
        assert(!jc_cloth_capture(read_memory,f,&lease,1));
        unsigned char entry[16];memcpy(entry,f->barn+0x669d10+768*16,16);
        uint64_t args=(uintptr_t)f->barn+0x669ac0,streams=(uintptr_t)f->barn+0x669960;
        uint64_t state=(uintptr_t)f->barn+0x270080+0xe0;
        if (variant==1) { put32(entry,2);put32(entry+4,4);state+=0xe0; }
        if (variant==2) lease.state=JC_CLOTH_QUARANTINED;
        if (variant==3) { put32(entry,3);state+=2*0xe0; }
        if (variant==4) put32(entry+4,4);
        if (variant==5) ++args;
        if (variant==6) ++streams;
        if (variant==7) ++state;
        if (variant==8) lease.state=JC_CLOTH_RETIRED;
        if (variant==9) put64(f->dude+0x9b58,0);
        if (variant==10) f->dude[0x81]=0;
        if (variant==11) put64(f->game+0x108,0);
        assert(jc_cloth_producer_owns_worker(read_memory,f,&f->participant,&lease,entry,args,streams,state)==(variant<3));
        cleanup(f);++scenarios;
    }
    printf("Cloth producer: %u current-job ownership, pass0/pass1, partition sizing, fresh capture, rollback and quarantine scenarios passed; no native scheduling or GPU publication\n",scenarios);
    free(out);free(f);return 0;
}
