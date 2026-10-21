PT("DBUG")
local DM="zDM_LocalJoint"

local g_pad

local function Pad(p)
return g_pad:WasButtonPressed(p)
end

local function PadDown(p)
return g_pad:IsButtonDown(p)
end

DebugKeys={}
DebugActions=
{{Key=23,Action=function(game) game:netGui():SelectPreviousItem() end}
,{Key=25,Action=function(game) game:netGui():SelectNextItem() end}
,{Key=19,Action=function(game) game:pauseSystem():Toggle(game) end}
}

function ProcessDebugKeys(game,input)
g_pad=input:GetPad(0)
for i,v in ipairs(DebugActions) do
if Pad(v.Key) then
v.Action(game)
end end end
PT("D1")

--init
if not Mod then
Mod={
localOutfit="None"
,wmFace="Mask"
,hideBody=false
,hideScarf=false
,hideRobe=false
,Loops={}
}
local j={"Joints","ObjOutfits"}
for i,v in ipairs(j) do
local f=assert(io.open("Data/Mods/ObjectOutfits/"..v..".json","r"))
local t=Decode(f:read("*all"))
Mod[v]=t
f:close()
end
local n=0
for line in io.lines("Data/Mods/ObjectOutfits/HideRobe.txt") do
if string.lower(line) == "on" then Mod["hideRobe"]=true end
end end
PT("D2")

--crash fix
Mod.NoCrash=function()
local good={[0]=1,1,1,1,1,1,1,1,0,1,0,1}
if good[game:GetCurrentLevelNumber()] == 1 then return true end
end

--queue res
Mod.NewWads={Cloth=1,ClothGlow=1}

Mod.QRes=function(rC)
if Mod.NewWads[rC.name] then
if Mod.hideRobe then
rC.name=rC.name.."OO"
end end end

function QueueResourcesForLoad(resManager,resTbl,alloc)
for i,resCons in ipairs(resTbl) do
Mod.QRes(resCons)
local metatbl=_G[resCons.class]
local res=metatbl.new(alloc)
resCons.class=nil
for varName,varValue in pairs(resCons) do
local varAccessor=res.__vars[varName]
if varAccessor then varAccessor(res,varValue) end
end
resManager:AddToLoadList(res)
end end

function QueueResourcesForUnload(resManager,resTbl)
Mod.Loops={}
for i,resCons in ipairs(resTbl) do
resManager:AddToUnloadList(resCons.class,resCons.name)
end end

--if Mod.hideRobe then
--function QueueResourcesForUnload(resManager,resTbl)
--Mod.Loops={}
--for i,resCons in ipairs(resTbl) do
--if Mod.NewWads[resCons.name] then resCons.name=resCons.name.."OO" end
--resManager:AddToUnloadList(resCons.class,resCons.name)
--end end end
PT("D3")

--obj load
Mod.LevelObjects={
Add={
 Clump={}
,Marker={}
,Trigger={}
}
,Edit={
 Clump={}
,Marker={}
,Trigger={}
}
,Null={
 Clump={NULL=[[Objects={}]]}
,Marker={}
,Trigger={NULL=[[Type="NOP",Vars={}]]}
}
}

local function AddObjects(objType,lvl)
local tbl=Mod.LevelObjects.Add[objType][lvl]
if tbl then
for k,v in pairs(tbl) do
local obj=load("table.insert("..objType.."Instances,{"..v.."})")
obj()
end end end

local function EditObjects(objType,lvl)
local tbl=Mod.LevelObjects.Edit[objType][lvl]
if tbl then
for i,v in ipairs(_G[objType.."Instances"]) do
local n=v.ObjectName
if tbl[n] then
local obj=load(objType.."Instances["..tostring(i).."]={"..tbl[n].."}")
obj()
end end end end

local function NullObjects(objType,lvl)
local tbl=Mod.LevelObjects.Null[objType]
if tbl[lvl] then
for i,v in ipairs(_G[objType.."Instances"]) do
local n=v.ObjectName
if tbl[lvl][n] then
local obj=load(objType.."Instances["..tostring(i).."]={ObjectName=[["..n.."]],"..tbl["NULL"].."}")
obj()
end end end end

Mod.DoLevelObjects=function(objType,lua)
if not lua then
local uid=tostring(game:GetCurrentLevelNumber())
AddObjects(objType,"All")
AddObjects(objType,uid)
EditObjects(objType,"All")
EditObjects(objType,uid)
NullObjects(objType,"All")
NullObjects(objType,uid)
end end
PT("D4")

--loops
Mod.Timers={}
Mod.SetTimer=function(t) Mod.Timers[t]=game:gameTiming():GetTotalTime() end
Mod.TimeSince=function(t) return game:gameTiming():GetTotalTime()-Mod.Timers[t] end

Mod.Ticker=function(t)
t.timer=t.timer + 1
if t.timer >= t.delay then
t.timer=0
return true
end end

Mod.DoLoops=function()
for k,v in pairs(Mod.Loops) do
v.fun(v)
end end
PT("D5")

--outfit funcs
Mod.SetObjOutfit=function(outfit)
if Mod.NoCrash() then
for i,v in ipairs(Mod.Joints) do
for i2,v2 in ipairs(v.Mesh) do
Names[DM..v2]:SetEnabled(false)
end end
if Mod.ObjOutfits[outfit] then
for i,v in ipairs(Mod.ObjOutfits[outfit]) do
Names[DM..v]:SetEnabled(true)
end end
end end

Mod.SetWMFace=function()
if Mod.NoCrash() then
local anim={}
if Mod.wmFace == "Mask" then
anim={25,50}
else
anim={0,25}
end
SpawnEvent{DecorationPlayAnimation={
 animFadeSpeed=1
,bindPosAnimName=""
,decorationClump=Names["zCL_WMMask"]
,frame=anim[1]
,frameEnd=anim[2]
,targetAnimName=""
,useFrameEnd=true
}}
end end

Mod.DoHideBody=function()
if Mod.NoCrash() then
local lvl=game:GetCurrentLevelNumber()
local m="MeshDudeLocal"
local s=0.1
local r="973b49e7dcf903f724d534d0724c2033"
Names["zMK_LocalRoot"]:SetPosition(game:playerBarn():GetLocalDude():GetPos()) 
if lvl == 3 then s=0.09
elseif lvl == 5 then r="8c5431f15908eec7fdc380aaf1f0be5a"
elseif (lvl>=6 and lvl <=8) then m=m.."Frozen" 
end
if Mod.hideBody then m="Mesh" end
Names["7e6bbcef1785378406eb42a51e3bf5f8"]:SetShader(game:resources(), m)
SpawnEvent{InitLocalDude={runMesh=Names["7e6bbcef1785378406eb42a51e3bf5f8"], scale=s, runMeshRemote=Names[r], initialPos=Names["zMK_LocalRoot"]}}
end end

Mod.DoHideScarf=function()
if Mod.NoCrash() then
if Mod.hideScarf then 
Mod.Loops["HideScarf"]={timer=0,delay=30,fun=function(this)
if Mod.Ticker(this) then
SpawnEvent{SetScarfDirection={direction=Names["zMK_LocalRoot"]
,duration=1
,windMult=-1e+7}}
end end}
else 
Mod.SetTimer("ShowScarf")
Mod.Loops["HideScarf"]={done= false, fun=function(this)
if this.done then Mod.Loops["HideScarf"]=nil
end
if Mod.TimeSince("ShowScarf") >= 1 then
this.done=true
Mod.hideBody=not Mod.hideBody
Mod.DoHideBody() 
end end}
end end end
PT("D6")

--menu
Mod.Menu={
isUp=false
,crsr=1
,trySit=false
,bMov=0
,sMov=0
,xMov=3

,{Name="Resume"
,Text1="Resume"
,Text2=""
,Action=function () 
Mod.Menu.Stop()
end
}
,{Name="Outfit"
,Text1="Set Outfit"
,Text2=": None"
,Num=1
,Outfits={"None","AncestorSpirit","LongtailCarpet","SmallJellyfish","BigJellyfish","WarMachine"}
,Action=function (this) 
this.Num=this.Num + 1
if not this.Outfits[this.Num] then this.Num=1 end
local otft=this.Outfits[this.Num]
Mod.localOutfit=otft
this.Text2=": "..otft
Mod.SetObjOutfit(otft)
end
}
,{Name="WM_Face"
,Text1="War Machine Face"
,Text2=": Mask"
,Action=function (this) 
if Mod.wmFace == "Mask" then Mod.wmFace="Eye" else Mod.wmFace="Mask" end
this.Text2=": "..Mod.wmFace
Mod.SetWMFace()
end
}
,{Name="HideBody"
,Text1="Hide Body"
,Text2=": Off"
,Action=function (tgl)
if Mod.SafeHide("b") then
Mod.hideBody = not Mod.hideBody
Mod.DoHideBody()
elseif tgl==1 then Mod.Menu[4].Text2=": Off"
else Mod.Menu[4].Text2=": On"
end end 
}
,{Name="HideScarf"
,Text1="Hide Scarf"
,Text2=": Off"
,Action=function (tgl)
if Mod.SafeHide("s") then
Mod.hideScarf = not Mod.hideScarf
Mod.DoHideScarf()
elseif tgl==1 then Mod.Menu[5].Text2=": Off"
else Mod.Menu[5].Text2=": On"
end end
}
,{Name="HideRobe"
,Text1="Hide Robe (must restart mod)"
,Text2=": Off"
,Action=function (tgl)
local s=""
if tgl == 1 then
Mod.hideRobe=true
s="ON"
else
Mod.hideRobe=false
s="OFF"
end
local f=io.open("Data/Mods/ObjectOutfits/HideRobe.txt","w+")
f:write(s)
f:close()
end
}
,{Name="InBadLvl"
,Text1=""
,Text2=""
,Action=function ()
end
}
}
if Mod.hideRobe then Mod.Menu[6].Text2=": On" end

Mod.SafeHide=function(n)
local M=Mod.Menu
if game:playerBarn():IsMultiplayer() then
local vel=game:playerBarn():GetLocalDude():GetVel()
vel=math.abs(vel[1])+math.abs(vel[3])
if game:GetCurrentLevelNumber()==3 then vel = vel/0.9 end
if (vel<=1 and game:playerBarn():GetLocalDude():IsInAir()==false) then
M.bMov=0
M.sMov=0
M[7].Text1=""
return true
else
M[n.."Mov"]=M[n.."Mov"]+1
if M[n.."Mov"]<=2 then
Mod.Loops["ResetMov"]={timer = 0, delay = 60, fun=function(this) 
if Mod.Ticker(this) then
if Mod.SafeHide("x") then Mod.Loops["ResetMov"]=nil end 
end end}
return true
else
M[7].Text1='(stop moving to hide/un-hide)'
return false
end 
end
else 
M[7].Text1="" 
return true 
end 
end

Mod.Menu.Stop=function()
Mod.Menu.isUp=false
Mod.Menu.crsr=1
Vars.Dude.kInputDeadSpace(0.1)
Vars.Dude.kIdleTime(40)
Mod.Menu.trySit=false
SpawnEvent{HudRemoveElement={elementName="MenuBG"}}
end

Mod.DoMenu=function()
local M=Mod.Menu

if M.isUp then
if M.trySit then
if game:cameraSystem():UsingPauseCam() then
Vars.Dude.kInputDeadSpace(10)
Vars.Dude.kIdleTime(0.1)
M.trySit=false
game:pauseSystem():Deactivate(game:cameraSystem())
else game:pauseSystem():Activate(game:cameraSystem()) end 
end

if Pad(20) then
M.Stop()
end
if Pad(13) then
local v=M[M.crsr]
if v.Text2 == ": Off" then 
v.Text2=": On"
v.Action(1)
elseif v.Text2 == ": On" then 
v.Text2=": Off" 
v.Action(0)
else
M[M.crsr].Action(M[M.crsr])
end end

if Mod.TimeSince("menu") >= 0.15 then
local mover=g_pad:Left()
M.crsr=M.crsr + math.ceil(mover[2])
if M.crsr < 1 then M.crsr=1 end
if M.crsr > 6 then M.crsr=6 end
SpawnEvent{DisplayText={text=">",x=-0.61,y=0.5-(0.075*M.crsr),duration=.56,fadeTime=0.25,leftJustify=true}}
SpawnEvent{DisplayText={text="Outfit Mod Menu",x=-0.58,y=0.51,duration=.56,fadeTime=0.25,leftJustify=true,scale=0.06}}
for i,v in ipairs(M) do
local menutext=v.Text1..v.Text2
if (v.Cnfm ~= nil and M.crsr ~= i) then v.Cnfm=false end
if v.Cnfm then menutext="Confirm "..v.Text1.."?" end
SpawnEvent{DisplayText={text=menutext,x=-0.58,y=0.5-(0.075*i),duration=.56,fadeTime=0.25,leftJustify=true}}
end
Mod.SetTimer("menu")
end
elseif (PadDown(12) and PadDown(13) and PadDown(19)) then
PT("menu")
M.isUp=true
Mod.SetTimer("menu")
M.trySit=true
SpawnEvent{HudAddElement={alpha=0.33,dimx=1.3,dimy=.65,duration=1e+008,elementName="MenuBG",fadeIn=0.3,fadeOut=0.25,posx=-0.65,posy=-0.05,texName="Black",depth=0,isPauseElem=true}}
end 
end
PT("D7")

--orig
function ProcessDebugKeys(game,input)
g_pad=input:GetPad(0)
for i,v in ipairs(DebugActions) do
if Pad(v.Key) then
v.Action(game)
end end
Mod.DoLoops()
end
PT("D_DONE")