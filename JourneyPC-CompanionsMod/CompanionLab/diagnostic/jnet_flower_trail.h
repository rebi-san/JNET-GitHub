#ifndef JNET_FLOWER_TRAIL_H
#define JNET_FLOWER_TRAIL_H
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "jnet_character.h"

/* Flower Trail: while the fl0w creature holds the Flower Trail action and is within JNET_FLOWER_FLOOR_RANGE of the floor on the
   vertical axis, a random assortment of the eight Flower types is planted on the floor around it. The placement list is a ring:
   the oldest flower is replaced once JNET_FLOWER_CAPACITY is reached, and it is emptied when the level changes. */
#define JNET_FLOWER_TYPES 8u
#define JNET_FLOWER_CAPACITY 512u
#define JNET_FLOWER_FLOOR_RANGE 2.0f
#define JNET_FLOWER_RATE 7.0f
#define JNET_FLOWER_SPREAD 1.6f
#define JNET_FLOWER_BURST 4u
#define JNET_FLOWER_TOUCH_RADIUS 1.0f
#define JNET_FLOWER_NEW_MS 1200u
#define JNET_FLOWER_TOUCH_HEIGHT 3.0f
#define JNET_FLOWER_CHIRP_RANGE 9.0f
#define JNET_FLOWER_CHIRP_SPEED_MS 45.0f

/* `born` and `touched` are millisecond clock values (wrapping 32-bit); touched 0 means never. A touched time in the future is a chirp
   ripple that has not reached the flower yet. */
struct jnet_flower_item {
    float position[3],yaw,scale;
    unsigned type;
    uint32_t born,touched;
};
struct jnet_flower_trail {
    struct jnet_flower_item items[JNET_FLOWER_CAPACITY];
    unsigned count,next,level;
    float carry;
    uint32_t seed;
    uint64_t placed;
};
typedef int (*jnet_flower_floor_function)(void *context,const float position[3],float *height);

static inline void jnet_flower_trail_clear(struct jnet_flower_trail *trail) {
    trail->count=trail->next=0;trail->carry=0;
}
static inline uint32_t jnet_flower_random(struct jnet_flower_trail *trail) {
    uint32_t x=trail->seed?trail->seed:0x9e3779b9u;
    x^=x<<13;x^=x>>17;x^=x<<5;
    return trail->seed=x;
}
static inline float jnet_flower_unit(struct jnet_flower_trail *trail) {
    return (float)(jnet_flower_random(trail)>>8)*(1.f/16777216.f);
}
static inline int jnet_flower_finite(const float position[3]) {
    return isfinite(position[0]) && isfinite(position[1]) && isfinite(position[2]);
}
/* True when the creature is within the allowed vertical distance of the floor beneath it; floor receives that height. */
static inline int jnet_flower_trail_reachable(const float creature[3],jnet_flower_floor_function floor,void *context,float *height) {
    if(!floor || !jnet_flower_finite(creature) || !floor(context,creature,height) || !isfinite(*height))return 0;
    float above=creature[1]-*height;
    return above<=JNET_FLOWER_FLOOR_RANGE && above>=-JNET_FLOWER_FLOOR_RANGE;
}
static inline void jnet_flower_trail_add(struct jnet_flower_trail *trail,const float position[3],unsigned type,float yaw,float scale,uint32_t now) {
    struct jnet_flower_item *item=&trail->items[trail->next];
    memcpy(item->position,position,sizeof(item->position));
    item->type=type;item->yaw=yaw;item->scale=scale;item->born=now;item->touched=0;
    trail->next=(trail->next+1)%JNET_FLOWER_CAPACITY;
    if(trail->count<JNET_FLOWER_CAPACITY)++trail->count;
    ++trail->placed;
}
/* One frame. `held` is the Flower Trail action; only fl0w plants flowers. Returns how many were planted this frame. */
static inline unsigned jnet_flower_trail_step(struct jnet_flower_trail *trail,unsigned level,unsigned kind,unsigned held,
        const float creature[3],float dt,jnet_flower_floor_function floor,void *context,uint32_t now) {
    if(trail->level!=level){jnet_flower_trail_clear(trail);trail->level=level;}
    if(!held || !jnet_character_is_flow(kind) || !isfinite(dt) || dt<=0){trail->carry=0;return 0;}
    float height;
    if(!jnet_flower_trail_reachable(creature,floor,context,&height))return 0;
    trail->carry+=JNET_FLOWER_RATE*fminf(dt,.1f);
    unsigned planted=0;
    while(trail->carry>=1.f && planted<JNET_FLOWER_BURST){
        trail->carry-=1.f;
        float angle=6.28318530718f*jnet_flower_unit(trail),radius=JNET_FLOWER_SPREAD*sqrtf(jnet_flower_unit(trail));
        float spot[3]={creature[0]+radius*cosf(angle),0,creature[2]+radius*sinf(angle)};
        spot[1]=creature[1];
        float ground;
        if(!floor(context,spot,&ground) || !isfinite(ground) || fabsf(ground-height)>JNET_FLOWER_FLOOR_RANGE)continue;
        spot[1]=ground;
        unsigned type=jnet_flower_random(trail)%JNET_FLOWER_TYPES;
        float yaw=6.28318530718f*jnet_flower_unit(trail),scale=.8f+.5f*jnet_flower_unit(trail);
        jnet_flower_trail_add(trail,spot,type,yaw,scale,now);++planted;
    }
    if(trail->carry>=1.f)trail->carry=0;
    return planted;
}
/* Signed milliseconds from `then` to `now`, correct across the 32-bit wrap. */
static inline int32_t jnet_flower_elapsed(uint32_t now,uint32_t then) {return (int32_t)(now-then);}
/* The creature touching flowers opens them, as walking into the Desert's flower does: every flower within reach of the creature is
   touched now unless it is still growing from being planted. Returns how many were woken: a flower that was not already responding
   (never touched, or its last response is over). `woken_at` receives the first such flower's position, for the response sound. */
#define JNET_FLOWER_AWAKE_MS 4000
static inline int jnet_flower_awake(const struct jnet_flower_item *item,uint32_t when) {
    return item->touched && jnet_flower_elapsed(when,item->touched)<JNET_FLOWER_AWAKE_MS;
}
static inline unsigned jnet_flower_trail_touch(struct jnet_flower_trail *trail,const float creature[3],uint32_t now,float woken_at[3]) {
    if(!jnet_flower_finite(creature))return 0;
    unsigned touched=0;
    for(unsigned i=0;i<trail->count;++i){
        struct jnet_flower_item *item=&trail->items[i];
        if(jnet_flower_elapsed(now,item->born)<(int32_t)JNET_FLOWER_NEW_MS)continue;
        float dx=item->position[0]-creature[0],dz=item->position[2]-creature[2],reach=JNET_FLOWER_TOUCH_RADIUS*item->scale;
        if(dx*dx+dz*dz>reach*reach || fabsf(item->position[1]-creature[1])>JNET_FLOWER_TOUCH_HEIGHT)continue;
        if(!jnet_flower_awake(item,now)){if(!touched && woken_at)memcpy(woken_at,item->position,sizeof(item->position));++touched;}
        item->touched=now?now:1;
    }
    return touched;
}
/* A chirp (the Desert flower's EventOnShout) reaches flowers within range of where it was made, a little later the farther they are, so
   the response travels outward like the chirp. `radius` is the chirp's own radius; the range is never smaller than the default. */
static inline unsigned jnet_flower_trail_chirp(struct jnet_flower_trail *trail,const float source[3],float radius,uint32_t now,float woken_at[3]) {
    if(!jnet_flower_finite(source) || !isfinite(radius))return 0;
    float range=radius>JNET_FLOWER_CHIRP_RANGE?radius:JNET_FLOWER_CHIRP_RANGE;
    if(range>60.f)range=60.f;
    unsigned reached=0;
    for(unsigned i=0;i<trail->count;++i){
        struct jnet_flower_item *item=&trail->items[i];
        float d[3]={item->position[0]-source[0],item->position[1]-source[1],item->position[2]-source[2]};
        float distance=sqrtf(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
        if(distance>range)continue;
        uint32_t when=now+(uint32_t)(distance*JNET_FLOWER_CHIRP_SPEED_MS);
        if(!jnet_flower_awake(item,when)){if(!reached && woken_at)memcpy(woken_at,item->position,sizeof(item->position));++reached;}
        item->touched=when?when:1;
    }
    return reached;
}
#endif
