#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#else
#define JC_RETIREMENT_TEST_PLATFORM
#define STDMETHODCALLTYPE
typedef int32_t HRESULT;
typedef int BOOL;
typedef unsigned UINT,ULONG,SRWLOCK,DWORD;
typedef enum { D3D11_DEVICE_CONTEXT_IMMEDIATE, D3D11_DEVICE_CONTEXT_DEFERRED } D3D11_DEVICE_CONTEXT_TYPE;
typedef struct { unsigned Query,MiscFlags; } D3D11_QUERY_DESC;
#define D3D11_QUERY_EVENT 0
#define D3D11_ASYNC_GETDATA_DONOTFLUSH 1
#define S_OK 0
#define FALSE 0
#define FAILED(x) ((x)<0)
typedef struct ID3D11Query ID3D11Query;
typedef ID3D11Query ID3D11Asynchronous;
typedef struct ID3D11Device ID3D11Device;
typedef struct ID3D11DeviceContext ID3D11DeviceContext;
typedef struct ID3D11Buffer ID3D11Buffer;
typedef ID3D11Buffer ID3D11Resource;
typedef struct { UINT ByteWidth,Usage,BindFlags,CPUAccessFlags,MiscFlags,StructureByteStride; } D3D11_BUFFER_DESC;
typedef struct { const void *pSysMem; UINT SysMemPitch,SysMemSlicePitch; } D3D11_SUBRESOURCE_DATA;
typedef struct { void *pData; UINT RowPitch,DepthPitch; } D3D11_MAPPED_SUBRESOURCE;
typedef enum { D3D11_MAP_WRITE_DISCARD=4 } D3D11_MAP;
#define D3D11_USAGE_DYNAMIC 2
#define D3D11_BIND_VERTEX_BUFFER 1
#define D3D11_BIND_INDEX_BUFFER 2
#define D3D11_CPU_ACCESS_WRITE 0x10000
typedef struct { ULONG (*Release)(ID3D11Buffer *); } ID3D11BufferVtbl;
struct ID3D11Buffer { const ID3D11BufferVtbl *lpVtbl; };
typedef struct { ULONG (*Release)(ID3D11Query *); } ID3D11QueryVtbl;
struct ID3D11Query { const ID3D11QueryVtbl *lpVtbl; };
typedef struct {
    ULONG (*Release)(ID3D11Device *);
    HRESULT (*CreateQuery)(ID3D11Device *,const D3D11_QUERY_DESC *,ID3D11Query **);
    HRESULT (*CreateBuffer)(ID3D11Device *,const D3D11_BUFFER_DESC *,const D3D11_SUBRESOURCE_DATA *,ID3D11Buffer **);
} ID3D11DeviceVtbl;
struct ID3D11Device { const ID3D11DeviceVtbl *lpVtbl; };
typedef struct {
    D3D11_DEVICE_CONTEXT_TYPE (*GetType)(ID3D11DeviceContext *);
    void (*GetDevice)(ID3D11DeviceContext *,ID3D11Device **);
    ULONG (*AddRef)(ID3D11DeviceContext *);
    ULONG (*Release)(ID3D11DeviceContext *);
    void (*End)(ID3D11DeviceContext *,ID3D11Asynchronous *);
    HRESULT (*GetData)(ID3D11DeviceContext *,ID3D11Asynchronous *,void *,UINT,UINT);
    HRESULT (*Map)(ID3D11DeviceContext *,ID3D11Resource *,UINT,D3D11_MAP,UINT,D3D11_MAPPED_SUBRESOURCE *);
    void (*Unmap)(ID3D11DeviceContext *,ID3D11Resource *,UINT);
    void (*Flush)(ID3D11DeviceContext *);
} ID3D11DeviceContextVtbl;
struct ID3D11DeviceContext { const ID3D11DeviceContextVtbl *lpVtbl; };
static unsigned GetCurrentThreadId(void) { return 42; }
static void AcquireSRWLockExclusive(SRWLOCK *s) { assert(!*s); *s=1; }
static void ReleaseSRWLockExclusive(SRWLOCK *s) { assert(*s); *s=0; }
#endif
#include "native_render_retirement.h"
#include "native_cloth_material_live.h"
#ifndef _WIN32
#define JC_CLOTH_LIVE_TEST_PLATFORM
/* Material construction has its own fixture. This COM fixture exercises no
   material registrations; these shims must never be called. */
struct jc_cloth_live_material {
    unsigned char *subscriber;
    uint64_t peer,generation,model,control;
    DWORD thread;
    unsigned role,pins;
    int ready,quarantined;
    struct jc_cloth_storage storage;
};
static int jc_cloth_live_material_pin(jc_layout_reader r,void *c,struct jc_cloth_live_material *m,uint64_t p,uint64_t g) {
    (void)r;(void)c;(void)m;(void)p;(void)g; assert(0); return 1;
}
static int jc_cloth_live_material_unpin(struct jc_cloth_live_material *m) { (void)m; assert(0); return 1; }
static int jc_cloth_live_material_destroy(jc_layout_reader r,void *c,struct jc_cloth_live_material *m) {
    (void)r;(void)c;(void)m; assert(0); return 1;
}
#endif
#include "native_cloth_live.h"
#ifndef _WIN32
#define JC_CLOTH_RENDER_TEST_PLATFORM
#define SRWLOCK_INIT 0
#define TLS_OUT_OF_INDEXES ((DWORD)-1)
#define HEAP_ZERO_MEMORY 8
static void *runtime_tls;
static DWORD TlsAlloc(void) { return 0; }
static void *TlsGetValue(DWORD i) { assert(!i); return runtime_tls; }
static int TlsSetValue(DWORD i,void *p) { assert(!i); runtime_tls=p; return 1; }
static void *GetProcessHeap(void) { return (void *)1; }
static void *HeapAlloc(void *h,unsigned flags,size_t bytes) { assert(h && flags==8); return calloc(1,bytes); }
static int HeapFree(void *h,unsigned flags,void *p) { assert(h && !flags); free(p); return 1; }
static int jc_cloth_live_bindings_match(struct jc_cloth_live *s,const struct jc_cloth_material *d) {
    (void)s;return d->peer==77;
}
#endif
#include "native_cloth_render_runtime.h"
#include "native_actor_frame.h"
static unsigned context_refs,query_refs,device_refs,ends,polls,deferred,create_failure;
static unsigned flushes;
static HRESULT query_result;
static BOOL query_complete;
static ID3D11Query query;
static ID3D11Device device;
static ID3D11Buffer buffers[2];
static unsigned buffer_refs[2],mapped[2],created,map_failure,unmaps;
static unsigned char buffer_data[2][4096];
static ULONG STDMETHODCALLTYPE buffer_release(ID3D11Buffer *b) {
    unsigned i=(unsigned)(b-buffers); assert(i<2 && buffer_refs[i] && !mapped[i]); return --buffer_refs[i];
}
static HRESULT STDMETHODCALLTYPE create_buffer(ID3D11Device *d,const D3D11_BUFFER_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data,ID3D11Buffer **out) {
    assert(d==&device && created<2 && !data && desc->ByteWidth==4096 &&
        desc->Usage==D3D11_USAGE_DYNAMIC && desc->CPUAccessFlags==D3D11_CPU_ACCESS_WRITE &&
        desc->BindFlags==(created ? D3D11_BIND_INDEX_BUFFER : D3D11_BIND_VERTEX_BUFFER));
    buffer_refs[created]=1; *out=buffers+created++; return S_OK;
}
static HRESULT STDMETHODCALLTYPE map_buffer(ID3D11DeviceContext *c,ID3D11Resource *resource,
        UINT sub,D3D11_MAP mode,UINT flags,D3D11_MAPPED_SUBRESOURCE *out) {
    (void)c; unsigned i=(unsigned)((ID3D11Buffer *)resource-buffers);
    assert(i<2 && buffer_refs[i] && !mapped[i] && !sub && mode==D3D11_MAP_WRITE_DISCARD && !flags);
    if (map_failure) return (HRESULT)-1;
    mapped[i]=1; out->pData=buffer_data[i]; return S_OK;
}
static void STDMETHODCALLTYPE unmap_buffer(ID3D11DeviceContext *c,ID3D11Resource *resource,UINT sub) {
    (void)c; unsigned i=(unsigned)((ID3D11Buffer *)resource-buffers);
    assert(i<2 && mapped[i] && !sub); mapped[i]=0; ++unmaps;
}
static ULONG STDMETHODCALLTYPE query_release(ID3D11Query *q) { assert(q==&query && query_refs); return --query_refs; }
static ULONG STDMETHODCALLTYPE device_release(ID3D11Device *d) { assert(d==&device && device_refs); return --device_refs; }
static HRESULT STDMETHODCALLTYPE create_query(ID3D11Device *d,const D3D11_QUERY_DESC *desc,ID3D11Query **out) {
    assert(d==&device && desc->Query==D3D11_QUERY_EVENT && !desc->MiscFlags);
    if (create_failure) return (HRESULT)-1;
    ++query_refs; *out=&query; return S_OK;
}
static D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE get_type(ID3D11DeviceContext *c) {
    (void)c; return deferred ? D3D11_DEVICE_CONTEXT_DEFERRED : D3D11_DEVICE_CONTEXT_IMMEDIATE;
}
static void STDMETHODCALLTYPE get_device(ID3D11DeviceContext *c,ID3D11Device **out) { (void)c;++device_refs;*out=&device; }
static ULONG STDMETHODCALLTYPE add_context(ID3D11DeviceContext *c) { (void)c;return ++context_refs; }
static ULONG STDMETHODCALLTYPE release_context(ID3D11DeviceContext *c) { (void)c;assert(context_refs);return --context_refs; }
static void STDMETHODCALLTYPE end_query(ID3D11DeviceContext *c,ID3D11Asynchronous *q) {
    (void)c;assert(q==(ID3D11Asynchronous *)&query && query_refs && context_refs);++ends;
}
static HRESULT STDMETHODCALLTYPE get_data(ID3D11DeviceContext *c,ID3D11Asynchronous *q,void *data,UINT bytes,UINT flags) {
    (void)c;assert(q==(ID3D11Asynchronous *)&query && bytes==sizeof(BOOL) && flags==D3D11_ASYNC_GETDATA_DONOTFLUSH);
    ++polls; *(BOOL *)data=query_complete; return query_result;
}
static void STDMETHODCALLTYPE flush_context(ID3D11DeviceContext *c) { (void)c; ++flushes; }
static struct jc_live_retirement *publishing_retirement;
static ID3D11DeviceContext *publishing_context;
static unsigned workers_complete,discarded,prepared,prepare_mode,asynchronous;
static _Alignas(16) unsigned char commands[128];
static int no_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx;(void)address;(void)out;(void)bytes; assert(0); return 0;
}
static int workers_done(void *ctx,uint64_t generation) { (void)ctx; assert(generation==1); return (int)workers_complete; }
static int prepare(void *ctx,uint64_t generation,const struct jc_cloth_frame_draw *draws,unsigned n,
        struct jc_cloth_live_commands *out) {
    (void)ctx; assert(generation==1 && draws && !n && !mapped[0] && !mapped[1]);
    *out=(struct jc_cloth_live_commands){.executor=1,.context=2,.fence=3,
        .begin=(uintptr_t)commands,.end=(uintptr_t)commands+sizeof(commands),.owner=commands};
    if (prepare_mode==2) out->end=out->begin;
    ++prepared; return prepare_mode==1 ? 0 : 1;
}
static void discard(void *ctx,struct jc_cloth_live_commands *c) {
    (void)ctx; assert(c->owner==commands && !mapped[0] && !mapped[1]); ++discarded;
}
static uint32_t handoff(uintptr_t executor,uintptr_t context,uintptr_t fence,uintptr_t begin,uintptr_t end) {
    assert(context==2 && fence==3);
    if (asynchronous) return 0;
    unsigned mask=jc_live_retirement_enter(publishing_retirement,executor,begin,end);
    assert(mask); /* synchronous execution must already be armed */
    jc_live_retirement_leave(publishing_retirement,mask,0,publishing_context); return 0;
}
static int no_atlas_pin(void *ctx,uint64_t object) { (void)ctx;(void)object; assert(0); return 0; }
static void no_atlas_unpin(void *ctx,uint64_t object) { (void)ctx;(void)object; assert(0); }
static void cloth_worker(void *ctx,void *worker,const void *entry,void *args,void *streams,void *desc,void *state) {
    (void)ctx;(void)worker;(void)entry;(void)streams;(void)desc;(void)state;
    uint32_t v=3,i=6;
    memcpy((unsigned char *)args+0x210+4*4,&v,4);
    memcpy((unsigned char *)args+0x22c+4*4,&i,4);
    assert(mapped[0] && mapped[1]); buffer_data[0][0]=42;
}
static void integration(ID3D11DeviceContext *context) {
    created=workers_complete=discarded=prepared=unmaps=0;
    struct jc_live_retirement retirement={0};
    struct jc_cloth_live *s=calloc(1,sizeof(*s)); assert(s);
    struct jc_cloth_live_ops ops={NULL,workers_done,prepare,discard,handoff,no_atlas_pin,no_atlas_unpin};
    assert(!jc_cloth_live_init(s,context,&retirement,no_read,NULL,ops));
    struct jc_cloth_live_buffer *vb,*ib;
    assert(!jc_cloth_live_buffer_create(s,JC_FRAME_VERTEX,4096,&vb));
    assert(!jc_cloth_live_buffer_create(s,JC_FRAME_INDEX,4096,&ib));
    struct jc_cloth_partition plan={.ready=1};
    plan.vertex_capacity[4]=64;plan.index_capacity[4]=256;
    struct jc_cloth_stream prepared_streams[7];
    assert(!jc_cloth_live_streams_prepare(s,&plan,prepared_streams));
    assert(created==2 && prepared_streams[4].vertex==(uintptr_t)vb->object &&
        prepared_streams[4].index==(uintptr_t)ib->object && vb->mapped && ib->mapped);
    jc_cloth_live_streams_abandon(s,prepared_streams);
    assert(!vb->mapped && !ib->mapped && unmaps==2 && !prepared_streams[4].vertex);
    map_failure=1;
    assert(jc_cloth_live_streams_prepare(s,&plan,prepared_streams)==JC_FRAME_RETAIN);
    assert(!vb->mapped && !ib->mapped && !prepared_streams[4].vertex);
    map_failure=0;unmaps=0;
    map_failure=1; assert(jc_cloth_live_buffer_map(s,vb)==JC_FRAME_RETAIN && !vb->mapped);
    map_failure=0; assert(!jc_cloth_live_buffer_map(s,vb) && !jc_cloth_live_buffer_map(s,ib));
    unsigned char args[JC_CLOTH_WORKER_ARGS]={0},stream_args[0x130]={0},entry[10]={0},state[0xe0]={0};
    struct jc_cloth_stream streams[7]={0};
    streams[4]=(struct jc_cloth_stream){.vertex=(uintptr_t)vb->object,.index=(uintptr_t)ib->object,
        .vertex_bytes=4096,.index_bytes=4096,.vertices=64,.indices=256,
        .mapped_vertices=vb->mapped,.mapped_indices=ib->mapped};
    struct jc_cloth_partition_job job={0,0,4,3,6}; unsigned index;
    streams[4].mapped_vertices=(unsigned char *)vb->mapped+4;
    assert(jc_cloth_live_begin(s,1,&job,1,args,streams,NULL,0,&index)==JC_FRAME_OWNERSHIP);
    streams[4].mapped_vertices=vb->mapped;
    assert(!jc_cloth_live_begin(s,1,&job,1,args,streams,NULL,0,&index));
    assert(vb->pins==1 && ib->pins==1 && jc_cloth_live_buffer_map(s,vb)==JC_FRAME_BUSY);
    uint32_t role=4,vertices=3,indices=6;
    memcpy(entry+4,&role,4); entry[9]=1;
    memcpy(state+0x54,&vertices,4); memcpy(state+0x5c,&indices,4);
    memcpy(stream_args+0x10+4*8,&vb->mapped,8); memcpy(stream_args+0x48+4*8,&ib->mapped,8);
    assert(!jc_cloth_frame_run(s->frames.frames+index,1,NULL,cloth_worker,NULL,entry,stream_args,args,state));
    assert(jc_cloth_frame_seal(&s->frames,index)==JC_FRAME_BUSY && !unmaps);
    assert(jc_cloth_live_unmap(s,index)==JC_FRAME_BUSY);
    workers_complete=1; assert(!jc_cloth_frame_seal(&s->frames,index) && !unmaps);
    assert(!jc_cloth_frame_commit_stock(s->frames.frames+index,1,args,sizeof(args)));
    assert(jc_cloth_frame_submit(&s->frames,index)==JC_FRAME_QUEUE && !prepared);
    assert(!jc_cloth_live_unmap(s,index) && unmaps==2);
    publishing_retirement=&retirement; publishing_context=context;
    query_result=1; query_complete=0;
    for (prepare_mode=1;prepare_mode<=2;++prepare_mode) {
        assert(jc_cloth_frame_submit(&s->frames,index)==JC_FRAME_QUEUE);
        assert(vb->pins==1 && !s->frames.frames[index].ticket);
    }
    prepare_mode=0;
    assert(!jc_cloth_frame_submit(&s->frames,index) && prepared==3 && discarded==2);
    assert(jc_cloth_live_close(s)==JC_FRAME_BUSY && vb->pins==1 && buffer_refs[0]==1);
    assert(jc_cloth_live_begin(s,2,&job,1,args,streams,NULL,0,&index)!=JC_FRAME_OK);
    assert(jc_cloth_live_buffer_map(s,vb)==JC_FRAME_BUSY);
    if (asynchronous) {
        uint64_t ticket=s->frames.frames[index].ticket;
        assert(jc_live_retirement_result(&retirement,ticket,0)==0 && !query_refs);
        unsigned mask=jc_live_retirement_enter(&retirement,1,(uintptr_t)commands,(uintptr_t)commands+128);
        assert(mask); jc_live_retirement_leave(&retirement,mask,0,context);
    }
    query_result=0; query_complete=1; jc_live_retirement_poll(&retirement);
    assert(!jc_cloth_live_close(s) && discarded==3 && !buffer_refs[0] && !buffer_refs[1]);
    assert(!jc_cloth_live_close(s) && discarded==3 && !context_refs && !query_refs && !device_refs);
    free(s);
}
static unsigned runtime_submits,runtime_releases;
static void runtime_release(void *ctx,unsigned kind,uint64_t object) {
    (void)ctx;assert(kind==JC_FRAME_VERTEX && object==66);++runtime_releases;
}
static int runtime_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx;if (!address) return 0;memcpy(out,(void *)(uintptr_t)address,bytes);return 1;
}
static uint32_t runtime_submit(uintptr_t executor,uintptr_t reservation,uintptr_t fence,uintptr_t begin,uintptr_t end) {
    assert(executor && reservation==123 && fence==456 && end-begin==128);++runtime_submits;return 0;
}
static uint32_t runtime_base_submit(uintptr_t executor,uintptr_t begin,uintptr_t end,uintptr_t fence) {
    return runtime_submit(executor,123,fence,begin,end);
}
#ifndef _WIN32
static unsigned registration_commits;
static void JC_COMMAND_ABI registration_commit(uintptr_t material) { assert(material);++registration_commits; }
static void registration_cases(void) {
    for (unsigned failure=0;failure<10;++failure) {
        uint64_t before_counts[2][7][4];memcpy(before_counts,jc_cloth_render_counts,sizeof(before_counts));
        struct jc_cloth_live *s=calloc(1,sizeof(*s));
        unsigned char *barn=calloc(1,0x66a000),*queue=calloc(1,0x10100);
        assert(s && barn && queue);
        s->thread=GetCurrentThreadId();s->read=runtime_read;
        struct jc_cloth_frame *f=s->frames.frames;
        f->phase=JC_FRAME_READY;f->stock_committed=1;f->draw_count=2;f->generation=1;
        uint64_t model[4][16]={{0}},item[4][8]={{0}},geometry[4][8]={{0}},collection[4][2]={{0}},binding[4][3]={{11,12,13},{21,22,23},{31,32,33},{41,42,43}},members[4][3],items[4];
        uint32_t batches[4]={2,2,2,2};
        unsigned char subscribers[2][0x90]={{0}},context[0x20]={0};int controls[2]={1,1};
        struct jc_cloth_live_material materials[2]={0};
        for (unsigned n=0;n<4;++n) {
            items[n]=(uintptr_t)item[n];model[n][12]=(uintptr_t)&items[n];model[n][13]=(uintptr_t)(&items[n]+1);
            item[n][0]=(uintptr_t)geometry[n];item[n][4]=(uintptr_t)collection[n];
            geometry[n][0]=1;geometry[n][1]=(uintptr_t)&batches[n];
            for (unsigned k=0;k<3;++k) members[n][k]=(uintptr_t)&binding[n][k];
            collection[n][0]=3;collection[n][1]=(uintptr_t)members[n];
            if (n<2) jc_command_ptr(barn+0x666ed8+(n+4)*0x630,(uintptr_t)model[n]);
        }
        for (unsigned n=0;n<2;++n) {
            materials[n]=(struct jc_cloth_live_material){.subscriber=subscribers[n],.model=(uintptr_t)model[n+2],.control=(uintptr_t)&controls[n],.pins=1};
            s->materials[n]=materials+n;
            f->streams[n+4].indices=64;
            f->draws[n].material=(struct jc_cloth_material){.material=(uintptr_t)subscribers[n]+0x88,.role=n+4,.participant=1,.peer=77};
            f->route.plan.ranges[1][n+4].index_begin=24;f->route.plan.ranges[1][n+4].indices=6;
            jc_command_ptr(queue+0x80+n*16,materials[n].model);jc_command_ptr(queue+0x88+n*16,materials[n].control);
        }
        jc_command_ptr(context+0x18,(uintptr_t)queue);
        if (failure==1) model[0][13]=model[0][12];
        if (failure==2) geometry[0][0]=2;
        if (failure==3) item[1][4]=item[0][4];
        if (failure==4) materials[0].pins=0;
        if (failure==5) f->draws[0].material.peer=0;
        if (failure==6) batches[0]=3;
        if (failure==7) collection[0][1]=0;
        if (failure==8) members[0][2]=0;
        if (failure==9) for (unsigned n=0;n<4;++n) {
            batches[n]=0;collection[n][0]=0x80000001u;collection[n][1]=members[n][0];
        }
        unsigned detail=999;registration_commits=0;
        int code=jc_cloth_render_register(s,0,(uintptr_t)registration_commit-0x274d70,(uintptr_t)barn,(uintptr_t)context,1,&detail);
        if (!failure || failure==9) {
            assert(!code && !detail && registration_commits==2 && s->automatic_pending[0]);
            assert(jc_cloth_render_owners[0]->count==4 && jc_cloth_u32(queue+0x10080)==2);
            for (unsigned n=0;n<4;++n) assert(jc_cloth_render_owners[0]->draws[n].shader==binding[n][failure==9 ? 0 : 2]);
            free(jc_cloth_render_owners[0]);jc_cloth_render_owners[0]=NULL;
        } else {
            unsigned expected[]={0,0x142,0x142,0x156,0x200,0x208,0x142,0x142,0x142};
            assert(code==1 && detail==expected[failure] && !registration_commits && !s->automatic_pending[0] && !jc_cloth_render_owners[0]);
        }
        for (unsigned owner=0;owner<2;++owner) for (unsigned role=4;role<6;++role) {
            assert(jc_cloth_render_counts[owner][role][0]==before_counts[owner][role][0]+(!failure || failure==9));
            for (unsigned stage=1;stage<4;++stage) assert(jc_cloth_render_counts[owner][role][stage]==before_counts[owner][role][stage]);
        }
        free(s);free(barn);free(queue);
    }
    puts("Render registration: stock plus private models, empty-stock fallback, queue publication and precise pre-publication refusals passed; material commit simulated.");
}
#endif
static void runtime_cases(ID3D11DeviceContext *context) {
    struct jc_cloth_live *s=calloc(1,sizeof(*s));assert(s);
    struct jc_live_retirement retirement={0};s->thread=GetCurrentThreadId();s->read=runtime_read;s->retirement=&retirement;
    unsigned detail=0;
    assert(jc_cloth_render_register(s,0,0,0,0,0,&detail) && detail==1);
    unsigned char sync=0,executor[0x30]={0},commands[2][128]={{0}},renderer[0xa0]={0};
    jc_command_ptr(executor+0x20,1);
    struct jc_cloth_render_owner *o=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*o));assert(o);
    o->backend=s;o->count=2;o->image=(uintptr_t)&sync-0x3c03cd0;o->executor_mode=-1;
    s->frames.frames[0].phase=JC_FRAME_READY;s->automatic_pending[0]=1;s->commands[0].owner=o;
    jc_cloth_render_owners[0]=o;
    if (jc_cloth_render_tls==TLS_OUT_OF_INDEXES) jc_cloth_render_tls=TlsAlloc();
    unsigned char passes[24]={0},table[0x2c0]={0},state[0x38]={0},vertices[0x48]={0},bindings[24]={0},batch[32]={0};
    uint32_t one=1,stride=60,semantic=12;uint16_t offset=0;
    memcpy(vertices,&stride,4);jc_command_ptr(bindings,(uintptr_t)vertices);jc_command_ptr(bindings+8,(uintptr_t)&semantic);
    jc_command_ptr(state+0x18,(uintptr_t)bindings);memcpy(passes+8,&one,4);jc_command_ptr(passes+0x10,(uintptr_t)table);
    memcpy(table+0x20,&one,4);jc_command_ptr(table+0x28,(uintptr_t)&offset);
    struct jc_cloth_command_source source={0x1111,(uintptr_t)passes,0,(uintptr_t)batch,(uintptr_t)state,0,0,0};
    s->frames.frames[0].streams[4]=(struct jc_cloth_stream){.vertex=66,.index=88,.vertex_bytes=4096,.index_bytes=4096,.vertices=32,.indices=64};
    for(unsigned i=0;i<2;++i) {
        struct jc_cloth_render_draw *d=o->draws+i;d->item=10+i;d->bindings=20+i;d->shader=source.material;d->role=4;d->count=9;
        struct jc_cloth_render_scope scope,nested;
        jc_command_ptr(renderer+0x68,(uintptr_t)commands[i]);jc_command_ptr(renderer+0x60,(uintptr_t)commands[i]+128);
        assert(jc_cloth_render_dispatch_begin((uintptr_t)renderer,d->item,d->bindings,&scope)==1);
        assert(!jc_cloth_render_dispatch_begin((uintptr_t)renderer,99,99,&nested));
        const struct jc_cloth_command_source *redirected=NULL;
        assert(!jc_cloth_render_build_before((uintptr_t)renderer,&source,&redirected));
        jc_cloth_render_dispatch_end(&nested,0);
        assert(jc_cloth_render_build_before((uintptr_t)renderer,&source,&redirected)==1);
        assert(redirected->state!=(uintptr_t)state && redirected->storage==source.storage);
        memcpy(commands[i],&one,4);jc_command_ptr(commands[i]+0x20,source.material);jc_command_ptr(commands[i]+0x28,(uintptr_t)table);
        jc_command_ptr(commands[i]+0x58,(uintptr_t)d->view.binding.bindings);jc_command_ptr(commands[i]+0x68,(uintptr_t)&d->view.binding.index);
        memcpy(commands[i]+0x70,&d->count,4);jc_command_ptr(renderer+0x68,(uintptr_t)commands[i]+128);jc_command_ptr(renderer+0x98,(uintptr_t)commands[i]+128);
        jc_cloth_render_build_after(0);jc_cloth_render_dispatch_end(&scope,0);
        assert(d->built && !o->building && !s->failed && !TlsGetValue(jc_cloth_render_tls));
    }
    int handled=0;
    assert(!jc_cloth_render_base_range(1,2));
    /* Later CPU task hands off first; neither order grants premature retirement. */
    for(unsigned i=2;i--;) {
        uint32_t code=i ? jc_cloth_render_handoff((uintptr_t)executor,123,456,(uintptr_t)commands[i],(uintptr_t)commands[i]+128,runtime_submit,&handled) :
            jc_cloth_render_base_handoff((uintptr_t)executor,(uintptr_t)commands[i],(uintptr_t)commands[i]+128,456,runtime_base_submit,&handled);
        assert(!code && handled);
        assert(o->ticket_count==2-i);
    }
    uint64_t ticket=o->tickets[0];assert(o->submitted && runtime_submits==2 && !s->automatic_pending[0]);
    assert(!jc_cloth_render_retired(s,ticket,&handled) && handled);
    unsigned mask=jc_live_retirement_enter(&retirement,(uintptr_t)executor,(uintptr_t)commands[0],(uintptr_t)commands[0]+128);
    assert(mask);jc_live_retirement_leave(&retirement,mask,0,context);query_complete=1;query_result=0;jc_live_retirement_poll(&retirement);
    assert(!jc_cloth_render_retired(s,ticket,&handled) && handled && jc_cloth_render_owners[0]);
    mask=jc_live_retirement_enter(&retirement,(uintptr_t)executor,(uintptr_t)commands[1],(uintptr_t)commands[1]+128);
    assert(mask);jc_live_retirement_leave(&retirement,mask,0,context);jc_live_retirement_poll(&retirement);
    assert(jc_cloth_render_retired(s,ticket,&handled)==1 && handled && !jc_cloth_render_owners[0]);
    assert(!jc_cloth_render_discard(s->commands));
    for (unsigned failure=0;failure<4;++failure) {
        o=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*o));assert(o);o->backend=s;o->count=1;o->image=(uintptr_t)&sync-0x3c03cd0;o->executor_mode=-1;
        o->draws[0].built=1;o->draws[0].begin=(uintptr_t)commands[0];o->draws[0].end=(uintptr_t)commands[0]+128;
        memcpy(o->draws[0].snapshot,commands[0],128);jc_cloth_render_owners[0]=o;s->commands[0].owner=o;s->failed=0;
        s->frames.frames[0].phase=JC_FRAME_READY;
        if (failure==0) {
            commands[0][4]^=1;
            assert(jc_cloth_render_base_handoff((uintptr_t)executor,(uintptr_t)commands[0],(uintptr_t)commands[0]+128,456,runtime_base_submit,&handled)==5 && handled);
            commands[0][4]^=1;
        } else if (failure==1) {
            o->executor=(uintptr_t)executor+8;
            assert(jc_cloth_render_handoff((uintptr_t)executor,123,456,(uintptr_t)commands[0],(uintptr_t)commands[0]+128,runtime_submit,&handled)==5 && handled);
        } else if (failure==2) {
            assert(jc_cloth_render_base_range((uintptr_t)commands[0]+1,(uintptr_t)commands[0]+127));
        } else {
            assert(jc_cloth_render_discard(s->commands));
        }
        assert(s->failed && o->failed && s->commands[0].owner==o && s->frames.frames[0].phase==JC_FRAME_UNKNOWN);
        assert(!o->ticket_count && runtime_submits==2);jc_cloth_render_owners[0]=NULL;HeapFree(GetProcessHeap(),0,o);memset(s->commands,0,sizeof(s->commands));
    }
    s->failed=0;o=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*o));assert(o);
    o->backend=s;o->count=2;o->native_frame=7;o->draws[0].item=10;o->draws[0].bindings=20;
    s->frames.frames[0].phase=JC_FRAME_READY;s->commands[0].owner=o;s->automatic_pending[0]=1;
    s->frames.backend.release=runtime_release;s->frames.frames[0].refs[0]=(struct jc_cloth_frame_ref){66,JC_FRAME_VERTEX};
    s->frames.frames[0].ref_count=1;jc_cloth_render_owners[0]=o;
    struct jc_cloth_render_scope empty_scope;
    assert(jc_cloth_render_dispatch_begin((uintptr_t)renderer,10,20,&empty_scope)==1);
    jc_cloth_render_dispatch_end(&empty_scope,0);
    assert(!s->failed && o->draws[0].visited && !o->draws[0].built);
    assert(jc_cloth_render_frame_pending(s,7) && !jc_cloth_render_frame_pending(s,6));
    assert(!jc_cloth_render_frame_complete(s,6) && !runtime_releases);
    assert(jc_cloth_render_frame_complete(s,7)==1 && runtime_releases==1 && !jc_cloth_render_owners[0] &&
        !s->automatic_pending[0] && s->frames.frames[0].phase==JC_FRAME_FREE);
    free(s);
    puts("Automatic native render publication: shared-shader item scopes, nested dispatch isolation, private stream redirection, mixed/reversed native handoffs, all-batch EVENT retirement, changed command/executor and premature-discard rejection, native culling and exact-frame closure passed");
}
struct actor_frame_fixture {
    uintptr_t image,render,executor;
    unsigned char synchronous;
    unsigned flushed,reserved,handed,fail_reserve;
    struct jc_actor_frame *frame;
};
static struct actor_frame_fixture afx;
static int actor_frame_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx;
    if(address==afx.image+0x3c942d0 && bytes==8) { uintptr_t global=afx.render-0xd0;memcpy(out,&global,8);return 1; }
    if(address==afx.image+0x3c03cd0 && bytes==1) { memcpy(out,&afx.synchronous,1);return 1; }
    return runtime_read(NULL,address,out,bytes);
}
static uintptr_t JC_COMMAND_ABI actor_frame_executor(void) { return afx.executor; }
static uint32_t JC_COMMAND_ABI actor_frame_flush(uintptr_t renderer) {
    assert(renderer==afx.render && !afx.reserved);++afx.flushed;
    memset((void *)(renderer+0x90),0,16);return 0;
}
static uint32_t JC_COMMAND_ABI actor_frame_reserve(uintptr_t executor,uintptr_t *reservation) {
    assert(executor==afx.executor && afx.flushed && !afx.handed && afx.frame->ticket);
    ++afx.reserved;if(afx.fail_reserve)return 5;*reservation=0x1234;return 0;
}
static uint32_t JC_COMMAND_ABI actor_frame_handoff(uintptr_t executor,uintptr_t reservation,uintptr_t fence,uintptr_t begin,uintptr_t end) {
    assert(executor==afx.executor && afx.reserved==1 && *(uintptr_t *)reservation==0x1234 && !fence);
    assert(begin==(uintptr_t)afx.frame->commands && end-begin==128 && jc_cloth_u32((void *)begin)==0x2c);
    uintptr_t callback;memcpy(&callback,(void *)(begin+8),8);
    ((void (JC_COMMAND_ABI *)(const uintptr_t *))callback)((void *)(begin+16));
    ++afx.handed;return 0;
}
static void actor_frame_cases(ID3D11DeviceContext *context) {
    unsigned char game[0xa0]={0},scene[0x68]={0},renderer[0x200]={0},vtable[0x18]={0},executor[0x30]={0};
    unsigned char *body=calloc(1,0x10088),*glyph=calloc(1,0x10088);assert(body && glyph);
    uintptr_t image=0x140000000ull;
    jc_command_ptr(game+0x10,(uintptr_t)renderer);jc_command_ptr(game+0x98,(uintptr_t)scene);
    jc_command_ptr(scene+0x18,(uintptr_t)body);jc_command_ptr(scene+0x60,(uintptr_t)glyph);
    jc_command_ptr(renderer,(uintptr_t)vtable);jc_command_ptr(vtable+0x10,image+0x352540);jc_command_ptr(executor+0x20,1);
    struct jc_actor_frame_ops ops={actor_frame_executor,actor_frame_reserve,actor_frame_flush,actor_frame_handoff};
    for (unsigned failure=0;failure<4;++failure) {
        struct jc_actor_frame frame={0};afx=(struct actor_frame_fixture){image,(uintptr_t)renderer,(uintptr_t)executor,0,0,0,0,0,&frame};
        jc_command_ptr(renderer+0x90,0x1000);jc_command_ptr(renderer+0x98,0x1080);
        if(failure==1) jc_command_ptr(glyph+0x10080,1);
        if(failure==2) jc_command_ptr(vtable+0x10,image+0x352de0);
        if(failure==3) afx.fail_reserve=1;
        int result=jc_actor_frame_after_ops(&frame,actor_frame_read,NULL,image,(uintptr_t)game,1,&ops);
        if(failure) {
            assert(result<0 && frame.failed && !afx.handed);
            if(failure<3) assert(!afx.flushed && !afx.reserved && !frame.ticket);
            else assert(afx.flushed && afx.reserved && frame.ticket && jc_actor_frame_poll(&frame)<0);
            jc_command_ptr(glyph+0x10080,0);jc_command_ptr(vtable+0x10,image+0x352540);
            continue;
        }
        assert(!result && afx.flushed==1 && afx.reserved==1 && afx.handed==1 && !jc_actor_frame_poll(&frame));
        unsigned mask=jc_live_retirement_enter(&frame.retirement,(uintptr_t)executor,(uintptr_t)frame.commands,(uintptr_t)frame.commands+128);
        query_result=1;query_complete=0;assert(mask);jc_live_retirement_leave(&frame.retirement,mask,0,context);
        jc_live_retirement_poll(&frame.retirement);assert(!jc_actor_frame_poll(&frame));
        query_result=0;query_complete=1;jc_live_retirement_poll(&frame.retirement);assert(jc_actor_frame_poll(&frame)==1 && !frame.ticket);
        assert(jc_actor_frame_after_ops(&frame,actor_frame_read,NULL,image,(uintptr_t)game,1,&ops)<0);
    }
    free(body);free(glyph);
    puts("Actor whole-frame marker: body/glyph consumption and main-renderer identity guards, flush-before-reserve, arm-before-handoff, GPU-delayed retirement and failed-reservation retention passed");
}
int main(void) {
    static ID3D11QueryVtbl qv={.Release=query_release}; query.lpVtbl=&qv;
    static ID3D11DeviceVtbl dv={.Release=device_release,.CreateQuery=create_query,.CreateBuffer=create_buffer}; device.lpVtbl=&dv;
    static ID3D11BufferVtbl bv={.Release=buffer_release}; buffers[0].lpVtbl=&bv; buffers[1].lpVtbl=&bv;
    static ID3D11DeviceContextVtbl cv={.GetType=get_type,.GetDevice=get_device,.AddRef=add_context,
        .Release=release_context,.End=end_query,.GetData=get_data,.Map=map_buffer,.Unmap=unmap_buffer,
        .Flush=flush_context};
    ID3D11DeviceContext context={&cv}; struct jc_live_retirement r={0};
    uint64_t t=jc_live_retirement_arm(&r,1,0x1000,0x1100); assert(t);
    assert(jc_live_retirement_result(&r,t,0)==0 && !ends);
    unsigned mask=jc_live_retirement_enter(&r,1,0x1000,0x1100); assert(mask==1);
    assert(!jc_live_retirement_cancel(&r,t));
    query_result=1; query_complete=1;
    jc_live_retirement_leave(&r,mask,0,&context);
    assert(ends==1 && polls==1 && context_refs==1 && query_refs==1 && !device_refs);
    assert(jc_live_retirement_result(&r,t,0)==0);
    assert(jc_live_retirement_pending_executor(&r)==1);
    jc_live_retirement_progress(&r); jc_live_retirement_progress(&r);
    assert(flushes==1 && jc_live_retirement_result(&r,t,0)==0);
    query_result=0; query_complete=0; jc_live_retirement_poll(&r);
    assert(jc_live_retirement_result(&r,t,0)==0);
    query_complete=1; jc_live_retirement_poll(&r);
    assert(jc_live_retirement_result(&r,t,0)==1 && !context_refs && !query_refs);
    assert(!jc_live_retirement_pending_executor(&r));
    assert(jc_live_retirement_result(&r,t,1)==1 && jc_live_retirement_result(&r,t,0)==-1);
    t=jc_live_retirement_arm(&r,1,0x1000,0x1100); assert(t && jc_live_retirement_cancel(&r,t));
    actor_frame_cases(&context);
    runtime_cases(&context);
#ifndef _WIN32
    registration_cases();
#endif
    integration(&context);
    asynchronous=1; integration(&context);
    for (unsigned failure=0;failure<4;++failure) {
        memset(&r,0,sizeof(r)); deferred=failure==0;create_failure=failure==1;
        query_result=failure==3 ? -1 : 1;
        t=jc_live_retirement_arm(&r,1,0x1000,0x1100);
        mask=jc_live_retirement_enter(&r,1,0x1000,0x1100);
        jc_live_retirement_leave(&r,mask,failure==2 ? 1 : 0,&context);
        assert(jc_live_retirement_result(&r,t,1)==-1);
        if (failure<3) assert(!query_refs && !context_refs && !device_refs);
        else assert(query_refs==1 && context_refs==1); /* error retains uncertain ownership */
    }
    puts("Production D3D backend: private buffer creation/mapping, exclusive leases, worker-gated unmap, publication armed before synchronous handoff, GPU-gated chapter cleanup, End/GetData ordering and errors passed with intercepted COM; no GPU runtime or material construction");
    return 0;
}
