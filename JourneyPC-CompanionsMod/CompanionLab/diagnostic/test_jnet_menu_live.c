#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include "jnet_menu_draw.h"
#include "menu_code.h"

#define __declspec(x)
typedef int32_t LONG;
typedef size_t SIZE_T;
#define JC_RVA_MenuAppend 0x1024a0
#define JC_IMAGE_SIZE 0x4100000
static uintptr_t fault_image;
static LONG JNETFrames;
static unsigned char menu[0xe00],nodes[96][0x48],storage[2048*112];
static int inside(uintptr_t address,size_t bytes,const void *base,size_t size) {
    uintptr_t start=(uintptr_t)base;
    return address>=start && address-start<=size && bytes<=size-(address-start);
}
static uint64_t GetTickCount64(void){return 1000;}
static void *GetCurrentProcess(void){return NULL;}
static int ReadProcessMemory(void *process,const void *source,void *out,size_t bytes,SIZE_T *got) {
    (void)process;uintptr_t address=(uintptr_t)source;
    if(!inside(address,bytes,menu,sizeof(menu)) && !inside(address,bytes,nodes,sizeof(nodes)) &&
       !inside(address,bytes,(void *)fault_image,0x4100000))return 0;
    memcpy(out,source,bytes);*got=bytes;return 1;
}
static LONG InterlockedExchange(volatile LONG *p,LONG value){LONG old=*p;*p=value;return old;}
static LONG InterlockedIncrement(volatile LONG *p){return ++*p;}
static void message(const char *text){(void)text;}
#include "jnet_menu_live.h"

static void pointer(unsigned char *where,const void *value){uintptr_t p=(uintptr_t)value;memcpy(where,&p,8);}
static void number(unsigned char *where,uint32_t value){memcpy(where,&value,4);}
static uint32_t number_at(unsigned char *where){uint32_t n;memcpy(&n,where,4);return n;}
static unsigned char *tree(unsigned lo,unsigned hi) {
    if(lo>=hi)return nodes[95];
    unsigned mid=lo+(hi-lo)/2;unsigned char *node=nodes[mid];
    pointer(node,tree(lo,mid));pointer(node+0x10,tree(mid+1,hi));
    uint16_t code=(uint16_t)(mid+32);memcpy(node+0x1c,&code,2);
    struct jnet_menu_glyph glyph={.1f,.8f,.2f,.2f,0,0,.4f,.8f,.5f};
    memcpy(node+0x20,&glyph,sizeof(glyph));return node;
}
static void string(unsigned char *p,const char *text) {
    size_t length=strlen(text);memcpy(p,text,length);memcpy(p+16,&length,8);
    uint64_t capacity=15;memcpy(p+24,&capacity,8);
}
static unsigned text_calls;
static float text_y[4],text_size[4];
static uint32_t text_color[4];
static float native_text_probe(uintptr_t m,const char *text,float x,float y,float size,uint32_t color) {
    assert(m==(uintptr_t)menu && text_calls<4);
    assert(x==-.4f);
    assert(!strcmp(text,"Options") || !strcmp(text,"Einstellungen"));
    text_y[text_calls]=y;text_size[text_calls]=size;text_color[text_calls++]=color;return 42;
}
static float node_y;
static uintptr_t menu_nodes_probe(uintptr_t m,uintptr_t v,uintptr_t n,uint32_t f,float x,float *y){(void)m;(void)v;(void)n;(void)f;(void)x;node_y=*y;return 99;}
static uintptr_t original(uintptr_t value){return value;}
static uintptr_t original_nodes(uintptr_t m,uintptr_t v,uintptr_t n,uint32_t f,float x,float *y) {
    assert(m==(uintptr_t)menu && v==12 && n==34 && f==7 && x==.25f && *y==.5f);
    return 99;
}
static unsigned native_input_calls;
static unsigned native_pop_calls;
static uintptr_t native_pop_probe(uintptr_t m,uintptr_t count){
    uintptr_t end=0;memcpy(&end,(void *)(m+0xc18),8);end-=count*0x30;
    memcpy((void *)(m+0xc18),&end,8);++native_pop_calls;return 1;
}
static void native_input_probe(uintptr_t m,uintptr_t r){(void)m;(void)r;++native_input_calls;}
static void navigation(unsigned char *image) {
    unsigned char *path=image+0x2000000,*rows=image+0x2000100,*vector=image+0x2000400;
    unsigned char *start=image+0x2000500,*options=start+0x40;
    string(path,"Root");string(path+0x30,"Menu");string(start,"Start");string(options,"Options");
    memset(menu+0xd00,0,0x60);
    pointer(menu+0xc10,path);pointer(menu+0xc18,path+0x60);pointer(path+0x58,start);
    pointer(vector,rows);pointer(vector+8,rows+256);pointer(rows,start);pointer(rows+128,options);
    rows[0x7a]=rows[128+0x7a]=1;jnet_original_menu_input=native_input_probe;
    menu[0xcc8]=1;jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(native_input_calls==1 && !jnet_menu_page);
    menu[0xcc8]=0;string(start,"JNETCharacter");
    assert(jnet_menu_action((uintptr_t)menu,(uintptr_t)start)==1);
    assert(jnet_menu_page==1 && jnet_menu_key_active);
    menu[0xce0]=1;jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_key_click && jnet_menu_key_x<jnet_character_column);
    jnet_menu_edit.draft.character=JNET_CHARACTER_LONGTAIL;
    jnet_menu_key_row=6;jnet_menu_key_click=0;
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_key_click && jnet_menu_key_x<jnet_character_column && jnet_menu_key_y==-.68f);
    jnet_menu_key_row=10;jnet_menu_key_click=0;
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_key_click && jnet_menu_key_x==.70f && jnet_menu_key_y==-.67f);
    assert(jnet_blackness==1);
    jnet_menu_key_row=11;jnet_menu_key_click=0;
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_key_click && jnet_menu_key_x==.70f && jnet_menu_key_y==-.81f);
    jnet_menu_edit.draft.character=JNET_CHARACTER_WAYFARER;
    menu[0xce0]=0;menu[0xc98]=1;jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(!jnet_menu_page && !jnet_menu_key_click && jnet_menu_input_blocked);
    menu[0xc98]=0;menu[0xc99]=1;++JNETFrames;
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_input_blocked);
    string(start,"JNETWorld");
    assert(jnet_menu_action((uintptr_t)menu,(uintptr_t)start)==1 && !jnet_menu_page);
    menu[0xc99]=0;menu[0xd41]=1;++JNETFrames;
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);assert(jnet_menu_input_blocked);
    menu[0xd41]=0;++JNETFrames;
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);assert(!jnet_menu_input_blocked);
    string(start,"JNETWorld");
    assert(jnet_menu_action((uintptr_t)menu,(uintptr_t)start)==1 && jnet_menu_page==2);
    jnet_lobby_view.count=2;jnet_lobby_view.rows[0]=(struct jnet_lobby_row){123,42,1,0};
    jnet_lobby_view.rows[1]=(struct jnet_lobby_row){124,43,1,0};
    jnet_menu_key_row=0;menu[0xd10]=1;jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_selected_room==123 && jnet_menu_selected_host==42);
    menu[0xd10]=0;menu[0xcc8]=1;jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(jnet_menu_selected_room==124 && jnet_menu_selected_host==43);
    menu[0xcc8]=0;memset(&jnet_lobby_view,0,sizeof(jnet_lobby_view));

    jnet_menu_reset_page();
    for(unsigned i=0;i<2;++i){
        string(path+0x30,i?"Graphics":"Audio");
        menu[0xcc8]=1;jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
        assert(native_input_calls==5+i);menu[0xcc8]=0;
    }
    string(path+0x30,"Menu");string(path+0x60,"Controls");string(path+0x90,"Longtail");
    pointer(menu+0xc18,path+0xc0);float speed=2;memcpy(menu+0xd74,&speed,4);
    jnet_menu_pop_original=native_pop_probe;menu[0xc98]=1;menu[0xc99]=0;jnet_menu_page=1;
    uintptr_t escape=1;memcpy((void *)(fault_image+0x3c47b60+27*8),&escape,8);
    float opacity=.75f;memcpy(menu+0xd6c,&opacity,4);
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    uintptr_t end=0;memcpy(&end,menu+0xc18,8);memcpy(&speed,menu+0xd70,4);
    assert(native_pop_calls==0 && end==(uintptr_t)path+0xc0 && speed==-2);
    assert(jnet_menu_closing && jnet_menu_page==1 && jnet_menu_input_blocked);
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    jnet_menu_finish_close((uintptr_t)menu);
    assert(jnet_menu_closing && native_pop_calls==0);
    menu[0xd68]=0;opacity=0;memcpy(menu+0xd6c,&opacity,4);
    jnet_menu_finish_close((uintptr_t)menu);
    memcpy(&end,menu+0xc18,8);
    assert(!jnet_menu_closing && !jnet_menu_page && native_pop_calls==1 && end==(uintptr_t)path+0x60);
    speed=2;memcpy(menu+0xd70,&speed,4);
    menu[0xd68]=1;opacity=.1f;memcpy(menu+0xd6c,&opacity,4);
    jnet_menu_input((uintptr_t)menu,(uintptr_t)vector);
    assert(!jnet_menu_closing && !jnet_menu_page);
    menu[0xc98]=0;menu[0xc99]=0;jnet_menu_input_blocked=0;
    escape=0;memcpy((void *)(fault_image+0x3c47b60+27*8),&escape,8);
    pointer(menu+0xc10,menu+0xd00);pointer(menu+0xc18,menu+0xd60);
    string(menu+0xd00,"Root");string(menu+0xd30,"Menu");
}
static void mouse_input(unsigned char *image) {
    unsigned char saved_menu[sizeof(menu)],saved_dimensions[8];
    memcpy(saved_menu,menu,sizeof(menu));memcpy(saved_dimensions,image+0x3763e74,8);
    float x,y;const unsigned dimensions[][2]={{1080,1920},{1440,3440},{1200,1600}};
    memset(menu+0xc98,0,0xc0);
    pointer(menu+0xc10,image+0x2000000);pointer(menu+0xc18,image+0x2000060);
    string(image+0x2000000,"Root");string(image+0x2000030,"Menu");menu[0xd68]=1;
    jnet_menu_input_blocked=0;jnet_menu_closing=0;
    for(unsigned page=1;page<=2;++page)for(unsigned i=0;i<3;++i){
        jnet_menu_page=page;
        number(image+0x3763e74,dimensions[i][0]);number(image+0x3763e78,dimensions[i][1]);
        float point[2]={.87f*dimensions[i][1]/dimensions[i][0],.48f};
        memcpy(menu+0xc78,point,sizeof(point));
        menu[0xd40]=menu[0xd41]=0;jnet_menu_input((uintptr_t)menu,0);
        assert(jnet_menu_pointer((uintptr_t)menu,&x,&y) && fabsf(x-.87f)<.00001f && y==.48f);
        menu[0xd40]=1;jnet_menu_input((uintptr_t)menu,0);
        assert(jnet_menu_take_click((uintptr_t)menu,&x,&y) && fabsf(x-.87f)<.00001f && y==.48f);
        assert(!jnet_menu_take_click((uintptr_t)menu,&x,&y));
        jnet_menu_input((uintptr_t)menu,0);assert(!jnet_menu_take_click((uintptr_t)menu,&x,&y));
        menu[0xd41]=1;jnet_menu_input((uintptr_t)menu,0);assert(!jnet_menu_click.pending);
        menu[0xd40]=menu[0xd41]=0;jnet_menu_input((uintptr_t)menu,0);
        menu[0xd40]=1;jnet_menu_input((uintptr_t)menu,0);
        jnet_menu_reset_page();assert(!jnet_menu_take_click((uintptr_t)menu,&x,&y));
    }
    float invalid[2]={NAN,0};memcpy(menu+0xc78,invalid,sizeof(invalid));
    assert(!jnet_menu_pointer((uintptr_t)menu,&x,&y));
    invalid[0]=INFINITY;memcpy(menu+0xc78,invalid,sizeof(invalid));assert(!jnet_menu_pointer((uintptr_t)menu,&x,&y));
    invalid[0]=0;invalid[1]=2;memcpy(menu+0xc78,invalid,sizeof(invalid));assert(!jnet_menu_pointer((uintptr_t)menu,&x,&y));
    number(image+0x3763e74,0);assert(!jnet_menu_pointer((uintptr_t)menu,&x,&y));
    memset(menu+0xc78,0,8);menu[0xd40]=menu[0xd41]=0;
    memcpy(menu,saved_menu,sizeof(menu));memcpy(image+0x3763e74,saved_dimensions,8);
    puts("Native custom-menu mouse: both pages, aspect ratios, press/release, held-button suppression, one-shot consumption and stale/invalid input passed.");
}
int main(void) {
    jnet_menu_page=1;jnet_menu_create=1;jnet_menu_password_focus=1;
    strcpy(jnet_appearance_status,"Appearance applied - save submitted");
    jnet_menu_reset_page();
    assert(!jnet_menu_page && !jnet_menu_create && !jnet_menu_password_focus && !jnet_appearance_status[0]);
    assert(jnet_menu_level_step(8,1)==10 && jnet_menu_level_step(10,1)==11);
    assert(jnet_menu_level_step(11,1)==9 && jnet_menu_level_step(9,1)==0);
    assert(jnet_menu_level_step(9,-1)==11 && jnet_menu_level_step(10,-1)==8);
    assert(jnet_menu_level_step(0,-1)==9);
    unsigned char *image=mmap(NULL,0x4100000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);fault_image=(uintptr_t)image;load_menu_code(image);
    const char *long_names[]={"%JNET_CONTROL_CHARACTER%","%JNET_CREATURE_3_2_6%"};
    for(unsigned i=0;i<2;++i){
        unsigned char *object=image+0x2001000,*data=object+64;
        size_t length=strlen(long_names[i]);uint64_t capacity=31;
        memcpy(data,long_names[i],length+1);pointer(object,data);
        memcpy(object+16,&length,8);memcpy(object+24,&capacity,8);
        assert(jnet_menu_string_is((uintptr_t)object,long_names[i]));
        assert(!jnet_menu_string_is((uintptr_t)object,"%JNET_CONTROL_CHARACTEX%"));
        pointer(object,(void *)1);assert(!jnet_menu_string_is((uintptr_t)object,long_names[i]));
        pointer(object,data);capacity=15;memcpy(object+24,&capacity,8);
        assert(!jnet_menu_string_is((uintptr_t)object,long_names[i]));
    }
    assert(!mprotect(image,0x110000,PROT_READ|PROT_EXEC));
    number(image+0x3763e74,1920);number(image+0x3763e78,1080);
    nodes[95][0x19]=1;pointer(nodes[95]+8,tree(0,95));pointer(menu,nodes[95]);
    pointer(menu+0xbb8,nodes[95]);float scale=.07f;memcpy(menu+0xc48,&scale,4);
    pointer(menu+0xc10,menu+0xd00);pointer(menu+0xc18,menu+0xd60);
    string(menu+0xd00,"Root");string(menu+0xd30,"Menu");menu[0xd68]=1;
    navigation(image);
    mouse_input(image);
    number(menu+0xb0,2048);pointer(menu+0xb8,storage);
    pointer(menu+0x50,storage);pointer(menu+0x90,menu+0x200);
    pointer(menu+0x208,menu+0x220);pointer(menu+0x248,menu+0x280);
    menu[0x29c]=2;number(menu+0x2c8,sizeof(storage));
    float geometry[4]={-.6f,-.6f,1.6f,.538f},padding=.06f;
    memcpy(menu+0x20,geometry,sizeof(geometry));memcpy(menu+0xc5c,&padding,4);
    for(unsigned ratio=0;ratio<3;++ratio) {
        uint32_t widths[]={1920,1440,2560};number(image+0x3763e74,widths[ratio]);
        float x=.25f,y=.5f,shifted[4],aspect=1080.f/widths[ratio];
        jnet_menu_layout_owner=(uintptr_t)menu;
        assert(!jnet_menu_corner((uintptr_t)menu,fault_image+1,&x,&y));
        assert(x==.25f && y==.5f && !memcmp(menu+0x20,geometry,sizeof(geometry)));
        assert(jnet_menu_corner((uintptr_t)menu,fault_image+0x105ca3,&x,&y));
        memcpy(shifted,menu+0x20,sizeof(shifted));
        float left=shifted[0]-shifted[2]*.5f+padding*.5f;
        float right=shifted[0]+shifted[2]*.5f-padding*.5f;
        assert(fabsf(x/aspect+.88f)<.00001f && fabsf(y-(-.88f+7.f*scale))<.00001f);
        assert(left<x && x<right && left>=-aspect && right<=aspect);
        assert(fabsf((right-left)/aspect-.84f)<.00001f);
        assert(y-7*scale>shifted[1]-shifted[3]*.5f+padding);
        assert(fabsf(y-7*scale+.88f)<.00001f);
        assert(!jnet_menu_corner((uintptr_t)menu,fault_image+0x105ca3,&x,&y));
        jnet_menu_restore_layout((uintptr_t)menu);
        assert(!jnet_menu_layout_owner && !memcmp(menu+0x20,geometry,sizeof(geometry)));
    }
    number(image+0x3763e74,1920);
    jnet_original_menu_nodes=original_nodes;
    float cursor=.5f;
    assert(jnet_menu_nodes((uintptr_t)menu,12,34,7,.25f,&cursor)==99);
    jnet_original_menu_nodes=menu_nodes_probe;jnet_menu_layout_owner=(uintptr_t)menu;
    string(menu+0xd90,"Options");cursor=.40f;
    assert(jnet_menu_nodes((uintptr_t)menu,12,(uintptr_t)(menu+0xd90),0,-.20f,&cursor)==99);
    assert(fabsf(node_y-.40f)<.00001f);
    jnet_original_menu_text=native_text_probe;
    const char *labels[]={"Options","Einstellungen"};
    for(unsigned i=0;i<2;++i){
        strcpy((char *)menu+0xd90,labels[i]);text_calls=0;
        assert(jnet_menu_text((uintptr_t)menu,(const char *)(menu+0xd90),-.4f,.19f,.07f,0xffffffff)==42);
        assert(text_calls==1 && text_y[0]==.19f);
    }
    jnet_menu_page=1;node_y=10;
    assert(!jnet_menu_nodes((uintptr_t)menu,12,(uintptr_t)(menu+0xd90),0,-.20f,&cursor) && node_y==10);
    jnet_menu_page=0;jnet_menu_layout_owner=0;
    puts("Native menu corner: three aspect ratios, text inside mouse bounds, four visible rows, recursive refusal, restoration and unrelated-call passthrough passed.");
    /* Match the Windows calling convention of the isolated native append. */
    assert(jnet_menu_visible_root((uintptr_t)menu));
    jnet_menu_page=2;
    menu[0xda0]=1;assert(jnet_menu_draw_live((uintptr_t)menu)==0);menu[0xda0]=0;
    assert(number_at(menu+0xb4)>20);number(menu+0xb4,0);
    menu[0xd68]=0;assert(jnet_menu_draw_live((uintptr_t)menu)==1);menu[0xd68]=1;
    string(menu+0xd30,"Options");assert(jnet_menu_visible_options((uintptr_t)menu));assert(jnet_menu_draw_live((uintptr_t)menu)==0);number(menu+0xb4,0);
    const char *pages[]={"Options","Audio","Graphics","Controls","Quit","Back"};
    for(unsigned i=0;i<6;++i){
        memset(menu+0xd30,0,32);string(menu+0xd30,pages[i]);
        jnet_menu_layout_owner=(uintptr_t)menu;float x=0,y=0;
        assert(jnet_menu_visible_native((uintptr_t)menu));
        assert(jnet_menu_corner((uintptr_t)menu,fault_image+0x105ca3,&x,&y));
        jnet_menu_restore_layout((uintptr_t)menu);
        jnet_menu_native_height=.35f;jnet_menu_layout_owner=(uintptr_t)menu;
        assert(jnet_menu_corner((uintptr_t)menu,fault_image+0x105ca3,&x,&y));
        assert(fabsf(y-(i==3?.31f:(.35f-.88f)))<.00001f);
        jnet_menu_restore_layout((uintptr_t)menu);
    }
    memset(menu+0xd30,0,32);string(menu+0xd30,"Audio");
    memset(menu+0xd90,0,32);string(menu+0xd90,"Back");
    jnet_menu_layout_owner=(uintptr_t)menu;cursor=-.60f;
    assert(jnet_menu_nodes((uintptr_t)menu,12,(uintptr_t)(menu+0xd90),0,-.20f,&cursor)==99);
    assert(fabsf(cursor-(-.60f))<.00001f);jnet_menu_layout_owner=0;
    memset(menu+0xd30,0,32);string(menu+0xd30,"Menu");
    pointer(menu+0x50,storage+1);
    assert(jnet_menu_draw_live((uintptr_t)menu)==4 && JNETMenu[7]==1);
    pointer(menu+0x50,storage);pointer(menu+0x90,NULL);
    assert(jnet_menu_draw_live((uintptr_t)menu)==4 && JNETMenu[7]==2);
    pointer(menu+0x90,menu+0x200);menu[0x29c]=0;
    assert(jnet_menu_draw_live((uintptr_t)menu)==4 && JNETMenu[7]==3);
    menu[0x29c]=2;number(menu+0x2c8,0);
    assert(jnet_menu_draw_live((uintptr_t)menu)==4 && JNETMenu[7]==4);
    number(menu+0x2c8,112);
    assert(jnet_menu_draw_live((uintptr_t)menu)==5 && number_at(menu+0xb4)==0);
    number(menu+0x2c8,sizeof(storage));number(menu+0xb0,1);
    assert(jnet_menu_draw_live((uintptr_t)menu)==5 && number_at(menu+0xb4)==0);
    number(menu+0xb0,2048);
    assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    assert(number_at(menu+0xb4)==(uint32_t)JNETMenu[3] && JNETMenu[3]>20);
    uint32_t foreground=number_at(menu+0xb4);
    number(menu+0xb4,0);menu[0xc4c]=1;
    float sx=1.75f,sy=-1.75f;memcpy(menu+0xc50,&sx,4);memcpy(menu+0xc54,&sy,4);
    number(menu+0xb0,foreground+32);
    assert(jnet_menu_draw_live((uintptr_t)menu)==5 && number_at(menu+0xb4)==0);
    number(menu+0xb0,2048);
    assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    assert(number_at(menu+0xb4)==foreground*2);
    for(uint32_t i=0;i<foreground;++i) {
        unsigned char *shadow=storage+i*112,*text=storage+(foreground+i)*112;
        for(unsigned vertex=0;vertex<4;++vertex) {
            float a,b;
            assert(number_at(shadow+vertex*28+24)==0xff000000);
            assert(number_at(text+vertex*28+24)==0xffffffff);
            memcpy(&a,shadow+vertex*28,4);memcpy(&b,text+vertex*28,4);
            assert(fabsf(a-b-2.f*1.75f/1080.f)<.000001f);
            memcpy(&a,shadow+vertex*28+4,4);memcpy(&b,text+vertex*28+4,4);
            assert(fabsf(a-b+2.f*1.75f/1920.f)<.000001f);
        }
    }
    puts("JNET readability: white foreground, native opaque black shadow offsets, and complete two-pass capacity reservation passed.");
    jnet_lobby_view.count=0;jnet_lobby_view.searching=1;
    number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_lobby_view.searching=0;
    jnet_lobby_view.count=3;
    for(unsigned i=0;i<3;++i)jnet_lobby_view.rows[i]=(struct jnet_lobby_row){10+i,i+1,3,0};
    memset(jnet_menu_password,'a',32);jnet_menu_password[32]=0;
    number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_menu_create=1;jnet_menu_private=1;
    number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_lobby_view.online=1;jnet_lobby_view.total=50;jnet_menu_create=0;
    for(unsigned i=0;i<3;++i){memset(jnet_lobby_view.names[i],'W',32);jnet_lobby_view.names[i][32]=0;}
    memset(jnet_lobby_view.status,'W',79);jnet_lobby_view.status[79]=0;strcpy(jnet_lobby_view.code,"ABCD");
    number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_menu_create=1;number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_menu_page=1;number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_menu_page=2;jnet_lobby_view.room=1;jnet_lobby_view.self=43;jnet_lobby_view.host=42;
    jnet_lobby_view.member_count=3;for(unsigned i=0;i<3;++i)jnet_lobby_view.members[i]=76561198627333172ULL+i;
    number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    jnet_menu_create=0;
    for(unsigned i=0;i<3;++i)strcpy(jnet_lobby_view.member_names[i],"Companion");
    jnet_lobby_view.cmatw=0;number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    unsigned companion_names=number_at(menu+0xb4);
    jnet_lobby_view.cmatw=1;number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    assert(number_at(menu+0xb4)==companion_names);
    jnet_lobby_view.cmatw=0;
    for(unsigned i=0;i<3;++i)strcpy(jnet_lobby_view.member_names[i],"76561198627333172");
    number(menu+0xb4,0);assert(jnet_menu_draw_live((uintptr_t)menu)==0);
    assert(number_at(menu+0xb4)==companion_names);
    memset(&jnet_lobby_view,0,sizeof(jnet_lobby_view));memset(jnet_menu_password,0,sizeof(jnet_menu_password));
    jnet_menu_create=jnet_menu_private=0;
    jnet_menu_page=2;number(menu+0xb4,0);jnet_menu_font.present['P'-32]=0;
    assert(jnet_menu_draw_live((uintptr_t)menu)==3 && number_at(menu+0xb4)==0);
    jnet_original_menu_build=original;
    assert(jnet_menu_build((uintptr_t)menu)==(uintptr_t)menu && JNETMenu[0]==0 && JNETMenu[4]==1);
    {
        unsigned char *rows=menu+0x500,*vector=menu+0x900;
        memset(rows,0,256);memset(menu+0x100,0,128);
        string(menu+0x100,"Options");string(menu+0x140,"QuitToDesktop");
        pointer(rows,menu+0x100);pointer(rows+128,menu+0x140);
        pointer(vector,rows);pointer(vector+8,rows+256);
        float size=.07f;memcpy(menu+0xc48,&size,4);
        const unsigned offsets[]={0x0c,0x14,0x1c,0x7c};
        for(unsigned row=0;row<2;++row)for(unsigned i=0;i<4;++i){
            float value=-.60f-row*.07f;memcpy(rows+128*row+offsets[i],&value,4);
        }
        unsigned char before[256];memcpy(before,rows,256);
        menu[0xda0]=0;jnet_menu_align_title((uintptr_t)menu,(uintptr_t)vector,NULL);
        assert(!memcmp(before,rows,256));
        menu[0xda0]=1;jnet_menu_align_title((uintptr_t)menu,(uintptr_t)vector,NULL);
        for(unsigned row=0;row<2;++row)for(unsigned i=0;i<4;++i){float value;
            memcpy(&value,rows+128*row+offsets[i],4);
            assert(fabsf(value-(-.74f-row*.07f))<.00001f);
        }
        memcpy(before,rows,256);jnet_menu_align_title((uintptr_t)menu,(uintptr_t)vector,NULL);
        assert(!memcmp(before,rows,256));
    }
    assert(!munmap(image,0x4100000));
    puts("JNET live menu adapter: title and pause drawing, submenu/hidden gates, glyph refusal, mapping and capacity checks, and wrong-caller reporting passed.");
}
