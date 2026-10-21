static uintptr_t jnet_chapter_menu,jnet_chapter_game;
static void (*jnet_menu_frame_original)(uintptr_t,uintptr_t);
static void jnet_menu_frame(uintptr_t menu,uintptr_t game) {
    jnet_chapter_menu=menu;jnet_chapter_game=game;
    jnet_menu_frame_original(menu,game);
}
static int jnet_travel_playable_menu(void) {
    uintptr_t type=0;unsigned char flags[2]={0};
    return jnet_chapter_game && jnet_chapter_menu &&
        own_read(NULL,jnet_chapter_menu,&type,8) && type==fault_image+0x679c18 &&
        own_read(NULL,jnet_chapter_menu+0x20,flags,sizeof(flags)) && !flags[0] && flags[1]==1;
}
static int jnet_travel_entry_chapter(unsigned level,unsigned restart,unsigned chapter_select,int playable) {
    return chapter_select || (!level && !restart && !playable);
}
static int jnet_travel_menu_prepare(uintptr_t game,unsigned chapter_select) {
    uintptr_t save=0,type=0,menu=jnet_chapter_menu;
    unsigned char snapshot[0x1ba8],menu_state[0xf0],spawn[12];
    uint64_t zero=0;unsigned char enabled=1;float fade;
    if(game!=jnet_chapter_game || !menu ||
       !own_read(NULL,menu,menu_state,sizeof(menu_state)))return 0;
    memcpy(&type,menu_state,8);if(type!=fault_image+0x679c18)return 0;
    if(!chapter_select)return jc_local_windows_write(menu+0x20,&zero,1) &&
        jc_local_windows_write(menu+0x21,&enabled,1);
    if(!own_read(NULL,game+0x170,&save,8) || !save ||
       !own_read(NULL,save,&type,8) || type!=fault_image+0x5e3178 ||
       !own_read(NULL,save+8,snapshot,sizeof(snapshot)) ||
       !own_read(NULL,game+0x1c0374,spawn,sizeof(spawn)))return 0;

    uintptr_t target=fault_image+0x5d8c58,black=fault_image+0x5dfe64;
    if(!own_read(NULL,fault_image+0x68b7f0,&fade,4))return 0;
    for(unsigned i=0;i<13;++i)
        if(!jc_local_windows_write(save+0x60+i*0x158,&zero,8))return 0;
    return jc_local_windows_write(save+0x1590,&zero,4) &&
        jc_local_windows_write(save+0x50,&enabled,1) &&
        jc_local_windows_write(menu+0x20,&zero,1) &&
        jc_local_windows_write(menu+0x21,&enabled,1) &&
        jc_local_windows_write(menu+0xe8,&zero,4) &&
        jc_local_windows_write(menu+0xe0,&black,8) &&
        jc_local_windows_write(game+0x1c0374,&zero,8) &&
        jc_local_windows_write(game+0x1c037c,&zero,4) &&
        jc_local_windows_write(fault_image+0x3772ae0,&target,8) &&
        jc_local_windows_write(fault_image+0x403e290,&fade,4);
}
