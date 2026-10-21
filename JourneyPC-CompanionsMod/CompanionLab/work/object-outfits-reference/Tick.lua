local function Clip(x,xmin,xmax)
return math.min(xmax,math.max(xmin,x))
end
local function Delerp(t,x0,x1)
local diff=x1 - x0
if diff > -0.00001 and diff < 0.00001 then return 1 end
local val=(t - x0) / diff
return Clip(val,0,1)
end
local function Lerp(t,x0,x1)
local val=Clip(t,0,1)
return x1*val+x0*(1-val)
end
local function Dist2Sq(x1,y1,x2,y2)
 return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)
end

function PrintMeta(out) end

function DumpMetaSys() end

function DumpClassSizes() end

--mod
function PT() end --print

local function FinishMod()
PT("TICK")
function SpawnEvent(eventTable)
local eventBarn=_G["game"]:eventBarn()

for eventType,eventParams in pairs(eventTable) do
local event=eventBarn:MetaAddEvent(eventType)
local eventImplPtr=_G[eventType].cast(event)

for key,val in pairs(eventParams) do
eventImplPtr[key](eventImplPtr,val)
end
eventImplPtr:Start()
end end

Mod.QRes = function(rC)
if rC.name=="DecorationMeshInstances" then 
rC.source="Mods/ObjectOutfits/"..game:GetCurrentLevelString().."_DMI.lua"
end end

Mod.LevelObjects.Add.Clump["All"]={
zCL_Jellyfishes=[[ObjectName="zCL_Jellyfishes",
Objects={"zDM_LocalJoint17","zDM_LocalJoint18"}]]
,zCL_Spirits=[[ObjectName="zCL_Spirits",
Objects={"zDM_LocalJoint5"}]]
,zCL_WMMask=[[ObjectName="zCL_WMMask",
Objects={"zDM_LocalJoint6","zDM_LocalJoint9"}]]
}

Mod.LevelObjects.Add.Marker["All"]={
zMK_LocalRoot=[[ObjectName="zMK_LocalRoot"
,Position={0,0,0}
,Quaternion={0,0,0,1}
,Scale={1,1,1}
,Enabled=true]]
}

Mod.LevelObjects.Add.Trigger["All"]={
zTR_LocalRootAttach=[[ObjectName="zTR_LocalRootAttach",
Type="AttachObjectToJoint",
Vars={meshJoint="" 
,objects=Names["zMK_LocalRoot"]
,useLocalDudeMesh=true
,attachPos=false
,relative=true
,framesDelayed=0}]]
,zTR_LongtailCloth=[[ObjectName="zTR_LongtailCloth"
,Type="ClothSetProperties"
,Vars={clothOrFlags=Names["zDM_LocalJoint16"]
,colDetApply=true
,colDetSelf=false
,colDetSpheres=false
,colDetVoxels=true
,gravity={0,0,0}
,gravityApply=true
,wind={0,0,0}
,windApply=true
,accumulateDirt=true
,accumulateDirtApply=true
,gameplayOn=false 
,gameplayApply=true
,dampingApply=false
,dragApply=false
,globalGravityMultApply=false
,globalWindMultApply=false
,simpleShadingApply=false
,maxIterationsApply=false 
}]]
,zTR_LongtailAnimate=[[ObjectName="zTR_LongtailAnimate"
,Type="DecorationPlayAnimation"
,Vars={animFadeSpeed=1
,bindPosAnimName=""
,decorationClump=Names["zDM_LocalJoint16"]
,loopAnim=true
,speed=0.75
,targetAnimName="FlatFishSwim"
}]]
,zTR_JellyfishCloth=[[ObjectName="zTR_JellyfishCloth"
,Type="ClothSetProperties"
,Vars={clothOrFlags=Names["zCL_Jellyfishes"]
,colDetApply=true
,colDetSelf=false
,colDetSpheres=false
,colDetVoxels=false
,damping=0.01
,dampingApply=true
,drag=0.01
,dragApply=true
,globalGravityMult=1
,globalGravityMultApply=true
,accumulateDirt=true
,accumulateDirtApply=true
,globalWindMultApply=false
,gameplayApply=false
,simpleShadingApply=false
,maxIterationsApply=false
,gravityApply=false
,windApply=false
}]]
,zTR_JellyfishAnimate=[[ObjectName="zTR_JellyfishAnimate"
,Type="DecorationPlayAnimation"
,Vars={animFadeSpeed=1
,bindPosAnimName=""
,decorationClump=Names["zCL_Jellyfishes"]
,loopAnim=true
,speed=0.1
,targetAnimName="JellyFishIdle"
,useFrameEnd=false
}]]
,zTR_SpiritAnimate=[[ObjectName="zTR_SpiritAnimate"
,Type="DecorationPlayAnimation"
,Vars={animFadeSpeed=1
,decorationClump=Names["zCL_Spirits"]
,loopAnim=true
,speed=0.25
,frame=0
,frameEnd=0
,targetAnimName="SpiritIdleDown"
,useFrameEnd=false
,bindPosAnimName=""
}]]
}

for i,v in ipairs(Mod.Joints) do
Mod.LevelObjects.Add.Marker["All"]["zMK_LocalJoint"..i]=[[ObjectName="zMK_LocalJoint]]..i..[[",
Position={0,0,0},Quaternion={0,0,0,1},Scale={1,1,1},Enabled=true]]

local c=[[ObjectName="zCL_LocalJoint]]..i..[[",Objects={]]

for i2,v2 in ipairs(v.Mesh) do
c=c..[["zDM_LocalJoint]]..v2..[[",]]
end

c=c:sub(1,-2)
c=c.."}"
Mod.LevelObjects.Add.Clump["All"]["zCL_LocalJoint"..i]=c

Mod.LevelObjects.Add.Trigger["All"]["zTR_LocalJoint_DMtoMK"..i]=[[ObjectName="zTR_LocalJoint_DMtoMK]]..i..[[",
Type="AttachObjectToMarker",
Vars={
framesDelayed=0
,objects=Names["zCL_LocalJoint]]..i..[["]
,src=Names["zMK_LocalJoint]]..i..[["]
,attachPos=true
,attachRotScale=true
,cacheScale=true
,relativePos=true
,relativeRot=true
}]]

Mod.LevelObjects.Add.Trigger["All"]["zTR_LocalJoint_MKtoDude"..i]=[[ObjectName="zTR_LocalJoint_MKtoDude]]..i..[[",
Type="AttachObjectToJoint",
Vars={
meshJoint="]]..v.Name..[[" 
,objects=Names["zMK_LocalJoint]]..i..[["]
,useLocalDudeMesh=true 
,relative=true
,framesDelayed=0
}]]
end

function PrintNameTableSize()
local DM="zDM_LocalJoint"
Mod.Loops["Menu"]={fun=Mod.DoMenu}
if Mod.NoCrash() then
Mod.Menu[7].Text1=""
for i,v in ipairs(Mod.Joints) do Names["zTR_LocalJoint_DMtoMK"..i]:Start() end
for i,v in ipairs(Mod.Joints) do Names["zTR_LocalJoint_MKtoDude"..i]:Start() end
ClumpInstances={ObjectType="ClumpInstances",
{ObjectName="zCL_AllCloth",
Objects={DM.."5",DM.."16",DM.."17",DM.."18"}}
} 
LoadClumps(game:clumpBarn(),true)
SetClumpMembers(game:clumpBarn(),true)
PT(1)
if game:GetCurrentLevelNumber()==6 then
SpawnEvent{FrozenCloth={cloth=Names["zCL_AllCloth"],maxFreeze=0.25}}
end
PT(2)
for i,v in ipairs{"LocalRootAttach","LongtailCloth","LongtailAnimate","JellyfishAnimate","JellyfishCloth","SpiritAnimate"} do
Names["zTR_"..v]:Start()
end
PT(3)
PT(4)
Mod.SetObjOutfit(Mod.localOutfit)
PT(5)
Mod.SetWMFace()
Mod.Loops["Init"]={timer=0,delay=15,fun=function(this)
if Mod.Ticker(this) then
Mod.DoHideScarf()
Mod.DoHideBody()
Mod.Loops["Init"]=nil
end end}
Mod.bMov=0
Mod.sMov=0
PT(6)
else
Mod.Menu[7].Text1="(mod disabled to prevent crash)"
end
PT(7)
end
--
PT("READY!")
function Tick() end
end

function Tick()
FinishMod()
end