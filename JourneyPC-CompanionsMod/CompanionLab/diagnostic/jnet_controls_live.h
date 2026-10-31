#ifndef JNET_CONTROLS_LIVE_H
#define JNET_CONTROLS_LIVE_H
#include <xinput.h>
#include "jnet_character.h"
static int jnet_bindings[3][8]={{'C','B','V','G',0,0,0,16},{0},{0,0,0,0,0,0,0,7}};
static int jnet_bindings_saved[3][8];
static int jnet_creature_bindings[JNET_CHARACTER_COUNT][3][8],jnet_creature_saved[JNET_CHARACTER_COUNT][3][8];
static unsigned jnet_control_character,jnet_creature_down[8],jnet_attack_down;
static unsigned jnet_grab_down,jnet_fly_up_down,jnet_fly_down_down,jnet_level_down;
static unsigned jnet_binding_down[8],jnet_sprint_down,jnet_spotlight_down,jnet_crouch_active,jnet_controls_ready;
static wchar_t jnet_controls_path[MAX_PATH];
static uintptr_t (*jnet_menu_xml_original)(const char *);
static uintptr_t (*jnet_setting_original)(uintptr_t,uintptr_t,uintptr_t);
static int jnet_controls_limit(unsigned device) {return device==0?255:device==1?5:16;}
static unsigned jnet_saved_credits=1;
static LONG jnet_saved_fps;
static unsigned jnet_saved_lighting;
static void (*jnet_settings_push)(uintptr_t,const char *);
static int jnet_settings_dirty(unsigned group) {
    return group==1?jnet_credits!=jnet_saved_credits:group==2?jnet_fps_limit!=jnet_saved_fps:
        group==3?(memcmp(jnet_bindings_saved,jnet_bindings,sizeof(jnet_bindings))!=0 ||
        memcmp(jnet_creature_saved,jnet_creature_bindings,sizeof(jnet_creature_bindings))!=0):0;
}
static int jnet_settings_apply(unsigned group) {
    int bindings[3][8];memcpy(bindings,group==3?jnet_bindings:jnet_bindings_saved,sizeof(bindings));
    unsigned credits=group==1?jnet_credits:jnet_saved_credits;
    LONG fps=group==2?jnet_fps_limit:jnet_saved_fps;
    unsigned lighting=group==4?jnet_daynight_mode:jnet_saved_lighting;
    if(lighting>2)lighting=0;
    if(lighting>2)return 0;
    char data[8192];int length=snprintf(data,sizeof(data),"[Settings]\r\nCredits=%u\r\nFPS=%ld\r\nLighting=%u\r\n[Controls]\r\n",credits,(long)fps,lighting);
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<8;++action)
        length+=snprintf(data+length,sizeof(data)-length,"Binding%u%u=%d\r\n",device,action,bindings[device][action]);
    const int (*creatures)[3][8]=group==3?jnet_creature_bindings:jnet_creature_saved;
    for(unsigned kind=1;kind<JNET_CHARACTER_COUNT;++kind)if(jnet_character_is_creature(kind))
        for(unsigned device=0;device<3;++device)for(unsigned action=0;action<8;++action){
            int added=snprintf(data+length,sizeof(data)-(size_t)length,"Creature%u%u%u=%d\r\n",kind,device,action,creatures[kind][device][action]);
            if(added<0 || (size_t)added>=sizeof(data)-(size_t)length)return 0;
            length+=added;
        }
    wchar_t temporary[MAX_PATH];
    if(!jnet_controls_path[0] || wcslen(jnet_controls_path)+5>=MAX_PATH)return 0;
    swprintf(temporary,MAX_PATH,L"%ls.tmp",jnet_controls_path);
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    DWORD written=0;int ok=WriteFile(file,data,(DWORD)length,&written,NULL) && written==(DWORD)length;
    if(ok)ok=FlushFileBuffers(file);CloseHandle(file);
    if(ok)ok=MoveFileExW(temporary,jnet_controls_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok){DeleteFileW(temporary);return 0;}
    memcpy(jnet_creature_saved,creatures,sizeof(jnet_creature_saved));
    memcpy(jnet_bindings_saved,bindings,sizeof(bindings));jnet_saved_credits=credits;jnet_saved_fps=fps;jnet_saved_lighting=lighting;
    return 1;
}
static void (*jnet_quit_to_title)(void);
static int jnet_settings_action(uintptr_t menu,uintptr_t node) {
    uintptr_t begin=0,end=0;
    if(!jnet_menu_visible_native(menu) || !jnet_menu_read(menu+0xc10,&begin,8) ||
       !jnet_menu_read(menu+0xc18,&end,8))return 0;
    if(!begin || end<begin || end-begin<0x30 || (end-begin)%0x30)return 0;
    uintptr_t page=end-0x30;
    if(jnet_menu_string_is(page,"QuitToMainMenu") && jnet_menu_string_is(node,"Yes")){
        if(jnet_quit_to_title)jnet_quit_to_title();
        return 0;
    }
    int prompt=jnet_menu_string_is(page,"Back");
    if(prompt){if(end-begin<0x90)return 0;page-=0x30;}
    unsigned group=jnet_menu_string_is(page,"Audio")?1:jnet_menu_string_is(page,"Graphics")?2:jnet_menu_string_is(page,"Controls")?3:0;
    if(!group)return 0;
    if(!prompt && jnet_menu_string_is(node,"Back") && jnet_settings_dirty(group)){
        jnet_settings_push(menu,"Back");return 1;
    }
    if(prompt && jnet_menu_string_is(node,"Apply") && jnet_settings_dirty(group)){
        if(!jnet_settings_apply(group)){
            MessageBoxW(NULL,L"JNET settings could not be saved. Check that the JNET folder is writable, then try Apply again or choose Revert.",L"JNET",MB_OK|MB_ICONERROR);
            return 1;
        }
    }
    if(prompt && jnet_menu_string_is(node,"Revert")){
        if(group==1)jnet_credits=jnet_saved_credits;
        if(group==2)InterlockedExchange(&jnet_fps_limit,jnet_saved_fps);
        if(group==3){memcpy(jnet_bindings,jnet_bindings_saved,sizeof(jnet_bindings));
            memcpy(jnet_creature_bindings,jnet_creature_saved,sizeof(jnet_creature_bindings));}
    }
    return 0;
}
static void jnet_controls_initialize(void) {
    DWORD length=GetModuleFileNameW(self,jnet_controls_path,MAX_PATH);
    if(!length || length>=MAX_PATH){jnet_controls_path[0]=0;return;}
    wchar_t *slash=wcsrchr(jnet_controls_path,L'\\');
    if(!slash || slash-jnet_controls_path+24>=MAX_PATH){jnet_controls_path[0]=0;return;}
    wcscpy(slash+1,L"JNET-controls.ini");
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<8;++action){
        wchar_t key[24];swprintf(key,24,L"Binding%u%u",device,action);
        unsigned value=GetPrivateProfileIntW(L"Controls",key,jnet_bindings[device][action],jnet_controls_path);
        if(value<=(unsigned)jnet_controls_limit(device))jnet_bindings[device][action]=(int)value;
    }
    wcscpy(slash+1,L"JNET-settings.ini");
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<8;++action){
        wchar_t key[24];swprintf(key,24,L"Binding%u%u",device,action);
        unsigned value=GetPrivateProfileIntW(L"Controls",key,jnet_bindings[device][action],jnet_controls_path);
        if(value<=(unsigned)jnet_controls_limit(device))jnet_bindings[device][action]=(int)value;
    }
    for(unsigned kind=1;kind<JNET_CHARACTER_COUNT;++kind)if(jnet_character_is_creature(kind)){
        const int defaults[3][8]={{32,'C','L',0,0,'G',16,'F'},{0},{11,12,10,0,0,13,7,8}};
        for(unsigned device=0;device<3;++device)for(unsigned action=0;action<8;++action){
            wchar_t key[32];swprintf(key,32,L"Creature%u%u%u",kind,device,action);
            int fallback=action==5?jnet_bindings[device][3]:action==6?jnet_bindings[device][7]:defaults[device][action];
            unsigned value=GetPrivateProfileIntW(L"Controls",key,fallback,jnet_controls_path);
            jnet_creature_bindings[kind][device][action]=value<=(unsigned)jnet_controls_limit(device)?(int)value:fallback;
        }
    }
    memcpy(jnet_creature_saved,jnet_creature_bindings,sizeof(jnet_creature_saved));
    unsigned lighting=GetPrivateProfileIntW(L"Settings",L"Lighting",0,jnet_controls_path);
    jnet_daynight_mode=jnet_saved_lighting=lighting<=2?lighting:0;
    unsigned credits=GetPrivateProfileIntW(L"Settings",L"Credits",1,jnet_controls_path);
    unsigned fps=GetPrivateProfileIntW(L"Settings",L"FPS",0,jnet_controls_path);
    jnet_credits=jnet_saved_credits=credits<=1?credits:1;
    jnet_fps_limit=jnet_saved_fps=fps<=2?(LONG)fps:0;
    memcpy(jnet_bindings_saved,jnet_bindings,sizeof(jnet_bindings));
    jnet_settings_push=(void *)(fault_image+0x107c70);
    jnet_menu_settings_action=jnet_settings_action;
}
static uintptr_t jnet_menu_xml(const char *path) {
    if(path && !strcmp(path,"MenuSystem.xml")){
        char filename[MAX_PATH];DWORD length=GetModuleFileNameA(self,filename,MAX_PATH);
        char *slash=length && length<MAX_PATH?strrchr(filename,'\\'):NULL;
        if(slash && slash-filename+16<MAX_PATH){
            strcpy(slash+1,"JNETMenu.xml");
            uintptr_t result=jnet_menu_xml_original(filename);
            if(result){jnet_controls_ready=1;return result;}
            message("[JNET] Custom Controls menu unavailable; using original menu");
        }
    }
    return jnet_menu_xml_original(path);
}
static uintptr_t jnet_setting(uintptr_t context,uintptr_t out,uintptr_t name) {
    unsigned char object[32];char key[32];uint64_t length=0,capacity=0;uintptr_t data=0;
    if(!jnet_menu_read(name,object,sizeof(object)))return jnet_setting_original(context,out,name);
    memcpy(&length,object+16,8);memcpy(&capacity,object+24,8);
    if(length>=sizeof(key) || capacity<length)return jnet_setting_original(context,out,name);
    if(capacity<16){
        if(length>=16)return jnet_setting_original(context,out,name);
        memcpy(key,object,(size_t)length);
    }else{
        memcpy(&data,object,8);
        if(!jnet_menu_read(data,key,(size_t)length))return jnet_setting_original(context,out,name);
    }
    key[length]=0;
    if(length==13 && !memcmp(key,"%JNETCredits%",13)){*(uintptr_t *)out=(uintptr_t)&jnet_credits;return out;}
    if(length==9 && !memcmp(key,"%JNETFPS%",9)){*(uintptr_t *)out=(uintptr_t)&jnet_fps_limit;return out;}
    if(length==24 && !memcmp(key,"%JNET_CONTROL_CHARACTER%",24)){
        if(jnet_control_character>4)jnet_control_character=0;
        *(uintptr_t *)out=(uintptr_t)&jnet_control_character;return out;
    }
    if(length==21 && !memcmp(key,"%JNET_CREATURE_",15) && key[16]=='_' && key[18]=='_' && key[20]=='%'){
        unsigned kind=(unsigned)(key[15]-'0'),device=(unsigned)(key[17]-'0'),action=(unsigned)(key[19]-'0');
        if(kind<JNET_CHARACTER_COUNT && jnet_character_is_creature(kind) && device<3 && action<8){
            *(uintptr_t *)out=(uintptr_t)&jnet_creature_bindings[kind==JNET_CHARACTER_FLOW_2?JNET_CHARACTER_FLOW_1:kind][device][action];return out;
        }
    }
    if(length==15 && !memcmp(key,"%JNET_BIND_",11) && key[12]=='_' && key[14]=='%'){
        unsigned device=(unsigned)(key[11]-'0'),action=(unsigned)(key[13]-'0');
        if(device<3 && action<8){
            if(jnet_bindings[device][action]<0 || jnet_bindings[device][action]>jnet_controls_limit(device))jnet_bindings[device][action]=0;
            *(uintptr_t *)out=(uintptr_t)&jnet_bindings[device][action];return out;
        }
    }
    return jnet_setting_original(context,out,name);
}
static unsigned jnet_controls_playstation_buttons(void) {
    static const unsigned order[]={12,14,15,13,9,8,10,11,4,5,1,2,0,3,6,7};
    unsigned buttons=0;uintptr_t devices[4];
    if(!jnet_menu_read(fault_image+0x403e400,devices,sizeof(devices)))return 0;
    for(unsigned slot=0;slot<4;++slot){
        uintptr_t device=devices[slot],type=0,again=0;unsigned char connected=0,still_connected=0,state[16];
        uintptr_t entry=fault_image+0x403e400+8*slot;
        if(!device ||
           !jnet_menu_read(device,&type,8) || type!=fault_image+0x682b78 ||
           !jnet_menu_read(device+0xc5,&connected,1) || connected!=1 ||
           !jnet_menu_read(device+0x454,state,sizeof(state)) ||
           !jnet_menu_read(entry,&again,8) || again!=device ||
           !jnet_menu_read(device+0xc5,&still_connected,1) || still_connected!=1)continue;
        unsigned valid=1;
        for(unsigned i=0;i<16;++i)if(state[i]>1)valid=0;
        if(valid)for(unsigned i=0;i<16;++i)if(state[order[i]])buttons|=1u<<i;
    }
    return buttons;
}
struct jnet_controls_pad_state {unsigned buttons,motion;};
static struct jnet_controls_pad_state jnet_controls_pad(void) {
    static struct jnet_controls_pad_state sampled;
    static LONG sampled_frame;static unsigned sampled_valid;
    LONG frame=JNETFrames;
    if(sampled_valid && sampled_frame==frame)return sampled;
    sampled_valid=1;sampled_frame=frame;sampled=(struct jnet_controls_pad_state){0};
    sampled.buttons=jnet_controls_playstation_buttons();
    uintptr_t devices[4];
    if(!jnet_menu_read(fault_image+0x403e420,devices,sizeof(devices)))return sampled;
    for(unsigned i=0;i<4;++i){
        uintptr_t device=devices[i],type=0,again=0;
        unsigned index=4;unsigned char connected=0,still_connected=0;
        XINPUT_STATE state={0};
        if(!device ||
           !jnet_menu_read(device,&type,8) || type!=fault_image+0x682b10 ||
           !jnet_menu_read(device+0xc5,&connected,1) || connected!=1 ||
           !jnet_menu_read(device+0x468,&index,4) || index!=i ||
           !jnet_menu_read(device+0x46c,&state,sizeof(state)) ||
           !jnet_menu_read(fault_image+0x403e420+8*i,&again,8) || again!=device ||
           !jnet_menu_read(device+0xc5,&still_connected,1) || still_connected!=1)continue;
        static const unsigned masks[]={1,2,4,8,16,32,64,128,256,512,4096,8192,16384,32768};
        for(unsigned bit=0;bit<14;++bit)if(state.Gamepad.wButtons&masks[bit])sampled.buttons|=1u<<bit;
        if(state.Gamepad.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD)sampled.buttons|=1u<<14;
        if(state.Gamepad.bRightTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD)sampled.buttons|=1u<<15;
        sampled.motion|=state.Gamepad.sThumbLX>16000 || state.Gamepad.sThumbLX<-16000 ||
            state.Gamepad.sThumbLY>16000 || state.Gamepad.sThumbLY<-16000 ||
            state.Gamepad.sThumbRX>16000 || state.Gamepad.sThumbRX<-16000 ||
            state.Gamepad.sThumbRY>16000 || state.Gamepad.sThumbRY<-16000;
    }
    return sampled;
}
static unsigned jnet_controls_buttons(void){return jnet_controls_pad().buttons;}
static uintptr_t jnet_crouch_controller;
static unsigned jnet_controls_queuing;
static unsigned char (*jnet_controls_passive_original)(uintptr_t);
static const char *(*jnet_controls_current_action)(uintptr_t);
static unsigned char jnet_controls_passive(uintptr_t dude) {
    unsigned char native=jnet_controls_passive_original(dude);
    if(native)return native;
    const char *name=jnet_controls_current_action(dude+0xa390);
    char action[10];
    return name && own_read(NULL,(uintptr_t)name,action,sizeof(action)) &&
        !memcmp(action,"Crouching",sizeof(action));
}
static void (*jnet_controls_queue_original)(uintptr_t,const char *);
static void jnet_controls_queue(uintptr_t controller,const char *name) {
    if(jnet_crouch_active && controller==jnet_crouch_controller && !jnet_controls_queuing &&
       name){
        if(strcmp(name,"Crouch") && strcmp(name,"Crouching"))jnet_crouch_active=0;
    }
    jnet_controls_queue_original(controller,name);
}
static int jnet_controls_action(uintptr_t dude,const char *name) {
    uintptr_t controller=dude+0xa390;uint32_t count=0,pending=0;
    if(!jnet_menu_read(controller+0x64a0,&count,4) || !count || count>196 ||
       !jnet_menu_read(controller+0x6734,&pending,4) || pending>=16)return 0;
    for(unsigned i=0;i<count;++i){char candidate[24];
        if(!own_read(NULL,controller+0x33a0+i*0x40+0x28,candidate,sizeof(candidate)))return 0;
        if(memchr(candidate,0,sizeof(candidate)) && !strcmp(candidate,name)){
            void (*queue)(uintptr_t,const char *)=(void *)(fault_image+0xaea60);
            jnet_controls_queuing=1;queue(controller,name);jnet_controls_queuing=0;return 1;
        }
    }
    return 0;
}
static void jnet_controls_tick(uintptr_t barn,uintptr_t game,unsigned kind) {
    uintptr_t local=0;unsigned char menu=0;DWORD process=0;
    static unsigned resume;
    jnet_sprint_down=jnet_spotlight_down=jnet_attack_down=jnet_grab_down=0;
    jnet_fly_up_down=jnet_fly_down_down=jnet_level_down=0;
    if(!jnet_controls_ready){resume=1;return;}
    if(!own_read(NULL,game+0x1e0,&local,8) || local!=barn){resume=1;return;}
    if(jnet_menu_last && !own_read(NULL,jnet_menu_last+0xd68,&menu,1)){resume=1;return;}
    menu=menu || jnet_chat_focus;
    GetWindowThreadProcessId(GetForegroundWindow(),&process);
    static uintptr_t crouch_barn;
    if(crouch_barn!=barn || kind!=0){jnet_crouch_active=0;crouch_barn=barn;}
    jnet_crouch_controller=barn+0xb0+0xa390;
    if(menu || process!=GetCurrentProcessId()){
        resume=1;
        return;
    }
    if(jnet_crouch_active){
        float axes[2]={0};
        if(own_read(NULL,barn+0x30,axes,sizeof(axes)) && isfinite(axes[0]) && isfinite(axes[1]) &&
           (fabsf(axes[0])>.01f || fabsf(axes[1])>.01f) && jnet_controls_action(barn+0xb0,"CrouchToIdle"))jnet_crouch_active=0;
    }
    static unsigned spin_variant;
    if(kind==JNET_CHARACTER_FLOW_2)kind=JNET_CHARACTER_FLOW_1;
    const int (*bindings)[8]=jnet_character_is_creature(kind)?jnet_creature_bindings[kind]:jnet_bindings;
    unsigned buttons=0;
    for(unsigned action=0;action<(jnet_character_is_creature(kind)?8u:7u);++action)
        if(bindings[2][action]>0 && bindings[2][action]<=16){buttons=jnet_controls_buttons();break;}
    static const unsigned mouse[]={0,VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
    if(jnet_character_is_creature(kind)){
        unsigned held[8]={0};
        for(unsigned action=0;action<8;++action){
            int key=jnet_creature_bindings[kind][0][action],m=jnet_creature_bindings[kind][1][action],pad=jnet_creature_bindings[kind][2][action];
            unsigned down=(key>0 && key<256 && (GetAsyncKeyState(key)&0x8000)) ||
                (m>0 && m<6 && (GetAsyncKeyState(mouse[m])&0x8000)) ||
                (pad>0 && pad<=16 && (buttons&(1u<<(pad-1))));
            held[action]=down;
            if(resume)jnet_creature_down[action]=down;
            if(action==5)jnet_spotlight_down=held[action] && !jnet_creature_down[action];
            jnet_creature_down[action]=down;
        }
        jnet_fly_up_down=held[0];jnet_fly_down_down=held[1];jnet_level_down=held[2];
        jnet_grab_down=kind==JNET_CHARACTER_LONGTAIL && held[5];
        jnet_sprint_down=held[6];
        jnet_attack_down=kind==JNET_CHARACTER_WAR_MACHINE && held[7];
        resume=0;return;
    }
    for(unsigned action=0;action<8;++action){
        int key=jnet_bindings[0][action],m=jnet_bindings[1][action],pad=jnet_bindings[2][action];
        unsigned down=(key>0 && key<256 && (GetAsyncKeyState(key)&0x8000)) ||
            (m>0 && m<6 && (GetAsyncKeyState(mouse[m])&0x8000)) ||
            (pad>0 && pad<=16 && (buttons&(1u<<(pad-1))));
        if(resume)jnet_binding_down[action]=down;
        if(action==3)jnet_spotlight_down=down && !jnet_binding_down[action];
        if(kind==0){
            static const char *const names[]={"Crouch","SlideJump2","SlideJump1",NULL,"RollF","JNETFlipL","JNETFlipR"};
            if(action<7 && action!=3 && down && !jnet_binding_down[action]){
                const char *name=action==0 && jnet_crouch_active?"CrouchToIdle":
                    action==2?(spin_variant?"JNETSpin3":"SlideJump1"):names[action];
                int queued=jnet_controls_action(barn+0xb0,name);
                if(action==2 && queued)spin_variant=!spin_variant;
                if(queued)jnet_crouch_active=action==0?!jnet_crouch_active:0;
            }
        }
        jnet_binding_down[action]=down;
    }
    resume=0;
}
#endif
