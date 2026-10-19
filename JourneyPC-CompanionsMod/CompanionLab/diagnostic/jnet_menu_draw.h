#ifndef JNET_MENU_DRAW_H
#define JNET_MENU_DRAW_H
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

static inline int jnet_menu_image_fit(float out[4],const float box[4],
        float source_width,float source_height,unsigned width,unsigned height) {
    if(!out || !box || !width || !height || !isfinite(source_width) || !isfinite(source_height) ||
       source_width<=0 || source_height<=0)return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(box[i]))return 0;
    float w=box[2]-box[0],h=box[1]-box[3];
    if(w<=0 || h<=0)return 0;
    float pixels_per_source=fminf(w*width/source_width,h*height/source_height);
    float fitted_w=source_width*pixels_per_source/width,fitted_h=source_height*pixels_per_source/height;
    float x=(box[0]+box[2])*.5f,y=(box[1]+box[3])*.5f;
    out[0]=x-fitted_w*.5f;out[1]=y+fitted_h*.5f;
    out[2]=x+fitted_w*.5f;out[3]=y-fitted_h*.5f;
    return 1;
}

struct jnet_menu_quad {
    float position[4][4];
    float uv[2][4];
    uint32_t color;
};
_Static_assert(sizeof(struct jnet_menu_quad)==0x64,"Native menu quad size");
_Static_assert(offsetof(struct jnet_menu_quad,uv)==0x40,"Native menu UV offset");
_Static_assert(offsetof(struct jnet_menu_quad,color)==0x60,"Native menu color offset");

static inline void jnet_menu_append_aligned(
        void (__attribute__((ms_abi)) *append)(uintptr_t,const struct jnet_menu_quad *),
        uintptr_t batch,const struct jnet_menu_quad *quad) {
    _Alignas(16) struct jnet_menu_quad aligned=*quad;
    append(batch,&aligned);
}

static inline int jnet_menu_quad_make(struct jnet_menu_quad *out,float left,float top,
                                      float right,float bottom,float u0,float v0,float u1,float v1,
                                      uint32_t color) {
    const float values[]={left,top,right,bottom,u0,v0,u1,v1};
    if(!out)return 0;
    for(unsigned i=0;i<8;++i)if(!isfinite(values[i]))return 0;
    if(left>=right || top>=bottom || u0<0 || v0<0 || u1>1 || v1>1 || u0>=u1 || v0>=v1)return 0;
    struct jnet_menu_quad quad={0};
    quad.position[0][0]=left;quad.position[0][1]=top;
    quad.position[1][0]=right;quad.position[1][1]=top;
    quad.position[2][0]=right;quad.position[2][1]=bottom;
    quad.position[3][0]=left;quad.position[3][1]=bottom;
    quad.uv[0][0]=u0;quad.uv[0][1]=v0;quad.uv[0][2]=1;
    quad.uv[1][0]=u1;quad.uv[1][1]=v1;quad.uv[1][2]=1;
    quad.color=color;*out=quad;return 1;
}
static inline int jnet_menu_batch_room(uint32_t capacity,uint32_t used,const void *mapped,
                                       size_t mapped_bytes,uint32_t width,uint32_t height,uint32_t count) {
    if(!mapped || !width || !height || !count || capacity>UINT32_MAX/4 || used>capacity || count>capacity-used)
        return 0;
    return (uint64_t)(used+count)*4*0x1c<=mapped_bytes;
}

struct jnet_menu_glyph {
    float u0,v0,u1,v1;
    float xoffset,yoffset,width,height,advance;
};
_Static_assert(sizeof(struct jnet_menu_glyph)==0x24,"Native menu glyph size");
_Static_assert(offsetof(struct jnet_menu_glyph,advance)==0x20,"Native menu glyph advance");

static inline int jnet_menu_glyph_quad(struct jnet_menu_quad *out,
                                      const struct jnet_menu_glyph *glyph,
                                      float x,float y,float scale,uint32_t color,float *next_x) {
    if(!out || !glyph || !next_x || !isfinite(x) || !isfinite(y) ||
       !isfinite(scale) || scale<=0)return 0;
    const float values[]={glyph->u0,glyph->v0,glyph->u1,glyph->v1,glyph->xoffset,
                          glyph->yoffset,glyph->width,glyph->height,glyph->advance};
    for(unsigned i=0;i<9;++i)if(!isfinite(values[i]))return 0;
    if(glyph->width<0 || glyph->height<0 || glyph->advance<0 ||
       glyph->u0<0 || glyph->v0<0 || glyph->u1<0 || glyph->v1<0 ||
       glyph->u0>1 || glyph->v0>1 || glyph->u1>1 || glyph->v1>1)return 0;
    float left=x+scale*glyph->xoffset,top=y+scale*glyph->yoffset;
    float right=left+scale*glyph->width,bottom=top+scale*glyph->height;
    float advance=x+scale*glyph->advance;
    if(!isfinite(left) || !isfinite(top) || !isfinite(right) ||
       !isfinite(bottom) || !isfinite(advance))return 0;
    struct jnet_menu_quad quad={0};
    quad.position[0][0]=left;quad.position[0][1]=top;
    quad.position[1][0]=right;quad.position[1][1]=top;
    quad.position[2][0]=right;quad.position[2][1]=bottom;
    quad.position[3][0]=left;quad.position[3][1]=bottom;
    for(unsigned i=0;i<4;++i)quad.position[i][2]=0.9999f;
    quad.uv[0][0]=glyph->u0;quad.uv[0][1]=glyph->v0;
    quad.uv[1][0]=glyph->u1;quad.uv[1][1]=glyph->v1;
    quad.color=color;*out=quad;*next_x=advance;return 1;
}

#define JNET_MENU_TEXT_LIMIT 768u
struct jnet_menu_text {
    struct jnet_menu_quad quads[JNET_MENU_TEXT_LIMIT];
    uint32_t count;
    float next_x;
};
static inline int jnet_menu_text_layout(struct jnet_menu_text *out,
                                        const struct jnet_menu_glyph *glyphs,size_t count,
                                        float x,float y,float scale,uint32_t color) {
    if(!out || (count && !glyphs) || count>JNET_MENU_TEXT_LIMIT ||
       !isfinite(x) || !isfinite(y) || !isfinite(scale) || scale<=0)return 0;
    struct jnet_menu_text text={0};
    for(size_t i=0;i<count;++i) {
        if(!jnet_menu_glyph_quad(&text.quads[i],&glyphs[i],x,y,scale,color,&x))return 0;
    }
    text.count=(uint32_t)count;text.next_x=x;*out=text;return 1;
}
#endif
