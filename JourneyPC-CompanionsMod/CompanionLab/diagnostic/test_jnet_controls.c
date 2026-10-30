#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "jnet_character.h"
typedef unsigned DWORD;
#define VK_LBUTTON 1
#define VK_RBUTTON 2
#define VK_MBUTTON 4
#define VK_XBUTTON1 5
#define VK_XBUTTON2 6
static unsigned char actor_storage[0x14700],game[0x200],menu[0xe00];
static unsigned char *actor=actor_storage+0xb0;
static uintptr_t fault_image,jnet_menu_last;
static int jnet_bindings[3][8]={{'C','B','V','G'},{0},{0}};
static unsigned jnet_binding_down[8],jnet_sprint_down,jnet_spotlight_down,jnet_crouch_active,jnet_controls_ready=1;
static unsigned jnet_chat_focus;
static int jnet_creature_bindings[JNET_CHARACTER_COUNT][3][8];
static unsigned jnet_creature_down[8],jnet_attack_down,jnet_grab_down,jnet_fly_up_down,jnet_fly_down_down,jnet_level_down;
static unsigned keys[256],buttons,foreground=1,calls,pad_polls,key_polls;
static uintptr_t failed_read;
static char last[32];
static int jnet_menu_read(uintptr_t at,void *out,size_t n){if(at==failed_read)return 0;memcpy(out,(void *)at,n);return 1;}
static int own_read(void *ctx,uintptr_t at,void *out,size_t n){(void)ctx;return jnet_menu_read(at,out,n);}
static void *GetForegroundWindow(void){return NULL;}
static void GetWindowThreadProcessId(void *w,DWORD *p){(void)w;*p=foreground;}
static unsigned GetCurrentProcessId(void){return 1;}
static unsigned GetAsyncKeyState(unsigned key){assert(key<256);++key_polls;return keys[key]?0x8000:0;}
static unsigned jnet_controls_buttons(void){++pad_polls;return buttons;}
static void queue(uintptr_t controller,const char *name){
    assert(controller==(uintptr_t)actor+0xa390);++calls;strcpy(last,name);
    unsigned pending;memcpy(&pending,(void *)(controller+0x6734),4);++pending;
    memcpy((void *)(controller+0x6734),&pending,4);
}
#include "controls_fixture.h"
static void set(unsigned offset,unsigned value){memcpy(actor+0xa390+offset,&value,4);}
static void tick(unsigned kind){set(0x6734,0);jnet_controls_tick((uintptr_t)actor-0xb0,(uintptr_t)game,kind);}
static unsigned char native_passive;
static char current_action[32];
static unsigned char passive(uintptr_t dude){assert(dude==(uintptr_t)actor);return native_passive;}
static const char *current(uintptr_t controller){assert(controller==(uintptr_t)actor+0xa390);return current_action;}
int main(void){
    jnet_controls_passive_original=passive;jnet_controls_current_action=current;
    strcpy(current_action,"Crouching");assert(jnet_controls_passive((uintptr_t)actor));
    strcpy(current_action,"Idle");assert(!jnet_controls_passive((uintptr_t)actor));
    native_passive=1;assert(jnet_controls_passive((uintptr_t)actor));native_passive=0;

    fault_image=(uintptr_t)queue-0xaea60;jnet_menu_last=(uintptr_t)menu;
    uintptr_t barn=(uintptr_t)actor-0xb0;memcpy(game+0x1e0,&barn,8);
    const char *names[]={"Crouch","SlideJump2","SlideJump1","CrouchToIdle","JNETSpin3","RollF","JNETFlipL","JNETFlipR"};
    set(0x64a0,8);
    for(unsigned i=0;i<8;++i)strcpy((char *)actor+0xa390+0x33a0+i*0x40+0x28,names[i]);
    assert(!jnet_controls_action((uintptr_t)actor,"Missing"));
    set(0x6734,16);assert(!jnet_controls_action((uintptr_t)actor,"Crouch"));
    set(0x64a0,197);assert(!jnet_controls_action((uintptr_t)actor,"Crouch"));set(0x64a0,8);
    keys['B']=1;tick(0);assert(calls==1 && !strcmp(last,"SlideJump2"));tick(0);assert(calls==1);
    keys['B']=0;tick(0);keys['B']=1;tick(1);assert(calls==1);
    keys['B']=0;keys['C']=1;tick(0);assert(jnet_crouch_active && !strcmp(last,"Crouch"));
    menu[0xd68]=1;keys['C']=0;tick(0);assert(jnet_crouch_active);
    menu[0xd68]=0;tick(0);assert(jnet_crouch_active);
    float move=.005f;memcpy(actor-0xb0+0x30,&move,4);tick(0);assert(jnet_crouch_active);
    move=.5f;memcpy(actor-0xb0+0x30,&move,4);tick(0);
    assert(!jnet_crouch_active && !strcmp(last,"CrouchToIdle"));
    move=0;memcpy(actor-0xb0+0x30,&move,4);
    keys['C']=1;tick(0);assert(jnet_crouch_active);
    keys['C']=0;tick(0);assert(jnet_crouch_active);
    jnet_controls_queue_original=queue;
    unsigned protected_calls=calls;
    jnet_controls_queue((uintptr_t)actor+0xa390,"CrouchToIdle");
    assert(calls==protected_calls+1 && !jnet_crouch_active);
    jnet_controls_queue((uintptr_t)actor+0xa390,"LandHard");
    jnet_controls_queue((uintptr_t)actor+0xa390,"SlideJump2");
    assert(calls==protected_calls+3 && !jnet_crouch_active);
    keys['C']=1;tick(0);assert(jnet_crouch_active);
    keys['C']=0;tick(0);keys['C']=1;tick(0);assert(!jnet_crouch_active);keys['C']=0;tick(0);
    jnet_creature_bindings[2][0][5]='G';keys['G']=1;tick(2);assert(jnet_spotlight_down);menu[0xd68]=1;tick(2);assert(!jnet_spotlight_down);
    menu[0xd68]=0;foreground=2;keys['V']=1;unsigned before=calls;tick(0);assert(calls==before);
    foreground=1;tick(0);assert(calls==before);keys['V']=0;tick(0);keys['V']=1;tick(0);assert(!strcmp(last,"SlideJump1"));
    keys['V']=0;tick(0);keys['V']=1;tick(0);assert(!strcmp(last,"JNETSpin3"));
    keys['V']=0;tick(0);keys['V']=1;tick(0);assert(!strcmp(last,"SlideJump1"));
    jnet_bindings[2][1]=16;buttons=1u<<15;tick(0);assert(!strcmp(last,"SlideJump2"));
    buttons=0;tick(0);jnet_bindings[1][1]=5;keys[VK_XBUTTON2]=1;tick(0);assert(!strcmp(last,"SlideJump2"));
    memset(keys,0,sizeof(keys));buttons=0;tick(0);
    for(unsigned device=0;device<3;++device)for(unsigned action=4;action<7;++action){
        memset(jnet_bindings,0,sizeof(jnet_bindings));
        jnet_bindings[device][action]=device==0?'F':device==1?1:11;
        if(device==0)keys['F']=1;else if(device==1)keys[VK_LBUTTON]=1;else buttons=1u<<10;
        before=calls;tick(0);assert(calls==before+1 && !strcmp(last,names[action+1]));
        tick(0);assert(calls==before+1);
        memset(keys,0,sizeof(keys));buttons=0;tick(0);
    }
    memset(jnet_bindings,0,sizeof(jnet_bindings));
    before=calls;
    for(unsigned device=0;device<3;++device){
        for(unsigned kind=0;kind<JNET_CHARACTER_COUNT;++kind)jnet_creature_bindings[kind][device][6]=device==0?'F':device==1?5:16;
        for(unsigned kind=0;kind<JNET_CHARACTER_COUNT;++kind){
            keys['F']=keys[VK_XBUTTON2]=0;buttons=0;tick(kind);assert(!jnet_sprint_down);
            if(device==0)keys['F']=1;else if(device==1)keys[VK_XBUTTON2]=1;else buttons=1u<<15;
            tick(kind);assert(jnet_sprint_down==(unsigned)jnet_character_is_creature(kind));
            tick(kind);assert(jnet_sprint_down==(unsigned)jnet_character_is_creature(kind));
            menu[0xd68]=1;tick(kind);assert(!jnet_sprint_down);menu[0xd68]=0;
            foreground=2;tick(kind);assert(!jnet_sprint_down);foreground=1;
        }
        for(unsigned kind=0;kind<JNET_CHARACTER_COUNT;++kind)jnet_creature_bindings[kind][device][6]=0;
    }
    assert(calls==before);
    memset(keys,0,sizeof(keys));buttons=0;
    jnet_creature_bindings[JNET_CHARACTER_WHALE][0][1]='D';
    jnet_creature_bindings[JNET_CHARACTER_WHALE][0][2]='L';
    jnet_creature_bindings[JNET_CHARACTER_WHALE][0][3]='Q';
    keys['D']=keys['L']=keys['Q']=1;tick(JNET_CHARACTER_WHALE);
    assert(jnet_fly_down_down && jnet_level_down);
    tick(JNET_CHARACTER_LONGTAIL);
    assert(!jnet_fly_down_down && !jnet_level_down);
    before=calls;jnet_controls_ready=0;keys['C']=1;tick(0);assert(calls==before);
    memset(keys,0,sizeof(keys));buttons=0;
    jnet_controls_ready=1;
    jnet_creature_bindings[JNET_CHARACTER_LONGTAIL][0][5]='G';keys['G']=1;
    tick(JNET_CHARACTER_LONGTAIL);assert(jnet_grab_down);tick(JNET_CHARACTER_LONGTAIL);assert(jnet_grab_down);
    keys['G']=0;tick(JNET_CHARACTER_LONGTAIL);assert(!jnet_grab_down);
    keys['G']=1;menu[0xd68]=1;tick(JNET_CHARACTER_LONGTAIL);assert(!jnet_grab_down);
    menu[0xd68]=0;tick(JNET_CHARACTER_WHALE);assert(!jnet_grab_down);
    memset(keys,0,sizeof(keys));buttons=0;memset(jnet_creature_bindings,0,sizeof(jnet_creature_bindings));
    for(unsigned device=0;device<3;++device){
        memset(jnet_creature_bindings,0,sizeof(jnet_creature_bindings));
        jnet_creature_bindings[JNET_CHARACTER_WAR_MACHINE][device][7]=device==0?'F':device==1?5:8;
        memset(keys,0,sizeof(keys));buttons=0;
        if(device==0)keys['F']=1;else if(device==1)keys[VK_XBUTTON2]=1;else buttons=1u<<7;
        tick(JNET_CHARACTER_WAR_MACHINE);assert(jnet_attack_down && !jnet_spotlight_down);
        tick(JNET_CHARACTER_WAR_MACHINE);assert(jnet_attack_down);
        menu[0xd68]=1;tick(JNET_CHARACTER_WAR_MACHINE);assert(!jnet_attack_down);menu[0xd68]=0;
        foreground=2;tick(JNET_CHARACTER_WAR_MACHINE);assert(!jnet_attack_down);foreground=1;
        tick(JNET_CHARACTER_FLOW_1);assert(!jnet_attack_down);
        memset(keys,0,sizeof(keys));buttons=0;tick(JNET_CHARACTER_WAR_MACHINE);assert(!jnet_attack_down);
    }
    memset(jnet_bindings,0,sizeof(jnet_bindings));memset(keys,0,sizeof(keys));buttons=0;tick(0);
    for(unsigned key=1;key<256;++key){
        jnet_bindings[0][1]=(int)key;keys[key]=1;before=calls;tick(0);
        assert(calls==before+1 && !strcmp(last,"SlideJump2"));keys[key]=0;tick(0);
    }
    jnet_bindings[0][1]=0;
    const unsigned mouse_keys[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
    for(unsigned button=1;button<=5;++button){
        jnet_bindings[1][1]=(int)button;keys[mouse_keys[button-1]]=1;before=calls;tick(0);
        assert(calls==before+1);keys[mouse_keys[button-1]]=0;tick(0);
    }
    jnet_bindings[1][1]=0;
    for(unsigned button=1;button<=16;++button){
        jnet_bindings[2][1]=(int)button;buttons=1u<<(button-1);before=calls;tick(0);
        assert(calls==before+1);buttons=0;tick(0);
    }
    jnet_bindings[2][1]=0;jnet_bindings[2][7]=7;
    unsigned pads=pad_polls,keys_before=key_polls;tick(0);assert(pad_polls==pads && key_polls==keys_before);
    jnet_bindings[2][1]=11;tick(0);assert(pad_polls==pads+1);pads=pad_polls;
    foreground=2;tick(0);foreground=1;menu[0xd68]=1;tick(0);menu[0xd68]=0;
    jnet_chat_focus=1;tick(0);jnet_chat_focus=0;
    assert(pad_polls==pads && key_polls==keys_before);
    buttons=1u<<10;before=calls;tick(0);tick(0);assert(calls==before);
    buttons=0;tick(0);buttons=1u<<10;tick(0);assert(calls==before+1);
    buttons=0;memset(jnet_creature_bindings,0,sizeof(jnet_creature_bindings));
    jnet_creature_bindings[JNET_CHARACTER_LONGTAIL][0][5]='G';keys['G']=1;
    tick(JNET_CHARACTER_LONGTAIL);assert(jnet_grab_down);
    jnet_controls_ready=0;tick(JNET_CHARACTER_LONGTAIL);assert(!jnet_grab_down);jnet_controls_ready=1;
    tick(JNET_CHARACTER_LONGTAIL);assert(jnet_grab_down);
    failed_read=(uintptr_t)game+0x1e0;tick(JNET_CHARACTER_LONGTAIL);assert(!jnet_grab_down);failed_read=0;
    tick(JNET_CHARACTER_LONGTAIL);assert(jnet_grab_down);
    failed_read=(uintptr_t)menu+0xd68;tick(JNET_CHARACTER_LONGTAIL);assert(!jnet_grab_down);failed_read=0;
    tick(JNET_CHARACTER_LONGTAIL);assert(jnet_grab_down);
    uintptr_t missing=0;memcpy(game+0x1e0,&missing,8);tick(JNET_CHARACTER_LONGTAIL);assert(!jnet_grab_down);memcpy(game+0x1e0,&barn,8);
    puts("Controls: all 255 assignable virtual-key values, five mouse buttons and sixteen pad bindings; no idle/unbound controller polling, focus/menu/chat release edges, and Grab reset after unavailable input passed.");
}
