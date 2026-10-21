static void (*jnet_character_eye_original)(uintptr_t,uintptr_t);
static void jnet_character_eye(uintptr_t event,uintptr_t game) {
    struct jnet_character_state *instance=NULL;unsigned active=0;
    for(unsigned owner=0;owner<3;++owner){
        if(jnet_character_instances[owner].eye_event==event){instance=&jnet_character_instances[owner];active=1;break;}
        for(unsigned skin=0;skin<3;++skin)
            if(jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*skin].eye_event==event)
                instance=&jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*skin];
    }
    if(!instance){jnet_character_eye_original(event,game);return;}
    if(instance->game!=game || !jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation))return;
    if(!active || !instance->initialized || instance->failed) {
        uint64_t sound=0,handle=0;
        if(own_read(NULL,game+0x150,&sound,8) && sound && own_read(NULL,event+0xa4,&handle,8)) {
            void (*volume)(uintptr_t,uint64_t,float,float)=(void *)(fault_image+0x1468e0);
            volume(sound,handle,0,0);
        }
        return;
    }
    const struct jnet_character_spec *spec=&jnet_character_specs[instance->kind];
    unsigned checked=0;
    for(unsigned i=0;i<spec->mesh_count;++i){
        const char *name=spec->meshes[i].mesh;
        if(strcmp(name,"JNETCharacter_P_MechHead") && strcmp(name,"JNETCharacter_P_MechHeadGlowMask"))continue;
        uint64_t mesh=instance->pools.decorations+0x10+(uint64_t)(instance->pools.decoration_count+i)*0x10d0;
        uint64_t states=0;uint32_t count=0;unsigned char first[80];
        if(!own_read(NULL,mesh+0x588,&states,8) || !states ||
           !own_read(NULL,mesh+0x590,&count,4) || !count || count>64 ||
           !own_read(NULL,states,first,sizeof(first))){
            if(!instance->eye_warned){message("[JNET] spotlight waiting for native eyelid animation storage");instance->eye_warned=1;}
            instance->eye_ready=0;return;
        }
        ++checked;
    }
    if(checked!=2)return;
    unsigned requested=active && instance->initialized && !instance->failed && instance->control.spotlight;
    if(!instance->attack_busy && !jc_local_windows_write(event+0x7c,&requested,4))return;
    instance->eye_ready=1;
    jnet_character_eye_original(event,game);
}
