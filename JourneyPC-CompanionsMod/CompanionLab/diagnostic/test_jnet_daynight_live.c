#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "jnet_lobby_bridge.h"
typedef int lua_State;
static lua_State lua;
static uint64_t clock_ms=100;
static unsigned char pending;
static int game_mode=1,fail_part;
static unsigned calls,updates;
static unsigned jnet_saved_lighting;
static char command[200];
static int gettop(lua_State *s){(void)s;return 0;}
static void settop(lua_State *s,int n){(void)s;(void)n;}
static const char *tolstring(lua_State *s,int n,void *p){(void)s;(void)n;(void)p;return "fixture failure";}
static int original_loadbuffer(lua_State *s,const char *p,size_t n,const char *name,const char *mode){
    (void)s;(void)n;(void)name;(void)mode;++calls;
    if(!strncmp(p,"__JNETDayNightUpdate(",20)){++updates;snprintf(command,sizeof(command),"%s",p);}
    return fail_part && !strncmp(p,"__JNETDayNightPart(",18);
}
static int pcallfn(lua_State *s,int a,int b,int c,int d,void *p){(void)s;(void)a;(void)b;(void)c;(void)d;(void)p;return 0;}
static void pushclosure(lua_State *s,int (*fn)(lua_State *),int n){(void)s;(void)fn;(void)n;}
static void message(const char *s){(void)s;}
static uint64_t GetTickCount64(void){return clock_ms;}
static int jnet_settings_apply(unsigned group){assert(group==4);jnet_saved_lighting=jnet_daynight_mode>2?0:jnet_daynight_mode;return 1;}
static int own_read(void *ctx,uintptr_t at,void *out,size_t n){
    (void)ctx;
    if(at==0x1c0330 && n==1)memcpy(out,&pending,n);
    else if((at==0x278 || at==0x27c) && n==4)memcpy(out,&game_mode,n);
    else if(at==0x20 && n==8){lua_State *p=&lua;memcpy(out,&p,n);}
    else return 0;
    return 1;
}
#include "jnet_daynight_live.h"
int main(void){
    for(unsigned level=0;level<8;++level){
        assert(jnet_daynight_supported(level));
        assert(!strcmp(jnet_daynight_label(level,0),"Day"));
        assert(!strcmp(jnet_daynight_label(level,1),"Night"));
        assert(!strcmp(jnet_daynight_label(level,2),"Cycle"));
        assert(jnet_daynight_variants(level)==(level==2 || level==3 || level==7?4u:3u));
    }
    assert(!strcmp(jnet_daynight_label(2,3),"Dusk"));assert(!strcmp(jnet_daynight_label(3,3),"Dawn"));
    assert(!strcmp(jnet_daynight_label(7,3),"Cutscene"));
    jnet_daynight_install(&lua);jnet_daynight_loaded(&lua);jnet_current_level=2;
    jnet_daynight_mode=2;jnet_daynight_elapsed=1350000;
    jnet_daynight_game(0);assert(jnet_daynight_part==8 && !updates && jnet_daynight_elapsed==1350000);
    while(jnet_daynight_part<jnet_daynight_data[2].count){clock_ms+=50;jnet_daynight_game(0);}
    assert(updates==1 && strstr(command,"1350.000,2,2"));
    clock_ms+=250;jnet_daynight_game(0);assert(jnet_daynight_elapsed==1350250);
    pending=1;clock_ms+=10;jnet_daynight_game(0);
    clock_ms+=60000;jnet_daynight_loaded(&lua);jnet_current_level=3;pending=0;
    while(jnet_daynight_part<jnet_daynight_data[3].count){clock_ms+=50;jnet_daynight_game(0);}
    assert(jnet_daynight_elapsed==1350250 && strstr(command,"1350.250,3,2"));
    clock_ms+=100;jnet_daynight_game(0);assert(jnet_daynight_elapsed==1350350);
    jnet_travel_busy=1;clock_ms+=100;jnet_daynight_game(0);
    jnet_daynight_sync=1799901;clock_ms+=30000;jnet_daynight_game(0);assert(jnet_daynight_elapsed==1799900);
    jnet_travel_busy=0;jnet_daynight_game(0);clock_ms+=200;jnet_daynight_game(0);assert(jnet_daynight_elapsed==100);
    jnet_daynight_mode=0;clock_ms+=100;jnet_daynight_game(0);assert(!jnet_daynight_elapsed);
    jnet_daynight_loaded(&lua);fail_part=1;unsigned before=updates;
    jnet_daynight_game(0);assert(jnet_daynight_failed && updates==before && !jnet_daynight_part);
    fail_part=0;jnet_daynight_loaded(&lua);assert(!jnet_daynight_failed);
    jnet_current_level=7;jnet_paradise_cutscene=1;before=calls;jnet_daynight_game(0);assert(calls==before);
    puts("Lighting clock: bounded loading, partial-data gate, preserved return phase, loading pause, host phase, wrap, Day reset, failure recovery and Cutscene bypass passed; fixture only.");
}
