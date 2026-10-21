-------------------------------------------------------------------------------
-- Notes:  Loads all the decoration meshes on the level.
--
-- Author: Nick Clark
-- Date C: 10-08-2007
-------------------------------------------------------------------------------

function DecorationMeshes( resources, decBarn )
	for i,meshDesc in ipairs( MeshNodeInstances ) do
		local animStart = 0
		if meshDesc.AnimStartMin and meshDesc.AnimStartMax then
			animStart = meshDesc.AnimStartMin + math.random() * (meshDesc.AnimStartMax - meshDesc.AnimStartMin)
		end

		local shaderName = "MeshTexture"
		if meshDesc.Shader then shaderName = meshDesc.Shader end
		--@HACK: We'll leave this in for a while, until all meshes are moved to the new shader.
		if "Mesh" == shaderName then
			print( "ERROR: Incorrect shader on: "..meshDesc.ObjectName )
		end

		local deco = decBarn:AddDecoration( resources, meshDesc.Mesh, shaderName, meshDesc.Transformation, animStart )

		if meshDesc.ShaderParams then
			for i,param in ipairs( meshDesc.ShaderParams ) do
				--@HACK: The fadeDists and chamRange controls should probably not be mashed
				-- in with the shader params.
				if "fadeDists" == param.ParamName then
					deco:SetFadeDist( param.ParamVal[1], param.ParamVal[2] )
				elseif "chamRange" == param.ParamName then
					deco:SetChamRange( param.ParamVal[1], param.ParamVal[2] )
				elseif "" ~= param.ParamName then
					if "texture" == param.ParamType then
						deco:SetShaderTextureParam( resources, param.ParamName, param.ParamVal, 100000 )
					elseif "float" == param.ParamType then
						deco:SetShaderParamf( param.ParamName, param.ParamVal, 100000 )
					elseif "float4" == param.ParamType then
						deco:SetShaderParam4f( param.ParamName, param.ParamVal, 100000 )
					end
				end
			end
		end

		if meshDesc.Additive then
			deco:SetBlend( DecorationBlend_Additive )
		else
			deco:SetBlend( DecorationBlend_Solid )
		end

		-- Add any sound triggers related to the mesh
		local sounds = SoundQueueInstances[ meshDesc.Mesh ]
		if sounds then
			for i,sound in ipairs( sounds ) do
				deco:SetSoundQueue( sound.animPos, sound.bankName, sound.soundName, sound.vol, sound.minDist, sound.fromController or false )
			end
		end

		deco:SetEnabled( true )

		Names[ meshDesc.ObjectName ] = deco
	end
	
	MeshNodeInstances = {}
end

SoundQueueInstances =
{

	Windmill =
	{
		{
			animPos = 0.0
		,   soundName = "WindmillWhsh_Sm"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 4.0
		}
	,	{
			animPos = 0.333333
		,   soundName = "WindmillWhsh_Sm"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 4.0
		}
		,	{
			animPos = 0.666667
		,   soundName = "WindmillWhsh_Sm"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 4.0
		}
	}
	
	,WindmillSmall =
	{
		{
			animPos = 0.5
		,   soundName = "WindmillWhsh_Sm"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 4.0
		}
	,	{
			animPos = 1.0
		,   soundName = "WindmillWhsh_Sm"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 4.0
		}
	}
	
	,WindmillLarge =
	{
		{
			animPos = 0.166667
		,   soundName = "WindmillWhoosh"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 8.0
		}
	,	{
			animPos = 0.5
		,   soundName = "WindmillWhoosh"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 8.0
		}
	,	{
			animPos = .8333333
		,   soundName = "WindmillWhoosh"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 8.0
		}
	}
	
	,Metal9a = 
	{
		{
			animPos = 0.01
		,	soundName = "BeamCollapse"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 20.0
		}
	,	{
			animPos = 0.325
		,	soundName = "BeamHit"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 20.0
		}
	}
	
	,Metal10 =
	{
		{
			animPos = 0.01
		,   soundName = "VineGrow"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 30.0
		}
	}
	
	,Highwayframe =
	{
		{
			animPos = 0.01
		,	soundName = "StreetSignGrow"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 30.0
		}
	}
	
	,Pole3 =
	{
		{
			animPos = 0.01
		,	soundName = "StreetSignGrow"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 30.0
		}
	}
	,LightBlink = 
	{
		{
			animPos = 0.13542
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 10.0
		--,	fromController = true
		}
		,{
			animPos = 0.19792
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 7.0
		--,	fromController = true
		}
		,{
			animPos = 0.34375
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 10.0
		--,	fromController = true
		}
		,{
			animPos = 0.51042
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 10.0
		--,	fromController = true
		}
		,{
			animPos = 0.76042
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 7.0
		--,	fromController = true
		}
		,{
			animPos = 0.91667
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 10.0
		--,	fromController = true
		}
		,{
			animPos = 0.96875
		,	soundName = "LightFlicker"
		,	bankName = "LvlSfx"
		,	vol = 1.0
		,	minDist = 7.0
		--,	fromController = true
		}
	}
}

