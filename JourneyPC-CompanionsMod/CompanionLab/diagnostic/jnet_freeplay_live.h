#ifndef JNET_FREEPLAY_LIVE_H
#define JNET_FREEPLAY_LIVE_H
#include "jnet_hull_payload.h"
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
            "if type(soundEffectData)=='table' then for _,name in ipairs({'Guardian_VisorDown','Guardian_VisorUp','Guardian_LockOn','Guardian_LeapUp','Guardian_DiveBomb','Guardian_TearPlayerCape','GuardianSearchlight'}) do "
            "local row=soundEffectData[name] if row then row.bankName='JNTMech' end end end "
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
            "local mountainCameraEvents={['a77d34545bc9b6fc95e857f67fa0067a']=true,"
            "['4f6615f5a3f9e5695f3a7fe7789a2fc8']=true,['d1a0e0b6c348ba723d4a355ddd335ee1']=true,"
            "['c37c3c3fccbc5d5275b657290f4318e2']=true,['0a72efb72de0bf4b2796c1873eadaae0']=true,"
            "['b877b24f969fd3a7dc14e5fc7b194960']=true,['2d83a6adaecf2c9643db41b6e5304c22']=true} "
            "local function mountaincamera(desc) "
            "if not game or not game.GetCurrentLevelString or game:GetCurrentLevelString()~='Level_Mountain' then return end "
            "local a=desc.Vars if not a then return end "
            "if desc.Type=='EventFromRegion' and mountainCameraEvents[desc.ObjectName] then "
            "a.onEnter=false a.onExit=false a.fireContinuously=false "
            "elseif desc.Type=='EventFromRegion' and desc.ObjectName=='eecf196522e6ad01d63b595c99a94fa3' "
            "and Names and Names['8790e3ade473ed442455d1987ba6ff53'] then "
            "a.effects=Names['8790e3ade473ed442455d1987ba6ff53'] "
            "elseif desc.Type=='SnapToCamTrigger' and desc.ObjectName=='91d5861a781402da21b2985d52dcf46e' then "
            "a.activationRegion=0 end end "
            "if not __JNETCreditSounds then __JNETCreditSounds=setmetatable({},{__mode='k'}) end "
            "local function creditsound(desc) local a=desc.Vars "
            "if desc.ObjectName=='ac2588adb7a2f6c1ad3d5dc48fbb2811' and "
            "desc.Type=='HudAddElement' and a and a.elementName=='JourneyTitle' then a.duration=12 a.fadeIn=1.5 a.fadeOut=1.5 a.alpha=1 end "
            "if desc.Type~='PlaySFXTrigger' or not a then return end "
            "if a.soundName=='M_9mC1' and not __JNETCreditSounds[a] then "
            "__JNETCreditSounds[a]={name=a.soundName,vol=a.vol} end "
            "local saved=__JNETCreditSounds[a] if saved then "
            "a.soundName=__JNETCreditsEnabled and saved.name or '' "
            "a.vol=__JNETCreditsEnabled and saved.vol or 0 end end "
            "for _,desc in ipairs(TriggerInstances or {}) do creditsound(desc) mountaincamera(desc) end "
            "if type(InitializeSignBarn)=='function' and not __JNETSignWrapped then "
            "local original=InitializeSignBarn InitializeSignBarn=function(...) "
            "if not __JNETCreditsEnabled and type(Credits)=='table' then "
            "local iscredits=false for _,s in ipairs(Credits) do for _,e in ipairs(s.Entries or {}) do "
            "if e.Text=='Thank you for playing' then iscredits=true end end end "
            "if iscredits then for _,s in ipairs(Credits) do for _,e in ipairs(s.Entries or {}) do "
            "if s.SectionName~='Companions' and e.Text~='Thank you for playing' then e.Text='' end end end end end "
            "return original(...) end __JNETSignWrapped=true end "
            "if type(TriggerInitialize)=='function' and TriggerInitialize~=__JNETCreditTrigger then "
            "local original=TriggerInitialize __JNETCreditTrigger=function(trig,desc) "
            "creditsound(desc) mountaincamera(desc) local a=desc.Vars if not __JNETCreditsEnabled and a then "
            "if desc.Type=='DisplayText' and a.text~='Thank you for playing' and "
            "a.text~='Companions Met Along the Way' and Names['500dd125848f5b8df2b076f6270bfbe5'] then a.text='' "
            "elseif desc.Type=='ScrollTextTrigger' and Names['500dd125848f5b8df2b076f6270bfbe5'] then a.alpha=0 end end "
            "return original(trig,desc) end TriggerInitialize=__JNETCreditTrigger end "
            "if type(PutHullsInNamesTable)=='function' and PutHullsInNamesTable~=__JNETParadiseHullWrapper then "
            "local original=PutHullsInNamesTable __JNETParadiseHullWrapper=function(barn) original(barn) "
            "if __JNETShrinkHulls then __JNETShrinkHulls() end "
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
    for(unsigned i=0;i<sizeof(jnet_hull_chunks)/sizeof(jnet_hull_chunks[0]);++i){
        int ok=original_loadbuffer(state,jnet_hull_chunks[i],strlen(jnet_hull_chunks[i]),"JNET-hulls","t")==0;
        if(ok)ok=pcallfn(state,0,0,0,0,NULL)==0;
        settop(state,top);
        if(!ok){message("[JNET] hull setup preparation failed");return 0;}
    }
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
    jnet_script_execute_original(resource,state);
    if(jnet_mepal_play && caller==fault_image+0x14dd12){
        const char empty[]="for _,row in ipairs(TimelineInstances or {}) do "
            "if row.ObjectName=='d00b9fce57bce1fe2e6f1f53983009bc' then return end end "
            "TimelineInstances={ObjectType='TimelineInstances'}";
        int top=gettop(state);
        int ok=original_loadbuffer(state,empty,sizeof(empty)-1,"JNET-freeplay","t")==0;
        if(ok)ok=pcallfn(state,0,0,0,0,NULL)==0;
        settop(state,top);
        if(!ok)message("[JNET] free-play timeline filtering failed; native timelines retained");
    }
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

}
static void jnet_script_execute(uintptr_t resource,lua_State *state) {
    jnet_script_execute_at(resource,state,(uintptr_t)__builtin_return_address(0));
}
#endif
