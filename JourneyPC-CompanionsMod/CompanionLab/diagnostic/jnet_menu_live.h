#ifndef JNET_MENU_LIVE_H
#define JNET_MENU_LIVE_H
#include "jnet_menu_draw.h"
#include "jnet_menu_model.h"
#include "jnet_lobby_bridge.h"
#include "jnet_chat.h"
#define JNET_MENU_IMAGE_PREVIEW 1
#define JNET_MENU_IMAGE_INPUT 1

static uint64_t jnet_menu_selected_room,jnet_menu_selected_host;
static unsigned jnet_menu_create,jnet_menu_private,jnet_menu_password_focus;
static char jnet_menu_password[33];
static unsigned jnet_menu_chat,jnet_menu_cmatw=1,jnet_menu_daynight;
static struct jnet_appearance_edit jnet_menu_edit;
static unsigned jnet_menu_level,jnet_menu_page,jnet_menu_closing;
static uintptr_t jnet_menu_last;
static unsigned jnet_menu_level_step(unsigned level,int direction) {
    static const unsigned order[]={0,1,2,3,4,5,6,7,8,10,11,9};
    for(unsigned i=0;i<12;++i)if(order[i]==level) {
        if(direction>0)return order[(i+1)%12];
        if(direction<0)return order[(i+11)%12];
        return level;
    }
    return 0;
}
static unsigned jnet_menu_key_row,jnet_menu_key_active,jnet_menu_key_click;
static float jnet_menu_key_x,jnet_menu_key_y;
static struct { uintptr_t owner; unsigned page,pending; float x,y; } jnet_menu_click;
static int jnet_menu_apply_unavailable;
static unsigned jnet_menu_supported_characters;
static char jnet_appearance_status[80];
static uint64_t jnet_appearance_applied_at;
static float jnet_appearance_arrow=.22f;
static float jnet_variant_arrow=.12f;
static const float jnet_level_left=.33f,jnet_level_right=.87f;
static const float jnet_appearance_rows[]={.58f,.36f,.14f,-.08f,-.30f,-.52f,-.68f};
static const float jnet_character_column=0.f;
static const float jnet_character_toggle_rows[]={-.25f,-.39f,-.53f,-.67f,-.81f};
static void jnet_menu_reset_page(void) {
    jnet_menu_click.pending=0;
    jnet_menu_page=0;jnet_menu_create=0;jnet_menu_password_focus=0;jnet_menu_closing=0;
    jnet_appearance_status[0]=0;jnet_appearance_applied_at=0;jnet_menu_key_active=0;jnet_menu_key_click=0;
}
static void jnet_menu_back_page(void) {
    if(jnet_menu_page==2 && jnet_menu_create){
        jnet_menu_create=0;jnet_menu_password_focus=0;jnet_menu_click.pending=0;
        jnet_menu_key_active=0;jnet_menu_key_click=0;
    }else jnet_menu_reset_page();
}
#include "../work/menu-images/jnet_menu_assets.h"

static uintptr_t (*jnet_original_menu_build)(uintptr_t);
static uintptr_t (*jnet_original_menu_nodes)(uintptr_t,uintptr_t,uintptr_t,uint32_t,float,float *);
static __declspec(thread) uintptr_t jnet_menu_layout_owner;
static __declspec(thread) int jnet_menu_layout_changed;
static __declspec(thread) float jnet_menu_saved_geometry[4];
static __declspec(thread) int jnet_menu_drawing;
static __declspec(thread) struct {
    uintptr_t owner,head,font;
    struct jnet_menu_glyph glyphs[95];
    unsigned char present[95];
} jnet_menu_font;
__declspec(dllexport) volatile LONG JNETMenu[19];

static int jnet_menu_read(uintptr_t address,void *out,size_t bytes) {
    SIZE_T got=0;
    return address && bytes<=4096 && ReadProcessMemory(GetCurrentProcess(),(void *)address,out,bytes,&got) && got==bytes;
}
static int jnet_menu_pointer(uintptr_t menu,float *x,float *y) {
    float point[2];uint32_t width=0,height=0;
    if(!jnet_menu_read(menu+0xc78,point,sizeof(point)) ||
       !jnet_menu_read(fault_image+0x3763e74,&width,4) || !width ||
       !jnet_menu_read(fault_image+0x3763e78,&height,4) || !height ||
       !isfinite(point[0]) || !isfinite(point[1]))return 0;
    float nx=point[0]*(float)width/(float)height;
    if(!isfinite(nx) || fabsf(nx)>1.001f || fabsf(point[1])>1.001f)return 0;
    *x=nx;*y=point[1];return 1;
}
static int jnet_menu_take_click(uintptr_t menu,float *x,float *y) {
    unsigned pending=jnet_menu_click.pending;jnet_menu_click.pending=0;
    if(!pending || jnet_menu_click.owner!=menu || jnet_menu_click.page!=jnet_menu_page)return 0;
    *x=jnet_menu_click.x;*y=jnet_menu_click.y;return 1;
}
static int jnet_menu_visible_root(uintptr_t menu);
static int jnet_menu_visible_options(uintptr_t menu);
static int jnet_menu_visible_native(uintptr_t menu);
static volatile LONG jnet_fps_limit;
static int jnet_menu_visible_audio(uintptr_t menu);
static float jnet_menu_native_height;
static unsigned char jnet_menu_native_path[32];
static uintptr_t (*jnet_menu_pop_original)(uintptr_t,uintptr_t);
static unsigned jnet_menu_input_blocked;
static uint64_t jnet_menu_input_block_frame;
static void jnet_menu_block_activation(void) {
    jnet_menu_input_blocked=1;jnet_menu_input_block_frame=(uint64_t)JNETFrames;
}
static int jnet_menu_string_is(uintptr_t node,const char *text);
static unsigned jnet_menu_restart_pending;
static int jnet_menu_is_controls(uintptr_t menu) {
    uintptr_t begin=0,end=0;
    if(!jnet_menu_read(menu+0xc10,&begin,8) || !jnet_menu_read(menu+0xc18,&end,8) ||
       !begin || end<begin || end-begin>0x600 || (end-begin)%0x30)return 0;
    if(end>begin && jnet_menu_string_is(end-0x30,"Back"))return 0;
    for(uintptr_t p=begin;p<end;p+=0x30)if(jnet_menu_string_is(p,"Controls"))return 1;
    return 0;
}
static void jnet_menu_finish_close(uintptr_t menu) {
    unsigned char visible=1;float opacity=1;
    if(!jnet_menu_closing || !menu || !jnet_menu_read(menu+0xd68,&visible,1) ||
       !jnet_menu_read(menu+0xd6c,&opacity,4) || (visible && opacity>.001f))return;
    uintptr_t begin=0,end=0;
    if(!jnet_menu_read(menu+0xc10,&begin,8) || !jnet_menu_read(menu+0xc18,&end,8) ||
       !begin || end<begin+0x60 || end-begin>0x600 || (end-begin)%0x30 ||
       !jnet_menu_string_is(begin,"Root"))return;
    uintptr_t depth=(end-begin)/0x30;
    if(depth>2)jnet_menu_pop_original(menu,depth-2);
    jnet_menu_reset_page();
}
#if defined(_WIN32) && JNET_MENU_IMAGE_PREVIEW
#include "jnet_menu_images.h"
#if JNET_MENU_IMAGE_INPUT
static uintptr_t jnet_menu_pop(uintptr_t menu,uintptr_t count) {
    if(jnet_menu_password_focus || jnet_menu_closing)return 0;
    jnet_menu_block_activation();
    if(jnet_menu_page){jnet_menu_back_page();return 0;}
    return jnet_menu_pop_original(menu,count);
}
static void jnet_menu_escape(void) {
    jnet_menu_finish_close(jnet_menu_last);
}
static void jnet_menu_controls(uintptr_t menu) {
    static unsigned char keys[256];
    HWND window=GetForegroundWindow();DWORD pid=0;
    GetWindowThreadProcessId(window,&pid);
    if((!jnet_menu_visible_root(menu) && !jnet_menu_visible_options(menu) && !jnet_menu_visible_audio(menu)) || jnet_menu_closing) {
        jnet_menu_click.pending=0;jnet_menu_key_click=0;jnet_menu_password_focus=0;memset(keys,0,sizeof(keys));return;
    }
    struct jnet_lobby_view view;jnet_lobby_snapshot(&view);
    static uint64_t observed_room;
    if(view.room && view.room!=observed_room){
        memset(jnet_menu_password,0,sizeof(jnet_menu_password));jnet_menu_password_focus=0;
    }
    observed_room=view.room;
    static unsigned observed_level=~0u,observed_lighting=~0u;
    unsigned current=jnet_current_level;
    unsigned lighting=jnet_daynight_mode;
    if(observed_lighting!=lighting){jnet_menu_daynight=lighting;observed_lighting=lighting;}
    if(current<12 && observed_level!=current){jnet_menu_level=current;observed_level=current;}
    unsigned shift=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;
    if(pid!=GetCurrentProcessId())jnet_menu_password_focus=0;
    if(!jnet_menu_password_focus)memset(keys,0,sizeof(keys));
    for(unsigned key=0;key<256;++key) {
        if(!jnet_menu_password_focus)break;
        unsigned down=(GetAsyncKeyState((int)key)&0x8000)!=0,press=down&&!keys[key];keys[key]=(unsigned char)down;
        if(!press || !jnet_menu_password_focus)continue;
        size_t length=strlen(jnet_menu_password);char ch=0;
        if(key==VK_BACK && length)jnet_menu_password[length-1]=0;
        else if(key==VK_ESCAPE)jnet_menu_password_focus=0;
        else if(key>='A' && key<='Z')ch=(char)(key+((shift!=!!(GetKeyState(VK_CAPITAL)&1))?0:32));
        else if(key>='0' && key<='9')ch=shift?")!@#$%^&*("[key-'0']:(char)key;
        else if(key==VK_SPACE)ch=' ';
        else if(key==VK_OEM_MINUS)ch=shift?'_':'-';
        else if(key==VK_OEM_PERIOD)ch='.';
        if(view.online){if(ch>='a' && ch<='z')ch-=32;if(!((ch>='A' && ch<='Z') || (ch>='0' && ch<='9')))ch=0;}
        if(ch && length<(view.online?4u:32u)){jnet_menu_password[length]=ch;jnet_menu_password[length+1]=0;}
    }
    float x,y;
    if(jnet_menu_take_click(menu,&x,&y))jnet_menu_key_click=0;
    else if(jnet_menu_key_click){x=jnet_menu_key_x;y=jnet_menu_key_y;jnet_menu_key_click=0;}
    else return;
    if(jnet_menu_visible_audio(menu) || jnet_menu_visible_options(menu))return;
    if(jnet_menu_page && fabsf(x+.76f)<.14f && y>-.96f && y<-.82f) {
        jnet_menu_back_page();return;
    }
    if(!jnet_menu_page)return;
    if(jnet_menu_page==1) {
        if(x>-.3f && x<.3f && y>-.96f && y<-.82f){
            jnet_appearance_applied_at=0;
            int result=jnet_appearance_apply(&jnet_menu_edit,JNET_SCARF_MAX,jnet_menu_supported_characters);
            if(result==JNET_MENU_OK)
                jnet_appearance_status[0]=0;
            else if(result==JNET_MENU_BUSY)strcpy(jnet_appearance_status,"Character change pending");
            else if(result==JNET_MENU_UNAVAILABLE)strcpy(jnet_appearance_status,"Character unavailable");
            else strcpy(jnet_appearance_status,"Invalid appearance selection");
            return;
        }
        int direction=fabsf(x-jnet_character_column-jnet_appearance_arrow)<.065f?1:(fabsf(x-jnet_character_column+jnet_appearance_arrow)<.065f?-1:0);
        const unsigned selectors[]={JNET_SELECT_ROBE,JNET_SELECT_SCARF,JNET_SELECT_SCARF_MODE,JNET_SELECT_SCARF_GROW,JNET_SELECT_SYMBOL,JNET_SELECT_CHARACTER,JNET_SELECT_VARIANT};
        for(unsigned i=0;i<jnet_appearance_row_count(jnet_menu_edit.draft.character);++i)if(direction && fabsf(y-jnet_appearance_rows[i])<.09f){
            if(jnet_menu_edit.awaiting)jnet_appearance_complete(&jnet_menu_edit,0);
            jnet_appearance_step(&jnet_menu_edit,selectors[i],direction,JNET_SCARF_MAX);
            jnet_appearance_status[0]=0;
        }
        int toggle_direction=fabsf(x-.90f)<.06f?1:(fabsf(x-.70f)<.06f?-1:0);
        if(toggle_direction){
            if(fabsf(y-jnet_character_toggle_rows[1])<.065f)jnet_freezing=!jnet_freezing;
            if(fabsf(y-jnet_character_toggle_rows[2])<.065f)jnet_sliding=!jnet_sliding;
            if(fabsf(y-jnet_character_toggle_rows[3])<.065f)jnet_windwalls=!jnet_windwalls;
            if(fabsf(y-jnet_character_toggle_rows[0])<.065f){
                jnet_blackness=!jnet_blackness;
                jnet_appearance_status[0]=0;
            }
            if(fabsf(y-jnet_character_toggle_rows[4])<.065f)jnet_fog=!jnet_fog;
        }
        return;
    }
    if(jnet_menu_create) {
        if(x>-.84f && x<.3f && y>.04f && y<.15f){jnet_menu_private=!jnet_menu_private;if(!jnet_menu_private && !jnet_menu_cmatw)jnet_menu_chat=0;}
        else if(x>-.84f && x<.3f && y>-.12f && y<.02f){jnet_menu_cmatw=!jnet_menu_cmatw;if(!jnet_menu_private && !jnet_menu_cmatw)jnet_menu_chat=0;}
        else if(x>-.84f && x<.3f && y>-.26f && y<-.12f && (jnet_menu_private || jnet_menu_cmatw))jnet_menu_chat=!jnet_menu_chat;
        else if(y>-.77f && y<-.64f && x>-.86f && x<-.50f) {
            struct jnet_lobby_action action={.kind=1,.access=jnet_menu_private,.mode=JNET_JOURNEY,.chat=jnet_menu_chat,.cmatw=jnet_menu_cmatw};
            memcpy(action.password,jnet_menu_password,sizeof(action.password));
            if(jnet_lobby_submit(&action)){jnet_menu_create=0;jnet_menu_password_focus=0;memset(jnet_menu_password,0,sizeof(jnet_menu_password));}
        } else if(y>-.77f && y<-.64f && x>-.43f && x<-.12f){jnet_menu_create=0;jnet_menu_password_focus=0;}
        return;
    }
    if(x>-.90f && x<-.04f && y>-.77f && y<-.64f){jnet_menu_password_focus=1;return;}
    jnet_menu_password_focus=0;
    if(y>-.94f && y<-.81f) {
        if(x>-.56f && x<-.30f && !view.room) {
            struct jnet_lobby_action action={.kind=(view.room || view.busy)?3:2,.host=jnet_menu_selected_host,.room=jnet_menu_selected_room};
            memcpy(action.password,jnet_menu_password,sizeof(action.password));
            if(jnet_lobby_submit(&action))memset(jnet_menu_password,0,sizeof(jnet_menu_password));return;
        }
        if(x>-.23f && x<.09f){jnet_menu_create=1;jnet_menu_chat=0;jnet_menu_cmatw=1;return;}
    }
    if(view.room && x>.47f && x<.72f && y>-.84f && y<-.73f){struct jnet_lobby_action action={.kind=3};jnet_lobby_submit(&action);return;}
    if(view.online && x>-.88f && x<-.57f && y>-.59f && y<-.45f){struct jnet_lobby_action action={.kind=5};jnet_lobby_submit(&action);return;}
    if(view.online && x>.04f && x<.4f && y>-.94f && y<-.81f){struct jnet_lobby_action action={.kind=4};jnet_lobby_submit(&action);return;}
    for(unsigned i=0;i<view.count;++i)if(x>-.92f && x<-.04f && y<.55f-i*.14f && y>.43f-i*.14f) {
        jnet_menu_selected_room=view.rows[i].room;jnet_menu_selected_host=view.rows[i].host;return;
    }
    if(x>-.92f && x<-.04f && y>.0f && y<.3f)jnet_menu_selected_room=jnet_menu_selected_host=0;
    int direction=fabsf(x-jnet_level_right)<.06f?1:(fabsf(x-jnet_level_left)<.06f?-1:0);
    int variant_direction=fabsf(x-(.60f+jnet_variant_arrow))<.045f?1:(fabsf(x-(.60f-jnet_variant_arrow))<.045f?-1:0);
    if(variant_direction && y>.17f && y<.29f && (jnet_daynight_supported(jnet_menu_level) || jnet_menu_level==7) && (!view.room || view.self==view.host)){
        unsigned count=jnet_daynight_variants(jnet_menu_level);
        jnet_menu_daynight=(jnet_menu_daynight+(variant_direction>0?1:count-1))%count;return;
    }
    if(direction && y>.38f && y<.58f && (!view.room || view.self==view.host)) {
        jnet_menu_level=jnet_menu_level_step(jnet_menu_level,direction);
        if(jnet_menu_daynight>=jnet_daynight_variants(jnet_menu_level))jnet_menu_daynight=0;
    }
    if(x>.45f && x<.75f && y>.05f && y<.17f && (!view.room || view.self==view.host)) {
        struct jnet_lobby_action action={.kind=6,.level=jnet_menu_level,.lighting=jnet_menu_level==7 && jnet_menu_daynight==3?0:jnet_menu_daynight,.paradise=jnet_menu_level==7 && jnet_menu_daynight==3};jnet_lobby_submit(&action);
    }

}
#endif
static void jnet_menu_publish_images(uintptr_t menu,unsigned width,unsigned height) {
    struct jnet_image_frame frame={.owner=menu,.time=GetTickCount64()};
    if(jnet_menu_visible_root(menu) && width && height) {
        if(jnet_menu_page==2 && !jnet_menu_create){
            frame.count=1;frame.items[0]=(struct jnet_image_item){{.36f,.67f,.84f,.32f},jnet_level_assets[jnet_menu_level<12?jnet_menu_level:0][jnet_menu_daynight<4?jnet_menu_daynight:0],1};
        } else if(jnet_menu_page==1){
            frame.count=3;
            frame.items[0]=(struct jnet_image_item){{-.10f,.68f,.10f,.48f},12+jnet_menu_edit.draft.outfit,1};
            float filled=fminf(1.f,(jnet_menu_edit.draft.scarf_segments+1.f)/30.f);
            frame.items[1]=(struct jnet_image_item){{-.15f,.31f,-.15f+.30f*filled,.26f},40u+(unsigned)jnet_scarf_white(&jnet_menu_edit),filled};
            frame.items[2]=(struct jnet_image_item){{-.065f,-.235f,.065f,-.365f},19u+jnet_menu_edit.draft.symbol,1};
            for(unsigned i=0;i<frame.count;++i){frame.items[i].box[0]+=jnet_character_column;frame.items[i].box[2]+=jnet_character_column;}
        }
    }
    float opacity=0;
    if(jnet_menu_read(menu+0xd6c,&opacity,4) && isfinite(opacity))frame.opacity=fminf(1.f,fmaxf(0.f,opacity));
    AcquireSRWLockExclusive(&jnet_image_lock);jnet_image_frame=frame;ReleaseSRWLockExclusive(&jnet_image_lock);
    InterlockedExchange(&JNETMenu[8],jnet_image_result);InterlockedExchange(&JNETMenu[9],jnet_image_frames);
}
#endif
static int jnet_menu_string_is(uintptr_t object,const char *expected) {
    unsigned char value[32];uintptr_t data;uint64_t length,capacity;
    if(!jnet_menu_read(object,value,sizeof(value)))return 0;
    memcpy(&length,value+16,8);memcpy(&capacity,value+24,8);
    size_t wanted=strlen(expected);
    if(length!=wanted || length>sizeof(value) || capacity<length)return 0;
    if(capacity<16)return length<16 && !memcmp(value,expected,wanted);
    memcpy(&data,value,8);
    return jnet_menu_read(data,value,wanted) && !memcmp(value,expected,wanted);
}
static int jnet_menu_visible_root(uintptr_t menu) {
    uintptr_t begin,end;unsigned char visible;
    if(!jnet_menu_read(menu+0xd68,&visible,1) || !visible ||
       !jnet_menu_read(menu+0xc10,&begin,8) || !jnet_menu_read(menu+0xc18,&end,8) ||
       !begin || end<begin || end-begin!=0x60)return 0;
    return jnet_menu_string_is(begin,"Root") && jnet_menu_string_is(begin+0x30,"Menu");
}
static int jnet_menu_visible_options(uintptr_t menu) {
    uintptr_t begin,end;unsigned char visible;
    if(!jnet_menu_read(menu+0xd68,&visible,1) || !visible ||
       !jnet_menu_read(menu+0xc10,&begin,8) || !jnet_menu_read(menu+0xc18,&end,8) ||
       !begin || end<begin || end-begin<0x60 || end-begin>0x180 || (end-begin)%0x30)return 0;
    return jnet_menu_string_is(begin,"Root") && jnet_menu_string_is(end-0x30,"Options");
}
static int jnet_menu_visible_native(uintptr_t menu) {
    uintptr_t begin,end;unsigned char visible;
    return jnet_menu_read(menu+0xd68,&visible,1) && visible &&
        jnet_menu_read(menu+0xc10,&begin,8) && jnet_menu_read(menu+0xc18,&end,8) && begin &&
        end>=begin+0x60 && end-begin<=0x300 && (end-begin)%0x30==0 && jnet_menu_string_is(begin,"Root");
}
static int jnet_menu_visible_audio(uintptr_t menu) {
    uintptr_t end=0;
    return jnet_menu_visible_native(menu) && jnet_menu_read(menu+0xc18,&end,8) && (jnet_menu_string_is(end-0x30,"Audio") || jnet_menu_string_is(end-0x30,"Graphics"));
}
static int jnet_menu_pressed(uintptr_t menu,unsigned offset) {
    unsigned char state[2]={0};
    return jnet_menu_read(menu+offset,state,2) && state[0] && !state[1];
}
static void (*jnet_original_menu_input)(uintptr_t,uintptr_t);
static void jnet_menu_input(uintptr_t menu,uintptr_t rows) {
    static uintptr_t mouse_owner;static unsigned mouse_held;
    unsigned char mouse[2]={0},accept_state[2]={0},back_state[2]={0};
    jnet_menu_read(menu+0xd40,mouse,2);jnet_menu_read(menu+0xd10,accept_state,2);
    jnet_menu_read(menu+0xc98,back_state,2);
    if(mouse_owner!=menu){mouse_owner=menu;mouse_held=mouse[1]!=0;}
    unsigned mouse_pressed=mouse[0] && !mouse[1] && !mouse_held;
    mouse_held=mouse[0]!=0;
    if(jnet_menu_input_blocked && (uint64_t)JNETFrames>jnet_menu_input_block_frame &&
       !mouse[0] && !mouse[1] && !accept_state[0] && !accept_state[1] &&
       !back_state[0] && !back_state[1])
        jnet_menu_input_blocked=0;
    if(jnet_menu_closing)return;
    uintptr_t escape=0;
    if(jnet_menu_pressed(menu,0xc98) && jnet_menu_visible_native(menu) &&
       jnet_menu_read(fault_image+0x3c47b60+27*8,&escape,8) && escape){
        float speed=0,velocity=0;
        if(jnet_menu_read(menu+0xd74,&speed,4) && isfinite(speed) && speed>0 &&
           jnet_menu_read(menu+0xd70,&velocity,4) && isfinite(velocity)){
            if(velocity>0)return;
            jnet_menu_closing=1;jnet_menu_block_activation();
            speed=-speed;memcpy((void *)(menu+0xd70),&speed,4);
            return;
        }
    }
    int root=jnet_menu_visible_root(menu),audio=jnet_menu_visible_audio(menu);
    if((!root && !audio) || jnet_menu_closing){jnet_original_menu_input(menu,rows);return;}
    int up=jnet_menu_pressed(menu,0xcb0),down=jnet_menu_pressed(menu,0xcc8);
    int left=jnet_menu_pressed(menu,0xce0),right=jnet_menu_pressed(menu,0xcf8);
    int accept=jnet_menu_pressed(menu,0xd10),back=jnet_menu_pressed(menu,0xc98);
    if(jnet_menu_page){
        if(back && !jnet_menu_password_focus){jnet_menu_back_page();jnet_menu_block_activation();return;}
        struct jnet_lobby_view view;jnet_lobby_snapshot(&view);
        float points[32][2];unsigned count=0;
        if(jnet_menu_page==1){
            for(unsigned i=0;i<jnet_appearance_row_count(jnet_menu_edit.draft.character);++i){points[count][0]=jnet_character_column+jnet_appearance_arrow;points[count++][1]=jnet_appearance_rows[i];}
            for(unsigned i=0;i<5;++i){points[count][0]=.90f;points[count++][1]=jnet_character_toggle_rows[i];}
            points[count][0]=0;points[count++][1]=-.89f;
        } else if(jnet_menu_create){
            const float y[]={.09f,-.05f,-.19f,-.70f,-.70f};
            for(unsigned i=0;i<5;++i){if(i==2 && !jnet_menu_private && !jnet_menu_cmatw)continue;points[count][0]=i==4?-.28f:-.68f;points[count++][1]=y[i];}
        } else {
            if(view.online){points[count][0]=-.73f;points[count++][1]=-.53f;}
            for(unsigned i=0;i<view.count && count<16;++i){points[count][0]=-.50f;points[count++][1]=.49f-i*.14f;}
            points[count][0]=-.50f;points[count++][1]=-.71f;
            if(!view.room){points[count][0]=-.43f;points[count++][1]=-.89f;}
            points[count][0]=-.08f;points[count++][1]=-.89f;
            if(!view.room || view.self==view.host){
                points[count][0]=jnet_level_right;points[count++][1]=.48f;
                if(jnet_daynight_supported(jnet_menu_level) || jnet_menu_level==7){points[count][0]=.60f+jnet_variant_arrow;points[count++][1]=.23f;}
                points[count][0]=.60f;points[count++][1]=.11f;
            }
            if(view.room){points[count][0]=.60f;points[count++][1]=-.79f;}
            if(view.online && view.total>3){points[count][0]=.20f;points[count++][1]=-.89f;}
        }
        points[count][0]=-.76f;points[count++][1]=-.89f;
        static uintptr_t previous_owner;static float previous_x,previous_y;
        float mx=0,my=0;int pointer_valid=jnet_menu_pointer(menu,&mx,&my);
        if(pointer_valid && (previous_owner!=menu || mx!=previous_x || my!=previous_y || mouse_pressed)){
            previous_owner=menu;previous_x=mx;previous_y=my;
            jnet_menu_key_active=0;
            float best=10;
            for(unsigned i=0;i<count;++i){
                float center=points[i][0],half=.18f;
                if(jnet_menu_page==1 && i<jnet_appearance_row_count(jnet_menu_edit.draft.character)){center=jnet_character_column;half=jnet_appearance_arrow+.08f;}
                else if(jnet_menu_page==1 && i<count-1){center=.72f;half=.26f;}
                else if(jnet_menu_create && points[i][1]>-.60f){center=-.27f;half=.57f;}
                else if(jnet_menu_page==2 && !jnet_menu_create && center==-.50f){half=.42f;}
                else if(jnet_menu_page==2 && !jnet_menu_create && points[i][1]==.48f){center=.60f;half=.38f;}
                else if(jnet_menu_page==2 && !jnet_menu_create && points[i][1]==.23f){center=.60f;half=jnet_variant_arrow+.06f;}
                float distance=fabsf(mx-center);
                if(fabsf(my-points[i][1])<.065f && distance<half && distance<best){
                    best=distance;jnet_menu_key_row=i;jnet_menu_key_active=1;
                }
            }
        }
        if(jnet_menu_key_row>=count)jnet_menu_key_row=0;
        if(!jnet_menu_password_focus){
            if(up || down || left || right || accept)jnet_menu_key_active=1;
            if(up)jnet_menu_key_row=(jnet_menu_key_row+count-1)%count;
            if(down)jnet_menu_key_row=(jnet_menu_key_row+1)%count;
            int selector=jnet_menu_page==1?jnet_menu_key_row<count-1:
                (!jnet_menu_create && (points[jnet_menu_key_row][1]==.48f || points[jnet_menu_key_row][1]==.23f));
            if(accept || (selector && (left || right)))jnet_menu_key_click=1;
            if(jnet_menu_key_active){jnet_menu_key_x=points[jnet_menu_key_row][0];jnet_menu_key_y=points[jnet_menu_key_row][1];
                if(jnet_menu_page==2 && !jnet_menu_create && !view.room)
                    for(unsigned i=0;i<view.count;++i)if(jnet_menu_key_y==.49f-i*.14f){
                        jnet_menu_selected_room=view.rows[i].room;jnet_menu_selected_host=view.rows[i].host;
                    }
                if(left && jnet_menu_page==1 && jnet_menu_key_row<jnet_appearance_row_count(jnet_menu_edit.draft.character))jnet_menu_key_x=jnet_character_column-jnet_appearance_arrow;
                else if(left && jnet_menu_page==1 && jnet_menu_key_row<count-1)jnet_menu_key_x=.70f;
                else if(left && jnet_menu_page==2 && jnet_menu_key_y==.48f)jnet_menu_key_x=jnet_level_left;
                else if(left && jnet_menu_page==2 && jnet_menu_key_y==.23f)jnet_menu_key_x=.60f-jnet_variant_arrow;
            }
        } else if(accept || back)jnet_menu_password_focus=0;
        if(mouse_pressed && pointer_valid && !jnet_menu_input_blocked){
            jnet_menu_click.owner=menu;jnet_menu_click.page=jnet_menu_page;
            jnet_menu_click.x=mx;jnet_menu_click.y=my;jnet_menu_click.pending=1;
        }
        return;
    }
    jnet_original_menu_input(menu,rows);
}
static int (*jnet_menu_settings_action)(uintptr_t,uintptr_t);
static unsigned char (*jnet_original_menu_action)(uintptr_t,uintptr_t);
static unsigned char jnet_menu_action(uintptr_t menu,uintptr_t node) {
    if(jnet_menu_input_blocked || jnet_menu_closing)return 1;
    if(jnet_menu_settings_action && jnet_menu_settings_action(menu,node))return 1;
    unsigned page=jnet_menu_string_is(node,"JNETCharacter")?1:jnet_menu_string_is(node,"JNETWorld")?2:0;
    if(page && jnet_menu_visible_root(menu)){
        jnet_menu_page=page;jnet_menu_key_row=0;jnet_menu_key_active=1;
        jnet_appearance_status[0]=0;
        if(page==2)jnet_menu_level=jnet_current_level<12?jnet_current_level:2;
        return 1;
    }
    unsigned back=jnet_menu_string_is(node,"Back");
    unsigned char result=jnet_original_menu_action(menu,node);
    if(back)jnet_menu_block_activation();
    return result;
}
static int jnet_menu_corner(uintptr_t menu,uintptr_t caller,float *x,float *y) {
    uint32_t width,height;float padding,size,geometry[4];
    if(caller!=fault_image+0x105ca3 || menu!=jnet_menu_layout_owner ||
       jnet_menu_layout_changed || !y || !jnet_menu_visible_native(menu) ||
       !jnet_menu_read(menu+0x20,geometry,sizeof(geometry)) ||
       !jnet_menu_read(menu+0xc5c,&padding,4) || !isfinite(padding) || padding<0 || padding>.2f ||
       !jnet_menu_read(menu+0xc48,&size,4) || !isfinite(size) || size<=0 || size>.15f ||
       !jnet_menu_read(fault_image+0x3763e74,&width,4) || !width ||
       !jnet_menu_read(fault_image+0x3763e78,&height,4) || !height)return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(geometry[i]))return 0;
    float aspect=(float)height/(float)width;
    memcpy(jnet_menu_saved_geometry,geometry,sizeof(geometry));
    if(!jnet_menu_visible_root(menu)){
        uintptr_t end=0;unsigned char path[32]={0};
        if(!jnet_menu_read(menu+0xc18,&end,8) || !jnet_menu_read(end-0x30,path,sizeof(path)))return 0;
        if(memcmp(path,jnet_menu_native_path,sizeof(path))){memcpy(jnet_menu_native_path,path,sizeof(path));jnet_menu_native_height=0;}
        float h=jnet_menu_native_height>0?jnet_menu_native_height:8.f*size;
        if(jnet_menu_is_controls(menu))h=1.4f;
        *x=-.88f*aspect;*y=jnet_menu_is_controls(menu)?.31f:fminf(.84f,-.88f+h);
        geometry[3]=h+2.f*padding;
        geometry[0]=*x+geometry[2]*.5f-padding;geometry[1]=*y-h*.5f;
        memcpy((void *)(menu+0x20),geometry,sizeof(geometry));jnet_menu_layout_changed=1;return 1;
    }
    *x=-.88f*aspect;*y=-.88f+7.f*size;
    geometry[2]=.84f*aspect+padding;
    geometry[3]=8.f*size+2.f*padding;
    geometry[0]=-.48f*aspect;
    geometry[1]=*y-geometry[3]*.5f+padding;
    memcpy((void *)(menu+0x20),geometry,sizeof(geometry));
    jnet_menu_layout_changed=1;return 1;
}
static void jnet_menu_restore_layout(uintptr_t menu) {
    if(jnet_menu_layout_changed)memcpy((void *)(menu+0x20),jnet_menu_saved_geometry,sizeof(jnet_menu_saved_geometry));
    jnet_menu_layout_changed=0;jnet_menu_layout_owner=0;
}
static void jnet_menu_align_title(uintptr_t menu,uintptr_t vector,float *y) {
    unsigned char boot=0;uintptr_t begin=0,end=0;float size=0,delta=0;
    if(!jnet_menu_read(menu+0xda0,&boot,1) || !boot ||
       !jnet_menu_read(menu+0xc48,&size,4) || !isfinite(size) || size<=0 || size>.15f ||
       !jnet_menu_read(vector,&begin,8) || !jnet_menu_read(vector+8,&end,8) ||
       !begin || end<begin || end-begin>128*32 || (end-begin)%128)return;
    for(uintptr_t row=begin;row<end;row+=128){uintptr_t node=0;float baseline=0;
        if(!jnet_menu_read(row,&node,8) || !jnet_menu_read(row+0x7c,&baseline,4) || !isfinite(baseline))return;
        if(jnet_menu_string_is(node,"QuitToDesktop")){delta=-.88f+size-baseline;break;}
    }
    if(!isfinite(delta) || fabsf(delta)>.8f || delta==0)return;
    for(uintptr_t row=begin;row<end;row+=128){
        const unsigned offsets[]={0x0c,0x14,0x1c,0x7c};
        for(unsigned i=0;i<4;++i){float value=0;
            if(!jnet_menu_read(row+offsets[i],&value,4) || !isfinite(value))return;
            value+=delta;memcpy((void *)(row+offsets[i]),&value,4);
        }
    }
    if(y)*y+=delta;
}
static void jnet_menu_controls_back(uintptr_t vector) {
    uintptr_t begin=0,end=0;
    if(!jnet_menu_read(vector,&begin,8) || !jnet_menu_read(vector+8,&end,8) ||
       !begin || end<begin || end-begin>128*32 || (end-begin)%128)return;
    for(uintptr_t row=begin;row<end;row+=128){
        uintptr_t node=0;float baseline=0;
        if(!jnet_menu_read(row,&node,8) || !jnet_menu_read(row+0x7c,&baseline,4) || !isfinite(baseline))return;
        if(!jnet_menu_string_is(node,"Back"))continue;
        const unsigned offsets[]={0x0c,0x14,0x1c,0x7c};
        float delta=-.88f-baseline;
        for(unsigned i=0;i<4;++i){float value=0;
            if(!jnet_menu_read(row+offsets[i],&value,4) || !isfinite(value))return;
            value+=delta;memcpy((void *)(row+offsets[i]),&value,4);
        }
    }
}
static uintptr_t jnet_menu_nodes(uintptr_t menu,uintptr_t vector,uintptr_t node,uint32_t flags,float x,float *y) {
    if(menu==jnet_menu_layout_owner && jnet_menu_visible_root(menu) && jnet_menu_page)return 0;
    int top=jnet_menu_corner(menu,(uintptr_t)__builtin_return_address(0),&x,y);
    float start=y?*y:0;
    uintptr_t result=jnet_original_menu_nodes(menu,vector,node,flags,x,y);
    if(top && jnet_menu_is_controls(menu))jnet_menu_controls_back(vector);
    if(top && jnet_menu_visible_root(menu))jnet_menu_align_title(menu,vector,y);
    if(top && y && !jnet_menu_visible_root(menu) && isfinite(*y) && start>*y && start-*y<1.75f)jnet_menu_native_height=start-*y;
    return result;
}
static int jnet_menu_find_glyph(uintptr_t head,uint16_t code,struct jnet_menu_glyph *out) {
    uintptr_t node;
    if(!jnet_menu_read(head+8,&node,8))return 0;
    for(unsigned depth=0;depth<32 && node && node!=head;++depth) {
        unsigned char data[0x48];uint16_t key;
        if(!jnet_menu_read(node,data,sizeof(data)) || data[0x19])return 0;
        memcpy(&key,data+0x1c,2);
        if(key==code){memcpy(out,data+0x20,sizeof(*out));return 1;}
        memcpy(&node,data+(code<key?0:0x10),8);
    }
    return 0;
}
static int jnet_menu_line(struct jnet_menu_quad *quads,uint32_t *used,const char *label,
                           float x,float y,float scale,float width,uint32_t color) {
    struct jnet_menu_glyph glyphs[JNET_MENU_TEXT_LIMIT];
    size_t count=strlen(label);
    if(!count)return 1;
    if(count>JNET_MENU_TEXT_LIMIT-*used)return 0;
    float advance=0;
    for(size_t i=0;i<count;++i) {
        unsigned c=(unsigned char)label[i];
        if(c<32 || c>126 || !jnet_menu_font.present[c-32])return 0;
        glyphs[i]=jnet_menu_font.glyphs[c-32];advance+=glyphs[i].advance;
    }
    if(!isfinite(advance) || advance<=0)return 0;
    if(advance*scale>width)scale=width/advance;
    struct jnet_menu_text text;
    if(!jnet_menu_text_layout(&text,glyphs,count,x,y,scale,color))return 0;
    memcpy(quads+*used,text.quads,count*sizeof(*quads));*used+=(uint32_t)count;return 1;
}
static int jnet_menu_centered(struct jnet_menu_quad *quads,uint32_t *used,const char *label,
        float x,float y,float scale,float width,uint32_t color) {
    uint32_t start=*used;
    if(!jnet_menu_line(quads,used,label,0,0,scale,width,color))return 0;
    if(*used==start)return 1;
    float left=INFINITY,right=-INFINITY,top=INFINITY,bottom=-INFINITY;
    for(uint32_t i=start;i<*used;++i)for(unsigned v=0;v<4;++v) {
        float px=quads[i].position[v][0],py=quads[i].position[v][1];
        left=fminf(left,px);right=fmaxf(right,px);top=fminf(top,py);bottom=fmaxf(bottom,py);
    }
    for(uint32_t i=start;i<*used;++i)for(unsigned v=0;v<4;++v) {
        quads[i].position[v][0]+=x-(left+right)*.5f;
        quads[i].position[v][1]+=y-(top+bottom)*.5f;
    }
    return 1;
}
static int jnet_menu_row(struct jnet_menu_quad *quads,uint32_t *used,const char *label,
        float x,float y,float scale,float width,uint32_t color) {
    uint32_t start=*used;
    if(!jnet_menu_line(quads,used,label,0,y,scale,width,color))return 0;
    if(*used==start)return 1;
    float left=INFINITY,right=-INFINITY;
    for(uint32_t i=start;i<*used;++i)for(unsigned v=0;v<4;++v) {
        float px=quads[i].position[v][0];
        left=fminf(left,px);right=fmaxf(right,px);
    }
    for(uint32_t i=start;i<*used;++i)for(unsigned v=0;v<4;++v) {
        quads[i].position[v][0]+=x-(left+right)*.5f;
    }
    return 1;
}
static int jnet_menu_inline(struct jnet_menu_quad *quads,uint32_t *used,const char *label,float x,float y,float scale,float width,uint32_t color) {
    uint32_t first=*used;if(!jnet_menu_line(quads,used,label,0,y,scale,width,color))return 0;
    float left=INFINITY,right=-INFINITY;
    for(uint32_t i=first;i<*used;++i)for(unsigned v=0;v<4;++v){left=fminf(left,quads[i].position[v][0]);right=fmaxf(right,quads[i].position[v][0]);}
    for(uint32_t i=first;i<*used;++i)for(unsigned v=0;v<4;++v)quads[i].position[v][0]+=x-(left+right)*.5f;
    return 1;
}
static int jnet_menu_mapping(uintptr_t batch,uintptr_t mapped,size_t *bytes) {
    uintptr_t published,reference,resource,ring;unsigned char state;uint32_t length;
    if(!mapped || !jnet_menu_read(batch+0x20,&published,8) || published!=mapped)return 1;
    if(!jnet_menu_read(batch+0x60,&reference,8) || !reference ||
       !jnet_menu_read(reference+8,&resource,8) || !resource ||
       !jnet_menu_read(resource+0x28,&ring,8) || !ring)return 2;
    if(!jnet_menu_read(ring+0x1c,&state,1) || state!=2)return 3;
    if(!jnet_menu_read(ring+0x48,&length,4) || !length || mapped>UINTPTR_MAX-length)return 4;
    *bytes=length;return 0;
}
static float jnet_menu_notice_alpha(uint64_t elapsed) {
    return elapsed<200?(float)elapsed/200.f:elapsed<1200?1.f:
        elapsed<1800?(1800.f-(float)elapsed)/600.f:0.f;
}
static int jnet_menu_draw_live(uintptr_t menu) {
    (void)jnet_menu_selected_host;
    if(!jnet_menu_visible_root(menu) && !jnet_menu_visible_options(menu) && !jnet_menu_visible_audio(menu)){jnet_menu_font.owner=0;return 1;}
    uintptr_t head,font,mapped;uint32_t capacity,used,width,height;float size,shadow_x,shadow_y;
    unsigned char shadow_enabled;
    uintptr_t batch=menu+0x30;
    if(!jnet_menu_read(menu,&head,8) || !head ||
       !jnet_menu_read(menu+0xbb8,&font,8) || !font ||
       !jnet_menu_read(menu+0xc48,&size,4) || !isfinite(size) || size<=0 || size>1 ||
       !jnet_menu_read(menu+0xc4c,&shadow_enabled,1) ||
       !jnet_menu_read(menu+0xc50,&shadow_x,4) || !isfinite(shadow_x) ||
       !jnet_menu_read(menu+0xc54,&shadow_y,4) || !isfinite(shadow_y) ||
       fabsf(shadow_x)>64 || fabsf(shadow_y)>64 ||
       !jnet_menu_read(batch+0x80,&capacity,4) || !jnet_menu_read(batch+0x84,&used,4) ||
       !jnet_menu_read(batch+0x88,&mapped,8) ||
       !jnet_menu_read(fault_image+0x3763e74,&width,4) ||
       !jnet_menu_read(fault_image+0x3763e78,&height,4) || !width || !height)return 2;
    if(jnet_menu_font.owner!=menu || jnet_menu_font.head!=head || jnet_menu_font.font!=font) {
        memset(&jnet_menu_font,0,sizeof(jnet_menu_font));
        jnet_menu_font.owner=menu;jnet_menu_font.head=head;jnet_menu_font.font=font;
        for(unsigned i=0;i<95;++i)
            jnet_menu_font.present[i]=(unsigned char)jnet_menu_find_glyph(head,(uint16_t)(i+32),&jnet_menu_font.glyphs[i]);
    }
    struct jnet_menu_quad quads[JNET_MENU_TEXT_LIMIT];uint32_t count=0;
    float aspect=(float)height/(float)width,scale=size;
    struct jnet_lobby_view view;jnet_lobby_snapshot(&view);
    static unsigned observed_level=~0u,observed_lighting=~0u;
    unsigned current=jnet_current_level;
    unsigned lighting=jnet_daynight_mode;
    if(observed_lighting!=lighting){jnet_menu_daynight=lighting;observed_lighting=lighting;}
    if(current<12 && observed_level!=current){jnet_menu_level=current;observed_level=current;}
    char text[96];
#define JNET_LINE(label,x,y,w) do {if(!jnet_menu_line(quads,&count,(label),(x)*aspect,(y),scale,(w)*aspect,0xffffffff))return 3;}while(0)
#define JNET_INLINE(label,x,y,w) do {if(!jnet_menu_inline(quads,&count,(label),(x)*aspect,(y),scale,(w)*aspect,0xffffffff))return 3;}while(0)
#define JNET_CENTER(label,x,y,w) do {if(!jnet_menu_centered(quads,&count,(label),(x)*aspect,(y),scale,(w)*aspect,0xffffffff))return 3;}while(0)
    int options=jnet_menu_visible_options(menu),audio=jnet_menu_visible_audio(menu);
    if(audio){

    } else if(options){
    } else if(!jnet_menu_page) {
    } else if(jnet_menu_page==1) {
        const char *labels[]={"Robe",NULL,jnet_menu_edit.draft.infinite_scarf?"Infinite":"Drain",
            jnet_menu_edit.draft.locked_scarf?"Locked":"Grow",NULL,(jnet_character_is_flow(jnet_menu_edit.draft.character)?"fl0w":jnet_character_name(jnet_menu_edit.draft.character)),jnet_menu_edit.draft.character==JNET_CHARACTER_WAR_MACHINE?(jnet_menu_edit.draft.variant==2?"Snow":jnet_menu_edit.draft.variant==1?"Sand":"Stone"):(jnet_character_is_flow(jnet_menu_edit.draft.character)?(jnet_menu_edit.draft.character==JNET_CHARACTER_FLOW_1?"Snake":"Hunter"):(jnet_menu_edit.draft.variant?"White":"Red"))};
        snprintf(text,sizeof(text),"Scarf Length %u",jnet_menu_edit.draft.scarf_segments);
        float widest=.30f;
        const char *width_labels[]={"Scarf Length 30","Rocket Deathmatch","Infinite","Locked"};
        for(unsigned n=0;n<4;++n){float advance=0;
            for(const char *c=width_labels[n];*c;++c)advance+=jnet_menu_font.glyphs[(unsigned char)*c-32].advance;
            widest=fmaxf(widest,fminf(.65f,advance*scale/aspect));
        }
        jnet_appearance_arrow=widest*.5f+.055f;
        for(unsigned i=0;i<jnet_appearance_row_count(jnet_menu_edit.draft.character);++i){float y=jnet_appearance_rows[i];JNET_CENTER("<",jnet_character_column-jnet_appearance_arrow,y,.10f);JNET_CENTER(">",jnet_character_column+jnet_appearance_arrow,y,.10f);
            
            if(i!=0 && i!=4)JNET_CENTER(labels[i]?labels[i]:text,jnet_character_column,y,.65f);}
        JNET_CENTER("Robe",jnet_character_column,.77f,.5f);
        const char *toggle_labels[]={"Blackness","Freezing","Sliding","Windwalls","Fog"};
        const unsigned toggle_values[]={jnet_blackness,jnet_freezing,jnet_sliding,jnet_windwalls,jnet_fog};
        for(unsigned i=0;i<5;++i){float y=jnet_character_toggle_rows[i];
            JNET_INLINE(toggle_labels[i],.49f,y,i==0?.34f:.32f);
            JNET_INLINE("<",.70f,y,.08f);JNET_INLINE(toggle_values[i]?"On":"Off",.80f,y,.14f);JNET_INLINE(">",.90f,y,.08f);
        }
        if(jnet_options_failed)JNET_CENTER("Option unavailable",.64f,-.12f,.62f);
        JNET_CENTER("Apply",0,-.89f,.5f);
        if(jnet_appearance_applied_at && !jnet_appearance_status[0]){
            uint64_t elapsed=GetTickCount64()-jnet_appearance_applied_at;
            float alpha=jnet_menu_notice_alpha(elapsed);
            if(alpha>0 && !jnet_menu_centered(quads,&count,"Applied",0,-.785f,scale,
                .5f*aspect,((uint32_t)(alpha*255.f)<<24)|0x00ffffff))return 3;
        }
        JNET_CENTER(jnet_appearance_status,0,-.785f,.65f);
    } else {
        if(jnet_menu_create) {
            if(!jnet_menu_line(quads,&count,"Create Lobby",-.84f*aspect,.34f,scale*1.35f,1.1f*aspect,0xffffffff))return 3;
            JNET_LINE(jnet_menu_private?"Access: Private":"Access: Public",-.84f,.09f,.8f);
            JNET_LINE(jnet_menu_cmatw?"CMATW: On":"CMATW: Off",-.84f,-.05f,1.1f);
            if(jnet_menu_private)JNET_LINE("Code generated when created",-.84f,-.57f,1.1f);
            JNET_LINE(jnet_menu_chat?"Chat: On":"Chat: Off",-.84f,-.19f,.8f);
            JNET_LINE("Create",-.84f,-.70f,.3f);JNET_LINE("Cancel",-.4f,-.70f,.3f);
        } else {
            JNET_LINE("Public",-.88f,.69f,.5f);JNET_LINE("Refresh",-.88f,-.53f,.30f);
            if(!view.count)JNET_LINE(view.searching?"Searching for lobbies":"No public lobbies",-.88f,.49f,.8f);
            for(unsigned i=0;i<view.count;++i){const struct jnet_lobby_row *row=&view.rows[i];
                snprintf(text,sizeof(text),"%s %.24s's lobby  %u/3",jnet_menu_selected_room==row->room?">":" ",view.names[i],row->players);
                JNET_LINE(text,-.88f,.49f-i*.14f,.82f);
            }
            snprintf(text,sizeof(text),"Code: %s%s",jnet_menu_password,jnet_menu_password_focus?"_":"");
            JNET_LINE(text,-.88f,-.71f,.82f);
            if(!view.room)JNET_LINE(view.busy?"Cancel":"Join",-.55f,-.89f,.23f);
            JNET_LINE("Create",-.22f,-.89f,.28f);
            static char notice[80];static uint64_t notice_at;
            if(strcmp(notice,view.status)){snprintf(notice,sizeof(notice),"%s",view.status);notice_at=GetTickCount64();}
            float notice_alpha=jnet_menu_notice_alpha(GetTickCount64()-notice_at);
            if(notice[0] && notice_alpha>0 && !jnet_menu_line(quads,&count,notice,
                -.88f*aspect,-.35f,scale,.84f*aspect,
                ((uint32_t)(notice_alpha*255.f)<<24)|0x00ffffff))return 3;
            JNET_CENTER(jnet_menu_level==9?"Level 09":jnet_level_names[jnet_menu_level],.60f,.75f,.64f);
            if(!view.room || view.self==view.host){JNET_CENTER("<",jnet_level_left,.48f,.1f);JNET_CENTER(">",jnet_level_right,.48f,.1f);JNET_CENTER("Go",.60f,.11f,.3f);}
            if(jnet_daynight_supported(jnet_menu_level) || jnet_menu_level==7){
                float widest=0;
                for(unsigned n=0;n<jnet_daynight_variants(jnet_menu_level);++n){float advance=0;
                    for(const char *c=jnet_daynight_label(jnet_menu_level,n);*c;++c)advance+=jnet_menu_font.glyphs[(unsigned char)*c-32].advance;
                    widest=fmaxf(widest,fminf(.4f,advance*scale/aspect));
                }
                jnet_variant_arrow=widest*.5f+.045f;
                JNET_CENTER(jnet_daynight_label(jnet_menu_level,jnet_menu_daynight),.60f,.23f,.4f);
                if(!view.room || view.self==view.host){JNET_CENTER("<",.60f-jnet_variant_arrow,.23f,.1f);JNET_CENTER(">",.60f+jnet_variant_arrow,.23f,.1f);}
            }
            if(view.room){
                if(view.anonymous)snprintf(text,sizeof(text),"Random's Lobby");else snprintf(text,sizeof(text),"%.32s's lobby",view.host_name);JNET_CENTER(text,.60f,-.04f,.70f);
                if(view.code[0])JNET_CENTER(view.code,.60f,-.14f,.6f);
            }
            JNET_CENTER("CMATW",.60f,-.28f,.6f);
            for(unsigned i=0;i<view.member_count && i<3;++i)
                if(!jnet_menu_row(quads,&count,view.cmatw?view.member_names[i]:"Companion",.60f*aspect,-.40f-i*.11f,scale,.65f*aspect,0xffffffff))return 3;
            if(view.room)JNET_CENTER("Leave",.60f,-.79f,.3f);
            if(view.total>3)JNET_LINE("Next page",.04f,-.89f,.36f);
        }
    }
    if(jnet_menu_page && !options && !audio)JNET_LINE("Back",-.88f,-.89f,.24f);
#undef JNET_LINE
#undef JNET_CENTER
#undef JNET_INLINE
    size_t bytes=0;
    int mapping=jnet_menu_mapping(batch,mapped,&bytes);
    InterlockedExchange(&JNETMenu[7],mapping);
    if(mapping)return 4;
    uint32_t total=count*(shadow_enabled?2u:1u);
    if(!jnet_menu_batch_room(capacity,used,(void *)mapped,bytes,width,height,total+32))return 5;
    void (__attribute__((ms_abi)) *append)(uintptr_t,const struct jnet_menu_quad *)=(void *)(fault_image+JC_RVA_MenuAppend);
    if(shadow_enabled) {
        float dx=2.f*shadow_x/(float)width;
        float dy=2.f*shadow_y/(float)width;
        for(uint32_t i=0;i<count;++i) {
            struct jnet_menu_quad shadow=quads[i];shadow.color=quads[i].color&0xff000000;
            for(unsigned vertex=0;vertex<4;++vertex) {
                shadow.position[vertex][0]+=dx;shadow.position[vertex][1]+=dy;
            }
            jnet_menu_append_aligned(append,batch,&shadow);
        }
    }
    for(uint32_t i=0;i<count;++i)jnet_menu_append_aligned(append,batch,&quads[i]);
    InterlockedExchange(&JNETMenu[3],(LONG)total);return 0;
}
static float (*jnet_original_menu_text)(uintptr_t,const char *,float,float,float,uint32_t);
static float jnet_menu_text(uintptr_t menu,const char *text,float x,float y,float size,uint32_t color) {
    char title[8];
    if(menu==jnet_menu_layout_owner && jnet_menu_read((uintptr_t)text,title,sizeof(title)) &&
       (!memcmp(title,"Journey",8) || !memcmp(title,"JOURNEY",8)))return 0;
    return jnet_original_menu_text(menu,text,x,y,size,color);
}
static uintptr_t jnet_menu_build(uintptr_t menu) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    InterlockedExchange(&JNETMenu[10],1);
    LONG entries=InterlockedIncrement(&JNETMenu[4]);
    InterlockedExchange(&JNETMenu[5],caller>=fault_image && caller-fault_image<JC_IMAGE_SIZE?(LONG)(caller-fault_image):-1);
    if(entries==1)message("[JNET] menu hook entered; beginning native menu build");
    int layout_scope=caller==fault_image+0x21ac69 && !jnet_menu_layout_owner;
    if(layout_scope){
        jnet_menu_last=menu;
        jnet_menu_finish_close(menu);
        if(jnet_menu_restart_pending){
            uintptr_t begin=0,end=0;
            if(jnet_menu_read(menu+0xc10,&begin,8) && jnet_menu_read(menu+0xc18,&end,8) &&
               begin && end>=begin+0x60 && end-begin<=0x600 && (end-begin)%0x30==0){
                uintptr_t depth=(end-begin)/0x30;
                if(depth>2)jnet_menu_pop_original(menu,depth-2);
                jnet_menu_reset_page();jnet_menu_restart_pending=0;
            }
        }
        jnet_menu_layout_owner=menu;
        if(!jnet_menu_visible_root(menu) && !jnet_menu_closing)jnet_menu_reset_page();
        float cursor=.045f;uint32_t white=0x00ffffff;
        if(jnet_menu_visible_root(menu)){memcpy((void *)(menu+0xc80),&cursor,4);memcpy((void *)(menu+0xc88),&white,4);}
    }
    InterlockedExchange(&JNETMenu[10],2);
    uintptr_t result=jnet_original_menu_build(menu);
    InterlockedExchange(&JNETMenu[10],3);
    if(entries==1)message("[JNET] native menu build returned");
    if(layout_scope)jnet_menu_restore_layout(menu);
    if(caller!=fault_image+0x21ac69 || jnet_menu_drawing)return result;
    jnet_menu_drawing=1;
#if defined(_WIN32) && JNET_MENU_IMAGE_PREVIEW
#if JNET_MENU_IMAGE_INPUT
    InterlockedExchange(&JNETMenu[10],4);
    jnet_menu_controls(menu);
#endif
    uint32_t width=0,height=0;
    jnet_menu_read(fault_image+0x3763e74,&width,4);jnet_menu_read(fault_image+0x3763e78,&height,4);
    InterlockedExchange(&JNETMenu[10],5);
    jnet_menu_publish_images(menu,width,height);
#endif
    InterlockedExchange(&JNETMenu[10],6);
    if(entries==1)message("[JNET] appending menu text");
    int status=jnet_menu_draw_live(menu);
    InterlockedExchange(&JNETMenu[10],7);
    if(entries==1)message("[JNET] menu text append returned");
    LONG frames=InterlockedIncrement(&JNETMenu[0]);
    LONG previous=InterlockedExchange(&JNETMenu[1],status);
    if(!status)InterlockedIncrement(&JNETMenu[2]);
    if(frames==1 || previous!=status) {
        char line[120];snprintf(line,sizeof(line),"[JNET] pause menu draw result=%d (0 drawn,1 hidden,2 state,3 glyph,4 mapping,5 capacity)",status);
        message(line);
    }
    jnet_menu_drawing=0;return result;
}
#endif
