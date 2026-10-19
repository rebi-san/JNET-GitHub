#include "native_cloth_partition.h"
#include "native_cloth_draw.h"
#include "native_cloth_commands.h"
#include "native_cloth_publication.h"
#include "native_cloth_material_live.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#define JC_ENVIRONMENT_TEST_PLATFORM
typedef unsigned long DWORD;
typedef int32_t LONG;
typedef struct { void *BaseAddress;size_t RegionSize;unsigned State,Protect; } MEMORY_BASIC_INFORMATION;
enum { HEAP_ZERO_MEMORY=8,MEM_COMMIT=0x1000,PAGE_GUARD=0x100,PAGE_NOACCESS=1,
    PAGE_READWRITE=4,PAGE_WRITECOPY=8,PAGE_EXECUTE_READWRITE=64,PAGE_EXECUTE_WRITECOPY=128 };
static DWORD GetCurrentThreadId(void) { return 1; }
static void *GetProcessHeap(void) { return (void *)1; }
static void *HeapAlloc(void *heap,unsigned flags,size_t n) { assert(heap==(void *)1);return flags ? calloc(1,n) : malloc(n); }
static int HeapFree(void *heap,unsigned flags,void *p) { assert(heap==(void *)1 && !flags);free(p);return 1; }
static LONG InterlockedIncrement(volatile LONG *p) { return ++*p; }
static uintptr_t mapped_probe;
static size_t VirtualQuery(void *address,MEMORY_BASIC_INFORMATION *out,size_t bytes) {
    assert(bytes==sizeof(*out));if ((uintptr_t)address!=mapped_probe) return 0;
    *out=(MEMORY_BASIC_INFORMATION){address,1536,MEM_COMMIT,PAGE_READWRITE};return sizeof(*out);
}
#include "native_environment_probe.h"
#include "cloth_code.h"

#define ABI __attribute__((ms_abi))
typedef void (ABI *worker_fn)(void *,const void *,void *,void *,void *,void *);
static worker_fn worker;
static atomic_uint calls,allocations;
static unsigned char *image;
static void *ABI copy_bytes(void *d,const void *s,size_t n) { return memcpy(d,s,n); }
static void *ABI fill_bytes(void *d,int c,size_t n) { return memset(d,c,n); }
static void *ABI allocate_bytes(size_t n,size_t align) {
    void *p=NULL; assert(!posix_memalign(&p,align,n)); ++allocations; return p;
}
static void ABI release_bytes(void *p) { assert(allocations); --allocations; free(p); }
static void *ABI pool(void) { return (void *)(uintptr_t)1; }
static void *ABI ref_allocate(void *p) {
    assert(p==(void *)(uintptr_t)1); return calloc(1,16);
}
static void ABI ref_release(void *p,void *ref) {
    assert(p==(void *)(uintptr_t)1); assert(!jc_cloth_u32(ref)); free(ref);
}
static void thunk(unsigned rva,void *target) {
    /* Runtime-service adapters only. The cloth instruction bytes stay intact. */
    unsigned char code[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
    memcpy(code+2,&target,8); memcpy(image+rva,code,sizeof(code));
}
static void run(void *context,void *job,const void *entry,void *args,
        void *streams,void *descriptor,void *state) {
    (void)context; worker(job,entry,args,streams,descriptor,state); ++calls;
}
struct inputs {
    _Alignas(16) unsigned char entry[16],state[0xe0],descriptor[0xd00],streams[0x130];
    _Alignas(16) unsigned char positions[64*16],uv[64*8],properties[64*12],normals[64*12+16];
    _Alignas(16) unsigned char lights[0x400],spheres[0x40],slot_aux[768*0xb0];
    uint16_t indices[256],subdivided[1024],edges[128];
};
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static void putptr(void *p,void *v) { memcpy(p,&v,8); }
static void putfloat(void *p,float v) { memcpy(p,&v,4); }
static void initialize(struct inputs *in,unsigned slot,unsigned role,unsigned vertices,unsigned indices,
        unsigned char *vout,uint32_t *iout,int subdivision,int visible) {
    memset(in,0,sizeof(*in));
    put32(in->entry,slot); put32(in->entry+4,role);
    in->entry[8]=(unsigned char)subdivision; in->entry[9]=(unsigned char)visible;
    put32(in->state+0x54,vertices); put32(in->state+0x5c,indices);
    putfloat(in->descriptor+0xc88,1); putfloat(in->descriptor+0xca0,1);
    putfloat(in->descriptor+0xca4,1);
    for (unsigned i=0;i<vertices;++i) {
        putfloat(in->positions+i*16,(float)(slot*10+i));
        putfloat(in->positions+i*16+4,(float)(i%3));
        putfloat(in->positions+i*16+8,(float)(i%2));
        putfloat(in->uv+i*8,(float)i/vertices);
        putfloat(in->uv+i*8+4,(float)i/vertices);
        putfloat(in->normals+i*12+4,1);
    }
    for (unsigned i=0;i<indices;++i) in->indices[i]=(uint16_t)(i%vertices);
    for (unsigned i=0;i<indices*4;++i) in->subdivided[i]=(uint16_t)(i%(vertices+1));
    in->edges[0]=0; in->edges[1]=1;
    putptr(in->state+0xa0,in->edges); putptr(in->state+0xa8,in->edges+2);
    putptr(in->state+0xb8,in->subdivided);
    putptr(in->streams+0x10+8*role,vout); putptr(in->streams+0x48+8*role,iout);
    putptr(in->streams+0x80,in->positions); putptr(in->streams+0x98,in->uv);
    putptr(in->streams+0xa0,in->properties); putptr(in->streams+0xb0,in->indices);
    putptr(in->streams+0xb8,in->normals); putptr(in->streams+0xd8,in->slot_aux);
    putptr(in->streams+0x120,in->lights); putptr(in->streams+0x128,in->spheres);
}
static void cases(void) {
    enum { V=1024,I=4096,N=8 };
    unsigned char *vertices=malloc(V*60),*reference=malloc(V*60);
    uint32_t *indices=malloc(I*4),*reference_indices=malloc(I*4);
    struct inputs *in=malloc(sizeof(*in)),*baseline=malloc(sizeof(*baseline));
    struct jc_cloth_route *route=calloc(1,sizeof(*route));
    assert(vertices && reference && indices && reference_indices && in && baseline && route);
    unsigned char args[JC_CLOTH_WORKER_ARGS]={0};
    uint32_t vc[7],ic[7]; for (unsigned r=0;r<7;++r) { vc[r]=V; ic[r]=I; }
    unsigned seed=987;
    for (unsigned iteration=0;iteration<64;++iteration) {
        struct jc_cloth_partition_job jobs[N];
        unsigned order[N],vcount[N],icount[N],sub[N],visible[N];
        unsigned role=4+(iteration&1);
        for (unsigned j=0;j<N;++j) {
            order[j]=j; vcount[j]=3+j*3; icount[j]=3+j*6;
            sub[j]=(iteration>>1)&1;
            visible[j]=!(((iteration&4) && j==3) || ((iteration&8) && j%3==0));
            initialize(in,j,role,vcount[j],icount[j],vertices,indices,sub[j],visible[j]);
            assert(!jc_cloth_worker_counts(in->entry,in->state,j%3,jobs+j));
        }
        assert(!jc_cloth_route_prepare(route,iteration+1,jobs,N,args,sizeof(args),vc,ic));
        memset(vertices,0xa5,V*60); memset(indices,0xa5,I*4);
        for (unsigned j=N-1;j;--j) {
            seed=seed*1664525u+1013904223u; unsigned at=seed%(j+1),old=order[j];
            order[j]=order[at]; order[at]=old;
        }
        for (unsigned at=0;at<N;++at) {
            unsigned j=order[at],owner=j%3;
            uint32_t vb=jc_cloth_u32(route->args[owner].bytes+0x210+4*role);
            uint32_t ib=jc_cloth_u32(route->args[owner].bytes+0x22c+4*role);
            memcpy(reference,vertices,V*60); memcpy(reference_indices,indices,I*4);
            initialize(in,j,role,vcount[j],icount[j],vertices,indices,sub[j],visible[j]);
            initialize(baseline,j,role,vcount[j],icount[j],reference,reference_indices,sub[j],visible[j]);
            unsigned char normal_args[JC_CLOTH_WORKER_ARGS]; memcpy(normal_args,args,sizeof(args));
            put32(normal_args+0x210+4*role,vb); put32(normal_args+0x22c+4*role,ib);
            memset(normal_args+0x150,0xff,16);
            run(NULL,NULL,baseline->entry,normal_args,baseline->streams,baseline->descriptor,baseline->state);
            assert(!jc_cloth_route_run(route,iteration+1,NULL,run,NULL,in->entry,in->streams,in->descriptor,in->state));
            assert(!memcmp(vertices,reference,V*60)); assert(!memcmp(indices,reference_indices,I*4));
            assert(!memcmp(in->state,baseline->state,0xa0));
            assert(!memcmp(in->state+0xc0,baseline->state+0xc0,0x20));
            assert(!memcmp(in->slot_aux,baseline->slot_aux,sizeof(in->slot_aux)));
            assert(!memcmp(in->properties,baseline->properties,sizeof(in->properties)));
            assert(!memcmp(route->args[owner].bytes,args,0x210));
            for (unsigned k=0;k<jobs[j].indices;++k)
                assert(indices[ib+k]>=vb && indices[ib+k]<vb+jobs[j].vertices);
            if (visible[j]) {
                float x; memcpy(&x,vertices+vb*60,4); assert(x==(float)(j*10));
            }
            assert(!allocations);
        }
        assert(!jc_cloth_route_finish(route,iteration+1));
        if (!route->plan.ranges[0][role].indices) {
            assert(route->plan.ranges[1][role].index_begin>=3);
            /* Exact CPU fallback store from 8a482..8a490, not a GPU fence. */
            memset(indices,0,12);
        }
        for (unsigned owner=0;owner<3;++owner) {
            const struct jc_cloth_partition_range *p=&route->plan.ranges[owner][role];
            for (unsigned k=p->vertex_begin+p->vertices;k<p->vertex_begin+p->vertex_capacity;++k)
                for (unsigned b=0;b<60;++b) assert(vertices[k*60+b]==0xa5);
            unsigned first=p->index_begin+p->indices;
            if (!owner && !p->indices) first=3;
            for (unsigned k=first;k<p->index_begin+p->index_capacity;++k)
                assert(indices[k]==0xa5a5a5a5);
        }
    }
    free(route); free(baseline); free(in); free(reference_indices); free(indices); free(reference); free(vertices);
}
struct thread_job { struct jc_cloth_route *route; struct inputs *in; uint64_t generation; };
static void *thread_run(void *p) {
    struct thread_job *j=p; struct inputs *in=j->in;
    assert(!jc_cloth_route_run(j->route,j->generation,NULL,run,NULL,in->entry,in->streams,in->descriptor,in->state));
    return NULL;
}
static void parallel_cases(void) {
    enum { N=12,V=128,I=512 };
    struct inputs *in=calloc(N,sizeof(*in)); struct jc_cloth_route *route=calloc(1,sizeof(*route));
    struct inputs *reference=malloc(sizeof(*reference));
    assert(in && route && reference);
    unsigned char vertices[V*60],expected[N][3*60],rv[V*60],args[JC_CLOTH_WORKER_ARGS]={0};
    uint32_t indices[I],ri[I],vc[7],ic[7];
    struct jc_cloth_partition_job jobs[N]; struct thread_job threads[N]; pthread_t ids[N];
    for (unsigned role=4;role<=5;++role) {
        for (unsigned k=0;k<7;++k) { vc[k]=V; ic[k]=I; }
        memset(vertices,0xa5,sizeof(vertices)); memset(indices,0xa5,sizeof(indices));
        for (unsigned j=0;j<N;++j) {
            initialize(in+j,j,role,3,3,vertices,indices,0,1);
            assert(!jc_cloth_worker_counts(in[j].entry,in[j].state,j%3,jobs+j));
            initialize(reference,j,role,3,3,rv,ri,0,1);
            unsigned char normal[JC_CLOTH_WORKER_ARGS]={0}; memset(rv,0xa5,sizeof(rv));
            run(NULL,NULL,reference->entry,normal,reference->streams,reference->descriptor,reference->state);
            memcpy(expected[j],rv,sizeof(expected[j]));
        }
        assert(!jc_cloth_route_prepare(route,role,jobs,N,args,sizeof(args),vc,ic));
        for (unsigned j=0;j<N;++j) {
            threads[j]=(struct thread_job){route,in+j,role};
            assert(!pthread_create(ids+j,NULL,thread_run,threads+j));
        }
        for (unsigned j=0;j<N;++j) assert(!pthread_join(ids[j],NULL));
        assert(!jc_cloth_route_finish(route,role) && !allocations);
        unsigned seen_vertices=0,seen_indices=0;
        for (unsigned owner=0;owner<3;++owner) {
            const struct jc_cloth_partition_range *p=&route->plan.ranges[owner][role];
            for (unsigned v=p->vertex_begin;v<p->vertex_begin+p->vertices;v+=3) {
                float x; memcpy(&x,vertices+v*60,4); unsigned slot=(unsigned)(x/10);
                assert(slot<N && slot%3==owner && !(seen_vertices&(1u<<slot)));
                assert(!memcmp(vertices+v*60,expected[slot],sizeof(expected[slot])));
                seen_vertices|=1u<<slot;
            }
            for (unsigned i=p->index_begin;i<p->index_begin+p->indices;i+=3) {
                uint32_t v=indices[i];
                assert(v>=p->vertex_begin && v+3<=p->vertex_begin+p->vertices);
                assert(indices[i+1]==v+1 && indices[i+2]==v+2);
                float x; memcpy(&x,vertices+v*60,4); unsigned slot=(unsigned)(x/10);
                assert(slot<N && slot%3==owner && !(seen_indices&(1u<<slot)));
                seen_indices|=1u<<slot;
            }
        }
        assert(seen_vertices==(1u<<N)-1 && seen_indices==seen_vertices);
    }
    free(reference); free(route); free(in);
}
#include "native_cloth_frame.h"
static struct { unsigned order,offset,count,instances; uint64_t buffer; } draw_capture;
static void ABI topology(void *device,unsigned kind) {
    (void)device; assert(draw_capture.order++==0 && kind==4);
}
static void ABI bind_indices(void *device,void *buffer,unsigned format,unsigned offset) {
    (void)device; assert(draw_capture.order++==1 && format==0x2a);
    draw_capture.buffer=(uintptr_t)buffer; draw_capture.offset=offset;
}
static void ABI draw_indices(void *device,unsigned count,unsigned first,int base) {
    (void)device; assert(draw_capture.order++==2 && !first && !base);
    draw_capture.count=count; draw_capture.instances=1;
}
static void ABI draw_instances(void *device,unsigned count,unsigned instances,unsigned first,int base,unsigned start) {
    (void)device; assert(draw_capture.order++==2 && !first && !base && !start);
    draw_capture.count=count; draw_capture.instances=instances;
}
static void draw_cases(void) {
    typedef unsigned char (ABI *draw_fn)(void *,unsigned,unsigned,const void *,unsigned);
    typedef unsigned char (ABI *instanced_fn)(void *,unsigned,unsigned,const void *,unsigned,unsigned);
    draw_fn draw=(draw_fn)(void *)(image+0x36d5b0);
    instanced_fn instanced=(instanced_fn)(void *)(image+0x36d6a0);
    void *vtable[25]={0}; void **device=vtable;
    vtable[0xc0/8]=(void *)topology; vtable[0x98/8]=(void *)bind_indices;
    vtable[0x60/8]=(void *)draw_indices; vtable[0xa0/8]=(void *)draw_instances;
    unsigned char context[0xb0]={0}; putptr(context+0xa8,&device);
    for (unsigned owner=0;owner<3;++owner) for (unsigned ring=0;ring<4;++ring) {
        uint64_t handle=0x10000+ring*0x1000;
        unsigned first=owner*240,count=90+owner*3,offset=ring*64;
        struct jc_cloth_index_view view,saved;
        assert(!jc_cloth_index_view_make(handle,8192,offset,1024,first,count,&view));
        saved=view;
        memset(&draw_capture,0,sizeof(draw_capture));
        assert(draw(context,2,count,&view,0));
        assert(draw_capture.order==3 && draw_capture.buffer==handle &&
            draw_capture.offset==offset+first*4 && draw_capture.count==count && draw_capture.instances==1);
        memset(&draw_capture,0,sizeof(draw_capture));
        assert(instanced(context,2,count,&view,0,2));
        assert(draw_capture.order==3 && draw_capture.buffer==handle &&
            draw_capture.offset==offset+first*4 && draw_capture.count==count && draw_capture.instances==2);
        assert(!memcmp(&saved,&view,sizeof(view)));
    }
    puts("Native indexed draw: 24 original calls delivered participant byte offsets/counts and selected ring handles unchanged; ordinary/instanced paths; graphics API intercepted, no GPU execution");
}
static unsigned vertex_calls;
static unsigned vertex_base;
static uintptr_t vertex_handle;
static void ABI vertex_bind(void *device,unsigned first,unsigned count,const uintptr_t *buffers,
        const unsigned *strides,const unsigned *offsets) {
    (void)device; assert(!first && count==32);
    assert(buffers[0]==vertex_handle && buffers[1]==vertex_handle);
    assert(strides[0]==60 && strides[1]==60 && offsets[0]==vertex_base && offsets[1]==vertex_base+12);
    for (unsigned i=2;i<32;++i) assert(!buffers[i] && !strides[i] && !offsets[i]);
    ++vertex_calls;
}
static void *ABI input_layout(void *a,void *b) { (void)a; (void)b; return (void *)0x7890; }
static void ABI bind_layout(void *device,void *layout) { (void)device; assert(layout==(void *)0x7890); }
static int stage_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx; if (!address) return 0; memcpy(out,(void *)(uintptr_t)address,bytes); return 1;
}
static void staging_cases(void) {
    struct jc_cloth_command_stage *stage=calloc(1,sizeof(*stage)); assert(stage);
    unsigned char passes[0x18]={0},table[8*0x2c0]={0},state[0x38]={0},batch[0x48]={0};
    unsigned char optional[16]={0},original_bindings[48]={0},original_vertex[0x48]={0};
    uint16_t semantics[]={0,24}; uint32_t offsets[]={0,12};
    put32(original_vertex,60); putptr(optional+8,(void *)0x1234);
    for (unsigned i=0;i<2;++i) {
        putptr(original_bindings+i*24,original_vertex); putptr(original_bindings+i*24+8,offsets+i);
    }
    putptr(state+0x18,original_bindings); putptr(passes+0x10,table);
    for (unsigned i=0;i<8;++i) { put32(table+i*0x2c0+0x20,2); putptr(table+i*0x2c0+0x28,semantics); }
    struct jc_cloth_command_source src={(uintptr_t)1,(uintptr_t)passes,(uintptr_t)optional,
        (uintptr_t)batch,(uintptr_t)state,0,0,0};
    struct jc_cloth_stream stream={.vertex=0x6789,.index=0x4321,.vertex_bytes=4096,.index_bytes=4096,
        .vertices=32,.indices=128};
    unsigned char shader[0x790]={0},native_context[0xb0]={0},bind_context[8]={0};
    void *vtable[32]={0}; void **device=vtable;
    vtable[0x90/8]=(void *)vertex_bind; vtable[0x88/8]=(void *)bind_layout;
    putptr(native_context+0xa8,&device); putptr(bind_context,native_context); putptr(shader+0x788,(void *)1);
    typedef unsigned char (ABI *bind_fn)(void *,void *,void *);
    bind_fn bind=(bind_fn)(void *)(image+0x35d5d0);
    for (unsigned n=1;n<=8;++n) {
        put32(passes+8,n); assert(!jc_cloth_command_stage_init(stage));
        vertex_base=stream.vertex_offset=n*16; vertex_handle=stream.vertex;
        assert(!jc_cloth_command_stage_add(stage,stage_read,NULL,(jc_native_command_build)(void *)(image+0x34e990),
            &src,&stream,24,6));
        assert(stage->count==n && stage->draws==1);
        for (unsigned p=0;p<n;++p) assert(bind(shader,bind_context,stage->commands+p*128+8));
        assert(!memcmp(state+0x18,&(uintptr_t){(uintptr_t)original_bindings},8));
    }
    assert(vertex_calls==36);
    put32(passes+8,8); assert(!jc_cloth_command_stage_init(stage));
    for (unsigned d=0;d<JC_CLOTH_STAGE_DRAWS;++d)
        assert(!jc_cloth_command_stage_add(stage,stage_read,NULL,(jc_native_command_build)(void *)(image+0x34e990),
            &src,&stream,24,6));
    assert(stage->count==JC_CLOTH_STAGE_DRAWS*8);
    assert(jc_cloth_command_stage_add(stage,stage_read,NULL,(jc_native_command_build)(void *)(image+0x34e990),
        &src,&stream,24,6));
    assert(!jc_cloth_command_stage_init(stage));
    put32(passes+8,9); unsigned before=stage->count;
    assert(jc_cloth_command_stage_add(stage,stage_read,NULL,(jc_native_command_build)(void *)(image+0x34e990),
        &src,&stream,0,6) && stage->count==before);
    put32(passes+8,1); put32(original_vertex,48);
    assert(jc_cloth_command_stage_add(stage,stage_read,NULL,(jc_native_command_build)(void *)(image+0x34e990),
        &src,&stream,0,6) && stage->count==before);
    free(stage);
    puts("Native staging: 45 original builder calls / 332 commands and 36 original vertex bindings; selected private buffer, stride60, semantic offsets, stock-compatible base, preserved source bindings, full 37-draw stage and rejected pass/stride/capacity bounds; no GPU");
}
static void publication_cases(void) {
    struct jc_cloth_command_stage *stage=calloc(1,sizeof(*stage));
    struct jc_cloth_publication *capture=calloc(1,sizeof(*capture));
    assert(stage && capture);
    jc_native_command_build build=(jc_native_command_build)(void *)(image+0x34e990);
    uintptr_t materials[7]={0,0,0,0,0x1111,0x2222,0};
    unsigned char passes[24]={0},table[8*0x2c0]={0},state[0x38]={0},batch[0x48]={0};
    unsigned char optional[16]={0},index_view[0x18]={0};
    putptr(passes+0x10,table); putptr(state+0x30,index_view);
    put32(index_view+8,9); putptr(optional+8,(void *)0x7788);
    for (unsigned i=0;i<4;++i) putptr(state+i*8,(void *)(uintptr_t)(0x100+i));
    for (unsigned n=1;n<=8;++n) {
        assert(!jc_cloth_command_stage_init(stage));
        assert(!jc_cloth_publication_init(capture,n,materials,(1u<<4)|(1u<<5)));
        struct jc_cloth_command_source src={0x9999,(uintptr_t)passes,(uintptr_t)optional,
            (uintptr_t)batch,(uintptr_t)state,0x7777,7,0};
        put32(passes+8,1);
        unsigned role=99;
        assert(!jc_cloth_publication_before(capture,stage_read,NULL,(uintptr_t)stage->renderer,&src,&role));
        assert(!build((uintptr_t)stage->renderer,src.material,src.passes,src.optional,
            src.batch,src.index,src.state,src.storage,src.instances));
        for (unsigned r=4;r<=5;++r) {
            src.material=materials[r]; src.instances=r==5 ? 3 : 0;
            src.optional=n&1 ? (uintptr_t)optional : 0;
            put32(passes+8,n);
            assert(jc_cloth_publication_before(capture,stage_read,NULL,(uintptr_t)stage->renderer,&src,&role)==1 && role==r);
            assert(!build((uintptr_t)stage->renderer,src.material,src.passes,src.optional,
                src.batch,src.index,src.state,src.storage,src.instances));
            assert(!jc_cloth_publication_after(capture,stage_read,NULL,role,0));
        }
        uintptr_t begin=(uintptr_t)stage->commands,end=begin+(1+2*n)*128;
        struct jc_cloth_command_source stock[7]; unsigned first[7],counts[7];
        assert(!jc_cloth_publication_resolve(capture,stage_read,NULL,begin,end,stock,first,counts));
        assert(first[4]==1 && first[5]==1+n && counts[4]==n && counts[5]==n);
        assert(stock[4].state!=(uintptr_t)state && !memcmp((void *)stock[4].state,state,sizeof(state)));
        assert(stock[4].optional && jc_cloth_u32((void *)(stock[4].optional+8))==(n&1 ? 0x7788 : 0));
        stage->commands[128+0x60]^=1;
        assert(jc_cloth_publication_resolve(capture,stage_read,NULL,begin,end,stock,first,counts));
        stage->commands[128+0x60]^=1;
        assert(jc_cloth_publication_resolve(capture,stage_read,NULL,begin+256,end,stock,first,counts));
        assert(jc_cloth_publication_before(capture,stage_read,NULL,(uintptr_t)stage->renderer,&src,&role)==-1);
    }
    assert(jc_cloth_publication_init(capture,1,materials,0));
    materials[5]=materials[4];
    assert(jc_cloth_publication_init(capture,1,materials,48));
    unsigned char models[2][0x70]={{0}},items[2][0x28]={{0}},collections[2][16]={{0}},bindings[2][16]={{0}};
    uint64_t model_geometry[2][2]={{1,0},{1,0}}; uint32_t model_batches[2]={0};
    uintptr_t item_addresses[2]={(uintptr_t)items[0],(uintptr_t)items[1]},model_addresses[7]={0};
    for (unsigned n=0;n<2;++n) {
        putptr(models[n]+0x60,item_addresses+n); putptr(models[n]+0x68,item_addresses+n+1);
        putptr(items[n],model_geometry[n]); model_geometry[n][1]=(uintptr_t)&model_batches[n];
        putptr(items[n]+0x20,collections[n]); put32(collections[n],1);
        putptr(collections[n]+8,bindings[n]); putptr(bindings[n],(void *)0x1111);
        model_addresses[4+n]=(uintptr_t)models[n];
    }
    assert(!jc_cloth_publication_init_models(capture,stage_read,NULL,1,model_addresses,48));
    unsigned role;
    assert(jc_cloth_publication_dispatch_role(capture,(uintptr_t)items[0],(uintptr_t)collections[0],&role)==1 && role==4);
    assert(jc_cloth_publication_dispatch_role(capture,(uintptr_t)items[1],(uintptr_t)collections[1],&role)==1 && role==5);
    assert(jc_cloth_publication_dispatch_role(capture,(uintptr_t)items[0],(uintptr_t)collections[1],&role)==-1);
    unsigned char context[0x20]={0},*queue=calloc(1,0x10084); assert(queue);
    putptr(context+0x18,queue);
    int32_t controls[2]={1,2};
    assert(!jc_cloth_publication_append(stage_read,NULL,(uintptr_t)image,(uintptr_t)context,model_addresses[4],(uintptr_t)&controls[0]));
    assert(controls[0]==2 && jc_cloth_u32(queue+0x10080)==1);
    putptr(queue+0x90,(void *)model_addresses[5]); putptr(queue+0x98,controls+1);
    assert(!jc_cloth_publication_append(stage_read,NULL,(uintptr_t)image,(uintptr_t)context,model_addresses[4],(uintptr_t)&controls[0]));
    assert(controls[0]==3 && controls[1]==1 && jc_cloth_u32(queue+0x10080)==2);
    put32(queue+0x10080,0);
    assert(!jc_cloth_publication_append(stage_read,NULL,(uintptr_t)image,(uintptr_t)context,model_addresses[4],(uintptr_t)&controls[0]));
    assert(controls[0]==3);
    put32(queue+0x10080,4096);
    assert(jc_cloth_publication_append(stage_read,NULL,(uintptr_t)image,(uintptr_t)context,model_addresses[4],(uintptr_t)&controls[0]));
    free(queue);
    struct jc_cloth_publication_view view;
    unsigned char vertices[0x48]={0},vertex_bindings[24]={0}; uint32_t semantic=12; uint16_t binding_offset=0;
    put32(vertices,60); putptr(vertex_bindings,vertices); putptr(vertex_bindings+8,&semantic);
    putptr(state+0x18,vertex_bindings); put32(passes+8,1); put32(table+0x20,1); putptr(table+0x28,&binding_offset);
    struct jc_cloth_command_source source={0x1111,(uintptr_t)passes,0,(uintptr_t)batch,(uintptr_t)state,0x7777,0,0};
    struct jc_cloth_stream stream={.vertex=0x6666,.index=0x8888,.vertex_bytes=4096,.index_bytes=4096,.vertices=32,.indices=64};
    assert(!jc_cloth_publication_view_prepare(&view,stage_read,NULL,&source,&stream,6,9));
    assert(!jc_cloth_command_stage_init(stage));
    assert(!build((uintptr_t)stage->renderer,view.source.material,view.source.passes,view.source.optional,
        view.source.batch,view.source.index,view.source.state,view.source.storage,view.source.instances));
    assert(!jc_cloth_publication_view_check(&view,stage_read,NULL,(uintptr_t)stage->commands,(uintptr_t)stage->commands+128));
    assert(!memcmp(stage->commands+0x40,state,0x18));
    assert(jc_cloth_u32(stage->commands+0x70)==9 && jc_cloth_u32(view.binding.index.bytes+0x38)==24);
    stage->commands[0x70]=6;
    assert(jc_cloth_publication_view_check(&view,stage_read,NULL,(uintptr_t)stage->commands,(uintptr_t)stage->commands+128));
    free(capture); free(stage);
    puts("Native publication: 25 original builder calls, 81 commands; copied state, instancing, multi-pass ranges and changed-byte/batch/duplicate rejection; distinct model dispatch with shared shader; native model reference release and queue append/reuse/capacity checks; private stream redirection preserves native constants and reservation/fence");
}
struct frame_capture { unsigned held,done,retired,draws; uint64_t ticket; unsigned char context[0xb0]; };
static int frame_acquire(void *ctx,unsigned kind,uint64_t object) {
    struct frame_capture *f=ctx; assert(kind<=JC_FRAME_TEXTURE && object); ++f->held; return 1;
}
static void frame_release(void *ctx,unsigned kind,uint64_t object) {
    struct frame_capture *f=ctx; assert(kind<=JC_FRAME_TEXTURE && object && f->held); --f->held;
}
static int frame_done(void *ctx,uint64_t generation) { assert(generation); return ((struct frame_capture *)ctx)->done; }
static int frame_retired(void *ctx,uint64_t ticket) {
    struct frame_capture *f=ctx; assert(ticket==f->ticket); return f->retired;
}
static int frame_publish(void *ctx,uint64_t generation,const struct jc_cloth_frame_draw *draws,
        unsigned count,uint64_t *ticket) {
    struct frame_capture *f=ctx; assert(generation && count==4 && f->held==32);
    typedef unsigned char (ABI *draw_fn)(void *,unsigned,unsigned,const void *,unsigned);
    draw_fn draw=(draw_fn)(void *)(image+0x36d5b0);
    for (unsigned i=0;i<count;++i) {
        const struct jc_cloth_frame_draw *d=draws+i;
        /* Material/vertex binding is intercepted here, as is the graphics API.
           This asserts identity propagation, not native shader construction. */
        assert(d->material.peer==10+d->material.participant);
        assert(d->material.outfit==d->material.participant+2);
        assert(d->material.symbol==d->material.participant+10);
        assert(d->material.material==300+(d->material.participant-1)*2+d->material.role-4);
        assert(d->vertex==100+d->material.role && d->vertex_offset==0);
        memset(&draw_capture,0,sizeof(draw_capture));
        assert(draw(f->context,2,d->count,&d->index,0));
        assert(draw_capture.order==3 && draw_capture.count==6 &&
            draw_capture.buffer==200+d->material.role && draw_capture.offset==d->material.participant*96);
        ++f->draws;
    }
    *ticket=++f->ticket; return 1;
}
static void frame_cases(void) {
    struct jc_cloth_frames *scheduler=calloc(1,sizeof(*scheduler));
    struct inputs *in=calloc(6,sizeof(*in));
    unsigned char (*vertices)[64*60]=calloc(2,sizeof(*vertices));
    uint32_t (*indices)[256]=calloc(2,sizeof(*indices));
    assert(scheduler && in && vertices && indices);
    struct frame_capture capture={0};
    void *vtable[25]={0}; void **device=vtable;
    vtable[0xc0/8]=(void *)topology; vtable[0x98/8]=(void *)bind_indices;
    vtable[0x60/8]=(void *)draw_indices; putptr(capture.context+0xa8,&device);
    assert(!jc_cloth_frames_init(scheduler,(struct jc_cloth_frame_backend){
        &capture,frame_acquire,frame_release,frame_done,frame_publish,frame_retired}));
    for (unsigned cycle=1;cycle<=32;++cycle) {
        struct jc_cloth_stream streams[7]={0};
        for (unsigned r=4;r<=5;++r) streams[r]=(struct jc_cloth_stream){
            .vertex=100+r,.index=200+r,.vertex_bytes=64*60,.index_bytes=256*4,
            .vertices=64,.indices=256,.mapped_vertices=vertices[r-4],.mapped_indices=indices[r-4]};
        struct jc_cloth_partition_job jobs[6];
        for (unsigned j=0;j<6;++j) {
            unsigned r=4+j%2;
            initialize(in+j,j,r,3,6,vertices[r-4],indices[r-4],0,1);
            assert(!jc_cloth_worker_counts(in[j].entry,in[j].state,j/2,jobs+j));
        }
        struct jc_cloth_material m[4];
        for (unsigned i=0;i<4;++i) {
            unsigned owner=1+i/2;
            m[i]=(struct jc_cloth_material){.peer=10+owner,.generation=cycle,
                .material=300+i,.uniforms=0x10000+i*256,.uniform_bytes=256,
                .atlas={500+owner,510+owner,520+owner,530,540,550},.environment=600+owner,.participant=owner,.role=4+i%2,
                .outfit=owner+2,.symbol=owner+10,.scarf=owner*7,.scarf_capacity=100};
        }
        unsigned char args[JC_CLOTH_WORKER_ARGS]={0}; unsigned frame=99;
        capture.done=capture.retired=0;
        assert(!jc_cloth_frame_begin(scheduler,cycle,jobs,6,args,streams,m,4,&frame));
        for (unsigned j=0;j<6;++j)
            assert(jc_cloth_frame_dispatch(scheduler->frames+frame,cycle,0x30,in[j].streams,
                NULL,run,NULL,in[j].entry,args,in[j].streams,in[j].descriptor,in[j].state)==1);
        assert(jc_cloth_frame_seal(scheduler,frame)==JC_FRAME_BUSY);
        capture.done=1; assert(!jc_cloth_frame_seal(scheduler,frame));
        assert(!jc_cloth_frame_commit_stock(scheduler->frames+frame,cycle,args,sizeof(args)));
        for (unsigned r=4;r<=5;++r) {
            assert(jc_cloth_u32(args+0x210+4*r)==3);
            assert(jc_cloth_u32(args+0x22c+4*r)==6);
        }
        assert(!jc_cloth_frame_submit(scheduler,frame));
        assert(!jc_cloth_frames_poll(scheduler) && capture.held==32);
        capture.retired=1; assert(!jc_cloth_frames_poll(scheduler) && !capture.held);
    }
    assert(capture.draws==128);
    free(indices); free(vertices); free(in); free(scheduler);
}
static void splice_cases(void) {
    struct jc_cloth_command_stage *stage=calloc(1,sizeof(*stage));
    assert(stage); stage->count=3;
    memset(stage->commands,0x61,128); memset(stage->commands+128,0x62,128);
    memset(stage->commands+256,0x63,128);
    unsigned char original[5*128],output[6*128],before[sizeof(output)];
    memset(original,0x31,sizeof(original));
    uint32_t kind=1; uintptr_t material=0x1234;
    memcpy(original+128,&kind,4); memcpy(original+128+0x20,&material,8);
    memcpy(original+384,&kind,4); memcpy(original+384+0x20,&material,8);
    const struct jc_cloth_command_patch patches[2]={{1,1,0,2,material},{3,1,2,1,material}};
    unsigned written=99;
    assert(!jc_cloth_command_splice(original,5,stage,patches,2,output,6,&written));
    assert(written==6 && !memcmp(output,original,128));
    assert(!memcmp(output+128,stage->commands,256));
    assert(!memcmp(output+384,original+256,128));
    assert(!memcmp(output+512,stage->commands+256,128));
    assert(!memcmp(output+640,original+512,128));
    memcpy(before,output,sizeof(before)); written=99;
    assert(jc_cloth_command_splice(original,5,stage,patches,2,output,5,&written));
    assert(written==99 && !memcmp(before,output,sizeof(before)));
    original[128]=0x2c;
    assert(jc_cloth_command_splice(original,5,stage,patches,2,output,6,&written));
    original[128]=1; original[128+0x20]^=1;
    assert(jc_cloth_command_splice(original,5,stage,patches,2,output,6,&written));
    original[128+0x20]^=1;
    struct jc_cloth_command_patch overlap[2]={patches[0],patches[1]}; overlap[1].first=1;
    assert(jc_cloth_command_splice(original,5,stage,overlap,2,output,6,&written));
    assert(jc_cloth_command_splice(original,5,stage,patches,2,original,6,&written));
    assert(written==99 && !memcmp(before,output,sizeof(before)));
    free(stage);
}
static unsigned final_rotations,final_maps,final_releases,final_fail;
static unsigned char *final_barn;
static _Alignas(16) unsigned char final_storage[7][2][1024];
static uint32_t ABI finalize_rotate(uintptr_t object,uint32_t ring,void *output) {
    (void)object; (void)output; assert(ring<3); ++final_rotations; return final_fail;
}
static uint32_t ABI finalize_map(uintptr_t object,void *output) {
    uintptr_t delta=(uintptr_t)output-(uintptr_t)final_barn-0x666e00;
    unsigned role=(unsigned)(delta/0x630),kind=(unsigned)((delta%0x630)/16);
    assert(role<7 && kind<2); ++final_maps;
    ((unsigned char *)object)[kind ? 0xe : 0x1c]=2;
    putptr(output,final_storage[role][kind]);
    uint32_t bytes=64; memcpy((unsigned char *)output+8,&bytes,4);
    return final_fail;
}
static void ABI finalize_release(uintptr_t dependency) {
    assert(dependency==(uintptr_t)final_barn+0x6bce00); ++final_releases;
    final_barn[0x6bcf84]=1;
}
static struct jc_cloth_live { struct jc_cloth_frames frames; unsigned failed; } finish_backend;
static struct jc_cloth_live *active_cloth_backend;
static struct {
    uintptr_t barn,objects[7][2]; uint64_t generation; unsigned index,roles,finish_failed;
    struct jc_cloth_ring_checkpoint rings[7][2];
} cloth_maps;
static unsigned cloth_participant_ready,cloth_production_pass;
static DWORD cloth_production_thread;
static struct { uintptr_t barn; } cloth_participant;
static volatile LONG JNETCompanionFeatures[40];
static LONG JNETActorTrial[5];
static void *traveler_actor;
static unsigned finish_messages,finish_unmap_fail;
enum { JC_ACTOR_RUNTIME_SCOPE=77 };
typedef size_t SIZE_T;
static int jc_actor_runtime_quarantine(void *p,int code) { (void)p;return code; }
static void traveler_actor_report(int code) { (void)code; }
static void message(const char *s) {
    assert(strstr(s,"cloth finish failure") || strstr(s,"cloth finish counters"));
    if(strstr(s,"cloth finish failure"))++finish_messages;
}
static LONG InterlockedExchange(volatile LONG *p,LONG v) { LONG old=*p;*p=v;return old; }
static int jc_local_windows_write(uint64_t to,const void *from,size_t n) {
    memcpy((void *)(uintptr_t)to,from,n);return 1;
}
static int own_read(void *ctx,uint64_t at,void *out,size_t n) { return stage_read(ctx,at,out,n); }
static int jc_cloth_live_owner(struct jc_cloth_live *s) { return s==&finish_backend && !s->failed; }
static int jc_cloth_live_unmap(struct jc_cloth_live *s,unsigned index) {
    assert(s==&finish_backend && !index);
    assert(s->frames.frames[0].phase==JC_FRAME_READY && s->frames.frames[0].stock_committed);
    return finish_unmap_fail ? JC_FRAME_OWNERSHIP : 0;
}
static int cloth_map_objects(uintptr_t barn,unsigned role,uintptr_t out[2]) {
    uintptr_t slot,geometry;memcpy(&slot,(void *)(barn+0x666e40+role*0x630),8);
    memcpy(&geometry,(void *)(slot+8),8);memcpy(out,(void *)(geometry+0x28),8);out[1]=geometry+0x30;return 1;
}
static uintptr_t original_cloth_finish(uintptr_t barn,void *out,unsigned pass,uintptr_t fourth) {
    typedef uintptr_t (ABI *fn)(uintptr_t,void *,unsigned,uintptr_t);
    return ((fn)(void *)(image+0x8a3a0))(barn,out,pass,fourth);
}
#include "cloth_finish_code.h"
static int production_pass_done(void *ctx,uint64_t generation,unsigned pass) {
    (void)ctx;assert(generation==1 && pass==0);return 1;
}
static void production_finish_cases(unsigned char *barn,unsigned char *job) {
    for(unsigned variant=0;variant<6;++variant) {
        memset(&finish_backend,0,sizeof(finish_backend));memset(&cloth_maps,0,sizeof(cloth_maps));
        memset((void *)JNETCompanionFeatures,0,sizeof(JNETCompanionFeatures));
        active_cloth_backend=&finish_backend;cloth_maps.barn=(uintptr_t)barn;
        cloth_maps.roles=1u<<4;cloth_maps.generation=1;cloth_participant.barn=(uintptr_t)barn;
        finish_messages=0;finish_unmap_fail=variant==3;final_fail=variant==2 ? 5 : 0;
        barn[0x6bcf84]=1;barn[0x6bcad0]=0;barn[0x6bcad1]=0;job[0x58]=0;
        for(unsigned r=0;r<7;++r) {
            uintptr_t objects[2];assert(cloth_map_objects((uintptr_t)barn,r,objects));
            *(unsigned char *)(objects[0]+0x1c)=2;*(unsigned char *)(objects[1]+0xe)=2;
            putptr(barn+0x669970+r*8,final_storage[r][0]);
            putptr(barn+0x6699a8+r*8,final_storage[r][1]);
            if(r==4) {
                memcpy(cloth_maps.objects[r],objects,sizeof(objects));
                for(unsigned k=0;k<2;++k)
                    assert(!jc_cloth_ring_capture(stage_read,NULL,objects[k],k,&cloth_maps.rings[r][k]));
            }
        }
        struct jc_cloth_frame *f=finish_backend.frames.frames;
        struct jc_cloth_partition_job one={0,1,4,3,3};
        uint32_t capacity[7]={0};capacity[4]=64;
        memset(barn+0x669ac0,0,JC_CLOTH_WORKER_ARGS);
        assert(!jc_cloth_route_prepare(&f->route,1,&one,1,barn+0x669ac0,JC_CLOTH_WORKER_ARGS,capacity,capacity));
        f->generation=1;f->source_args=barn+0x669ac0;f->phase=JC_FRAME_WRITING;
        uint32_t passes[7]={0};assert(!jc_cloth_frame_configure_passes(&finish_backend.frames,0,passes,production_pass_done,NULL));
        struct inputs *in=malloc(sizeof(*in));assert(in);
        initialize(in,0,4,3,3,final_storage[4][0],(uint32_t *)final_storage[4][1],0,1);
        f->streams[4].mapped_vertices=final_storage[4][0];f->streams[4].mapped_indices=(uint32_t *)final_storage[4][1];
        memset((unsigned char *)f->source_args+0x150,0xff,16);
        assert(jc_cloth_frame_dispatch(f,1,1u<<4,in->streams,NULL,run,NULL,in->entry,
            (void *)f->source_args,in->streams,in->descriptor,in->state)==1);free(in);
        if(variant==1)atomic_store(&f->route.failed,JC_PARTITION_CAPACITY);
        if(variant==4)finish_backend.failed=1;
        if(variant==5)put32(barn+0x669ac0+0x210+4*4,1);
        unsigned char result[16];memset(result,0xcd,sizeof(result));
        if(!variant) {
            job[0x58]=1;
            assert(cloth_finish_observer((uintptr_t)barn,result,0,0)==(uintptr_t)result && !result[0]);
            uintptr_t waiting=0;memcpy(&waiting,result+8,8);assert(waiting==(uintptr_t)job);
            assert(!finish_messages && f->phase==JC_FRAME_WRITING);job[0x58]=0;
        }
        assert(cloth_finish_observer((uintptr_t)barn,result,0,0)==(uintptr_t)result && result[0]==1);
        if(!variant) {
            assert(!finish_messages && !finish_backend.failed && f->phase==JC_FRAME_READY);
            const uint32_t *indices=(const uint32_t *)final_storage[4][1];
            assert(indices[0]==0 && indices[1]==0 && indices[2]==0);
            assert(indices[24]==0 && indices[25]==1 && indices[26]==2);
        }
        else {
            assert(finish_messages==1 && finish_backend.failed && f->phase==JC_FRAME_UNKNOWN);
            assert(JNETCompanionFeatures[33]==(variant==1 ? 101 : variant==2 ? 103 : variant==3 ? 104 : variant==4 ? 100 : 102));
            assert(JNETActorTrial[0]==-1);
            assert(cloth_finish_observer((uintptr_t)barn,result,0,0)==(uintptr_t)result && result[0]==1);
            assert(finish_messages==1);
        }
    }
    active_cloth_backend=NULL;final_fail=finish_unmap_fail=0;
    puts("Production cloth finish: real worker/seal/commit/native finalizer, all five failure stages and pending-job polling preserve native completion and first failure");
}
static void finalizer_cases(void) {
    unsigned char *barn=calloc(1,0x6bd080); assert(barn); final_barn=barn;
    unsigned char slots[7][16]={{0}},geometry[7][0x60]={{0}},vertices[7][0x50]={{0}};
    unsigned char job[0x60]={0},result[16];
    struct jc_cloth_ring_checkpoint checkpoints[7][2];
    typedef void *(ABI *finish_fn)(void *,void *,unsigned,uintptr_t);
    finish_fn finish=(finish_fn)(void *)(image+0x8a3a0);
    putptr(barn+0x6bcef0,job);
    for (unsigned r=0;r<7;++r) {
        putptr(barn+0x666e40+r*0x630,slots[r]); putptr(slots[r]+8,geometry[r]);
        putptr(geometry[r]+0x28,vertices[r]);
        uint32_t rings=3,count=r ? 6 : 0;
        memcpy(vertices[r]+0x20,&rings,4); memcpy(geometry[r]+0x48,&rings,4);
        vertices[r][0x1c]=2; geometry[r][0x3e]=2;
        assert(!jc_cloth_ring_capture(stage_read,NULL,(uintptr_t)vertices[r],0,&checkpoints[r][0]));
        assert(!jc_cloth_ring_capture(stage_read,NULL,(uintptr_t)geometry[r]+0x30,1,&checkpoints[r][1]));
        memcpy(barn+0x669cec+r*4,&count,4); barn[0x666e58+r*0x630]=1;
        putptr(barn+0x669970+r*8,final_storage[r][0]);
        putptr(barn+0x6699a8+r*8,final_storage[r][1]);
        memset(final_storage[r],0x7b,sizeof(final_storage[r]));
    }
    barn[0x6bcf84]=1; job[0x58]=1;
    assert(finish(barn,result,0,0)==result && !result[0]);
    assert(!final_rotations && !final_maps && !final_releases);
    uintptr_t pending=0; memcpy(&pending,result+8,8); assert(pending==(uintptr_t)job);
    job[0x58]=0;
    assert(finish(barn,result,0,0)==result && result[0]);
    assert(final_rotations==10 && final_maps==4 && final_releases==1);
    for (unsigned r=0;r<7;++r) {
        uintptr_t pointer=0; memcpy(&pointer,barn+0x6699a8+r*8,8);
        assert(pointer==(r<2 ? (uintptr_t)final_storage[r][1] : 0));
        assert(vertices[r][0x18]==(r<2 ? 0 : 1));
        if (r>=2) {
            assert(!jc_cloth_ring_finished(stage_read,NULL,&checkpoints[r][0]));
            assert(!jc_cloth_ring_finished(stage_read,NULL,&checkpoints[r][1]));
        }
    }
    assert(finish(barn,result,1,0)==result && result[0]);
    assert(final_rotations==14 && final_maps==4 && final_releases==1);
    for (unsigned r=0;r<2;++r) {
        assert(!jc_cloth_ring_finished(stage_read,NULL,&checkpoints[r][0]));
        assert(!jc_cloth_ring_finished(stage_read,NULL,&checkpoints[r][1]));
    }
    assert(!memcmp(final_storage[0][1],(unsigned char[12]){0},12));
    assert(jc_cloth_u32(barn+0x667390)==3);
    assert(!barn[0x6bcf84]);
    /* Native completion alone does not certify buffer rotation success. */
    barn[0x6bcf84]=1; barn[0x6bcad1]=0; final_fail=5;
    for (unsigned r=0;r<2;++r) {
        putptr(barn+0x6699a8+r*8,final_storage[r][1]);
        vertices[r][0x1c]=2; geometry[r][0x3e]=2;
        assert(!jc_cloth_ring_capture(stage_read,NULL,(uintptr_t)vertices[r],0,&checkpoints[r][0]));
        assert(!jc_cloth_ring_capture(stage_read,NULL,(uintptr_t)geometry[r]+0x30,1,&checkpoints[r][1]));
    }
    assert(finish(barn,result,1,0)==result && result[0]);
    assert(final_rotations==18 && vertices[0][0x18]==1 && geometry[0][0x40]==1);
    assert(jc_cloth_ring_finished(stage_read,NULL,&checkpoints[0][0]));
    assert(jc_cloth_ring_finished(stage_read,NULL,&checkpoints[0][1]));
    production_finish_cases(barn,job);
    free(barn); final_barn=NULL; final_fail=0;
    puts("Native cloth finalizer: pending job, pass0/pass1 mapping and dependency order, ring advancement, empty-stock triangle, cleared outputs and ignored rotation errors verified against original code; buffer services intercepted, no GPU");
}
static int atlas_fixture_read(void *ctx,uint64_t address,void *out,size_t size) {
    (void)ctx;memcpy(out,(void *)(uintptr_t)address,size);return 1;
}
static void native_atlas_subscribers(void) {
    void (ABI *bind)(void *,void *,void *)=(void *)(image+0x2cca00);
    void (ABI *destroy)(void *)=(void *)(image+0x85b70);
    unsigned char entry[2][64]={0},holders[3][0x28]={0};
    void *vtable[2]={(void *)(image+0x85b70),(void *)(image+0x78a00)};
    for (unsigned first=0;first<3;++first) for (unsigned second=0;second<3;++second) {
        if (second==first) continue;
        memset(entry,0,sizeof(entry));memset(holders,0,sizeof(holders));
        struct jc_cloth_atlas_list before,after;
        for (unsigned i=0;i<3;++i) {
            putptr(holders[i],vtable);
            assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[0],&before));
            bind(entry[0],NULL,holders[i]);
            assert(holders[i][0x20]==1);
            assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[0],&after));
            assert(!jc_cloth_atlas_list_changed(&before,&after,(uintptr_t)holders[i]+8,1));
        }
        before=after;bind(entry[0],NULL,holders[1]);
        assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[0],&after));
        assert(!memcmp(&before,&after,sizeof(before)));
        unsigned order[3]={first,second,3-first-second};
        for (unsigned i=0;i<3;++i) {
            before=after;destroy(holders[order[i]]);
            assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[0],&after));
            assert(!jc_cloth_atlas_list_changed(&before,&after,(uintptr_t)holders[order[i]]+8,0));
            unsigned char zero[24]={0};assert(!memcmp(holders[order[i]]+8,zero,sizeof(zero)));
        }
    }
    memset(holders,0,sizeof(holders));putptr(holders[0],vtable);
    bind(entry[0],NULL,holders[0]);bind(entry[1],NULL,holders[0]);
    struct jc_cloth_atlas_list list;
    assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[0],&list) && !list.count);
    assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[1],&list) && list.count==1);
    destroy(holders[0]);
    assert(!jc_cloth_atlas_list_read(atlas_fixture_read,NULL,(uintptr_t)entry[1],&list) && !list.count);
    puts("Native atlas subscribers: original bind/rebind/unlink and destructor, repeated binding and all six three-holder retirement orders preserve neighbors; no texture loading or GPU lifetime claim");
}
static unsigned char scalar_field[24],vector_field[24],matrix_field[24];
static void *ABI uniform_field(void *definition,const char *name) {
    assert(definition==scalar_field);
    if (!strcmp(name,"scalar")) return scalar_field;
    if (!strcmp(name,"vector")) return vector_field;
    assert(!strcmp(name,"matrix"));return matrix_field;
}
static void *ABI plain_parameter(const char *name,const char *delimiter) {
    (void)delimiter;assert(!strcmp(name,"scalar") || !strcmp(name,"vector") || !strcmp(name,"matrix") ||
        !strcmp(name,"envOccCube") || !strncmp(name,"texture",7));
    return NULL;
}
static void native_texture_bindings(void) {
    struct texture_material {
        unsigned char material[64],slot[32],link[32],payload[160],collection[16],member[16];
        unsigned char definition[80],fields[7*24],buffer[192];
    } m[4]={0};
    _Alignas(8) static const char names[7][32]={"textureAtlasA","textureAtlasB","textureMask",
        "textureDetail","textureFilledA","textureFilledB","envOccCube"};
    unsigned char textures[7][0x120]={0},handles[7][0xa0]={0};
    for (unsigned i=0;i<7;++i) {
        putptr(textures[i]+0x70,handles[i]);put32(textures[i]+0x110,1+i%3);
        put32(handles[i]+0xc,100+i);put32(handles[i]+0x94,i%2 ? 0x80000000u : 0);
    }
    for (unsigned i=0;i<4;++i) {
        putptr(m[i].material+0x30,m[i].slot);putptr(m[i].slot+0x10,m[i].link);
        putptr(m[i].link+0x18,m[i].payload);putptr(m[i].payload+0x80,m[i].collection);
        put32(m[i].collection,1);putptr(m[i].collection+8,m[i].member);
        putptr(m[i].member,m[i].definition);putptr(m[i].member+8,m[i].buffer);
        put32(m[i].definition+0x40,7);putptr(m[i].definition+0x48,m[i].fields);
        put32(m[i].buffer,sizeof(m[i].buffer));
        for (unsigned j=0;j<7;++j) {
            unsigned char *field=m[i].fields+j*24; uint16_t offset=8+j*24,width=24;
            field[2]=(unsigned char)(0x42+j%3);putptr(field+8,(void *)names[j]);
            memcpy(field+16,&offset,2);memcpy(field+18,&width,2);
        }
    }
    void (ABI *set)(uintptr_t,const char *,uintptr_t)=(void *)(image+0x273640);
    unsigned char stock[2][192];memcpy(stock[0],m[0].buffer,192);memcpy(stock[1],m[1].buffer,192);
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<(i ? 6u : 4u);++j) {
        unsigned char expected[24]={0};uint32_t type=0x42+j%3;
        if (j%3==0 && j%2) type|=0x80000000u;
        put32(expected,type);put32(expected+4,100+j);
        putptr(expected+8,textures[j]+8);putptr(expected+16,handles[j]);
        assert(!jc_cloth_texture_check(atlas_fixture_read,NULL,(uintptr_t)m[i+2].material,
            names[j],(uintptr_t)textures[j],0));
        set((uintptr_t)m[i+2].material,names[j],(uintptr_t)textures[j]);
        assert(!memcmp(expected,m[i+2].buffer+8+j*24,24));
        assert(!jc_cloth_texture_bound(atlas_fixture_read,NULL,(uintptr_t)m[i+2].material,
            names[j],(uintptr_t)textures[j]));
        assert(jc_cloth_texture_bound(atlas_fixture_read,NULL,(uintptr_t)m[i+2].material,
            names[j],(uintptr_t)textures[(j+1)%6]));
        unsigned char *field=m[i+2].fields+j*24;
        unsigned char saved_type=field[2];field[2]=0;
        assert(jc_cloth_texture_check(atlas_fixture_read,NULL,(uintptr_t)m[i+2].material,
            names[j],(uintptr_t)textures[j],0));field[2]=saved_type;
        uint16_t saved_offset,bad_offset=180;memcpy(&saved_offset,field+16,2);
        memcpy(field+16,&bad_offset,2);
        assert(jc_cloth_texture_check(atlas_fixture_read,NULL,(uintptr_t)m[i+2].material,
            names[j],(uintptr_t)textures[j],0));memcpy(field+16,&saved_offset,2);
        m[i+2].buffer[8+j*24+4]^=1;
        assert(jc_cloth_texture_bound(atlas_fixture_read,NULL,(uintptr_t)m[i+2].material,
            names[j],(uintptr_t)textures[j]));m[i+2].buffer[8+j*24+4]^=1;
        set((uintptr_t)m[i+2].material,names[j],(uintptr_t)textures[j]);
        assert(!memcmp(expected,m[i+2].buffer+8+j*24,24));
        assert(!memcmp(stock[i],m[i].buffer,192));
    }
    uint64_t atlas[6];for (unsigned i=0;i<6;++i) atlas[i]=(uintptr_t)textures[i];
    for (unsigned i=0;i<2;++i) {
        uintptr_t material=(uintptr_t)m[i+2].material;
        assert(jc_cloth_texture_bundle_bound(atlas_fixture_read,NULL,material,4+i,atlas,(uintptr_t)textures[6]));
        set(material,names[6],(uintptr_t)textures[6]);
        assert(!jc_cloth_texture_bundle_bound(atlas_fixture_read,NULL,material,4+i,atlas,(uintptr_t)textures[6]));
        assert(jc_cloth_texture_bundle_bound(atlas_fixture_read,NULL,material,4+i,atlas,(uintptr_t)textures[0]));
        put32(handles[6]+0xc,999);
        assert(jc_cloth_texture_bundle_bound(atlas_fixture_read,NULL,material,4+i,atlas,(uintptr_t)textures[6]));
        put32(handles[6]+0xc,106);
    }
    puts("Native texture bindings: original named setter, lookup and three texture-kind writes; four robe/six scarf bindings, owner/handle/index and repeated assignment verified; pointer copies do not retain textures, no GPU");
}
static void native_environment_jobs(void) {
    void (ABI *initialize)(void)=(void *)(image+0x1da580);
    void (ABI *run_cube)(void *,void *)=(void *)(image+0x1db1f0);
    unsigned char *scene=calloc(1,0x20010);assert(scene);
    _Alignas(16) unsigned char streams[0x160]={0},job[0x48]={0},descriptor[0x88]={0};
    _Alignas(16) unsigned char input[0x30]={0},output[3][32+1536+32];
    memset(output,0xa5,sizeof(output));
    /* Native tables are initialized once, then immutable across participant jobs. */
    assert(!mprotect(image+0x3dfc000,0x250000,PROT_READ|PROT_WRITE));
    initialize();
    assert(!mprotect(image+0x3dfc000,0x250000,PROT_READ));
    putptr(streams+0x138,scene);putptr(job+8,streams);
    descriptor[0x37]=1;putptr(descriptor+0x38,input);
    put32(input+0x18,32);put32(input+0x1c,256);
    for (unsigned participant=0;participant<3;++participant) {
        putfloat(input,100.f*participant);putfloat(input+4,5.f);putfloat(input+8,-20.f*participant);
        putptr(input+0x10,output[participant]+32);input[0x20]=0;
        run_cube(job,descriptor);
        for (unsigned i=0;i<sizeof(output[participant]);++i) assert(output[participant][i]==0xa5);
        input[0x20]=1;run_cube(job,descriptor);
        unsigned changed=0;
        for (unsigned i=0;i<1536;++i) changed+=output[participant][32+i]!=0xa5;
        assert(changed);
        for (unsigned i=0;i<32;++i)
            assert(output[participant][i]==0xa5 && output[participant][1568+i]==0xa5);
        if (participant) assert(!memcmp(output[0]+32,output[participant]+32,1536));
    }
    unsigned char empty[1536];memcpy(empty,output[0]+32,sizeof(empty));
    _Alignas(16) float vertices[8][4]={{-1,0,-1,1},{1,0,-1,1},{1,2,-1,1},{-1,2,-1,1},
        {-1,0,1,1},{1,0,1,1},{1,2,1,1},{-1,2,1,1}};
    unsigned char faces[36]={0,3,2,0,2,1, 4,5,6,4,6,7, 0,4,7,0,7,3,
        1,2,6,1,6,5, 0,1,5,0,5,4, 3,7,6,3,6,2};
    unsigned char sizes[6]={2,2,2,2,2,2};
    size_t page=(size_t)sysconf(_SC_PAGESIZE);assert(page>=sizeof(vertices));
    unsigned char *source=mmap(NULL,page*6,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(source!=MAP_FAILED);
    const void *parts[3]={faces,sizes,vertices};
    const size_t lengths[3]={sizeof(faces),sizeof(sizes),sizeof(vertices)};
    unsigned char *bounded[3];
    for (unsigned i=0;i<3;++i) {
        assert(!mprotect(source+2*i*page,page,PROT_READ|PROT_WRITE));
        bounded[i]=source+(2*i+1)*page-lengths[i];
        memcpy(bounded[i],parts[i],lengths[i]);
        assert(!mprotect(source+2*i*page,page,PROT_READ));
    }
    _Alignas(16) unsigned char node[0xd0]={0};
    putfloat(node+0x10,-1);putfloat(node+0x14,0);putfloat(node+0x18,-1);
    putfloat(node+0x20,1);putfloat(node+0x24,2);putfloat(node+0x28,1);
    putfloat(node+0x40,.1f);putfloat(node+0x44,.3f);putfloat(node+0x48,.8f);
    putptr(node+0x50,bounded[0]);putptr(node+0x58,bounded[1]);putptr(node+0x68,bounded[2]);put32(node+0x74,6);
    put32(node+0x70,12);put32(node+0x7c,8);put32(node+0x80,1);
    struct jc_environment_occluder snapshot;
    assert(!jc_environment_occluder_read(atlas_fixture_read,NULL,(uintptr_t)node,&snapshot));
    assert(snapshot.bytes==0x190 && snapshot.planes==6);
    void (ABI *append)(void *,const void *)=(void *)(image+0xe94a0);
    append(scene,snapshot.node);
    assert(jc_cloth_u32(scene+0x20000)==snapshot.bytes);
    uint64_t head;memcpy(&head,scene+0x20008,8);assert(head==(uintptr_t)scene);
    put32(node+0x70,11);
    assert(jc_environment_occluder_read(atlas_fixture_read,NULL,(uintptr_t)node,&snapshot));
    put32(node+0x70,12);put32(node+0x7c,7);
    assert(jc_environment_occluder_read(atlas_fixture_read,NULL,(uintptr_t)node,&snapshot));
    put32(node+0x7c,8);
    const float origins[3][3]={{0,1,3},{3,1,0},{-3,1,0}};
    unsigned char preserved[sizeof(node)];memcpy(preserved,node,sizeof(node));
    for (unsigned participant=0;participant<3;++participant) {
        memcpy(input,origins[participant],12);putptr(input+0x10,output[participant]+32);
        run_cube(job,descriptor);
        assert(memcmp(empty,output[participant]+32,1536));
        if (participant) assert(memcmp(output[0]+32,output[participant]+32,1536));
        for (unsigned i=0;i<32;++i)
            assert(output[participant][i]==0xa5 && output[participant][1568+i]==0xa5);
    }
    assert(!memcmp(node,preserved,sizeof(node)));
    /* GPU maps need not have the tight pitches used by the CPU fixture. */
    const unsigned pitches[][2]={{32,256},{64,512},{256,2048},{64,768}};
    unsigned pitched_jobs=0;
    for (unsigned p=0;p<sizeof(pitches)/sizeof(pitches[0]);++p) {
        unsigned row=pitches[p][0],face=pitches[p][1],bytes=face*6;
        unsigned char *mapped=malloc(bytes+64);assert(mapped);
        for (unsigned participant=0;participant<3;++participant) {
            memset(mapped,0xa5,bytes+64);
            memcpy(input,origins[participant],12);putptr(input+0x10,mapped+32);
            put32(input+0x18,row);put32(input+0x1c,face);
            run_cube(job,descriptor);++pitched_jobs;
            for (unsigned f=0;f<6;++f) for (unsigned offset=0;offset<face;++offset) {
                unsigned y=offset/row,x=offset%row;
                unsigned char expected=y<8 && x<32 ? output[participant][32+f*256+y*32+x] : 0xa5;
                if (mapped[32+f*face+offset]!=expected) {
                    fprintf(stderr,"probe pitch %u participant %u face %u offset %u: %u != %u\n",
                        p,participant,f,offset,mapped[32+f*face+offset],expected);
                    abort();
                }
            }
            for (unsigned i=0;i<32;++i) assert(mapped[i]==0xa5 && mapped[32+bytes+i]==0xa5);
        }
        free(mapped);
    }
    assert(pitched_jobs==12 && !memcmp(node,preserved,sizeof(node)));
    for (unsigned i=0;i<3;++i) assert(!memcmp(bounded[i],parts[i],lengths[i]));
    assert(!munmap(source,page*6));
    free(scene);
    puts("Native environment cube: original lookup initialization, six tight and twelve pitched jobs; three origins produce distinct six-face 8x8 outputs; row/face padding, source and guards preserved; GPU textures and live world capture/scheduling not exercised");
}
static uintptr_t probe_handle,probe_texture;
static unsigned probe_gpu_error,probe_resizes,probe_samplers,probe_gpu_calls;
static unsigned probe_pipeline,probe_resources,probe_objects,probe_maps,probe_queries;
static unsigned char probe_query[0x28];
static void ABI probe_release_resource(uint64_t pair[2]) {
    assert(pair[0] && pair[1]);unsigned *refs=(void *)(uintptr_t)pair[1];assert(*refs);
    if (!--*refs) { free((void *)(uintptr_t)pair[0]);free(refs);--probe_resources; }
}
static void ABI probe_delete_texture(uintptr_t texture,unsigned deleting) {
    assert(deleting==1);uint64_t handle,refs,resource[2];
    memcpy(&handle,(void *)(texture+0x70),8);memcpy(&refs,(void *)(texture+0x58),8);
    assert(*(unsigned *)(uintptr_t)refs>1);--*(unsigned *)(uintptr_t)refs;
    memcpy(resource,(void *)(texture+0x60),16);probe_release_resource(resource);
    void *pixels;memcpy(&pixels,(void *)(handle+0x80),8);free(pixels);free((void *)handle);free((void *)texture);--probe_objects;
}
static void ABI probe_delete_mapper(uintptr_t mapping,unsigned deleting) { assert(deleting==1);free((void *)mapping);--probe_maps; }
static unsigned ABI probe_map(uintptr_t mapping,void *output,unsigned level) {
    assert(!level);uintptr_t texture,handle;void *pixels;
    memcpy(&texture,(void *)(mapping+8),8);memcpy(&handle,(void *)(texture+0x70),8);
    memcpy(&pixels,(void *)(handle+0x80),8);assert(pixels);
    putptr(output,pixels);put32((unsigned char *)output+8,32);put32((unsigned char *)output+12,256);
    mapped_probe=(uintptr_t)pixels;return 0;
}
static unsigned ABI probe_unmap(uintptr_t mapping,unsigned level) { assert(mapping && !level);mapped_probe=0;return 0; }
static uintptr_t ABI probe_get_map(uintptr_t texture,uint64_t out[2],unsigned mode,unsigned char dynamic) {
    static const uintptr_t table[]={(uintptr_t)probe_delete_mapper,(uintptr_t)probe_map,(uintptr_t)probe_unmap};
    assert(mode==2 && !dynamic);unsigned char *mapping=calloc(1,16);assert(mapping);
    putptr(mapping,(void *)table);putptr(mapping+8,(void *)texture);
    out[0]=(uintptr_t)mapping;out[1]=(uintptr_t)calloc(1,4);assert(out[1]);*(unsigned *)(uintptr_t)out[1]=1;++probe_maps;
    return (uintptr_t)out;
}
static void ABI probe_string(void *out,const char *text,size_t length) {
    assert(length==strlen(text));char *copy=malloc(length+1);assert(copy);memcpy(copy,text,length+1);
    putptr(out,copy);
}
static void ABI probe_string_delete(void *str) { void *p;memcpy(&p,str,8);free(p); }
static uintptr_t ABI probe_resource(uintptr_t owner,uint64_t out[2],void *name) {
    const char *text;memcpy(&text,name,8);assert(owner && !strncmp(text,"JCEnv_",6));
    out[0]=(uintptr_t)calloc(1,16);out[1]=(uintptr_t)calloc(1,4);assert(out[0] && out[1]);
    *(unsigned *)(uintptr_t)out[1]=1;++probe_resources;return (uintptr_t)out;
}
static void ABI probe_resource_init(uintptr_t resource,unsigned count,uintptr_t type,unsigned flags) {
    assert(resource && count==1 && type==(uintptr_t)image+0x3c11060 && !flags);
}
static uintptr_t ABI probe_object_alloc(size_t bytes) { assert(bytes==0x120);return (uintptr_t)calloc(1,bytes); }
static uintptr_t ABI probe_construct(uintptr_t texture,const uint64_t ns[2],const uint64_t resource[2]) {
    static const uintptr_t table[]={(uintptr_t)probe_delete_texture};
    unsigned char *handle=calloc(1,0xa0);assert(handle);
    putptr((void *)texture,(void *)table);memcpy((void *)(texture+0x50),ns,16);memcpy((void *)(texture+0x60),resource,16);
    ++*(unsigned *)(uintptr_t)ns[1];++*(unsigned *)(uintptr_t)resource[1];
    putptr((unsigned char *)texture+0x70,handle);put32((unsigned char *)texture+0x110,3);put32(handle+0x30,1);
    probe_texture=texture;probe_handle=(uintptr_t)handle;++probe_objects;return texture;
}
static uintptr_t ABI probe_select(uintptr_t world,const float *low,const float *high,unsigned filter) {
    assert(world==0x100000 && filter==0x22 && low[0]<high[0]);++probe_queries;return (uintptr_t)probe_query;
}
static unsigned char ABI probe_not_allocated(uintptr_t handle) {
    assert(handle==probe_handle);return 0;
}
static unsigned ABI probe_resize(uintptr_t handle,unsigned rings) {
    assert(handle==probe_handle && rings==3);
    put32((unsigned char *)handle+0x30,rings);++probe_resizes;return 0;
}
static void ABI probe_sampler(uintptr_t sampler) {
    const unsigned char expected[5]={5,1,2,2,2};
    assert(sampler==probe_texture+8 && !memcmp((void *)sampler,expected,5));++probe_samplers;
}
static unsigned ABI probe_allocate_gpu(uintptr_t handle) {
    assert(handle==probe_handle);++probe_gpu_calls;
    if (probe_pipeline && !probe_gpu_error) { void *pixels=calloc(1,1536);assert(pixels);putptr((unsigned char *)handle+0x80,pixels); }
    return probe_gpu_error;
}
static void native_environment_pipeline(void) {
    unsigned char *barn=calloc(1,0x646d50),game[0x120]={0};assert(barn);
    unsigned nsrefs=1;uint64_t ns[2]={0x90000,(uintptr_t)&nsrefs};memcpy(barn+0x646d10,ns,16);
    unsigned char stock[2][0x120]={0},handles[2][0xa0]={0};
    for (unsigned i=0;i<2;++i) {
        putptr(stock[i]+0x50,(void *)(uintptr_t)ns[0]);putptr(stock[i]+0x60,(void *)(uintptr_t)(0xa0000+i*256));
        putptr(stock[i]+0x70,handles[i]);put32(stock[i]+0x110,3);
        memcpy(stock[i]+8,(unsigned char[]){5,1,2,2,2},5);
        put32(handles[i]+0x18,0x1c);put32(handles[i]+0x28,8);put32(handles[i]+0x30,3);
        putptr(barn+0x646d30+i*16,stock[i]);
    }
    _Alignas(16) unsigned char node[0xd0]={0};
    _Alignas(16) float vertices[4][4]={{-1,0,-1,1},{1,0,-1,1},{1,2,-1,1},{-1,2,-1,1}};
    unsigned char triangles[6]={0,3,2,0,2,1},faces[1]={2};
    putfloat(node+0x10,-1);putfloat(node+0x18,-1);putfloat(node+0x20,1);putfloat(node+0x24,2);putfloat(node+0x28,-1);
    putfloat(node+0x40,.1f);putfloat(node+0x44,.3f);putfloat(node+0x48,.8f);
    putptr(node+0x50,triangles);putptr(node+0x58,faces);putptr(node+0x68,vertices);
    put32(node+0x70,2);put32(node+0x74,1);put32(node+0x7c,4);put32(node+0x80,1);
    putptr(probe_query+0x18,node);putptr(game+0x110,(void *)0x100000);
    struct jc_environment_probe probes[2]={0};probe_pipeline=1;
    for (unsigned i=0;i<2;++i)
        assert(!jc_environment_probe_create(atlas_fixture_read,NULL,(uintptr_t)image,(uintptr_t)barn,11+i,1,i+1,probes+i));
    assert(nsrefs==3 && probe_resources==2 && probe_objects==2);
    assert(probes[0].identity.handle!=probes[1].identity.handle);
    const float centers[2][4]={{0,1,3,0},{0,1,-3,0}};
    unsigned char outputs[2][1536];
    for (unsigned i=0;i<2;++i) {
        assert(!jc_environment_probe_update(atlas_fixture_read,NULL,(uintptr_t)game,centers[i],probes+i));
        assert(probes[i].produced && !probe_maps && !mapped_probe);
        void *pixels;memcpy(&pixels,(void *)(uintptr_t)(probes[i].identity.handle+0x80),8);memcpy(outputs[i],pixels,1536);
        probes[i].pins=1;
        assert(jc_environment_probe_update(atlas_fixture_read,NULL,(uintptr_t)game,centers[i],probes+i));
        assert(jc_environment_probe_destroy(atlas_fixture_read,NULL,probes+i));probes[i].pins=0;
    }
    assert(probe_queries==2 && memcmp(outputs[0],outputs[1],1536));
    triangles[0]=255;
    assert(jc_environment_probe_update(atlas_fixture_read,NULL,(uintptr_t)game,centers[0],probes));
    assert(!probes[0].produced && !probes[0].quarantined && !probes[0].scene && !probe_maps);
    triangles[0]=0;
    put32(probe_query+0x10,0x7fc00000);
    assert(jc_environment_probe_update(atlas_fixture_read,NULL,(uintptr_t)game,centers[0],probes));
    assert(!probes[0].quarantined && !probes[0].scene && !probe_maps);
    put32(probe_query+0x10,0);
    putptr(probe_query+0x20,probe_query);
    assert(jc_environment_probe_update(atlas_fixture_read,NULL,(uintptr_t)game,centers[0],probes));
    assert(!probes[0].quarantined && !probes[0].scene && !probe_maps);
    putptr(probe_query+0x20,NULL);
    void *unchanged;memcpy(&unchanged,(void *)(uintptr_t)(probes[0].identity.handle+0x80),8);
    assert(!memcmp(unchanged,outputs[0],1536));
    assert(!jc_environment_probe_update(atlas_fixture_read,NULL,(uintptr_t)game,centers[0],probes));
    assert(probes[0].produced && !memcmp(unchanged,outputs[0],1536));
    for (unsigned i=0;i<2;++i) assert(!jc_environment_probe_destroy(atlas_fixture_read,NULL,probes+i));
    assert(nsrefs==1 && !probe_objects && !probe_resources && !probe_maps);free(barn);probe_pipeline=0;
    puts("Environment owner pipeline: production creation/query/private-copy/map/native-job/unmap/retirement, two identities, lease refusals, malformed/cyclic query refusal before mapping, retry and balanced native refcounts; resource factory, spatial query and GPU services intercepted, no live GPU");
}
static void native_environment_configuration(void) {
    unsigned char textures[3][0x120]={0},handles[3][0xa0]={0};
    for (unsigned i=0;i<3;++i) {
        putptr(textures[i]+0x50,(void *)(uintptr_t)0x100000);
        putptr(textures[i]+0x60,(void *)(uintptr_t)(0x200000+i*0x100));
        putptr(textures[i]+0x70,handles[i]);put32(textures[i]+0x110,3);
        put32(handles[i]+0x18,0x220);put32(handles[i]+0x30,1);
    }
    unsigned char stock_texture[0x120],stock_handle[0xa0];
    memcpy(stock_texture,textures[0],sizeof(stock_texture));memcpy(stock_handle,handles[0],sizeof(stock_handle));
    for (unsigned i=1;i<3;++i) {
        struct jc_environment_texture result;
        probe_texture=(uintptr_t)textures[i];probe_handle=(uintptr_t)handles[i];
        assert(!jc_environment_texture_configure(atlas_fixture_read,NULL,(uintptr_t)image,probe_texture,&result));
        assert(jc_environment_texture_ready(&result) && result.handle==probe_handle);
        uint64_t format;memcpy(&format,handles[i],8);assert(format==(uintptr_t)image+0x3bf4f80);
        assert(!memcmp(stock_texture,textures[0],sizeof(stock_texture)));
        assert(!memcmp(stock_handle,handles[0],sizeof(stock_handle)));
    }
    assert(probe_resizes==2 && probe_samplers==2 && probe_gpu_calls==2);
    struct jc_environment_texture result={0};
    put32(textures[2]+0x24,0x80);
    assert(jc_environment_texture_configure(atlas_fixture_read,NULL,(uintptr_t)image,probe_texture,&result));
    assert(!result.texture && probe_gpu_calls==2);put32(textures[2]+0x24,0);
    probe_gpu_error=7;
    assert(jc_environment_texture_configure(atlas_fixture_read,NULL,(uintptr_t)image,probe_texture,&result));
    assert(!result.texture && probe_gpu_calls==3);probe_gpu_error=0;
    puts("Native environment configuration: original cube size/format, mip and ring setters; two private identities, native sampler bytes, preserved stock, immutable texture and GPU-error refusal; allocation/sampler services intercepted, no GPU");
}
static void native_appearance_routes(void) {
    struct material_fixture {
        unsigned char material[64],slot[32],link[32],payload[160],collection[16],member[16];
        _Alignas(16) unsigned char buffer[128];
    } m[4]={0};
    uint16_t offsets[3]={4,16,32},widths[3]={4,16,64};
    unsigned char *fields[3]={scalar_field,vector_field,matrix_field};
    for (unsigned i=0;i<3;++i) {
        memcpy(fields[i]+0x10,offsets+i,2);memcpy(fields[i]+0x12,widths+i,2);
    }
    for (unsigned i=0;i<4;++i) {
        putptr(m[i].material+0x30,m[i].slot);putptr(m[i].slot+0x10,m[i].link);
        putptr(m[i].link+0x18,m[i].payload);putptr(m[i].payload+0x80,m[i].collection);
        put32(m[i].collection,1);putptr(m[i].collection+8,m[i].member);
        putptr(m[i].member,scalar_field);putptr(m[i].member+8,m[i].buffer);put32(m[i].buffer,128);
    }
    uint64_t sources[2],targets[2];struct jc_cloth_storage stock[2]={0},owned[2]={0};
    for (unsigned i=0;i<2;++i) {
        sources[i]=(uintptr_t)m[i].material;targets[i]=(uintptr_t)m[i+2].material;
        stock[i].payload=(uintptr_t)m[i].payload;owned[i].payload=(uintptr_t)m[i+2].payload;
        stock[i].count=owned[i].count=1;
        stock[i].spans[0].begin=(uintptr_t)m[i].buffer;stock[i].spans[0].bytes=128;
        owned[i].spans[0].begin=(uintptr_t)m[i+2].buffer;owned[i].spans[0].bytes=128;
    }
    struct jc_cloth_appearance_route route;
    assert(!jc_cloth_appearance_route_make(sources,targets,stock,owned,&route));
    void (ABI *vector)(uintptr_t,const char *,const void *)=(void *)(image+0x273200);
    void (ABI *matrix)(uintptr_t,const char *,const void *)=(void *)(image+0x273400);
    float *(ABI *scalar)(uintptr_t,const char *)=(void *)(image+0x273a00);
    unsigned char before[2][128];memcpy(before[0],m[0].buffer,128);memcpy(before[1],m[1].buffer,128);
    for (unsigned i=0;i<2;++i) {
        _Alignas(16) float values[16];for (unsigned j=0;j<16;++j) values[j]=(float)(i*20+j+1);
        uintptr_t target=jc_cloth_appearance_target(&route,sources[i],0);
        vector(target,"vector",values);matrix(target,"matrix",values);
        float *destination=scalar(jc_cloth_appearance_target(&route,stock[i].payload,1),"scalar");
        assert((void *)destination==m[i+2].buffer+4);*destination=values[0];
        assert(!memcmp(m[i+2].buffer+16,values,16));assert(!memcmp(m[i+2].buffer+32,values,64));
        assert(!memcmp(before[i],m[i].buffer,128));
    }
    puts("Native appearance routing: original vector/matrix setters and scalar lookup write two private destinations, preserve stock buffers; name-table service intercepted, no texture/GPU runtime");
}
int main(void) {
    splice_cases();
    image=mmap(NULL,0x4050000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED); load_pieces(image);
    image[0x49a820]=0xc3; /* fixture stack already has ample space; retain RAX */
    thunk(0x2709a0,uniform_field);putptr(image+0x5537a8,plain_parameter);
    thunk(0x49ba58,copy_bytes); thunk(0x49ba88,fill_bytes);
    thunk(0x91d90,pool); thunk(0x91c80,ref_allocate); thunk(0x91ac0,ref_release);
    thunk(0x35b0f0,input_layout);
    thunk(0x346440,finalize_rotate); thunk(0x347ff0,finalize_rotate);
    thunk(0x267d10,finalize_map); thunk(0xc0c60,finalize_map); thunk(0x8e210,finalize_release);
    thunk(0x368c50,probe_not_allocated);thunk(0x368d50,probe_resize);
    thunk(0x362db0,probe_sampler);thunk(0x282dd0,probe_allocate_gpu);
    thunk(0x7b920,probe_string);thunk(0x7b7c0,probe_string_delete);
    thunk(0x2d0330,probe_resource);thunk(0x341e70,probe_resource_init);thunk(0x340f10,probe_object_alloc);
    thunk(0x2d8480,probe_construct);thunk(0xc0d70,probe_release_resource);
    thunk(0xea040,probe_select);thunk(0x2d7fd0,probe_get_map);
    putptr(image+0x5539d8,allocate_bytes); putptr(image+0x5539e0,release_bytes);
    assert(!mprotect(image,0x4050000,PROT_READ));
    assert(!mprotect(image+0x1d9000,0x3000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x3b9000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x37f000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x2d8000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x368000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x362000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x282000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0xe9000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0xea000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x7b000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x2d0000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x340000,0x2000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x2d7000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x8a000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x8e000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x78000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x7a000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x85000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x2cc000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x2da000,0x2000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x363000,0x2000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0xc0000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x267000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x270000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x273000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x346000,0x2000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x1e2000,0x5000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x490000,0x10000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x91000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x36d000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x372000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x345000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x34e000,0x1000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x35b000,0x3000,PROT_READ|PROT_EXEC));
    assert(!mprotect(image+0x3e1000,0x1000,PROT_READ|PROT_EXEC));
    native_atlas_subscribers();native_appearance_routes();native_texture_bindings();native_environment_jobs();
    native_environment_configuration();
    native_environment_pipeline();
    worker=(worker_fn)(void *)(image+0x1e2d10); cases(); parallel_cases(); frame_cases();
    worker=(worker_fn)(void *)(image+0x1e4970); cases(); parallel_cases(); frame_cases();
    draw_cases();
    staging_cases();
    publication_cases();
    finalizer_cases();
    puts("Integrated cloth frames: 64 complete cycles, 384 native worker calls and 256 native indexed draws, distinct sender material identities, delayed worker/render completion and balanced fixture resource leases; native resource backend and GPU not executed");
    printf("Native cloth: %u original worker calls; both implementations, stock plus two recipients, robe/scarf roles, subdivision, hidden jobs, shuffled routing, 12 concurrent workers, exact vertex/index equivalence and untouched padding passed; no GPU or Journey runtime\n",atomic_load(&calls));
    assert(!munmap(image,0x740000));
    return 0;
}
