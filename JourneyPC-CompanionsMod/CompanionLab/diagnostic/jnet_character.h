#ifndef JNET_CHARACTER_H
#define JNET_CHARACTER_H
#include <math.h>
#include <string.h>

enum jnet_character_kind {
    JNET_CHARACTER_WAYFARER, JNET_CHARACTER_LONGTAIL,
    JNET_CHARACTER_WAR_MACHINE, JNET_CHARACTER_WHALE,
    JNET_CHARACTER_ROCKET, JNET_CHARACTER_FLOW_1, JNET_CHARACTER_FLOW_2,
    JNET_CHARACTER_COUNT
};
static inline int jnet_character_is_flow(unsigned kind) {
    return kind==JNET_CHARACTER_FLOW_1 || kind==JNET_CHARACTER_FLOW_2;
}
static inline unsigned jnet_character_variant_limit(unsigned kind) {
    if(kind==JNET_CHARACTER_WAR_MACHINE)return 2;
    return kind==JNET_CHARACTER_LONGTAIL || kind==JNET_CHARACTER_WHALE;
}
static inline const char *jnet_character_name(unsigned kind) {
    static const char *names[]={"Wayfarer","Longtail","War Machine","Whale","Rocket Deathmatch","fl0w 1","fl0w 2"};
    return kind<JNET_CHARACTER_COUNT?names[kind]:"Unknown";
}
static inline int jnet_character_is_creature(unsigned kind) {
    return kind>JNET_CHARACTER_WAYFARER && kind<JNET_CHARACTER_COUNT && kind!=JNET_CHARACTER_ROCKET;
}
struct jnet_character_animation_dependency {
    const char *name;
    unsigned characters,missing;
};
static inline unsigned jnet_character_animation_blocked(struct jnet_character_animation_dependency *items,
        unsigned count,const char *name,int ready) {
    unsigned blocked=0;
    for(unsigned i=0;i<count;++i){
        if(!strcmp(items[i].name,name))items[i].missing=!ready;
        if(items[i].missing)blocked|=items[i].characters;
    }
    return blocked;
}
static inline float jnet_character_speed(unsigned kind) {
    return kind==JNET_CHARACTER_LONGTAIL || kind==JNET_CHARACTER_FLOW_1 || kind==JNET_CHARACTER_FLOW_2?10.8f:8.28f;
}
static inline float jnet_character_move_speed(unsigned kind,unsigned sprint) {
    return jnet_character_speed(kind)*(sprint && jnet_character_is_creature(kind)?2.f:1.f);
}
static inline int jnet_character_camera_distance(unsigned kind,float baseline,float *out) {
    if(!out || !jnet_character_is_creature(kind) ||
       !isfinite(baseline) || baseline<=0 || baseline>1000)return 0;
    *out=baseline*((kind==JNET_CHARACTER_LONGTAIL || kind==JNET_CHARACTER_FLOW_1 || kind==JNET_CHARACTER_FLOW_2)?1.25f:5.0f);
    return 1;
}
static inline int jnet_character_large_chirp(unsigned kind,unsigned charge) {
    return charge==4 && (kind==JNET_CHARACTER_WHALE || kind==JNET_CHARACTER_WAR_MACHINE);
}
struct jnet_character_input {
    float forward,strafe,camera_forward[3],camera_right[3];
    unsigned fly,enabled,interact;
    unsigned fly_down,level;
};
struct jnet_character_control {
    unsigned kind,spotlight,interact_down;
};
struct jnet_character_intent {
    float direction[3],strength;
    float roll;
    unsigned spotlight,spotlight_changed;
};
struct jnet_character_pose {
    float position[3],rotation[4];
};
static inline int jnet_character_select(struct jnet_character_control *control,unsigned kind,unsigned interact_down) {
    if(!control || kind>=JNET_CHARACTER_COUNT)return 0;
    *control=(struct jnet_character_control){kind,0,!!interact_down};
    return 1;
}
static inline int jnet_character_unit(float v[3]) {
    float n=0;
    for(unsigned i=0;i<3;++i){if(!isfinite(v[i]))return 0;n+=v[i]*v[i];}
    if(!isfinite(n) || n<0.000001f)return 0;
    n=sqrtf(n);for(unsigned i=0;i<3;++i)v[i]/=n;
    return 1;
}
static inline int jnet_character_control_step(struct jnet_character_control *control,
        const struct jnet_character_input *input,struct jnet_character_intent *out) {
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!control || !input || control->kind>=JNET_CHARACTER_COUNT)return 0;
    unsigned pressed=input->interact && !control->interact_down;
    control->interact_down=!!input->interact;
    out->spotlight=control->spotlight;
    if(!input->enabled || control->kind==JNET_CHARACTER_WAYFARER)return 1;
    if(!isfinite(input->forward) || !isfinite(input->strafe))return 0;
    float forward[3],right[3];
    memcpy(forward,input->camera_forward,sizeof(forward));
    memcpy(right,input->camera_right,sizeof(right));
    if(input->level){
        forward[1]=right[1]=0;
        if(!jnet_character_unit(forward)){
            if(!jnet_character_unit(right))return 0;
            forward[0]=-right[2];forward[2]=right[0];
        }
    }
    if(!jnet_character_unit(forward) || !jnet_character_unit(right))return 0;
    float dot=0;for(unsigned i=0;i<3;++i)dot+=forward[i]*right[i];
    for(unsigned i=0;i<3;++i)right[i]-=dot*forward[i];
    if(!jnet_character_unit(right))return 0;
    float f=fmaxf(-1,fminf(1,input->forward)),r=fmaxf(-1,fminf(1,input->strafe));
    float length=sqrtf(f*f+r*r);
    if(length>0.1f){
        out->strength=fminf(1,(length-0.1f)/0.9f);
        for(unsigned i=0;i<3;++i)out->direction[i]=(f*forward[i]+r*right[i])/length;
    }
    if(input->fly!=input->fly_down){
        for(unsigned i=0;i<3;++i)out->direction[i]*=out->strength;
        out->direction[1]=input->fly?fmaxf(0,out->direction[1])+1:fminf(0,out->direction[1])-1;
        float magnitude=0;
        for(unsigned i=0;i<3;++i)magnitude+=out->direction[i]*out->direction[i];
        magnitude=sqrtf(magnitude);
        for(unsigned i=0;i<3;++i)out->direction[i]/=magnitude;
        out->strength=1;
    }
    if(control->kind==JNET_CHARACTER_WAR_MACHINE && pressed){
        control->spotlight=!control->spotlight;
        out->spotlight_changed=1;out->spotlight=control->spotlight;
    }
    return 1;
}
static inline int jnet_character_smooth(float motion[3],struct jnet_character_intent *intent,float dt,int enabled) {
    if(!motion || !intent || !isfinite(dt) || dt<0 || !isfinite(intent->strength) ||
       intent->strength<0 || intent->strength>1 || !isfinite(intent->roll))return 0;
    float next[3],length=0,alpha=1-expf(-fminf(dt,.05f)/.28f);
    for(unsigned i=0;i<3;++i){
        if(!isfinite(motion[i]) || !isfinite(intent->direction[i]))return 0;
        next[i]=enabled?motion[i]+alpha*(intent->direction[i]*intent->strength-motion[i]):0;
        length+=next[i]*next[i];
    }
    length=sqrtf(length);
    if(length<.001f){memset(next,0,sizeof(next));length=0;}
    memcpy(motion,next,sizeof(next));intent->strength=fminf(length,1);
    for(unsigned i=0;i<3;++i)intent->direction[i]=length>0?next[i]/length:0;
    return 1;
}
static inline int jnet_character_motion_step(const struct jnet_character_pose *current,
        const struct jnet_character_intent *intent,float speed,float dt,
        struct jnet_character_pose *next) {
    if(!current || !intent || !next || !isfinite(speed) || speed<0 ||
       !isfinite(dt) || dt<0 || !isfinite(intent->strength) ||
       intent->strength<0 || intent->strength>1 || !isfinite(intent->roll))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(current->position[i]))return 0;
    float norm=0;
    for(unsigned i=0;i<4;++i){
        if(!isfinite(current->rotation[i]))return 0;
        norm+=current->rotation[i]*current->rotation[i];
    }
    if(!isfinite(norm) || norm<0.000001f)return 0;
    struct jnet_character_pose result=*current;
    if(dt>0 && speed>0){
        float direction[3];
        if(intent->strength>0)memcpy(direction,intent->direction,sizeof(direction));
        else {
            const float *q=current->rotation;
            direction[0]=2*(q[0]*q[2]+q[3]*q[1])/norm;
            direction[1]=2*(q[1]*q[2]-q[3]*q[0])/norm;
            direction[2]=1-2*(q[0]*q[0]+q[1]*q[1])/norm;
        }
        if(!jnet_character_unit(direction))return 0;
        float distance=speed*intent->strength*fminf(dt,0.05f);
        if(!isfinite(distance))return 0;
        for(unsigned i=0;i<3;++i){
            result.position[i]+=direction[i]*distance;
            if(!isfinite(result.position[i]))return 0;
        }
        float yaw=atan2f(direction[0],direction[2])*0.5f;
        float pitch=-asinf(fmaxf(-1,fminf(1,direction[1])))*0.5f;
        float sy=sinf(yaw),cy=cosf(yaw),sp=sinf(pitch),cp=cosf(pitch);
        float sr=sinf(intent->roll*.5f),cr=cosf(intent->roll*.5f);
        float target[4]={sp*cy*cr+cp*sy*sr,cp*sy*cr-sp*cy*sr,
            cp*cy*sr-sp*sy*cr,cp*cy*cr+sp*sy*sr};
        float dot=0,blend=1-expf(-fminf(dt,.05f)/.4f),scale=1/sqrtf(norm);
        for(unsigned i=0;i<4;++i)dot+=current->rotation[i]*target[i];
        float magnitude=0;
        for(unsigned i=0;i<4;++i){
            result.rotation[i]=current->rotation[i]*scale*(1-blend)+target[i]*(dot<0?-blend:blend);
            magnitude+=result.rotation[i]*result.rotation[i];
        }
        for(unsigned i=0;i<4;++i)result.rotation[i]/=sqrtf(magnitude);
    }
    *next=result;return 1;
}
#endif
