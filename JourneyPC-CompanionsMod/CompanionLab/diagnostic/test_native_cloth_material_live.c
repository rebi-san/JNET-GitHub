#ifdef _WIN32
#include <windows.h>
#endif
#include "native_cloth_material_live.h"
#include <assert.h>
#include <stdio.h>
#include <float.h>
static unsigned char geometry[32],a[32],b[32],atlas_entry[64];
static unsigned char atlas_payload[0x100],atlas_texture[0x120],atlas_gpu[0x98];
static uint32_t atlas_refs;
static int read_bytes(void *ctx,uint64_t address,void *out,size_t n) {
    (void)ctx;
    const unsigned char *blocks[]={geometry,a,b,atlas_entry,atlas_payload,atlas_texture,atlas_gpu,(void *)&atlas_refs};
    const size_t sizes[]={sizeof(geometry),sizeof(a),sizeof(b),sizeof(atlas_entry),sizeof(atlas_payload),
        sizeof(atlas_texture),sizeof(atlas_gpu),sizeof(atlas_refs)};
    for (unsigned i=0;i<sizeof(blocks)/sizeof(*blocks);++i) if (address>=(uintptr_t)blocks[i] &&
        address-(uintptr_t)blocks[i]<=sizes[i] && n<=sizes[i]-(address-(uintptr_t)blocks[i])) {
        memcpy(out,(void *)(uintptr_t)address,n); return 1;
    }
    return 0;
}
static void put(unsigned char *p,uint64_t v) { memcpy(p,&v,8); }
static void put32(unsigned char *p,uint32_t v) { memcpy(p,&v,4); }
struct graphics_fixture { uint32_t owner,depth,thread;uint64_t device;unsigned enters,leaves,fail; };
static int graphics_read(void *ctx,uint64_t address,void *out,size_t n) {
    struct graphics_fixture *s=ctx;
    if (address==UINT64_C(0x144040800) && n==8) {
        uint32_t state[2]={s->owner,s->depth};memcpy(out,state,8);return 1;
    }
    if (address==UINT64_C(0x1440408b8) && n==8) { memcpy(out,&s->device,8);return 1; }
    return 0;
}
static uint64_t graphics_mutex(void *ctx) { (void)ctx;return UINT64_C(0x1440407e0); }
static uint32_t graphics_thread(void *ctx) { return ((struct graphics_fixture *)ctx)->thread; }
static void graphics_enter(void *ctx,uint64_t mutex) {
    struct graphics_fixture *s=ctx;assert(mutex==graphics_mutex(ctx));++s->enters;
    if (!s->fail) { assert(!s->owner || s->owner==s->thread);s->owner=s->thread;++s->depth; }
}
static void graphics_leave(void *ctx,uint64_t mutex) {
    struct graphics_fixture *s=ctx;assert(mutex==graphics_mutex(ctx));++s->leaves;
    if (s->owner==s->thread) { assert(s->depth);if (!--s->depth) s->owner=0; }
}
static void graphics_locks(void) {
    struct graphics_fixture s={.thread=17,.device=1234};
    struct jc_cloth_graphics_ops ops={&s,graphics_mutex,graphics_thread,graphics_enter,graphics_leave};
    struct jc_cloth_graphics_token a={0},b={0};uint64_t image=UINT64_C(0x140000000);
    assert(!jc_cloth_graphics_lock(graphics_read,&s,image,&ops,0,&a) && a.device_context==1234);
    assert(jc_cloth_graphics_lock(graphics_read,&s,image,&ops,0,&a));
    assert(!jc_cloth_graphics_lock(graphics_read,&s,image,&ops,1234,&b) && b.prior_depth==1);
    assert(!jc_cloth_graphics_unlock(graphics_read,&s,image,&ops,&b) && s.depth==1);
    s.thread=99;assert(jc_cloth_graphics_unlock(graphics_read,&s,image,&ops,&a) && a.held);
    s.thread=17;s.device=4321;
    assert(jc_cloth_graphics_unlock(graphics_read,&s,image,&ops,&a) && !a.held && !s.depth);
    assert(jc_cloth_graphics_lock(graphics_read,&s,image,&ops,1234,&a) && !s.depth);
    s.fail=1;s.owner=99;s.depth=3;
    assert(jc_cloth_graphics_lock(graphics_read,&s,image,&ops,0,&a) && s.owner==99 && s.depth==3);
    s.fail=0;s.owner=0;s.depth=0;s.device=0;
    assert(jc_cloth_graphics_lock(graphics_read,&s,image,&ops,0,&a) && !s.depth);
    assert(s.enters==s.leaves);
}
struct atlas_fixture { unsigned holds,drops,clones,releases,fail_after,mutate,fail_hold,shared_sampler; };
static int atlas_hold(void *ctx,const uint64_t pair[2]) {
    struct atlas_fixture *s=ctx;assert(pair[0]==(uintptr_t)atlas_texture && pair[1]==(uintptr_t)&atlas_refs);
    if (s->fail_hold) return 0;
    ++s->holds;++atlas_refs;return 1;
}
static void atlas_drop(void *ctx,const uint64_t pair[2]) {
    struct atlas_fixture *s=ctx;assert(pair[0]==(uintptr_t)atlas_texture && atlas_refs==2);
    ++s->drops;--atlas_refs;
}
static int atlas_clone(void *ctx,const uint64_t source[4],uint64_t shell,uint64_t out[4]) {
    struct atlas_fixture *s=ctx;++s->clones;assert(shell!=(uintptr_t)atlas_texture && source[0]==1000 && source[1]==2000);
    for (unsigned i=0;i<4;++i) {
        if (s->fail_after==i) return 0;
        if (source[i] || i==3) out[i]=i==3 && s->shared_sampler ? source[3] : 10000+i;
    }
    if (s->mutate==1) atlas_gpu[0x94]^=2;
    if (s->mutate==2) atlas_texture[0xa]^=1;
    if (s->mutate==3) put(atlas_entry+0x38,0);
    return s->fail_after!=4;
}
static void atlas_release(void *ctx,uint64_t object) {
    struct atlas_fixture *s=ctx;assert((object>=10000 && object<10004) || (object==4000 && s->shared_sampler));++s->releases;
}
static void atlas_reset(void) {
    memset(atlas_entry,0,sizeof(atlas_entry));memset(atlas_payload,0,sizeof(atlas_payload));
    memset(atlas_texture,0,sizeof(atlas_texture));memset(atlas_gpu,0,sizeof(atlas_gpu));atlas_refs=1;
    put(atlas_entry+0x38,(uintptr_t)atlas_payload);put(atlas_payload+0xc8,(uintptr_t)atlas_texture);
    put(atlas_payload,UINT64_C(0x140680b18));
    put(atlas_payload+0xd0,(uintptr_t)&atlas_refs);put(atlas_texture,UINT64_C(0x1406894c0));
    put(atlas_texture+0x70,(uintptr_t)atlas_gpu);put32(atlas_texture+0x110,1);put32(atlas_texture+0x24,0x80);
    put(atlas_texture+0x28,4000);put32(atlas_gpu+0x30,1);put32(atlas_gpu+0x94,1);
    put(atlas_gpu+0x38,1000);put(atlas_gpu+0x40,2000);put(atlas_gpu+0x48,3000);
}
static void atlas_snapshots(void) {
    for(unsigned role=4;role<=5;++role)for(unsigned index=0;index<6;++index) {
        unsigned char source[0x120],copy[0x120];memset(source,0x01,sizeof(source));
        memcpy(copy,source,sizeof(copy));
        assert(jc_cloth_atlas_sampler_prepare(copy,role,index)==(index<3));
        if(index<3) {
            assert(copy[0xa]==2 && copy[0xb]==(role==5?2:1));
            source[0xa]=2;if(role==5)source[0xb]=2;
            source[0x24]&=~0x80u;
        }
        assert(!memcmp(source,copy,sizeof(copy)));
    }

    struct jc_cloth_atlas_snapshot snapshot={0};struct atlas_fixture s={.fail_after=5};
    struct jc_cloth_atlas_copy_ops ops={&s,atlas_hold,atlas_drop,atlas_clone,atlas_release};
    atlas_reset();
    assert(!jc_cloth_atlas_snapshot_create(read_bytes,NULL,UINT64_C(0x140000000),
        (uintptr_t)atlas_entry,(uintptr_t)atlas_texture,&ops,&snapshot));
    assert(s.holds==1 && s.drops==1 && atlas_refs==1 && s.clones==1 && !s.releases);
    assert(!jc_cloth_atlas_snapshot_check(&snapshot));uint64_t object=(uintptr_t)snapshot.texture;
    assert(jc_cloth_atlas_snapshot_pin(&snapshot,object) && jc_cloth_atlas_snapshot_pin(&snapshot,object));
    assert(!jc_cloth_atlas_snapshot_pin(&snapshot,(uintptr_t)atlas_texture));
    memset(atlas_texture,0,sizeof(atlas_texture));memset(atlas_gpu,0,sizeof(atlas_gpu));put(atlas_entry+0x38,0);
    assert(!jc_cloth_atlas_snapshot_check(&snapshot));
    assert(jc_cloth_atlas_snapshot_destroy(&ops,&snapshot) && !s.releases);
    assert(jc_cloth_atlas_snapshot_unpin(&snapshot,object) && jc_cloth_atlas_snapshot_unpin(&snapshot,object));
    assert(!jc_cloth_atlas_snapshot_unpin(&snapshot,object));
    snapshot.texture[1]^=1;assert(jc_cloth_atlas_snapshot_check(&snapshot));snapshot.texture[1]^=1;
    snapshot.handle[8]^=1;assert(jc_cloth_atlas_snapshot_check(&snapshot));snapshot.handle[8]^=1;
    assert(!jc_cloth_atlas_snapshot_destroy(&ops,&snapshot) && s.releases==4 && !snapshot.ready);
    for (unsigned failure=0;failure<5;++failure) {
        atlas_reset();s=(struct atlas_fixture){.fail_after=failure};
        assert(jc_cloth_atlas_snapshot_create(read_bytes,NULL,UINT64_C(0x140000000),
            (uintptr_t)atlas_entry,(uintptr_t)atlas_texture,&ops,&snapshot));
        assert(s.holds==1 && s.drops==1 && s.releases==failure && !snapshot.ready && atlas_refs==1);
    }
    for (unsigned mutation=1;mutation<=3;++mutation) {
        atlas_reset();s=(struct atlas_fixture){.fail_after=5,.mutate=mutation};
        assert(jc_cloth_atlas_snapshot_create(read_bytes,NULL,UINT64_C(0x140000000),
            (uintptr_t)atlas_entry,(uintptr_t)atlas_texture,&ops,&snapshot));
        assert(s.holds==1 && s.drops==1 && s.releases==4 && !snapshot.ready);
    }
    for (unsigned invalid=0;invalid<6;++invalid) {
        atlas_reset();s=(struct atlas_fixture){.fail_after=5};
        if (invalid==0) put32(atlas_texture+0x110,3);
        if (invalid==1) put32(atlas_gpu+0x30,2);
        if (invalid==2) put32(atlas_gpu+0xc,1);
        if (invalid==3) { put32(atlas_gpu+0x94,0x80000001);put(atlas_gpu+0x48,0); }
        if (invalid==4) put(atlas_texture+0x78,1);
        if (invalid==5) s.fail_hold=1;
        assert(jc_cloth_atlas_snapshot_create(read_bytes,NULL,UINT64_C(0x140000000),
            (uintptr_t)atlas_entry,(uintptr_t)atlas_texture,&ops,&snapshot));
        assert(!s.clones && s.holds==s.drops && !s.releases);
    }
    atlas_reset();s=(struct atlas_fixture){.fail_after=5,.shared_sampler=1};
    assert(!jc_cloth_atlas_snapshot_create(read_bytes,NULL,UINT64_C(0x140000000),
        (uintptr_t)atlas_entry,(uintptr_t)atlas_texture,&ops,&snapshot));
    assert(!jc_cloth_atlas_snapshot_destroy(&ops,&snapshot) && s.releases==4);
    atlas_reset();s=(struct atlas_fixture){.fail_after=5};put(atlas_texture+0x28,0);put(atlas_gpu+0x48,0);
    assert(!jc_cloth_atlas_snapshot_create(read_bytes,NULL,UINT64_C(0x140000000),
        (uintptr_t)atlas_entry,(uintptr_t)atlas_texture,&ops,&snapshot));
    assert(!jc_cloth_atlas_snapshot_destroy(&ops,&snapshot) && s.releases==3);
    memset(atlas_entry,0,sizeof(atlas_entry));
}
static void atlas_lists(void) {
    struct jc_cloth_atlas_list empty,one,two,bad;
    assert(!jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&empty));
    put(atlas_entry+0x28,(uintptr_t)a);put(atlas_entry+0x30,(uintptr_t)a);
    put(a,(uintptr_t)atlas_entry+0x28);
    assert(!jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&one));
    assert(!jc_cloth_atlas_list_changed(&empty,&one,(uintptr_t)a,1));
    put(atlas_entry+0x30,(uintptr_t)b);put(a+8,(uintptr_t)b);
    put(b,(uintptr_t)atlas_entry+0x28);put(b+16,(uintptr_t)a);
    assert(!jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&two));
    assert(!jc_cloth_atlas_list_changed(&one,&two,(uintptr_t)b,1));
    assert(!jc_cloth_atlas_list_changed(&two,&one,(uintptr_t)b,0));
    assert(jc_cloth_atlas_list_changed(&two,&one,(uintptr_t)a,0));
    bad=two;bad.nodes[0]=(uintptr_t)b;
    assert(jc_cloth_atlas_list_changed(&one,&bad,(uintptr_t)b,1));
    put(b+8,(uintptr_t)a);assert(jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&bad));
    put(b+8,0);put(b+16,0);assert(jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&bad));
    put(b+16,(uintptr_t)a);put(b,0);assert(jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&bad));
    put(b,(uintptr_t)atlas_entry+0x28);put(b+8,0x10000);
    assert(jc_cloth_atlas_list_read(read_bytes,NULL,(uintptr_t)atlas_entry,&bad));
    memset(a,0,sizeof(a));memset(b,0,sizeof(b));
}
static void appearance_routes(void) {
    uint64_t source[2]={100,200},target[2]={300,400};
    struct jc_cloth_storage stock[2]={
        {.payload=1000,.count=1,.spans={{10000,32}}},
        {.payload=2000,.count=1,.spans={{20000,32}}}};
    struct jc_cloth_storage owned[2]={
        {.payload=3000,.count=1,.spans={{30000,32}}},
        {.payload=4000,.count=1,.spans={{40000,32}}}};
    struct jc_cloth_appearance_route route;
    assert(!jc_cloth_appearance_route_make(source,target,stock,owned,&route));
    for (unsigned i=0;i<2;++i) {
        assert(jc_cloth_appearance_target(&route,source[i],0)==target[i]);
        assert(jc_cloth_appearance_target(&route,stock[i].payload,1)==owned[i].payload);
        assert(jc_cloth_appearance_target(&route,target[i],0)==target[i]);
    }
    assert(jc_cloth_appearance_target(&route,999,0)==999);
    owned[1].spans[0].begin=10000;
    assert(jc_cloth_appearance_route_make(source,target,stock,owned,&route));
    assert(jc_cloth_appearance_target(&route,100,0)==100);
    owned[1].spans[0].begin=40000;owned[1].payload=1000;
    assert(jc_cloth_appearance_route_make(source,target,stock,owned,&route));
    owned[1].payload=4000;target[1]=target[0];
    assert(jc_cloth_appearance_route_make(source,target,stock,owned,&route));
}
#ifdef _WIN32
static int gpu_hold(void *ctx,const uint64_t pair[2]) {
    (void)ctx;assert(pair[1]==(uintptr_t)&atlas_refs && atlas_refs==1);++atlas_refs;return 1;
}
static void gpu_drop(void *ctx,const uint64_t pair[2]) {
    (void)ctx;assert(pair[1]==(uintptr_t)&atlas_refs && atlas_refs==2);--atlas_refs;
}
static int gpu_wait(ID3D11DeviceContext *context,ID3D11Query *query) {
    context->lpVtbl->End(context,(ID3D11Asynchronous *)query);context->lpVtbl->Flush(context);
    DWORD start=GetTickCount();
    while (GetTickCount()-start<5000) {
        BOOL done=FALSE;HRESULT result=context->lpVtbl->GetData(context,(ID3D11Asynchronous *)query,&done,sizeof(done),0);
        if (result==S_OK && done) return 1;
        if (FAILED(result)) return 0;
        Sleep(1);
    }
    return 0;
}
static int gpu_fixture(void) {
    typedef HRESULT (WINAPI *create_device_fn)(IDXGIAdapter *,D3D_DRIVER_TYPE,HMODULE,UINT,
        const D3D_FEATURE_LEVEL *,UINT,UINT,ID3D11Device **,D3D_FEATURE_LEVEL *,ID3D11DeviceContext **);
    HMODULE dll=LoadLibraryW(L"d3d11.dll");
    create_device_fn create=dll ? (create_device_fn)(uintptr_t)GetProcAddress(dll,"D3D11CreateDevice") : NULL;
    ID3D11Device *device=NULL;ID3D11DeviceContext *context=NULL;D3D_FEATURE_LEVEL level=0;
    ID3D11Texture2D *source=NULL,*staging=NULL;ID3D11ShaderResourceView *view=NULL;
    ID3D11SamplerState *sampler=NULL;ID3D11Query *query=NULL;ID3D11Buffer *buffer=NULL,*buffer_read=NULL;
    struct jc_cloth_atlas_snapshot snapshot={0};int success=0;unsigned stage=1,driver=1;
    if (!create) goto done;
    HRESULT result=create(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,0,NULL,0,D3D11_SDK_VERSION,&device,&level,&context);
    if (FAILED(result)) {
        driver=2;result=create(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&device,&level,&context);
    }
    if (FAILED(result) || !device || !context) goto done;
    uint32_t pixels[4]={0xff0000ffu,0xff00ff00u,0xffff0000u,0xffabcdefu},changed[4]={0,0,0,0};
    D3D11_TEXTURE2D_DESC desc={.Width=2,.Height=2,.MipLevels=1,.ArraySize=1,.Format=DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc={1,0},.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_SHADER_RESOURCE};
    D3D11_SUBRESOURCE_DATA data={.pSysMem=pixels,.SysMemPitch=8};
    stage=2;
    if (FAILED(device->lpVtbl->CreateTexture2D(device,&desc,&data,&source)) || !source ||
        FAILED(device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)source,NULL,&view)) || !view) goto done;
    D3D11_SAMPLER_DESC sd={.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR,.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV=D3D11_TEXTURE_ADDRESS_CLAMP,.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP,.MaxAnisotropy=1,
        .ComparisonFunc=D3D11_COMPARISON_ALWAYS,.MaxLOD=FLT_MAX};
    if (FAILED(device->lpVtbl->CreateSamplerState(device,&sd,&sampler)) || !sampler) goto done;
    atlas_reset();put(atlas_gpu+0x38,(uintptr_t)source);put(atlas_gpu+0x40,(uintptr_t)view);
    put(atlas_gpu+0x48,0);put(atlas_texture+0x28,(uintptr_t)sampler);
    struct jc_cloth_atlas_native native={UINT64_C(0x140000000),read_bytes,NULL,context};
    struct jc_cloth_atlas_copy_ops ops=jc_cloth_atlas_native_ops(&native);ops.hold=gpu_hold;ops.drop=gpu_drop;
    stage=3;
    if (jc_cloth_atlas_snapshot_create(read_bytes,NULL,native.image,(uintptr_t)atlas_entry,
        (uintptr_t)atlas_texture,&ops,&snapshot)) goto done;
    context->lpVtbl->UpdateSubresource(context,(ID3D11Resource *)source,0,NULL,changed,8,0);
    view->lpVtbl->Release(view);view=NULL;sampler->lpVtbl->Release(sampler);sampler=NULL;
    source->lpVtbl->Release(source);source=NULL;
    memset(atlas_texture,0,sizeof(atlas_texture));memset(atlas_gpu,0,sizeof(atlas_gpu));put(atlas_entry+0x38,0);
    uint64_t texture=(uintptr_t)snapshot.texture;
    if (!jc_cloth_atlas_snapshot_pin(&snapshot,texture) || !jc_cloth_atlas_snapshot_pin(&snapshot,texture) ||
        jc_cloth_atlas_snapshot_check(&snapshot) || !jc_cloth_atlas_snapshot_destroy(&ops,&snapshot)) goto done;
    stage=4;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    if (FAILED(device->lpVtbl->CreateTexture2D(device,&desc,NULL,&staging)) || !staging) goto done;
    context->lpVtbl->CopyResource(context,(ID3D11Resource *)staging,(ID3D11Resource *)(uintptr_t)snapshot.objects[0]);
    D3D11_QUERY_DESC qd={D3D11_QUERY_EVENT,0};
    if (FAILED(device->lpVtbl->CreateQuery(device,&qd,&query)) || !query || !gpu_wait(context,query)) goto done;
    D3D11_MAPPED_SUBRESOURCE mapped={0};
    if (FAILED(context->lpVtbl->Map(context,(ID3D11Resource *)staging,0,D3D11_MAP_READ,0,&mapped))) goto done;
    int identical=mapped.pData && mapped.RowPitch>=8 && !memcmp(mapped.pData,pixels,8) &&
        !memcmp((unsigned char *)mapped.pData+mapped.RowPitch,pixels+2,8);
    context->lpVtbl->Unmap(context,(ID3D11Resource *)staging,0);if (!identical) goto done;
    stage=5;
    for (unsigned kind=0;kind<2;++kind) {
        D3D11_BUFFER_DESC bd={.ByteWidth=64,.Usage=D3D11_USAGE_DYNAMIC,
            .BindFlags=kind ? D3D11_BIND_INDEX_BUFFER : D3D11_BIND_VERTEX_BUFFER,.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE};
        if (FAILED(device->lpVtbl->CreateBuffer(device,&bd,NULL,&buffer)) || !buffer ||
            FAILED(context->lpVtbl->Map(context,(ID3D11Resource *)buffer,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) goto done;
        for (unsigned i=0;i<64;++i) ((unsigned char *)mapped.pData)[i]=(unsigned char)(i+kind*64);
        context->lpVtbl->Unmap(context,(ID3D11Resource *)buffer,0);
        bd.Usage=D3D11_USAGE_STAGING;bd.BindFlags=0;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        if (FAILED(device->lpVtbl->CreateBuffer(device,&bd,NULL,&buffer_read)) || !buffer_read) goto done;
        context->lpVtbl->CopyResource(context,(ID3D11Resource *)buffer_read,(ID3D11Resource *)buffer);
        if (!gpu_wait(context,query) ||
            FAILED(context->lpVtbl->Map(context,(ID3D11Resource *)buffer_read,0,D3D11_MAP_READ,0,&mapped))) goto done;
        identical=1;for (unsigned i=0;i<64;++i)
            if (((unsigned char *)mapped.pData)[i]!=(unsigned char)(i+kind*64)) identical=0;
        context->lpVtbl->Unmap(context,(ID3D11Resource *)buffer_read,0);if (!identical) goto done;
        buffer_read->lpVtbl->Release(buffer_read);buffer_read=NULL;buffer->lpVtbl->Release(buffer);buffer=NULL;
    }
    stage=6;
    if (!jc_cloth_atlas_snapshot_unpin(&snapshot,texture) || !jc_cloth_atlas_snapshot_unpin(&snapshot,texture) ||
        jc_cloth_atlas_snapshot_destroy(&ops,&snapshot) || atlas_refs!=1) goto done;
    success=1;
done:
    if (snapshot.ready) {
        struct jc_cloth_atlas_copy_ops cleanup_ops={.release=jc_cloth_atlas_native_release};snapshot.pins=0;
        jc_cloth_atlas_snapshot_destroy(&cleanup_ops,&snapshot);
    }
    if (buffer_read) buffer_read->lpVtbl->Release(buffer_read);
    if (buffer) buffer->lpVtbl->Release(buffer);
    if (query) query->lpVtbl->Release(query);
    if (staging) staging->lpVtbl->Release(staging);
    if (sampler) sampler->lpVtbl->Release(sampler);
    if (view) view->lpVtbl->Release(view);
    if (source) source->lpVtbl->Release(source);
    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);
    if (dll) FreeLibrary(dll);
    printf("Real D3D11 atlas fixture: %s; stage %u; driver %s; feature level 0x%x; private copy survives original update/release, VB/IB map readback and EVENT retirement%s\n",
        success ? "PASS" : "FAIL",stage,driver==1 ? "hardware" : "WARP",(unsigned)level,success ? " passed" : " incomplete");
    return success ? 0 : 1;
}
#endif
int main(void) {
#ifdef _WIN32
    if (wcsstr(GetCommandLineW(),L"--d3d")) return gpu_fixture();
#endif
    graphics_locks();
    atlas_snapshots();
    atlas_lists();
    appearance_routes();
    struct jc_cloth_subscribers list;
    put(geometry+24,(uintptr_t)geometry);
    assert(!jc_cloth_subscribers_read(read_bytes,NULL,(uintptr_t)geometry,&list) && !list.count);
    put(geometry+8,(uintptr_t)a);put(geometry+16,(uintptr_t)b);
    put(a,(uintptr_t)geometry+8);put(a+8,(uintptr_t)b);
    put(b,(uintptr_t)geometry+8);put(b+16,(uintptr_t)a);
    assert(!jc_cloth_subscribers_read(read_bytes,NULL,(uintptr_t)geometry,&list) && list.count==2);
    put(b+8,(uintptr_t)a);
    assert(jc_cloth_subscribers_read(read_bytes,NULL,(uintptr_t)geometry,&list));
    put(b+8,0);put(b+16,0);
    assert(jc_cloth_subscribers_read(read_bytes,NULL,(uintptr_t)geometry,&list));
    struct jc_cloth_storage x={.payload=1,.count=1,.spans={{1000,32}}};
    struct jc_cloth_storage y={.payload=2,.count=1,.spans={{1032,32}}};
    assert(jc_cloth_storage_disjoint(&x,&y));
    y.spans[0].begin=1031; assert(!jc_cloth_storage_disjoint(&x,&y));
    y.spans[0].begin=1032;y.payload=1;assert(!jc_cloth_storage_disjoint(&x,&y));
    puts("Cloth material guards: recursive graphics lock/failure balance; private atlas copy, reload isolation, frame pins, all partial clone rollback, changed source rejection, unsupported layout rejection and sampler cache alias; subscriber and shader guards passed. GPU calls mocked, native construction not executed");
    return 0;
}
