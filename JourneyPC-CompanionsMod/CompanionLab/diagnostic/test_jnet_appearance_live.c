#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jnet_menu_model.h"
static unsigned char game[0x1c0400],barn[0x2000],save[0x1c00],files[0x20300];
static uintptr_t fault_image=0x10000000;
static struct jnet_appearance_edit jnet_menu_edit;
static char jnet_appearance_status[80];
static uint64_t jnet_appearance_applied_at;
static uint64_t GetTickCount64(void){return 1000;}
static unsigned jnet_save_requested,jnet_current_level=10,saves;
static int own_read(void *ctx,uintptr_t at,void *out,size_t bytes){
    (void)ctx;
    if(at==fault_image+0x3c9420e && bytes==1){*(unsigned char *)out=1;return 1;}
    memcpy(out,(void *)at,bytes);return 1;
}
static int jc_local_windows_write(uintptr_t at,const void *in,size_t bytes){memcpy((void *)at,in,bytes);return 1;}
static void put(unsigned char *p,unsigned x){memcpy(p,&x,4);}
static unsigned get(unsigned char *p){unsigned x;memcpy(&x,p,4);return x;}
static void ptr(unsigned char *p,uintptr_t x){memcpy(p,&x,8);}
static void mock_save_game(uintptr_t g){assert(g==(uintptr_t)game);++saves;put(files+0x200bc,2);}
#define JC_RVA_SaveGame ((uintptr_t)mock_save_game-fault_image)
#include "jnet_appearance_live.h"
static int accept_character(uintptr_t g,uintptr_t b,unsigned kind){
    assert(g==(uintptr_t)game && b==(uintptr_t)barn && kind==JNET_CHARACTER_LONGTAIL);
    return 1;
}
static int decline_character(uintptr_t g,uintptr_t b,unsigned kind){
    assert(g==(uintptr_t)game && b==(uintptr_t)barn && kind==JNET_CHARACTER_LONGTAIL);
    return 0;
}
int main(void){
    ptr(game+0x1e0,(uintptr_t)barn);ptr(game+0x170,(uintptr_t)save);ptr(game+0x50,(uintptr_t)files);
    put(game+0x278,1);put(game+0x27c,1);ptr(barn,fault_image+0x5da340);ptr(barn+0xb0,fault_image+0x5d0380);
    put(barn+0x118,4);put(barn+0x11c,12);put(barn+0x120,5);put(barn+0x124,19);put(files+0x200bc,1);jnet_appearance_saving=0;
    put(save+0x10,6);put(save+0x14,20);put(save+0x18,8);
    jnet_appearance_loaded((uintptr_t)save);
    assert(jnet_menu_edit.draft.outfit==6 && jnet_menu_edit.draft.scarf_segments==8 && jnet_menu_edit.draft.symbol==20);
    uintptr_t b=0,s=0;
    ptr(save,fault_image+0x5c1ef0);assert(!jnet_appearance_context((uintptr_t)game,&b,&s));
    ptr(save,fault_image+0x5e3178);assert(jnet_appearance_context((uintptr_t)game,&b,&s));
    put(game+0x27c,2);assert(!jnet_appearance_context((uintptr_t)game,&b,&s));put(game+0x27c,1);
    jnet_appearance_game((uintptr_t)game);
    assert(jnet_menu_edit.draft.outfit==5 && jnet_menu_edit.draft.scarf_segments==12 && jnet_menu_edit.draft.symbol==19);
    assert(!jnet_menu_edit.applied.character && !jnet_menu_edit.applied.infinite_scarf && !jnet_menu_edit.applied.locked_scarf);
    jnet_appearance_test_boot=1;jnet_appearance_game((uintptr_t)game);
    assert(jnet_menu_edit.draft.outfit==6 && jnet_menu_edit.draft.scarf_segments==8);
    jnet_menu_edit.draft.scarf_segments=9;jnet_appearance_game((uintptr_t)game);
    assert(jnet_menu_edit.draft.scarf_segments==9);
    jnet_appearance_test_boot=0;
    jnet_menu_edit.draft.symbol=20;jnet_menu_edit.draft.outfit=6;jnet_menu_edit.draft.scarf_segments=20;
    assert(!jnet_appearance_apply(&jnet_menu_edit,29,0));game[0x1c0330]='x';
    jnet_appearance_game((uintptr_t)game);assert(jnet_menu_edit.awaiting && !saves);
    game[0x1c0330]=0;jnet_appearance_game((uintptr_t)game);
    assert(!jnet_menu_edit.awaiting && saves==1 && get(barn+0x120)==6 && get(barn+0x11c)==20 && get(barn+0x124)==20);
    assert(get(save+0x20)==10 && get(save+0x19a8)==20);
    put(barn+0x11c,21);jnet_appearance_game((uintptr_t)game);assert(jnet_menu_edit.draft.scarf_segments==21);
    for(unsigned symbol=0;symbol<=20;++symbol){
        jnet_menu_edit.draft.symbol=symbol;put(files+0x200bc,1);jnet_appearance_saving=0;
        assert(!jnet_appearance_apply(&jnet_menu_edit,29,0));jnet_appearance_game((uintptr_t)game);
        assert(get(barn+0x124)==symbol && jnet_menu_edit.applied.symbol==symbol);
    }
    unsigned before[4];memcpy(before,barn+0x118,sizeof(before));unsigned saves_before=saves;
    jnet_character_apply_native=decline_character;
    jnet_menu_edit.draft.character=JNET_CHARACTER_LONGTAIL;
    jnet_menu_edit.draft.outfit=1;jnet_menu_edit.draft.scarf_segments=2;
    assert(!jnet_appearance_apply(&jnet_menu_edit,29,1u<<JNET_CHARACTER_LONGTAIL));
    jnet_appearance_game((uintptr_t)game);
    assert(!jnet_menu_edit.awaiting && !jnet_menu_edit.applied.character);
    assert(!memcmp(before,barn+0x118,sizeof(before)) && saves==saves_before && !jnet_save_requested);
    jnet_character_apply_native=accept_character;
    jnet_menu_edit.draft=jnet_menu_edit.applied;
    jnet_menu_edit.draft.character=JNET_CHARACTER_LONGTAIL;
    jnet_menu_edit.draft.infinite_scarf=1;jnet_menu_edit.draft.locked_scarf=1;
    put(files+0x200bc,1);jnet_appearance_saving=0;
    assert(!jnet_appearance_apply(&jnet_menu_edit,29,1u<<JNET_CHARACTER_LONGTAIL));
    jnet_appearance_game((uintptr_t)game);
    assert(jnet_menu_edit.applied.character==JNET_CHARACTER_LONGTAIL && saves==saves_before && !jnet_save_requested);
    put(barn+0x118,0);
    assert(!jnet_appearance_apply(&jnet_menu_edit,29,1u<<JNET_CHARACTER_LONGTAIL));
    jnet_appearance_game((uintptr_t)game);
    assert(saves==saves_before && !jnet_save_requested);
    for(unsigned field=0;field<3;++field){
        put(files+0x200bc,1);jnet_appearance_saving=0;
        if(field==0)jnet_menu_edit.draft.outfit=1;
        if(field==1)jnet_menu_edit.draft.scarf_segments=2;
        if(field==2)jnet_menu_edit.draft.symbol=1;
        assert(!jnet_appearance_apply(&jnet_menu_edit,29,1u<<JNET_CHARACTER_LONGTAIL));
        jnet_appearance_game((uintptr_t)game);
        assert(saves==saves_before+field+1 && !jnet_save_requested);
    }
    for(unsigned level=0;level<12;++level)for(unsigned infinite=0;infinite<2;++infinite)for(unsigned locked=0;locked<2;++locked){
        jnet_current_level=level;
        jnet_menu_edit.applied=(struct jnet_appearance_choice){.outfit=1,.symbol=1,.scarf_segments=29,.infinite_scarf=infinite,.locked_scarf=locked};
        jnet_menu_edit.draft=jnet_menu_edit.applied;
        put(barn+0x118,30);put(barn+0x11c,30);put(barn+0x120,1);put(barn+0x124,1);
        jnet_appearance_game((uintptr_t)game);
        assert(get(barn+0x11c)==(locked?29u:30u));
        assert(get(barn+0x118)==(locked?29u:30u));
        assert(jnet_menu_edit.applied.infinite_scarf==infinite && jnet_menu_edit.applied.locked_scarf==locked);
        assert(jnet_menu_edit.draft.infinite_scarf==infinite && jnet_menu_edit.draft.locked_scarf==locked);
        assert(jnet_menu_edit.applied.scarf_segments==(locked?29u:30u));
        put(barn+0x118,0);jnet_appearance_game((uintptr_t)game);
        assert(get(barn+0x118)==(infinite?(locked?29u:30u):0));
        put(save+0x10,1);put(save+0x14,1);put(save+0x18,30);
        jnet_appearance_loaded((uintptr_t)save);
        assert(jnet_menu_edit.applied.scarf_segments==(locked?29u:30u));
        assert(jnet_menu_edit.applied.infinite_scarf==infinite && jnet_menu_edit.applied.locked_scarf==locked);
    }
    unsigned char newer[0x1ba8]={0},older[0x1ba8]={0};uint64_t stamp=100;
    put(newer,22);put(newer+8,6);put(newer+12,20);put(newer+16,8);put(newer+20,8);put(newer+24,6);
    memcpy(newer+0x40,&stamp,8);assert(jnet_save_newer(newer,older));
    memcpy(older,newer,sizeof(older));assert(!jnet_save_newer(newer,older));
    stamp=99;memcpy(newer+0x40,&stamp,8);assert(!jnet_save_newer(newer,older));
    stamp=101;memcpy(newer+0x40,&stamp,8);put(newer+12,21);assert(!jnet_save_newer(newer,older));
    puts("Appearance adapter: live save type, boot values, deferred Apply, native save submission, gameplay refresh and 48 level/scarf-mode combinations including pickup to 30, draining and save reload passed with mocked game memory.");
}
