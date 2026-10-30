#ifndef JNET_REMOTE_POSE_H
#define JNET_REMOTE_POSE_H
#include <stdint.h>
#include <string.h>
#include <math.h>
struct jnet_remote_pose_sample {uint64_t tick;float position[3],rotation[4];};
struct jnet_remote_pose {
    struct jnet_remote_pose_sample samples[8];
    uint64_t sequence;
    unsigned count;
    uint64_t output_tick;
    float output_position[3],output_rotation[4];
    unsigned output_valid;
};
static inline int jnet_remote_pose_update(struct jnet_remote_pose *p,uint64_t sequence,
        uint64_t seen,uint64_t now,float position[3],float rotation[4]) {
    float norm=0;
    if(!sequence || seen>now)return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i]))return 0;
    for(unsigned i=0;i<4;++i){if(!isfinite(rotation[i]))return 0;norm+=rotation[i]*rotation[i];}
    if(!isfinite(norm) || norm<.99f || norm>1.01f)return 0;
    if(sequence!=p->sequence){
        if(p->count){
            const struct jnet_remote_pose_sample *last=&p->samples[p->count-1];float distance=0;
            for(unsigned i=0;i<3;++i){float d=position[i]-last->position[i];distance+=d*d;}
            if(sequence<p->sequence || seen<last->tick || seen-last->tick>500 || distance>1024){p->count=0;p->output_valid=0;}
            else if(seen==last->tick)--p->count;
        }
        if(p->count==8){memmove(p->samples,p->samples+1,7*sizeof(*p->samples));--p->count;}
        struct jnet_remote_pose_sample *s=&p->samples[p->count++];s->tick=seen;
        memcpy(s->position,position,sizeof(s->position));memcpy(s->rotation,rotation,sizeof(s->rotation));
        p->sequence=sequence;
    }
    if(!p->count)return 0;
    uint64_t tick=now>100?now-100:0;
    const struct jnet_remote_pose_sample *a=p->samples,*b=a;float t=0;
    for(unsigned i=1;i<p->count && tick>a->tick;++i){
        b=&p->samples[i];
        if(tick<b->tick){t=(float)(tick-a->tick)/(float)(b->tick-a->tick);break;}
        a=b;
    }
    float dot=0;for(unsigned i=0;i<4;++i)dot+=a->rotation[i]*b->rotation[i];
    norm=0;
    for(unsigned i=0;i<3;++i)position[i]=a->position[i]+t*(b->position[i]-a->position[i]);
    for(unsigned i=0;i<4;++i){rotation[i]=(1-t)*a->rotation[i]+t*(dot<0?-b->rotation[i]:b->rotation[i]);norm+=rotation[i]*rotation[i];}
    float scale=1/sqrtf(norm);for(unsigned i=0;i<4;++i)rotation[i]*=scale;
    if(p->output_valid && now>=p->output_tick && now-p->output_tick<=500){
        float blend=1-expf(-(float)(now-p->output_tick)/50.f);
        for(unsigned i=0;i<3;++i)position[i]=p->output_position[i]+blend*(position[i]-p->output_position[i]);
        dot=0;for(unsigned i=0;i<4;++i)dot+=p->output_rotation[i]*rotation[i];
        norm=0;
        for(unsigned i=0;i<4;++i){rotation[i]=(1-blend)*p->output_rotation[i]+blend*(dot<0?-rotation[i]:rotation[i]);norm+=rotation[i]*rotation[i];}
        scale=1/sqrtf(norm);for(unsigned i=0;i<4;++i)rotation[i]*=scale;
    }
    memcpy(p->output_position,position,sizeof(p->output_position));
    memcpy(p->output_rotation,rotation,sizeof(p->output_rotation));
    p->output_tick=now;p->output_valid=1;
    return 1;
}
#endif
