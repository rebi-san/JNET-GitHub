#ifndef JNET_CONTROLS_LIVE_H
#define JNET_CONTROLS_LIVE_H
#include <xinput.h>
static int jnet_bindings[3][7]={{'C','B','V','G'},{0},{0}};
static int jnet_bindings_saved[3][7];
static unsigned jnet_binding_down[7],jnet_spotlight_down,jnet_crouch_active,jnet_controls_ready;
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
        group==3?memcmp(jnet_bindings_saved,jnet_bindings,sizeof(jnet_bindings))!=0:0;
}
static int jnet_settings_apply(unsigned group) {
    int bindings[3][7];memcpy(bindings,group==3?jnet_bindings:jnet_bindings_saved,sizeof(bindings));
    unsigned credits=group==1?jnet_credits:jnet_saved_credits;
    LONG fps=group==2?jnet_fps_limit:jnet_saved_fps;
    unsigned lighting=group==4?jnet_daynight_mode:jnet_saved_lighting;
    if(lighting>2)return 0;
    char data[1024];int length=snprintf(data,sizeof(data),"[Settings]\r\nCredits=%u\r\nFPS=%ld\r\nLighting=%u\r\n[Controls]\r\n",credits,(long)fps,lighting);
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<7;++action)
        length+=snprintf(data+length,sizeof(data)-length,"Binding%u%u=%d\r\n",device,action,bindings[device][action]);
    wchar_t temporary[MAX_PATH];
    if(!jnet_controls_path[0] || wcslen(jnet_controls_path)+5>=MAX_PATH)return 0;
    swprintf(temporary,MAX_PATH,L"%ls.tmp",jnet_controls_path);
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    DWORD written=0;int ok=WriteFile(file,data,(DWORD)length,&written,NULL) && written==(DWORD)length;
    if(ok)ok=FlushFileBuffers(file);CloseHandle(file);
    if(ok)ok=MoveFileExW(temporary,jnet_controls_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok){DeleteFileW(temporary);return 0;}
    memcpy(jnet_bindings_saved,bindings,sizeof(bindings));jnet_saved_credits=credits;jnet_saved_fps=fps;jnet_saved_lighting=lighting;
    return 1;
}
static int jnet_settings_action(uintptr_t menu,uintptr_t node) {
    uintptr_t begin=0,end=0;
    if(!jnet_menu_visible_native(menu) || !jnet_menu_read(menu+0xc10,&begin,8) ||
       !jnet_menu_read(menu+0xc18,&end,8))return 0;
    uintptr_t page=end-0x30;
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
        if(group==3)memcpy(jnet_bindings,jnet_bindings_saved,sizeof(jnet_bindings));
    }
    return 0;
}
static void jnet_controls_initialize(void) {
    DWORD length=GetModuleFileNameW(self,jnet_controls_path,MAX_PATH);
    if(!length || length>=MAX_PATH){jnet_controls_path[0]=0;return;}
    wchar_t *slash=wcsrchr(jnet_controls_path,L'\\');
    if(!slash || slash-jnet_controls_path+24>=MAX_PATH){jnet_controls_path[0]=0;return;}
    wcscpy(slash+1,L"JNET-controls.ini");
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<7;++action){
        wchar_t key[24];swprintf(key,24,L"Binding%u%u",device,action);
        unsigned value=GetPrivateProfileIntW(L"Controls",key,jnet_bindings[device][action],jnet_controls_path);
        if(value<=(unsigned)jnet_controls_limit(device))jnet_bindings[device][action]=(int)value;
    }
    wcscpy(slash+1,L"JNET-settings.ini");
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<7;++action){
        wchar_t key[24];swprintf(key,24,L"Binding%u%u",device,action);
        unsigned value=GetPrivateProfileIntW(L"Controls",key,jnet_bindings[device][action],jnet_controls_path);
        if(value<=(unsigned)jnet_controls_limit(device))jnet_bindings[device][action]=(int)value;
    }
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
    if(jnet_menu_string_is(name,"%JNETCredits%")){*(uintptr_t *)out=(uintptr_t)&jnet_credits;return out;}
    if(jnet_menu_string_is(name,"%JNETFPS%")){*(uintptr_t *)out=(uintptr_t)&jnet_fps_limit;return out;}
    for(unsigned device=0;device<3;++device)for(unsigned action=0;action<7;++action){
        char key[32];snprintf(key,sizeof(key),"%%JNET_BIND_%u_%u%%",device,action);
        if(jnet_menu_string_is(name,key)){
            if(jnet_bindings[device][action]<0 || jnet_bindings[device][action]>jnet_controls_limit(device))jnet_bindings[device][action]=0;
            *(uintptr_t *)out=(uintptr_t)&jnet_bindings[device][action];return out;
        }
    }
    return jnet_setting_original(context,out,name);
}
static unsigned jnet_controls_buttons(void) {
    static DWORD (WINAPI *get_state)(DWORD,XINPUT_STATE *);
    static unsigned checked;
    if(!checked){
        checked=1;
        const wchar_t *libraries[]={L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"};
        for(unsigned i=0;i<3 && !get_state;++i){HMODULE module=GetModuleHandleW(libraries[i]);
            if(!module)module=LoadLibraryW(libraries[i]);
            if(module)get_state=(void *)GetProcAddress(module,"XInputGetState");}
    }
    static unsigned connected;static uint64_t scan_at;
    uint64_t now=GetTickCount64();unsigned scan=now>=scan_at;
    if(scan)scan_at=now+1000;
    unsigned buttons=0;
    if(get_state)for(unsigned i=0;i<4;++i){
        if(!scan && !(connected&(1u<<i)))continue;
        XINPUT_STATE state={0};
        if(get_state(i,&state)==ERROR_SUCCESS){
            connected|=1u<<i;
            static const unsigned masks[]={1,2,4,8,16,32,64,128,256,512,4096,8192,16384,32768};
            for(unsigned bit=0;bit<14;++bit)if(state.Gamepad.wButtons&masks[bit])buttons|=1u<<bit;
            if(state.Gamepad.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD)buttons|=1u<<14;
            if(state.Gamepad.bRightTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD)buttons|=1u<<15;
        } else connected&=~(1u<<i);
    }
    return buttons;
}
static int jnet_controls_action(uintptr_t dude,const char *name) {
    uintptr_t controller=dude+0xa390;uint32_t count=0,pending=0;
    if(!jnet_menu_read(controller+0x64a0,&count,4) || !count || count>196 ||
       !jnet_menu_read(controller+0x6734,&pending,4) || pending>=16)return 0;
    for(unsigned i=0;i<count;++i){char candidate[24];
        if(!own_read(NULL,controller+0x33a0+i*0x40+0x28,candidate,sizeof(candidate)))return 0;
        if(memchr(candidate,0,sizeof(candidate)) && !strcmp(candidate,name)){
            void (*queue)(uintptr_t,const char *)=(void *)(fault_image+0xaea60);
            queue(controller,name);return 1;
        }
    }
    return 0;
}
static void jnet_controls_tick(uintptr_t barn,uintptr_t game,unsigned kind) {
    uintptr_t local=0;unsigned char menu=0;DWORD process=0;
    if(!jnet_controls_ready)return;
    if(!own_read(NULL,game+0x1e0,&local,8) || local!=barn)return;
    if(jnet_menu_last && !own_read(NULL,jnet_menu_last+0xd68,&menu,1))return;
    menu=menu || jnet_chat_focus;
    GetWindowThreadProcessId(GetForegroundWindow(),&process);
    static uintptr_t crouch_barn;
    if(crouch_barn!=barn || kind!=0){jnet_crouch_active=0;crouch_barn=barn;}
    if(jnet_crouch_active && !menu && process==GetCurrentProcessId()){
        float axes[2]={0};
        if(own_read(NULL,barn+0x30,axes,sizeof(axes)) && isfinite(axes[0]) && isfinite(axes[1]) &&
           (fabsf(axes[0])>.01f || fabsf(axes[1])>.01f) && jnet_controls_action(barn+0xb0,"CrouchToIdle"))jnet_crouch_active=0;
    }
    static unsigned spin_variant;
    unsigned buttons=jnet_controls_buttons();
    static const unsigned mouse[]={0,VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
    for(unsigned action=0;action<7;++action){
        int key=jnet_bindings[0][action],m=jnet_bindings[1][action],pad=jnet_bindings[2][action];
        unsigned down=(key>0 && key<256 && (GetAsyncKeyState(key)&0x8000)) ||
            (m>0 && m<6 && (GetAsyncKeyState(mouse[m])&0x8000)) ||
            (pad>0 && pad<=16 && (buttons&(1u<<(pad-1))));
        if(action==3)jnet_spotlight_down=!menu && process==GetCurrentProcessId() && down && !jnet_binding_down[action];
        if(!menu && process==GetCurrentProcessId() && kind==0){
            static const char *const names[]={"Crouch","SlideJump2","SlideJump1",NULL,"RollF","JNETFlipL","JNETFlipR"};
            if(action!=3 && down && !jnet_binding_down[action]){
                const char *name=action==0 && jnet_crouch_active?"CrouchToIdle":
                    action==2?(spin_variant?"JNETSpin3":"SlideJump1"):names[action];
                int queued=jnet_controls_action(barn+0xb0,name);
                if(action==2 && queued)spin_variant=!spin_variant;
                if(queued)jnet_crouch_active=action==0?!jnet_crouch_active:0;
            }
        }
        jnet_binding_down[action]=down;
    }
}
#endif
