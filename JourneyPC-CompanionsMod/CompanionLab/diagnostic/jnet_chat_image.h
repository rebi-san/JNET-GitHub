#ifndef JNET_CHAT_IMAGE_H
#define JNET_CHAT_IMAGE_H
#include "jnet_chat_font.h"
static ID3D11Texture2D *jnet_chat_texture;
static ID3D11ShaderResourceView *jnet_chat_resource;
static char jnet_chat_drawn[1024];
static float jnet_chat_opacity;
static void jnet_chat_image_release(void) {
    if(jnet_chat_resource){jnet_chat_resource->lpVtbl->Release(jnet_chat_resource);jnet_chat_resource=NULL;}
    if(jnet_chat_texture){jnet_chat_texture->lpVtbl->Release(jnet_chat_texture);jnet_chat_texture=NULL;}
    jnet_chat_drawn[0]=0;
}
static int jnet_chat_image_text(char text[1024]) {
    text[0]=0;if(!jnet_chat_gameplay)return 0;
    uint64_t now=GetTickCount64();
    AcquireSRWLockExclusive(&jnet_chat_lock);
    jnet_chat_expire(now);
    uint64_t age=now-jnet_chat.last_activity;
    jnet_chat_opacity=jnet_chat_focus?1.f:fminf(1.f,(float)(now-jnet_chat.shown)/200.f)*fmaxf(0.f,1.f-(age>10000?(float)(age-10000)/400.f:0.f));
    for(unsigned i=2;i<5;++i)if(jnet_chat.lines[i][0] && (jnet_chat_focus || age<10400)){
        strncat(text,jnet_chat.lines[i],1023-strlen(text));strncat(text,"\n",1023-strlen(text));
    }
    if(jnet_chat_focus){strncat(text,"> ",1023-strlen(text));strncat(text,jnet_chat.input,1023-strlen(text));strncat(text,"_",1023-strlen(text));}
    ReleaseSRWLockExclusive(&jnet_chat_lock);return text[0]!=0;
}
static int jnet_chat_image_update(ID3D11Device *device,ID3D11DeviceContext *context,const char *text) {
    if(jnet_chat_texture && !strcmp(text,jnet_chat_drawn))return 1;
    static unsigned char font[JNET_CHAT_FONT_BYTES];static int ready;
    if(!ready){size_t at=0;for(size_t i=0;i<sizeof(jnet_chat_font_rle);i+=2){unsigned n=jnet_chat_font_rle[i];if(at+n>sizeof(font))return 0;memset(font+at,jnet_chat_font_rle[i+1],n);at+=n;}if(at!=sizeof(font))return 0;ready=1;}
    unsigned char *pixels=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,1024*384*4);if(!pixels)return 0;
    int x=8,rows=1;
    for(const unsigned char *p=(const unsigned char *)text;*p;++p){
        if(*p=='\n'){if(p[1])++rows;x=8;continue;}
        if(*p<32 || *p>126)continue;
        int advance=(jnet_chat_glyphs[*p-32].advance*40+50)/100;
        if(x+advance>1016){++rows;x=8;}
        x+=advance;
    }
    x=8;int y=rows<8?376-rows*48:4;
    for(const unsigned char *p=(const unsigned char *)text;*p;++p){
        if(*p=='\n'){x=8;y+=48;continue;}
        if(*p<32 || *p>126)continue;
        const struct jnet_chat_glyph *g=&jnet_chat_glyphs[*p-32];
        int advance=(g->advance*40+50)/100;
        if(x+advance>1016){x=8;y+=48;}if(y+48>=384)break;
        for(unsigned pass=0;pass<2;++pass)for(int dy=0;dy<(g->h*40+99)/100;++dy)for(int dx=0;dx<(g->w*40+99)/100;++dx){
            int sx=dx*100/40,sy=dy*100/40;if(sx>=g->w || sy>=g->h)continue;
            unsigned a=font[g->offset+sy*g->w+sx];
            int px=x+g->x*40/100+dx+(pass?0:2),py=y+g->y*40/100+dy+(pass?0:2);
            if(px<0 || px>=1024 || py<0 || py>=384)continue;
            unsigned char *q=pixels+(py*1024+px)*4;
            if(pass){unsigned old=q[3],combined=a+old*(255-a)/255;q[0]=q[1]=q[2]=combined?(unsigned char)(a*255/combined):0;q[3]=(unsigned char)combined;}
            else if(a>q[3])q[3]=(unsigned char)a;
        }
        x+=advance;
    }
    HRESULT result=S_OK;
    if(!jnet_chat_texture){
        D3D11_TEXTURE2D_DESC td={.Width=1024,.Height=384,.MipLevels=1,.ArraySize=1,.Format=DXGI_FORMAT_R8G8B8A8_UNORM,.SampleDesc={1,0},.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_SHADER_RESOURCE};
        D3D11_SUBRESOURCE_DATA data={pixels,1024*4,0};
        result=device->lpVtbl->CreateTexture2D(device,&td,&data,&jnet_chat_texture);
        if(SUCCEEDED(result))result=device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)jnet_chat_texture,NULL,&jnet_chat_resource);
        if(FAILED(result))jnet_chat_image_release();
    }else context->lpVtbl->UpdateSubresource(context,(ID3D11Resource *)jnet_chat_texture,0,NULL,pixels,1024*4,0);
    HeapFree(GetProcessHeap(),0,pixels);
    if(FAILED(result))return 0;snprintf(jnet_chat_drawn,sizeof(jnet_chat_drawn),"%s",text);return 1;
}
#endif
