#ifndef JNET_INTRO_LIVE_H
#define JNET_INTRO_LIVE_H
#if defined(_WIN32)
static int jnet_intro_input_down(void) {
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    if(pid!=GetCurrentProcessId())return -1;
    for(int key=1;key<256;++key)if(GetAsyncKeyState(key)&0x8000)return 1;
    struct jnet_controls_pad_state pad=jnet_controls_pad();
    return pad.buttons || pad.motion;
}
#endif
static void (*jnet_intro_update_original)(uintptr_t,float,uintptr_t);
static void jnet_intro_update(uintptr_t slides,float dt,uintptr_t resources) {
    static uintptr_t owner;
    static unsigned down,ready,armed,skipping;
    uintptr_t game=(uintptr_t)JNETGame,expected=0;
    uint32_t count=0,index=0;
    int active=game && own_read(NULL,game+0xa8,&expected,8) && expected==slides &&
        own_read(NULL,slides+0x3530,&count,4) && count>0 && count<=16 &&
        own_read(NULL,slides+0x353c,&index,4) && index<count;
    if(owner!=slides && active)message("[JNET] startup slideshow entered; two-input skip available");
    if(owner!=slides || !active){owner=slides;ready=armed=skipping=down=0;}
    if(active){
        int input=jnet_intro_input_down();
        if(input<0){ready=armed=down=0;}
        else {
            unsigned pressed=input!=0;
            if(!pressed)ready=1;
            if(pressed && !down && ready){if(armed)skipping=1;else armed=1;}
            down=pressed;
        }
        if(skipping){
            uint32_t end=index+1;
            float elapsed=1000000.f;
            if(jc_local_windows_write(slides+0x3530,&end,4) &&
               jc_local_windows_write(slides+0x3538,&elapsed,4))
                message("[JNET] startup skip: current slide completes sequence; remaining logos omitted");
        }
    }
    jnet_intro_update_original(slides,dt,resources);
}
#endif
