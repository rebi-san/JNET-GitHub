-------------------------------------------------------------------------------

-- Notes:  Contains all script functions for rendering the main scene.

--

-- RenderInitialize()

--     Called once at startup.  You should allocate all render targets,

--     textures and rendering passes in here.

-- 

-- RenderInitializeContext()

--     Called once towards the beginning of every frame.  Lets you set up a

--     RenderContext which is passed to all objects that want to be rendered.

--

-- RenderUpdate()

--     Called once a frame right before actual rendering.  Lets you set up

--     the shader parameters of all the passes.  It's important to set shader

--     params in this function and not InitRender().

--

-- Author: Rick Nelson and John Edwards

-- Date C: 6-1-2007

-------------------------------------------------------------------------------



-------------------------------------------------------------------------------

-- WISHLIST

--

--   Cleaner integration with rendering of rest of scene (putting grass blurriness in alpha channel, etc)

--   Better support for multiple resolutions

--   Automatic updating of certain parameters (texture inputs of render targets, etc)

-------------------------------------------------------------------------------



-------------------------------------------------------------------------------

-- Debug turn on and off

-- enabled low res particles

local bEnableLowParticles = true



-- enable DOF... requires bEnableLowParticles

local bEnableDOF = true



-- enable motion blur... requires bEnableDOF

local bEnableMotionBlur = true



-- enable macro tile sync - PSP2 only

local bEnableMacroTile = false



--- set to true to resolve MSAA before doing transparencies

local bResolveMSAA = false



--- set to allow stereo 3D

local bEnableS3D = false

--- no need to perform two-step downsample (single step with 4x samples might be cheaper).
local bEnableTwoStepDownSample = true

--- luminance delta stored in alpha channel
local bHalfSizedPostProcessing = true


local kFenceTerrain = 32
local kFenceDepthBuffer = 33
local kFenceOutputAdditive = 34
local kFenceRipple = 35
local kFenceDofComposite = 36
local kFenceDoFBlurStep = 37
local kFenceDoFGlareStep = 38
local kFenceDoFDownSampleStep = 39


local bRemoveMSAA = true

-------------------------------------------------------------------------------

local ResX, ResY = 1024, 768

local blurScaleX, blurScaleY = 1, 1		-- scale for the blur samples - defaults to 1.0

local msaaType = PE_TEXTURE_MSAA_NONE    -- So we can quickly try out different AA types.

local	topV, bottomV = 0,1



-------------------------------------------------------------------------------

-- platform specific specifications

if TARGET=="PS3" then



	--ResX, ResY = 1280, 720

	--ResX, ResY = 1440, 810

	--ResX, ResY = 1536, 864 -- This is supposed to play nicely with the RSX.

	--ResX, ResY = 1680, 945

	--ResX, ResY = 1920, 1080

	ResX = 1440

	ResY = 810

	

	--msaaType = PE_TEXTURE_MSAA_X4ROT

	--msaaType = PE_TEXTURE_MSAA_X2

	msaaType = PE_TEXTURE_MSAA_NONE

	

elseif TARGET=="PSP2" then



	--ResX, ResY = 960, 544

	-- MB: lower res render buffer per Daryl

	--ResX, ResY = 640, 512

	--ResX, ResY = 800, 416

	ResX, ResY = 768, 544

	

	--bResolveMSAA = true

	msaaType = PE_TEXTURE_MSAA_X4ROT

	--msaaType = PE_TEXTURE_MSAA_X2

	--msaaType = PE_TEXTURE_MSAA_NONE	

	

	bEnableMacroTile = true

	

	-- flip postFX uv on psp2

	topV, bottomV = 1, 0

	

elseif TARGET=="PS4" then



	-- 1080P

	ResX, ResY = 1920, 1080

	msaaType = PE_TEXTURE_MSAA_X4ROT



	-- scale for the blur kernel

	-- higher res, so divide our res by the PS3 res, so we sample the same # of pixels

	blurScaleX, blurScaleY = ResX / 1440.0, ResY / 810.0



	-- flip postFX uv on PS4

	topV, bottomV = 1, 0



	-- disablemotion blur on PS4

	--bEnableMotionBlur = false



	-- due to large res, we resolve msaa before doing transparencies

	bResolveMSAA = true



	-- allow stereoscopic 3D

	bEnableS3D = true



elseif TARGET=="IOS64" then



    -- 1080P

    ResX, ResY = 1920, 1080

    msaaType = PE_TEXTURE_MSAA_NONE



    -- scale for the blur kernel

    -- higher res, so divide our res by the PS3 res, so we sample the same # of pixels

    blurScaleX, blurScaleY = ResX / 1440.0, ResY / 810.0



    -- flip postFX uv on PS4

    topV, bottomV = 1, 0



    -- disablemotion blur on PS4

    --bEnableMotionBlur = false



    -- due to large res, we resolve msaa before doing transparencies

    bResolveMSAA = true



    -- allow stereoscopic 3D

    bEnableS3D = false

    bEnableLowParticles = false



elseif TARGET=="WIN64" then

    -- 1080P
    ResX, ResY = 1920, 1080
    msaaType = PE_TEXTURE_MSAA_NONE

    -- scale for the blur kernel
    -- higher res, so divide our res by the PS3 res, so we sample the same # of pixels
	blurScaleX, blurScaleY = ResX / 1440.0, ResY / 810.0

    -- flip postFX uv on PS4
    topV, bottomV = 1, 0
    -- disablemotion blur on PS4
    --bEnableMotionBlur = false

    -- due to large res, we resolve msaa before doing transparencies
    bResolveMSAA = true

    -- allow stereoscopic 3D
    bEnableS3D = false
    bEnableLowParticles = false

else

	error("Unspecified TARGET platform")

end



local OnePixX, OnePixY = 1/ResX, 1/ResY



---------------------------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------------------------

-- Dictionary<string,Render::Pass*> passes

-- render passes defined by this file

local passes = {}



-- Dictionary<string,PTexture*> renderTargets

-- renderTargets defined by this file

local renderTargets = {}



-- Dictionary<string,PTexture*> textures

-- textures used (render targets used as textures should be repeated here)

local textures = {}



-- Dictionary<string,Texture*>	images

-- non-RT textures

local images = {}



-- Dictionary<string,float[4]>	renderTargetRegions

-- regions of render targets

-- optional, used for Render::QuadPass::SetPos

local renderTargetRegions = {}



-- Dictionary<string,bool> glareInit

-- Initialization flags for glare textures

local glareInit = {}





---------------------------------------------------------------------------------------------------------

--	Create a region within a render target
---------------------------------------------------------------------------------------------------------
local function CreateRegion( rect, rtSize )
	local	ret = {}
	local	widthMul = 2.0/rtSize[1]
	local	heightMul = 2.0/rtSize[2]

	-- [1] is left side (-1)
	-- [2] is BOTTOM (-1)
	-- [3] is right side (1)
	-- [4] is TOP (1)
	ret[1] = rect[1]*widthMul - 1.0
	ret[2] = 1.0 - (rect[2] + rect[4])*heightMul
	ret[3] = (rect[1] + rect[3])*widthMul - 1.0
	ret[4] = 1.0 - rect[2]*heightMul 
	return ret
end



local function CreateMacroRT( ren, rt, name, rect, rtSize )
	renderTargets[name] = rt
	renderTargetRegions[name] = CreateRegion(rect, rtSize)
	textures[name] = ren:CreateSubTexture(rt,rect[1],rect[2],rect[3],rect[4])
end



---------------------------------------------------------------------------------------------------------
--	Setup a quad pass
--		Sets the color target to renderTargets[rtName]
--		if renderTargetRegions[rtName] is not null
--			Sets to render to the given portion
--		else
--			Renders to the entire RT
--		Sets tex0 to the texture with the given name
--		Sets to read from the whole texture
---------------------------------------------------------------------------------------------------------

local function SetupQuadPass( pass, rtName, texName )
	local rt = renderTargets[rtName]
	local region = renderTargetRegions[rtName]

	pass:colorTarget( rt )
    pass:SetAction(ActionIgnore, ActionIgnore, ActionStore, ActionIgnore)

	-- the following ensures that a blit of a texture onto a same size render target has no filtering
	-- and a blit of a texture onto a 2x render target properly bilinears
	local subPixX = 0 --  -1.0/rt:getWidth()
	local subPixY = 0 --  -1.0/rt:getHeight()

	if region~=nil then
		pass:SetPos( region[1] + subPixX, region[2] + subPixY, region[3] + subPixX, region[4] + subPixY )
	else
		pass:SetPos( -1.0 + subPixX, -1.0 + subPixY, 1.0 + subPixX, 1.0 + subPixY )
	end

	pass:SetClear(true)
	pass:GetShader():SetTextureParam( "tex0", textures[texName] )
	pass:SetUv( 0.0, topV, 1.0, bottomV )  -- Read the whole texture
end



---------------------------------------------------------------------------------------------------------
--	Setup main render passes
--		Early geometry
--		Geometry
--		Late geometry
--		Additive geometry
--		Additive NoZ geometry
---------------------------------------------------------------------------------------------------------

local function SetupOpaqueRender( ren, color, depth, colorLow, depthLow )
-- Render the scene to an MSAA buffer
	local pass
	pass = ren:AddGeometryPass("Geometry")
    pass:AddFenceWait(FenceTypeVertex, kFenceRipple)
	pass:SetClear( true )
	pass:colorTarget( color )
	pass:depthTarget( depth )
    pass:AddFenceWait(FenceTypeFragment, kFenceTerrain)
    pass:AddFenceInsert(FenceTypeVertex, kFenceRipple)

-- resolve (ie downsample) to low-res textures while at it
pass:SetAction(ActionClear, ActionClear, ActionStore, ActionStore)
--pass:resolveColorTarget( colorLow )
--pass:resolveDepthTarget( depthLow )

	passes.Geometry = pass

	pass = ren:AddGeometryPass("EarlyGeometry")
	pass:colorTarget( color )
	pass:depthTarget( depth )
	passes.EarlyGeometry = pass

	pass = ren:AddGeometryPass("LateGeometry")
	pass:colorTarget( color )
	pass:depthTarget( depth )
	passes.LateGeometry = pass

-- Discard geometry
	pass = ren:AddGeometryPass("DiscardGeometry")
	pass:colorTarget( color )
	pass:depthTarget( depth )
	passes.Discard = pass
end



local function SetupTransparentRender( ren, color, depth )
	local pass

-- Blended some extra geometry to the MSAA buffer
	pass = ren:AddGeometryPass("BlendedGeometry")
	pass:SetMSAAResolve( bResolveMSAA )		-- remove msaa
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE_MINUS_SRC_ALPHA )
	pass:colorTarget( color )
	pass:depthTarget( depth )
	passes.BlendGeom = pass

-- Additively blend some extra geometry to the MSAA buffer
	pass = ren:AddGeometryPass("AdditiveGeometry")
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE )
	pass:SetDepthWrite( false )
	pass:colorTarget( color )
	pass:depthTarget( depth )
--pass:AddFenceInsert(FenceTypeFragment, kFenceDepthBuffer)
	passes.AdditiveGeom = pass
end



local function SetupAdditiveNoZ( ren, color )
	local pass
	-- With some additive stuff we want to handle depth testing ourselves
	-- pass nil as depth target
	pass = ren:AddGeometryPass("AdditiveNoZ")
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE )
	pass:SetDepthWrite( false )
	pass:SetDepthRead( false )
	pass:colorTarget( color )
	pass:depthTarget( nil )
    pass:SetPriority(-1500)
pass:AddFenceInsert(FenceTypeFragment, kFenceOutputAdditive)
	passes.AdditiveNoZGeom = pass
end



local function SetupRemoveMSAA( ren, resources )
-- Shrink the depth buffer for particle rendering.
	local pass
	pass = ren:AddQuadPass( resources, "RemoveMSAA" )
	pass:SetClear( true )
	pass:colorTarget( renderTargets.fullColor )
	pass:depthTarget( renderTargets.fullDepth )
	pass:SetPos( -1.0, -1.0, 1.0, 1.0 )
	pass:SetUv( 0.0, topV, 1.0, bottomV )
	pass:GetShader():SetTextureParam( "texColor", textures.MsaaColor ) 
	pass:GetShader():SetTextureParam( "texDepth", textures.MsaaDepth ) 
	pass:GetShader():SetParam4f( "pixOffset", { OnePixX, OnePixY, -OnePixX, -OnePixY } ) 
	passes.RemoveMSAA = pass
end


---------------------------------------------------------------------------------------------------------
--	Setup particles for low res rendering
--		DepthDownsample
--		Particles
--		ParticlesNoZ
--		SubParticles
--		DepthStore
---------------------------------------------------------------------------------------------------------
local function SetupParticlesLow( ren, resources, colorLow, depthLow, colorHi )
-- Shrink the depth buffer for particle rendering.
	local pass
	pass = ren:AddQuadPass( resources, "DownSampleDepth" )
	pass:SetClear( true )
	pass:colorTarget( colorLow )
	pass:depthTarget( depthLow )
	pass:SetPos( -1.0, -1.0, 1.0, 1.0 )
	pass:SetUv( 0.0, topV, 1.0, bottomV )
    pass:SetAction(ActionIgnore, ActionIgnore, ActionStore, ActionStore)
	pass:GetShader():SetParam4f( "pixOffset", { OnePixX, OnePixY, -OnePixX, -OnePixY } )
    pass:SetPriority( -1000 )
--pass:AddFenceWait(FenceTypeFragment, kFenceDepthBuffer)
	passes.DepthDownsample = pass

-- Render particles to a small buffer to save fill rate.
	pass = ren:AddGeometryPass("Particles")
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE )
	pass:SetDepthWrite( false )
	pass:colorTarget( colorLow )
	pass:depthTarget( depthLow )
    pass:SetPriority( -1001 )
	passes.Particles = pass

-- Render non depth tested particles to a small buffer to save fill rate.
	pass = ren:AddGeometryPass("ParticleNoZ")
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE )
	pass:SetDepthWrite( false )
	pass:SetDepthRead( false )
	pass:colorTarget( colorLow )
	pass:depthTarget( depthLow )
    pass:SetPriority( -1002 )
	passes.ParticlesNoZ = pass

-- Subtractively blend some extra geometry to the MSAA buffer
	pass = ren:AddGeometryPass("SubParticles")
	pass:SetBlend( true, PE_SHADER_BLEND_ZERO, PE_SHADER_BLEND_ONE_MINUS_SRC_COLOR )
	pass:SetDepthWrite( false )
	pass:colorTarget( colorLow )
	pass:depthTarget( depthLow )
    pass:SetPriority( -1003 )

pass:AddFenceInsert(FenceTypeFragment, 39)
	passes.SubParticles = pass

-- To be used by the DoF pass, later.  Encodes depth-based blurriness in the
-- alpha channel of the target.  This should happen in the geometry shaders.
	pass = ren:AddQuadPass( resources, "DepthBlurStoreSet" )
	pass:colorTarget( colorHi )
	pass:SetPos( -1.0, -1.0, 1.0, 1.0 )
	pass:SetUv( 0.0, topV, 1.0, bottomV )
    pass:SetPriority( -1004 )
pass:AddFenceWait(FenceTypeFragment, 39)
    pass:SetAction(ActionIgnore, ActionIgnore, ActionStore, ActionIgnore)
	passes.DepthStore = pass
end



---------------------------------------------------------------------------------------------------------
--	Scaled depth buffer for S3D
---------------------------------------------------------------------------------------------------------
local function SetupS3DDepth( ren, resources, srcDepth, dstColor, dstDepth )
	if not srcDepth then
		srcDepth = renderTargets.fullDepth
	end

	local pass, shader
	local sw, sh = srcDepth:getWidth(), srcDepth:getHeight()
	local dw, dh = dstDepth:getWidth(), dstDepth:getHeight()

	-- This needs to scale based upon the ratio of src and dst sizes
	local offsx, offsy = 2*dw/(sw*sw), 2*dh/(sh*sh)

	pass = ren:AddQuadPass( resources, "DownSampleDepth" )
	pass:SetClear( true )
	pass:colorTarget( dstColor )
	pass:depthTarget( dstDepth )
	pass:SetPos( -1.0, -1.0, 1.0, 1.0 )
	pass:SetUv( 0.0, topV, 1.0, bottomV )
	pass:fieldMask(1)				-- only left field
	passes.S3DDepth = pass
	shader = pass:GetShader()
	shader:SetTextureParam( "texDepth", srcDepth )
	shader:SetParam4f( "pixOffset", { offsx, offsy, -offsx, -offsy } ) 
end


local function UpdateS3DDepth()
	local pass, shader
	local swap = renderTargets.S3DDepth1
	renderTargets.S3DDepth1 = renderTargets.S3DDepth0
	renderTargets.S3DDepth0 = swap
	pass = passes.S3DDepth
	shader = pass:GetShader()
	pass:depthTarget(swap)
end



---------------------------------------------------------------------------------------------------------
--	Setup particles for high res rendering
--		Particles
--		ParticlesNoZ
--		SubParticles
---------------------------------------------------------------------------------------------------------
local function SetupParticlesHigh( ren, color, depth )
	local pass

-- Render particles to a small buffer to save fill rate.
	pass = ren:AddGeometryPass("Particles")
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE )
	pass:SetDepthWrite( false )
	pass:colorTarget( color )
	pass:depthTarget( depth )
	passes.Particles = pass

-- Render non depth tested particles to a small buffer to save fill rate.
	pass = ren:AddGeometryPass("ParticlesNoZ")
	pass:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE )
	pass:SetDepthWrite( false )
	pass:SetDepthRead( false )
	pass:colorTarget( color )
	--pass:depthTarget( depth )
	passes.ParticlesNoZ = pass

-- Subtractively blend some extra geometry to the MSAA buffer
	pass = ren:AddGeometryPass("SubParticles")
	pass:SetBlend( true, PE_SHADER_BLEND_ZERO, PE_SHADER_BLEND_ONE_MINUS_SRC_COLOR )
	pass:SetDepthWrite( false )
	pass:colorTarget( color )
	pass:depthTarget( depth )
	passes.SubParticles = pass
end



---------------------------------------------------------------------------------------------------------
--	SetupDOF
--	Requires SetupParticlesLow

--		DownSamp1

--		DownSamp2

--		BlurHoriz

--		BlurVert

--		BloomGlareStore

--		GlareBlurHoriz

--		GlareBlurVert

--		GlareComposite

--		DOF Composite

---------------------------------------------------------------------------------------------------------

local function SetupDOF( ren, resources, outColor )

	local pass

	local shader



   	-- This bias controls the width of the Gaussian blur filter. We can keep it around for 

   	--  experimenting (especially with depth of field)

	local bias, glareBias = 1.6, 8.0

	local biasX, biasY = bias * blurScaleX, bias * blurScaleY

	local glareX, glareY = glareBias * blurScaleX, glareBias * blurScaleY

    local d2target = "DOFBlur0"

    if bEnableTwoStepDownSample then
        d2target = "DOFHalfColor"
    end

	

	-- if bEnableMacroTile, the following is one scene

	-- blur the framebuffer

	-- Downsample Everything	 

	pass = ren:AddQuadPass( resources, "DownSampleSet" )
	SetupQuadPass(pass, d2target,"DOFSource")
    pass:SetPriority( -2000 )
    pass:AddFenceWait(FenceTypeFragment, kFenceOutputAdditive)
    pass:AddFenceInsert(FenceTypeFragment, kFenceDoFDownSampleStep)

	passes.DownSamp1 = pass

    if bEnableTwoStepDownSample then

	    pass = ren:AddQuadPass( resources, "DownSampleSet" )

	    SetupQuadPass(pass,"DOFBlur0","DOFHalfColor")
        pass:AddFenceWait(FenceTypeFragment, kFenceDoFDownSampleStep)
        pass:AddFenceInsert(FenceTypeFragment, kFenceDoFDownSampleStep)
        pass:SetPriority( -2001 )

	    passes.DownSamp2 = pass
    end



	-- Two pass blur

	pass = ren:AddQuadPass( resources, "BlurSet" )

	SetupQuadPass(pass,"DOFBlur1","DOFBlur0")

   	shader = pass:GetShader()

	shader:SetParam4f("xOffsets", {OnePixX*biasX, OnePixX*2*biasX, OnePixX*3*biasX, OnePixX*4*biasX})

	shader:SetParam4f("yOffsets", {0.0, 0.0, 0.0, 0.0})
pass:AddFenceWait(FenceTypeFragment, kFenceDoFDownSampleStep)
pass:AddFenceInsert(FenceTypeFragment, kFenceDoFBlurStep)
    pass:SetPriority( -2002 )

	passes.BlurHoriz = pass



	pass = ren:AddQuadPass( resources, "BlurSet" )

	SetupQuadPass(pass,"DOFBlur2","DOFBlur1")

    shader = pass:GetShader()

	shader:SetParam4f("xOffsets", {0.0, 0.0, 0.0, 0.0})

	shader:SetParam4f("yOffsets", {OnePixY*biasY, OnePixY*2*biasY, OnePixY*3*biasY, OnePixY*4*biasY})
pass:AddFenceWait(FenceTypeFragment, kFenceDoFBlurStep)
pass:AddFenceInsert(FenceTypeFragment, kFenceDoFBlurStep)
    pass:SetPriority( -2003 )

	passes.BlurVert = pass



	-- bloom image

	-- Store off the GlareTex

	pass = ren:AddQuadPass( resources, "GlareStoreSet" )

	SetupQuadPass(pass,"DOFGlare0","DOFGlareSource")
pass:AddFenceWait(FenceTypeFragment, kFenceDoFDownSampleStep)
pass:AddFenceInsert(FenceTypeFragment, kFenceDoFGlareStep)
    pass:SetPriority( -2004 )
	pass:SetClear(true)

	passes.BloomGlareStore = pass



	-- Blur the GlareTex

	pass = ren:AddQuadPass( resources, "BlurSet" )

	SetupQuadPass(pass,"DOFGlare1","DOFGlare0")

	shader = pass:GetShader()

	shader:SetParam4f( "xOffsets", {OnePixX*glareX, OnePixX*2*glareX, OnePixX*3*glareX, OnePixX*4*glareX} )

	shader:SetParam4f( "yOffsets", {0.0, 0.0, 0.0, 0.0} )
pass:AddFenceWait(FenceTypeFragment, kFenceDoFGlareStep)
pass:AddFenceInsert(FenceTypeFragment, kFenceDoFGlareStep)
    pass:SetPriority( -2005 )
	pass:SetClear(true)

	passes.GlareBlurHoriz = pass



	pass = ren:AddQuadPass( resources, "BlurSet" )

	SetupQuadPass(pass,"DOFGlare2","DOFGlare1")

	shader = pass:GetShader()

	shader:SetParam4f( "xOffsets", {0.0, 0.0, 0.0, 0.0} )

	shader:SetParam4f( "yOffsets", {OnePixY*glareY, OnePixY*2*glareY, OnePixY*3*glareY, OnePixY*4*glareY} )
pass:AddFenceWait(FenceTypeFragment, kFenceDoFGlareStep)
pass:AddFenceInsert(FenceTypeFragment, kFenceDoFGlareStep)
    pass:SetPriority( -2006 )
	pass:SetClear(true)

	passes.GlareBlurVert = pass

	

	-- Combine this frame's glare with an accumulation of the previous frames'.

	-- Note this is a new, non-macrotile, scene

	pass = ren:AddQuadPass( resources, "GlareComposite" )
	pass:SetClear(true)

	SetupQuadPass(pass,"DOFGlare3","DOFGlare2")

	shader = pass:GetShader()

	shader:SetTextureParam("texGlare", textures.DOFGlare2)

	shader:SetParamf( "glareInt", 0.3 )

	shader:SetParamf( "accumInt", 0.7 )

	shader:SetParam4f( "pixelSize", { OnePixX, OnePixY, 0, 0 } )
pass:AddFenceWait(FenceTypeFragment, kFenceDoFGlareStep)
pass:AddFenceInsert(FenceTypeFragment, kFenceDoFGlareStep)
    pass:SetPriority( -2007 )

	passes.GlareComposite = pass

	

	if bEnableMacroTile then

		pass = ren:AddQuadPass( resources, "DownSampleSet" )

		SetupQuadPass(pass, "DOFGlareAccum","DOFGlare3")

		passes.DOFGlareAccum = pass

	end



	-- Final Bloom Composite

	-- add glare tex back into the Bloom Composite Set

    pass = ren:AddQuadPass( resources, (bHalfSizedPostProcessing and "DofBloomCompPerf" or "DofBloomComp") )
    pass:SetAction(ActionClear, ActionIgnore, ActionStore, ActionIgnore)
    pass:SetBlend( true, PE_SHADER_BLEND_ONE, PE_SHADER_BLEND_ONE )
	pass:SetClear(true)

	pass:colorTarget( outColor )

	pass:SetPos( -1.0, -1.0, 1.0, 1.0 )

	pass:SetUv(  0.0, topV, 1.0, bottomV  )
    pass:SetPriority( -2100 )

pass:AddFenceWait(FenceTypeFragment, kFenceDoFGlareStep)
pass:AddFenceWait(FenceTypeFragment, kFenceDoFBlurStep)
	passes.DoFComposite = pass



end



---------------------------------------------------------------------------------------------------------

--	SetupPostDOF

---------------------------------------------------------------------------------------------------------

local function SetupPostDOF(ren, outColor, outDepth)

	local pass 

-- Additive after DOF for some particles

	pass = ren:AddGeometryPass("PostDOF")

	pass:SetBlend( true, PE_SHADER_BLEND_ONE, PE_SHADER_BLEND_ONE )

	pass:SetDepthWrite( false )

	pass:SetDepthRead( false )

	pass:colorTarget( outColor )

	pass:depthTarget( outDepth )
    pass:SetPriority(-2500)

pass:AddFenceInsert(FenceTypeFragment, kFenceDofComposite)
	passes.AdditivePostDOFGeom = pass

end



---------------------------------------------------------------------------------------------------------

--	Setup motion blur

---------------------------------------------------------------------------------------------------------

local function SetupMotionBlur( ren, outColor, outDepth )

	local pass 

	-- Motion blur everything

	pass = ren:AddGeometryPass("MotionBlur")

	pass:SetClear( true )
    pass:SetAction(ActionClear, ActionClear, ActionStore, ActionIgnore)

	pass:colorTarget( outColor )

	pass:depthTarget( outDepth )
    pass:SetPriority( -3000 )
pass:SetLowResolution( true )
pass:AddFenceWait(FenceTypeFragment, kFenceDofComposite)
pass:AddFenceWait(FenceTypeFragment, kFenceTerrain)

	passes.MotionBlur = pass



	pass = ren:AddGeometryPass("LateMotion")

	pass:colorTarget( outColor )

	pass:depthTarget( outDepth )
    pass:SetPriority( -3001 )
pass:SetLowResolution( true )

	passes.LateMotionBlur = pass

end



---------------------------------------------------------------------------------------------------------

--	Turn motion blur on and off

---------------------------------------------------------------------------------------------------------

function RenderGetMotionBlurEnable( )

	if bEnableMotionBlur then

		return (textures.lensDistortInput==textures.postMotionBlur)

	end

	return false

end

function RenderSetMotionBlurEnable( onOff )

	if bEnableMotionBlur then



		-- change the input to the lens distortion

		if onOff then

			textures.lensDistortInput = textures.postMotionBlur

		else

			textures.lensDistortInput = textures.preMotionBlur

		end



		-- turn the passes on or off

		passes.MotionBlur:SetEnabled(onOff)

		passes.LateMotionBlur:SetEnabled(onOff)

	end

end



---------------------------------------------------------------------------------------------------------

--	Setup lens distortion

---------------------------------------------------------------------------------------------------------

local function SetupLensDistort( ren, resources )

	local pass 

	-- Lens Distort the DoF result

    pass = ren:AddQuadPass( resources, (bHalfSizedPostProcessing and "LensDistortionPerf" or "LensDistortion") )
    pass:SetAction(ActionIgnore, ActionIgnore, ActionStore, ActionIgnore)
    pass:SetDirectOutput(true)

	pass:colorTarget( nil )

	pass:SetGridSize(33,17)

	pass:SetPos( -1.0-OnePixX*17, -1.0-OnePixY*17, 1.0+OnePixX*17, 1.0+OnePixY*17 )

	pass:SetUv(  0.0, topV, 1.0, bottomV  )
    pass:SetPriority( -20000 )

	passes.LensDistortion = pass



	-- Lens Distort the DoF result (with moar pain!!)

    pass = ren:AddQuadPass( resources, (bHalfSizedPostProcessing and "LensDistortionPainPerf" or "LensDistortionPain") )

	pass:colorTarget( nil )

	pass:SetGridSize(33,17)

	pass:SetPos( -1.0-OnePixX*17, -1.0-OnePixY*17, 1.0+OnePixX*17, 1.0+OnePixY*17 )

	pass:SetUv(  0.0, topV, 1.0, bottomV  )

	pass:SetEnabled(false);
    pass:SetPriority( -20001 )

	passes.LensDistortionPain = pass

end





---------------------------------------------------------------------------------------------------------

--	Initialize rendering

---------------------------------------------------------------------------------------------------------

function RenderInitialize( ren, resources, bHalfRez, msaaInput )
-- Allocate textures
    bHalfSizedPostProcessing = bHalfRez
	msaaType = msaaInput

if TARGET=="IOS64" then
	ResX = ren:GetScreenWidth()
	ResY = ren:GetScreenHeight()
	OnePixX = 1/ResX
	OnePixY = 1/ResY
elseif TARGET=="WIN64" then
	ResX = ren:GetScreenWidth()
	ResY = ren:GetScreenHeight()
	OnePixX = 1/ResX
	OnePixY = 1/ResY
end

	-- correct stereoscopic option
	if bEnableS3D then
		bEnableS3D = ren:GetS3DAvailable()
	end

	-- MB: Always enable the depth texture or we wont get depth testing
	renderTargets.MsaaDepth = ren:AddRenderTarget( "MsaaDepth", ResX, ResY, RenderTarget_Depth, msaaType )
	renderTargets.MsaaColor = ren:AddRenderTarget( "MsaaColor", ResX, ResY, RenderTarget_Color, msaaType )
	renderTargets.HiResStore = ren:AddRenderTarget( "HiResStore", ResX, ResY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )

--	renderTargets.ShaderColor = ren:AddRenderTarget( "ShaderColor", ResX, ResY, RenderTarget_Color, msaaType )
--	renderTargets.ShaderDepth = ren:AddRenderTarget( "ShaderDepth", ResX, ResY, RenderTarget_Depth, msaaType )

	textures.MsaaColor = renderTargets.MsaaColor
	textures.MsaaDepth = renderTargets.MsaaDepth

	if bEnableMotionBlur or bEnableDOF then
		renderTargets.HiResPong = ren:AddRenderTarget( "HiResPong", ResX, ResY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )

		if bEnableMotionBlur then
			renderTargets.MotionBlurDepth = ren:AddRenderTarget( "MotionBlurDepth", ResX, ResY, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE )
		end
	end



	if bRemoveMSAA then
		renderTargets.fullDepth = ren:AddRenderTarget( "fullDepth", ResX, ResY, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE )
		renderTargets.fullColor = ren:AddRenderTarget( "fullColor", ResX, ResY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
	else
		renderTargets.fullDepth = renderTargets.MsaaDepth	--ren:AddRenderTarget( ResX, ResY, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE )
		renderTargets.fullColor = renderTargets.MsaaColor	--ren:AddRenderTarget( ResX, ResY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
	end

	textures.fullColor = renderTargets.fullColor
	textures.fullDepth = renderTargets.fullDepth


	if bEnableLowParticles then
		renderTargets.LoResColor = ren:AddRenderTarget( "LoResColor", ResX/4, ResY/4, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
		renderTargets.LoResDepth = ren:AddRenderTarget( "LoResDepth", ResX/4, ResY/4, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE )
		textures.LoResColor = renderTargets.LoResColor
		textures.LoResDepth = renderTargets.LoResDepth
	end

	if bEnableS3D then
		-- ultra low depth buffer
		local dw, dh = 128, 32
		-- used for configuring S3D
		-- need to double buffer
		-- size is limited by minimum depth buffer height
		renderTargets.S3DColor  = ren:AddRenderTarget("S3DColor", dw, dh, RenderTarget_Color, PE_TEXTURE_MSAA_NONE)
		renderTargets.S3DDepth0 = ren:AddRenderTarget("S3DDepth0", dw, dh, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE)
		renderTargets.S3DDepth1 = ren:AddRenderTarget("S3DDepth1", dw, dh, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE)
	end

    if bHalfSizedPostProcessing then
        local proX = ResX
        local proY = ResY/2

        renderTargets.LumComp0 = ren:AddRenderTarget( "LumComp0", proX, proY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
        renderTargets.LumComp1 = ren:AddRenderTarget( "LumComp1", proX, proY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
        renderTargets.LumDepth = ren:AddRenderTarget( "LumDepth", proX, proY, RenderTarget_Depth, PE_TEXTURE_MSAA_NONE )
    end

	

	if bEnableDOF then
		local halfX = ResX/2
		local halfY = ResY/2
		local blurX = ResX/4
		local blurY = ResY/4
		local glareX = ResX/8
		local glareY = ResY/8

		if bEnableMacroTile then
			local rt
			local rtSize = { 512*2, 384*3 }
			local rtHalfColor = { 0, 0, halfX, halfY }
			local rtBlur0 = { 0, 384, blurX, blurY }
			local rtBlur1 = { 0, 768, blurX, blurY }
			local rtBlur2 = { 512, 0, blurX, blurY }
			local rtGlare0 = { 0, 576, blurX, blurY }	-- 1st glare same size as blur
			local rtGlare1 = { 0, 960, glareX, glareY }
			local rtGlare2 = { 512, 192, glareX, glareY }
			local rtGlare3 = { 512, 576, glareX, glareY }
			local DOFGlareAccum = { 512, 960, glareX, glareY }

			-- macro surface for blur
			rt = ren:AddRenderTarget( "DOFMacroBlur", rtSize[1], rtSize[2], RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			rt:setMacroTile(2,3)

			renderTargets.DOFMacroBlur = rt

			CreateMacroRT(ren,rt,"DOFHalfColor",rtHalfColor,rtSize)
			CreateMacroRT(ren,rt,"DOFBlur0",rtBlur0,rtSize)
			CreateMacroRT(ren,rt,"DOFBlur1",rtBlur1,rtSize)
			CreateMacroRT(ren,rt,"DOFBlur2",rtBlur2,rtSize)
			CreateMacroRT(ren,rt,"DOFGlare0",rtGlare0,rtSize)
			CreateMacroRT(ren,rt,"DOFGlare1",rtGlare1,rtSize)
			CreateMacroRT(ren,rt,"DOFGlare2",rtGlare2,rtSize)
			CreateMacroRT(ren,rt,"DOFGlare3",rtGlare3,rtSize)
			CreateMacroRT(ren,rt,"DOFGlareAccum",DOFGlareAccum,rtSize)

			textures.DOFGlareSource = textures.DOFHalfColor
			-- cannot do stereoscopic with macro tiles
		else

		

			-- NOTE: PS3 used 2x half size buffers

			--		We use 1x half size, 3x quarter size buffers

			--		net savings of 1 quarter size buffer
			renderTargets.DOFHalfColor = ren:AddRenderTarget( "DOFHalfColor", halfX, halfY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFBlur0 = ren:AddRenderTarget( "DOFBlur0", blurX, blurY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFBlur1 = ren:AddRenderTarget( "DOFBlur1", blurX, blurY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFBlur2 = ren:AddRenderTarget( "DOFBlur2", blurX, blurY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			textures.DOFHalfColor = renderTargets.DOFHalfColor
			textures.DOFBlur0 = renderTargets.DOFBlur0
			textures.DOFBlur1 = renderTargets.DOFBlur1
			textures.DOFBlur2 = renderTargets.DOFBlur2

			-- MB: Original PS3 version used 2 ping pong buffers and an accumulation buffer

			--		We use 5 total buffers to make it easier to accomodate macro tile
			renderTargets.DOFGlare0 = ren:AddRenderTarget( "DOFGlare0", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFGlare1 = ren:AddRenderTarget( "DOFGlare1", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFGlare2 = ren:AddRenderTarget( "DOFGlare2", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFGlare3 = ren:AddRenderTarget( "DOFGlare3", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
			renderTargets.DOFGlareAccum = ren:AddRenderTarget( "DOFGlareAccum", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )

			textures.DOFGlareSource = textures.DOFBlur0
			textures.DOFGlare0 = renderTargets.DOFGlare0
			textures.DOFGlare1 = renderTargets.DOFGlare1
			textures.DOFGlare2 = renderTargets.DOFGlare2
			textures.DOFGlare3 = renderTargets.DOFGlare3
			textures.DOFGlareAccum = renderTargets.DOFGlareAccum

			-- stereo glare accumulation

			if bEnableS3D then
				renderTargets.DOFGlare3Left = ren:AddRenderTarget( "DOFGlare3Left", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
				renderTargets.DOFGlareAccumLeft = ren:AddRenderTarget( "DOFGlareAccumLeft", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
				renderTargets.DOFGlare3Right = ren:AddRenderTarget( "DOFGlare3Right", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
				renderTargets.DOFGlareAccumRight = ren:AddRenderTarget( "DOFGlareAccumRight", glareX, glareY, RenderTarget_Color, PE_TEXTURE_MSAA_NONE )
				textures.DOFGlare3Left = renderTargets.DOFGlare3Left
				textures.DOFGlareAccumLeft = renderTargets.DOFGlareAccumLeft
				textures.DOFGlare3Right = renderTargets.DOFGlare3Right
				textures.DOFGlareAccumRight = renderTargets.DOFGlareAccumRight
			end
		end
	end

	images.Black = ren:AddImageTexture( resources, "Black" )
	images.WhiteNoise = ren:AddImageTexture( resources, "WhiteNoise" )
    images.BlurredNoise = ren:AddImageTexture( resources, "BlurredNoise" )

-- Create render passes	

	if 1 then

        local    targets = { renderTargets.HiResStore, renderTargets.HiResPong, renderTargets.HiResStore, renderTargets.HiResPong }

        if bHalfSizedPostProcessing then
            targets = { renderTargets.HiResStore, renderTargets.LumComp1, renderTargets.LumComp0, renderTargets.LumComp1 }
        end

		local	targetIndex = 1;

		SetupOpaqueRender(ren, renderTargets.MsaaColor, renderTargets.MsaaDepth, renderTargets.LoResColor,renderTargets.LoResDepth)

		-- transfer to our internal readabile depth

--			local pass
--			pass = ren:AddQuadPass( resources, "RemoveMSAA" )
--			pass:SetClear( true )
--			pass:colorTarget( renderTargets.ShaderColor )
--			pass:depthTarget( renderTargets.ShaderDepth )
--			pass:SetPos( -1.0, -1.0, 1.0, 1.0 )
--			pass:SetUv( 0.0, topV, 1.0, bottomV )
--			pass:GetShader():SetTextureParam( "texColor", textures.MsaaColor ) 
--			pass:GetShader():SetTextureParam( "texDepth", textures.MsaaDepth ) 
--			pass:GetShader():SetParam4f( "pixOffset", { OnePixX, OnePixY, -OnePixX, -OnePixY } ) 
--			passes.DepthTransfer = pass


		if bRemoveMSAA then
			SetupRemoveMSAA(ren, resources)
		end

		SetupTransparentRender(ren, renderTargets.fullColor, renderTargets.fullDepth)

		if bEnableLowParticles then
			SetupParticlesLow(ren, resources, renderTargets.LoResColor,renderTargets.LoResDepth,targets[targetIndex])
		else
			targets[1] = renderTargets.fullColor
			SetupParticlesHigh(ren, targets[1], renderTargets.fullDepth)
		end

		if bEnableS3D then
			-- relies on LoResDepth
			SetupS3DDepth(ren,resources,renderTargets.LoResDepth,renderTargets.S3DColor,renderTargets.S3DDepth0)
		end

		-- MB: additive noZ
		-- writes to same as final particle target
		-- relies on depth blur being computed during DOF phase
		-- original PS3 shaders compute blur during particle recombine
		-- cannot be written as part of SetupTransparentRender pass without a render target change
		-- because the depth target MUST be null here (we read from it)
		SetupAdditiveNoZ(ren,targets[targetIndex])

		if bEnableDOF then
			textures.DOFSource = targets[targetIndex]

			-- DOF writes to a new target
			targetIndex = targetIndex + 1
			SetupDOF(ren,resources, targets[targetIndex])
		end

		-- postDOF writes to same as DOF
		SetupPostDOF(ren,targets[targetIndex])

		textures.preMotionBlur = targets[targetIndex]

		if bEnableMotionBlur then
			textures.MotionBlur = targets[targetIndex]

			-- motion blur writes to a new target
			targetIndex = targetIndex + 1
			SetupMotionBlur(ren, targets[targetIndex], renderTargets.LumDepth) --renderTargets.MotionBlurDepth)
		end

		textures.postMotionBlur = targets[targetIndex]
		textures.lensDistortInput = targets[targetIndex]
		SetupLensDistort(ren,resources)
	end



-- A debug HUD where we render text and stuff in screen space
	passes.Hud = ren:AddGeometryPass("HUD")
	passes.Hud:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE_MINUS_SRC_ALPHA )
	passes.Hud:colorTarget( nil )
end



function RenderInitializeContext( ren, renderContext )
	renderContext:motionTex( textures.MotionBlur )
	renderContext:blurTex( textures.DOFBlur2 )
	renderContext:noiseTex( images.WhiteNoise:GetTexture() )
	renderContext:fullDepthTex( textures.fullDepth )
	renderContext:reducedDepthTex( textures.LoResDepth )

    -- IA: we need this here (as well?), as <environment.AddToRenderContext> reads this. this causes flicker on IOS!
    renderContext:oldViewProj( ren:m_oldViewProj() )

	if bEnableS3D then
		-- return the one that is going to be rendered to this frame
		-- we do calculations on it before rendering starts
		renderContext:s3dDepthTex( renderTargets.S3DDepth1 )
	end

	local w, h = ResX, ResY

	if renderTargets.fullColor then

		w = renderTargets.fullColor:getWidth()

		h = renderTargets.fullColor:getHeight()

	end



	renderContext:src2Noise(

		{ w / images.WhiteNoise:GetTexture():getWidth()

		, h / images.WhiteNoise:GetTexture():getHeight()

		, 0, 0 } )

	

end





-- update the blur parameters

function UpdateBlurShader( cam, shader )
	local Near = cam:GetNearClip()
	local Far = cam:GetFarClip()
	local DepthBias = (Far+Near)/(Far-Near)
	local DepthScale = (-2.0*Far*Near)/(Far-Near)
	local zstart = cam:GetDoFZStart()
	local zfstart = cam:GetDoFZFocalStart()
	local zfend = cam:GetDoFZFocalEnd()
	local zend = cam:GetDoFZEnd()

	shader:SetParamf( "depthBias", DepthBias ) 
	shader:SetParamf( "depthScale", DepthScale ) 
	shader:SetParam4f( "blurZRanges", { zfstart, 1.0/(zstart-zfstart), zfend, 1.0/(zend-zfend)} ) 
	shader:SetTextureParam( "depthTex", textures.fullDepth )
end



-- update the glare textures

-- sets the glare textures into the appropriate shader

-- ping-pongs the textures

function UpdateGlareTextures( rtName, srcName )
   	local shader
	local pass = passes.GlareComposite

	if pass~=nil then

		-- write to the correct glare texture
		pass:colorTarget( renderTargets[rtName] )

		-- accum is black on first frame 		
		local glareAccum = images.Black:GetTexture()
		if glareInit[srcName] then
			glareAccum = textures[srcName]
		else
			glareInit[srcName] = true
		end

		-- read from the accumulation texture
		shader = pass:GetShader()
		shader:SetTextureParam( "texAccum", glareAccum )

		-- if we are using macro tile, this is non-nil
		-- and is a copy of rtName to srcName
		-- otherwise, we need to ping-pong textures
		if passes.DOFGlareAccum==nil then
			-- DOFGlare3<==>DOFGlareAccum
			local temp = renderTargets[rtName]
			renderTargets[rtName] = renderTargets[srcName]
			renderTargets[srcName] = temp
			temp = textures[rtName]
			textures[rtName] = textures[srcName]
			textures[srcName] = temp
		end
	end

	-- write the new glare texture into the DOFComposite source
	pass = passes.DoFComposite
	if pass~=nil then
		shader = pass:GetShader()
		shader:SetTextureParam( "GlareTex", textures[srcName] )
	end
end



function RenderSelectField(ren,renderContext,field)
	-- select the field
	RCSelectField(renderContext,ren,field)
	if field==0 then
		-- left field
		UpdateGlareTextures("DOFGlare3Left","DOFGlareAccumLeft")
	else
		-- right field
		UpdateGlareTextures("DOFGlare3Right","DOFGlareAccumRight")
	end
end

function RenderUpdate( ren, renderContext )
	-- Cache some values from the render context so we don't have to do as
	-- much typing.
	local cam = renderContext:camera()
	local env = renderContext:env()
	local pass, shader

	-- MB: moved from RenderInitializeContext
	renderContext:oldViewProj( ren:m_oldViewProj() )

--@TODO: Put these in the Environment Nodes?
-- RickNote: I moved these to global variables in camera.cpp, this helps with the new 
--   camera system
	passes.Geometry:traversal( renderContext:colorTraversal() )
	passes.EarlyGeometry:traversal( renderContext:earlyColorTraversal() )
	passes.LateGeometry:traversal( renderContext:lateColorTraversal() )
	passes.Discard:traversal( renderContext:discardTraversal() )
	passes.BlendGeom:traversal( renderContext:blendTraversal() )
	passes.AdditiveGeom:traversal( renderContext:additiveTraversal() )
	passes.AdditiveNoZGeom:traversal( renderContext:additiveNoZTraversal() )

	if passes.DepthDownsample then
		passes.DepthDownsample:GetShader():SetTextureParam( "texDepth", renderTargets.fullDepth ) --renderTargets.MsaaDepth
	end

	if passes.Particles then
		passes.Particles:traversal( renderContext:particleTraversal() )
		passes.ParticlesNoZ:traversal( renderContext:particleNoZTraversal() )
		passes.SubParticles:traversal( renderContext:subParticleTraversal() )
	end


--@TODO: Put this stuff in the Camera class? Seems kinda specialized...
	if passes.DepthStore then
		pass = passes.DepthStore
		shader = pass:GetShader()
		shader:SetParam4f( "HalfPartTexPxl", { 0.5 / renderTargets.LoResColor:getWidth(), 0.5 / renderTargets.LoResColor:getHeight(), 0, 0 } ) 
		shader:SetTextureParam( "colorTex", textures.fullColor )
		shader:SetTextureParam( "partTex", textures.LoResColor )

		-- MB: Moved this to the DOFComposite pass
		--UpdateBlurShader(cam,shader)
	end

	if passes.BloomGlareStore then
    	shader = passes.BloomGlareStore:GetShader()
		shader:SetParam4f( "GlareThreshold", {0.2,0.2,0.2,1*env:GetInterpBloomAlpha() } )
		UpdateGlareTextures("DOFGlare3","DOFGlareAccum")
	end

	if passes.DoFComposite then
		pass = passes.DoFComposite
		shader = pass:GetShader()
		shader:SetParamf( "bloomAlpha", env:GetInterpBloomAlpha()+0.5*cam:GetBloomBias() )
		shader:SetParamf( "stepBegin", env:GetInterpStepBegin())
		shader:SetParamf( "stepEnd", env:GetInterpStepEnd())	
		shader:SetParam4f( "PixelSize", { OnePixX, OnePixY, 0, 0 } )
		shader:SetParam4f( "PixelSizeLo", { 2*OnePixX, 2*OnePixY, 0, 0 } )
		shader:SetTextureParam( "LoTex", textures.DOFBlur2 )
		shader:SetTextureParam( "HiTex", textures.DOFSource )
		shader:SetTextureParam( "NoiseTex", images.BlurredNoise:GetTexture() )
		UpdateBlurShader(cam,shader)
	end

	if passes.AdditivePostDOFGeom then
		passes.AdditivePostDOFGeom:traversal( renderContext:additivePostDOFTraversal() )
	end

	if passes.MotionBlur then
		passes.MotionBlur:traversal( renderContext:motionTraversal() )
		passes.LateMotionBlur:traversal( renderContext:lateMotionTraversal() )
	end

	if passes.LensDistortion then
		local pain = renderContext:pain()
		local aspect = cam:GetAspect()
		local fov = math.rad( cam:GetFov() )
		local lensMaxRadius = math.sqrt( 0.25 + 0.25 / (aspect * aspect) )

		if pain>0 then
			shader = passes.LensDistortionPain:GetShader()
			passes.LensDistortionPain:SetEnabled(true)
			passes.LensDistortion:SetEnabled(false)
			shader:SetParamf( "pain", pain )
			passes.LensDistortionPain:SetClear(true)
		else
			shader = passes.LensDistortion:GetShader()
			passes.LensDistortionPain:SetEnabled(false)
			passes.LensDistortion:SetEnabled(true)
			passes.LensDistortion:SetClear(true)
		end

		shader:SetTextureParam( "tex0", textures.lensDistortInput )
		shader:SetParamf( "aspectRatio", aspect )
		shader:SetParamf( "maxRadius", lensMaxRadius )
		shader:SetParamf( "fov", fov )
		shader:SetParamf( "R", lensMaxRadius / math.tan( fov*0.5 ) )
		shader:SetParam4f( "pixelSize", { OnePixX, OnePixY, 0, 0 } )
	end

	if bEnableS3D then
		UpdateS3DDepth()
	end

	if passes.Hud then
		passes.Hud:traversal( renderContext:hudTraversal() )
	end
end


