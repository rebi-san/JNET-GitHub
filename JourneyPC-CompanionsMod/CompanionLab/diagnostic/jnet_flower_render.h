#ifndef JNET_FLOWER_RENDER_H
#define JNET_FLOWER_RENDER_H
/* Draws the Flower Trail flowers with a private D3D11 pass on the back buffer, just before the menu overlay. Included by
   jnet_menu_images.h, so it reuses that file's pipeline state save/restore. The game thread owns the trail and camera (see
   jnet_flower_live.h) and publishes them under jnet_flower_lock; this thread only reads a snapshot.
   The pass has no access to the scene's depth, so it keeps a private depth buffer for flower-to-flower ordering, hides flowers whose
   line of sight crosses terrain (decided on the game thread) and fades distant ones. */
#include "jnet_flower_trail.h"
#include "jnet_flower_view.h"
#include "jnet_flower_dds.h"
#include "../work/flower/jnet_flower_assets.h"

static SRWLOCK jnet_flower_lock=SRWLOCK_INIT;
static struct jnet_flower_trail jnet_flower_trail_state;
static unsigned char jnet_flower_hidden[JNET_FLOWER_CAPACITY];
static struct jnet_flower_camera jnet_flower_camera_state;
static ULONGLONG jnet_flower_camera_time;
static volatile LONG jnet_flower_frames,jnet_flower_failure;
static struct {
    ID3D11Device *device;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11InputLayout *layout;
    ID3D11Buffer *vertices,*indices,*constants;
    ID3D11ShaderResourceView *atlas,*grass;
    ID3D11SamplerState *sampler;
    ID3D11BlendState *blend;
    ID3D11RasterizerState *raster;
    ID3D11DepthStencilState *depth_state;
    ID3D11Texture2D *depth_texture;
    ID3D11DepthStencilView *depth_view;
    UINT depth_width,depth_height,depth_samples,depth_quality;
    int failed,compatible_pixel;
} jnet_flower_gpu;
struct jnet_flower_constants {float matrix[16],turn[4],anim[4],light[4],depth[4];};

#include "jnet_flower_scene.h"
static void jnet_flower_release(void) {
    jnet_flower_scene_release();
    JNET_RELEASE(jnet_flower_gpu.vs);JNET_RELEASE(jnet_flower_gpu.ps);JNET_RELEASE(jnet_flower_gpu.layout);
    JNET_RELEASE(jnet_flower_gpu.vertices);JNET_RELEASE(jnet_flower_gpu.indices);JNET_RELEASE(jnet_flower_gpu.constants);
    JNET_RELEASE(jnet_flower_gpu.atlas);JNET_RELEASE(jnet_flower_gpu.grass);JNET_RELEASE(jnet_flower_gpu.sampler);JNET_RELEASE(jnet_flower_gpu.blend);
    JNET_RELEASE(jnet_flower_gpu.raster);JNET_RELEASE(jnet_flower_gpu.depth_state);
    JNET_RELEASE(jnet_flower_gpu.depth_view);JNET_RELEASE(jnet_flower_gpu.depth_texture);
    JNET_RELEASE(jnet_flower_gpu.device);
    jnet_flower_gpu.depth_width=jnet_flower_gpu.depth_height=jnet_flower_gpu.depth_samples=jnet_flower_gpu.depth_quality=0;
}
static void jnet_flower_fail(const char *stage,HRESULT result) {
    char line[160];snprintf(line,sizeof(line),"[JNET] flower trail unavailable: %.80s (%08lx)",stage,(unsigned long)result);
    message(line);jnet_flower_gpu.failed=1;InterlockedExchange(&jnet_flower_failure,(LONG)result?(LONG)result:1);
}
/* Reads a BGRA DDS with mip levels from beside the DLL: Flower's 1024x2048 petal atlas, or the 256x256 grass sheet. */
static ID3D11ShaderResourceView *jnet_flower_load_texture(ID3D11Device *device,const wchar_t *name,unsigned width,unsigned height) {
    wchar_t path[MAX_PATH];DWORD length=GetModuleFileNameW(self,path,MAX_PATH);
    wchar_t *slash=length && length<MAX_PATH?wcsrchr(path,L'\\'):NULL;
    if(!slash || (size_t)(slash-path)+wcslen(name)+2>MAX_PATH){jnet_flower_fail("texture path",0);return NULL;}
    wcscpy(slash+1,name);
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE){jnet_flower_fail("texture file missing",HRESULT_FROM_WIN32(GetLastError()));return NULL;}
    LARGE_INTEGER size;ID3D11ShaderResourceView *view=NULL;unsigned char *data=NULL;
    if(!GetFileSizeEx(file,&size) || size.QuadPart<128 || size.QuadPart>(64<<20)){jnet_flower_fail("texture size",0);goto done;}
    data=HeapAlloc(GetProcessHeap(),0,(SIZE_T)size.QuadPart);
    DWORD read=0;
    if(!data || !ReadFile(file,data,(DWORD)size.QuadPart,&read,NULL) || read!=(DWORD)size.QuadPart){jnet_flower_fail("texture read",0);goto done;}
    struct jnet_flower_dds_level levels[JNET_FLOWER_DDS_MAX_LEVELS];
    unsigned count=jnet_flower_dds_levels(data,(size_t)size.QuadPart,width,height,levels);
    if(!count){jnet_flower_fail("texture format",0);goto done;}
    D3D11_SUBRESOURCE_DATA initial[JNET_FLOWER_DDS_MAX_LEVELS]={0};
    for(unsigned i=0;i<count;++i){initial[i].pSysMem=levels[i].pixels;initial[i].SysMemPitch=levels[i].width*4;}
    D3D11_TEXTURE2D_DESC desc={.Width=width,.Height=height,.MipLevels=count,.ArraySize=1,
        .Format=DXGI_FORMAT_B8G8R8A8_UNORM,.SampleDesc={1,0},.Usage=D3D11_USAGE_IMMUTABLE,.BindFlags=D3D11_BIND_SHADER_RESOURCE};
    ID3D11Texture2D *texture=NULL;
    HRESULT result=device->lpVtbl->CreateTexture2D(device,&desc,initial,&texture);
    if(FAILED(result)){jnet_flower_fail("texture creation",result);goto done;}
    result=device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)texture,NULL,&view);
    texture->lpVtbl->Release(texture);
    if(FAILED(result)){jnet_flower_fail("texture view",result);view=NULL;}
done:
    if(data)HeapFree(GetProcessHeap(),0,data);
    CloseHandle(file);
    return view;
}
static void jnet_flower_compile_error(ID3DBlob *errors) {
    if(!errors)return;
    char line[1024];SIZE_T size=errors->lpVtbl->GetBufferSize(errors);
    snprintf(line,sizeof(line),"[JNET] flower shader compiler: %.*s",
        (int)(size<900?size:900),(const char *)errors->lpVtbl->GetBufferPointer(errors));
    message(line);
}
static int jnet_flower_initialize(ID3D11Device *device) {
    /* Vertex kind rides in the colour alpha: 0 stem, 0.5 grass, 1 petal. Every vertex also carries its closed-bud position and colour; the
       flower's bloom (cbuffer anim.x) morphs between them, the glow (anim.y) brightens and shivers petals, and grass sways with time. */
    static const char vertex_source[]=
        "cbuffer C:register(b0){row_major float4x4 wvp;float4 turn;float4 anim;float4 light;float4 depth;}\n"
        "struct V{float3 p:POSITION0;float3 n:NORMAL;float2 u:TEXCOORD0;float4 c:COLOR0;float3 p0:POSITION1;float4 c0:COLOR1;};\n"
        "struct O{float4 p:SV_POSITION;float3 n:TEXCOORD1;float2 u:TEXCOORD0;float4 c:COLOR0;};\n"
        "O main(V i){O o;float kind=i.c.a;float3 p=lerp(i.p0,i.p,anim.x);float4 c=lerp(i.c0,i.c,anim.x);\n"
        "if(kind>.2&&kind<.8){float h=saturate(p.y/.3);float k=h*h*(1+anim.y*2);\n"
        "p.x+=sin(turn.w*1.7+p.z*40+p.x*23)*.012*k;p.z+=cos(turn.w*1.3+p.x*37)*.01*k;}\n"
        "else if(kind>=.8)p+=i.n*sin(turn.w*28+p.x*90+p.z*70)*.006*anim.y;\n"
        "c.rgb*=1+anim.y*.9;\n"
        "o.p=mul(float4(p,1),wvp);"
        "o.n=float3(i.n.x*turn.x+i.n.z*turn.y,i.n.y,i.n.z*turn.x-i.n.x*turn.y);o.u=i.u;o.c=c;return o;}\n";
    /* Petals multiply their vertex colour (Flower's base-to-tip gradient) by the atlas and are cut out by its alpha; grass is cut out by the
       grass sheet; darks are lifted because Flower's flowers are dull until woken. The scene's own colour around the flower's base
       (light.xy, when light.z is set) supplies the brightness and tint, so flowers dim at night, in shade and in caves and pick up fog and
       sunset colour. The game's depth buffer (when light.w is set) hides pixels that something in front of the flower covers: depth.xyz
       are near, far and whether depth is reversed, depth.w the bias in world units. */
    static const char pixel_source[]=
        "cbuffer C:register(b0){row_major float4x4 wvp;float4 turn;float4 anim;float4 light;float4 depth;}\n"
        "Texture2D atlasTexture:register(t0);Texture2D grassTexture:register(t1);Texture2D sceneColor:register(t2);Texture2DMS<float> sceneDepth:register(t3);\n"
        "SamplerState atlasSampler:register(s0);\n"
        "struct O{float4 p:SV_POSITION;float3 n:TEXCOORD1;float2 u:TEXCOORD0;float4 c:COLOR0;};\n"
        "float4 main(O i):SV_TARGET{float kind=i.c.a;float3 albedo;float glow=0;\n"
        "if(kind>.8){float4 t=atlasTexture.Sample(atlasSampler,i.u);clip(t.a-.5);albedo=pow(max(i.c.rgb,0),.55)*lerp(float3(1,1,1),t.rgb*1.8,.65);glow=anim.y;}\n"
        "else if(kind>.2){float4 t=grassTexture.Sample(atlasSampler,i.u);clip(t.a-.5);albedo=pow(max(i.c.rgb,0),.8);}\n"
        "else albedo=pow(max(i.c.rgb,0),.55);\n"
        "if(light.w>.5){float d=sceneDepth.Load(int2(i.p.xy),0).r;if(depth.z>.5)d=1-d;\n"
        "float zs=depth.x*depth.y/(depth.y-d*(depth.y-depth.x));float zf=depth.x*depth.y/(depth.y-i.p.z*(depth.y-depth.x));\n"
        "if(zs<zf-depth.w)discard;}\n"
        "float3 n=normalize(i.n);float sun=saturate(dot(n,normalize(float3(.35,.8,.45))));"
        "float lit=.55+.2*n.y+.5*sun;float3 colour=albedo*lit*1.15;\n"
        "if(light.z>.5){float3 s=sceneColor.SampleLevel(atlasSampler,light.xy,0).rgb;\n"
        "s+=sceneColor.SampleLevel(atlasSampler,light.xy+float2(.03,0),0).rgb;s+=sceneColor.SampleLevel(atlasSampler,light.xy-float2(.03,0),0).rgb;\n"
        "s+=sceneColor.SampleLevel(atlasSampler,light.xy+float2(0,.04),0).rgb;s+=sceneColor.SampleLevel(atlasSampler,light.xy-float2(0,.04),0).rgb;s*=.2;\n"
        "float lum=dot(s,float3(.30,.59,.11));float3 tint=lerp(float3(1,1,1),s/max(lum,.04),.5);colour*=tint*lerp(.2,1.25,saturate(lum*1.6));}\n"
        "colour+=float3(1,.9,.6)*glow*.25;\n"
        "return float4(colour,turn.z);}\n";
    static const char vertex_fallback[]=
        "cbuffer C:register(b0){float4 transform0;float4 transform1;float4 transform2;float4 transform3;float4 turn;float4 anim;float4 light;float4 depth;}"
        "struct V{float3 p:POSITION0;float3 n:NORMAL;float2 u:TEXCOORD0;float4 c:COLOR0;float3 p0:POSITION1;float4 c0:COLOR1;};"
        "struct O{float4 p:SV_POSITION;float3 n:TEXCOORD1;float2 u:TEXCOORD0;float4 c:COLOR0;};"
        "O main(V i){O o;float3 p=i.p0+(i.p-i.p0)*anim.x;o.p=p.x*transform0+p.y*transform1+p.z*transform2+transform3;"
        "o.n=float3(i.n.x*turn.x+i.n.z*turn.y,i.n.y,i.n.z*turn.x-i.n.x*turn.y);"
        "o.u=i.u;o.c=(i.c0+(i.c-i.c0)*anim.x);o.c.rgb*=1+anim.y*.9;return o;}";
    static const char pixel_fallback[]=
        "cbuffer C:register(b0){float4 transform0;float4 transform1;float4 transform2;float4 transform3;float4 turn;float4 anim;float4 light;float4 depth;}"
        "Texture2D atlasTexture:register(t0);Texture2D grassTexture:register(t1);SamplerState atlasSampler:register(s0);"
        "struct O{float4 p:SV_POSITION;float3 n:TEXCOORD1;float2 u:TEXCOORD0;float4 c:COLOR0;};"
        "float4 main(O i):SV_TARGET{float4 t=float4(1,1,1,1);"
        "if(i.c.a>.8)t=atlasTexture.Sample(atlasSampler,i.u);"
        "else if(i.c.a>.2)t=grassTexture.Sample(atlasSampler,i.u);"
        "float alpha=turn.z;if(t.a<.5)alpha=0;float ny=i.n.y;if(ny<0)ny=0;if(ny>1)ny=1;return float4(i.c.rgb*t.rgb*(.8+.4*ny),alpha);}";
    ID3DBlob *vs=NULL,*ps=NULL,*errors=NULL;int ok=0;
    jnet_flower_gpu.compatible_pixel=0;
    HMODULE compiler=LoadLibraryW(L"d3dcompiler_47.dll");
    HRESULT (WINAPI *compile)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO *,ID3DInclude *,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob **,ID3DBlob **)=
        compiler?(void *)GetProcAddress(compiler,"D3DCompile"):NULL;
    HRESULT result;
    if(!compile){jnet_flower_fail("shader compiler",HRESULT_FROM_WIN32(GetLastError()));goto done;}
    if(FAILED(result=compile(vertex_source,sizeof(vertex_source)-1,"Flower vertex",NULL,NULL,"main","vs_4_0",0,0,&vs,&errors))){
        jnet_flower_compile_error(errors);JNET_RELEASE(errors);JNET_RELEASE(vs);
        if(FAILED(result=compile(vertex_fallback,sizeof(vertex_fallback)-1,"Flower vertex fallback",NULL,NULL,"main","vs_4_0",0,0,&vs,&errors))){
            jnet_flower_compile_error(errors);jnet_flower_fail("vertex shader compile",result);goto done;
        }
        message("[JNET] flower trail using compatible vertex shader; wind sway disabled");
    }
    JNET_RELEASE(errors);
    if(FAILED(result=compile(pixel_source,sizeof(pixel_source)-1,"Flower pixel",NULL,NULL,"main","ps_4_1",0,0,&ps,&errors))){
        jnet_flower_compile_error(errors);JNET_RELEASE(errors);JNET_RELEASE(ps);
        if(FAILED(result=compile(pixel_fallback,sizeof(pixel_fallback)-1,"Flower pixel fallback",NULL,NULL,"main","ps_4_0",0,0,&ps,&errors))){
            jnet_flower_compile_error(errors);jnet_flower_fail("pixel shader compile",result);goto done;
        }
        jnet_flower_gpu.compatible_pixel=1;
        message("[JNET] flower trail using compatible pixel shader; scene lighting and depth sampling disabled");
    }
    if(FAILED(result=device->lpVtbl->CreateVertexShader(device,vs->lpVtbl->GetBufferPointer(vs),vs->lpVtbl->GetBufferSize(vs),NULL,&jnet_flower_gpu.vs)) ||
       FAILED(result=device->lpVtbl->CreatePixelShader(device,ps->lpVtbl->GetBufferPointer(ps),ps->lpVtbl->GetBufferSize(ps),NULL,&jnet_flower_gpu.ps))){jnet_flower_fail("shader creation",result);goto done;}
    D3D11_INPUT_ELEMENT_DESC elements[]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"POSITION",1,DXGI_FORMAT_R32G32B32_FLOAT,0,36,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",1,DXGI_FORMAT_R8G8B8A8_UNORM,0,48,D3D11_INPUT_PER_VERTEX_DATA,0}};
    if(FAILED(result=device->lpVtbl->CreateInputLayout(device,elements,6,vs->lpVtbl->GetBufferPointer(vs),vs->lpVtbl->GetBufferSize(vs),&jnet_flower_gpu.layout))){jnet_flower_fail("input layout",result);goto done;}
    D3D11_BUFFER_DESC vertex_desc={.ByteWidth=sizeof(jnet_flower_vertices),.Usage=D3D11_USAGE_IMMUTABLE,.BindFlags=D3D11_BIND_VERTEX_BUFFER};
    D3D11_SUBRESOURCE_DATA vertex_data={jnet_flower_vertices,0,0};
    D3D11_BUFFER_DESC index_desc={.ByteWidth=sizeof(jnet_flower_indices),.Usage=D3D11_USAGE_IMMUTABLE,.BindFlags=D3D11_BIND_INDEX_BUFFER};
    D3D11_SUBRESOURCE_DATA index_data={jnet_flower_indices,0,0};
    D3D11_BUFFER_DESC constant_desc={.ByteWidth=sizeof(struct jnet_flower_constants),.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_CONSTANT_BUFFER};
    if(FAILED(result=device->lpVtbl->CreateBuffer(device,&vertex_desc,&vertex_data,&jnet_flower_gpu.vertices)) ||
       FAILED(result=device->lpVtbl->CreateBuffer(device,&index_desc,&index_data,&jnet_flower_gpu.indices)) ||
       FAILED(result=device->lpVtbl->CreateBuffer(device,&constant_desc,NULL,&jnet_flower_gpu.constants))){jnet_flower_fail("buffers",result);goto done;}
    D3D11_SAMPLER_DESC sampler={.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR,.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP,.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressW=D3D11_TEXTURE_ADDRESS_CLAMP,.MaxLOD=D3D11_FLOAT32_MAX,.MaxAnisotropy=1,.ComparisonFunc=D3D11_COMPARISON_NEVER};
    D3D11_BLEND_DESC blend={0};
    blend.RenderTarget[0]=(D3D11_RENDER_TARGET_BLEND_DESC){TRUE,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA,D3D11_BLEND_OP_ADD,
        D3D11_BLEND_ONE,D3D11_BLEND_INV_SRC_ALPHA,D3D11_BLEND_OP_ADD,D3D11_COLOR_WRITE_ENABLE_ALL};
    D3D11_RASTERIZER_DESC raster={.FillMode=D3D11_FILL_SOLID,.CullMode=D3D11_CULL_NONE,.DepthClipEnable=TRUE};
    D3D11_DEPTH_STENCIL_DESC depth={.DepthEnable=TRUE,.DepthWriteMask=jnet_flower_gpu.compatible_pixel?D3D11_DEPTH_WRITE_MASK_ZERO:D3D11_DEPTH_WRITE_MASK_ALL,.DepthFunc=D3D11_COMPARISON_LESS,
        .StencilReadMask=D3D11_DEFAULT_STENCIL_READ_MASK,.StencilWriteMask=D3D11_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS},
        .BackFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS}};
    if(FAILED(result=device->lpVtbl->CreateSamplerState(device,&sampler,&jnet_flower_gpu.sampler)) ||
       FAILED(result=device->lpVtbl->CreateBlendState(device,&blend,&jnet_flower_gpu.blend)) ||
       FAILED(result=device->lpVtbl->CreateRasterizerState(device,&raster,&jnet_flower_gpu.raster)) ||
       FAILED(result=device->lpVtbl->CreateDepthStencilState(device,&depth,&jnet_flower_gpu.depth_state))){jnet_flower_fail("pipeline state",result);goto done;}
    if(!(jnet_flower_gpu.atlas=jnet_flower_load_texture(device,JNET_FLOWER_ATLAS_FILE,JNET_FLOWER_ATLAS_WIDTH,JNET_FLOWER_ATLAS_HEIGHT)) ||
       !(jnet_flower_gpu.grass=jnet_flower_load_texture(device,JNET_FLOWER_GRASS_FILE,JNET_FLOWER_GRASS_SIZE,JNET_FLOWER_GRASS_SIZE)))goto done;
    ok=1;
    /* The scene probe is optional: without it flowers still draw, just without depth occlusion. */
    if(!jnet_flower_gpu.compatible_pixel)jnet_flower_scene_initialize(device,compile);
done:
    JNET_RELEASE(vs);JNET_RELEASE(ps);JNET_RELEASE(errors);
    if(compiler)FreeLibrary(compiler);
    if(!ok){
        int failed=jnet_flower_gpu.failed;jnet_flower_release();jnet_flower_gpu.failed=failed?failed:1;
    }
    jnet_flower_gpu.device=device;device->lpVtbl->AddRef(device);
    if(ok)message("[JNET] flower trail renderer ready");
    return ok;
}
/* The depth buffer must have the render target's sample count and quality or D3D drops every draw. Journey's swap-chain buffer is 8x multisampled. */
static int jnet_flower_depth(ID3D11Device *device,UINT width,UINT height,DXGI_SAMPLE_DESC samples) {
    if(jnet_flower_gpu.depth_view && jnet_flower_gpu.depth_width==width && jnet_flower_gpu.depth_height==height &&
       jnet_flower_gpu.depth_samples==samples.Count && jnet_flower_gpu.depth_quality==samples.Quality)return 1;
    JNET_RELEASE(jnet_flower_gpu.depth_view);JNET_RELEASE(jnet_flower_gpu.depth_texture);
    D3D11_TEXTURE2D_DESC desc={.Width=width,.Height=height,.MipLevels=1,.ArraySize=1,.Format=DXGI_FORMAT_D32_FLOAT,
        .SampleDesc=samples,.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_DEPTH_STENCIL};
    if(FAILED(device->lpVtbl->CreateTexture2D(device,&desc,NULL,&jnet_flower_gpu.depth_texture)) ||
       FAILED(device->lpVtbl->CreateDepthStencilView(device,(ID3D11Resource *)jnet_flower_gpu.depth_texture,NULL,&jnet_flower_gpu.depth_view))){
        JNET_RELEASE(jnet_flower_gpu.depth_view);JNET_RELEASE(jnet_flower_gpu.depth_texture);return 0;
    }
    jnet_flower_gpu.depth_width=width;jnet_flower_gpu.depth_height=height;
    jnet_flower_gpu.depth_samples=samples.Count;jnet_flower_gpu.depth_quality=samples.Quality;return 1;
}
struct jnet_flower_draw_item {float distance,fade;unsigned index;};
static int jnet_flower_far_first(const void *a,const void *b) {
    float x=((const struct jnet_flower_draw_item *)a)->distance,y=((const struct jnet_flower_draw_item *)b)->distance;
    return x<y?1:x>y?-1:0;
}
/* Why the pass did nothing, logged when it changes: 1 level differs, 2 travelling, 3 no camera yet, 4 camera stale, 5 menu open, 6 GPU unavailable. */
static void jnet_flower_gate(unsigned reason) {
    static unsigned last;
    if(reason==last)return;
    last=reason;
    char line[160];snprintf(line,sizeof(line),"[JNET] flower trail not drawing: reason %u (0 drawing,1 level,2 travel,3 no camera,4 stale camera,5 menu,6 gpu)",reason);
    message(line);
}
static void jnet_flower_draw_pass(uintptr_t renderer) {
    static struct jnet_flower_trail snapshot;
    static unsigned char hidden[JNET_FLOWER_CAPACITY];
    static struct jnet_flower_draw_item order[JNET_FLOWER_CAPACITY];
    struct jnet_flower_camera camera;ULONGLONG camera_time;
    AcquireSRWLockShared(&jnet_flower_lock);
    if(!jnet_flower_trail_state.count){ReleaseSRWLockShared(&jnet_flower_lock);return;}
    snapshot=jnet_flower_trail_state;memcpy(hidden,jnet_flower_hidden,sizeof(hidden));
    camera=jnet_flower_camera_state;camera_time=jnet_flower_camera_time;
    ReleaseSRWLockShared(&jnet_flower_lock);
    ULONGLONG now=GetTickCount64();
    uint32_t now32=(uint32_t)now;float clock_seconds=(float)(now%3600000)/1000.f;
    if(snapshot.level!=jnet_current_level){jnet_flower_gate(1);return;}
    if(jnet_travel_busy){jnet_flower_gate(2);return;}
    if(!camera_time){jnet_flower_gate(3);return;}
    if(now<camera_time || now-camera_time>10000){jnet_flower_gate(4);return;}
    if(jnet_flower_gpu.failed){jnet_flower_gate(6);return;}
    ID3D11DeviceContext *context=NULL;IDXGISwapChain *swap=NULL;
    if(!jnet_menu_read(renderer+0xa8,&context,8) || !context || !jnet_menu_read(renderer+0xb0,&swap,8) || !swap)return;
    ID3D11Device *device=NULL;context->lpVtbl->GetDevice(context,&device);if(!device)return;
    if(jnet_flower_gpu.device!=device){jnet_flower_release();jnet_flower_gpu.failed=0;jnet_flower_initialize(device);}
    if(jnet_flower_gpu.failed){JNET_RELEASE(device);return;}
    static const GUID texture_id={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
    ID3D11Texture2D *back=NULL;ID3D11RenderTargetView *target=NULL;
    if(FAILED(swap->lpVtbl->GetBuffer(swap,0,&texture_id,(void **)&back)) ||
       FAILED(device->lpVtbl->CreateRenderTargetView(device,(ID3D11Resource *)back,NULL,&target)))goto done;
    D3D11_TEXTURE2D_DESC desc;back->lpVtbl->GetDesc(back,&desc);
    if(!desc.Width || !desc.Height || !jnet_flower_depth(device,desc.Width,desc.Height,desc.SampleDesc))goto done;
    float view_projection[16];
    jnet_flower_view_projection(&camera,(float)desc.Width/(float)desc.Height,view_projection);
    unsigned visible=0,hidden_count=0,faded=0,outside=0;float nearest=1e30f;unsigned nearest_index=0;
    for(unsigned i=0;i<snapshot.count;++i){
        const struct jnet_flower_item *item=&snapshot.items[i];
        float distance=jnet_flower_distance(&camera,item->position),fade=jnet_flower_fade(distance);
        if(distance<nearest){nearest=distance;nearest_index=i;}
        if(hidden[i]){++hidden_count;continue;}
        if(fade<=0 || item->type>=JNET_FLOWER_MODEL_COUNT){++faded;continue;}
        if(!jnet_flower_in_view(view_projection,item)){++outside;continue;}
        order[visible++]=(struct jnet_flower_draw_item){distance,fade,i};
    }
    jnet_flower_gate(0);
    static ULONGLONG report_next;
    if(now>=report_next && snapshot.count){
        report_next=now+30000;
        const struct jnet_flower_item *item=&snapshot.items[nearest_index];
        float clip[4]={0},centre[3]={item->position[0],item->position[1]+.5f*item->scale,item->position[2]};
        jnet_flower_clip(view_projection,centre,clip);
        char line[300];
        snprintf(line,sizeof(line),"[JNET] flower trail frame: %u flowers, %u to draw, %u behind terrain, %u too far, %u outside view, %ld frames drawn; nearest %.1f away at %.1f %.1f %.1f, clip %.2f %.2f %.2f w %.2f, view %ux%u",
            snapshot.count,visible,hidden_count,faded,outside,(long)jnet_flower_frames,(double)nearest,(double)item->position[0],
            (double)item->position[1],(double)item->position[2],(double)clip[0],(double)clip[1],(double)clip[2],(double)clip[3],desc.Width,desc.Height);
        message(line);
    }
    if(!visible)goto done;
    qsort(order,visible,sizeof(order[0]),jnet_flower_far_first);
    struct jnet_image_saved saved;
    jnet_image_save(context,&saved);
    if(!jnet_image_saved_valid(&saved)){jnet_image_saved_release(&saved);goto done;}
    unsigned scene=jnet_flower_gpu.compatible_pixel?0:jnet_flower_scene_prepare(device,context,back,&desc,now);
    jnet_flower_scene.busy=1;
    context->lpVtbl->GSSetShader(context,NULL,NULL,0);
    context->lpVtbl->HSSetShader(context,NULL,NULL,0);
    context->lpVtbl->DSSetShader(context,NULL,NULL,0);
    context->lpVtbl->OMSetRenderTargets(context,1,&target,jnet_flower_gpu.depth_view);
    context->lpVtbl->ClearDepthStencilView(context,jnet_flower_gpu.depth_view,D3D11_CLEAR_DEPTH,1.f,0);
    context->lpVtbl->OMSetBlendState(context,jnet_flower_gpu.blend,NULL,UINT32_MAX);
    context->lpVtbl->OMSetDepthStencilState(context,jnet_flower_gpu.depth_state,0);
    context->lpVtbl->RSSetState(context,jnet_flower_gpu.raster);
    D3D11_VIEWPORT viewport={0,0,(float)desc.Width,(float)desc.Height,0,1};context->lpVtbl->RSSetViewports(context,1,&viewport);
    UINT stride=sizeof(struct jnet_flower_vertex),offset=0;
    context->lpVtbl->IASetVertexBuffers(context,0,1,&jnet_flower_gpu.vertices,&stride,&offset);
    context->lpVtbl->IASetIndexBuffer(context,jnet_flower_gpu.indices,DXGI_FORMAT_R16_UINT,0);
    context->lpVtbl->IASetInputLayout(context,jnet_flower_gpu.layout);
    context->lpVtbl->IASetPrimitiveTopology(context,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->lpVtbl->VSSetShader(context,jnet_flower_gpu.vs,NULL,0);context->lpVtbl->PSSetShader(context,jnet_flower_gpu.ps,NULL,0);
    context->lpVtbl->VSSetConstantBuffers(context,0,1,&jnet_flower_gpu.constants);
    context->lpVtbl->PSSetConstantBuffers(context,0,1,&jnet_flower_gpu.constants);
    ID3D11ShaderResourceView *textures[4]={jnet_flower_gpu.atlas,jnet_flower_gpu.grass,(scene&1)?jnet_flower_scene.color_view:NULL,(scene&2)?jnet_flower_scene.depth_view:NULL};
    context->lpVtbl->PSSetShaderResources(context,0,4,textures);
    context->lpVtbl->PSSetSamplers(context,0,1,&jnet_flower_gpu.sampler);
    for(unsigned n=0;n<visible;++n){
        const struct jnet_flower_item *item=&snapshot.items[order[n].index];
        const struct jnet_flower_model *model=&jnet_flower_models[item->type];
        struct jnet_flower_constants constants;float world[16];
        struct jnet_flower_look look=jnet_flower_animate(item,now32);
        struct jnet_flower_item shown=*item;shown.scale*=look.scale;
        jnet_flower_model_matrix(&shown,world);jnet_flower_multiply(world,view_projection,constants.matrix);
        constants.turn[0]=cosf(item->yaw);constants.turn[1]=sinf(item->yaw);constants.turn[2]=order[n].fade;constants.turn[3]=clock_seconds;
        constants.anim[0]=look.bloom;constants.anim[1]=look.glow;constants.anim[2]=constants.anim[3]=0;
        float uv[2]={.5f,.5f};
        int lit=(scene&1) && jnet_flower_screen_uv(view_projection,item->position,uv);
        constants.light[0]=uv[0];constants.light[1]=uv[1];constants.light[2]=lit?1.f:0.f;constants.light[3]=(scene&2)?1.f:0.f;
        constants.depth[0]=camera.near_plane;constants.depth[1]=camera.far_plane;constants.depth[2]=(float)jnet_flower_scene.reversed;constants.depth[3]=.12f;
        context->lpVtbl->UpdateSubresource(context,(ID3D11Resource *)jnet_flower_gpu.constants,0,NULL,&constants,0,0);
        context->lpVtbl->DrawIndexed(context,model->index_count,model->first_index,(INT)model->first_vertex);
    }
    context->lpVtbl->OMSetRenderTargets(context,0,NULL,NULL);
    jnet_image_restore(context,&saved);
    jnet_image_saved_release(&saved);
    jnet_flower_scene.busy=0;
    InterlockedIncrement(&jnet_flower_frames);
done:
    JNET_RELEASE(target);JNET_RELEASE(back);JNET_RELEASE(device);
}
static void jnet_flower_draw(uintptr_t renderer) {
    jnet_flower_draw_pass(renderer);
    /* Depth buffers noted this frame are only for this frame, drawn or not: release the references. */
    jnet_flower_scene.busy=0;jnet_flower_scene_forget();
}
#endif
