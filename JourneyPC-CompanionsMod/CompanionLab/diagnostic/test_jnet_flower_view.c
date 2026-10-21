#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "jnet_flower_view.h"

static int near(float a,float b,float e) {return fabsf(a-b)<=e;}
static int floor_ridge(void *context,const float position[3],float *height) {
    (void)context;*height=fabsf(position[0]-5.f)<1.f?20.f:0.f;return 1;
}
static int floor_fail(void *context,const float position[3],float *height) {(void)context;(void)position;(void)height;return 0;}
static struct jnet_flower_camera make(float fov) {
    /* At the origin looking down -z: right +x, up +y, back +z. */
    const float rows[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    struct jnet_flower_camera camera;
    assert(jnet_flower_camera_rows(&camera,rows,NULL) && jnet_flower_camera_lens(&camera,fov,16.f/9.f,.1f,1000.f));
    return camera;
}
static void project(const struct jnet_flower_camera *camera,const float p[3],float ndc[3],float *w) {
    float vp[16],clip[4];
    jnet_flower_view_projection(camera,16.f/9.f,vp);jnet_flower_clip(vp,p,clip);
    *w=clip[3];ndc[0]=clip[0]/clip[3];ndc[1]=clip[1]/clip[3];ndc[2]=clip[2]/clip[3];
}
int main(void) {
    struct jnet_flower_camera camera=make(60.f);
    float ndc[3],w;
    /* Straight ahead is the centre of the screen, right and up move the matching way, and depth runs 0..1 between near and far. */
    const float ahead[3]={0,0,-10},right[3]={2,0,-10},up[3]={0,2,-10},behind[3]={0,0,10},near_point[3]={0,0,-.1f},far_point[3]={0,0,-1000};
    project(&camera,ahead,ndc,&w);assert(near(ndc[0],0,1e-5f) && near(ndc[1],0,1e-5f) && ndc[2]>0 && ndc[2]<1 && near(w,10,1e-4f));
    project(&camera,right,ndc,&w);assert(ndc[0]>0 && near(ndc[1],0,1e-5f));
    project(&camera,up,ndc,&w);assert(ndc[1]>0 && near(ndc[0],0,1e-5f));
    project(&camera,behind,ndc,&w);assert(w<0);
    project(&camera,near_point,ndc,&w);assert(near(ndc[2],0,1e-4f));
    project(&camera,far_point,ndc,&w);assert(near(ndc[2],1,1e-4f));
    /* The empirical correction narrows the angle: 2 units up at 10 away is tan(a)=.2 against f. */
    float angle=60.f*(1.f-.5f/9.f)*(3.14159265359f/180.f);
    project(&camera,up,ndc,&w);assert(near(ndc[1],.2f/tanf(angle*.5f),1e-4f));
    project(&camera,right,ndc,&w);assert(near(ndc[0],.2f/tanf(angle*.5f)/(16.f/9.f),1e-4f));
    /* A turned and raised camera: looking along +x from (3,4,5) with up +y. */
    const float turned[16]={0,0,1,0, 0,1,0,0, -1,0,0,0, 3,4,5,1};
    struct jnet_flower_camera side;
    assert(jnet_flower_camera_rows(&side,turned,NULL) && jnet_flower_camera_lens(&side,60.f,16.f/9.f,.1f,1000.f));
    const float forward[3]={13,4,5};
    project(&side,forward,ndc,&w);assert(near(ndc[0],0,1e-5f) && near(ndc[1],0,1e-5f) && near(w,10,1e-4f));
    /* The basis and position checks refuse a wrong layout. */
    struct jnet_flower_camera other;const float player[3]={0,0,0};
    float skewed[16]={1,0,0,0, 0,1,0,0, 1,0,1,0, 0,0,0,1},far_away[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 500,0,0,1},mirrored[16]={-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    assert(!jnet_flower_camera_rows(&other,skewed,NULL) && !jnet_flower_camera_rows(&other,far_away,player) && !jnet_flower_camera_rows(&other,(float[16]){2,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1},NULL));
    /* A mirrored (left-handed) basis is a valid camera: screen right is world -x, so a point at x=-2 is right of centre. */
    struct jnet_flower_camera lefty;
    assert(jnet_flower_camera_rows(&lefty,mirrored,NULL) && jnet_flower_camera_lens(&lefty,60.f,16.f/9.f,.1f,1000.f));
    const float left_world[3]={-2,0,-10};project(&lefty,left_world,ndc,&w);assert(ndc[0]>0);
    skewed[0]=NAN;assert(!jnet_flower_camera_rows(&other,skewed,NULL));
    assert(!jnet_flower_camera_lens(&other,0,1.7f,.1f,100) && !jnet_flower_camera_lens(&other,60,NAN,.1f,100) &&
           !jnet_flower_camera_lens(&other,60,1.7f,0,100) && !jnet_flower_camera_lens(&other,60,1.7f,50,60) && !jnet_flower_camera_lens(&other,170,1.7f,.1f,100));
    /* Model matrix: scale, yaw about the vertical axis and translation. */
    struct jnet_flower_item item={{10,2,-4},1.57079632679f,2.f,0,0,0};
    float model[16],point[4],unit[3]={1,1,0};
    jnet_flower_model_matrix(&item,model);jnet_flower_clip(model,unit,point);
    assert(near(point[0],10,1e-5f) && near(point[1],4,1e-5f) && near(point[2],-6,1e-4f));
    /* Distance fade: solid, fading, gone. */
    assert(jnet_flower_fade(0)==1 && jnet_flower_fade(JNET_FLOWER_FADE_DISTANCE)==1 && jnet_flower_fade(JNET_FLOWER_DRAW_DISTANCE)==0 &&
           jnet_flower_fade(NAN)==0 && jnet_flower_fade(1e9f)==0);
    float middle=jnet_flower_fade((JNET_FLOWER_FADE_DISTANCE+JNET_FLOWER_DRAW_DISTANCE)/2);assert(near(middle,.5f,1e-5f));
    assert(near(jnet_flower_distance(&camera,(float[]){3,4,0}),5,1e-5f));
    /* View culling uses the flower's centre with a margin. */
    float vp[16];jnet_flower_view_projection(&camera,16.f/9.f,vp);
    struct jnet_flower_item in_front={{0,-.5f,-10},0,1,0,0,0},to_side={{80,-.5f,-10},0,1,0,0,0},at_back={{0,-.5f,10},0,1,0,0,0},too_far={{0,-.5f,-2000},0,1,0,0,0};
    assert(jnet_flower_in_view(vp,&in_front) && !jnet_flower_in_view(vp,&to_side) && !jnet_flower_in_view(vp,&at_back) && !jnet_flower_in_view(vp,&too_far));
    /* Terrain between camera and flower hides it; open ground or an unreadable floor does not. */
    struct jnet_flower_camera low=camera;low.position[1]=2;
    struct jnet_flower_item behind_ridge={{10,0,0},0,1,0,0,0},clear_side={{0,0,10},0,1,0,0,0};
    assert(!jnet_flower_line_clear(&low,&behind_ridge,floor_ridge,NULL) && jnet_flower_line_clear(&low,&clear_side,floor_ridge,NULL) &&
           jnet_flower_line_clear(&low,&behind_ridge,floor_fail,NULL));
    /* The calibration search finds a vector by value and refuses empty or non-finite targets. */
    unsigned char memory[64]={0};float spot[3]={202.3f,39.8f,347.1f},wanted[3]={202.31f,39.79f,347.12f},zero[3]={0,0,0};size_t found=99;
    memcpy(memory+20,spot,12);
    assert(jnet_flower_find_vector(memory,sizeof(memory),wanted,.05f,&found) && found==20);
    wanted[1]=41.f;assert(!jnet_flower_find_vector(memory,sizeof(memory),wanted,.05f,&found));
    assert(!jnet_flower_find_vector(memory,sizeof(memory),zero,.05f,&found) && !jnet_flower_find_vector(memory,8,spot,.05f,&found) &&
           !jnet_flower_find_vector(NULL,64,spot,.05f,&found));
    spot[0]=NAN;assert(!jnet_flower_find_vector(memory,sizeof(memory),spot,.05f,&found));
    /* Lua's number text parses without strtod. */
    float parsed;
    assert(jnet_flower_number("1.5",&parsed) && parsed==1.5f && jnet_flower_number("-3",&parsed) && parsed==-3.f &&
           jnet_flower_number("6.5e-05",&parsed) && near(parsed,6.5e-5f,1e-9f) && jnet_flower_number("1e+3",&parsed) && parsed==1000.f &&
           jnet_flower_number("0.1666666666666667",&parsed) && near(parsed,.16666667f,1e-7f) && jnet_flower_number("+2.",&parsed) && parsed==2.f &&
           jnet_flower_number(".5",&parsed) && parsed==.5f);
    assert(!jnet_flower_number("",&parsed) && !jnet_flower_number("-",&parsed) && !jnet_flower_number("nan",&parsed) && !jnet_flower_number("inf",&parsed) &&
           !jnet_flower_number("1.5x",&parsed) && !jnet_flower_number("1e",&parsed) && !jnet_flower_number("1e999",&parsed) && !jnet_flower_number(NULL,&parsed) &&
           !jnet_flower_number("1",NULL));
    /* Animation: grows from the ground, opens part way, and a touch opens it fully with a glow that settles; a future touch waits. */
    struct jnet_flower_item plant={{0,0,0},0,1.f,0,1000,0};
    struct jnet_flower_look look=jnet_flower_animate(&plant,1000);
    assert(near(look.bloom,0,1e-6f) && near(look.scale,.3f,1e-6f) && look.glow==0);
    look=jnet_flower_animate(&plant,900);assert(near(look.scale,.3f,1e-6f));
    look=jnet_flower_animate(&plant,6000);assert(near(look.bloom,JNET_FLOWER_IDLE_BLOOM,1e-5f) && near(look.scale,1.f,1e-5f) && look.glow==0);
    plant.touched=10000;
    look=jnet_flower_animate(&plant,9000);assert(near(look.bloom,JNET_FLOWER_IDLE_BLOOM,1e-5f) && look.glow==0);
    look=jnet_flower_animate(&plant,10350);assert(near(look.glow,1.f,1e-4f) && near(look.bloom,1.f,1e-4f) && near(look.scale,1.12f,1e-4f));
    look=jnet_flower_animate(&plant,10000+350+3500);assert(near(look.glow,expf(-1.f),1e-4f) && look.bloom<1.f && look.bloom>JNET_FLOWER_IDLE_BLOOM);
    look=jnet_flower_animate(&plant,10000+30000);assert(near(look.bloom,JNET_FLOWER_IDLE_BLOOM,1e-5f) && look.glow==0);
    plant.born=0xfffffff0u;look=jnet_flower_animate(&plant,0x20);assert(look.scale>=.3f && look.scale<=1.12f);
    puts("Flower view: camera basis validation, projection depth/orientation, model matrix, fade, culling and terrain line of sight passed.");
    return 0;
}
