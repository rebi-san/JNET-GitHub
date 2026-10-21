#ifndef JNET_MENU_IMAGES_H
#define JNET_MENU_IMAGES_H
#include <d3d11.h>
#include <d3dcompiler.h>
#include <mmsystem.h>
#include "jnet_frame_pacing.h"
#include "../work/menu-images/jnet_menu_assets.h"

#include "jnet_chat_image.h"

struct jnet_image_item { float box[4]; unsigned asset; float fraction; };
struct jnet_image_frame { uintptr_t owner; ULONGLONG time; unsigned count; float opacity; struct jnet_image_item items[4]; };
static SRWLOCK jnet_image_lock=SRWLOCK_INIT;
static struct jnet_image_frame jnet_image_frame;
static unsigned char (*jnet_original_present)(uintptr_t);
static volatile LONG jnet_image_result, jnet_image_frames;
static struct {
    ID3D11Device *device;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Buffer *vertices;
    ID3D11InputLayout *layout;
    ID3D11Buffer *constants;
    ID3D11ShaderResourceView *atlas;
    ID3D11SamplerState *sampler;
    ID3D11BlendState *blend;
    ID3D11RasterizerState *raster;
    ID3D11DepthStencilState *depth;
    int failed;LONG failed_stage;
} jnet_image_gpu;
#define JNET_RELEASE(p) do { if(p) { (p)->lpVtbl->Release(p); (p)=NULL; } } while(0)
static void jnet_image_release(void) {
    jnet_chat_image_release();
    JNET_RELEASE(jnet_image_gpu.vs);JNET_RELEASE(jnet_image_gpu.ps);
    JNET_RELEASE(jnet_image_gpu.vertices);JNET_RELEASE(jnet_image_gpu.layout);
    JNET_RELEASE(jnet_image_gpu.constants);JNET_RELEASE(jnet_image_gpu.atlas);
    JNET_RELEASE(jnet_image_gpu.sampler);JNET_RELEASE(jnet_image_gpu.blend);
    JNET_RELEASE(jnet_image_gpu.raster);JNET_RELEASE(jnet_image_gpu.depth);
    JNET_RELEASE(jnet_image_gpu.device);jnet_image_gpu.failed=0;
}
static volatile LONG jnet_image_stages_seen;
static void jnet_image_stage(LONG stage,const char *text) {
    InterlockedExchange(&JNETMenu[11],stage);
    LONG mask=1L<<stage;
    if(!(InterlockedOr(&jnet_image_stages_seen,mask)&mask))message(text);
}
static int jnet_image_failed(HRESULT result) {
    if(FAILED(result)) {InterlockedExchange(&JNETMenu[12],(LONG)result);return 1;}
    return 0;
}
struct jnet_image_saved {
    ID3D11RenderTargetView *targets[8];
    ID3D11DepthStencilView *depth_view;
    ID3D11UnorderedAccessView *uavs[8];
    UINT target_count;
    ID3D11BlendState *blend;float blend_factor[4];UINT sample_mask;
    ID3D11DepthStencilState *depth;UINT stencil_ref;
    ID3D11RasterizerState *raster;
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];UINT viewport_count;
    ID3D11InputLayout *layout;D3D11_PRIMITIVE_TOPOLOGY topology;
    ID3D11VertexShader *vs;ID3D11PixelShader *ps;ID3D11GeometryShader *gs;
    ID3D11HullShader *hs;ID3D11DomainShader *ds;
    ID3D11ClassInstance *vc[256],*pc[256],*gc[256],*hc[256],*dc[256];
    UINT vn,pn,gn,hn,dn;
    ID3D11Buffer *vertices;UINT stride,offset;
    ID3D11Buffer *vb,*pb;ID3D11ShaderResourceView *resource;ID3D11SamplerState *sampler;
};
static void jnet_image_save(ID3D11DeviceContext *c,struct jnet_image_saved *s) {
    memset(s,0,sizeof(*s));
    c->lpVtbl->OMGetRenderTargets(c,8,s->targets,&s->depth_view);
    c->lpVtbl->OMGetRenderTargetsAndUnorderedAccessViews(c,0,NULL,NULL,0,8,s->uavs);
    for(UINT i=0;i<8;++i)if(s->targets[i])s->target_count=i+1;
    c->lpVtbl->OMGetBlendState(c,&s->blend,s->blend_factor,&s->sample_mask);
    c->lpVtbl->OMGetDepthStencilState(c,&s->depth,&s->stencil_ref);
    c->lpVtbl->RSGetState(c,&s->raster);
    s->viewport_count=D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    c->lpVtbl->RSGetViewports(c,&s->viewport_count,s->viewports);
    c->lpVtbl->IAGetVertexBuffers(c,0,1,&s->vertices,&s->stride,&s->offset);
    c->lpVtbl->IAGetInputLayout(c,&s->layout);c->lpVtbl->IAGetPrimitiveTopology(c,&s->topology);
#define JNET_GET_SHADER(stage,shader,classes,count) do { \
    c->lpVtbl->stage##GetShader(c,&s->shader,NULL,&s->count); \
    if(s->shader && s->count && s->count<=256) \
        c->lpVtbl->stage##GetShader(c,NULL,s->classes,&s->count); \
    if(!s->shader)s->count=0; \
} while(0)
    JNET_GET_SHADER(VS,vs,vc,vn);JNET_GET_SHADER(PS,ps,pc,pn);
    JNET_GET_SHADER(GS,gs,gc,gn);JNET_GET_SHADER(HS,hs,hc,hn);JNET_GET_SHADER(DS,ds,dc,dn);
#undef JNET_GET_SHADER
    c->lpVtbl->VSGetConstantBuffers(c,0,1,&s->vb);c->lpVtbl->PSGetConstantBuffers(c,0,1,&s->pb);
    c->lpVtbl->PSGetShaderResources(c,0,1,&s->resource);c->lpVtbl->PSGetSamplers(c,0,1,&s->sampler);
}
static int jnet_image_saved_valid(const struct jnet_image_saved *s) {
    if(s->viewport_count>D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)return 0;
#define JNET_CLASSES_VALID(classes,count) do { \
    if(s->count>256)return 0; \
    for(UINT i=0;i<s->count;++i)if(!s->classes[i])return 0; \
} while(0)
    JNET_CLASSES_VALID(vc,vn);JNET_CLASSES_VALID(pc,pn);JNET_CLASSES_VALID(gc,gn);
    JNET_CLASSES_VALID(hc,hn);JNET_CLASSES_VALID(dc,dn);
#undef JNET_CLASSES_VALID
    return 1;
}
static void jnet_image_restore(ID3D11DeviceContext *c,struct jnet_image_saved *s) {
    jnet_image_stage(16,"[JNET] restore: render targets");
    UINT last=s->target_count;
    for(UINT i=s->target_count;i<8;++i)if(s->uavs[i])last=i+1;
    if(last==s->target_count)c->lpVtbl->OMSetRenderTargets(c,s->target_count,s->targets,s->depth_view);
    else {
        UINT counts[8];for(UINT i=0;i<8;++i)counts[i]=UINT32_MAX;
        c->lpVtbl->OMSetRenderTargetsAndUnorderedAccessViews(c,s->target_count,s->targets,s->depth_view,
            s->target_count,last-s->target_count,s->uavs+s->target_count,counts+s->target_count);
    }
    jnet_image_stage(17,"[JNET] restore: blend");
    c->lpVtbl->OMSetBlendState(c,s->blend,s->blend_factor,s->sample_mask);
    jnet_image_stage(18,"[JNET] restore: depth");
    c->lpVtbl->OMSetDepthStencilState(c,s->depth,s->stencil_ref);
    jnet_image_stage(19,"[JNET] restore: raster");
    c->lpVtbl->RSSetState(c,s->raster);c->lpVtbl->RSSetViewports(c,s->viewport_count,s->viewports);
    jnet_image_stage(20,"[JNET] restore: vertex input");
    c->lpVtbl->IASetVertexBuffers(c,0,1,&s->vertices,&s->stride,&s->offset);
    c->lpVtbl->IASetInputLayout(c,s->layout);c->lpVtbl->IASetPrimitiveTopology(c,s->topology);
    jnet_image_stage(21,"[JNET] restore: vertex shader");
    c->lpVtbl->VSSetShader(c,s->vs,s->vn?s->vc:NULL,s->vn);jnet_image_stage(22,"[JNET] restore: pixel shader");
    c->lpVtbl->PSSetShader(c,s->ps,s->pn?s->pc:NULL,s->pn);
    jnet_image_stage(23,"[JNET] restore: geometry shader");
    c->lpVtbl->GSSetShader(c,s->gs,s->gn?s->gc:NULL,s->gn);jnet_image_stage(24,"[JNET] restore: hull shader");
    c->lpVtbl->HSSetShader(c,s->hs,s->hn?s->hc:NULL,s->hn);
    jnet_image_stage(25,"[JNET] restore: domain shader");
    c->lpVtbl->DSSetShader(c,s->ds,s->dn?s->dc:NULL,s->dn);
    jnet_image_stage(26,"[JNET] restore: constants");
    c->lpVtbl->VSSetConstantBuffers(c,0,1,&s->vb);c->lpVtbl->PSSetConstantBuffers(c,0,1,&s->pb);
    jnet_image_stage(27,"[JNET] restore: resources");
    c->lpVtbl->PSSetShaderResources(c,0,1,&s->resource);c->lpVtbl->PSSetSamplers(c,0,1,&s->sampler);
}
static void jnet_image_saved_release(struct jnet_image_saved *s) {
    for(UINT i=0;i<8;++i){JNET_RELEASE(s->targets[i]);JNET_RELEASE(s->uavs[i]);}
    for(UINT i=0;i<s->vn && i<256;++i)JNET_RELEASE(s->vc[i]);
    for(UINT i=0;i<s->pn && i<256;++i)JNET_RELEASE(s->pc[i]);
    for(UINT i=0;i<s->gn && i<256;++i)JNET_RELEASE(s->gc[i]);
    for(UINT i=0;i<s->hn && i<256;++i)JNET_RELEASE(s->hc[i]);
    for(UINT i=0;i<s->dn && i<256;++i)JNET_RELEASE(s->dc[i]);
    JNET_RELEASE(s->depth_view);JNET_RELEASE(s->blend);JNET_RELEASE(s->depth);
    JNET_RELEASE(s->raster);JNET_RELEASE(s->layout);JNET_RELEASE(s->vs);JNET_RELEASE(s->ps);
    JNET_RELEASE(s->gs);JNET_RELEASE(s->hs);JNET_RELEASE(s->ds);
    JNET_RELEASE(s->vertices);
    JNET_RELEASE(s->vb);JNET_RELEASE(s->pb);JNET_RELEASE(s->resource);JNET_RELEASE(s->sampler);
}
static int jnet_image_initialize(ID3D11Device *device) {
    ID3DBlob *vs=NULL,*ps=NULL,*errors=NULL;
    ID3D11Texture2D *texture=NULL;unsigned char *pixels=NULL;int ok=0;
    jnet_image_stage(3,"[JNET] images: loading shader compiler");
    HMODULE compiler=LoadLibraryW(L"d3dcompiler_47.dll");
    HRESULT (WINAPI *compile)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO *,ID3DInclude *,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob **,ID3DBlob **)=
        compiler?(void *)GetProcAddress(compiler,"D3DCompile"):NULL;
    const char *vertex_shader=
        "cbuffer C:register(b0){float4 bounds;float4 texBounds;float4 tint;}\n"
        "struct V{float4 p:SV_POSITION;float2 u:TEXCOORD0;};\n"
        "V main(float2 q:POSITION){V o;"
        "o.p=float4((bounds.xy+(bounds.zw-bounds.xy)*q),0.0,1.0);"
        "o.u=(texBounds.xy+(texBounds.zw-texBounds.xy)*q);return o;}\n";
    const char *pixel_shader=
        "cbuffer C:register(b0){float4 bounds;float4 texBounds;float4 tint;}\n"
        "Texture2D atlasTexture:register(t0);SamplerState atlasSampler:register(s0);\n"
        "float4 main(float4 p:SV_POSITION,float2 u:TEXCOORD0):SV_TARGET"
        "{return atlasTexture.Sample(atlasSampler,u)*tint;}\n";
    if(!compile) {InterlockedExchange(&JNETMenu[12],(LONG)HRESULT_FROM_WIN32(GetLastError()));goto done;}
    jnet_image_stage(4,"[JNET] images: compiling vertex shader");
    if(jnet_image_failed(compile(vertex_shader,strlen(vertex_shader),"Journet vertex",NULL,NULL,"main","vs_4_0",0,0,&vs,&errors)))goto done;
    JNET_RELEASE(errors);
    jnet_image_stage(12,"[JNET] images: compiling pixel shader");
    if(jnet_image_failed(compile(pixel_shader,strlen(pixel_shader),"Journet pixel",NULL,NULL,"main","ps_4_0",0,0,&ps,&errors)))goto done;
    JNET_RELEASE(errors);
    jnet_image_stage(13,"[JNET] images: creating vertex shader");
    if(jnet_image_failed(device->lpVtbl->CreateVertexShader(device,vs->lpVtbl->GetBufferPointer(vs),vs->lpVtbl->GetBufferSize(vs),NULL,&jnet_image_gpu.vs)))goto done;
    jnet_image_stage(14,"[JNET] images: creating pixel shader");
    if(jnet_image_failed(device->lpVtbl->CreatePixelShader(device,ps->lpVtbl->GetBufferPointer(ps),ps->lpVtbl->GetBufferSize(ps),NULL,&jnet_image_gpu.ps)))goto done;
    jnet_image_stage(15,"[JNET] images: creating quad input");
    D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
    if(jnet_image_failed(device->lpVtbl->CreateInputLayout(device,&element,1,vs->lpVtbl->GetBufferPointer(vs),vs->lpVtbl->GetBufferSize(vs),&jnet_image_gpu.layout)))goto done;
    const float corners[8]={0,0,1,0,0,1,1,1};
    D3D11_BUFFER_DESC vertices={.ByteWidth=sizeof(corners),.Usage=D3D11_USAGE_IMMUTABLE,.BindFlags=D3D11_BIND_VERTEX_BUFFER};
    D3D11_SUBRESOURCE_DATA vertex_data={corners,0,0};
    if(jnet_image_failed(device->lpVtbl->CreateBuffer(device,&vertices,&vertex_data,&jnet_image_gpu.vertices)))goto done;
    jnet_image_stage(5,"[JNET] images: creating atlas");
    pixels=HeapAlloc(GetProcessHeap(),0,1024*1024*4);if(!pixels)goto done;
    size_t used=0;
    for(size_t i=0;i<sizeof(jnet_atlas_rle);i+=6) {
        if(i+6>sizeof(jnet_atlas_rle))goto done;
        unsigned count=jnet_atlas_rle[i]|((unsigned)jnet_atlas_rle[i+1]<<8);
        if(!count || count>1024*1024-used)goto done;
        for(unsigned j=0;j<count;++j)memcpy(pixels+4*(used++),jnet_atlas_rle+i+2,4);
    }
    if(used!=1024*1024)goto done;
    D3D11_TEXTURE2D_DESC td={.Width=1024,.Height=1024,.MipLevels=1,.ArraySize=1,.Format=DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc={1,0},.Usage=D3D11_USAGE_IMMUTABLE,.BindFlags=D3D11_BIND_SHADER_RESOURCE};
    D3D11_SUBRESOURCE_DATA initial={pixels,4096,0};
    if(jnet_image_failed(device->lpVtbl->CreateTexture2D(device,&td,&initial,&texture)) ||
       jnet_image_failed(device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)texture,NULL,&jnet_image_gpu.atlas)))goto done;
    jnet_image_stage(6,"[JNET] images: creating pipeline");
    D3D11_BUFFER_DESC bd={.ByteWidth=48,.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_CONSTANT_BUFFER};
    D3D11_SAMPLER_DESC sd={.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR,.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV=D3D11_TEXTURE_ADDRESS_CLAMP,.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP,.MaxLOD=D3D11_FLOAT32_MAX,.MaxAnisotropy=1};
    D3D11_BLEND_DESC blend={0};blend.RenderTarget[0]=(D3D11_RENDER_TARGET_BLEND_DESC){TRUE,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA,D3D11_BLEND_OP_ADD,D3D11_BLEND_ONE,D3D11_BLEND_INV_SRC_ALPHA,D3D11_BLEND_OP_ADD,D3D11_COLOR_WRITE_ENABLE_ALL};
    D3D11_RASTERIZER_DESC rd={.FillMode=D3D11_FILL_SOLID,.CullMode=D3D11_CULL_NONE,.DepthClipEnable=TRUE};
    D3D11_DEPTH_STENCIL_DESC dd={.DepthEnable=FALSE,.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO,.DepthFunc=D3D11_COMPARISON_ALWAYS,
        .StencilReadMask=D3D11_DEFAULT_STENCIL_READ_MASK,.StencilWriteMask=D3D11_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS},
        .BackFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS}};
    if(jnet_image_failed(device->lpVtbl->CreateBuffer(device,&bd,NULL,&jnet_image_gpu.constants)) ||
       jnet_image_failed(device->lpVtbl->CreateSamplerState(device,&sd,&jnet_image_gpu.sampler)) ||
       jnet_image_failed(device->lpVtbl->CreateBlendState(device,&blend,&jnet_image_gpu.blend)) ||
       jnet_image_failed(device->lpVtbl->CreateRasterizerState(device,&rd,&jnet_image_gpu.raster)) ||
       jnet_image_failed(device->lpVtbl->CreateDepthStencilState(device,&dd,&jnet_image_gpu.depth)))goto done;
    ok=1;
done:
    if(!ok && errors) {
        const unsigned char *data=errors->lpVtbl->GetBufferPointer(errors);
        SIZE_T size=errors->lpVtbl->GetBufferSize(errors);
        if(size>1536)size=1536;
        for(SIZE_T at=0;data && at<size;) {
            char line[240]="[JNET] shader compiler: ";size_t n=strlen(line);
            while(at<size && n<sizeof(line)-1) {
                unsigned char ch=data[at++];
                if(ch=='\n' || !ch)break;
                line[n++]=ch>=32 && ch<=126?(char)ch:' ';
            }
            line[n]=0;message(line);
        }
    }
    if(pixels)HeapFree(GetProcessHeap(),0,pixels);
    JNET_RELEASE(texture);JNET_RELEASE(vs);JNET_RELEASE(ps);JNET_RELEASE(errors);
    if(compiler)FreeLibrary(compiler);
    LONG failed_stage=ok?0:JNETMenu[11];
    if(!ok)jnet_image_release();
    jnet_image_gpu.failed_stage=failed_stage;
    jnet_image_gpu.device=device;device->lpVtbl->AddRef(device);jnet_image_gpu.failed=!ok;
    return ok;
}
static void jnet_image_draw(uintptr_t renderer) {
    struct jnet_image_frame frame;
    AcquireSRWLockShared(&jnet_image_lock);frame=jnet_image_frame;ReleaseSRWLockShared(&jnet_image_lock);
    unsigned char any_menu=0;
    if(frame.owner)jnet_menu_read(frame.owner+0xd68,&any_menu,1);
    int menu_visible=frame.owner && jnet_menu_visible_root(frame.owner);
    jnet_chat_gameplay=frame.owner && !any_menu;
    if(any_menu && jnet_chat_focus)jnet_chat_cancel();
    if(!frame.count || frame.count>4 || !isfinite(frame.opacity) || frame.opacity<=0 || GetTickCount64()-frame.time>250 || !menu_visible)frame.count=0;
    char chat_text[1024];int chat_visible=jnet_chat_image_text(chat_text);
    if(!frame.count && !chat_visible)return;
    jnet_image_stage(1,"[JNET] images: reading renderer");
    ID3D11DeviceContext *context=NULL;IDXGISwapChain *swap=NULL;
    if(!jnet_menu_read(renderer+0xa8,&context,8) || !context ||
       !jnet_menu_read(renderer+0xb0,&swap,8) || !swap)return;
    jnet_image_stage(2,"[JNET] images: obtaining device");
    ID3D11Device *device=NULL;context->lpVtbl->GetDevice(context,&device);if(!device)return;
    if(jnet_image_gpu.device!=device) {jnet_image_release();jnet_image_initialize(device);}
    if(jnet_image_gpu.failed) {InterlockedExchange(&JNETMenu[11],jnet_image_gpu.failed_stage);InterlockedExchange(&jnet_image_result,2);JNET_RELEASE(device);return;}
    jnet_image_stage(7,"[JNET] images: acquiring back buffer");
    static const GUID texture_id={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
    ID3D11Texture2D *back=NULL;ID3D11RenderTargetView *target=NULL;
    if(jnet_image_failed(swap->lpVtbl->GetBuffer(swap,0,&texture_id,(void **)&back)) ||
       jnet_image_failed(device->lpVtbl->CreateRenderTargetView(device,(ID3D11Resource *)back,NULL,&target))) {
        HRESULT reason=device->lpVtbl->GetDeviceRemovedReason(device);
        if(FAILED(reason)){
            char line[160];snprintf(line,sizeof(line),"[JNET] menu images suspended: graphics device unavailable; removed reason=%08lx",(unsigned long)reason);
            message(line);jnet_image_gpu.failed=1;jnet_image_gpu.failed_stage=7;
        }
        InterlockedExchange(&jnet_image_result,3);goto done;
    }
    D3D11_TEXTURE2D_DESC desc;back->lpVtbl->GetDesc(back,&desc);
    struct jnet_image_saved saved;
    jnet_image_stage(8,"[JNET] images: saving drawing state");
    jnet_image_save(context,&saved);
    if(!jnet_image_saved_valid(&saved)) {
        InterlockedExchange(&jnet_image_result,4);
        jnet_image_saved_release(&saved);goto done;
    }
    jnet_image_stage(9,"[JNET] images: drawing previews");
    context->lpVtbl->GSSetShader(context,NULL,NULL,0);
    context->lpVtbl->HSSetShader(context,NULL,NULL,0);
    context->lpVtbl->DSSetShader(context,NULL,NULL,0);
    context->lpVtbl->OMSetRenderTargets(context,1,&target,NULL);
    context->lpVtbl->OMSetBlendState(context,jnet_image_gpu.blend,NULL,UINT32_MAX);
    context->lpVtbl->OMSetDepthStencilState(context,jnet_image_gpu.depth,0);
    context->lpVtbl->RSSetState(context,jnet_image_gpu.raster);
    D3D11_VIEWPORT viewport={0,0,(float)desc.Width,(float)desc.Height,0,1};context->lpVtbl->RSSetViewports(context,1,&viewport);
    UINT stride=2*sizeof(float),offset=0;
    context->lpVtbl->IASetVertexBuffers(context,0,1,&jnet_image_gpu.vertices,&stride,&offset);
    context->lpVtbl->IASetInputLayout(context,jnet_image_gpu.layout);context->lpVtbl->IASetPrimitiveTopology(context,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->lpVtbl->VSSetShader(context,jnet_image_gpu.vs,NULL,0);context->lpVtbl->PSSetShader(context,jnet_image_gpu.ps,NULL,0);
    context->lpVtbl->VSSetConstantBuffers(context,0,1,&jnet_image_gpu.constants);context->lpVtbl->PSSetConstantBuffers(context,0,1,&jnet_image_gpu.constants);
    context->lpVtbl->PSSetShaderResources(context,0,1,&jnet_image_gpu.atlas);context->lpVtbl->PSSetSamplers(context,0,1,&jnet_image_gpu.sampler);
    for(unsigned i=0;i<frame.count;++i) {
        struct jnet_image_item *item=&frame.items[i];if(item->asset>=42)continue;
        const struct jnet_asset_rect *r=&jnet_assets[item->asset];
        float box[4];
        if(!jnet_menu_image_fit(box,item->box,r->w*item->fraction,(float)r->h,desc.Width,desc.Height))continue;
        float constants[12]={box[0],box[1],box[2],box[3],
            (r->x+.5f)/1024.f,(r->y+.5f)/1024.f,(r->x+r->w*item->fraction-.5f)/1024.f,(r->y+r->h-.5f)/1024.f,1,1,1,frame.opacity};
        context->lpVtbl->UpdateSubresource(context,(ID3D11Resource *)jnet_image_gpu.constants,0,NULL,constants,0,0);
        context->lpVtbl->Draw(context,4,0);
    }
    if(chat_visible && jnet_chat_image_update(device,context,chat_text)){
        float size=.07f;
        if(frame.owner)jnet_menu_read(frame.owner+0xc48,&size,4);
        if(!isfinite(size) || size<=0 || size>.15f)size=.07f;
        float height=384.f*size/40.f,width=1024.f*size/40.f*(float)desc.Height/(float)desc.Width;
        float left=.96f-width;
        float bottom=-.94f;
        float constants[12]={left,bottom+height,left+width,bottom,0,0,1,1,1,1,1,jnet_chat_opacity};
        context->lpVtbl->PSSetShaderResources(context,0,1,&jnet_chat_resource);
        context->lpVtbl->UpdateSubresource(context,(ID3D11Resource *)jnet_image_gpu.constants,0,NULL,constants,0,0);
        context->lpVtbl->Draw(context,4,0);
    }
    context->lpVtbl->OMSetRenderTargets(context,0,NULL,NULL);
    jnet_image_stage(10,"[JNET] images: restoring drawing state");
    jnet_image_restore(context,&saved);
    jnet_image_stage(28,"[JNET] restore: releasing saved references");
    jnet_image_saved_release(&saved);
    jnet_image_stage(11,"[JNET] images: frame complete");
    InterlockedExchange(&jnet_image_result,0);InterlockedIncrement(&jnet_image_frames);
done:
    JNET_RELEASE(target);JNET_RELEASE(back);JNET_RELEASE(device);
}
static unsigned char jnet_present(uintptr_t renderer) {
    jnet_image_draw(renderer);
    LONG fps=jnet_fps_limit;
    jnet_frame_wait(fps==1?30:fps==2?60:0);
    return jnet_original_present(renderer);
}
#undef JNET_RELEASE
#endif
