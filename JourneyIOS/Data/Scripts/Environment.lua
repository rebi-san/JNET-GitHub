-------------------------------------------------------------------------------
-- Notes:  Loads all environment nodes in the level. Env nodes are stored in 
--		   a lua table called "EnvZNodeInstances"
--
-- Author: Nick Clark
-- Date C: 10-08-2007
-------------------------------------------------------------------------------

local function ToVec4( vec3 )
	local vec4 = {}
	vec4[1] = vec3[1]
	vec4[2] = vec3[2]
	vec4[3] = vec3[3]
	vec4[4] = 1
	return vec4
end

local function GammaToLinearVec3_Div255( v )
	return GammaToLinearVec3( vec3_scalar_mult( v, 1 / 255 ) )
end

local function GammaToLinearVec4_Div255( v )
	return GammaToLinearVec4( vec4_scalar_mult( v, 1 / 255 ) )
end

function LoadEnvNodes( env )
	for i, desc in ipairs( EnvNodeInstances ) do
		local envNode
		if desc.IsZNode then
			envNode = env:AddZNode( desc.ZPos, desc.Radius )
		else
			local ignoreLightDir = true
			if desc.Env_IgnoreLightDir ~= nil then
				ignoreLightDir = desc.Env_IgnoreLightDir
			end
			envNode = env:AddVolumeNode( desc.MinBoundingBox.Position, desc.MinBoundingBox.Extent, desc.MaxBoundingBox.Extent, desc.FadeDuration, ignoreLightDir )
		end

		if envNode == nil then
			print( "*** Error loading env node "..desc.ObjectName )
		else
			EnvNodeInitialize( envNode, desc )

			--load the envNode into the name table to be accessible to the trigger system
			Names[ desc.ObjectName ] = envNode
		end
	end
	--For some reason, this causes a crash.. not fixing right now
	--EnvNodeInstances = {}
end

function EnvNodeInitialize( envNode, desc )
	envNode:SetEnabled( desc.Enabled )

	if desc.IsZNode then
		envNode:SetZPosition( desc.ZPos )
		envNode:SetRadius( desc.Radius )		
	end

	if not desc.Env_SpecularColor then
		desc.Env_SpecularColor = desc.Env_LightColor
		desc.Env_SpecularInt = desc.Env_LightInt
		desc.Env_SpecularPower = 120
	end

	local fogHeightStart = 0
	local fogHeightEnd = 0
	if desc.FogHeightStart ~= nil and desc.FogHeightEnd ~= nil then
		fogHeightStart = desc.FogHeightStart
		fogHeightEnd = desc.FogHeightEnd
	end

	local ShadowLength = 1
	local ShadowAlpha = 1
	if desc.ShadowLength ~= nil and desc.ShadowAlpha ~= nil then
		ShadowLength = desc.ShadowLength
		ShadowAlpha = desc.ShadowAlpha
	end

	envNode:SetShadow(
		ShadowLength,
		ShadowAlpha )
	envNode:SetFog(
		desc.FogStart,
		desc.FogEnd,
		GammaToLinearVec4_Div255( desc.FogCloseColor ),
		GammaToLinearVec4_Div255( desc.FogFarColor ),
		desc.FogCloseAlpha,
		desc.FogFarAlpha,
		fogHeightStart,
		fogHeightEnd )
	envNode:SetLight(
		ToVec4( desc.Env_LightDir ),
		ToVec4( GammaToLinearVec3( desc.Env_LightColor ) ),
		desc.Env_LightInt,
		ToVec4( GammaToLinearVec3( desc.Env_SpecularColor ) ),
		desc.Env_SpecularInt,
		desc.Env_SpecularPower,
		ToVec4( GammaToLinearVec3( desc.Env_AmbientColor ) ),
		desc.Env_AmbientInt )
	envNode:SetSky(
		GammaToLinearVec3_Div255( desc.SkyMaxColor ),
		GammaToLinearVec3_Div255( desc.SkyHighColor ),
		GammaToLinearVec3_Div255( desc.SkyMidColor ),
		GammaToLinearVec3_Div255( desc.SkyLowColor ),
		GammaToLinearVec3_Div255( desc.SkyMinColor ),
		desc.SkyHighHeight,
		desc.SkyMidHeight,
		desc.SkyLowHeight )
	envNode:SetBloom(
		desc.BloomSize,
		desc.BloomIntensity,
		desc.BloomOverbright,
		desc.ChromaticAberration )
	envNode:SetVelvet(
		desc.VelvetRatio,
		desc.VelvetInt )

	--@TODO: This data should be exported by Gardener.
	--   John, 5-4-2008
	envNode:SetColorCorrect(
		{ 1.0, 1.0, 1.0 },
		{ 1.0, 1.0, 1.0 },
		{ 0.0, 0.0, 0.0 },
		0.0 )
		
	envNode:SetNamesTableName( desc.ObjectName )
		
	game:environment():ForceSort()
end

function UpdateEnvNamesTableEntry( envNodePtr, strName )
	Names[ strName ] = envNodePtr
end