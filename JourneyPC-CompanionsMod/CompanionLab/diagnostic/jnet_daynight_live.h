#ifndef JNET_DAYNIGHT_LIVE_H
#define JNET_DAYNIGHT_LIVE_H
#include "jnet_daynight_payload.h"
static lua_State *jnet_daynight_state;
static unsigned jnet_daynight_visit,jnet_daynight_observed,jnet_daynight_failed;
static int jnet_daynight_checkpoint(lua_State *state) {
    const char *stage=tolstring(state,1,NULL);char line[160];
    snprintf(line,sizeof(line),"[JNET] lighting update stage: %.100s",stage?stage:"unknown");message(line);
    return 0;
}
static int jnet_daynight_loaded(lua_State *state) {
    (void)state;++jnet_daynight_visit;jnet_daynight_failed=0;return 0;
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
/* Journey's Lua allocator serves every request from fixed pools (table at RVA 0x5C38F0). Their descriptors are 0x40 bytes in a
   vector at RVA 0x3CE26F0: used at +0, block size at +8, capacity at +0xC, peak at +0x3C. A full pool is fatal (int3 at RVA 0xF0967),
   so the headroom of the large classes is logged for each level. Reads are bounds-checked and a failure only skips the line. */
static void jnet_pool_report(const char *when) {
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL),first=0,last=0;
    if(!own_read(NULL,image+0x3ce26f0,&first,8) || !own_read(NULL,image+0x3ce26f8,&last,8) ||
       !first || last<=first || (last-first)%0x40 || (last-first)/0x40>32)return;
    char line[400];int length=snprintf(line,sizeof(line),"[JNET] game allocator pools at %.24s (used/peak/capacity):",when);
    for(uintptr_t at=first;at<last && length<(int)sizeof(line)-32;at+=0x40){
        unsigned used=0,size=0,capacity=0,peak=0;
        if(!own_read(NULL,at,&used,4) || !own_read(NULL,at+8,&size,4) || !own_read(NULL,at+0xc,&capacity,4) || !own_read(NULL,at+0x3c,&peak,4))return;
        if(size>=4096)length+=snprintf(line+length,sizeof(line)-(size_t)length," %u:%u/%u/%u",size,used,peak,capacity);
    }
    message(line);
}
/* The profile index installed with the script is small; a level's lighting data is compiled afterwards as many small parts
   (see daynight_resources.py) so that no single Lua allocation needs one of the game's few large pool blocks. */
static unsigned jnet_daynight_loaded_visit;
static int jnet_daynight_load_level(lua_State *state,unsigned level) {
    unsigned count=0;
    if(level>=JNET_DAYNIGHT_LEVELS)return 1;
    for(const char *const *part=jnet_daynight_parts[level];*part;++part,++count)
        if(!jnet_daynight_call(state,*part))return 0;
    char line[96];snprintf(line,sizeof(line),"[JNET] day/night level %u data loaded in %u parts",level,count);message(line);
    jnet_pool_report("level data loaded");
    return 1;
}
static void jnet_daynight_game(uintptr_t game) {
    static uint64_t next;
    unsigned char pending=1;int mode=0,resume=0;lua_State *state=NULL;
    if(jnet_daynight_failed || !jnet_daynight_visit ||
       !own_read(NULL,game+0x1c0330,&pending,1) || pending ||
       !own_read(NULL,game+0x278,&mode,4) || mode<=0 ||
       !own_read(NULL,game+0x27c,&resume,4) || mode!=resume ||
       !own_read(NULL,game+0x20,&state,8) || !state || state!=jnet_daynight_state)return;
    unsigned level=jnet_current_level;
    if(jnet_daynight_observed!=jnet_daynight_visit){
        jnet_daynight_observed=jnet_daynight_visit;next=0;
        jnet_daynight_enter(level);
        char line[128];snprintf(line,sizeof(line),"[JNET] day/night visit=%u level=%u mode=%u (0=day, 1=night/dusk/dawn, 2=30 minute cycle, 3=night on Pink Desert/Sunken City)",
            jnet_daynight_visit,level,(unsigned)(jnet_daynight_supported(level)?jnet_daynight_normalize(level,jnet_daynight_mode):0));message(line);
        jnet_pool_report("level visit");
    }
    uint64_t now=GetTickCount64();if(now<next)return;next=now+1000;
    if(!jnet_daynight_supported(level))return;
    jnet_daynight_enter(level);
    if(jnet_saved_lighting!=jnet_daynight_mode && !jnet_settings_apply(4))
        message("[JNET] lighting preference could not be saved; will retry");
    if(jnet_daynight_normalize(level,jnet_daynight_mode) && jnet_daynight_loaded_visit!=jnet_daynight_visit){
        if(!jnet_daynight_load_level(state,level)){message("[JNET] day/night level data failed to load");jnet_daynight_failed=1;return;}
        jnet_daynight_loaded_visit=jnet_daynight_visit;
    }
    char command[160];snprintf(command,sizeof(command),"__JNETDayNightUpdate(%.3f,%u,%u)",(double)now/1000.,level,(unsigned)jnet_daynight_normalize(level,jnet_daynight_mode));
    if(!jnet_daynight_call(state,command))jnet_daynight_failed=1;
}
#endif
