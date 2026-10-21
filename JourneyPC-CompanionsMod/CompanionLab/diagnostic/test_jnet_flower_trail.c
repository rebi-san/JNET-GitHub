#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "jnet_flower_trail.h"

static unsigned calls,fail_after;
static int floor_slope(void *context,const float position[3],float *height) {
    (void)context;
    if(fail_after && ++calls>fail_after)return 0;
    *height=.1f*position[0];
    return 1;
}
static int floor_cliff(void *context,const float position[3],float *height) {
    (void)context;*height=position[0]>.4f?-10.f:0.f;return 1;
}
static int floor_nan(void *context,const float position[3],float *height) {
    (void)context;(void)position;*height=NAN;return 1;
}
static struct jnet_flower_trail trail;
static uint32_t clock=1000;

static unsigned run(unsigned frames,unsigned kind,unsigned held,const float creature[3],jnet_flower_floor_function floor) {
    unsigned total=0;
    for(unsigned i=0;i<frames;++i)total+=jnet_flower_trail_step(&trail,3,kind,held,creature,1.f/60.f,floor,NULL,clock+=16);
    return total;
}
int main(void) {
    const float low[3]={0,1.5f,0};
    trail.seed=12345;
    /* Only fl0w plants, and only while the action is held. */
    for(unsigned kind=0;kind<JNET_CHARACTER_COUNT;++kind){
        unsigned planted=run(120,kind,1,low,floor_slope);
        assert(jnet_character_is_flow(kind)?planted>10:planted==0);
        jnet_flower_trail_clear(&trail);
    }
    trail.placed=0;
    assert(run(120,JNET_CHARACTER_FLOW_1,0,low,floor_slope)==0 && !trail.count);
    /* Rate: about fourteen a second. */
    unsigned planted=run(600,JNET_CHARACTER_FLOW_2,1,low,floor_slope);
    assert(planted>=130 && planted<=150);
    /* Every flower is on the floor within the spread, with a valid type, scale and yaw; all eight types appear. */
    unsigned seen=0;
    for(unsigned i=0;i<trail.count;++i){
        const struct jnet_flower_item *f=&trail.items[i];
        assert(fabsf(f->position[1]-.1f*f->position[0])<1e-5f);
        assert(hypotf(f->position[0]-low[0],f->position[2]-low[2])<=JNET_FLOWER_SPREAD+1e-4f);
        assert(f->type<JNET_FLOWER_TYPES && f->scale>=.9f && f->scale<=1.1f && f->yaw>=0 && f->yaw<=6.2832f);
        seen|=1u<<f->type;
    }
    assert(seen==(1u<<JNET_FLOWER_TYPES)-1);
    /* Vertical limit: within two units plants, beyond it does not, and the carry does not store up while away from the floor. */
    jnet_flower_trail_clear(&trail);
    const float edge[3]={0,2.0f,0},high[3]={0,2.01f,0},below[3]={0,-2.01f,0};
    assert(run(120,JNET_CHARACTER_FLOW_1,1,edge,floor_slope)>5);
    jnet_flower_trail_clear(&trail);
    assert(run(600,JNET_CHARACTER_FLOW_1,1,high,floor_slope)==0 && run(600,JNET_CHARACTER_FLOW_1,1,below,floor_slope)==0);
    assert(trail.carry==0);
    /* A floor that cannot be sampled, or is not a number, plants nothing; so do non-finite creature positions. */
    const float bad[3]={NAN,1,0};
    assert(run(120,JNET_CHARACTER_FLOW_1,1,low,floor_nan)==0 && run(120,JNET_CHARACTER_FLOW_1,1,bad,floor_slope)==0);
    assert(jnet_flower_trail_step(&trail,3,JNET_CHARACTER_FLOW_1,1,low,1.f/60.f,NULL,NULL,clock)==0);
    assert(jnet_flower_trail_step(&trail,3,JNET_CHARACTER_FLOW_1,1,low,NAN,floor_slope,NULL,clock)==0);
    assert(jnet_flower_trail_step(&trail,3,JNET_CHARACTER_FLOW_1,1,low,-1.f,floor_slope,NULL,clock)==0);
    calls=0;fail_after=1;
    assert(run(60,JNET_CHARACTER_FLOW_1,1,low,floor_slope)==0);fail_after=0;
    /* Spots where the floor drops away more than two units are skipped. */
    jnet_flower_trail_clear(&trail);
    const float rim[3]={0,.5f,0};
    run(600,JNET_CHARACTER_FLOW_1,1,rim,floor_cliff);
    for(unsigned i=0;i<trail.count;++i)assert(trail.items[i].position[0]<=.4f && trail.items[i].position[1]==0);
    /* A long frame cannot dump a burst of flowers. */
    jnet_flower_trail_clear(&trail);
    assert(jnet_flower_trail_step(&trail,3,JNET_CHARACTER_FLOW_1,1,low,5.f,floor_slope,NULL,clock)<=1);
    /* The oldest flower is replaced at capacity, and the level change empties the list. */
    jnet_flower_trail_clear(&trail);
    uint64_t before=trail.placed;
    run(60*100,JNET_CHARACTER_FLOW_1,1,low,floor_slope);
    assert(trail.placed-before>JNET_FLOWER_CAPACITY && trail.count==JNET_FLOWER_CAPACITY && trail.next<JNET_FLOWER_CAPACITY);
    jnet_flower_trail_step(&trail,4,JNET_CHARACTER_FLOW_1,0,low,1.f/60.f,floor_slope,NULL,clock);
    assert(trail.count==0 && trail.level==4);
    /* Each flower records when it was planted and starts untouched. */
    jnet_flower_trail_clear(&trail);
    uint32_t planted_at=clock+16;
    assert(run(30,JNET_CHARACTER_FLOW_1,1,low,floor_slope)>0 && trail.items[0].born>=planted_at && trail.items[0].touched==0);
    /* A creature touching flowers touches those within reach (scaled by flower size) and height, and no others. */
    jnet_flower_trail_clear(&trail);
    const float at[3]={0,0,0};
    jnet_flower_trail_add(&trail,(float[]){1,0,0},1,0,1.f,100);
    jnet_flower_trail_add(&trail,(float[]){2,0,0},2,0,1.f,100);
    jnet_flower_trail_add(&trail,(float[]){0,1,1},3,0,2.f,100);
    jnet_flower_trail_add(&trail,(float[]){0,5,0},4,0,1.f,100);
    assert(jnet_flower_trail_touch(&trail,at,5000,NULL)==2 && trail.items[0].touched==5000 && !trail.items[1].touched && trail.items[2].touched==5000 && !trail.items[3].touched);
    assert(jnet_flower_trail_touch(&trail,(float[]){NAN,0,0},5100,NULL)==0 && trail.items[0].touched==5000);
    /* A chirp reaches flowers in range, later with distance; out-of-range flowers and bad input are ignored. */
    jnet_flower_trail_clear(&trail);
    jnet_flower_trail_add(&trail,(float[]){3,0,0},1,0,1.f,100);
    jnet_flower_trail_add(&trail,(float[]){6,0,0},1,0,1.f,100);
    jnet_flower_trail_add(&trail,(float[]){30,0,0},1,0,1.f,100);
    assert(jnet_flower_trail_chirp(&trail,at,1.f,10000,NULL)==2);
    assert(trail.items[0].touched==10000+(uint32_t)(3*JNET_FLOWER_CHIRP_SPEED_MS) && trail.items[1].touched>trail.items[0].touched && !trail.items[2].touched);
    assert(jnet_flower_trail_chirp(&trail,at,40.f,20000,NULL)==3 && jnet_flower_trail_chirp(&trail,at,NAN,20000,NULL)==0 &&
           jnet_flower_trail_chirp(&trail,(float[]){0,INFINITY,0},1.f,20000,NULL)==0);
    /* The clock may wrap: elapsed time is a signed difference. */
    assert(jnet_flower_elapsed(5,0xfffffff0u)==21 && jnet_flower_elapsed(0xfffffff0u,5)==-21);
    jnet_flower_trail_clear(&trail);
    jnet_flower_trail_add(&trail,(float[]){1,0,0},1,0,1.f,0xfffff000u);
    assert(jnet_flower_trail_touch(&trail,at,0,NULL)==1 && trail.items[0].touched==1);
    /* A flower still growing from being planted is not touched, so a trail does not open as it is laid. */
    jnet_flower_trail_clear(&trail);
    jnet_flower_trail_add(&trail,(float[]){1,0,0},1,0,1.f,5000);
    assert(jnet_flower_trail_touch(&trail,at,5000+JNET_FLOWER_NEW_MS-1,NULL)==0 && jnet_flower_trail_touch(&trail,at,5000+JNET_FLOWER_NEW_MS,NULL)==1);
    /* Only a flower that is not already responding is "woken" (that is what plays the response sound); its position is reported, and
       continued contact keeps it open without waking it again until the response has ended. */
    jnet_flower_trail_clear(&trail);
    jnet_flower_trail_add(&trail,(float[]){.5f,0,.25f},1,0,1.f,0);jnet_flower_trail_add(&trail,(float[]){-.5f,0,0},2,0,1.f,0);
    float woken[3]={9,9,9};
    assert(jnet_flower_trail_touch(&trail,at,10000,woken)==2 && woken[0]==.5f && woken[2]==.25f);
    assert(jnet_flower_trail_touch(&trail,at,10016,woken)==0 && jnet_flower_trail_touch(&trail,at,13999,woken)==0);
    assert(jnet_flower_trail_touch(&trail,at,10016+JNET_FLOWER_AWAKE_MS+3984,woken)==2);
    jnet_flower_trail_clear(&trail);
    jnet_flower_trail_add(&trail,(float[]){4,0,0},1,0,1.f,0);
    assert(jnet_flower_trail_chirp(&trail,at,1.f,20000,woken)==1 && woken[0]==4.f && jnet_flower_trail_chirp(&trail,at,1.f,21000,woken)==0 &&
           jnet_flower_trail_chirp(&trail,at,1.f,20000+JNET_FLOWER_AWAKE_MS+1000,woken)==1);
    puts("Flower Trail: fl0w-only hold, 2-unit floor limit, floor placement, random assortment, ring capacity and level reset passed.");
    return 0;
}
