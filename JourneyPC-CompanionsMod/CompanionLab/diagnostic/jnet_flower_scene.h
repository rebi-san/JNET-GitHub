#ifndef JNET_FLOWER_SCENE_H
#define JNET_FLOWER_SCENE_H
/* Lets the Flower Trail pass see the game's own frame, so flowers take on the light around them and are hidden by anything in front.
   Included by jnet_flower_render.h; everything here fails soft (the flowers still draw, only that one feature is off).
   Colour: the multisampled back buffer is resolved to a plain texture before the flowers draw; the pixel shader samples it around each
   flower's base to pick up the scene's brightness and tint (day, night, fog, caves, shade).
   Depth: OMSetRenderTargets is hooked to remember which depth buffers the game binds during a frame. At Present the one that matches the
   back buffer (size and sample count, most often bound) is copied to a texture the pixel shader can read and compared per pixel.
   Because that buffer's contents at Present are not guaranteed, a probe pass samples sixteen points once a second and the depth is
   used only while the values look like a depth buffer; the probe also tells standard from reversed depth. */

struct jnet_flower_scene_state {
    ID3D11DeviceContext *context;
    void (STDMETHODCALLTYPE *original)(ID3D11DeviceContext *,UINT,ID3D11RenderTargetView *const *,ID3D11DepthStencilView *);
    ID3D11DepthStencilView *seen[8];
    unsigned binds[8],seen_count,hooked,busy;
    ID3D11Texture2D *color;ID3D11ShaderResourceView *color_view;UINT color_width,color_height;DXGI_FORMAT color_format;
    ID3D11Texture2D *depth;ID3D11ShaderResourceView *depth_view;UINT depth_width,depth_height,depth_samples;DXGI_FORMAT depth_format,depth_view_format;
    ID3D11VertexShader *probe_vs;ID3D11PixelShader *probe_ps;ID3D11Buffer *probe_constants;
    ID3D11Texture2D *probe_target,*probe_staging;ID3D11RenderTargetView *probe_rtv;
    unsigned usable,reversed,logged,candidates_logged;
    ULONGLONG probe_next;
};
static struct jnet_flower_scene_state jnet_flower_scene;

static void jnet_flower_scene_forget(void) {
    for(unsigned i=0;i<jnet_flower_scene.seen_count;++i)JNET_RELEASE(jnet_flower_scene.seen[i]);
    jnet_flower_scene.seen_count=0;memset(jnet_flower_scene.binds,0,sizeof(jnet_flower_scene.binds));
}
/* The render thread's depth-stencil bindings, noted while the game draws its frame. */
static void STDMETHODCALLTYPE jnet_flower_scene_bind(ID3D11DeviceContext *context,UINT count,ID3D11RenderTargetView *const *targets,
        ID3D11DepthStencilView *depth) {
    if(context==jnet_flower_scene.context && depth && !jnet_flower_scene.busy){
        unsigned at=0;
        while(at<jnet_flower_scene.seen_count && jnet_flower_scene.seen[at]!=depth)++at;
        if(at<jnet_flower_scene.seen_count)++jnet_flower_scene.binds[at];
        else if(at<8){depth->lpVtbl->AddRef(depth);jnet_flower_scene.seen[at]=depth;jnet_flower_scene.binds[at]=1;++jnet_flower_scene.seen_count;}
    }
    jnet_flower_scene.original(context,count,targets,depth);
}
static void jnet_flower_scene_hook(ID3D11DeviceContext *context) {
    jnet_flower_scene.context=context;
    if(jnet_flower_scene.hooked)return;
    jnet_flower_scene.hooked=2;
    void *target=(void *)context->lpVtbl->OMSetRenderTargets;
    if(MH_CreateHook(target,(void *)jnet_flower_scene_bind,(void **)&jnet_flower_scene.original)==MH_OK && MH_EnableHook(target)==MH_OK){
        jnet_flower_scene.hooked=1;message("[JNET] flower trail scene depth hook installed");
    } else message("[JNET] flower trail scene depth hook unavailable; flowers use terrain line of sight only");
}
static void jnet_flower_scene_release(void) {
    jnet_flower_scene_forget();
    JNET_RELEASE(jnet_flower_scene.color_view);JNET_RELEASE(jnet_flower_scene.color);
    JNET_RELEASE(jnet_flower_scene.depth_view);JNET_RELEASE(jnet_flower_scene.depth);
    JNET_RELEASE(jnet_flower_scene.probe_vs);JNET_RELEASE(jnet_flower_scene.probe_ps);JNET_RELEASE(jnet_flower_scene.probe_constants);
    JNET_RELEASE(jnet_flower_scene.probe_rtv);JNET_RELEASE(jnet_flower_scene.probe_target);JNET_RELEASE(jnet_flower_scene.probe_staging);
    jnet_flower_scene.color_width=jnet_flower_scene.color_height=jnet_flower_scene.depth_width=jnet_flower_scene.depth_height=0;
    jnet_flower_scene.usable=0;
}
/* The probe pass: one pixel of a 16x1 float target per probe point, each reading the copied depth at that point. */
static int jnet_flower_scene_initialize(ID3D11Device *device,
        HRESULT (WINAPI *compile)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO *,ID3DInclude *,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob **,ID3DBlob **)) {
    static const char vertex_source[]=
        "float4 main(uint id:SV_VertexID):SV_Position{float2 p=float2((id<<1)&2,id&2);return float4(p*float2(2,-2)+float2(-1,1),0,1);}\n";
    static const char pixel_source[]=
        "cbuffer P:register(b0){float4 xy[16];}\n"
        "Texture2DMS<float> sceneDepth:register(t3);\n"
        "float main(float4 p:SV_Position):SV_Target{return sceneDepth.Load(int2(xy[(uint)p.x].xy),0).r;}\n";
    ID3DBlob *vs=NULL,*ps=NULL,*errors=NULL;int ok=0;
    if(!compile)return 0;
    if(FAILED(compile(vertex_source,sizeof(vertex_source)-1,"Flower probe vertex",NULL,NULL,"main","vs_4_0",0,0,&vs,&errors)))goto done;
    JNET_RELEASE(errors);
    if(FAILED(compile(pixel_source,sizeof(pixel_source)-1,"Flower probe pixel",NULL,NULL,"main","ps_4_1",0,0,&ps,&errors)))goto done;
    if(FAILED(device->lpVtbl->CreateVertexShader(device,vs->lpVtbl->GetBufferPointer(vs),vs->lpVtbl->GetBufferSize(vs),NULL,&jnet_flower_scene.probe_vs)) ||
       FAILED(device->lpVtbl->CreatePixelShader(device,ps->lpVtbl->GetBufferPointer(ps),ps->lpVtbl->GetBufferSize(ps),NULL,&jnet_flower_scene.probe_ps)))goto done;
    D3D11_BUFFER_DESC constants={.ByteWidth=16*16,.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_CONSTANT_BUFFER};
    D3D11_TEXTURE2D_DESC target={.Width=JNET_FLOWER_PROBES,.Height=1,.MipLevels=1,.ArraySize=1,.Format=DXGI_FORMAT_R32_FLOAT,.SampleDesc={1,0},
        .Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_RENDER_TARGET};
    D3D11_TEXTURE2D_DESC staging=target;staging.Usage=D3D11_USAGE_STAGING;staging.BindFlags=0;staging.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    if(FAILED(device->lpVtbl->CreateBuffer(device,&constants,NULL,&jnet_flower_scene.probe_constants)) ||
       FAILED(device->lpVtbl->CreateTexture2D(device,&target,NULL,&jnet_flower_scene.probe_target)) ||
       FAILED(device->lpVtbl->CreateTexture2D(device,&staging,NULL,&jnet_flower_scene.probe_staging)) ||
       FAILED(device->lpVtbl->CreateRenderTargetView(device,(ID3D11Resource *)jnet_flower_scene.probe_target,NULL,&jnet_flower_scene.probe_rtv)))goto done;
    ok=1;
done:
    JNET_RELEASE(vs);JNET_RELEASE(ps);
    if(!ok && errors){
        const char *text=errors->lpVtbl->GetBufferPointer(errors);char line[200];
        snprintf(line,sizeof(line),"[JNET] flower trail depth probe shader: %.150s",text?text:"error");message(line);
    }
    JNET_RELEASE(errors);
    if(!ok){
        JNET_RELEASE(jnet_flower_scene.probe_vs);JNET_RELEASE(jnet_flower_scene.probe_ps);JNET_RELEASE(jnet_flower_scene.probe_constants);
        JNET_RELEASE(jnet_flower_scene.probe_rtv);JNET_RELEASE(jnet_flower_scene.probe_target);JNET_RELEASE(jnet_flower_scene.probe_staging);
    }
    return ok;
}
/* Typeless family and shader-view format for a depth format, 0 when it is not a depth format. */
static int jnet_flower_depth_formats(DXGI_FORMAT format,DXGI_FORMAT *typeless,DXGI_FORMAT *view) {
    switch(format){
    case DXGI_FORMAT_R32_TYPELESS:case DXGI_FORMAT_D32_FLOAT:*typeless=DXGI_FORMAT_R32_TYPELESS;*view=DXGI_FORMAT_R32_FLOAT;return 1;
    case DXGI_FORMAT_R24G8_TYPELESS:case DXGI_FORMAT_D24_UNORM_S8_UINT:*typeless=DXGI_FORMAT_R24G8_TYPELESS;*view=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;return 1;
    case DXGI_FORMAT_R32G8X24_TYPELESS:case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:*typeless=DXGI_FORMAT_R32G8X24_TYPELESS;*view=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;return 1;
    case DXGI_FORMAT_R16_TYPELESS:case DXGI_FORMAT_D16_UNORM:*typeless=DXGI_FORMAT_R16_TYPELESS;*view=DXGI_FORMAT_R16_UNORM;return 1;
    default:return 0;
    }
}
static void jnet_flower_scene_color(ID3D11Device *device,ID3D11DeviceContext *context,ID3D11Texture2D *back,const D3D11_TEXTURE2D_DESC *desc,unsigned *flags) {
    if(jnet_flower_scene.color && (jnet_flower_scene.color_width!=desc->Width || jnet_flower_scene.color_height!=desc->Height ||
                                   jnet_flower_scene.color_format!=desc->Format)){
        JNET_RELEASE(jnet_flower_scene.color_view);JNET_RELEASE(jnet_flower_scene.color);
    }
    if(!jnet_flower_scene.color){
        D3D11_TEXTURE2D_DESC copy={.Width=desc->Width,.Height=desc->Height,.MipLevels=1,.ArraySize=1,.Format=desc->Format,.SampleDesc={1,0},
            .Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_SHADER_RESOURCE};
        if(FAILED(device->lpVtbl->CreateTexture2D(device,&copy,NULL,&jnet_flower_scene.color)) ||
           FAILED(device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)jnet_flower_scene.color,NULL,&jnet_flower_scene.color_view))){
            JNET_RELEASE(jnet_flower_scene.color_view);JNET_RELEASE(jnet_flower_scene.color);return;
        }
        jnet_flower_scene.color_width=desc->Width;jnet_flower_scene.color_height=desc->Height;jnet_flower_scene.color_format=desc->Format;
    }
    if(desc->SampleDesc.Count>1)context->lpVtbl->ResolveSubresource(context,(ID3D11Resource *)jnet_flower_scene.color,0,(ID3D11Resource *)back,0,desc->Format);
    else context->lpVtbl->CopyResource(context,(ID3D11Resource *)jnet_flower_scene.color,(ID3D11Resource *)back);
    *flags|=1;
}
static void jnet_flower_scene_depth(ID3D11Device *device,ID3D11DeviceContext *context,const D3D11_TEXTURE2D_DESC *desc,unsigned *flags) {
    static const GUID texture_id={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
    ID3D11Texture2D *best=NULL;unsigned best_binds=0;D3D11_TEXTURE2D_DESC best_desc={0};
    for(unsigned i=0;i<jnet_flower_scene.seen_count;++i){
        ID3D11Resource *resource=NULL;ID3D11Texture2D *texture=NULL;D3D11_TEXTURE2D_DESC td;
        jnet_flower_scene.seen[i]->lpVtbl->GetResource(jnet_flower_scene.seen[i],&resource);
        if(!resource)continue;
        if(SUCCEEDED(resource->lpVtbl->QueryInterface(resource,&texture_id,(void **)&texture)) && texture){
            texture->lpVtbl->GetDesc(texture,&td);
            DXGI_FORMAT typeless,view;
            if(td.Width==desc->Width && td.Height==desc->Height && td.SampleDesc.Count==desc->SampleDesc.Count && td.ArraySize==1 &&
               td.MipLevels==1 && jnet_flower_depth_formats(td.Format,&typeless,&view) && jnet_flower_scene.binds[i]>best_binds){
                JNET_RELEASE(best);texture->lpVtbl->AddRef(texture);best=texture;best_binds=jnet_flower_scene.binds[i];best_desc=td;
            }
            texture->lpVtbl->Release(texture);
        }
        resource->lpVtbl->Release(resource);
    }
    if(jnet_flower_scene.candidates_logged<3){
        ++jnet_flower_scene.candidates_logged;char line[200];
        snprintf(line,sizeof(line),"[JNET] flower trail scene depth: %u buffers bound this frame, %s (%ux%u format %u samples %u binds %u)",
            jnet_flower_scene.seen_count,best?"using the largest match":"none matches the back buffer",
            best?best_desc.Width:0,best?best_desc.Height:0,best?(unsigned)best_desc.Format:0,best?best_desc.SampleDesc.Count:0,best_binds);
        message(line);
    }
    if(!best)return;
    DXGI_FORMAT typeless,view;jnet_flower_depth_formats(best_desc.Format,&typeless,&view);
    if(jnet_flower_scene.depth && (jnet_flower_scene.depth_format!=typeless || jnet_flower_scene.depth_width!=best_desc.Width ||
                                   jnet_flower_scene.depth_height!=best_desc.Height || jnet_flower_scene.depth_samples!=best_desc.SampleDesc.Count)){
        JNET_RELEASE(jnet_flower_scene.depth_view);JNET_RELEASE(jnet_flower_scene.depth);
    }
    if(!jnet_flower_scene.depth){
        D3D11_TEXTURE2D_DESC copy=best_desc;copy.Format=typeless;copy.Usage=D3D11_USAGE_DEFAULT;copy.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        copy.CPUAccessFlags=0;copy.MiscFlags=0;
        D3D11_SHADER_RESOURCE_VIEW_DESC srv={.Format=view,.ViewDimension=best_desc.SampleDesc.Count>1?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D};
        if(srv.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D){srv.Texture2D.MipLevels=1;}
        if(FAILED(device->lpVtbl->CreateTexture2D(device,&copy,NULL,&jnet_flower_scene.depth)) ||
           FAILED(device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)jnet_flower_scene.depth,&srv,&jnet_flower_scene.depth_view))){
            JNET_RELEASE(jnet_flower_scene.depth_view);JNET_RELEASE(jnet_flower_scene.depth);
            if(jnet_flower_scene.logged<3){++jnet_flower_scene.logged;message("[JNET] flower trail scene depth copy could not be created");}
            JNET_RELEASE(best);return;
        }
        jnet_flower_scene.depth_format=typeless;jnet_flower_scene.depth_view_format=view;
        jnet_flower_scene.depth_width=best_desc.Width;jnet_flower_scene.depth_height=best_desc.Height;jnet_flower_scene.depth_samples=best_desc.SampleDesc.Count;
    }
    context->lpVtbl->CopyResource(context,(ID3D11Resource *)jnet_flower_scene.depth,(ID3D11Resource *)best);
    JNET_RELEASE(best);
    *flags|=2;
}
/* Once a second, reads sixteen depth values back and decides whether the copy can be believed. Runs inside the caller's saved state. */
static void jnet_flower_scene_probe(ID3D11DeviceContext *context,const D3D11_TEXTURE2D_DESC *desc,ULONGLONG now) {
    if(now<jnet_flower_scene.probe_next || !jnet_flower_scene.probe_rtv || !jnet_flower_scene.depth_view)return;
    jnet_flower_scene.probe_next=now+1000;
    float positions[JNET_FLOWER_PROBES][4]={{0}};
    for(unsigned i=0;i<JNET_FLOWER_PROBES;++i){
        unsigned column=i%8;
        positions[i][0]=((float)column+.5f)/8.f*(float)desc->Width;
        positions[i][1]=(i<8?.08f:.88f)*(float)desc->Height;
    }
    jnet_flower_scene.busy=1;
    context->lpVtbl->UpdateSubresource(context,(ID3D11Resource *)jnet_flower_scene.probe_constants,0,NULL,positions,0,0);
    ID3D11ShaderResourceView *views[4]={NULL,NULL,NULL,jnet_flower_scene.depth_view};
    D3D11_VIEWPORT viewport={0,0,(float)JNET_FLOWER_PROBES,1,0,1};
    context->lpVtbl->OMSetRenderTargets(context,1,&jnet_flower_scene.probe_rtv,NULL);
    context->lpVtbl->RSSetViewports(context,1,&viewport);
    context->lpVtbl->IASetPrimitiveTopology(context,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->lpVtbl->IASetInputLayout(context,NULL);
    context->lpVtbl->VSSetShader(context,jnet_flower_scene.probe_vs,NULL,0);context->lpVtbl->PSSetShader(context,jnet_flower_scene.probe_ps,NULL,0);
    context->lpVtbl->PSSetConstantBuffers(context,0,1,&jnet_flower_scene.probe_constants);
    context->lpVtbl->PSSetShaderResources(context,0,4,views);
    context->lpVtbl->Draw(context,3,0);
    context->lpVtbl->CopyResource(context,(ID3D11Resource *)jnet_flower_scene.probe_staging,(ID3D11Resource *)jnet_flower_scene.probe_target);
    ID3D11ShaderResourceView *none[4]={NULL,NULL,NULL,NULL};
    context->lpVtbl->PSSetShaderResources(context,0,4,none);
    context->lpVtbl->OMSetRenderTargets(context,0,NULL,NULL);
    jnet_flower_scene.busy=0;
    D3D11_MAPPED_SUBRESOURCE mapped;float values[JNET_FLOWER_PROBES];
    if(FAILED(context->lpVtbl->Map(context,(ID3D11Resource *)jnet_flower_scene.probe_staging,0,D3D11_MAP_READ,0,&mapped)))return;
    memcpy(values,mapped.pData,sizeof(values));
    context->lpVtbl->Unmap(context,(ID3D11Resource *)jnet_flower_scene.probe_staging,0);
    unsigned reversed=jnet_flower_scene.reversed;
    unsigned usable=(unsigned)jnet_flower_depth_assess(values,&reversed);
    if((usable!=jnet_flower_scene.usable || reversed!=jnet_flower_scene.reversed || jnet_flower_scene.logged<3) && jnet_flower_scene.logged<12){
        ++jnet_flower_scene.logged;char line[300];
        snprintf(line,sizeof(line),"[JNET] flower trail scene depth probe: %s, %s depth; top %.5f %.5f %.5f %.5f, bottom %.5f %.5f %.5f %.5f",
            usable?"usable":"not usable",reversed?"reversed":"standard",(double)values[0],(double)values[2],(double)values[4],(double)values[6],
            (double)values[8],(double)values[10],(double)values[12],(double)values[14]);
        message(line);
    }
    jnet_flower_scene.usable=usable;jnet_flower_scene.reversed=reversed;
}
/* Per frame, before the flowers draw. Returns bit 0 when the scene colour is available and bit 1 when a believed depth buffer is. */
static unsigned jnet_flower_scene_prepare(ID3D11Device *device,ID3D11DeviceContext *context,ID3D11Texture2D *back,const D3D11_TEXTURE2D_DESC *desc,ULONGLONG now) {
    unsigned flags=0;
    jnet_flower_scene_hook(context);
    jnet_flower_scene_color(device,context,back,desc,&flags);
    unsigned depth_flags=0;
    jnet_flower_scene_depth(device,context,desc,&depth_flags);
    jnet_flower_scene_forget();
    if(depth_flags&2){jnet_flower_scene_probe(context,desc,now);if(jnet_flower_scene.usable)flags|=2;}
    return flags;
}
#endif
