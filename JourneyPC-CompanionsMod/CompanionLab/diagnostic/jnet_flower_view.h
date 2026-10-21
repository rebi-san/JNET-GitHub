#ifndef JNET_FLOWER_VIEW_H
#define JNET_FLOWER_VIEW_H
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "jnet_flower_trail.h"

/* Camera maths for drawing the trail flowers with a private D3D11 pass. Journey's camera node keeps its world matrix as
   right, up, back and position rows; the camera looks along -back, with `right` and `up` as screen directions whatever the handedness
   of the world. Matrices are row-major and multiply row vectors
   (HLSL `mul(float4(p,1),m)` with `row_major`), so a point's clip position is p * model * view * projection. */
#define JNET_FLOWER_DRAW_DISTANCE 48.f
#define JNET_FLOWER_FADE_DISTANCE 32.f
#define JNET_FLOWER_TERRAIN_STEPS 8u

struct jnet_flower_camera {
    float right[3],up[3],back[3],position[3];
    float fov,aspect,near_plane,far_plane;
};
/* Decimal text as Lua prints a number ("1.5", "-3", "6.5e-05") to a finite float. The CRT's strtod would add a new DLL import
   (api-ms-win-crt-convert), which the package audit forbids. Returns 0 for anything else, including "nan" and "inf". */
static inline int jnet_flower_number(const char *text,float *out) {
    if(!text || !out)return 0;
    double sign=1,value=0,scale=1;int digits=0,exponent=0,exponent_sign=1;
    if(*text=='-'){sign=-1;++text;}else if(*text=='+')++text;
    for(;*text>='0' && *text<='9';++text,++digits)value=value*10+(*text-'0');
    if(*text=='.'){
        for(++text;*text>='0' && *text<='9';++text,++digits){value=value*10+(*text-'0');scale*=10;}
    }
    if(!digits)return 0;
    if(*text=='e' || *text=='E'){
        ++text;
        if(*text=='-'){exponent_sign=-1;++text;}else if(*text=='+')++text;
        if(*text<'0' || *text>'9')return 0;
        for(;*text>='0' && *text<='9';++text)if(exponent<1000)exponent=exponent*10+(*text-'0');
    }
    if(*text)return 0;
    value=sign*value/scale*pow(10.0,exponent_sign*exponent);
    if(!isfinite(value) || fabs(value)>3e38)return 0;
    *out=(float)value;return 1;
}
/* Finds three consecutive floats in a memory copy that equal `target` within `tolerance`, scanning on 4-byte steps. Used once to locate
   the camera position in the camera object by matching what Lua's GetWorldMatrix reports. A target near the origin is refused: zeros
   match any cleared memory. */
static inline int jnet_flower_find_vector(const unsigned char *memory,size_t size,const float target[3],float tolerance,size_t *offset) {
    if(!memory || !offset || size<12 || !isfinite(target[0]) || !isfinite(target[1]) || !isfinite(target[2]))return 0;
    if(fabsf(target[0])+fabsf(target[1])+fabsf(target[2])<1.f)return 0;
    for(size_t at=0;at+12<=size;at+=4){
        float value[3];memcpy(value,memory+at,12);
        if(fabsf(value[0]-target[0])<=tolerance && fabsf(value[1]-target[1])<=tolerance && fabsf(value[2]-target[2])<=tolerance){*offset=at;return 1;}
    }
    return 0;
}
static inline float jnet_flower_dot(const float a[3],const float b[3]) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
/* Reads the four 16-byte rows the camera node holds at +0x170. Rejects a basis that is not orthonormal or whose position is not
   near the player, so a wrong guess about the node layout draws nothing instead of garbage. */
static inline int jnet_flower_camera_rows(struct jnet_flower_camera *camera,const float rows[16],const float player[3]) {
    for(unsigned i=0;i<16;++i)if(!isfinite(rows[i]))return 0;
    const float *right=rows,*up=rows+4,*back=rows+8,*position=rows+12;
    if(fabsf(jnet_flower_dot(right,right)-1)>.02f || fabsf(jnet_flower_dot(up,up)-1)>.02f || fabsf(jnet_flower_dot(back,back)-1)>.02f ||
       fabsf(jnet_flower_dot(right,up))>.02f || fabsf(jnet_flower_dot(right,back))>.02f || fabsf(jnet_flower_dot(up,back))>.02f)return 0;
    if(player){
        float dx=position[0]-player[0],dy=position[1]-player[1],dz=position[2]-player[2];
        if(dx*dx+dy*dy+dz*dz>120.f*120.f)return 0;
    }
    memcpy(camera->right,right,12);memcpy(camera->up,up,12);memcpy(camera->back,back,12);memcpy(camera->position,position,12);
    return 1;
}
static inline int jnet_flower_camera_lens(struct jnet_flower_camera *camera,float fov,float aspect,float near_plane,float far_plane) {
    if(!isfinite(fov) || !isfinite(aspect) || !isfinite(near_plane) || !isfinite(far_plane) ||
       fov<5.f || fov>150.f || aspect<.2f || aspect>6.f || near_plane<=0 || far_plane<=near_plane*2.f)return 0;
    camera->fov=fov;camera->aspect=aspect;camera->near_plane=near_plane;camera->far_plane=far_plane;return 1;
}
static inline void jnet_flower_multiply(const float a[16],const float b[16],float out[16]) {
    float result[16];
    for(unsigned row=0;row<4;++row)for(unsigned column=0;column<4;++column){
        float sum=0;for(unsigned k=0;k<4;++k)sum+=a[row*4+k]*b[k*4+column];
        result[row*4+column]=sum;
    }
    memcpy(out,result,sizeof(result));
}
/* View then projection. The vertical field of view uses the same empirical correction as the journey_detour overlay: Journey reports
   its field of view in degrees but the projected angle is a little narrower at wide settings. `aspect` is the back buffer's. */
static inline void jnet_flower_view_projection(const struct jnet_flower_camera *camera,float aspect,float out[16]) {
    float view[16]={
        camera->right[0],camera->up[0],-camera->back[0],0,
        camera->right[1],camera->up[1],-camera->back[1],0,
        camera->right[2],camera->up[2],-camera->back[2],0,
        -jnet_flower_dot(camera->position,camera->right),-jnet_flower_dot(camera->position,camera->up),
        jnet_flower_dot(camera->position,camera->back),1};
    float ratio=camera->fov/180.f,angle=camera->fov*(1.f-.5f*ratio*ratio)*(3.14159265359f/180.f);
    float f=1.f/tanf(angle*.5f),n=camera->near_plane,r=camera->far_plane;
    /* View z is the distance along the view direction: clip.w = z, depth in 0..1 for D3D. */
    float projection[16]={f/aspect,0,0,0, 0,f,0,0, 0,0,r/(r-n),1, 0,0,-n*r/(r-n),0};
    jnet_flower_multiply(view,projection,out);
}
/* Model matrix: scale, turn about the vertical axis (x'=x*c+z*s, z'=z*c-x*s) and move to the position. */
static inline void jnet_flower_model_matrix(const struct jnet_flower_item *item,float out[16]) {
    float c=cosf(item->yaw)*item->scale,s=sinf(item->yaw)*item->scale;
    float model[16]={c,0,-s,0, 0,item->scale,0,0, s,0,c,0, item->position[0],item->position[1],item->position[2],1};
    memcpy(out,model,sizeof(model));
}
static inline void jnet_flower_clip(const float matrix[16],const float point[3],float out[4]) {
    for(unsigned column=0;column<4;++column)
        out[column]=point[0]*matrix[column]+point[1]*matrix[4+column]+point[2]*matrix[8+column]+matrix[12+column];
}
static inline float jnet_flower_distance(const struct jnet_flower_camera *camera,const float point[3]) {
    float d[3]={point[0]-camera->position[0],point[1]-camera->position[1],point[2]-camera->position[2]};
    return sqrtf(jnet_flower_dot(d,d));
}
/* Opacity for a flower at this distance: solid up close, fading out before the draw limit so far flowers do not look cut out of
   the fogged scene. Zero means skip it. */
static inline float jnet_flower_fade(float distance) {
    if(!isfinite(distance) || distance>=JNET_FLOWER_DRAW_DISTANCE)return 0;
    if(distance<=JNET_FLOWER_FADE_DISTANCE)return 1;
    return (JNET_FLOWER_DRAW_DISTANCE-distance)/(JNET_FLOWER_DRAW_DISTANCE-JNET_FLOWER_FADE_DISTANCE);
}
/* True when a flower's centre (the point 0.5 above its base) is inside the view, with a margin for its width. */
static inline int jnet_flower_in_view(const float view_projection[16],const struct jnet_flower_item *item) {
    float centre[3]={item->position[0],item->position[1]+.5f*item->scale,item->position[2]},clip[4];
    jnet_flower_clip(view_projection,centre,clip);
    float margin=1.25f*clip[3];
    return clip[3]>0 && clip[2]>0 && clip[2]<clip[3] && fabsf(clip[0])<=margin && fabsf(clip[1])<=margin;
}
/* True when the terrain between the camera and a flower does not rise above the line of sight. The private pass has no scene depth,
   so this keeps flowers behind a dune from showing through it. */
static inline int jnet_flower_line_clear(const struct jnet_flower_camera *camera,const struct jnet_flower_item *item,
        jnet_flower_floor_function floor,void *context) {
    float target[3]={item->position[0],item->position[1]+.5f*item->scale,item->position[2]};
    for(unsigned step=1;step<JNET_FLOWER_TERRAIN_STEPS;++step){
        float t=(float)step/(float)JNET_FLOWER_TERRAIN_STEPS,spot[3],height;
        for(unsigned i=0;i<3;++i)spot[i]=camera->position[i]+(target[i]-camera->position[i])*t;
        if(!floor(context,spot,&height) || !isfinite(height))return 1;
        if(height>spot[1]+.15f)return 0;
    }
    return 1;
}
/* How a planted flower looks at `now`: it grows from the ground and opens part way; a touch or chirp opens it fully with a glow and a
   shiver that fade over a few seconds, after which it settles back to its idle state. */
#define JNET_FLOWER_IDLE_BLOOM 0.45f
struct jnet_flower_look {float bloom,scale,glow;};
static inline float jnet_flower_smooth(float t) {t=t<0?0:t>1?1:t;return t*t*(3-2*t);}
static inline struct jnet_flower_look jnet_flower_animate(const struct jnet_flower_item *item,uint32_t now) {
    float age=(float)jnet_flower_elapsed(now,item->born)/1000.f;
    if(age<0)age=0;
    float grow=jnet_flower_smooth(age/.8f),bloom=JNET_FLOWER_IDLE_BLOOM*jnet_flower_smooth((age-.3f)/1.f),glow=0;
    if(item->touched){
        float since=(float)jnet_flower_elapsed(now,item->touched)/1000.f;
        if(since>=0 && since<20.f){
            glow=since<.35f?jnet_flower_smooth(since/.35f):expf(-(since-.35f)/3.5f);
            bloom+=(1.f-bloom)*glow;
        }
    }
    return (struct jnet_flower_look){bloom,(.3f+.7f*grow)*(1.f+.12f*glow),glow};
}
#endif
