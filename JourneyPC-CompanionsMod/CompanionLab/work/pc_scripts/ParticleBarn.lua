-------------------------------------------------------------------------------
-- Notes:  Logic for loading particles
--
-- Author: Martin Middleton
-- Date C: 08-19-2010
-------------------------------------------------------------------------------
emitterTextureMap =
{
	cloud   = 0
,   wave    = 1
,	sphere  = 2
,   rubble  = 3
,   rubble2 = 4
,   square  = 5
,   egg     = 6
,	halo    = 7
,   spark   = 8
,   haloBig = 9
,   evil    = 10
,   poof    = 11
,   dust    = 12
,   trail   = 13
,	soft	= 14
,   blur    = 15
,	pat1	= 16
,   cloth   = 17
,	snow    = 18
,   star    = 19
,   sand    = 20
,   pixie   = 21
,   chunk   = 22
,   hollow  = 23
,	mist = 24
,   defaultRib = 25
,   paintedRib = 26
,   symbolRib  = 27
,   windRib    = 28
,   ripple  = 29

,   symbol_1_1 = 30
,   symbol_2_1 = 31
,   symbol_3_1 = 32
,   symbol_4_1 = 33
,   symbol_1_2 = 34
,   symbol_2_2 = 35
,   symbol_3_2 = 36
,   symbol_4_2 = 37
,   symbol_1_3 = 38
,   symbol_2_3 = 39
,   symbol_3_3 = 40
,   symbol_4_3 = 41
,   symbol_1_4 = 42
,   symbol_2_4 = 43
,   symbol_3_4 = 44
,   symbol_4_4 = 45
,   symbol_1_5 = 46
,   symbol_2_5 = 47
,   symbol_3_5 = 48
,   symbol_4_5 = 49
,   symbol_1_6 = 50
,   symbol_2_6 = 51
,   symbol_3_6 = 52
,   symbol_4_6 = 53
,   symbol_1_7 = 54
,   symbol_2_7 = 55
,   symbol_3_7 = 56
,   symbol_4_7 = 57
,   symbol_1_8 = 58
,   symbol_2_8 = 59
,   symbol_3_8 = 60
,   symbol_4_8 = 61

,   boon = 62
}

-- format is { lower left u, lower left v, upper right u, upper right v }
local textureUvs =
{
	cloud = { 0.125, 0.5, 0.1875, 0.75 }
,	wave = { 0.125, 0.25, 0.1875, 0.5 }
,   sphere = { 0.03125, 0.5, 0.0625, 0.625 }
,   rubble = { 0.03125, 0.75, 0.0625, 0.875 } 
,   rubble2 = { 0.03125, 0.625, 0.0625, 0.75 } 
,   square = { 0, 0.75, 0.03125, 0.875}
,	egg = { 0.125, 0, 0.1875, 0.25 }
,	halo = { 0.5, 0, 0.625, 0.5 }
,	spark = { 0, 0.625, 0.03125, 0.75 }
,	haloBig = { 0.5, 0.5, 0.625, 1 }
,	evil = { 0, 0.875, 0.03125, 1 }
,	poof = { 0.125, 0.75, 0.1875, 1 }
,	dust = { 0.375, 0.5, 0.5, 1 }
,	trail = { 0.25, 0.5, 0.375, 1 }
,	soft = { 0.25, 0, 0.375, 0.5 }
,   blur = { 0.1875, 0, 0.25, 0.25 }
,	pat1 = { 0.0625, 0, 0.125, 0.25 }
,	cloth = { 0.0625, 0.25, 0.125, 0.5 }
,	snow = { 0.0625, 0.5, 0.125, 0.75 }
,	star = { 0.0625, 0.75, 0.125, 1 }
,	sand = { 0.75, 0, 0.8125, 1 }
,	pixie = { 0.1875, 0.75, 0.25, 1 }
,   chunk = { 0, 0, 0.03125 * 2, 0.125 }
,   hollow = { 0.1875, 0.5, 0.25, 0.75 }
,	mist = { 0.1875, 0.25, 0.25, 0.5 }
,   defaultRib = { 0.71875, 0, 0.75, 1 }
,   paintedRib = { 0.625, 0, 0.65625, 1 }
,   symbolRib  = { 0.65625, 0, 0.6875, 1 }
,   windRib    = { 0.6875, 0, 0.71875, 1 }
,   ripple = { 0, 0.5, 0.03125, 0.625 }
,   boon = { 0, 0.25, 0.03125, 0.375 }

,   symbol_1_1 = { 0.878,   0,     0.90625, 0.125 }
,   symbol_2_1 = { 0.90625, 0,     0.9375,  0.125 }
,   symbol_3_1 = { 0.9375,  0,     0.96875, 0.125 }
,   symbol_4_1 = { 0.96875, 0,     1,       0.125 }
,   symbol_1_2 = { 0.878,   0.125, 0.90625, 0.25 }
,   symbol_2_2 = { 0.90625, 0.125, 0.9375,  0.25 }
,   symbol_3_2 = { 0.9375,  0.125, 0.96875, 0.25 }
,   symbol_4_2 = { 0.96875, 0.125, 1,       0.25 }
,   symbol_1_3 = { 0.878,   0.25,  0.90625, 0.375 }
,   symbol_2_3 = { 0.90625, 0.25,  0.9375,  0.375 }
,   symbol_3_3 = { 0.9375,  0.25,  0.96875, 0.375 }
,   symbol_4_3 = { 0.96875, 0.25,  1,       0.375 }
,   symbol_1_4 = { 0.878,   0.375, 0.90625, 0.5 }
,   symbol_2_4 = { 0.90625, 0.375, 0.9375,  0.5 }
,   symbol_3_4 = { 0.9375,  0.375, 0.96875, 0.5 }
,   symbol_4_4 = { 0.96875, 0.375, 1,       0.5 }
,   symbol_1_5 = { 0.878,   0.5,   0.90625, 0.625 }
,   symbol_2_5 = { 0.90625, 0.5,   0.9375,  0.625 }
,   symbol_3_5 = { 0.9375,  0.5,   0.96875, 0.625 }
,   symbol_4_5 = { 0.96875, 0.5,   1,       0.625 }
,   symbol_1_6 = { 0.878,   0.625, 0.90625, 0.75 }
,   symbol_2_6 = { 0.90625, 0.625, 0.9375,  0.75 }
,   symbol_3_6 = { 0.9375,  0.625, 0.96875, 0.75 }
,   symbol_4_6 = { 0.96875, 0.625, 1,       0.75 }
,   symbol_1_7 = { 0.878,   0.75,  0.90625, 0.875 }
,   symbol_2_7 = { 0.90625, 0.75,  0.9375,  0.875 }
,   symbol_3_7 = { 0.9375,  0.75,  0.96875, 0.875 }
,   symbol_4_7 = { 0.96875, 0.75,  1,       0.875 }
,   symbol_1_8 = { 0.878,   0.875, 0.90625, 1 }
,   symbol_2_8 = { 0.90625, 0.875, 0.9375,  1 }
,   symbol_3_8 = { 0.9375,  0.875, 0.96875, 1 }
,   symbol_4_8 = { 0.96875, 0.875, 1,       1 }
}

local shaderMap =
{
	Default  = kDefaultParticleShader
,	Blended = kBlendedParticleShader
,	Distortion = kDistortionParticleShader
,	LowRes = kLowResParticleShader
,	Additive = kAdditiveParticleShader
,	Ribbon = kRibbonParticleShader
}

local typeOf = type

function ApplyEmitterTypeChange( jsonArgs )
	local changes = Json.Decode( jsonArgs )
	local typeName = changes.typeName
	local varName = changes.varName
	local value = changes.value
	print( "Setting "..typeName.."."..varName.." = " )
	if ( "table" == typeOf( value ) ) then PrintTable( value ) else print( value ) end
	emitterData[ typeName ][ varName ] = value
	LoadEmitterTypes( game:particleBarn() )
	game:particleBarn():RefreshCachedEmitters()
end

function CloneEmitterType( typeName, cloneName )
	if( emitterData[ cloneName ] ~= nil ) then 
		print( "AddNewEmitterType(): Emitter type with name "..cloneName.." already exists" )
		return false
	end
	if( emitterData[ typeName ] == nil ) then
		print( "AddNewEmitterType(): Can't find emitter type with name "..typeName )
		return false
	end
	local cloneType = {}
	for varName,varData in pairs( emitterData[ typeName ] ) do
		if( typeOf( varData ) == "table" ) then
			local cloneData = {}
			for i,val in ipairs( varData ) do
				cloneData[ i ] = varData[ i ]
			end
			cloneType[ varName ] = cloneData
		else
			cloneType[ varName ] = varData
		end
	end
	emitterData[ cloneName ] = cloneType
	return true
end

function ChangeEmitterName( oldName, nextName )
	if( oldName == nextName ) then return end
	emitterData[ nextName ] = emitterData[ oldName ]
	emitterData[ oldName ] = nil
end

-- Init functions 
function InitParticleRender( particleRender )
	local sortedTextureMap = {}
	for texName,texVal in pairs( emitterTextureMap ) do sortedTextureMap[ texVal + 1 ] = texName end

	particleRender:ResetTexUvs()
	for i,texName in ipairs( sortedTextureMap ) do
		local texUv = textureUvs[ texName ]
		particleRender:AddTexUv( texUv[ 1 ], texUv[ 2 ], texUv[ 3 ], texUv[ 4 ] ) 
	end
end

function LoadEmitterType( particleBarn, emitter )
	local typeName = emitter:GetName()
	local type = emitterData[ typeName ]
	if( type == nil ) then 
		print( "ParticleDefs: Can't find type "..typeName )
		return
	end

	-- Optional fields
	local scaleEnd = { 0,0 }
	if ( type.scaleEnd ) then
		scaleEnd[ 1 ] = type.scaleEnd[ 1 ]
		scaleEnd[ 2 ] = type.scaleEnd[ 2 ]
	else
		type.scaleEnd = { 0, 0 }
	end

	if ( type.shaderType ) then emitter:shaderType( shaderMap[ type.shaderType ] )
	else emitter:shaderType( 0 ) end

	local blueprint = emitter:GetBlueprint()
	blueprint:scale( type.scale )
	blueprint:scaleEnd( scaleEnd )
	blueprint:lifeTime( type.lifeTime )
	blueprint:spin( type.spin )
	blueprint:decay( type.decay )
	blueprint:forceGravity( type.forceGravity )
	blueprint:forceWind( type.forceWind )
	blueprint:forceNoise( type.forceNoise )
	blueprint:forceAttract( type.forceAttract )
	blueprint:forceVortex( 0.0 )
	blueprint:forceCentripetal( 0.0 )
	blueprint:forceFluid( 0.0 )
	blueprint:intensity( 1.0 )
	if ( type.forceVortex ) then blueprint:forceVortex( type.forceVortex ) 
	else type.forceVortex = 0.0 end
	if ( type.forceCentripetal ) then blueprint:forceCentripetal( type.forceCentripetal )
	else type.forceCentripetal = 0.0 end
	if ( type.forceFluid ) then blueprint:forceFluid( type.forceFluid )
	else type.forceFluid = 0.0 end
	if type.intensity then blueprint:intensity( type.intensity )
	else type.intensity = 1.0 end
	blueprint:shadow( 0.0 )

	blueprint:texture( emitterTextureMap[ type.texture ] )
	blueprint:extraTextures( type.extraTextures or 0 )
	local linearStart = GammaToLinear255Vec4( { type.colorStart[ 1 ], type.colorStart[ 2 ], type.colorStart[ 3 ], type.colorStart[ 4 ] or 255 } )
	blueprint:SetColorStart( linearStart[ 1 ], linearStart[ 2 ], linearStart[ 3 ], linearStart[ 4 ] )	
	local linearEnd = GammaToLinear255Vec4( { type.colorEnd[ 1 ], type.colorEnd[ 2 ], type.colorEnd[ 3 ], type.colorEnd[ 4 ] or 255 } )
	blueprint:SetColorEnd( linearEnd[ 1 ], linearEnd[ 2 ], linearEnd[ 3 ], linearEnd[ 4 ] )	

	-- Set emitter info
	emitter:spawnRate( type.spawnRate )
	emitter:speed( type.speed )
	emitter:radius( type.radius )
	emitter:spreadMin( type.spreadMin )
	emitter:spreadMax( type.spreadMax )
	
	if type.ribStartLengthMult then emitter:ribStartLengthMult( type.ribStartLengthMult ) 
	else type.ribStartLengthMult = 1 end
	
	if ( type.ribbonSegCount ) then emitter:ribbonSegCount( { type.ribbonSegCount[ 1 ], type.ribbonSegCount[ 2 ] } )
	else emitter:ribbonSegCount( { 12, 12 } ) type.ribbonSegCount = { 12, 12 } end

	if ( type.clampedRibSegLength ) then emitter:clampedRibSegLength( { type.clampedRibSegLength[ 1 ], type.clampedRibSegLength[ 2 ] } )
	else emitter:clampedRibSegLength( { 0, 100 } ) type.clampedRibSegLength = { 0, 100 } end
	
	if ( typeOf( type.panRadius ) == "table" ) then emitter:panRadius( type.panRadius )
	else emitter:panRadius( { 0, type.panRadius } ) end

	if( type.relativeToEmitter ) then blueprint:relativeToEmitter( true )
	else blueprint:relativeToEmitter( false ) type.relativeToEmitter = false end

	if( type.billboardDirection ) then emitter:bbDir( type.billboardDirection )
	else emitter:bbDir( { 0, 0, 0 } ) type.billboardDirection = { 0, 0, 0 } end

	if( type.spawnOnGround ) then emitter:spawnOnGround( true )
	else emitter:spawnOnGround( false ) type.spawnOnGround = false end

	if( type.useGroundColor ) then emitter:useGroundColor( true )
	else emitter:useGroundColor( false ) type.useGroundColor = false end

	if( type.useGroundColorPP ) then emitter:useGroundColorPP( true )
	else emitter:useGroundColorPP( false ) type.useGroundColorPP = false  end

	if( type.heightAboveGround ) then emitter:heightAboveGround( type.heightAboveGround )
	else emitter:heightAboveGround( 0.0 ) type.heightAboveGround = 0.0 end

	if( type.scaleInTime ) then blueprint:scaleInTime( type.scaleInTime )
	else blueprint:scaleInTime( 0.0 ) type.scaleInTime = 0.0 end

	if( type.fadeInTime ) then blueprint:fadeInTime( type.fadeInTime )
	else blueprint:fadeInTime( 0.25 ) type.fadeInTime = 0.25 end

	if( type.maxSizeOnScreenBeginFadeout ) then
		emitter:maxSizeOnScreenBeginFadeout( type.maxSizeOnScreenBeginFadeout )
		emitter:maxSizeOnScreenEndFadeout( type.maxSizeOnScreenEndFadeout )
	else
		emitter:maxSizeOnScreenBeginFadeout( 0.75 )
		emitter:maxSizeOnScreenEndFadeout( 1.00 )
	end

	if( type.motionBlurMult ) then emitter:motionBlurMult( type.motionBlurMult )
	else emitter:motionBlurMult( 0.0 ) end
	if( type.shadowMult ) then emitter:shadowMult( type.shadowMult )
	else emitter:shadowMult( 0.0 ) end

	if( type.minDist ) then blueprint:minDist( type.minDist ) 
	else type.minDist = 1000 end
	if( type.maxDist ) then blueprint:maxDist( type.maxDist ) 
	else type.maxDist = 2000 end

	if( type.useLighting ) then emitter:useLighting( type.useLighting )
	else emitter:useLighting( false ) end
end

function LoadEmitterTypes( particleBarn )
	local numEmitters = particleBarn:GetNumEmitters()
	for i = 0, numEmitters - 1 do
		local emitter = particleBarn:GetEmitter( i )
		LoadEmitterType( particleBarn, emitter )		
	end
end

function PrepareEmitterData()

end

function SaveEmitterData()
	PrepareEmitterData()
	local outFile = io.open("/app_home/Data/Scripts/TempParticleDefs.lua", "w+")
	outFile:write( "emitterData = " )
	PrintTable( outFile, emitterData )
	outFile:close()
	
	LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"p4 edit Data\\Scripts\\ParticleDefs.lua\"" )
	Sleep( 1000 )
	LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"move /Y Data\\Scripts\\TempParticleDefs.lua Data\\Scripts\\ParticleDefs.lua\"" )
	Sleep( 1000 )
	LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"del Data\\Scripts\\TempParticleDefs.lua 2>nul\"" )
	return true
end
