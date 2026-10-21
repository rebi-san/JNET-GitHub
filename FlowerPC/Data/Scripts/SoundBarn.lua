-------------------------------------------------------------------------------
-- Notes:  SoundBarn
--
-- Author: Martin Middleton
-- Date C: 04-09-2008
-------------------------------------------------------------------------------

function LoadSoundEmitters( soundBarn )
	soundBarn:StopAllEmitters()
	LoadSoundDataFromGardener()
	--@TODO - Load emitters from soundData
	for i,sound in ipairs( SoundEmitterInstances ) do
		local emitter = soundBarn:AddSoundEmitter( sound.BankName, sound.SoundName )
		emitter:posX( sound.Position[1] )
		emitter:posY( sound.Position[2] )
		emitter:posZ( sound.Position[3] )
		emitter:vol( sound.Volume )
		emitter:minDist( sound.MinDist )
		emitter:is3D( sound.Is3D )
		emitter:autoPitch( sound.AutoPitch )
		emitter:fromController( sound.FromController or false )
		emitter:followSwarm( sound.FollowSwarm or false )
		if sound.Enabled then
			soundBarn:PlayEmitter( emitter )
		end
		
		Names[ sound.ObjectName ] = emitter
	end
	
	SoundEmitterInstances = {}
	
end

function LoadSoundTriggerData()
	for k,trigger in pairs( soundData.Triggers ) do
		local triggerData = Names[ trigger.ObjectName ]
		-- @TODO - If trigger no longer exists we should remove it from soundData
		if triggerData then
			for key,value in pairs( trigger.Vars ) do
				triggerData[ key ]( triggerData, value )
			end
		end
	end
end

-- The names table needs to be created before this will work
function CleanDeadSoundData()
	for k,trigger in pairs( soundData.Triggers ) do
		if not Names[ trigger.ObjectName ] then soundData.Triggers[ k ] = nil end
	end
	for k,emitter in pairs( soundData.Emitters ) do
		if not Names[ emitter.ObjectName ] then soundData.Emitters[ k ] = nil end
	end
end

function ResetSoundData()
	soundData = { Emitters = {}, Triggers = {} }
end

-- This should be called after data is loaded from SoundDataInstances
function LoadSoundDataFromGardener()
	--ResetSoundData()
	-- Load new emitter instances into sound data
	for i,emitter in ipairs( SoundEmitterInstances ) do
		if emitter.ObjectName and not soundData[ emitter.ObjectName ] then
			local emitterData = {}
			for key,value in pairs( emitter ) do
				emitterData[ key ] = value	
			end
			soundData.Emitters[ emitter.ObjectName ] = emitterData
		end
	end
	
	-- Load new trigger instances into sound data
	for i,trigger in ipairs( TriggerInstances ) do
		if ( trigger.TriggerType == "PlaySFXTrigger" or trigger.TriggerType == "PlaySFX3DTrigger" ) 
		and not soundData[ trigger.ObjectName ] then
			local triggerData = {}
			for key,value in pairs( trigger ) do
				triggerData[ key ] = value
			end
			soundData.Triggers[ trigger.ObjectName ] = triggerData
		end
	end
end

function WriteSoundData( filename )
	print( "Opening "..filename )
	local dataFile = io.open( filename, "w+" )
	dataFile:write( "-- Press Z to update this file from level data\n" )
	dataFile:write( "-- Press spacebar to reload this file\n\n" )
	dataFile:write( "soundData =" )
	PrintTable( dataFile, soundData )
	io.close( dataFile )
end

