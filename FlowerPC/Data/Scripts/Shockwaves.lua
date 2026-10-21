-------------------------------------------------------------------------------
-- Notes:  Loads all Shockwaves in the level. Shockwaves are stored in a
--		   lua table called "ShockwaveInstances"
--
-- Author: Rick Nelson
-- Date C: 1-16-2008
-------------------------------------------------------------------------------

function LoadShockwaves( ShockwaveBarn )
	for i, v in ipairs( ShockwaveInstances ) do

		local shockwavePtr = ShockwaveBarn:CreateShockwave()
		--ack, vectormath binding
		shockwavePtr:SetPosition( {v.Position[1], v.Position[2], v.Position[3]} )
		shockwavePtr:SetRadius( v.Radius, v.RadFalloff )
		shockwavePtr:SetDelay( v.Delay )
		shockwavePtr:SetShockwaveSpeed( v.Speed )
		shockwavePtr:SetColorChangeSpeed( math.ceil( v.ColorChangeSpeed * 255 ) / 255 )
		shockwavePtr:SetFireRepeatedly( v.FireRepeatedly )
		
		if v.ChangeFillerFlowers then
			shockwavePtr:SetFillerFlowerDensity( v.FillerFlowerDensity )
		end
		
		if v.SpawnGrassRipple then
			shockwavePtr:SetGrassRipple( v.GrassRippleStr, v.GrassRippleGrowRad, v.GrassRippleThickness )
			shockwavePtr:SetRippleDelay( v.GrassRippleDelay )
		end
		
		if v.ChangeGrass then		
			--ignoring alpha on color parameters
			shockwavePtr:SetGrassLandColor( {v.LandColor[1], v.LandColor[2], v.LandColor[3]} )
			shockwavePtr:SetGrassTopColor( {v.GrassColor[1], v.GrassColor[2], v.GrassColor[3]} )
		end
		
		if v.ChangeGrassHeight ~= nil and v.ChangeGrassHeight then
			shockwavePtr:SetGrassHeight( v.GrassHeight )
		end
		
		if v.ChangeDebris ~= nil and v.ChangeDebris then
			shockwavePtr:SetDebrisHeight( v.DebrisHeight )
		end
		
		if v.GrassHeightDependency ~= nil and v.GrassHeightDependency then
			shockwavePtr:SetGrassHeightDependency( v.MinGrassHeight )
		end
		
		if v.ChangeLandLight ~= nil and v.ChangeLandLight then
			shockwavePtr:SetLandLight( v.LandLight )
		end

        --for Events/Triggers
		Names[ v.ObjectName ] = shockwavePtr 

	end
	
	ShockwaveInstances = {}
	
end

