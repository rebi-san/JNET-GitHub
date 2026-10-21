#ifndef JNET_FREEPLAY_LIVE_H
#define JNET_FREEPLAY_LIVE_H
static int jnet_freeplay_prepare(lua_State *state) {
    static const char *const chunks[]={
        "function __JNETApplySpin() "
            "if type(LoadDudeActionAnimations)=='function' and LoadDudeActionAnimations~=__JNETSpinLoader then "
            "local load=LoadDudeActionAnimations local list=GetDudeActionAnims local queue=QueueAnimationSequence "
            "local loadseq=LoadDudeAnimSequences local seqlist=GetDudeAnimSequences "
            "local extra={'JNETSpin3','JNETFlipL','JNETFlipR'} "
            "__JNETSpinLoader=function(barn) load(barn) for _,name in ipairs(extra) do local a=barn:AddActionAnimation() "
            "a:SetName(name) a:isAction(true) end end LoadDudeActionAnimations=__JNETSpinLoader "
            "GetDudeActionAnims=function() local names=list() for _,name in ipairs(extra) do names[#names+1]=name end return names end "
            "LoadDudeAnimSequences=function(barn) loadseq(barn) for _,name in ipairs(extra) do barn:AddAnimSequence(name) end end "
            "GetDudeAnimSequences=function() local names=seqlist() for _,name in ipairs(extra) do names[#names+1]=name end return names end "
            "QueueAnimationSequence=function(barn,name) if name=='JNETSpin3' then "
            "barn:QueueAnimation('JNETSpin3',0.2,0.8,0.9,1,4) elseif name=='JNETFlipL' or name=='JNETFlipR' then "
            "barn:QueueAnimation(name,0.2,0.3,2,1,0) else return queue(barn,name) end end end "
        " end ",
        "function __JNETApplySounds() "
            "if type(soundEffectData)=='table' and soundEffectData.PlayerCoo and not soundEffectData.JNTFishShort then "
            "local rows={{'JNTFishShort','JNTBio','fish-idle-move-short'},{'JNTFishMedium','JNTBio','fish-idle-move-medium'},"
            "{'JNTFishLong','JNTBio','fish-idle-move-long'},{'JNTFishCall','JNTBio','fish-come-here'},"
            "{'JNTWarVoice','JNTMech','GuardianCameoBy'},{'JNTWarLock','JNTMech','GuardianFinalStretchHungry1'},"
            "{'JNTWarHungry','JNTMech','GuardianFinalStretchHungry2'},{'JNTWarAway','JNTMech','GuardianFlyAway'},"
            "{'JNTWhale','JNTBio','HappyWhale'},{'JNTFlowSnake','JNTFlow','FlowSnakeCall'},{'JNTFlowHunter','JNTFlow','FlowHunterCall'}} "
            "local n=0 for _ in pairs(soundEffectData) do n=n+1 end if n+#rows<=160 then "
            "for _,r in ipairs(rows) do soundEffectData[r[1]]={bankName=r[2],soundName=r[3],minDist=60,maxDist=200,autoPitchBend=false} end end end "
        " end ",
        "function __JNETApplyScenery() "
            "if type(InitializeSignBarn)=='function' and not __JNETSignWrapped then "
            "local original=InitializeSignBarn InitializeSignBarn=function(...) "
            "if not __JNETCreditsEnabled and type(Credits)=='table' then "
            "local iscredits=false for _,s in ipairs(Credits) do for _,e in ipairs(s.Entries or {}) do "
            "if e.Text=='Thank you for playing' then iscredits=true end end end "
            "if iscredits then for _,s in ipairs(Credits) do for _,e in ipairs(s.Entries or {}) do "
            "e.Text=e.Text=='Thank you for playing' and 'Thank you for playing' or '' end end end end "
            "return original(...) end __JNETSignWrapped=true end "
            "if type(PutHullsInNamesTable)=='function' and PutHullsInNamesTable~=__JNETParadiseHullWrapper then "
            "local original=PutHullsInNamesTable __JNETParadiseHullWrapper=function(barn) original(barn) "
            "if not __JNETParadiseCutscene then return end "
            "local hull=Names['edd23401f7232e3829cfd2c5695b5967'] if not hull then return end "
            "local function resize(h) "
            "if h:GetTriCount()~=12 or h:GetPlaneCount()~=6 or h:GetEdgeCount()~=12 or h:GetVertCount()~=8 then "
            "error('Unexpected Paradise revival hull topology') end "
            "local v={{0,-2.00266,-415.817},{0,2000,1024},{0,-2.00266,1024},{0,2000,-415.817},"
            "{512,-2.00266,1024},{512,2000,-415.817},{512,-2.00266,-415.817},{512,2000,1024}} "
            "h:ResetAabb() for i=1,8 do h:SetVert(i-1,v[i]) end end "
            "local ok,err=pcall(function() if type(hull)=='table' then for _,h in ipairs(hull) do resize(h) end "
            "else resize(hull) end end) "
            "if not ok then print('[JNET] Paradise hull resize failed: '..tostring(err)) end "
            "end PutHullsInNamesTable=__JNETParadiseHullWrapper end"
        " end ",
    };
    int top=gettop(state);
    for(unsigned i=0;i<sizeof(chunks)/sizeof(chunks[0]);++i){
        int ok=original_loadbuffer(state,chunks[i],strlen(chunks[i]),"JNET-setup-cache","t")==0;
        if(ok)ok=pcallfn(state,0,0,0,0,NULL)==0;
        settop(state,top);
        if(!ok){message("[JNET] setup cache preparation failed");return 0;}
    }
    message("[JNET] setup scripts cached for Lua state");
    return 0;
}
static void (*jnet_script_execute_original)(uintptr_t,lua_State *);
static void jnet_script_execute_at(uintptr_t resource,lua_State *state,uintptr_t caller) {
    if(jnet_mepal_play && caller==fault_image+0x14dd12){
        const char empty[]="TimelineInstances={ObjectType='TimelineInstances'}";
        int top=gettop(state);
        int ok=original_loadbuffer(state,empty,sizeof(empty)-1,"JNET-freeplay","t")==0;
        if(ok)ok=pcallfn(state,0,0,0,0,NULL)==0;
        settop(state,top);
        if(ok){message("[JNET] free-play native timeline load replaced");return;}
        message("[JNET] free-play timeline replacement failed; original forwarded");
    }
    jnet_script_execute_original(resource,state);
    jnet_daynight_install(state);
    {
        char code[512];
        snprintf(code,sizeof(code),
            "local prepare=... if not __JNETApplyScenery then prepare() end "
            "__JNETCreditsEnabled=%s __JNETParadiseCutscene=%s "
            "if __JNETApplyScenery then __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery() end",
            jnet_credits?"true":"false",jnet_paradise_cutscene?"true":"false");
        int top=gettop(state),ok=original_loadbuffer(state,code,strlen(code),"JNET-credits-signs","t")==0;
        if(ok){pushclosure(state,jnet_freeplay_prepare,0);ok=pcallfn(state,1,0,0,0,NULL)==0;}
        settop(state,top);
        if(!ok)message("[JNET] setup settings could not be applied");
    }
    if(!jnet_credits && caller==fault_image+0xd95a8){
        const char code[]=
            "local t=TriggerInstances if type(t)~='table' then return end "
            "local credits=false for _,v in ipairs(t) do local a=v.Vars "
            "if a and a.soundName=='M_9mC1' then credits=true break end end "
            "if not credits then return end "
            "for _,v in ipairs(t) do local a=v.Vars if a then "
            "if v.Type=='DisplayText' then a.text=a.text=='Thank you for playing' and 'Thank you for playing' or '' "
            "elseif v.Type=='ScrollTextTrigger' then a.alpha=0 a.lifetime=0 "
            "elseif a.soundName=='M_9mC1' then a.vol=0 end end end";
        int top=gettop(state);
        int ok=original_loadbuffer(state,code,sizeof(code)-1,"JNET-credits","t")==0;
        if(ok)ok=pcallfn(state,0,0,0,0,NULL)==0;
        settop(state,top);
        if(!ok)message("[JNET] Credits settings could not be applied to loaded triggers");
    }
}
static void jnet_script_execute(uintptr_t resource,lua_State *state) {
    jnet_script_execute_at(resource,state,(uintptr_t)__builtin_return_address(0));
}
#endif
