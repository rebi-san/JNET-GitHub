#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {int top;} lua_State;
static uintptr_t fault_image=0x10000000;
static unsigned jnet_credits=1,jnet_paradise_cutscene,jnet_mepal_play,calls,loads,sign_loads,fail;
static int gettop(lua_State *s){return s->top;}
static void settop(lua_State *s,int top){s->top=top;}
static int original_loadbuffer(lua_State *s,const char *code,size_t bytes,const char *name,const char *mode){
    (void)name;(void)mode;assert(bytes==strlen(code));assert(strstr(code,"TimelineInstances=") || strstr(code,"local t=TriggerInstances") || strstr(code,"__JNETCreditsEnabled"));if(strstr(code,"__JNETCreditsEnabled"))++sign_loads;else ++loads;++s->top;return fail;
}
static int pcallfn(lua_State *s,int a,int b,int c,int d,void *e){(void)b;(void)c;(void)d;(void)e;s->top-=1+a;return 0;}
static void pushclosure(lua_State *s,int (*callback)(lua_State *),int n){(void)callback;(void)n;++s->top;}
static void message(const char *s){(void)s;}
static void original(uintptr_t resource,lua_State *s){assert(resource==42 && s->top==7);++calls;}
static void jnet_daynight_install(lua_State *s){(void)s;}
#include "jnet_freeplay_live.h"
int main(void){
    lua_State s={7};jnet_script_execute_original=original;
    jnet_script_execute_at(42,&s,fault_image+0x14dd12);assert(calls==1 && !loads);
    jnet_mepal_play=1;jnet_script_execute_at(42,&s,fault_image+0x14dd12);assert(calls==1 && loads==1 && s.top==7);
    jnet_script_execute_at(42,&s,fault_image+0xd8c8a);assert(calls==2 && loads==1);
    fail=1;jnet_script_execute_at(42,&s,fault_image+0x14dd12);assert(calls==3 && s.top==7);
    jnet_script_execute(42,&s);assert(calls==4);
    fail=0;jnet_mepal_play=0;jnet_credits=0;
    unsigned before=loads;jnet_script_execute_at(42,&s,fault_image+0xd95a8);assert(loads==before+1 && s.top==7);
    before=loads;jnet_credits=1;jnet_script_execute_at(42,&s,fault_image+0xd95a8);assert(loads==before && s.top==7);
    puts("Free-play adapter: native timeline caller interception, normal/unrelated passthrough, stack restoration and failure fallback passed with mocked Lua.");
}
