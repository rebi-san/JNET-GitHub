-------------------------------------------------------------------------------
-- Notes:  SoundBarn
--
-- Author: Martin Middleton
-- Date C: 04-09-2008
-------------------------------------------------------------------------------

function LoadSoundEmitters( soundBarn )
	soundBarn:StopAllEmitters()
	for i,sound in ipairs( SoundEmitterInstances ) do
		local emitter = soundBarn:AddSoundEmitter( sound.BankName, sound.SoundName )
		SoundEmitterInitialize( emitter, sound )
		
		if sound.Enabled then
			soundBarn:PlayEmitter( emitter )
		end

		Names[ sound.ObjectName ] = emitter
	end
	
	SoundEmitterInstances = {}
end

function SoundEmitterInitialize( emitter, desc )
	emitter:SetPos( desc.Position[ 1 ], desc.Position[ 2 ], desc.Position[ 3 ] )
	emitter:SetBank( desc.BankName )
	emitter:SetSound( desc.SoundName )
	emitter:vol( desc.Volume )
	emitter:minDist( desc.MinDist )
	local maxDist = desc.MinDist + 50
	if desc.FadeDist then maxDist = desc.MinDist + desc.FadeDist end
	emitter:maxDist( maxDist )
	emitter:is3D( desc.Is3D )
	emitter:autoPitch( desc.AutoPitch )
	if desc.DopplerMult then emitter:dopplerMult( desc.DopplerMult ) end
end	

function __genOrderedIndex( t )
    local orderedIndex = {}
    for key in pairs(t) do
        table.insert( orderedIndex, key )
    end
    table.sort( orderedIndex )
    return orderedIndex
end

function tablelength(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

function orderedNext(t, state)
    -- Equivalent of the next function, but returns the keys in the alphabetic
    -- order. We use a temporary ordered key table that is stored in the
    -- table being iterated.

    --print("orderedNext: state = "..tostring(state) )
    if state == nil then
        -- the first time, generate the index
        t.__orderedIndex = __genOrderedIndex( t )
        key = t.__orderedIndex[1]
        return key, t[key]
    end
    -- fetch the next value
    key = nil
    for i = 1,tablelength(t) do
        if t.__orderedIndex[i] == state then
            key = t.__orderedIndex[i+1]
        end
    end

    if key then
        return key, t[key]
    end

    -- no more value to return, cleanup
    t.__orderedIndex = nil
    return
end

function orderedPairs(t)
    -- Equivalent of the pairs() function on tables. Allows to iterate
    -- in order
    return orderedNext, t, nil
end


function LoadSoundEffectsData( soundBarn )
	soundBarn:ResetEffectsData()
	for name,effectData in orderedPairs( soundEffectData ) do
		local maxDist = effectData.minDist + 50
		if effectData.maxDist then maxDist = effectData.maxDist end
		local effect = soundBarn:LuaAddEffect( name, effectData.bankName, effectData.soundName, effectData.minDist, maxDist, effectData.autoPitchBend )
		if effectData.dopplerMult then effect:dopplerMult( effectData.dopplerMult ) end
	end
	soundEffectData = {}
end

function GetSoundDebugContext()
	local ctx = game:soundBarn():GetDebugContext()
	local ctxData = {}
	ctxData.isEnabled = ctx:isEnabled()
	return Json.Encode( ctxData )
end

--[[
function CreateDataTable( object )
	local classtbl = getmetatable( object )
	for varname,varfun in pairs( classtbl.__vars ) do
		
	end
end
]]--
