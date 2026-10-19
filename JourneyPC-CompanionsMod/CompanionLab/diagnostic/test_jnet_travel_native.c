#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include "menu_code.h"
static void pointer(unsigned char *p,void *value){memcpy(p,&value,sizeof(value));}
typedef unsigned long DWORD;
#define PAGE_EXECUTE_READWRITE 1
static uintptr_t fault_image;
static unsigned jnet_mepal_play;
static int own_read(void *ctx,uintptr_t at,void *out,size_t bytes){(void)ctx;memcpy(out,(void *)at,bytes);return 1;}
static int VirtualProtect(void *p,size_t n,DWORD mode,DWORD *old){(void)p;(void)n;(void)mode;*old=1;return 1;}
static void *GetCurrentProcess(void){return NULL;}
static int FlushInstructionCache(void *p,void *at,size_t n){(void)p;(void)at;(void)n;return 1;}
static void message(const char *s){(void)s;}
#include "mepal_code.h"
static unsigned jnet_windwalls=1,jnet_sliding=1,jnet_freezing=1,jnet_options_failed;
#include "jnet_options_live.h"
static int jc_local_windows_write(uintptr_t at,const void *data,size_t bytes){memcpy((void *)at,data,bytes);return 1;}
#include "chapter_code.h"
static void menu_frame_fixture(uintptr_t menu,uintptr_t game){(void)menu;(void)game;}
int main(void) {
    unsigned char *image=mmap(NULL,0x4040000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_menu_code(image);
    assert(!mprotect(image,0x110000,PROT_READ|PROT_EXEC));
    unsigned char *game=calloc(1,0x1c1000),*sound=calloc(1,0x7c800),*match=calloc(1,0x12000);
    assert(game && sound && match);pointer(game+0x150,sound);pointer(game+0x180,match);
    void (__attribute__((ms_abi)) *queue)(void *,const char *,unsigned char,unsigned char)=(void *)(image+0xda6c0);
    const char *names[]={"Level_Graveyard","Level_Barrens","Level_Desert","Level_Canyon","Level_Cave","Level_Ruins",
        "Level_Mountain","Level_Summit","Level_Credits","Level_Bryan","Level_Matt","Level_Chris"};
    for(unsigned i=0;i<12;++i){
        strcpy((char *)game+0x1c02f0,"Level_Desert");game[0x1c0374]=0x5a;
        queue(game,names[i],1,1);float delay;memcpy(&delay,game+0x1c0370,4);
        assert(!strcmp((char *)game+0x1c0330,names[i]) && delay==-1.f && game[0x2a6]==1);
        assert(!strcmp((char *)game+0x1c02f0,"Level_Desert") && game[0x1c0374]==0x5a);
    }
    fault_image=(uintptr_t)image;
    assert(!mprotect(image,0x110000,PROT_READ|PROT_WRITE));
    const unsigned char slide[]={0x0f,0xb6,0x82,0,1,0,0},cold[]={0xf3,0x0f,0x11,0x43,0x14,0xf3,0x0f,0x11,0x4b,0x18};
    image[0x15b240]=0x40;image[0x15b241]=0x55;
    memcpy(image+0xaa239,slide,7);memcpy(image+0x184920,cold,10);
    jnet_windwalls=0;jnet_sliding=0;jnet_freezing=0;jnet_options_apply();assert(!jnet_options_failed);
    assert(image[0x15b240]==0xc3 && image[0x15b241]==0x90);
    assert(image[0xaa239]==0xb8 && image[0xaa239+1]==0);
    for(unsigned i=5;i<7;++i)assert(image[0xaa239+i]==0x90);
    for(unsigned i=0;i<10;++i)assert(image[0x184920+i]==0x90);
    jnet_windwalls=1;jnet_sliding=1;jnet_freezing=1;jnet_options_apply();assert(image[0x15b240]==0x40 && image[0x15b241]==0x55 && !memcmp(image+0xaa239,slide,7) && !memcmp(image+0x184920,cold,10));
    for(unsigned repeat=0;repeat<3;++repeat){
        jnet_sliding=0;jnet_options_apply();assert(!jnet_options_failed && image[0xaa239]==0xb8 && image[0xaa23a]==0);
        jnet_sliding=1;jnet_options_apply();assert(!jnet_options_failed && !memcmp(image+0xaa239,slide,7));
    }
    image[0x184920]=0xcc;jnet_sliding=0;jnet_options_apply();assert(jnet_options_failed && jnet_sliding==1 && !memcmp(image+0xaa239,slide,7));
    memcpy(image+0x184920,cold,10);assert(!mprotect(image,0x110000,PROT_READ|PROT_EXEC));
    puts("Windwalls, Sliding and Freezing: pinned boundary routine and CT byte sites, independent off/on restoration and unknown-code refusal passed with mocked protection APIs.");
    unsigned char *save=malloc(0x1bc0),menu[0xf0];assert(save);
    memset(save,0xa5,0x1bc0);memset(menu,0xa5,sizeof(menu));
    pointer(save,image+0x5e3178);pointer(menu,image+0x679c18);pointer(game+0x170,save);
    jnet_menu_frame_original=menu_frame_fixture;
    jnet_menu_frame((uintptr_t)menu,(uintptr_t)game);
    assert(!jnet_travel_playable_menu());
    menu[0x20]=0;menu[0x21]=0;assert(!jnet_travel_playable_menu());
    menu[0x20]=1;menu[0x21]=1;assert(!jnet_travel_playable_menu());
    menu[0x20]=0;assert(jnet_travel_playable_menu());
    pointer(menu,image+0x679c19);assert(!jnet_travel_playable_menu());
    pointer(menu,image+0x679c18);
    for(unsigned level=0;level<12;++level) {
        assert(jnet_travel_entry_chapter(level,0,0,0)==(level==0));
        assert(!jnet_travel_entry_chapter(level,0,0,1));
        assert(!jnet_travel_entry_chapter(level,1,0,0));
    }
    assert(jnet_travel_entry_chapter(0,1,1,0));
    assert(!jnet_travel_menu_prepare((uintptr_t)game+1,1));
    pointer(save,image+0x5c1ef0);assert(!jnet_travel_menu_prepare((uintptr_t)game,1));
    pointer(save,image+0x5e3178);
    for(unsigned repeat=0;repeat<2;++repeat){
        assert(jnet_travel_menu_prepare((uintptr_t)game,1));
        strcpy((char *)game+0x1c02f0,"Level_Graveyard");
        queue(game,names[0],0,0);
        assert(!strcmp((char *)game+0x1c0330,"Level_Graveyard") && !game[0x2a6]);
        assert(save[0x50]==1 && !menu[0x20] && menu[0x21]==1);
        uint64_t value=0;memcpy(&value,menu+0xe8,4);assert(!value && menu[0xec]==0xa5);
        for(unsigned i=0;i<13;++i){memcpy(&value,save+0x60+i*0x158,8);assert(!value);}
        assert(save[0x10]==0xa5 && save[0x18]==0xa5 && save[0x20]==0xa5 && save[0x54]==0xa5);
        assert(!game[0x1c0374] && !game[0x1c037f]);
    }
    memset(save+8,0xa5,0x1ba8);
    for(unsigned level=1;level<12;++level){
        menu[0x20]=1;menu[0x21]=0;
        assert(jnet_travel_menu_prepare((uintptr_t)game,0));
        assert(!menu[0x20] && menu[0x21]==1);
        for(unsigned i=8;i<0x1bb0;++i)assert(save[i]==0xa5);
    }
    free(save);
    puts("Chapter Select: guarded native entry state, playable entry event instead of boot event, gameplay menu flags, preserved outfit/scarf/progression, repeated Graveyard queue verified in isolation.");
    const size_t sites[]={0x187f67,0x170e22,0x170e9c,0x17dab7},sizes[]={6,6,4,11};
    const unsigned char off[4][16]={{0x66,0xc7,0x41,8,1,1},{0x66,0xc7,0x42,9,0,1},{0xc6,0x43,0x0a,1},{0x0f,0x92,0xc0,0x88,0x43,9,0x48,0x8b,0x5c,0x24,0x30}};
    const unsigned char on[4][16]={{0x66,0xc7,0x41,8,0,0},{0x66,0xc7,0x42,9,0,0},{0xc6,0x43,0x0a,0},{0x31,0xc0,0x88,0x43,9,0x90,0x48,0x8b,0x5c,0x24,0x30}};
    for(unsigned i=0;i<4;++i)memcpy(image+sites[i],off[i],sizes[i]);
    for(unsigned mode=0;mode<4;++mode){
        jnet_mepal_play=mode&2;
        for(unsigned level=0;level<12;++level){
            assert(jnet_mepal_prepare(level));
            for(unsigned i=0;i<4;++i)assert(!memcmp(image+sites[i],(i<3?1:jnet_mepal_play)?on[i]:off[i],sizes[i]));
        }
    }
    image[sites[3]]=0xcc;jnet_mepal_play=0;
    assert(!jnet_mepal_prepare(2));assert(!memcmp(image+sites[0],on[0],6));
    memcpy(image+sites[3],on[3],11);assert(jnet_mepal_prepare(2));
    for(unsigned i=0;i<4;++i)assert(!memcmp(image+sites[i],i<3?on[i]:off[i],sizes[i]));
    image[sites[2]]=0xcc;
    assert(!jnet_mepal_prepare(0));
    memcpy(image+sites[2],off[2],sizes[2]);
    assert(jnet_mepal_prepare(0) && !memcmp(image+sites[2],on[2],sizes[2]));
    puts("MEPAL patches: always-on encounters and independent freeplay, all destinations, exact restoration and unknown-code rejection passed with mocked protection APIs.");
    free(game);free(sound);free(match);assert(!munmap(image,0x4040000));
    puts("Pinned native QueueLevel: Windows ABI, all 12 destination names, immediate/reset arguments, bounded pending name and deferred loading verified in isolation. Journey loading and MEPAL gameplay not exercised.");
}
