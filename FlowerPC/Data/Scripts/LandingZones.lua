-------------------------------------------------------------------------------
-- Notes:  Loads all landing zones in the level. Landing zones are stored in a
--		   lua table called "LandingZoneInstances"
--
-- Author: Nick Clark
-- Date C: 10-08-2007
-------------------------------------------------------------------------------

function LoadLandingZones( landingZoneBarn )
	for i, v in ipairs( LandingZoneInstances ) do
		local landingZone = landingZoneBarn:CreateLandingZone()
		landingZone:SetPosition( v.BoundingBox.Position, v.BoundingBox.Extent )
		landingZone:SetLandHeight( 0.1 )
	    landingZone:m_allowRelaunch( v.AllowRelaunch )
	    
	    if v.AutoLand ~= nil then landingZone:m_autoLand( v.AutoLand )
	    else landingZone:m_autoLand( true ) end
	    
	    if v.SoundMinDist then landingZone:m_soundMinDist( v.SoundMinDist )
	    else landingZone:m_soundMinDist( 10 ) end

		if v.SoundMaxDist then landingZone:m_soundMaxDist( v.SoundMaxDist )
	    else landingZone:m_soundMaxDist( 25 ) end

		Names[ v.ObjectName ] = landingZone 
	end
	
	LandingZoneInstances = {} 
end

