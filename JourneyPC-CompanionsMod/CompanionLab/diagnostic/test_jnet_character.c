#include <assert.h>
#include <stdio.h>
#include "jnet_character.h"
#include "jnet_character_audio.h"
static int near(float a,float b){return fabsf(a-b)<0.0001f;}
int main(void) {
    assert(JNET_CHARACTER_ROCKET==4 && JNET_CHARACTER_WAR_MACHINE==2 && JNET_CHARACTER_WHALE==3);
    assert(!strcmp(jnet_character_name(JNET_CHARACTER_FLOW_1),"fl0w 1"));
    assert(!strcmp(jnet_character_name(JNET_CHARACTER_FLOW_2),"fl0w 2"));
    for(unsigned kind=0;kind<=JNET_CHARACTER_COUNT;++kind)
        assert(jnet_character_is_creature(kind)==(kind==1 || kind==2 || kind==3 || kind==5 || kind==6));
    struct jnet_character_animation_dependency animations[]={
        {"swim",1u<<JNET_CHARACTER_LONGTAIL,0},{"flip",1u<<JNET_CHARACTER_LONGTAIL,0},
        {"wing",1u<<JNET_CHARACTER_WHALE,0}};
    assert(jnet_character_animation_blocked(animations,3,"swim",0)==(1u<<JNET_CHARACTER_LONGTAIL));
    assert(jnet_character_animation_blocked(animations,3,"flip",0)==(1u<<JNET_CHARACTER_LONGTAIL));
    assert(jnet_character_animation_blocked(animations,3,"swim",1)==(1u<<JNET_CHARACTER_LONGTAIL));
    assert(jnet_character_animation_blocked(animations,3,"wing",0)==((1u<<JNET_CHARACTER_LONGTAIL)|(1u<<JNET_CHARACTER_WHALE)));
    assert(jnet_character_animation_blocked(animations,3,"flip",1)==(1u<<JNET_CHARACTER_WHALE));
    assert(!jnet_character_animation_blocked(animations,3,"wing",1));
    assert(near(jnet_character_speed(JNET_CHARACTER_LONGTAIL),10.8f));
    assert(near(jnet_character_speed(JNET_CHARACTER_FLOW_1),10.8f));
    assert(near(jnet_character_speed(JNET_CHARACTER_FLOW_2),10.8f));
    assert(near(jnet_character_speed(JNET_CHARACTER_WHALE),8.28f));
    assert(near(jnet_character_speed(JNET_CHARACTER_WAR_MACHINE),8.28f));
    for(unsigned kind=0;kind<JNET_CHARACTER_COUNT;++kind){
        float base=jnet_character_speed(kind);
        assert(near(jnet_character_move_speed(kind,0),base));
        assert(near(jnet_character_move_speed(kind,1),base*(jnet_character_is_creature(kind)?2.f:1.f)));
    }
    for(unsigned kind=0;kind<=JNET_CHARACTER_COUNT;++kind)
        for(unsigned charge=0;charge<=5;++charge)
            assert(jnet_character_large_chirp(kind,charge)==
                (charge==4 && (kind==JNET_CHARACTER_WHALE || kind==JNET_CHARACTER_WAR_MACHINE)));
    float camera=0;
    assert(jnet_character_camera_distance(JNET_CHARACTER_LONGTAIL,8,&camera) && near(camera,10));
    assert(jnet_character_camera_distance(JNET_CHARACTER_FLOW_1,8,&camera) && near(camera,10));
    assert(jnet_character_camera_distance(JNET_CHARACTER_FLOW_2,8,&camera) && near(camera,10));
    assert(jnet_character_camera_distance(JNET_CHARACTER_WHALE,8,&camera) && near(camera,40));
    assert(jnet_character_camera_distance(JNET_CHARACTER_WAR_MACHINE,8,&camera) && near(camera,40));
    assert(!jnet_character_camera_distance(JNET_CHARACTER_WAYFARER,8,&camera));
    assert(!jnet_character_camera_distance(JNET_CHARACTER_COUNT,8,&camera));
    assert(!jnet_character_camera_distance(JNET_CHARACTER_LONGTAIL,NAN,&camera));
    assert(!jnet_character_camera_distance(JNET_CHARACTER_LONGTAIL,-1,&camera));
    struct jnet_character_control control={0};
    struct jnet_character_input in={.forward=1,.camera_forward={0,3,4},.camera_right={1,0,0},.enabled=1};
    struct jnet_character_intent out;
    for(unsigned kind=JNET_CHARACTER_LONGTAIL;kind<JNET_CHARACTER_COUNT;++kind){
        assert(jnet_character_select(&control,kind,0));
        assert(jnet_character_control_step(&control,&in,&out));
        assert(near(out.direction[1],0.6f) && near(out.direction[2],0.8f) && near(out.strength,1));
        in.camera_forward[1]=-3;
        assert(jnet_character_control_step(&control,&in,&out) && near(out.direction[1],-0.6f));
        in.forward=0;
        assert(jnet_character_control_step(&control,&in,&out) && !out.strength);
        assert(!out.direction[0] && !out.direction[1] && !out.direction[2]);
        in.forward=1;in.strafe=1;
        assert(jnet_character_control_step(&control,&in,&out) && near(out.strength,1));
        assert(near(out.direction[0]*out.direction[0]+out.direction[1]*out.direction[1]+out.direction[2]*out.direction[2],1));
        in.strafe=0;in.camera_forward[1]=3;
    }
    for(unsigned kind=JNET_CHARACTER_LONGTAIL;kind<JNET_CHARACTER_COUNT;++kind){
        assert(jnet_character_select(&control,kind,0));
        in.forward=0;in.fly=1;
        assert(jnet_character_control_step(&control,&in,&out));
        assert(near(out.direction[1],1) && near(out.strength,1) && !out.spotlight_changed);
        in.forward=1;in.camera_forward[1]=-3;
        assert(jnet_character_control_step(&control,&in,&out) && out.direction[1]>0);
        float norm=0;for(unsigned i=0;i<3;++i)norm+=out.direction[i]*out.direction[i];
        assert(near(norm,1) && !out.spotlight_changed);
        in.fly=0;
        assert(jnet_character_control_step(&control,&in,&out) && out.direction[1]<0);
        in.forward=.55f;
        assert(jnet_character_control_step(&control,&in,&out) && near(out.strength,.5f));
        in.forward=1;in.camera_forward[1]=3;
    }
    assert(jnet_character_select(&control,JNET_CHARACTER_WAR_MACHINE,0));
    in.interact=1;
    assert(jnet_character_control_step(&control,&in,&out) && out.spotlight && out.spotlight_changed);
    assert(jnet_character_control_step(&control,&in,&out) && out.spotlight && !out.spotlight_changed);
    in.interact=0;assert(jnet_character_control_step(&control,&in,&out));
    in.interact=1;assert(jnet_character_control_step(&control,&in,&out) && !out.spotlight && out.spotlight_changed);
    in.enabled=0;in.interact=0;assert(jnet_character_control_step(&control,&in,&out) && !out.strength);
    in.interact=1;assert(jnet_character_control_step(&control,&in,&out) && !out.spotlight_changed);
    in.enabled=1;assert(jnet_character_control_step(&control,&in,&out) && !out.spotlight_changed);
    in.camera_forward[1]=NAN;
    assert(!jnet_character_control_step(&control,&in,&out) && !out.strength);
    in.camera_forward[1]=3;
    memcpy(in.camera_right,in.camera_forward,sizeof(in.camera_right));
    assert(!jnet_character_control_step(&control,&in,&out));
    assert(jnet_character_select(&control,JNET_CHARACTER_WAYFARER,1));
    assert(jnet_character_control_step(&control,&in,&out) && !out.strength && !out.spotlight);
    assert(!jnet_character_select(&control,JNET_CHARACTER_COUNT,0));
    struct jnet_character_pose pose={{0,10000,0},{0,0,0,1}},next;
    struct jnet_character_intent motion={.direction={0,1,0},.strength=1};
    assert(jnet_character_motion_step(&pose,&motion,20,0.025f,&next));
    assert(near(next.position[1],10000.5f));
    float *q=next.rotation;
    assert(near(2*(q[0]*q[2]+q[3]*q[1]),0));
    assert(2*(q[1]*q[2]-q[3]*q[0])>0 && 2*(q[1]*q[2]-q[3]*q[0])<.2f);
    assert(1-2*(q[0]*q[0]+q[1]*q[1])>.9f);
    motion.direction[1]=-1;
    assert(jnet_character_motion_step(&pose,&motion,20,10,&next));
    assert(near(next.position[1],9999));
    motion.strength=0;
    assert(jnet_character_motion_step(&pose,&motion,20,10,&next));
    assert(!memcmp(&pose,&next,sizeof(pose)));
    struct jnet_character_pose unchanged=next;
    motion.strength=1;motion.direction[1]=NAN;
    assert(!jnet_character_motion_step(&pose,&motion,20,.01f,&next));
    assert(!memcmp(&unchanged,&next,sizeof(next)));
    assert(!jnet_character_motion_step(&pose,&motion,20,-1,&next));
    float filtered[3]={0,0,1};
    struct jnet_character_intent requested={.direction={0,1,0},.strength=1};
    assert(jnet_character_smooth(filtered,&requested,1.f/60,1));
    assert(requested.direction[1]>0 && requested.direction[1]<.1f);
    for(unsigned i=0;i<120;++i){
        requested=(struct jnet_character_intent){.direction={0,1,0},.strength=1};
        assert(jnet_character_smooth(filtered,&requested,1.f/60,1));
        assert(requested.strength<=1);
    }
    assert(requested.direction[1]>.999f);
    assert(jnet_character_smooth(filtered,&requested,.016f,0) && !requested.strength);
    requested.direction[0]=NAN;assert(!jnet_character_smooth(filtered,&requested,.016f,1));
    struct jnet_character_control flight={.kind=JNET_CHARACTER_WHALE};
    struct jnet_character_input flight_input={.camera_forward={0,.8f,.6f},.camera_right={1,0,0},.enabled=1,.forward=1,.level=1};
    struct jnet_character_intent flight_intent;
    assert(jnet_character_control_step(&flight,&flight_input,&flight_intent));
    assert(near(flight_intent.direction[1],0) && near(flight_intent.direction[2],1));
    flight_input.forward=0;flight_input.fly=1;
    assert(jnet_character_control_step(&flight,&flight_input,&flight_intent));
    float up_motion[3]={0},down_motion[3]={0};
    assert(jnet_character_smooth(up_motion,&flight_intent,.016f,1));
    flight_input.fly=0;flight_input.fly_down=1;
    assert(jnet_character_control_step(&flight,&flight_input,&flight_intent));
    assert(jnet_character_smooth(down_motion,&flight_intent,.016f,1));
    assert(near(up_motion[1],-down_motion[1]));
    struct jnet_character_pose banked={.rotation={0,0,0,1}};
    flight_intent=(struct jnet_character_intent){.roll=1.57079632679f};
    for(unsigned i=0;i<240;++i){
        assert(jnet_character_motion_step(&banked,&flight_intent,10,.016f,&next));banked=next;
    }
    assert(near(banked.position[0],0) && near(banked.position[1],0));
    assert(fabsf(banked.rotation[2]-.70710678f)<.001f);
    flight_intent.roll=0;
    for(unsigned i=0;i<240;++i){assert(jnet_character_motion_step(&banked,&flight_intent,10,.016f,&next));banked=next;}
    assert(fabsf(banked.rotation[2])<.001f);
    char path[256];
    assert(jnet_character_bank_path("C:/Journey/Data/Sounds/JNTBioSummit-SFX.bnk",path,sizeof(path)));
    assert(!strcmp(path,"C:/Journey/Data/Sounds/LvlSfxSummit-SFX.bnk"));
    assert(!jnet_character_bank_path("Data/Sounds/LvlSfxCave-SFX.bnk",path,sizeof(path)));
    assert(!jnet_character_bank_path("Data/Sounds/JNTBioSummit-SFX.bnk.streams",path,sizeof(path)));
    assert(!jnet_character_bank_path("Data/Sounds/JNTBioSummit-SFX.bnk",path,4));
    assert(jnet_character_bank_path("Data/Sounds/JNTFlowRuins-SFX.bnk",path,sizeof(path)));
    assert(!strcmp(path,"Data/Sounds/LvlSfxRuins-SFX.bnk"));
    for(unsigned i=0;i<10;++i){
        assert(jnet_character_chirp_variant(1,i,1)!=jnet_character_chirp_variant(1,i+1,1));
        assert(jnet_character_chirp_variant(2,i,4)!=jnet_character_chirp_variant(2,i+1,4));
        assert(jnet_character_chirp_variant(3,i,4)==18);
        assert(jnet_character_chirp_variant(5,i,4)==21+i%2);
        assert(jnet_character_chirp_variant(6,i,1)==21+i%2);
    }
    puts("Character control intent: camera ascent/dive, neutral hover request, diagonal speed, spotlight edges and invalid input passed. Native creature playback not exercised.");
}
