#include <assert.h>
#include <stdio.h>
#include "jnet_remote_pose.h"
static float sample(struct jnet_remote_pose *p,uint64_t sequence,uint64_t seen,uint64_t now,float x,float angle) {
    float position[3]={x,0,0},rotation[4]={0,sinf(angle*.5f),0,cosf(angle*.5f)};
    assert(jnet_remote_pose_update(p,sequence,seen,now,position,rotation));
    float norm=0;for(unsigned i=0;i<4;++i){assert(isfinite(rotation[i]));norm+=rotation[i]*rotation[i];}
    assert(fabsf(norm-1)<.00001f);
    return position[0];
}
static float recovery(unsigned dt) {
    struct jnet_remote_pose p={0};
    assert(sample(&p,1,1000,1000,0,0)==0);
    assert(sample(&p,1,1000,1300,0,0)==0);
    for(unsigned t=dt;t<=200;t+=dt)sample(&p,2,1100,1300+t,4,1);
    return p.output_position[0];
}
int main(void) {
    struct jnet_remote_pose p={0};
    assert(sample(&p,1,1000,1000,0,0)==0);
    for(unsigned now=1010;now<1350;now+=10)assert(sample(&p,1,1000,now,0,0)==0);
    float previous=sample(&p,2,1343,1350,5.969f,1.5f);
    for(unsigned now=1360;now<=1600;now+=10){
        float x=sample(&p,2,1343,now,5.969f,1.5f);
        assert(x>=previous && x-previous<1.f);previous=x;
    }
    assert(previous>5.8f && previous<5.969f);
    assert(fabsf(recovery(10)-recovery(20))<.00001f);
    float position[3]={5.969f,0,0},rotation[4]={0,0,0,-1};
    assert(jnet_remote_pose_update(&p,3,1600,1610,position,rotation));
    assert(isfinite(rotation[3]));
    float held=p.output_position[0];
    assert(sample(&p,3,1600,1610,5.969f,0)==held);
    assert(sample(&p,4,1620,1620,100,0)==100);
    assert(sample(&p,5,2200,2200,102,0)==102);
    assert(sample(&p,1,2210,2210,2,0)==2);
    position[0]=NAN;assert(!jnet_remote_pose_update(&p,2,2220,2220,position,rotation));
    position[0]=2;assert(!jnet_remote_pose_update(&p,2,2230,2220,position,rotation));
    memset(rotation,0,sizeof(rotation));assert(!jnet_remote_pose_update(&p,2,2220,2220,position,rotation));
    memset(&p,0,sizeof(p));
    for(unsigned i=1;i<=30;++i)sample(&p,i,1000+i*33,1000+i*33,(float)i*.2f,0);
    assert(p.count==8 && isfinite(p.output_position[0]));
    puts("Remote pose: delivery-gap recovery, frame-rate independence, quaternion normalization, repeated ticks, teleports, stale resets and invalid inputs passed.");
}
