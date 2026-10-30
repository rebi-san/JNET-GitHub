#ifndef JC_TRAVELER_CHIRP_GLYPH_H
#define JC_TRAVELER_CHIRP_GLYPH_H
typedef void (JC_GLYPH_ABI *jc_glyph_update_fn)(uintptr_t,float,uintptr_t,uintptr_t,
    uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,unsigned char,uintptr_t,uintptr_t);
static jc_glyph_update_fn original_glyph_update;
typedef uintptr_t (JC_GLYPH_ABI *jc_chirp_play_fn)(uintptr_t,uint64_t *,const char *,const char *,
    const float *,float,float,float,unsigned char,float,unsigned char,unsigned char);
static jc_chirp_play_fn traveler_chirp_play;
static int traveler_chirp_audio_fallback(uintptr_t sounds,uintptr_t event,const char *name,
        float near_distance,float far_distance,uint64_t *handle) {
    unsigned kind=0;uint32_t count=0;
    if(!traveler_chirp_play || !isfinite(near_distance) || !isfinite(far_distance) ||
       near_distance<0 || far_distance<=near_distance)return 0;
    for(unsigned i=1;i<=4;++i)if(!strcmp(name,jc_chirp_sound_name(i,1)))kind=i;
    if(!kind || !own_read(NULL,sounds+0x7c7b4,&count,4) || count>JC_CHIRP_SOUND_CAP)return 0;
    for(unsigned i=0;i<count;++i){
        unsigned char descriptor[0x13c];float position[4];
        if(!own_read(NULL,sounds+0x70234+(uint64_t)i*0x13c,descriptor,sizeof(descriptor)))return 0;
        if(!memchr(descriptor,0,32) || strcmp((char *)descriptor,jc_chirp_sound_name(kind,0)))continue;
        if(!descriptor[0x20] || !memchr(descriptor+0x20,0,9) ||
           !descriptor[0x29] || !memchr(descriptor+0x29,0,0x100) ||
           !own_read(NULL,event+0x10,position,sizeof(position)))return 0;
        for(unsigned k=0;k<3;++k)if(!isfinite(position[k]))return 0;
        position[3]=1;
        float parameter=0;memcpy(&parameter,descriptor+0x134,4);
        if(!isfinite(parameter))return 0;
        uint64_t replacement=UINT64_MAX;
        traveler_chirp_play(sounds,&replacement,(char *)descriptor+0x20,(char *)descriptor+0x29,
            position,near_distance,far_distance,1,descriptor[0x138],parameter,0,0);
        uint32_t voice=(uint32_t)replacement,serial=0;unsigned char active=0;
        if(voice>=512 || !own_read(NULL,sounds+(uint64_t)voice*0x200+0x214,&serial,4) ||
           serial!=(uint32_t)(replacement>>32) ||
           !own_read(NULL,sounds+(uint64_t)voice*0x200+0x218,&active,1) || !active)return 0;
        if(!jc_local_windows_write(event+0x60,&replacement,8))return 0;
        *handle=replacement;return 1;
    }
    return 0;
}
static struct {
    uint64_t event;
    uint32_t counter;
} traveler_chirp_audio[JC_CHIRP_SLOTS];
static void traveler_chirp_audio_mark(const struct jc_chirp_bridge *bridge) {
    if(bridge->slot>=JC_CHIRP_SLOTS)return;
    traveler_chirp_audio[bridge->slot].event=bridge->barn+0x10+bridge->slot*0x70;
    traveler_chirp_audio[bridge->slot].counter=bridge->counter;
}
static struct {
    uint64_t event,peer,generation;
    uint32_t counter;
} traveler_chirp_pending[JC_CHIRP_SLOTS];
static struct {
    uint64_t event,actor,glyph,peer,generation;
    uint32_t counter;
    unsigned valid,routed;
} traveler_chirp_glyph;
static void traveler_chirp_glyph_mark(const struct jc_chirp_bridge *bridge) {
    if(!traveler_actor || !jc_actor_runtime_thread(traveler_actor) || bridge->slot>=JC_CHIRP_SLOTS)return;
    const struct jc_actor_identity *id=&traveler_actor->identity;
    traveler_chirp_pending[bridge->slot].event=bridge->barn+0x10+bridge->slot*0x70;
    traveler_chirp_pending[bridge->slot].counter=bridge->counter;
    traveler_chirp_pending[bridge->slot].peer=id->peer;
    traveler_chirp_pending[bridge->slot].generation=id->generation;
    traveler_chirp_glyph.event=bridge->barn+0x10+bridge->slot*0x70;
    traveler_chirp_glyph.counter=bridge->counter;
    traveler_chirp_glyph.actor=id->actor;traveler_chirp_glyph.glyph=id->glyph;
    traveler_chirp_glyph.peer=id->peer;traveler_chirp_glyph.generation=id->generation;
    traveler_chirp_glyph.valid=1;
}
static void JC_GLYPH_ABI traveler_glyph_update(uintptr_t event,float dt,uintptr_t barn,uintptr_t camera,
        uintptr_t a5,uintptr_t a6,uintptr_t sounds,uintptr_t a8,uintptr_t a9,
        uintptr_t terrain,uintptr_t physics,unsigned char display,uintptr_t a13,uintptr_t chirps) {
    if(traveler_chirp_glyph.valid && event==traveler_chirp_glyph.event && traveler_actor &&
       traveler_object && traveler_actor->state>=JC_ACTOR_RUNTIME_ACTIVE && traveler_actor->state<=JC_ACTOR_RUNTIME_RETIRED &&
       jc_actor_runtime_thread(traveler_actor) && traveler_context()) {
        const struct jc_actor_identity *id=&traveler_actor->identity;
        uint32_t counter=0;unsigned char flags[2]={1,1},active=0;uint64_t glyph=0,pool=0;
        if(id->actor==traveler_chirp_glyph.actor && id->actor==traveler_object &&
           id->glyph==traveler_chirp_glyph.glyph && id->peer==traveler_chirp_glyph.peer &&
           id->generation==traveler_chirp_glyph.generation && barn==id->barn &&
           own_read(NULL,id->game+0x1b8,&pool,8) && pool==chirps &&
           event>=pool+0x10 && event<pool+0x710 && (event-pool-0x10)%0x70==0 &&
           own_read(NULL,event+0x40,&counter,4) && counter==traveler_chirp_glyph.counter &&
           own_read(NULL,event+0x3c,flags,2) && !flags[0] && !flags[1] &&
           own_read(NULL,event+0x58,&active,1) && active==1 &&
           own_read(NULL,id->actor+0x2240,&glyph,8) && glyph==id->glyph &&
           !jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials)) {
            barn=id->actor-0xb0;display=1;
            if(++traveler_chirp_glyph.routed<=4 || traveler_chirp_glyph.routed%300==0) {
                char line[160];snprintf(line,sizeof(line),"[JNET] chirp glyph routed=%u event=%u own-symbol=1",
                    traveler_chirp_glyph.routed,counter);message(line);
            }
        }
    }
    int audio_slot=-1;uint32_t counter=0,index=0,count=0;unsigned char first=0,available=0;
    char name[32]={0};
    if(own_read(NULL,event+0x68,&first,1) && first &&
       own_read(NULL,event+0x40,&counter,4)){
        for(unsigned i=0;i<JC_CHIRP_SLOTS;++i)
            if(traveler_chirp_audio[i].event==event && traveler_chirp_audio[i].counter==counter){audio_slot=(int)i;break;}
        if(audio_slot>=0 && own_read(NULL,event+0x38,&index,4) &&
           own_read(NULL,sounds+0x7c7b4,&count,4) && index<count && count<=JC_CHIRP_SOUND_CAP){
            own_read(NULL,sounds+0x70234+(uint64_t)index*0x13c,name,sizeof(name)-1);
            own_read(NULL,sounds+0x7025d+(uint64_t)index*0x13c,&available,1);
        }
    }
    original_glyph_update(event,dt,barn,camera,a5,a6,sounds,a8,a9,terrain,physics,display,a13,chirps);
    if(audio_slot>=0){
        uint64_t handle=UINT64_MAX;unsigned char pending=1;
        int read=own_read(NULL,event+0x60,&handle,8) && own_read(NULL,event+0x68,&pending,1);
        uint32_t voice=(uint32_t)handle,serial=0,backend=0;unsigned char active=0;
        float gain=-1,target=-1,near_distance=-1,far_distance=-1;
        int voice_read=read && voice<512 &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x214,&serial,4) &&
            serial==(uint32_t)(handle>>32) &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x218,&active,1) &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x1e0,&backend,4) &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x16c,&gain,4) &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x1ec,&target,4) &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x208,&near_distance,4) &&
            own_read(NULL,sounds+(uint64_t)voice*0x200+0x20c,&far_distance,4);
        int fallback=voice_read && !active && traveler_chirp_audio_fallback(sounds,event,name,near_distance,far_distance,&handle);
        static unsigned audio_reports,last_audio_state=~0u;
        unsigned audio_state=(unsigned)voice_read | ((unsigned)active<<1) | ((unsigned)fallback<<2);
        if(++audio_reports<=4 || audio_reports%32==0 || audio_state!=last_audio_state){
            char line[384];snprintf(line,sizeof(line),"[JNET] received chirp audio event=%u sound=%s available=%u read=%d handle=%llu pending=%u voice-read=%d active=%u backend=%u gain=%.4g target=%.4g distance=%.4g/%.4g fallback-active=%d",
                counter,name,(unsigned)available,read,(unsigned long long)handle,(unsigned)pending,
                voice_read,(unsigned)active,backend,gain,target,near_distance,far_distance,fallback);message(line);
        }
        last_audio_state=audio_state;
        traveler_chirp_audio[audio_slot].event=0;
    }
}
#endif
