-------------------------------------------------------------------------------
-- Notes:  Lists all the resources that are loaded at program startup and
--         remain loaded until program termination.
--
-- Author: John Edwards
-- Date C: 5-2-2007
-------------------------------------------------------------------------------

-- Define this to support the kiosk upsell screen
--local KIOSK_VERSION = true

-- Put the resources you want to stream in first at the top of the file.
resource "Texture" "Font" { source = "Font.dds", category = "Font" }
resource "Texture" "fontios" { source = "fontios.dds", category = "Font" }
resource "Texture" "MenuSystem" { source = "MenuSystem.dds", category = "UI" }

resource "Mesh" "BlendNinja" { source = "C_ClothNinja.PSSG", instanceLimit = 2 }

resource "ScreamBank" "SharSfx" { source = "Shared-SFX.bank" }

--!MP! Shaders which I've had to add
resource "Shader" "Col3D" { vs = "SimpleColor_vs.cg", fs = "SimpleColor_fs.cg", backfaceCulling = 0, instanceLimit = 64 }
--resource "Shader" "SimpleTexture" { vs = "SimpleTexture_vs.cg", fs = "SimpleTexture_fs.cg", backfaceCulling = 0, instanceLimit = 1 }
--!MP! End

resource "Texture" "iphone_7" { source = "iphone7small.pvr" }
resource "Texture" "tutorial_controls" { source = "analogtutorial.pvr" }
resource "Shader" "VisualizeAccelerometer" { vs = "elvisWasHere", fs = "butHeresJohnny", backfaceCulling = -1, instanceLimit = 2 }


resource "Shader" "DebugLine" { vs = "DebugLine_vs.cg", fs = "DebugLine_fs.cg", backfaceCulling = -1, instanceLimit = 1024 }
resource "Shader" "DebugText" { vs = "Sprite_vs.cg", fs = "Text_fs.cg", alphaTest = 128, instanceLimit = 2 }
resource "Shader" "Text" { vs = "Sprite_vs.cg", fs = "Text_fs.cg", defines="MULTISAMPLE", instanceLimit = 4 }
resource "Shader" "TextManualAlpha" { vs = "Sprite_vs.cg", fs = "Text_fs.cg", defines="MANUAL_DEPTH_TEST", instanceLimit = 4 }
resource "Shader" "Menu" { vs = "Sprite_vs.cg", fs = "Text_fs.cg", defines="IOSMENU", instanceLimit = 4 }

resource "Shader" "MeshDudeLocal" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines="DECORATION DUDE AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS DECORATION DUDE AO", alphaTest = 128, backfaceCulling = -1, stencil = 1, instanceLimit = 2 }
resource "Shader" "MeshDudeRemote" { skinnedVersion=true,  vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines="REMOTE DECORATION DUDE AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS DECORATION DUDE AO", alphaTest = 128, backfaceCulling = -1, stencil = 2, instanceLimit = 2 }
resource "Shader" "MeshDudeLocalFrozen" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines="DECORATION DUDE FROZEN AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS DECORATION DUDE FROZEN AO", alphaTest = 128, backfaceCulling = -1, stencil = 1, instanceLimit = 2 }
resource "Shader" "MeshDudeRemoteFrozen" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines="REMOTE DECORATION DUDE FROZEN AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS DECORATION DUDE FROZEN AO", alphaTest = 128, backfaceCulling = -1, stencil = 2, instanceLimit = 2 }

resource "Shader" "Mesh" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL UV_OFFSET ALPHATESTENABLE", lodDefines="EDGE_DETAIL UV_OFFSET DITHER_OUT", forceLod = true,
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 354 }
resource "Shader" "MeshShadow" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL UV_OFFSET SHADOW_MAP ALPHATESTENABLE", lodDefines="EDGE_DETAIL UV_OFFSET SHADOW_MAP DITHER_OUT", forceLod = true,
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 16 }	
resource "Shader" "MeshSandTop" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_TOP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_TOP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 4 }
resource "Shader" "MeshSandSide" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_SIDE UV_OFFSET ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_SIDE UV_OFFSET DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 800 }
resource "Shader" "MeshSandTopSide" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_TOP SAND_SIDE ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_TOP SAND_SIDE DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 160 }
resource "Shader" "MeshSandSideShadow" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_SIDE UV_OFFSET SHADOW_MAP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_SIDE UV_OFFSET SHADOW_MAP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 32 }
resource "Shader" "MeshSandTopSideShadow" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_TOP SAND_SIDE SHADOW_MAP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_TOP SAND_SIDE SHADOW_MAP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 4 }
resource "Shader" "MeshSandSideFancy" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_SIDE UV_OFFSET SAND_FANCY SHADOW_MAP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_SIDE UV_OFFSET SPECULAR SAND_FANCY SHADOW_MAP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 16 }
resource "Shader" "MeshSandSideSpecular" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_SIDE UV_OFFSET SPECULAR SHADOW_MAP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SAND_SIDE UV_OFFSET SPECULAR SHADOW_MAP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 92 }
resource "Shader" "MeshSandFull" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_FULL UV_OFFSET SHADOW_MAP ALPHA ALPHATESTENABLE1", lodDefines="EDGE_DETAIL SAND_FULL UV_OFFSET SHADOW_MAP ALPHA",
	alphaTest = 1, backfaceCulling = -1, stencil = 0, instanceLimit = 8 }
resource "Shader" "MeshSnowTop" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SNOW_TOP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SNOW_TOP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 80 }
resource "Shader" "MeshSnowTopSide" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SNOW_TOP SAND_SIDE ALPHATESTENABLE", lodDefines="EDGE_DETAIL SNOW_TOP SAND_SIDE DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 528 }
resource "Shader" "MeshSnowTopSideShadow" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SNOW_TOP SAND_SIDE SHADOW_MAP ALPHATESTENABLE", lodDefines="EDGE_DETAIL SNOW_TOP SAND_SIDE SHADOW_MAP DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 90 }
resource "Shader" "MeshCham" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL CHAMELEON UV_OFFSET ALPHATESTENABLE", lodDefines="CHAMELEON UV_OFFSET DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 48 }
resource "Shader" "MeshChamUv" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL CHAMELEON UV_CHAM UV_OFFSET ALPHATESTENABLE", lodDefines="CHAMELEON UV_CHAM UV_OFFSET DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 12 }
resource "Shader" "MeshChamUvNorm" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL CHAMELEON CHAM_NORM UV_CHAM UV_OFFSET ALPHATESTENABLE", lodDefines="CHAMELEON CHAM_NORM UV_CHAM UV_OFFSET DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 56 }
resource "Shader" "MeshChamAlpha" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL CHAMELEON UV_OFFSET ALPHA", lodDefines="CHAMELEON UV_OFFSET ALPHA",
	backfaceCulling = -1, stencil = 0, instanceLimit = 12 }
resource "Shader" "MeshChamUvAlpha" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL CHAMELEON UV_CHAM UV_OFFSET ALPHA", lodDefines="CHAMELEON UV_CHAM UV_OFFSET ALPHA",
	backfaceCulling = -1, stencil = 0, instanceLimit = 2 }
resource "Shader" "MeshChamSandSide" {
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="EDGE_DETAIL SAND_DETAIL SAND_SIDE CHAMELEON ALPHATESTENABLE", lodDefines="CHAMELEON SAND_SIDE DITHER_OUT",
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 2 }
resource "Shader" "MeshBillboard" { 
	skinnedVersion=true, vs = "Mesh_vs.cg", fs = "Mesh_fs.cg", defines="ALPHATESTENABLE", prepassDefines = "PREPASS ALPHA UV_OFFSET", 
	alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 8 }

resource "Shader" "4K_MeshCloth" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", gs = "Cloth_gs.cg", defines = "DECORATION ALPHATESTENABLE", alphaTest = 128, instanceLimit = 80 }


resource "Shader" "MeshCloth" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "DECORATION ALPHATESTENABLE", alphaTest = 128, instanceLimit = 80 }
resource "Shader" "ClothAmbient" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS AO SAND", alphaTest = 128, stencil = 0, instanceLimit = 4 }
resource "Shader" "ClothAmbientSimple" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "AO ALPHATESTENABLE", prepassDefines = "PREPASS AO", alphaTest = 128, stencil = 0, instanceLimit = 2 }
resource "Shader" "ClothAmbientSimplest" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "AO ALPHATESTENABLE", prepassDefines = "PREPASS AO", alphaTest = 128, stencil = 0, instanceLimit = 2 }
resource "Shader" "ClothPainting" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "AO PAINTING ALPHATESTENABLE", prepassDefines = "PREPASS AO PAINTING", alphaTest = 128, stencil = 0, instanceLimit = 1 }
resource "Shader" "ClothLocal" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "DUDE AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS DUDE AO SAND", alphaTest = 128, stencil = 1, instanceLimit = 1 }
resource "Shader" "ClothLocalScarf" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "SCARF AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS SCARF AO SAND", alphaTest = 128, stencil = 1, instanceLimit = 1 }
resource "Shader" "ClothRemote" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "REMOTE DUDE AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS DUDE AO SAND", alphaTest = 128, stencil = 2, instanceLimit = 1 }
resource "Shader" "ClothRemoteScarf" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "REMOTE SCARF AO SAND ALPHATESTENABLE", prepassDefines = "PREPASS SCARF AO SAND", alphaTest = 128, stencil = 2, instanceLimit = 1 }
resource "Shader" "ClothSpiritMesh" { skinnedVersion=true, vs = "Cloth_vs.cg", fs = "Cloth_fs.cg", defines = "DECORATION SAND ALPHATESTENABLE", alphaTest = 128, stencil = 3, instanceLimit = 1 }

resource "Shader" "Skybox" { vs = "Skybox_vs.cg", fs = "Skybox_fs.cg", stencil = 0, backfaceCulling = 1, instanceLimit = 2 }
resource "Shader" "Particle" { vs = "Particle_vs.cg", fs = "Particle_fs.cg", defines = "ALPHATESTENABLE", alphaTest = 128, stencil = 0, instanceLimit = 4 }
resource "Shader" "ParticleSoft" { vs = "ParticleBlended_vs.cg", fs = "ParticleBlended_fs.cg", defines = "FOG SOFT", prepassDefines = "FOG SOFT LOW_RES", alphaTest = 0.0001, instanceLimit = 4 }
resource "Shader" "Scattering" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "", prepassDefines = "", backfaceCulling = -1, instanceLimit = 4 }
resource "Shader" "ScatteringFog" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "UV_OFFSET FOG SOFT", prepassDefines = "UV_OFFSET FOG SOFT", backfaceCulling = -1, instanceLimit = 40 }
resource "Shader" "FogTube" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "UV_OFFSET FOG SOFT CLOUDTUBE", prepassDefines = "UV_OFFSET FOG SOFT CLOUDTUBE", backfaceCulling = -1, instanceLimit = 4 }
resource "Shader" "ScatteringSpecular" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "UV_OFFSET FOG SPECULAR", prepassDefines = "UV_OFFSET FOG SPECULAR", backfaceCulling = -1, instanceLimit = 12 }
resource "Shader" "ScatteringUvOffset" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "UV_OFFSET", prepassDefines = "UV_OFFSET", backfaceCulling = -1, instanceLimit = 96 }
resource "Shader" "ScatteringAdditive" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "UV_OFFSET", prepassDefines = "ADDITIVE_ZHACK UV_OFFSET", backfaceCulling = -1, instanceLimit = 120 }
resource "Shader" "ScatteringSoft" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines="UV_OFFSET SOFT", prepassDefines="UV_OFFSET SOFT LOW_RES", backfaceCulling = -1, instanceLimit = 136 }
resource "Shader" "ScatteringAdditiveNoDepth" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines = "ADDITIVE_ZHACK UV_OFFSET NODEPTH", prepassDefines = "UV_OFFSET NODEPTH", backfaceCulling = -1, instanceLimit = 272 }
resource "Shader" "ScatteringSoftNoDepth" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines="UV_OFFSET SOFT NODEPTH", prepassDefines="UV_OFFSET SOFT LOW_RES NODEPTH", backfaceCulling = -1, instanceLimit = 32 }
resource "Shader" "CloudTube" { skinnedVersion=true, vs = "Scattering_vs.cg", fs = "Scattering_fs.cg", defines="UV_OFFSET SOFT CLOUDTUBE", prepassDefines="UV_OFFSET SOFT LOW_RES CLOUDTUBE", backfaceCulling = -1, instanceLimit = 120 }
resource "Shader" "Shout" { vs = "Shout_vs.cg", fs = "Shout_fs.cg", defines="SOFT", prepassDefines="SOFT LOW_RES ALPHATESTENABLE", alphaTest = 1, backfaceCulling = -1, instanceLimit = 64 }
resource "Shader" "Simple" { vs = "Simple_vs.cg", fs = "Simple_fs.cg", instanceLimit = 96 }
resource "Shader" "Dune" { vs = "Dune_vs.cg", fs = "Dune_fs.cg", defines = "DETAIL", lodDefines = "DETAIL", backfaceCulling = -1, forceLod = true, stencil = 4, instanceLimit = 45 }
resource "Shader" "Trail" { vs = "Trail_vs.cg", fs = "Trail_fs.cg", stencil = 0, instanceLimit = 1 }
resource "Shader" "GuiImage" { vs = "GuiImage_vs.cg", fs = "GuiImage_fs.cg", instanceLimit = 32 }
resource "Shader" "GuiImage_Special" { vs = "GuiImage_vs.cg", fs = "GuiImage_fs.cg", defines="SPECIAL", instanceLimit = 32 }
resource "Shader" "SandFallGradient" { vs = "SandFall_vs.cg", fs = "SandFall_fs.cg", defines="SAMPLETERRAIN GRADIENT ALPHATESTENABLE", alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 8 }
resource "Shader" "SandFallGradientLit" { vs = "SandFall_vs.cg", fs = "SandFall_fs.cg", defines="SAMPLETERRAIN GRADIENT LIT ALPHATESTENABLE", alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 48 }
resource "Shader" "SandFallLit" { vs = "SandFall_vs.cg", fs = "SandFall_fs.cg", defines="SAMPLETERRAIN LIT ALPHATESTENABLE", alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 32 }
resource "Shader" "SandFallLitCham" { vs = "SandFall_vs.cg", fs = "SandFall_fs.cg", defines="SAMPLETERRAIN GRADIENT LIT CHAM ALPHATESTENABLE", alphaTest = 128, backfaceCulling = -1, stencil = 0, instanceLimit = 4 }
resource "Shader" "FogVolume" { vs = "FogVolume_vs.cg", fs = "FogVolume_fs.cg", defines = "LOW_RES", instanceLimit = 1 }
resource "Shader" "Portal" { vs = "Portal_vs.cg", fs = "Portal_fs.cg", instanceLimit = 6 }

-- Post processing shaders
resource "Shader" "Occlusion" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionBias" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoise" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "NOISE", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseBias" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "NOISE COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionExclusion" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "EXCLUSION", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionExclusionBias" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "EXCLUSION COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseExclusion" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "NOISE EXCLUSION", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseExclusionBias" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "NOISE EXCLUSION COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionPlayer" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "PLAYER NOISE EXCLUSION COLOR_BIAS", backfaceCulling = 1, instanceLimit = 2 }

resource "Shader" "OcclusionMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionBiasMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA NOISE", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseBiasMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA NOISE COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionExclusionMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA EXCLUSION", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionExclusionBiasMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA EXCLUSION COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseExclusionMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA NOISE EXCLUSION", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionNoiseExclusionBiasMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA NOISE EXCLUSION COLOR_BIAS", backfaceCulling = 1, instanceLimit = 1 }
resource "Shader" "OcclusionPlayerMSAA" { vs = "Occlusion_vs.cg", fs = "Occlusion_fs.cg", defines = "MSAA PLAYER NOISE EXCLUSION COLOR_BIAS", backfaceCulling = 1, instanceLimit = 2 }




resource "Shader" "Fog" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", instanceLimit = 1 }
resource "Shader" "FogCrossfade" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", defines = "CROSSFADE", instanceLimit = 1 }
resource "Shader" "FogMSAA" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", defines = "MSAA CROSSFADE", instanceLimit = 1 }
resource "Shader" "FogCrossfadeMSAA" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", defines = "MSAA CROSSFADE", instanceLimit = 1 }

resource "Shader" "FogAlpha" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", instanceLimit = 1 }
resource "Shader" "FogCrossfadeAlpha" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", defines = "CROSSFADE", instanceLimit = 1 }
resource "Shader" "FogMSAAAlpha" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", defines = "MSAA CROSSFADE", instanceLimit = 1 }
resource "Shader" "FogCrossfadeMSAAAlpha" { vs = "Fog_vs.cg", fs = "Fog_fs.cg", defines = "MSAA CROSSFADE", instanceLimit = 1 }



resource "Shader" "DownSampleSet" { vs = "Resample_vs.cg", fs = "Resample_fs.cg", instanceLimit = 20 }
resource "Shader" "DownSampleSetAlpha" { vs = "Resample_vs.cg", fs = "Resample_fs.cg", instanceLimit = 20 }
resource "Shader" "PointSampleSet" { vs = "Resample_vs.cg", fs = "Resample_fs.cg", defines = "POINT_SAMPLED", instanceLimit = 16 }
resource "Shader" "DownSampleTrapNan" { vs = "Resample_vs.cg", fs = "Resample_fs.cg", defines = "TRAP_NAN", instanceLimit = 1 }
resource "Shader" "DownSampleDepth" { vs = "Resample_vs.cg", fs = "DepthDownsample_fs.cg", instanceLimit = 8 }

resource "Shader" "DownSampleSetMSAA" { vs = "Resample_vs.cg", fs = "Resample_fs.cg", defines="MSAA", instanceLimit = 20 }
resource "Shader" "DownSampleDepthMSAA" { vs = "Resample_vs.cg", fs = "DepthDownsample_fs.cg", defines="MSAA", instanceLimit = 8 }

resource "Shader" "CompositeTonemap" { vs = "Resample_vs.cg", fs = "CompositeTonemap_fs.cg", instanceLimit = 1 }
resource "Shader" "CompositeTonemapMSAA" { vs = "Resample_vs.cg", fs = "CompositeTonemap_fs.cg", defines="MSAA", instanceLimit = 1 }
resource "Shader" "CompositeTonemapAlpha" { vs = "Resample_vs.cg", fs = "CompositeTonemap_fs.cg", instanceLimit = 1 }
resource "Shader" "CompositeTonemapMSAAAlpha" { vs = "Resample_vs.cg", fs = "CompositeTonemap_fs.cg", defines="MSAA", instanceLimit = 1 }

resource "Shader" "Clear" { vs = "Resample_vs.cg", fs = "Clear_fs.cg", instanceLimit = 3 }
resource "Shader" "LowResEdges" { vs = "Resample_vs.cg", fs = "LowResEdges_fs.cg", instanceLimit = 1 }
resource "Shader" "ParticleComposite" { vs = "Resample_vs.cg", fs = "ParticleComposite_fs.cg", defines = "DISCARD", lodDefines = "BLACK", stencil = 80, instanceLimit = 1 }
resource "Shader" "HeatDistortion" { vs = "HeatDistortion_vs.cg", fs = "HeatDistortion_fs.cg", instanceLimit = 1 }
resource "Shader" "NearDofSet" { vs = "Resample_vs.cg", fs = "NearDofSet_fs.cg", instanceLimit = 1 }
resource "Shader" "NearDofBlur" { vs = "Resample_vs.cg", fs = "NearDofBlur_fs.cg", instanceLimit = 1 }
resource "Shader" "NearDofComposite" { vs = "Resample_vs.cg", fs = "NearDofComposite_fs.cg", instanceLimit = 1 }
resource "Shader" "MotionBlurDofNop" { vs = "Resample_vs.cg", fs = "MotionBlurDof_fs.cg", defines="", instanceLimit = 1 }
resource "Shader" "MotionBlur" { vs = "Resample_vs.cg", fs = "MotionBlurDof_fs.cg", defines="MOTION_BLUR", instanceLimit = 1 }
resource "Shader" "Dof1" { vs = "Resample_vs.cg", fs = "MotionBlurDof_fs.cg", defines="DOF1", instanceLimit = 1 }
resource "Shader" "Dof2" { vs = "Resample_vs.cg", fs = "MotionBlurDof_fs.cg", defines="DOF2", instanceLimit = 1 }
resource "Shader" "MotionBlurDof1" { vs = "Resample_vs.cg", fs = "MotionBlurDof_fs.cg", defines="MOTION_BLUR DOF1", instanceLimit = 1 }
resource "Shader" "MotionBlurDof2" { vs = "Resample_vs.cg", fs = "MotionBlurDof_fs.cg", defines="MOTION_BLUR DOF2", instanceLimit = 1 }
resource "Shader" "BloomDownsample" { vs = "Resample_vs.cg", fs = "BloomDownsample_fs.cg", instanceLimit = 12 }
resource "Shader" "BloomPass" { vs = "Resample_vs.cg", fs = "BloomPass_fs.cg", instanceLimit = 12 }
resource "Shader" "BloomBlur16" { vs = "Resample_vs.cg", fs = "BloomBlur_fs.cg", defines="TAP_16", instanceLimit = 4 }
resource "Shader" "BloomBlur8" { vs = "Resample_vs.cg", fs = "BloomBlur_fs.cg", defines="TAP_8", instanceLimit = 4 }
resource "Shader" "BloomBlur4" { vs = "Resample_vs.cg", fs = "BloomBlur_fs.cg", defines="TAP_4", instanceLimit = 4 }
resource "Shader" "BloomBlurComposite16" { vs = "Resample_vs.cg", fs = "BloomBlur_fs.cg", defines="COMPOSITE TAP_16", instanceLimit = 4 }
resource "Shader" "BloomBlurComposite8" { vs = "Resample_vs.cg", fs = "BloomBlur_fs.cg", defines="COMPOSITE TAP_8", instanceLimit = 4 }
resource "Shader" "BloomBlurComposite4" { vs = "Resample_vs.cg", fs = "BloomBlur_fs.cg", defines="COMPOSITE TAP_4", instanceLimit = 4 }
resource "Shader" "Scanout" { vs = "Resample_vs.cg", fs = "Scanout_fs.cg", instanceLimit = 1 }
resource "Shader" "Fxaa" { vs = "Resample_vs.cg", fs = "Fxaa_fs.cg", instanceLimit = 1 }
resource "Shader" "Deband" { vs = "Resample_vs.cg", fs = "deband_fs.cg", instanceLimit = 1 }
resource "Shader" "LensDistortionOffset" { vs = "Resample_vs.cg", fs = "LensDistortionOffset_fs.cg", instanceLimit = 1 }

-- !MP! my debug shaders
resource "Shader" "ShowTexture2D" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", instanceLimit=16 }
resource "Shader" "ShowTexture2DAlpha" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_TEX2D_ALPHA", instanceLimit=16 }
resource "Shader" "ShowTexture3D" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_UVW TEXTURE_3D", instanceLimit=16 }
resource "Shader" "ShowTextureCube" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_UVW", instanceLimit=16 }
resource "Shader" "ShowTextureDepth" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_DEPTH", instanceLimit=16 }
resource "Shader" "ShowTextureStencil" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_STENCIL", instanceLimit=16 }
resource "Shader" "ShowTextureAlpha" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_ALPHA", instanceLimit=16 }
resource "Shader" "ShowTextureRed" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_RED", instanceLimit=16 }
--resource "Shader" "ShowTextureGreen" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_GREEN", instanceLimit=16 }
--resource "Shader" "ShowTextureBlue" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_BLUE", instanceLimit=16 }
--resource "Shader" "ShowTextureRedDiff" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_DIFF_RED", instanceLimit=16 }
--resource "Shader" "ShowTextureRedWithDelta" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_RED_DELTA", instanceLimit=16 }
resource "Shader" "ShowTextureRedLevel" { vs="Resample_vs.cg", fs="ShowTexture_fs.cg", defines="TEXTURE_RED_LEVEL", instanceLimit=16 }

resource "Shader" "SandTextureSpriteBlit" { vs="SandTextureSprite_vs.cg", fs="SandTextureSprite_fs.cg", defines="", backfaceCulling=0, instanceLimit=16 }
resource "Shader" "SandTextureSpriteMip" { vs="SandTextureMipmap_vs.cg", fs="SandTextureMipmap_fs.cg", defines="", backfaceCulling=0, instanceLimit=16 }

resource "Shader" "PerformanceTest" { vs="SandTextureSprite_vs.cg", fs="SandTextureSprite_fs.cg", defines="PERFTEST", backfaceCulling=0, instanceLimit=16 }

resource "Shader" "MenuProjection" { vs="Resample_vs.cg", fs="MenuProject_fs.cg", defines="", instanceLimit=16 }

resource "Script" "Render" { source = "Render.lua" }
resource "Script" "Hud" { source = "Hud.lua" }
resource "Script" "Font" { source = "Font.lua" }
resource "Script" "fontios" { source = "fontios.lua" }
resource "Script" "HullBarn" { source = "HullBarn.lua" }
resource "Script" "TerrainData" { source = "TerrainData.lua" }
resource "Script" "Environment" { source = "Environment.lua" }
resource "Script" "ParticleDefs" { source = "ParticleDefs.lua" }
resource "Script" "ParticleBarn" { source = "ParticleBarn.lua" }
resource "Script" "DecorationMeshes" { source = "DecorationMeshes.lua" }
resource "Script" "SignBarn" { source = "SignBarn.lua" }
resource "Script" "FadeToWhite" { source = "FadeToWhite.lua" }
resource "Script" "FadeToBlack" { source = "FadeToBlack.lua" }
resource "Script" "EnvFX" { source = "EnvFX.lua" }
resource "Script" "EventSetter" { source = "EventSetter.lua" }
resource "Script" "TimelineBarn" { source = "TimelineBarn.lua" }
resource "Script" "ClumpLoader" { source = "ClumpLoader.lua" }
resource "Script" "CreatureLoader" { source = "Creatures.lua" }
resource "Script" "SoundBarn" { source = "SoundBarn.lua" }
resource "Script" "RailLoader" { source = "RailBarn.lua" }
resource "Script" "Markers" { source = "Markers.lua" }
resource "Script" "DudeAnimation" { source = "DudeAnimation.lua" }
resource "Script" "JetLoader" { source = "Jets.lua" }
resource "Script" "SoundEffectData" { source = "SoundEffectData.lua" }
resource "Script" "ReverbBarn" { source = "ReverbBarn.lua" }

resource "FileTracker" "ReverbPresets" { filename = "Sounds/Reverb/JourneyReverbRuntime.ini" }

-- Simple general textures
wad "TextureWad" "Utility"
{ wadresource "Texture" "Black" { source = "Black.dds", category = "Utility" }
, wadresource "Texture" "Blank" { source = "Blank.dds", category = "Utility" }
, wadresource  "Texture" "White" { source = "White.dds", category = "Utility" }
, wadresource  "Texture" "Grey" { source = "Grey.dds", category = "Utility" }
, wadresource  "Texture" "DarkGrey" { source = "DarkGrey.dds", category = "Utility" }
, wadresource  "Texture" "Shadow" { source = "Shadow.dds", category = "Utility" }
, wadresource  "Texture" "Grid" { source = "Grid.dds", filter = "aniso16", category = "Utility" }
, wadresource  "Texture" "LightGrey" { source = "LightGrey.dds", category = "Utility" }
, wadresource  "Texture" "SandMask" { source = "SandMask.dds", filter = "aniso16", category = "Utility" }
, wadresource  "Texture" "ClothMask" { source = "ClothMask.dds", filter = "aniso16", category = "Utility" }
, wadresource  "Texture" "WhiteCube" { source = "WhiteCube.dds", category = "Utility" }

--, wadresource  "Texture" "SandPass" { source = "SandPass.dds", filter = "aniso16", category = "Mesh" }
}

wad "TextureWad" "Noise"
{ wadresource  "Texture" "NoiseWhite" { source = "NoiseHF.dds", filter = "aniso16", category = "Noise" }
, wadresource  "Texture" "BlurredNoise" { source = "noise.dds", category = "Noise" }
, wadresource  "Texture" "NoiseSmall" { source = "NoiseSmall.dds", compression = "none", category = "Noise" }
, wadresource  "Texture" "NoiseSand" { source = "NoiseSand.dds", compression = "none", filter = "near16", category = "Noise" }
, wadresource  "Texture" "NoiseMotion" { source = "NoiseMotion.dds", compression = "none", filter = "near16", category = "Noise" }
, wadresource  "Texture" "Noise3d" { source = "Noise3d.dds", compression = "none", category = "Noise" }
, wadresource  "Texture" "Noise3dSmall" { source = "Noise3dSmall.dds", compression = "none", category = "Noise" }
}

-- Hud elements
wad "TextureWad" "Hud"
{ wadresource "Texture" "LoadingIndicator" { source = "LoadingIndicator.dds", category = "UI" }
, wadresource "Texture" "Menu_Vignette" { source = "Menu_Vignette.dds", category = "UI" }
, wadresource "Texture" "Menu_VignetteWhite" { source = "Menu_VignetteWhite.dds", category = "UI" }
, wadresource "Texture" "Menu_Controller" { source = "Menu_Controller.dds", category = "UI" }
, wadresource "Texture" "Menu_TiltFront" { source = "Menu_TiltFront.dds", category = "UI" }
, wadresource "Texture" "Menu_TiltFrontL" { source = "Menu_TiltFrontL.dds", category = "UI" }
, wadresource "Texture" "Menu_TiltFrontR" { source = "Menu_TiltFrontR.dds", category = "UI" }
, wadresource "Texture" "Menu_TiltSide" { source = "Menu_TiltSide.dds", category = "UI" }
, wadresource "Texture" "Menu_LeftStick" { source = "Menu_LeftStick.dds", category = "UI" }
, wadresource "Texture" "Menu_Shout" { source = "Menu_Shout.dds", category = "UI" }
, wadresource "Texture" "Nametags" { source = "Nametags.dds", category = "UI" }
}


-- Most in-game text
if "JP" == REGION then
	resource "Texture" "JourneyTitle"
		"japanese" { source = "JourneyTitle_Japanese.dds", category = "UI" }
		{ source = "JourneyTitle.dds", category = "UI" }
	resource "Texture" "Menu_Logo"
		"japanese" { source = "Menu_Logo_Japanese.dds", category = "UI" }
		{ source = "Menu_Logo.dds", category = "UI" }
	resource "Texture" "Menu_Circle"
		"japanese" { source = "Menu_Circle_Japanese.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Circle.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_CrossHold"
		"japanese" { source = "Menu_CrossHold_Japanese.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_CrossHold.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_Strings" 
		"japanese" { source = "Menu_Strings_Japanese.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Strings.dds", category = "UI", maxMipLevel = 0 }
elseif "AS" == REGION then
	resource "Texture" "JourneyTitle" { source = "JourneyTitle.dds", category = "UI" }
	resource "Texture" "Menu_Logo" { source = "Menu_Logo.dds", category = "UI" }
	resource "Texture" "Menu_Circle"
		"chinese_t" { source = "Menu_Circle_Chinese.dds", category = "UI", maxMipLevel = 0 }
		"korean" { source = "Menu_Circle_Korean.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Circle.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_CrossHold"
		"chinese_t" { source = "Menu_CrossHold_Chinese.dds", category = "UI", maxMipLevel = 0 }
		"korean" { source = "Menu_CrossHold_Korean.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_CrossHold.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_Strings" 
		"chinese_t" { source = "Menu_Strings_Trad_Chinese.dds", category = "UI", maxMipLevel = 0 }
		"korean" { source = "Menu_Strings_Korean.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Strings.dds", category = "UI", maxMipLevel = 0 }
elseif "CH" == REGION then
	resource "Texture" "JourneyTitle"
		"chinese_s" { source = "JourneyTitle_SC.dds", category = "UI" }
		{ source = "JourneyTitle.dds", category = "UI" }
	resource "Texture" "Menu_Logo"
		"chinese_s" { source = "Menu_Logo_SC.dds", category = "UI" }
		{ source = "Menu_Logo.dds", category = "UI" }
	resource "Texture" "Menu_Circle"
		"chinese_t" { source = "Menu_Circle_Chinese.dds", category = "UI", maxMipLevel = 0 }
		"chinese_s" { source = "Menu_Circle_SC.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Circle.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_CrossHold"
		"chinese_t" { source = "Menu_CrossHold_Chinese.dds", category = "UI", maxMipLevel = 0 }
		"chinese_s" { source = "Menu_CrossHold_SC.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_CrossHold.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_Strings" 
		"chinese_t" { source = "Menu_Strings_Trad_Chinese.dds", category = "UI", maxMipLevel = 0 }
		"chinese_s" { source = "Menu_Strings_SC.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Strings.dds", category = "UI", maxMipLevel = 0 }
elseif "UA" == REGION then
	resource "Texture" "JourneyTitle" { source = "JourneyTitle.dds", category = "UI" }
	resource "Texture" "Menu_Logo" { source = "Menu_Logo.dds", category = "UI" }
	resource "Texture" "Menu_Circle"
		"french" { source = "Menu_Circle_French.dds", category = "UI", maxMipLevel = 0 }
		"portuguesebr" { source = "Menu_Circle_Portuguese.dds", category = "UI", maxMipLevel = 0 }
		"spanishla" { source = "Menu_Circle_Spanish.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Circle.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_CrossHold"
		"french" { source = "Menu_CrossHold_French.dds", category = "UI", maxMipLevel = 0 }
		"portuguesebr" { source = "Menu_CrossHold_Portuguese.dds", category = "UI", maxMipLevel = 0 }
		"spanishla" { source = "Menu_CrossHold_Spanish.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_CrossHold.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_Strings"
		"french" { source = "Menu_Strings_French.dds", category = "UI", maxMipLevel = 0 }
		"portuguesebr" { source = "Menu_Strings_PortugueseBr.dds", category = "UI", maxMipLevel = 0 }
		"spanishla" { source = "Menu_Strings_SpanishLA.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Strings.dds", category = "UI", maxMipLevel = 0 }
else
	resource "Texture" "JourneyTitle"
		"polish" { source = "JourneyTitle_Polish.dds", category = "UI" }
		"russian" { source = "JourneyTitle_Russian.dds", category = "UI" }
		{ source = "JourneyTitle.dds", category = "UI" }
	resource "Texture" "Menu_Logo"
		"polish" { source = "Menu_Logo_Polish.dds", category = "UI" }
		"russian" { source = "Menu_Logo_Russian.dds", category = "UI" }
		{ source = "Menu_Logo.dds", category = "UI" }
	resource "Texture" "Menu_Circle"
		"danish" { source = "Menu_Circle_Danish.dds", category = "UI", maxMipLevel = 0 }
		"dutch" { source = "Menu_Circle_Dutch.dds", category = "UI", maxMipLevel = 0 }
		"finnish" { source = "Menu_Circle_Finnish.dds", category = "UI", maxMipLevel = 0 }
		"french" { source = "Menu_Circle_French.dds", category = "UI", maxMipLevel = 0 }
		"german" { source = "Menu_Circle_German.dds", category = "UI", maxMipLevel = 0 }
		"italian" { source = "Menu_Circle_Italian.dds", category = "UI", maxMipLevel = 0 }
		"norwegian" { source = "Menu_Circle_Norwegian.dds", category = "UI", maxMipLevel = 0 }
		"polish" { source = "Menu_Circle_Polish.dds", category = "UI", maxMipLevel = 0 }
		"portuguese" { source = "Menu_Circle_Portuguese.dds", category = "UI", maxMipLevel = 0 }
		"russian" { source = "Menu_Circle_Russian.dds", category = "UI", maxMipLevel = 0 }
		"spanish" { source = "Menu_Circle_Spanish.dds", category = "UI", maxMipLevel = 0 }
		"swedish" { source = "Menu_Circle_Swedish.dds", category = "UI", maxMipLevel = 0 }
		"turkish" { source = "Menu_Circle_Turkish.dds", category = "UI", maxMipLevel = 0 }
		"arabic" { source = "Menu_Circle_Arabic.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Circle.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_CrossHold"
		"danish" { source = "Menu_CrossHold_Danish.dds", category = "UI", maxMipLevel = 0 }
		"dutch" { source = "Menu_CrossHold_Dutch.dds", category = "UI", maxMipLevel = 0 }
		"finnish" { source = "Menu_CrossHold_Finnish.dds", category = "UI", maxMipLevel = 0 }
		"french" { source = "Menu_CrossHold_French.dds", category = "UI", maxMipLevel = 0 }
		"german" { source = "Menu_CrossHold_German.dds", category = "UI", maxMipLevel = 0 }
		"italian" { source = "Menu_CrossHold_Italian.dds", category = "UI", maxMipLevel = 0 }
		"norwegian" { source = "Menu_CrossHold_Norwegian.dds", category = "UI", maxMipLevel = 0 }
		"polish" { source = "Menu_CrossHold_Polish.dds", category = "UI", maxMipLevel = 0 }
		"portuguese" { source = "Menu_CrossHold_Portuguese.dds", category = "UI", maxMipLevel = 0 }
		"russian" { source = "Menu_CrossHold_Russian.dds", category = "UI", maxMipLevel = 0 }
		"spanish" { source = "Menu_CrossHold_Spanish.dds", category = "UI", maxMipLevel = 0 }
		"swedish" { source = "Menu_CrossHold_Swedish.dds", category = "UI", maxMipLevel = 0 }
		"turkish" { source = "Menu_CrossHold_Turkish.dds", category = "UI", maxMipLevel = 0 }
		"arabic" { source = "Menu_CrossHold_Arabic.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_CrossHold.dds", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_Strings" 
		"danish" { source = "Menu_Strings_Danish.dds", category = "UI", maxMipLevel = 0 }
		"dutch" { source = "Menu_Strings_Dutch.dds", category = "UI", maxMipLevel = 0 }
		"finnish" { source = "Menu_Strings_Finnish.dds", category = "UI", maxMipLevel = 0 }
		"french" { source = "Menu_Strings_French.dds", category = "UI", maxMipLevel = 0 }
		"german" { source = "Menu_Strings_German.dds", category = "UI", maxMipLevel = 0 }
		"italian" { source = "Menu_Strings_Italian.dds", category = "UI", maxMipLevel = 0 }
		"norwegian" { source = "Menu_Strings_Norwegian.dds", category = "UI", maxMipLevel = 0 }
		"polish" { source = "Menu_Strings_Polish.dds", category = "UI", maxMipLevel = 0 }
		"portuguese" { source = "Menu_Strings_Portuguese.dds", category = "UI", maxMipLevel = 0 }
		"russian" { source = "Menu_Strings_Russian.dds", category = "UI", maxMipLevel = 0 }
		"spanish" { source = "Menu_Strings_Spanish.dds", category = "UI", maxMipLevel = 0 }
		"swedish" { source = "Menu_Strings_Swedish.dds", category = "UI", maxMipLevel = 0 }
		"turkish" { source = "Menu_Strings_Turkish.dds", category = "UI", maxMipLevel = 0 }
		"arabic" { source = "Menu_Strings_Arabic.dds", category = "UI", maxMipLevel = 0 }
		{ source = "Menu_Strings.dds", category = "UI", maxMipLevel = 0 }
end

-- Special upsell screens for the end of the kiosk demo
if KIOSK_VERSION then
	resource "Texture" "Menu_KioskUpsell0" { source = "Menu_KioskUpsell0.dds", compression = "none", category = "UI", maxMipLevel = 0 }
	resource "Texture" "Menu_KioskUpsell1" { source = "Menu_KioskUpsell1.dds", compression = "none", category = "UI", maxMipLevel = 0 }
end

-- GraveMeshBarn textures
resource "Texture" "SymbolGlow" { source = "SymbolGlow.dds", category = "AutoAssigned" }

-- Other textures ...
resource "Texture" "JohnFace" { source = "Blank.dds", category = "UI" }
resource "Texture" "Crosshair" { source = "Crosshair.dds", category = "UI" }

-- Environment
wad "TextureWad" "Environment0"
{ wadresource "Texture" "CloudScroll" { source = "CloudScroll.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "CloudStrip" { source = "CloudStrip.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "DesertClouds" { source = "DesertClouds.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "DesertCloudScroll" { source = "DesertCloudScroll.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "Mist1" { source = "Mist1.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "MistLowRes" { source = "MistLowRes.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "StormClouds" { source = "StormClouds.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "StormCloudsLowRes" { source = "StormCloudsLowRes.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "LightningClouds" { source = "LightningClouds.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "CloudMask" { source = "CloudMask.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "Stars" { source = "Stars.dds", category = "Environment" }
}
if not KIOSK_VERSION then
 wad "TextureWad" "Environment1"
 { wadresource "Texture" "SkyPattern" { source = "SkyPattern.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "SkyClouds" { source = "SkyClouds.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "SkyCloudScroll" { source = "SkyCloudScroll.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "FlowerSecret" { source = "FlowerSecret.dds", gammaRemap = true, filter = "aniso4", category = "Environment" }
 , wadresource "Texture" "PassByCloudDust" { source = "PassByCloudDust.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "Sparkles" { source = "Sparkles.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "Rainbow" { source = "Rainbow.dds", gammaRemap = true, category = "Environment" }
 , wadresource "Texture" "AncestorCard" { source = "AncestorCard.dds", gammaRemap = true, category = "Environment" }
 }
end

-- Cloth
wad "TextureWad" "Cloth"
{ wadresource "Texture" "ClothAtlas" { source = "ClothAtlas.dds", filter = "aniso4", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRed0" { source = "ClothPlayerAtlasRed0.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRed1" { source = "ClothPlayerAtlasRed1.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRed2" { source = "ClothPlayerAtlasRed2.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRed3" { source = "ClothPlayerAtlasRed3.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasWhite1" { source = "ClothPlayerAtlasWhite1.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasWhite2" { source = "ClothPlayerAtlasWhite2.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasWhite3" { source = "ClothPlayerAtlasWhite3.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerScarf" { source = "ClothPlayerScarf.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerScarfFilled" { source = "ClothPlayerScarfFilled.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPainting" { source = "ClothPainting.dds", filter = "aniso4", gammaRemap = true, category = "Cloth" }
}
wad "TextureWad" "ClothGlow"
{ wadresource "Texture" "ClothAtlasGlow" { source = "ClothAtlasGlow.dds", filter = "aniso4", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRedGlow0" { source = "ClothPlayerAtlasRedGlow0.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRedGlow1" { source = "ClothPlayerAtlasRedGlow1.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRedGlow2" { source = "ClothPlayerAtlasRedGlow2.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasRedGlow3" { source = "ClothPlayerAtlasRedGlow3.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasWhiteGlow1" { source = "ClothPlayerAtlasWhiteGlow1.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasWhiteGlow2" { source = "ClothPlayerAtlasWhiteGlow2.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasWhiteGlow3" { source = "ClothPlayerAtlasWhiteGlow3.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerScarfGlow" { source = "ClothPlayerScarfGlow.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerScarfGlowFilled" { source = "ClothPlayerScarfGlowFilled.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPaintingGlow" { source = "ClothPaintingGlow.dds", filter = "aniso4", gammaRemap = true, category = "Cloth" }
}
wad "TextureWad" "ClothMask"
{ wadresource "Texture" "ClothAtlasMask" { source = "ClothAtlasMask.dds", filter = "aniso4", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasMask1" { source = "ClothPlayerAtlasMask1.dds", filter = "aniso2", gammaRemap = false, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasMask3" { source = "ClothPlayerAtlasMask3.dds", filter = "aniso2", gammaRemap = false, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasMask5" { source = "ClothPlayerAtlasMask5.dds", filter = "aniso2", gammaRemap = false, category = "Cloth" }
, wadresource "Texture" "ClothPlayerAtlasMask6" { source = "ClothPlayerAtlasMask6.dds", filter = "aniso2", gammaRemap = false, category = "Cloth" }
, wadresource "Texture" "ClothPlayerScarfMask" { source = "ClothPlayerScarfMask.dds", filter = "aniso2", gammaRemap = false, category = "Cloth" }
, wadresource "Texture" "ClothPaintingChamMask" { source = "ClothPaintingChamMask.dds", filter = "aniso2", gammaRemap = true, category = "Cloth" }
, wadresource "Texture" "ClothDetail" { source = "ClothDetail.dds", filter = "aniso16", category = "Cloth" }
, wadresource "Texture" "ClothSpirit" { source = "ClothSpirit.dds", filter = "aniso4", category = "Cloth" }
}

--Mesh
wad "TextureWad" "Mesh0"
{ wadresource "Texture" "Spirit" { source = "Spirit.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "TempSpirit" { source = "TempSpirit.dds", category = "Mesh" }
, wadresource "Texture" "SpiritMaskNew" { source = "SpiritMaskNew.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "AncestorTombstone" { source = "AncestorTombstone.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "Sphinx" { source = "Sphinx.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "SphinxSymbol" { source = "SphinxSymbol.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "BackgroundMountain" { source = "BackgroundMountain.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "Sun" { source = "Sun.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "Dust" { source = "Dust.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "GraveyardRuin" { source = "GraveyardRuin.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "MountainLight" { source = "MountainLight.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "Rock" { source = "Rock.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "Stripes" { source = "Stripes.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "SquareTiles" { source = "SquareTiles.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "RoofTiles" { source = "RoofTiles.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
}
wad "TextureWad" "Mesh1"
{ wadresource "Texture" "DesertRampLight" { source = "DesertRampLight.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "RuinRamp" { source = "RuinRamp.dds", gammaRemap = true, category = "Light" }
--, wadresource "Texture" "SandTrail" { source = "SandTrail.dds", gammaRemap = true, category = "Environment" }
, wadresource "Texture" "Sandstone" { source = "Sandstone.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "Dirt" { source = "Dirt.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "BlueSandstone" { source = "BlueSandstone.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "PatternStone" { source = "PatternStone.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "GraveStone" { source = "GraveStone.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "SandFall1" { source = "SandFall1.dds", gammaRemap = false, category = "Mesh" }
, wadresource "Texture" "SandEdgeAlpha" { source = "SandEdgeAlpha.dds", category = "Mesh" }
, wadresource "Texture" "GuardianMech" { source = "GuardianMech.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "GuardianWing" { source = "GuardianWing.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "MechLight" { source = "MechLight.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "StoneGrate" { source = "StoneGrate.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" }
, wadresource "Texture" "PaintedColumn" { source = "PaintedColumn.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "DesertBackdropCard" { source = "DesertBackdropCard.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
}
wad "TextureWad" "Mesh2"
{ wadresource "Texture" "PaintedColumnDamaged" { source = "PaintedColumnDamaged.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "PaintedColumnDamaged1" { source = "PaintedColumnDamaged1.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "Warm" { source = "Warm.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "Grate" { source = "Grate.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" }
, wadresource "Texture" "GrateNick" { source = "GrateNick.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" }
, wadresource "Texture" "GrateMatt" { source = "GrateMatt.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" }
, wadresource "Texture" "GrateBar" { source = "GrateBar.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "AlphaRamp" { source = "AlphaRamp.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "DesertRamp" { source = "DesertRamp.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "DesertShadowRamp" { source = "DesertShadowRamp.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "DesertShadowRampPaintLight" { source = "DesertShadowRampPaintLight.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "BluePaintedColumn1" { source = "BluePaintedColumn1.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "GoldLightRamp" { source = "GoldLightRamp.dds", gammaRemap = false, compression = "none", category = "Mesh" }
, wadresource "Texture" "SandPiles" { source = "SandPiles.dds", filter = "aniso16", gammaRemap = true, category = "Mesh"}
, wadresource "Texture" "MechStatue" { source = "MechStatue.dds", gammaRemap = true, category = "Mesh" }
}
if not KIOSK_VERSION then
 wad "TextureWad" "Mesh3"
 { wadresource "Texture" "FishSwarm" { source = "FishSwarm.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "WhistleStone" { source = "WhistleStone.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "Rock1" { source = "Rock1.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "Symbols" { source = "Symbols.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "RuinPattern" { source = "RuinPattern.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "RuinPattern1" { source = "RuinPattern1.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "CityCardRing" { source = "CityCardRing.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "CityCardRing1" { source = "CityCardRing1.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "Moon" { source = "Moon.dds", gammaRemap = true, category = "Environment" }
 , wadresource "Texture" "WaterFall" { source = "WaterFall.dds", filter = "aniso16", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "BlueGraveyardRuin" { source = "BlueGraveyardRuin.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "SnowStone" { source = "SnowStone.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "RuinShape" { source = "RuinShape.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "Turquoise" { source = "Turquoise.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "WaterEdgeAlpha" { source = "WaterEdgeAlpha.dds", category = "Mesh" }
 , wadresource "Texture" "Water" { source = "Water.dds", category = "Mesh" }
 }
 wad "TextureWad" "Mesh4"
 { wadresource "Texture" "Ruin_LightRamp" { source = "Ruin_LightRamp.dds", gammaRemap = false, compression = "none", category = "Light" }
 , wadresource "Texture" "RuinPainting" { source = "RuinPainting.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "RuinPaintingCharacters" { source = "RuinPaintingCharacters.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "RuinPaintingSymbols" { source = "RuinPaintingSymbols.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "BluePaintedColumn" { source = "BluePaintedColumn.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "CloudScrollSmall" { source = "CloudScrollSmall.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" }
 , wadresource "Texture" "CloudScrollSmallDust" { source = "CloudScrollSmallDust.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" }
 , wadresource "Texture" "SymbolsSmall" { source = "SymbolsSmall.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
 , wadresource "Texture" "SnowPiles" { source = "SnowPiles.dds", gammaRemap = true, category = "Mesh", filter = "aniso16" } 
 }
end

-- Paintings
wad "TextureWad" "Paintings0"
{ wadresource "Texture" "RuinsPainting1" { source = "RuinsPainting1.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting2" { source = "RuinsPainting2.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting3" { source = "RuinsPainting3.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting4" { source = "RuinsPainting4.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting5" { source = "RuinsPainting5.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting6" { source = "RuinsPainting6.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting7" { source = "RuinsPainting7.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" } 
, wadresource "Texture" "RuinsPainting2_SP" { source = "RuinsPainting2_SP.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting3_SP" { source = "RuinsPainting3_SP.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting4_SP" { source = "RuinsPainting4_SP.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting5_SP" { source = "RuinsPainting5_SP.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "RuinsPainting7_SP" { source = "RuinsPainting7_SP.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" } 
, wadresource "Texture" "AncestorPainting" { source = "AncestorPainting.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
}
wad "TextureWad" "Paintings1"
{ wadresource "Texture" "MenuPainting1_L1" { source = "MenuPainting1_L1.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting1_L2" { source = "MenuPainting1_L2.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting1_L3" { source = "MenuPainting1_L3.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting1_L4" { source = "MenuPainting1_L4.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting2_L1" { source = "MenuPainting2_L1.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting2_L2" { source = "MenuPainting2_L2.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting2_L3" { source = "MenuPainting2_L3.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting2_L4" { source = "MenuPainting2_L4.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting3_L1" { source = "MenuPainting3_L1.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting3_L2" { source = "MenuPainting3_L2.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting3_L3" { source = "MenuPainting3_L3.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
}
wad "TextureWad" "Paintings2"
{ wadresource "Texture" "MenuPainting4_L1" { source = "MenuPainting4_L1.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting4_L2" { source = "MenuPainting4_L2.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting4_L3" { source = "MenuPainting4_L3.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting4_L4" { source = "MenuPainting4_L4.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 5 }
, wadresource "Texture" "MenuPainting5_L1" { source = "MenuPainting5_L1.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 1 }
, wadresource "Texture" "MenuPainting5_L2" { source = "MenuPainting5_L2.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 1 }
, wadresource "Texture" "MenuPainting5_L3" { source = "MenuPainting5_L3.dds", gammaRemap = true, category = "Mesh", filter = "aniso16", maxMipLevel = 1 }
}

-- Occlusion
wad "TextureWad" "Occlusion0"
{ wadresource "Texture" "O_ShrineRoom01" { source = "O_ShrineRoom01.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_GraveyardFinalStruct" { source = "O_GraveyardFinalStruct.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_GraveTower_Damaged1" { source = "O_GraveTower_Damaged1.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_Aqueduct" { source = "O_Aqueduct.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_AqueductSet" { source = "O_AqueductSet.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_AqueductFar" { source = "O_AqueductFar.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_GraveyardAqueduct" { source = "O_GraveyardAqueduct.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_GraveyardTower" { source = "O_GraveyardTower.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_GraveyardCenterTower" { source = "O_GraveyardCenterTower.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_GraveyardGate" { source = "O_GraveyardGate.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_FirstShrineRoom" { source = "O_FirstShrineRoom.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_FortressWall" { source = "O_FortressWall.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_CanyonSlowDownTower" { source = "O_CanyonSlowDownTower.dds", gammaRemap = true, category = "Mesh" }
}
wad "TextureWad" "Occlusion1"
{ wadresource "Texture" "O_RockBed" { source = "O_RockBed.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_RockBed2" { source = "O_RockBed2.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_RockBed3" { source = "O_RockBed3.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_M6Rock" { source = "O_M6Rock.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_RockWallBack" { source = "O_RockWallBack.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_CliffLeft1" { source = "O_CliffLeft1.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_CliffLeft2" { source = "O_CliffLeft2.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_CliffCenterFar" { source = "O_CliffCenterFar.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_CliffCenterLeft" { source = "O_CliffCenterLeft.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_CliffCenterRight" { source = "O_CliffCenterRight.dds", gammaRemap = true, category = "Light" }
, wadresource "Texture" "O_HallWay" { source = "O_HallWay.dds", gammaRemap = true, category = "Light" }
}
wad "TextureWad" "Occlusion2"
{ wadresource "Texture" "O_ClothLantern" { source = "O_ClothLantern.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_BarrensPlatform01" { source = "O_BarrensPlatform01.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_LeapTemple" { source = "O_LeapTemple.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_RuinTowerSideA" { source = "O_RuinTowerSideA.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_RuinTowerSideB" { source = "O_RuinTowerSideB.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_FinalDesertTower" { source = "O_FinalDesertTower.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_FinalDesertTower01" { source = "O_FinalDesertTower01.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_FinalDesertTower02" { source = "O_FinalDesertTower02.dds", gammaRemap = true, category = "Mesh" }
, wadresource "Texture" "O_RuinSmallWall01" { source = "O_RuinSmallWall01.dds", gammaRemap = true, category = "Mesh" }
, wadresource  "Texture" "BlurredNoiseWide" { source = "BlurredNoiseWide.dds", category = "Utility" }
, wadresource  "Texture" "BlurredNoiseTall" { source = "BlurredNoiseTall.dds", category = "Utility" }
, wadresource "Texture" "AncestorPaintingRibbon" { source = "AncestorPaintingRibbon.dds", gammaRemap = true, compression = "none", category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "AncestorPaintingCity1" { source = "AncestorPaintingCity1.dds", gammaRemap = true, category = "Mesh", filter = "aniso4" }
, wadresource "Texture" "AncestorPaintingDune" { source = "AncestorPaintingDune.dds", gammaRemap = true, compression = "none", category = "Mesh", filter = "aniso4" }
}
if not KIOSK_VERSION then
 wad "TextureWad" "Occlusion3"
 { wadresource "Texture" "O_SnowMountainA" { source = "O_SnowMountainA.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_MountainShelter" { source = "O_MountainShelter.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_MountainBridgeGate" { source = "O_MountainBridgeGate.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_SnowMountain7" { source = "O_SnowMountain7.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_GraveTowerSide" { source = "O_GraveTowerSide.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_PipeColumn" { source = "O_PipeColumn.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_BarrensPipe" { source = "O_BarrensPipe.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_GraveyardTemple" { source = "O_GraveyardTemple.dds", gammaRemap = true, category = "Light" }
 }
 wad "TextureWad" "Occlusion4"
 { wadresource "Texture" "O_Bridge" { source = "O_Bridge.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_CanyonBuilding" { source = "O_CanyonBuilding.dds", gammaRemap = true, category = "Mesh" }
 , wadresource "Texture" "O_RuinDivider" { source = "O_RuinDivider.dds", gammaRemap = true, category = "Mesh" }
 }
end

wad "TextureWad" "Tapestries"
{ wadresource "Texture" "Tapestry1" { source = "Tapestry1.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry2" { source = "Tapestry2.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry3" { source = "Tapestry3.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry4" { source = "Tapestry4.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry5" { source = "Tapestry5.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry6" { source = "Tapestry6.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry7" { source = "Tapestry7.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry8" { source = "Tapestry8.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry9" { source = "Tapestry9.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry10" { source = "Tapestry10.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
}

wad "TextureWad" "TapestryFigures"
{ wadresource "Texture" "Tapestry1a" { source = "Tapestry1a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry2a" { source = "Tapestry2a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry3a" { source = "Tapestry3a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry4a" { source = "Tapestry4a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry5a" { source = "Tapestry5a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry6a" { source = "Tapestry6a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry7a" { source = "Tapestry7a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry8a" { source = "Tapestry8a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry9a" { source = "Tapestry9a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
, wadresource "Texture" "Tapestry10a" { source = "Tapestry10a.dds", gammaRemap = true, filter = "aniso8", category = "Mesh" }
}

-- Characters
wad "TextureWad" "Ninja"
{ wadresource "Texture" "Ninja" { source = "ClothNinja.dds", gammaRemap = true, category = "Character", maxMipLevel = 3}
, wadresource "Texture" "NinjaGlow" { source = "ClothNinjaGlow.dds", gammaRemap = true, category = "Character", maxMipLevel = 3}
, wadresource "Texture" "NinjaMask" { source = "ClothNinjaMask.dds", gammaRemap = false, category = "Character", maxMipLevel = 3}
, wadresource "Texture" "NinjaWhite" { source = "ClothNinjaWhite.dds", gammaRemap = true, category = "Code" }
, wadresource "Texture" "NinjaWhiteGlow" { source = "ClothNinjaWhiteGlow.dds", gammaRemap = true, category = "Code" }
}

wad "TextureWad" "EULA"
{ wadresource "Texture" "Menu_EULA_Accept" { source = "Menu_EULA_Accept.dds", category = "Titles" }
, wadresource "Texture" "Menu_EULA_Resume" { source = "Menu_EULA_Resume.dds", category = "Titles" }
}

resource "Texture" "Subtitle" { source = "Blank.dds", category = "Titles" }

resource "Texture" "ParticleTexture" { source = "particleAtlas.dds", filter = "aniso8", category = "Particle" }

-- if "PublicBeta" ~= CONFIG and not KIOSK_VERSION then
--	resource "FileTracker" "TrophyPack" { filename = "../TROPDIR/NPXX51519_00/TROPHY.TRP" }
-- end

resource "FileTracker" "Footprint" { filename = "Textures/Footprint.dds" }
resource "FileTracker" "Trailprint" { filename = "Textures/Trailprint.dds" }
resource "FileTracker" "DunePattern" { filename = "Textures/DunePattern.dds" }
resource "FileTracker" "DunePatternLow" { filename = "Textures/DunePatternLow.dds" }
resource "FileTracker" "DunePatternMap" { filename = "Textures/DunePatternMap.dds" }

resource "FileTracker" "Credits" { filename = "Scripts/Credits.txt" }
resource "FileTracker" "Credits2" { filename = "Scripts/Credits2.txt" }
resource "FileTracker" "PS4Credits" { filename = "Scripts/PS4Credits.txt" }
resource "FileTracker" "PS4Credits2" { filename = "Scripts/PS4Credits2.txt" }

wad "MeshWad" "MeshWadSymbols0"
{ wadresource "Mesh" "Symbol1" { source = "Symbol1.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol2" { source = "Symbol2.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol3" { source = "Symbol3.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol4" { source = "Symbol4.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol5" { source = "Symbol5.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol6" { source = "Symbol6.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol7" { source = "Symbol7.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol8" { source = "Symbol8.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol9" { source = "Symbol9.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol10" { source = "Symbol10.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol11" { source = "Symbol11.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol12" { source = "Symbol12.PSSG", instanceLimit = 20 }
}

wad "MeshWad" "MeshWadSymbols1"
{ wadresource "Mesh" "Symbol13" { source = "Symbol13.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol14" { source = "Symbol14.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol15" { source = "Symbol15.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol16" { source = "Symbol16.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol17" { source = "Symbol17.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol18" { source = "Symbol18.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol19" { source = "Symbol19.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol20" { source = "Symbol20.PSSG", instanceLimit = 20 }
, wadresource "Mesh" "Symbol21" { source = "Symbol21.PSSG", instanceLimit = 20 }
}

wad "MeshWad" "MeshWadMisc"
{ wadresource "Mesh" "Chime"    { source = "Chime.PSSG", instanceLimit = 12 }
, wadresource "Mesh" "ShoutInverse" { source = "ShoutInverse.PSSG", instanceLimit = 32 }
, wadresource "Mesh" "Flag" { source = "Flag.PSSG", instanceLimit = 64 }
, wadresource "Mesh" "SymbolGlow" { source = "P_SymbolGlow.PSSG", instanceLimit = 128 + 64 }
, wadresource "Mesh" "P_SecretSymbol" { source = "P_SecretSymbol.PSSG", instanceLimit = 32 }
}
