-------------------------------------------------------------------------------
-- Notes:  Loads all the decoration meshes on the level.
--
-- Author: Nick Clark
-- Date C: 10-08-2007
-------------------------------------------------------------------------------


function DecorationInitialize( resources, deco, meshDesc )
	deco:SetMatrix( meshDesc.Transformation )

	local brightness = { meshDesc.Brightness, meshDesc.Brightness, meshDesc.Brightness }
	local saturation = { meshDesc.Saturation, meshDesc.Saturation, meshDesc.Saturation }
	deco:SetColorCorrectMatrix( ColorCorrect( brightness, saturation, meshDesc.Hue ) )

	if meshDesc.ShaderLODMin and meshDesc.ShaderLODMax then
		deco:SetShaderLodDist( meshDesc.ShaderLODMin, meshDesc.ShaderLODMax )
	end
	if meshDesc.FadeMin and meshDesc.FadeMax then
		deco:SetFadeDist( meshDesc.FadeMin, meshDesc.FadeMax )
	end

	deco:SetShader( resources, meshDesc.Shader )
	if meshDesc.ShaderParams then
		for i,param in ipairs( meshDesc.ShaderParams ) do
			if "" ~= param.ParamName then
				if "texture" == param.ParamType then
					deco:SetShaderTextureParam( resources, param.ParamName, param.ParamVal, 100000 )
				elseif "float" == param.ParamType then
					deco:SetShaderParamf( param.ParamName, param.ParamVal, 100000 )
				elseif "float2" == param.ParamType then
					deco:SetShaderParam4f( param.ParamName, { param.ParamVal[1], param.ParamVal[2], 0, 0 }, 100000 )
				elseif "float3" == param.ParamType then
					deco:SetShaderParam4f( param.ParamName, { param.ParamVal[1], param.ParamVal[2], param.ParamVal[3], 0 }, 100000 )
				elseif "float4" == param.ParamType then
					deco:SetShaderParam4f( param.ParamName, param.ParamVal, 100000 )
				end
			end
		end
	end

	if meshDesc.RenderMode then
		local modeMap =
		{ ["Solid"] = DecorationBlend_Solid
		, ["Dither"] = DecorationBlend_Dither
		, ["Decal"] = DecorationBlend_Decal
		, ["Alpha"] = DecorationBlend_Alpha
		, ["Alpha Low Res"] = DecorationBlend_AlphaLowRes
		, ["Additive"] = DecorationBlend_Additive
		, ["Additive Low Res"] = DecorationBlend_AdditiveLowRes
		, ["Soft"] = DecorationBlend_Soft
		, ["Fog"] = DecorationBlend_Fog
		, ["Hidden"] = DecorationBlend_Hidden
		}
		deco:SetBlend( modeMap[ meshDesc.RenderMode ] )
	else
		deco:SetBlend( DecorationBlend_Solid )
	end
end

local decoRumbleTriggers =
{
	P_RoundColumnGrill_A =
	{
		[ 5 ] = { smallRumbleDuration = 0.25 }
	,	[ 45 ] = { smallRumbleDuration = 0.25 }
	}
	
}

local decoSoundTriggers =
{
	P_RoundColumnGrill_A =
	{
		[ 5 ] = {
			animPos = 5
		,   soundName = "CageDown"
		,	bankName = "LvlSfx"
		,	vol = 1
		,	minDist = 3.0
		,	maxDist = 25.0
		}
	,	[ 45 ] = {
			animPos = 45
		,   soundName = "CageUp"
		,	bankName = "LvlSfx"
		,	vol = 1
		,	minDist = 3.0
		,	maxDist = 25.0
		}
	}
,	P_GraveyardCenterTowerGrate_A_OIC =	
	{
		[ 5 ] = {
			animPos = 5
		,   soundName = "HubGratesOpen"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 15.0
		}
	,	[ 10 ] = {
			animPos = 10
		,   soundName = "HubGratesClothEscape"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 15.0
		}
	,   [ 45 ] = {
			animPos = 45
		,   soundName = "CageDown"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 15.0
		}		
	}	
,	P_MechHeadFan_A_OIC =
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "Guardian-FlapOpen"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 15.0
		}
	,	[ 70 ] = {
			animPos = 70
		,   soundName = "Guardian-FlapClose"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 15.0
		}		
	}	
,	P_MechNeckFan_A_OIC =
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "Guardian-FlapOpen"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 15.0
		}
	,	[ 70 ] = {
			animPos = 70
		,   soundName = "Guardian-FlapClose"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 15.0
		}		
	}	
,	P_MechWingLFan_A_OIC =
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "Guardian-FlapOpen"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 15.0
		}
	,	[ 70 ] = {
			animPos = 70
		,   soundName = "Guardian-FlapClose"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 10.0
		}			
	}				
,	P_MechWingRFan_A_OIC =
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "Guardian-FlapOpen"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 15.0
		}
	,	[ 70 ] = {
			animPos = 70
		,   soundName = "Guardian-FlapClose"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 2.0
		,	maxDist = 10.0
		}				
	}					
,   P_MechNeckStone_A_Hit = 
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "GuardianDamaged"
		,	bankName = "SharSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}				
	,	[ 3 ] = {
			animPos = 1
		,   soundName = "GuardianDestroyed"
		,	bankName = "SharSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}	
	}
,   P_MechNeckStone_A_Hit = 
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "GuardianDamaged"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}				
	,	[ 3 ] = {
			animPos = 1
		,   soundName = "GuardianDestroyed"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}	
	}
,   P_MechChestStone_A_Hit = 
	{
		[ 1 ] = {
			animPos = 1
		,   soundName = "GuardianDamaged"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}				
	,	[ 3 ] = {
			animPos = 1
		,   soundName = "GuardianDestroyed"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}	
	}		
,	P_MechBodyGrate_OIC =
	{
	    [ 1 ] = {
			animPos = 1
		,   soundName = "MechPieceOpen"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}		
	,   [ 34 ] = {
			animPos = 1
		,   soundName = "MechPieceClose"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}						
	}		
,	P_ClothGuardianNeck = 
	{
	    [ 1 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-MedClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}		
	,   [ 5 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-MedClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}						
	}
,	P_ClothGuardianHead = 
	{
	    [ 1 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-BigClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}		
	,   [ 5 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-BigClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}						
	}	
,	P_ClothGuardianWing = 
	{
	    [ 1 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-BigClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}		
	,   [ 5 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-BigClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}						
	}
,	P_ClothGuardianTail = 
	{
	    [ 1 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-TailClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}		
	,   [ 5 ] = {
			animPos = 1
		,   soundName = "ClothGuardian-TailClothMove"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 5.0
		,	maxDist = 50.0
		}						
	}	
,	P_DesertFinalTowerAnimGear1_A = 
	{
	    [ 75 ] = {
			animPos = 1
		,   soundName = "TowerGears-Turn"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 3.0
		,	maxDist = 50.0
		} 
	,   [ 155 ] = {
			animPos = 1
		,   soundName = "TowerGears-StopSlam"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 3.0
		,	maxDist = 50.0
		}			
	}	
}

-- This could be optimized, its set up right now to return the last marker passed
-- if we just want to return any marker passed then we can make this simpler.
-- In the case where multiple markers have been passed between oldNum and nextNum,
-- we return one at random.
local function GetLastMarkerPassed( oldNum, nextNum, markers )
	local lastFrame = nil
	
	--have we looped
	if nextNum < oldNum then
		-- Check everything above oldNum
		for frame,marker in pairs( markers ) do
			if frame > oldNum then lastFrame = frame end
		end
		-- Check everything below nextNum
		for frame,marker in pairs( markers ) do
			if frame <= nextNum then lastFrame = frame end
		end
	else
		-- Check everything both above oldNum and below nextNum 
		for frame,marker in pairs( markers ) do
			if frame <= nextNum and frame > oldNum then lastFrame = frame end
		end
	end
	return lastFrame
end

function TriggerRumbleFromDecoAnim( meshName, oldFrameNum, frameNum, meshPos )
	local markers = decoRumbleTriggers[ meshName ]
	if markers == nil then return end
	local lastFrame = GetLastMarkerPassed( oldFrameNum, frameNum, markers )
	if lastFrame == nil then return end
	local rumbleData = markers[ lastFrame ]
	local rumbleEvent = game:eventBarn():MetaAddEvent( "Rumble" )
	local rumble = Rumble.cast( rumbleEvent )
	rumble:falloffMinDist( 20 )
	for k,v in pairs( rumbleData ) do
		rumble[ k ]( rumble, v )
	end
	rumble:useOriginPos( true )
	rumble:originPos( meshPos )
	rumble:Start()
end

function TriggerSoundFromDecoAnim( meshName, oldFrameNum, frameNum, meshPos )
	local markers = decoSoundTriggers[ meshName ]
	if markers == nil then return end
	local lastFrame = GetLastMarkerPassed( oldFrameNum, frameNum, markers )
	if lastFrame == nil then return end
	local soundData = markers[ lastFrame ]
	local sound = soundData
	local maxDist = sound.minDist + 10
	if sound.maxDist ~= nil then maxDist = sound.maxDist end
	game:soundBarn():SoundPlayLua3D( sound.bankName, sound.soundName, meshPos, sound.minDist, maxDist, sound.vol )
end

function DecorationMeshes( resources, decBarn )
	--[[
	for i, meshDesc in ipairs( DecorationMeshInstances ) do
		local deco = decBarn:AddDecoration( resources, meshDesc.Mesh, meshDesc.Shader, meshDesc.Transformation )

		DecorationInitialize( resources, deco, meshDesc )

		if decoSoundTriggers[ meshDesc.Mesh ] then deco:hasSoundTriggers( true ) end
		if decoRumbleTriggers[ meshDesc.Mesh ] then deco:hasRumbleTriggers( true ) end

		deco:SetEnabled( true )

		Names[ meshDesc.ObjectName ] = deco
	end
	]]--
	DecorationMeshInstances = {}
end

function AddDecoToNames( deco, meshName, objectName )
	if decoSoundTriggers[ meshName ] then deco:hasSoundTriggers( true ) end
	if decoRumbleTriggers[ meshName ] then deco:hasRumbleTriggers( true ) end
	Names[ objectName ] = deco
end