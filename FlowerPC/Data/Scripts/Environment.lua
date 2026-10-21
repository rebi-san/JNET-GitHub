-------------------------------------------------------------------------------
-- Notes:  Loads all environment nodes in the level. Env nodes are stored in 
--		   a lua table called "EnvZNodeInstances"
--
--		   Will eventually load Env volume nodes from a table as well. 
--		   Currently these nodes are hard-coded into this file until they are
--         integrated into Gardener.
--
-- Author: Nick Clark
-- Date C: 10-08-2007
-------------------------------------------------------------------------------

local function ToVec4( vec3 )
	local vec4 = vec3;
	vec4[4] = 1
	return vec4;
end

function LoadEnvNodes( env )
	for i, v in ipairs( EnvNodeInstances ) do

		local envNodePtr
	
		if v.IsZNode then
			envNodePtr = env:AddZNode( v.ZPos, v.Radius )
		else
			local ignoreLightDir = true
			if v.IgnoreLightDir ~= nil then
				ignoreLightDir = v.IgnoreLightDir
			end
			envNodePtr = env:AddVolumeNode( v.MinBoundingBox.Position, v.MinBoundingBox.Extent, v.MaxBoundingBox.Extent, v.FadeDuration, ignoreLightDir )
			
			if envNodePtr == nil then
				print( "*** Error loading volume node "..v.ObjectName )
			else
				--load the envNode into the name table to be accessible to the trigger system
				Names[ v.ObjectName ] = envNodePtr  -- note this is only for VolumeNodes right now			
			end
		end
		
		if envNodePtr then
			env:SetCurrentNodeEnabled( v.Enabled )
			env:SetCurrentNodeFog( v.FogStart, v.FogEnd, v.FogCloseColor, v.FogFarColor, v.FogCloseAlpha, v.FogFarAlpha )
			env:SetCurrentNodeLight( ToVec4( v.LightDir ), v.LightColor, v.LightInt, v.AmbientColor, v.AmbientInt, v.GlobalInt )
			env:SetCurrentNodeSky( v.SkyTopColor, v.SkyBotColor, v.SmogColor, v.SkyHeight, v.SmogHeight, v.SkyTexture )
			env:SetCurrentNodeSun( v.SunColor, v.HaloColor, v.SunSize, v.SunSharp, v.HaloSharp, v.SunInt )
			env:SetCurrentNodeBloom( v.StepBegin, v.StepEnd, v.BloomAlpha, v.VelvetRatio, v.VelvetInt )

			--@TODO: This data should be exported by Gardener.
			--   John, 5-4-2008
			env:SetCurrentNodeColorCorrect(
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 1.0, 1.0 },
				{ 1.0, 1.0, 1.0 },
				0.0
				)
		end

	end
	--For some reason, this causes a crash.. not fixing right now
	--EnvNodeInstances = {}
	
end

