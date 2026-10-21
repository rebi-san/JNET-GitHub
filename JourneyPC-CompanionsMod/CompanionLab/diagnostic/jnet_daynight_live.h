#ifndef JNET_DAYNIGHT_LIVE_H
#define JNET_DAYNIGHT_LIVE_H
#include "jnet_daynight_payload.h"
static lua_State *jnet_daynight_state;
static unsigned jnet_daynight_visit,jnet_daynight_observed,jnet_daynight_failed;
static uint64_t jnet_daynight_last;
static unsigned jnet_daynight_part;
static int jnet_daynight_checkpoint(lua_State *state) {
    const char *stage=tolstring(state,1,NULL);char line[160];
    snprintf(line,sizeof(line),"[JNET] lighting update stage: %.100s",stage?stage:"unknown");message(line);
    return 0;
}
static int jnet_daynight_loaded(lua_State *state) {
    (void)state;++jnet_daynight_visit;jnet_daynight_failed=0;jnet_daynight_last=0;jnet_daynight_part=0;return 0;
}
static int jnet_daynight_call(lua_State *state,const char *source) {
    int top=gettop(state);
    int ok=original_loadbuffer(state,source,strlen(source),"JNET-daynight","t")==0 && pcallfn(state,0,0,0,0,NULL)==0;
    if(!ok){
        const char *error=tolstring(state,-1,NULL);char line[320];
        snprintf(line,sizeof(line),"[JNET] day/night unavailable: %.250s",error?error:"Lua error");message(line);
    }
    settop(state,top);return ok;
}
static void jnet_daynight_install(lua_State *state) {
    if(jnet_daynight_failed)return;
    if(jnet_daynight_state!=state){
        int top=gettop(state);
        int ok=original_loadbuffer(state,jnet_daynight_source,sizeof(jnet_daynight_source)-1,"JNET-daynight-install","t")==0;
        if(ok){pushclosure(state,jnet_daynight_loaded,0);pushclosure(state,jnet_daynight_checkpoint,0);ok=pcallfn(state,2,0,0,0,NULL)==0;}
        settop(state,top);
        if(!ok){message("[JNET] day/night script installation failed");jnet_daynight_failed=1;return;}
        jnet_daynight_state=state;
    } else if(!jnet_daynight_call(state,"__JNETDayNightInstall()"))jnet_daynight_failed=1;
}
static void jnet_daynight_game(uintptr_t game) {
    static uint64_t next;
    unsigned char pending=1;int mode=0,resume=0;lua_State *state=NULL;
    uint64_t now=GetTickCount64();
    uint64_t sync=atomic_exchange(&jnet_daynight_sync,0);
    if(sync){jnet_daynight_elapsed=(unsigned)(sync-1);jnet_daynight_last=0;next=0;}
    if(jnet_daynight_failed || !jnet_daynight_visit || jnet_travel_busy ||
       !own_read(NULL,game+0x1c0330,&pending,1) || pending ||
       !own_read(NULL,game+0x278,&mode,4) || mode<=0 ||
       !own_read(NULL,game+0x27c,&resume,4) || mode!=resume ||
       !own_read(NULL,game+0x20,&state,8) || !state || state!=jnet_daynight_state){jnet_daynight_last=0;return;}
    unsigned level=jnet_current_level;
    if(jnet_daynight_observed!=jnet_daynight_visit){
        jnet_daynight_observed=jnet_daynight_visit;next=0;
        jnet_daynight_enter(level);
        char line[128];snprintf(line,sizeof(line),"[JNET] day/night visit=%u level=%u mode=%u phase_ms=%u",
            jnet_daynight_visit,level,(unsigned)jnet_daynight_mode,(unsigned)jnet_daynight_elapsed);message(line);
    }
    if(!jnet_daynight_supported(level) || (level==7 && jnet_paradise_cutscene)){jnet_daynight_last=0;return;}
    jnet_daynight_enter(level);
    unsigned lighting=jnet_daynight_mode;
    unsigned count=jnet_daynight_data[level].count;
    if(jnet_daynight_part<count){
        unsigned budget=8;
        while(budget-- && jnet_daynight_part<count){
            if(!jnet_daynight_call(state,jnet_daynight_data[level].parts[jnet_daynight_part])){jnet_daynight_failed=1;return;}
            ++jnet_daynight_part;
        }
        jnet_daynight_last=0;
        if(jnet_daynight_part<count)return;
        if(!jnet_daynight_call(state,"__JNETDayNight.ready=true")){jnet_daynight_failed=1;return;}
        char line[96];snprintf(line,sizeof(line),"[JNET] day/night level %u data loaded in %u parts",level,count);message(line);
    }
    if(lighting!=2)jnet_daynight_elapsed=0;
    else if(jnet_daynight_last)jnet_daynight_elapsed=(unsigned)((jnet_daynight_elapsed+now-jnet_daynight_last)%1800000);
    jnet_daynight_last=now;
    if(now<next)return;next=now+50;
    unsigned saved=lighting>2?0:lighting;
    if(jnet_saved_lighting!=saved && !jnet_settings_apply(4))message("[JNET] lighting preference could not be saved; will retry");
    char command[160];snprintf(command,sizeof(command),"__JNETDayNightUpdate(%.3f,%u,%u)",(double)jnet_daynight_elapsed/1000.,level,lighting);
    if(!jnet_daynight_call(state,command))jnet_daynight_failed=1;
}
#endif
