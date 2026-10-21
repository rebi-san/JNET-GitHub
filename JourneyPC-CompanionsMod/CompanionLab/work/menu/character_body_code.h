static int jnet_character_restore_wayfarer(struct jnet_character_state *instance) {
    uint64_t type=0;
    return own_read(NULL,instance->barn+0xb0,&type,8) && type==fault_image+0x5d0380 &&
        jc_local_windows_write(instance->barn+0xb0+0x1e00,&instance->saved_dissolve[0],4) &&
        jc_local_windows_write(instance->barn+0xb0+0x2000,&instance->saved_dissolve[1],4) &&
        jc_local_windows_write(instance->barn+0xb0+0x9c34,&instance->saved_visibility[0],1) &&
        jc_local_windows_write(instance->barn+0xb0+0x9cc4,&instance->saved_visibility[1],1);
}
static struct jnet_character_state *jnet_character_creature_for_dude(uintptr_t dude) {
    for(unsigned owner=0;owner<3;++owner){
        struct jnet_character_state *instance=&jnet_character_instances[owner];
        if(instance->initialized && !instance->failed && instance->binding.creature &&
           dude==instance->barn+0xb0)return instance;
    }
    return NULL;
}
static void (*jnet_character_body_input_original)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,unsigned char);
static void jnet_character_body_input(uintptr_t dude,uintptr_t game,uintptr_t axes,uintptr_t direction,uintptr_t peer,unsigned char enabled) {
    struct jnet_character_state *instance=jnet_character_creature_for_dude(dude);
    if(instance && instance->initialized && !instance->failed && instance->game==game &&
       dude==instance->barn+0xb0 &&
       jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation)){
        float zero[4]={0},dt=0;uintptr_t clock=0;unsigned char clear=0;
        if(!own_read(NULL,game+0x68,&clock,8) || !clock || !own_read(NULL,clock+0x10,&dt,4) ||
           !jnet_character_vortex_decay(own_read,jnet_character_write_memory,NULL,dude,dt) ||
           !jc_local_windows_write(dude+0x1bd4,&clear,1) ||
           !jc_local_windows_write(dude+0x20,zero,16) ||
           !jc_local_windows_write(dude+0x34,zero,4) ||
           !jc_local_windows_write(dude+0x3c,zero,4) ||
           !jc_local_windows_write(dude+0x1c00,zero,16))jnet_character_fail(instance);
        return;
    }
    jnet_character_body_input_original(dude,game,axes,direction,peer,enabled);
}
