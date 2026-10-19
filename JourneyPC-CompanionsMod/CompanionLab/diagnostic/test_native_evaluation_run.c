#define main jc_evaluation_layout_fixture_main
#include "test_native_evaluation_layout.c"
#undef main
#include "native_evaluation_run.h"
static unsigned char scheduler_global[8],limits[24],allocator[0x58],vtable[0x40],root_vtable[8];
static unsigned char scheduler[0xa8],scratch[8192],temporary[4096],pool[0x120];
static unsigned char normal_vtable[0x60],add_vtable[0x60],sample_vtable[0x60],clock_vtable[8];
static uint64_t denied,late_node_change;
static struct jc_eval_run_result run_result;
struct segment { uint64_t address; unsigned char *bytes; size_t size; };
static struct segment segments[13];
static unsigned char *mapped(uint64_t at,size_t n) {
    for (unsigned i=0;i<sizeof(segments)/sizeof(segments[0]);++i) {
        struct segment *s=segments+i;
        if (at>=s->address && n<=s->size && at-s->address<=s->size-n) return s->bytes+at-s->address;
    }
    return NULL;
}
static int run_read(void *ctx,uint64_t at,void *out,size_t n) {
    unsigned char *p=mapped(at,n);
    if (!p) {
        int code=read_memory(ctx,at,out,n);
        if (code && late_node_change==at && n==0x38) {
            ((unsigned char *)memory)[at-base+0x30]^=1; late_node_change=0;
        }
        return code;
    }
    ++reads; if (fail_read && reads>=fail_read) return 0;
    memcpy(out,p,n); return 1;
}
static struct jc_read_batch evaluation_batch;
static int batched_run_read(void *ctx,uint64_t at,void *out,size_t size) {
    return jc_read_batch_get(&evaluation_batch,run_read,ctx,at,out,size);
}
static int run_writable(void *ctx,uint64_t at,size_t n) {
    (void)ctx;
    if (denied && at<=denied && denied-at<n) return 0;
    if (mapped(at,n)) return 1;
    return at>=base && n<=sizeof(memory) && at-base<=sizeof(memory)-n;
}
static void u32(unsigned char *p,unsigned at,unsigned n) { memcpy(p+at,&n,4); }
static void u64(unsigned char *p,unsigned at,uint64_t n) { memcpy(p+at,&n,8); }
static void run_fixture(void) {
    fixture(); memset(&work,0,sizeof(work)); memset(&run_result,0,sizeof(run_result)); denied=late_node_change=0;
    memset(scheduler_global,0,sizeof(scheduler_global)); memset(limits,0,sizeof(limits));
    memset(allocator,0,sizeof(allocator)); memset(vtable,0,sizeof(vtable)); memset(scheduler,0,sizeof(scheduler));
    memset(scratch,0,sizeof(scratch)); memset(temporary,0,sizeof(temporary)); memset(pool,0,sizeof(pool));
    segments[0]=(struct segment){image+0x3c0aee0,scheduler_global,sizeof(scheduler_global)};
    segments[1]=(struct segment){image+0x3c03e98,limits,sizeof(limits)};
    segments[2]=(struct segment){image+0x3c214d0,allocator,sizeof(allocator)};
    segments[3]=(struct segment){image+0x599a68,vtable,sizeof(vtable)};
    segments[4]=(struct segment){0x800000,scheduler,sizeof(scheduler)};
    segments[5]=(struct segment){0x900000,scratch,sizeof(scratch)};
    segments[6]=(struct segment){0xa00000,temporary,sizeof(temporary)};
    segments[7]=(struct segment){0xb00000,pool,sizeof(pool)};
    segments[8]=(struct segment){image+0x577b80+0x60,root_vtable,sizeof(root_vtable)};
    segments[9]=(struct segment){image+0x575ab8,normal_vtable,sizeof(normal_vtable)};
    segments[10]=(struct segment){image+0x576798,add_vtable,sizeof(add_vtable)};
    segments[11]=(struct segment){image+0x5782a8,sample_vtable,sizeof(sample_vtable)};
    segments[12]=(struct segment){image+0x67f3b0+0x20,clock_vtable,sizeof(clock_vtable)};
    u64(root_vtable,0,image+0x395d10);
    u64(scheduler_global,0,0x800000); u64(scheduler,0,image+0x599a68);
    u64(scheduler,0x18,0x900000); u32(scheduler,0x20,sizeof(scratch));
    const unsigned callbacks[]={0x411780,0x4117b0,0x4117d0,0x4117e0};
    for (unsigned i=0;i<4;++i) u64(vtable,0x20+i*8,image+callbacks[i]);
    u32(limits,0,1024); u64(limits,16,0xb00000); u64(pool,0,0xb00100);
    u64(allocator,0x28,0xa00000); u64(allocator,0x30,0xa01000); u32(allocator,0x38,4096);
    u64(allocator,0x40,0xa00100); u64(allocator,0x48,0xa00000);
    for (unsigned o=0;o<4;++o) {
        uint64_t b=base+o*0x10000;
        q(o,0x1008,b+0xbe00); d(o,0xbe00,3);
        q(o,0x8028,base+0xb800); q(o,0x8030,b+0xb700); d(o,0x8038,6);
        q(o,0xb700,image+0x577b80); q(o,0xb710,base+0xb800); q(o,0xb718,b+0xb000);
        d(o,0x8400,1); q(o,0x8408,b+0x4120); d(o,0x8410,64);
        q(o,0x9010,b+0x8800); d(o,0x9018,256); d(o,0x9028,144); d(o,0x902c,3); d(o,0x9030,2);
        d(o,0x9048+0x94,1); d(o,0x9048+0xa4,128);
    }
    d(0,0xb810,2); d(0,0xb820,3); q(0,0xb828,base+0xb900);
    const unsigned transform_types[]={0x72c6e8,0x72c700,0x72c718};
    for (unsigned i=0;i<3;++i) {
        q(0,0xb900+i*24,image+transform_types[i]);
        d(0,0xb900+i*24+0xc,i%2); d(0,0xb900+i*24+0x10,i);
    }
}
static void known_graph(void) {
    run_fixture();
    const unsigned normal[]={0x3f9840,0x38ea60,0x3f9850,0x3f9860,0x38e950,0x38ea80,0x38eb80};
    const unsigned add[]={0x3f9840,0x391260,0x3f9850,0x3f9860,0x391130,0x391280,0x391380};
    const unsigned sample[]={0x397f40,0x397f70,0x3f9850,0x398010,0x3f9870,0x398040,0x3981e0};
    for (unsigned i=0;i<7;++i) {
        u64(normal_vtable,0x28+i*8,image+normal[i]); u64(add_vtable,0x28+i*8,image+add[i]);
        u64(sample_vtable,0x28+i*8,image+sample[i]);
    }
    u64(clock_vtable,0,image+0x25fab0);
    for (unsigned o=0;o<4;++o) {
        uint64_t b=base+o*0x10000;
        q(o,0x1030,b+0xa800); q(o,0x1038,2); q(o,0xa800,base+0xc000); q(o,0xa808,base+0xc100);
        q(o,0xb000,image+0x575ab8); q(o,0xb010,base+0xb800); d(o,0xb018,1); q(o,0xb020,b+0xb200);
        d(o,0xb028,1); q(o,0xb030,b+0xb300); q(o,0xb200,b+0xb100);
        float weight=1; put(o,0xb300,&weight,4);
        q(o,0xb100,image+0x5782a8); q(o,0xb110,base+0xb800); q(o,0xb118,base+0xc000); q(o,0xb120,b+0xa900);
        q(o,0xa900,image+0x67f3b0); q(o,0xaa00,image+0x67f3b0);
        d(o,0x8108,1); d(o,0x810c,1);
        d(o,0x9048+0xa8+0x10,1); d(o,0x9048+0xa8+0x14,1);
    }
    q(0,0xc000,base+0xc200); d(0,0xc008,1);
    const uint16_t header[]={20,1,0,1,1,0,0,0,0,0}; put(0,0xc200,header,sizeof(header));
    q(0,0xc218,base+0xc400); q(0,0xc220,base+0xc500); d(0,0xc228,0x04000002);
    const float times[]={0,1},values[]={0,0,0,1,0,0,0,1};
    put(0,0xc400,times,sizeof(times)); put(0,0xc500,values,sizeof(values));
}
static int environment(void) {
    work.region_count=work.read_count=work.byte_count=0; memset(&run_result,0,sizeof(run_result));
    return jc_eval_run_environment(run_read,run_writable,NULL,image,2,3,memory[2]+0x8000,&run_result,&work);
}
static int clocks(void) {
    work.region_count=work.read_count=work.byte_count=0; memset(&run_result,0,sizeof(run_result));
    return jc_eval_run_clocks(run_read,NULL,&work,base+0x20000+0x1000,&run_result);
}
static int outputs(void) {
    work.region_count=work.read_count=work.byte_count=0; memset(&run_result,0,sizeof(run_result));
    for (unsigned i=0;i<4;++i) assert(!jc_skel_layout_read(run_read,NULL,image,meshes[i],work.skeleton+i));
    unsigned items=0;
    int code=jc_eval_run_outputs(run_read,run_writable,NULL,image,2,memory[2]+0x8000,&run_result,&work,&items);
    if (!code) assert(items==3 && run_result.bindings==1);
    return code;
}
static int execution(void) {
    work.region_count=work.read_count=work.byte_count=0; memset(&run_result,0,sizeof(run_result));
    return jc_eval_run_execution(run_read,NULL,memory[2]+0x8000,3,&run_result,&work);
}
int main(void) {
    known_graph(); memcpy(before,memory,sizeof(before));
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work));
    assert(run_result.ready==1 && !run_result.stage && run_result.nodes==2 && run_result.bindings==1);
    assert(!memcmp(before,memory,sizeof(before)));
    for (unsigned owner=2;owner<4;++owner) {
        known_graph();
        for (unsigned o=0;o<4;++o) { q(o,0x1008,base+0xbe00+o*4); d(0,0xbe00+o*4,3); }
        memcpy(before,memory,sizeof(before));
        assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,owner,&run_result,&work) && run_result.ready);
        assert(!memcmp(before,memory,sizeof(before)));
        d(0,0xbe00+owner*4,4); assert(jc_eval_run_recheck(run_read,NULL,image,&work)==3);
    }
    for (unsigned o=0;o<4;++o) {
        known_graph(); q(o,0x1008,base+o*0x10000+0xbe04); d(o,0xbe04,3);
        assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
        known_graph(); q(o,0x1008,base+o*0x10000+0xbe02);
        assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && run_result.stage==9 && run_result.detail==8+o);
        known_graph(); q(o,0x1008,0);
        assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && run_result.stage==9 && run_result.detail==4+o);
        known_graph(); d(o,0xbe00,0);
        assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && run_result.stage==9 && run_result.detail==16+o);
        known_graph(); d(o,0xbe00,0x7ffffffe);
        assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && run_result.stage==9 && run_result.detail==16+o);
    }
    known_graph(); q(2,0x1008,base+0xbe00);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==6 && !run_result.ready);
    known_graph(); denied=base+0x20000+0xbe03;
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_WRITE && run_result.stage==9);
    for (unsigned owner=2;owner<4;++owner) {
        known_graph(); q(owner,0x8408,base+owner*0x10000+0xa100); d(owner,0x8410,16);
        memcpy(before,memory,sizeof(before));
        assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,owner,&run_result,&work) && run_result.ready);
        assert(run_result.output[7]==(1u<<(owner+4)) && !memcmp(before,memory,sizeof(before)));
        q(owner,0xa008,base+owner*0x10000+0xa110);
        assert(jc_eval_run_recheck(run_read,NULL,image,&work)==3);
    }
    known_graph(); q(2,0x8408,base+0x20000+0xa110);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH);
    known_graph(); q(2,0x8408,base+0xa100); q(2,0xa008,base+0xa100);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==6 && !run_result.ready);
    known_graph(); q(2,0x8408,base+0x20000+0x8100); q(2,0xa008,base+0x20000+0x8100);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==6 && !run_result.ready);
    known_graph(); q(2,0x8408,base+0x20000+0xa100); denied=base+0x20000+0xa130;
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_WRITE);
    known_graph(); q(2,0x8408,base+0x20000+0xa100); q(2,0xa000,base+0x20000+0xa600);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH);
    known_graph(); d(2,0x8018,3); d(2,0x8460,3); q(2,0x8468,base+0x4120);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
    known_graph(); q(2,0x8408,base+0x20000+0xa600); d(2,0x8410,16);
    d(2,0x8430,1); q(2,0x8438,base+0x20000+0x4120); d(2,0x8440,16);
    for (unsigned i=0;i<3;++i) d(0,0xb90c+i*24,1);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.bindings==1);
    known_graph(); d(2,0x8018,3); d(2,0x8460,3); d(0,0xb90c,2);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && run_result.detail==23*JC_EVAL_CAP+2);
    known_graph(); q(2,0x3008,base+0x20000+0x3200); q(2,0x8408,base+0x20000+0x4020);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
    known_graph(); q(0,0xb900,0);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
    known_graph(); q(3,0x3008,base+0x30000+0x3200); q(3,0x8408,base+0x30000+0x4020);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,3,&run_result,&work) && run_result.ready);
    assert(run_result.output[7]==8);
    known_graph(); q(2,0xb000,image+0x576798);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
    known_graph(); float zero_weight=0; put(2,0xb300,&zero_weight,4); q(2,0xb118,0); q(2,0xb120,0);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
    known_graph(); put(2,0xb300,&zero_weight,4); q(2,0xb100,image+0x575ab8); d(2,0xb118,0); d(2,0xb128,0);
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work) && run_result.ready);
    known_graph(); put(2,0xb300,&zero_weight,4); q(2,0xb000,image+0x576798); q(2,0xb118,0); q(2,0xb120,0);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); d(2,0x8198+0x94,16);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); d(0,0xc228,2);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); d(0,0xc210,3);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); q(2,0xb120,base+0xa900);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); q(2,0xb200,base+0xb100);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); u64(sample_vtable,0x50,image+0x398041);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH && !run_result.ready);
    known_graph(); u64(scheduler,0x18,base+0xb800);
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==6 && !run_result.ready);
    known_graph(); late_node_change=base+0x20000+0xb000;
    assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==3 && !run_result.ready);
    run_fixture(); assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work)==JC_EVAL_RUN_GRAPH);
    assert(run_result.stage==6 && !run_result.ready);
    run_fixture(); assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,0,&run_result,&work)==JC_EVAL_RUN_ARGUMENT);
    known_graph(); reads=0;
    assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,2,&run_result,&work));
    unsigned unbatched_reads=reads;
    known_graph(); reads=0; jc_read_batch_reset(&evaluation_batch);
    assert(!jc_eval_run_prepare(batched_run_read,run_writable,NULL,image,meshes,2,&run_result,&work));
    unsigned batched_reads=reads;
    printf("Evaluator inspection batching: %u -> %u underlying reads in full fixture.\n",unbatched_reads,batched_reads);
    assert(batched_reads<unbatched_reads);
    assert(!jc_eval_run_recheck(batched_run_read,NULL,image,&work));
    u32(memory[2],0xbe00,7);
    assert(jc_eval_run_recheck(batched_run_read,NULL,image,&work)==3);
    run_fixture(); assert(!environment() && run_result.scheduler_kind==1 && !run_result.ready);
    assert(run_result.scratch_bytes==8192 && run_result.temp_bytes==272);
    unsigned environment_reads=reads;
    run_fixture(); u64(scheduler_global,0,0); assert(environment()==JC_EVAL_RUN_SCHEDULER);
    run_fixture(); u64(scheduler,0,image+0x599a70); assert(environment()==JC_EVAL_RUN_SCHEDULER);
    run_fixture(); u64(vtable,0x20,image+0x4117b0); assert(environment()==JC_EVAL_RUN_SCHEDULER);
    run_fixture(); u32(limits,4,1); assert(environment()==JC_EVAL_RUN_BUSY);
    run_fixture(); u32(limits,0,5); assert(environment()==JC_EVAL_RUN_SPACE);
    run_fixture(); u32(scheduler,0x20,287); assert(environment()==JC_EVAL_RUN_SPACE);
    run_fixture(); u64(scheduler,0x18,0x900010); assert(environment()==JC_EVAL_RUN_SPACE);
    run_fixture(); u32(allocator,0x20,1); assert(environment()==JC_EVAL_RUN_BUSY);
    run_fixture(); u32(allocator,0x18,1); assert(environment()==JC_EVAL_RUN_BUSY);
    run_fixture(); u64(allocator,0x40,0xa00ff0); assert(environment()==JC_EVAL_RUN_WAIT);
    run_fixture(); u64(allocator,0x48,0xa00180); assert(environment()==JC_EVAL_RUN_WAIT);
    for (unsigned owner=2;owner<4;++owner) {
        known_graph(); u64(allocator,0x40,0xa00ff0); u64(allocator,0x48,0xa00800);
        assert(!jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,owner,&run_result,&work) && run_result.ready);
        assert(jc_eval_run_recheck(run_read,NULL,image,&work)==0);
        u64(allocator,0x48,0xa00000);
        assert(jc_eval_run_recheck(run_read,NULL,image,&work)==3);
        known_graph(); u64(allocator,0x40,0xa00ff0); u64(allocator,0x48,0xa00800); denied=0xa00fff;
        assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,owner,&run_result,&work)==JC_EVAL_RUN_WRITE);
        known_graph(); u64(allocator,0x40,0xa00ff0); u64(allocator,0x48,0xa00800); denied=0xa00001;
        assert(jc_eval_run_prepare(run_read,run_writable,NULL,image,meshes,owner,&run_result,&work)==JC_EVAL_RUN_WRITE);
    }
    run_fixture(); u64(allocator,0x40,0xa00ef0); u64(allocator,0x48,0xa00800); assert(!environment()); /* exact end */
    run_fixture(); u64(allocator,0x40,0xa00ef0); assert(environment()==JC_EVAL_RUN_WAIT);
    run_fixture(); u64(allocator,0x40,0xa00ff0); u64(allocator,0x48,0xa00110); assert(environment()==JC_EVAL_RUN_WAIT);
    run_fixture(); u64(allocator,0x40,0xa00ff0); u64(allocator,0x48,0xa00120); assert(!environment());
    run_fixture(); u64(allocator,0x40,0xa01000); assert(environment()==JC_EVAL_RUN_SPACE && run_result.detail==5);
    run_fixture(); u64(pool,0,0); assert(environment()==JC_EVAL_RUN_SPACE);
    run_fixture(); denied=0x900001; assert(environment()==JC_EVAL_RUN_WRITE);
    run_fixture(); denied=0xb00108; assert(environment()==JC_EVAL_RUN_WRITE);
    run_fixture(); denied=image+0x3c03e9c; assert(environment()==JC_EVAL_RUN_WRITE);
    run_fixture(); assert(!clocks() && run_result.clocks==2 && !run_result.ready);
    run_fixture(); float tiny=0.0000001f; put(2,0xa924,&tiny,4); assert(clocks()==JC_EVAL_RUN_CLOCK);
    run_fixture(); float late=3; put(2,0xa910,&late,4); assert(clocks()==JC_EVAL_RUN_CLOCK);
    run_fixture(); float nan=NAN; put(2,0xaa14,&nan,4); assert(clocks()==JC_EVAL_RUN_CLOCK);
    run_fixture(); assert(!outputs() && !run_result.ready);
    run_fixture(); q(2,0xb700,image+0x577b88); assert(outputs()==JC_EVAL_RUN_GRAPH);
    run_fixture(); d(2,0x8400,2); assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==23*JC_EVAL_CAP);
    run_fixture(); d(2,0x8410,16); assert(!outputs());
    run_fixture(); q(0,0xb900,image+0x72c720); assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==22*JC_EVAL_CAP);
    assert(run_result.output[0]==3 && run_result.output[1]==2 && run_result.output[2]==0 &&
        run_result.output[3]==0 && run_result.output[4]==1 && run_result.output[5]==0x72c720 &&
        run_result.output[6]==64 && run_result.output[7]==UINT32_MAX);
    run_fixture(); q(0,0xb900,0); assert(!outputs());
    run_fixture(); q(0,0xb900+48,0); d(0,0xb910,2); assert(!outputs());
    run_fixture(); q(0,0xb900+48,image+0x72c720); d(0,0xb910,2);
    assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==22*JC_EVAL_CAP);
    run_fixture(); d(0,0xb90c,2); assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==21*JC_EVAL_CAP);
    run_fixture(); q(2,0x8408,base+0x4120); assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==24*JC_EVAL_CAP);
    run_fixture(); q(2,0x8408,base+0x20000+0x4020); assert(!outputs());
    run_fixture(); q(2,0x3008,base+0x20000+0x3200);
    q(2,0x8408,base+0x20000+0x4020); assert(!outputs() && work.skeleton[2].names==2);
    run_fixture(); q(2,0x8408,base+0x30000+0x4020);
    assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==24*JC_EVAL_CAP);
    run_fixture(); d(0,0xb910+24,3); assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==20*JC_EVAL_CAP+1);
    run_fixture(); denied=base+0x20000+0x4120; assert(outputs()==JC_EVAL_RUN_WRITE && run_result.detail==26*JC_EVAL_CAP);
    run_fixture(); d(2,0x8400,0); assert(outputs()==JC_EVAL_RUN_GRAPH && run_result.detail==27*JC_EVAL_CAP);
    run_fixture(); assert(!execution() && run_result.nodes==2);
    run_fixture(); d(2,0x902c,4); assert(execution()==JC_EVAL_RUN_GRAPH);
    run_fixture(); d(2,0x9048+0x94,0); assert(execution()==JC_EVAL_RUN_GRAPH);
    run_fixture(); q(2,0x9038,base+0xb900); assert(!execution());
    run_fixture(); q(2,0x9038,base+0xb918); assert(execution()==JC_EVAL_RUN_GRAPH);
    run_fixture(); q(2,0x9008,0xa00100); assert(execution()==JC_EVAL_RUN_BUSY);
    run_fixture(); u64(scheduler,0,image+0x599ba0); segments[3].address=image+0x599ba0;
    const unsigned threaded[]={0x412020,0x412040,0x412060,0x412130};
    for (unsigned i=0;i<4;++i) u64(vtable,0x20+i*8,image+threaded[i]);
    scheduler[0x90]=scheduler[0x91]=1; assert(!environment() && run_result.scheduler_kind==2);
    u32(scheduler,0x88,1); assert(environment()==JC_EVAL_RUN_BUSY);
    u32(scheduler,0x88,0); scheduler[0x91]=0; assert(environment()==JC_EVAL_RUN_BUSY);
    for (unsigned i=1;i<=environment_reads;++i) { run_fixture(); fail_read=i; assert(environment()==1); }
    printf("evaluation run guards: accepted native graphs, activity pruning, callback identity, allocator bounds, output ownership, metadata isolation and rechecks passed\n");
    return 0;
}
