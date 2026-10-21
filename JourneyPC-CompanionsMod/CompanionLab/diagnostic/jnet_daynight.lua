local visitCallback, traceCallback=...
if not __JNETDayNight then
    __JNETDayNight = {visit=0}
end
local state = __JNETDayNight
local function copy(value)
    if type(value) ~= 'table' then return value end
    local result = {} for k,v in pairs(value) do result[k] = copy(v) end return result
end
local function blend(a,b,t)
    if type(a) == 'table' then
        local result = {} for i,v in ipairs(a) do result[i] = blend(v,b[i],t) end return result
    end
    return a+(b-a)*t
end
local function light(node, day, night, t)
    local d = copy(day)
    for key,value in pairs(night) do d[key] = blend(day[key],value,t) end
    local function c3(v) return GammaToLinearVec3(vec3_scalar_mult(v,1/255)) end
    local function c4(v) return GammaToLinearVec4(vec4_scalar_mult(v,1/255)) end
    local function v4(v) return {v[1],v[2],v[3],1} end
    node:SetShadow(d.ShadowLength or 1,d.ShadowAlpha or 1)
    node:SetFog(d.FogStart,d.FogEnd,c4(d.FogCloseColor),c4(d.FogFarColor),d.FogCloseAlpha,d.FogFarAlpha,d.FogHeightStart or 0,d.FogHeightEnd or 0)
    node:SetLight(v4(d.Env_LightDir),v4(GammaToLinearVec3(d.Env_LightColor)),d.Env_LightInt,
        v4(GammaToLinearVec3(d.Env_SpecularColor)),d.Env_SpecularInt,d.Env_SpecularPower,
        v4(GammaToLinearVec3(d.Env_AmbientColor)),d.Env_AmbientInt)
    node:SetSky(c3(d.SkyMaxColor),c3(d.SkyHighColor),c3(d.SkyMidColor),c3(d.SkyLowColor),c3(d.SkyMinColor),d.SkyHighHeight,d.SkyMidHeight,d.SkyLowHeight)
    node:SetBloom(d.BloomSize,d.BloomIntensity,d.BloomOverbright,d.ChromaticAberration)
    node:SetVelvet(d.VelvetRatio,d.VelvetInt)
end
-- `profiles` is only an index (node names and star emitters) so the level can be recognised while it loads. The level's lighting
-- data arrives afterwards as many small parts, one per call, and only when a lighting mode is in use; see jnet_daynight_live.h.
local function fresh(emitters) return {nodes={},clouds={},suns={},emitters=emitters,moon=false} end
function __JNETDayNightPart(level,alt,kind,value)
    local data=state.data
    if not data then return end
    local entry=data[level+1]
    if not entry then entry=fresh(profiles[level+1].emitters) data[level+1]=entry end
    local target=entry
    if alt==1 then
        if not entry.alternate then entry.alternate=fresh({}) end
        target=entry.alternate
    end
    if kind=='meta' then target.moon=value.moon
    else for key,item in pairs(value) do target[kind][key]=item end end
end
function __JNETDayNightInstall()
if type(LoadEnvNodes)=='function' and LoadEnvNodes~=state.load then
    local original=LoadEnvNodes
    state.load=function(env)
        original(env)
        state.visit=state.visit+1
        visitCallback()
        local ok,err=pcall(function()
        local barn=game:particleBarn()
        for i=0,barn:GetNumEmitters()-1 do
            local emitter=barn:GetEmitter(i) local name=emitter:GetName()
            if state.effects and state.effects[name] and emitterData[name] then
                emitter:GetBlueprint():intensity(emitterData[name].intensity or 1)
            end
        end
        state.nodes={} state.active=false state.sunTextures={} state.reported=false state.profile=nil state.data={} state.ready=false
        for _,d in ipairs(EnvNodeInstances) do
            state.nodes[d.ObjectName]={day=copy(d)}
        end
        end)
        if not ok then state.nodes=nil print('[JNET] day/night capture failed: '..tostring(err)) end
    end
    LoadEnvNodes=state.load
end
if type(LoadEnvFXParticles)=='function' and LoadEnvFXParticles~=state.particles then
    local original=LoadEnvFXParticles
    state.particles=function(barn,particles)
        local descriptions=copy(ParticleEmitterInstances)
        local profile=nil
        local nodes=EnvNodeInstances or {}
        for _,candidate in ipairs(profiles) do
            local matches=#nodes==candidate.nodeCount
            for _,d in ipairs(nodes) do
                if not candidate.nodes[d.ObjectName] then matches=false break end
            end
            if matches then profile=candidate break end
        end
        if profile then
            local existing={}
            for _,d in ipairs(descriptions) do existing[d.EmitterType]=true end
            for _,d in ipairs(profile.emitters) do
                if not existing[d.EmitterType] then
                    local added=copy(d) added.Enabled=false
                    table.insert(descriptions,added) existing[d.EmitterType]=true
                end
            end
        end
        ParticleEmitterInstances=descriptions
        original(barn,particles)
        state.effects={}
        for _,d in ipairs(descriptions) do
            if d.EmitterType:find('Stars') then
                state.effects[d.EmitterType]={name=d.ObjectName,enabled=d.Enabled,kill=d.KillParticlesOnDisable or false}
            end
        end
    end
    LoadEnvFXParticles=state.particles
end
end
local function quiet() end
local function apply(profile,enabled,amount,trace)
    trace('environment')
    for name,row in pairs(state.nodes) do
        local node=Names[name]
        if node and profile.nodes[name] then light(node,row.day,profile.nodes[name],amount) end
    end
    trace('clouds, wind and dust')
    for name,values in pairs(profile.clouds) do
            local mesh=Names[name]
            if mesh then for key,value in pairs(values) do
                local mixed=blend(value.day,value.night,amount)
                if type(mixed)=='table' then mesh:SetShaderParam4f(key,mixed,100000)
                else mesh:SetShaderParamf(key,mixed,100000) end
            end end
    end
    trace('sun/moon textures')
    for name,values in pairs(profile.suns) do
        local mesh=Names[name]
        if mesh then
            local night=profile.moon and amount>=.5
            if profile.moon and not values.separate then
                local texture=night and values.night.tex or values.day.tex
                if state.sunTextures[name]~=texture then
                    mesh:SetShaderTextureParam(game:resources(),'tex',texture,100000)
                    state.sunTextures[name]=texture
                end
            end
            local alpha=blend(values.day.alpha,values.night.alpha,amount)
            if profile.moon and not values.separate then
                alpha=night and (2*amount-1)*values.night.alpha or (1-2*amount)*values.day.alpha
            end
            if values.separate=='Moon' then alpha=math.max(0,2*amount-1)*values.night.alpha
            elseif values.separate=='Sun' then alpha=math.max(0,1-2*amount)*values.day.alpha end
            mesh:SetShaderParam4f('mltColor',blend(values.day.mltColor,values.night.mltColor,amount),100000)
            mesh:SetShaderParamf('alpha',alpha,100000)
        end
    end
    trace('star intensity')
    local stars=profile.moon and math.max(0,2*amount-1) or amount
    if #profile.emitters==0 then stars=0 end
    local barn=game:particleBarn()
    for i=0,barn:GetNumEmitters()-1 do
        local emitter=barn:GetEmitter(i)
        local name=emitter:GetName()
        if state.effects and state.effects[name] and emitterData[name] then
            emitter:GetBlueprint():intensity((emitterData[name].intensity or 1)*stars)
        end
    end
    for _,row in pairs(state.effects or {}) do
        local effect=Names[row.name]
        if effect then
            effect:killParticlesOnDisable(profile.moon or row.kill)
            effect:SetEnabled(stars>0)

        end
    end
end
function __JNETDayNightUpdate(elapsed,level,mode)
    local base=state.data and state.data[level+1]
    if not state.ready or not base or not state.nodes then return end
    local profile=base
    if base.alternate and mode==3 then profile=base.alternate end
    local enabled=mode~=0
    state.mode=mode

    local phase=mode==2 and (elapsed%1800)/900 or 0
    local amount=phase<=1 and phase or 2-phase
    amount=(mode==1 or mode==3) and 1 or amount*amount*(3-2*amount)
    local function trace(stage) if not state.reported and traceCallback then traceCallback(stage) end end
    if state.profile and state.profile~=profile then apply(state.profile,false,0,quiet) end
    state.profile=profile
    apply(profile,enabled,amount,trace)
    trace('complete; no runtime object allocations')
    state.reported=true
    state.active=enabled
end

__JNETDayNightInstall()
