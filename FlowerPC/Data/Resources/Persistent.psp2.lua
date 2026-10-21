-------------------------------------------------------------------------------
-- Notes:  Lists all the resources that are loaded at program startup and
--         remain loaded until program termination.
--
-- Author: John Edwards
-- Date C: 5-2-2007
-------------------------------------------------------------------------------

if TARGET=="PSP2" then

resource "ScreamBank" "SharSfx" { source = "Shared.bank" }

-- force razor hud settings into the build directory
-- TODO: Don't do this for release?
resource "FileTracker" "RazorSettings" { filename = "../hud_settings.ini" }

resource "Shader" "DebugLine" { vs = "DebugLine_vs.cg", fs = "DebugLine_fs.cg", instanceLimit = 4 }
resource "Shader" "Sprite" { vs = "Sprite_vs.cg", fs = "Sprite_fs.cg", instanceLimit = 4 }
resource "Shader" "Text" { vs = "Sprite_vs.cg", fs = "Text_fs.cg", instanceLimit = 4 }
resource "Shader" "TextManualAlpha" { vs = "Sprite_vs.cg", fs = "Text_fs.cg", defines="MANUAL_DEPTH_TEST", instanceLimit = 4 }
resource "Shader" "MeshPortal" { vs = "Portal_vs.cg", fs = "Portal_fs.cg", instanceLimit = 2 }
resource "Shader" "DownSampleSet" { vs = "Resample_vs.cg", fs = "Resample_fs.cg", instanceLimit = 12 }
resource "Shader" "GuiImage" { vs = "MenuObject_vs.cg", fs = "GuiImage_fs.cg", instanceLimit = 32 }
resource "Shader" "MenuObject" { vs = "MenuObject_vs.cg", fs = "MenuObject_fs.cg", instanceLimit = 140 }
resource "Shader" "MenuVideo" { vs = "MenuObject_vs.cg", fs = "MenuObject_fs.cg", instanceLimit = 2 }
resource "Shader" "GlareStoreSet" { vs = "Resample_vs.cg", fs = "GlareStore_fs.cg", instanceLimit = 1 }

-- shaders changed from PS3 version
resource "Shader" "Debris"					{ vs = "BP_GrassCommon_vs.cg",			fs = "BP_GrassDebris_fs.cg",		instanceLimit = 1,		defines="DEBRIS"			}
resource "Shader" "DebrisRipple"			{ vs = "BP_GrassCommon_vs.cg",			fs = "BP_GrassDebris_fs.cg",		instanceLimit = 1,		defines="DEBRIS RIPPLE"		}
resource "Shader" "FlowerBlur"				{ vs = "BP_FlowerBlur_vs.cg",			fs = "BP_FlowerBlur_fs.cg",			instanceLimit = 1									}
resource "Shader" "FlowerBB"				{ vs = "BP_FlowerBB_vs.cg",				fs = "BP_FlowerBB_fs.cg",			instanceLimit = 1									}
resource "Shader" "FlowerHalo"				{ vs = "BP_FlowerHalo_vs.cg",			fs = "BP_Glow_fs.cg",				instanceLimit = 1									}
resource "Shader" "Grass"					{ vs = "BP_GrassCommon_vs.cg",			fs = "BP_GrassBlade_fs.cg",			instanceLimit = 1									}
resource "Shader" "GrassRipple"				{ vs = "BP_GrassCommon_vs.cg",			fs = "BP_GrassBlade_fs.cg",			instanceLimit = 1,		defines="RIPPLE"			}
resource "Shader" "MeshTerrain"				{ vs = "BP_MeshTerrain_vs.cg",			fs = "BP_MeshTerrain_fs.cg",		instanceLimit = 16									}
resource "Shader" "MotionBlur"				{ vs = "BP_MotionBlur_vs.cg",			fs = "BP_MotionBlur_fs.cg",			instanceLimit = 16									}
resource "Shader" "Particle"				{ vs = "BP_Particle_vs.cg",				fs = "BP_Particle_fs.cg",			instanceLimit = 4									}
resource "Shader" "AdditiveParticle"		{ vs = "BP_Particle_vs.cg",				fs = "BP_ParticleAdditive_fs.cg",	instanceLimit = 5,		defines="ADDITIVE"			}
resource "Shader" "SubtractiveParticle"		{ vs = "BP_Particle_vs.cg",				fs = "BP_ParticleAdditive_fs.cg",	instanceLimit = 5,		defines="ADDITIVE PREMULTALPHA"			}
resource "Shader" "ParticleSoft"			{ vs = "BP_ParticleSoft_vs.cg",			fs = "BP_ParticleSoft_fs.cg",		instanceLimit = 4									}
resource "Shader" "PetalInstance"			{ vs = "BP_Flower_vs.cg",				fs = "BP_Flower_fs.cg",				instanceLimit = 90,		defines="DISCARD"			}	-- 30 meshes, avg 3 anim frames
resource "Shader" "PetalInstanceSolid"		{ vs = "BP_Flower_vs.cg",				fs = "BP_Flower_fs.cg",				instanceLimit = 16									}	-- not many solid petal instances
resource "Shader" "PetalSwarm"				{ vs = "BP_swarm_vs.cg",				fs = "BP_swarm_fs.cg",				instanceLimit = 8,		defines="DISCARD"			}	-- only 8 possible petals in the swarm
resource "Shader" "PetalSwarmSolid"			{ vs = "BP_swarm_vs.cg",				fs = "BP_swarm_fs.cg",				instanceLimit = 8									}	-- only 8 possible petals in the swarm
resource "Shader" "PetalSwarmBurn"			{ vs = "BP_swarm_vs.cg",				fs = "BP_swarm_fs.cg",				instanceLimit = 8,		defines="DISCARD BURN"		}	-- only 8 possible petals in the swarm
resource "Shader" "Scattering"				{ vs = "BP_Scattering_vs.cg",			fs = "BP_Scattering_fs.cg",			instanceLimit = 16									}
resource "Shader" "ScatteringUvOffset"		{ vs = "BP_Scattering_vs.cg",			fs = "BP_Scattering_fs.cg",			instanceLimit = 16,		defines="UV_OFFSET"			}
resource "Shader" "StemInstance"			{ vs = "BP_FlowerStem_vs.cg",			fs = "BP_Flower_fs.cg",				instanceLimit = 5									}	-- 5 stem LODs
resource "Shader" "Sun"						{ vs = "BP_Sun_vs.cg",					fs = "BP_Sun_fs.cg",				instanceLimit = 2,		defines="MSAA SORT NOZWRITE"	}
resource "Shader" "Skybox"					{ vs = "BP_Skybox_vs.cg",				fs = "BP_Skybox_fs.cg",				instanceLimit = 2									}
resource "Shader" "TerrainNone"				{ vs = "BP_Terrain_vs.cg",				fs = "BP_Terrain_fs.cg",			instanceLimit = 64									}
resource "Shader" "TerrainR"				{ vs = "BP_Terrain_vs.cg",				fs = "BP_Terrain_fs.cg",			instanceLimit = 64,		defines="DETAIL_R"			}
resource "Shader" "TerrainRG"				{ vs = "BP_Terrain_vs.cg",				fs = "BP_Terrain_fs.cg",			instanceLimit = 64,		defines="DETAIL_RG"			}
resource "Shader" "TerrainMotionBlur"		{ vs = "BP_MotionBlur_vs.cg",			fs = "BP_MotionBlur_fs.cg",			instanceLimit = 64,		defines="TERRAIN"			}
resource "Shader" "TerrainTexture"			{ vs = "BP_TerrainTexture_vs.cg",		fs = "BP_TerrainTexture_fs.cg",		instanceLimit = 1									}
resource "Shader" "FillerFlower"			{ vs = "BP_GrassFlower_vs.cg",			fs = "BP_GrassFlower_fs.cg",		instanceLimit = 1								}
resource "Shader" "FillerFlowerRipple"		{ vs = "BP_GrassFlower_vs.cg",			fs = "BP_GrassFlower_fs.cg",		instanceLimit = 1,		defines="RIPPLE"		}
resource "Shader" "WindShow"				{ vs = "resample_vs.cg",				fs = "BP_WindShow_fs.cg",			instanceLimit = 1								}
resource "Shader" "RippleShow"				{ vs = "resample_vs.cg",				fs = "BP_RippleShow_fs.cg",			instanceLimit = 1								}
resource "Shader" "WindTexture"				{ vs = "BP_WindTexture_vs.cg",			fs = "BP_WindTexture_fs.cg",		instanceLimit = 1									}
resource "Shader" "RippleTexture"			{ vs = "BP_RippleTexture_vs.cg",		fs = "BP_RippleTexture_fs.cg",		instanceLimit = 2									}
resource "Shader" "RippleBorder"			{ vs = "BP_RippleBorder_vs.cg",			fs = "BP_RippleBorder_fs.cg",		instanceLimit = 1,		defines="SORT"				}	-- sort here makes it draw after rippleTexture

---------------------------------------------------------------------
---------------------------------------------------------------------
-- mesh shaders
-- mesh shader defines:
-- DISCARD : use alpha test
-- TRANSPARENT : needs blending
-- OPAQUE: shader is for opaque geometry only - there should be a non-opaque shader with the same name, with T appended to the name
-- CHAMELEON: chameleon effect in process (should have cham in its name)
---- CHAM0: Chameleon effect fully off - same name as above, but cham0 instead of cham
---- CHAM2: Chameleon effect fully on - same name as above, but cham1 instead of cham
-- BURN: burn effect in effect
---- BURN0: Burn effect fully off - same name as above, but burn0 instead of burn
---------------------------------------------------------------------
-- skinned shaders
resource "Shader" "MeshChamSkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON SKIN", instanceLimit = 1 }
resource "Shader" "MeshCham0Skin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 SKIN", instanceLimit = 1 }
resource "Shader" "MeshCham1Skin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 SKIN", instanceLimit = 1 }

resource "Shader" "MeshChamCubeSkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON CUBEMAP SKIN", instanceLimit = 4 }
resource "Shader" "MeshCham0CubeSkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 CUBEMAP SKIN", instanceLimit = 4 }
resource "Shader" "MeshCham1CubeSkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 CUBEMAP SKIN", instanceLimit = 4 }

resource "Shader" "MeshChamShySkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON SHY SKIN", instanceLimit = 8 }
resource "Shader" "MeshCham0ShySkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 SHY SKIN", instanceLimit = 8 }
resource "Shader" "MeshCham1ShySkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 SHY SKIN", instanceLimit = 8 }

---------------------------------------------------------------------
resource "Shader" "MeshTex" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="OPAQUE", instanceLimit = 4 }
resource "Shader" "MeshTexT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", instanceLimit = 4 }

resource "Shader" "MeshTexAlpha" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="ALPHA_IS_OPACITY DISCARD", instanceLimit = 48 }

resource "Shader" "MeshTexAlphaShy" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="ALPHA_IS_OPACITY SHY DISCARD", instanceLimit = 4 }

resource "Shader" "MeshTexCube" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CUBEMAP OPAQUE", instanceLimit = 256 }
resource "Shader" "MeshTexCubeT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CUBEMAP", instanceLimit = 256 }

resource "Shader" "MeshCham" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON OPAQUE", instanceLimit = 100 }
resource "Shader" "MeshCham0" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 OPAQUE", instanceLimit = 100 }
resource "Shader" "MeshCham1" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 OPAQUE", instanceLimit = 100 }
resource "Shader" "MeshChamT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON", instanceLimit = 100 }
resource "Shader" "MeshCham0T" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0", instanceLimit = 100 }
resource "Shader" "MeshCham1T" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1", instanceLimit = 100 }

resource "Shader" "MeshChamAlpha" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON TEX_ALPHA DISCARD", instanceLimit = 4 }
resource "Shader" "MeshCham0Alpha" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 TEX_ALPHA DISCARD", instanceLimit = 4 }
resource "Shader" "MeshCham1Alpha" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 TEX_ALPHA DISCARD", instanceLimit = 4 }

resource "Shader" "MeshChamCube" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON CUBEMAP OPAQUE", instanceLimit = 64 }
resource "Shader" "MeshCham0Cube" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 CUBEMAP OPAQUE", instanceLimit = 64 }
resource "Shader" "MeshCham1Cube" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 CUBEMAP OPAQUE", instanceLimit = 64 }
resource "Shader" "MeshChamCubeT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON CUBEMAP", instanceLimit = 64 }
resource "Shader" "MeshCham0CubeT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 CUBEMAP", instanceLimit = 64 }
resource "Shader" "MeshCham1CubeT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 CUBEMAP", instanceLimit = 64 }

resource "Shader" "MeshChamPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON POINT_LIGHT", instanceLimit = 6 }
resource "Shader" "MeshCham0Point" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 POINT_LIGHT", instanceLimit = 6 }
resource "Shader" "MeshCham1Point" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 POINT_LIGHT", instanceLimit = 6 }

resource "Shader" "MeshChamAlphaPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON TEX_ALPHA POINT_LIGHT DISCARD", instanceLimit = 4 }
resource "Shader" "MeshCham0AlphaPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 TEX_ALPHA POINT_LIGHT DISCARD", instanceLimit = 4 }
resource "Shader" "MeshCham1AlphaPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 TEX_ALPHA POINT_LIGHT DISCARD", instanceLimit = 4 }

resource "Shader" "MeshChamShy" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON SHY", instanceLimit = 8 }
resource "Shader" "MeshChamShyNoDither" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON SHY", instanceLimit = 2 }

-- burn
resource "Shader" "MeshTexBurn" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="BURN DISCARD", instanceLimit = 256 }			-- 242 in level 6
resource "Shader" "MeshTexBurn0" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="BURN0", instanceLimit = 256 }

-- haystacks
resource "Shader" "MeshChamAlphaPointShy" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON TEX_ALPHA POINT_LIGHT SHY DISCARD OPAQUE", instanceLimit = 6 }
resource "Shader" "MeshCham0AlphaPointShy" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 TEX_ALPHA POINT_LIGHT SHY DISCARD OPAQUE", instanceLimit = 6 }
resource "Shader" "MeshCham1AlphaPointShy" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 TEX_ALPHA POINT_LIGHT SHY DISCARD OPAQUE", instanceLimit = 6 }
resource "Shader" "MeshChamAlphaPointShyT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAMELEON TEX_ALPHA POINT_LIGHT SHY DISCARD", instanceLimit = 6 }
resource "Shader" "MeshCham0AlphaPointShyT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM0 TEX_ALPHA POINT_LIGHT SHY DISCARD", instanceLimit = 6 }
resource "Shader" "MeshCham1AlphaPointShyT" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="CHAM1 TEX_ALPHA POINT_LIGHT SHY DISCARD", instanceLimit = 6 }

-- wagon shader (level 4) -- non-skinned shader for PS3
resource "Shader" "MeshTexAlphaPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="ALPHA_IS_OPACITY POINT_LIGHT DISCARD", instanceLimit = 2 }
resource "Shader" "MeshTexPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="POINT_LIGHT", instanceLimit = 1 }
--resource "Shader" "MeshTexAlphaPointSkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="ALPHA_IS_OPACITY POINT_LIGHT DISCARD SKIN", instanceLimit = 1 }
resource "Shader" "MeshTexPointSkin" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="POINT_LIGHT SKIN", instanceLimit = 1 }


-- unused mesh shaders
--resource "Shader" "MeshTexBurnPoint" { vs = "BP_Mesh_vs.cg", fs = "BP_Mesh_fs.cg", defines="BURN POINT_LIGHT", instanceLimit = 32 }
---------------------------------------------------------------------
---------------------------------------------------------------------


resource "Shader" "BlurSet"					{ vs = "BP_Blur_vs.cg",					fs = "BP_Blur_fs.cg",				instanceLimit = 6,		defines="TAP7"				}
resource "Shader" "DofBloomComp"			{ vs = "BP_DofBloomComposite_vs.cg",	fs = "BP_DofBloomComposite_fs.cg",	instanceLimit = 1,		defines="COMPUTE_BLUR"		}
resource "Shader" "DepthBlurStoreSet"		{ vs = "BP_DepthBlurStore_vs.cg",		fs = "BP_DepthBlurStore_fs.cg",		instanceLimit = 1									}
resource "Shader" "DownSampleDepth"			{ vs = "BP_DepthDownsample_vs.cg",		fs = "BP_DepthDownsample_fs.cg",	instanceLimit = 4									}
resource "Shader" "RemoveMSAA"				{ vs = "BP_DepthDownsample_vs.cg",		fs = "BP_DepthDownsample_fs.cg",	instanceLimit = 4,		defines="TEXCOLOR"			}
resource "Shader" "GlareComposite"			{ vs = "BP_GlareComposite_vs.cg",		fs = "BP_GlareComposite_fs.cg",		instanceLimit = 2									}
resource "Shader" "Glow"					{ vs = "BP_Glow_vs.cg",					fs = "BP_Glow_fs.cg",				instanceLimit = 8									}
resource "Shader" "LensDistortion"			{ vs = "BP_LensDistortion_vs.cg",		fs = "BP_LensDistortion_fs.cg",		instanceLimit = 2									}
resource "Shader" "LensDistortionPain"		{ vs = "BP_LensDistortion_vs.cg",		fs = "BP_LensDistortion_fs.cg",		instanceLimit = 2,		defines="PAIN"				}
resource "Shader" "ShowTexture"				{ vs = "Resample_vs.cg",				fs = "Resample_fs.cg",				instanceLimit = 12									}


resource "Script" "Render" { source = "Render.lua" }
resource "Script" "Hud" { source = "Hud.lua" }
resource "Script" "DebugFont" { source = "DebugFont.lua" }
resource "Script" "GrassBarn" { source = "GrassBarn.lua" }
resource "Script" "HullBarn" { source = "HullBarn.lua" }
resource "Script" "FlowerMeshes" { source = "FlowerMeshes.lua" }
resource "Script" "FlowerDefs" { source = "FlowerDefs.lua" }
resource "Script" "FlowerCatalog" { source = "FlowerCatalog.lua" }
resource "Script" "Terrain" { source = "Terrain.lua" }
resource "Script" "TerrainData" { source = "TerrainData.lua" }
resource "Script" "Environment" { source = "Environment.lua" }
resource "Script" "SwarmLead" { source = "SwarmLead.lua" }
resource "Script" "ParticleDefs" { source = "ParticleDefs.lua" }
resource "Script" "LandingZones" { source = "LandingZones.lua" }
resource "Script" "AvatarJenova" { source = "AvatarJenova.lua" }
resource "Script" "DecorationMeshes" { source = "DecorationMeshes.lua" }
resource "Script" "SignBarn" { source = "SignBarn.lua" }
resource "Script" "MovieRender" { source = "MovieRender.lua" }
resource "Script" "FadeToWhite" { source = "FadeToWhite.lua" }
resource "Script" "FadeToBlack" { source = "FadeToBlack.lua" }
resource "Script" "FadeOut" { source = "FadeOut.lua" }
resource "Script" "EnvFX" { source = "EnvFX.lua" }
resource "Script" "MenuRender" { source = "MenuRender.lua" }
resource "Script" "EventSetter" {source = "EventSetter.lua" }
resource "Script" "Musician" {source = "Musician.lua" }
resource "Script" "ClumpLoader" {source = "ClumpLoader.lua" }
resource "Script" "ShockwaveLoader" { source = "Shockwaves.lua" }
resource "Script" "WindGuide" { source = "WindGuide.lua" }
resource "Script" "WaypointBarn" { source = "WaypointBarn.lua" }
resource "Script" "SoundBarn" { source = "SoundBarn.lua" }
resource "Script" "MovieBarn" { source = "MovieBarn.lua" }
resource "Script" "RailLoader" { source = "RailBarn.lua" }
resource "Script" "PetalRender" { source = "PetalRender.lua" }
resource "Script" "FadeSkipMovie" { source = "FadeSkipMovie.lua" }

resource "Texture" "Black" { source = "Black.dds" }
resource "Texture" "Blank" { source = "Blank.dds" }
resource "Texture" "White" { source = "White.dds" }
resource "Texture" "WhiteCube" { source = "WhiteCube.dds", cubemap=true }
resource "Texture" "BlackCube" { source = "BlackCube.dds", cubemap=true }
resource "Texture" "WhiteNoise" { source = "NoiseHF.dds" }
resource "Texture" "BlurredNoise" { source = "noise.dds" }
resource "Texture" "Grain1" { source = "grain1.dds" }
resource "Texture" "Grain2" { source = "grain2.dds" }
resource "Texture" "Grain3" { source = "grain3.dds" }

resource "Texture" "LoadingIndicator" { source = "LoadingIndicator.dds" }

--Menu Textures
resource "Texture" "Controls_Combined_Symbol" 
	"dutch" { source = "Controls_Combined_Symbol_dutch.dds", vram=false , nomipmap=true}
	"french" { source = "Controls_Combined_Symbol_french.dds", vram=false , nomipmap=true}
	"german" { source = "Controls_Combined_Symbol_german.dds", vram=false , nomipmap=true}
	"italian" { source = "Controls_Combined_Symbol_italian.dds", vram=false , nomipmap=true}
	"portuguese" { source = "Controls_Combined_Symbol_portugese.dds", vram=false , nomipmap=true}
	"russian" { source = "Controls_Combined_Symbol_russian.dds", vram=false , nomipmap=true}
	"spanish" { source = "Controls_Combined_Symbol_spanish.dds", vram=false , nomipmap=true}
	"japanese" { source = "Controls_Combined_Symbol_japanese.dds", vram=false , nomipmap=true}
	{ source = "Controls_Combined_Symbol.dds", vram=false, nomipmap=true }

if REGION:find( "JP" ) then
	resource "Texture" "Legal" { source = "LegalJapan.dds", vram=false }
elseif REGION:find( "EU" ) then
	resource "Texture" "Legal" { source = "LegalEurope.dds", vram=false }
else
	resource "Texture" "Legal" { source = "Legal.dds", vram=false }
end

resource "Texture" "Menu_flowera0" { source = "Menu_flowera0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_flowera1" { source = "Menu_flowera1.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowera2" { source = "Menu_flowera2.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerb0" { source = "Menu_flowerb0.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerb1" { source = "Menu_flowerb1.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerb2" { source = "Menu_flowerb2.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerc0" { source = "Menu_flowerc0.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerc1" { source = "Menu_flowerc1.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerc2" { source = "Menu_flowerc2.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerd0" { source = "Menu_flowerd0.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerd1" { source = "Menu_flowerd1.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerd2" { source = "Menu_flowerd2.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowere0" { source = "Menu_flowere0.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowere1" { source = "Menu_flowere1.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowere2" { source = "Menu_flowere2.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerf0" { source = "Menu_flowerf0.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerf1" { source = "Menu_flowerf1.dds", vram=false, nomipmap=true }
resource "Texture" "Menu_flowerf2" { source = "Menu_flowerf2.dds", vram=false, nomipmap=true }

if REGION:find( "JP" ) then
	resource "Texture" "Menu_demo_end" { source = "Menu_demo_end_japanese.dds", vram=false , nomipmap=true }
	resource "Texture" "Menu_demo_load" { source = "Menu_demo_load_japanese.dds", vram=false , nomipmap=true }
	resource "Texture" "Menu_demo_prompt" { source = "Menu_demo_prompt_japanese.dds", vram=false , nomipmap=true }
else
	resource "Texture" "Menu_demo_end"
		"dutch" { source = "Menu_demo_end_dutch.dds", vram=false, nomipmap=true }
		"french" { source = "Menu_demo_end_french.dds", vram=false, nomipmap=true }
		"german" { source = "Menu_demo_end_german.dds", vram=false, nomipmap=true }
		"italian" { source = "Menu_demo_end_italian.dds", vram=false, nomipmap=true }
		"portuguese" { source = "Menu_demo_end_portuguese.dds", vram=false, nomipmap=true }
		"russian" { source = "Menu_demo_end_russian.dds", vram=false, nomipmap=true }
		"spanish" { source = "Menu_demo_end_spanish.dds", vram=false, nomipmap=true }
		{ source = "Menu_demo_end.dds", vram=false, nomipmap=true }
	resource "Texture" "Menu_demo_load"
		"dutch" { source = "Menu_demo_load_dutch.dds", vram=false, nomipmap=true}
		"french" { source = "Menu_demo_load_french.dds", vram=false, nomipmap=true }
		"german" { source = "Menu_demo_load_german.dds", vram=false, nomipmap=true }
		"italian" { source = "Menu_demo_load_italian.dds", vram=false, nomipmap=true }
		"portuguese" { source = "Menu_demo_load_portuguese.dds", vram=false, nomipmap=true }
		"russian" { source = "Menu_demo_load_russian.dds", vram=false, nomipmap=true }
		"spanish" { source = "Menu_demo_load_spanish.dds", vram=false, nomipmap=true }
		{ source = "Menu_demo_load.dds", vram=false, nomipmap=true }
	resource "Texture" "Menu_demo_prompt"
		"dutch" { source = "Menu_demo_prompt_dutch.dds", vram=false, nomipmap=true}
		"french" { source = "Menu_demo_prompt_french.dds", vram=false, nomipmap=true }
		"german" { source = "Menu_demo_prompt_german.dds", vram=false, nomipmap=true }
		"italian" { source = "Menu_demo_prompt_italian.dds", vram=false, nomipmap=true }
		"portuguese" { source = "Menu_demo_prompt_portugese.dds", vram=false, nomipmap=true }
		"russian" { source = "Menu_demo_prompt_russian.dds", vram=false, nomipmap=true }
		"spanish" { source = "Menu_demo_prompt_spanish.dds", vram=false, nomipmap=true }
		{ source = "Menu_demo_prompt.dds", vram=false, nomipmap=true }
end

if REGION:find( "JP" ) then
	resource "Texture" "Menu_logo" { source = "LogoJapan.dds", vram=false , nomipmap=true }
else
	resource "Texture" "Menu_logo" { source = "Logo.dds", vram=false , nomipmap=true }
end

if REGION:find( "JP" ) then
	resource "Texture" "Menu_startresume" { source = "Menu_startresume_japanese.dds", vram=false , nomipmap=true }
	resource "Texture" "Menu_back" { source = "Menu_back_japanese.dds", vram=false , nomipmap=true }
	resource "Texture" "Menu_any" { source = "Menu_any_japanese.dds", vram=false , nomipmap=true }
	resource "Texture" "Menu_pressstart" { source = "Menu_pressstart_japanese.dds", vram=false , nomipmap=true }
else
	resource "Texture" "Menu_startresume" 
		"dutch" { source = "Menu_startresume_dutch.dds", vram=false , nomipmap=true}
		"french" { source = "Menu_startresume_french.dds", vram=false , nomipmap=true}
		"german" { source = "Menu_startresume_german.dds", vram=false , nomipmap=true}
		"italian" { source = "Menu_startresume_italian.dds", vram=false , nomipmap=true}
		"portuguese" { source = "Menu_startresume_portuguese.dds", vram=false , nomipmap=true}
		"russian" { source = "Menu_startresume_russian.dds", vram=false , nomipmap=true}
		"spanish" { source = "Menu_startresume_spanish.dds", vram=false , nomipmap=true}
		{ source = "Menu_startresume.dds", vram=false , nomipmap=true}
	resource "Texture" "Menu_back"
		"dutch" { source = "Menu_back_dutch.dds", vram=false , nomipmap=true}
		"french" { source = "Menu_back_french.dds", vram=false , nomipmap=true}
		"german" { source = "Menu_back_german.dds", vram=false , nomipmap=true}
		"italian" { source = "Menu_back_italian.dds", vram=false , nomipmap=true}
		"portuguese" { source = "Menu_back_portugese.dds", vram=false , nomipmap=true}
		"russian" { source = "Menu_back_russian.dds", vram=false , nomipmap=true}
		"spanish" { source = "Menu_back_spanish.dds", vram=false , nomipmap=true}
		{ source = "Menu_back.dds", vram=false , nomipmap=true}
	resource "Texture" "Menu_any"
		"dutch" { source = "Menu_any_dutch.dds", vram=false , nomipmap=true}
		"french" { source = "Menu_any_french.dds", vram=false , nomipmap=true}
		"german" { source = "Menu_any_german.dds", vram=false , nomipmap=true}
		"italian" { source = "Menu_any_italian.dds", vram=false , nomipmap=true}
		"portuguese" { source = "Menu_any_portuguese.dds", vram=false , nomipmap=true}
		"russian" { source = "Menu_any_russian.dds", vram=false , nomipmap=true}
		"spanish" { source = "Menu_any_spanish.dds", vram=false , nomipmap=true}
		{ source = "Menu_any.dds", vram=false , nomipmap=true}
	resource "Texture" "Menu_pressstart"
		"dutch" { source = "Menu_pressstart_dutch.dds", vram=false , nomipmap=true}
		"french" { source = "Menu_pressstart_french.dds", vram=false , nomipmap=true}
		"german" { source = "Menu_pressstart_german.dds", vram=false , nomipmap=true}
		"italian" { source = "Menu_pressstart_italian.dds", vram=false , nomipmap=true}
		"portuguese" { source = "Menu_pressstart_portuguese.dds", vram=false , nomipmap=true}
		"russian" { source = "Menu_pressstart_russian.dds", vram=false , nomipmap=true}
		"spanish" { source = "Menu_pressstart_spanish.dds", vram=false , nomipmap=true}
		{ source = "Menu_pressstart.dds", vram=false , nomipmap=true}
end
 
resource "Texture" "Menu_vignette" { source = "Menu_vignette.dds", vram=false , nomipmap=true}                   
resource "Texture" "Menu_glow" { source = "Menu_glow.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_special_leaf" { source = "Menu_special_leaf.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_tower_petal" { source = "Menu_tower_petal.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_fan" { source = "Menu_fan2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_cloth" { source = "Menu_cloth0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_grassfront" { source = "Menu_grassfront.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_grassmid" { source = "Menu_grassmid.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_grassfield" { source = "Menu_grassfield.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_grassfar" { source = "Menu_grassfar.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_grassmountain" { source = "Menu_grassmountain.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_grasssky" { source = "Menu_grasssky.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_rain" { source = "Menu_rain.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_rain2" { source = "Menu_rain2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_curtain" { source = "Menu_curtain.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_curtain2" { source = "Menu_curtain2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_curtain3" { source = "Menu_curtain3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_dust" { source = "Menu_dust.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_godray" { source = "Menu_godray.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall0" { source = "Menu_wall0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table0" { source = "Menu_table0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota0" { source = "Menu_pota0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb0" { source = "Menu_potb0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc0" { source = "Menu_potc0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd0" { source = "Menu_potd0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote0" { source = "Menu_pote0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf0" { source = "Menu_potf0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame0" { source = "Menu_frame0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair0" { source = "Menu_chair0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city0" { source = "Menu_city0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall0" { source = "Menu_citytall0.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky0" { source = "Menu_sky0.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall1" { source = "Menu_wall1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table1" { source = "Menu_table1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota1" { source = "Menu_pota1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb1" { source = "Menu_potb1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc1" { source = "Menu_potc1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd1" { source = "Menu_potd1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote1" { source = "Menu_pote1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf1" { source = "Menu_potf1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame1" { source = "Menu_frame1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair1" { source = "Menu_chair1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city1" { source = "Menu_city1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall1" { source = "Menu_citytall1.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky1" { source = "Menu_sky1.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall2" { source = "Menu_wall2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table2" { source = "Menu_table2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota2" { source = "Menu_pota2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb2" { source = "Menu_potb2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc2" { source = "Menu_potc2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd2" { source = "Menu_potd2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote2" { source = "Menu_pote2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf2" { source = "Menu_potf2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame2" { source = "Menu_frame2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair2" { source = "Menu_chair2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city2" { source = "Menu_city2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall2" { source = "Menu_citytall2.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky2" { source = "Menu_sky2.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall3" { source = "Menu_wall3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table3" { source = "Menu_table3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota3" { source = "Menu_pota3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb3" { source = "Menu_potb3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc3" { source = "Menu_potc3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd3" { source = "Menu_potd3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote3" { source = "Menu_pote3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf3" { source = "Menu_potf3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame3" { source = "Menu_frame3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair3" { source = "Menu_chair3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city3" { source = "Menu_city3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall3" { source = "Menu_citytall3.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky3" { source = "Menu_sky3.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall4" { source = "Menu_wall4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table4" { source = "Menu_table4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota4" { source = "Menu_pota4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb4" { source = "Menu_potb4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc4" { source = "Menu_potc4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd4" { source = "Menu_potd4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote4" { source = "Menu_pote4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf4" { source = "Menu_potf4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame4" { source = "Menu_frame4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair4" { source = "Menu_chair4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city4" { source = "Menu_city4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall4" { source = "Menu_citytall4.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky4" { source = "Menu_sky4.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall5" { source = "Menu_wall5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table5" { source = "Menu_table5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota5" { source = "Menu_pota5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb5" { source = "Menu_potb5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc5" { source = "Menu_potc5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd5" { source = "Menu_potd5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote5" { source = "Menu_pote5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf5" { source = "Menu_potf5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame5" { source = "Menu_frame5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair5" { source = "Menu_chair5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city5" { source = "Menu_city5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall5" { source = "Menu_citytall5.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky5" { source = "Menu_sky5.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall6" { source = "Menu_wall6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table6" { source = "Menu_table6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota6" { source = "Menu_pota6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb6" { source = "Menu_potb6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc6" { source = "Menu_potc6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd6" { source = "Menu_potd6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote6" { source = "Menu_pote6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf6" { source = "Menu_potf6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame6" { source = "Menu_frame6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair6" { source = "Menu_chair6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city6" { source = "Menu_city6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall6" { source = "Menu_citytall6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky6" { source = "Menu_sky6.dds", vram=false , nomipmap=true }

resource "Texture" "Menu_wall7" { source = "Menu_wall_comp.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_table7" { source = "Menu_table_comp.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pota7" { source = "Menu_pota6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potb7" { source = "Menu_potb6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potc7" { source = "Menu_potc6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potd7" { source = "Menu_potd6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_pote7" { source = "Menu_pote6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_potf7" { source = "Menu_potf6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_frame7" { source = "Menu_frame6.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_chair7" { source = "Menu_chair_comp.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_city7" { source = "Blank.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_citytall7" { source = "Blank.dds", vram=false , nomipmap=true }
resource "Texture" "Menu_sky7" { source = "White.dds", vram=false , nomipmap=true}

resource "Texture" "Controls_Glider" 
	"dutch" { source = "Controls_Glider_dutch.dds", vram=false , nomipmap=true}
	"french" { source = "Controls_Glider_french.dds", vram=false , nomipmap=true}
	"german" { source = "Controls_Glider_german.dds", vram=false , nomipmap=true}
	"italian" { source = "Controls_Glider_italian.dds", vram=false , nomipmap=true}
	"portuguese" { source = "Controls_Glider_portugese.dds", vram=false , nomipmap=true}
	"russian" { source = "Controls_Glider_russian.dds", vram=false , nomipmap=true}
	"spanish" { source = "Controls_Glider_spanish.dds", vram=false , nomipmap=true}
	"japanese" { source = "Controls_Glider_japanese.dds", vram=false , nomipmap=true}
	{ source = "Controls_Glider.dds", vram=false , nomipmap=true}
resource "Texture" "Controls_Motion" 
	"dutch" { source = "Controls_Motion_dutch.dds", vram=false , nomipmap=true}
	"french" { source = "Controls_Motion_french.dds", vram=false , nomipmap=true}
	"german" { source = "Controls_Motion_german.dds", vram=false , nomipmap=true}
	"italian" { source = "Controls_Motion_italian.dds", vram=false , nomipmap=true}
	"portuguese" { source = "Controls_Motion_portugese.dds", vram=false , nomipmap=true}
	"russian" { source = "Controls_Motion_russian.dds", vram=false , nomipmap=true}
	"spanish" { source = "Controls_Motion_spanish.dds", vram=false , nomipmap=true}
	"japanese" { source = "Controls_Motion_japanese.dds", vram=false , nomipmap=true}
	{ source = "Controls_Motion.dds", vram=false, nomipmap=true }
resource "Texture" "Symbol_Glider" 
	"dutch" { source = "Controls_Glider_Symbol_dutch.dds", vram=false , nomipmap=true}
	"french" { source = "Controls_Glider_Symbol_french.dds", vram=false , nomipmap=true}
	"german" { source = "Controls_Glider_Symbol_german.dds", vram=false , nomipmap=true}
	"italian" { source = "Controls_Glider_Symbol_italian.dds", vram=false , nomipmap=true}
	"portuguese" { source = "Controls_Glider_Symbol_portugese.dds", vram=false , nomipmap=true}
	"russian" { source = "Controls_Glider_Symbol_russian.dds", vram=false , nomipmap=true}
	"spanish" { source = "Controls_Glider_Symbol_spanish.dds", vram=false , nomipmap=true}
	"japanese" { source = "Controls_Glider_Symbol_japanese.dds", vram=false , nomipmap=true}
	{ source = "Controls_Glider_Symbol.dds", vram=false , nomipmap=true}
resource "Texture" "Symbol_Motion" 
	"dutch" { source = "Controls_Motion_Symbol_dutch.dds", vram=false , nomipmap=true}
	"french" { source = "Controls_Motion_Symbol_french.dds", vram=false , nomipmap=true}
	"german" { source = "Controls_Motion_Symbol_german.dds", vram=false , nomipmap=true}
	"italian" { source = "Controls_Motion_Symbol_italian.dds", vram=false , nomipmap=true}
	"portuguese" { source = "Controls_Motion_Symbol_portugese.dds", vram=false , nomipmap=true}
	"russian" { source = "Controls_Motion_Symbol_russian.dds", vram=false , nomipmap=true}
	"spanish" { source = "Controls_Motion_Symbol_spanish.dds", vram=false , nomipmap=true}
	"japanese" { source = "Controls_Motion_Symbol_japanese.dds", vram=false , nomipmap=true}
	{ source = "Controls_Motion_Symbol.dds", vram=false, nomipmap=true }

resource "Texture" "TouchLocation" { source = "TouchLocation.dds", vram=false, nomipmap=true }
resource "Texture" "TouchIcon1" { source = "TouchIcon.dds", vram=false, nomipmap=true }
resource "Texture" "Recalibration_Prone" { source = "Recalibrate_Prone.dds", vram=false, nomipmap=true }
resource "Texture" "Recalibration_Sitting" { source = "Recalibrate_Upright.dds", vram=false, nomipmap=true }
resource "Texture" "Recalibration_Leaning" { source = "Recalibrate_LeanForward.dds", vram=false, nomipmap=true }
resource "Texture" "Controls_Prompt" 
	"dutch" { source = "Controls_Prompt_dutch.dds", vram=false , nomipmap=true}
	"french" { source = "Controls_Prompt_french.dds", vram=false , nomipmap=true}
	"german" { source = "Controls_Prompt_german.dds", vram=false , nomipmap=true}
	"italian" { source = "Controls_Prompt_italian.dds", vram=false , nomipmap=true}
	"portuguese" { source = "Controls_Prompt_portugese.dds", vram=false , nomipmap=true}
	"russian" { source = "Controls_Prompt_russian.dds", vram=false , nomipmap=true}
	"spanish" { source = "Controls_Prompt_spanish.dds", vram=false , nomipmap=true}
	"japanese" { source = "Controls_Prompt_japanese.dds", vram=false , nomipmap=true}
	{ source = "Controls_Prompt.dds", vram=false , nomipmap=true}

--Object Textures
resource "Texture" "Fog2" { source = "Fog2.dds" }
resource "Texture" "LensFlayer" { source = "LensFlayer.dds" }
resource "Texture" "FreewaySign" { source = "FreewaySign.dds" }
resource "Texture" "TowerTree" { source = "TowerTree.dds" }
resource "Texture" "TowerTreetop" { source = "TowerTreetop.dds" }
resource "Texture" "TreeBark" { source = "TreeBark.dds" }
resource "Texture" "NightLine" { source = "nightskyline.dds" }
resource "Texture" "WindmillDirty" { source = "WindmillDirty.dds" }
resource "Texture" "Rock" { source = "rock.dds" }
resource "Texture" "Rock2" { source = "rock2.dds" }
resource "Texture" "Rock3" { source = "rock3.dds" }
resource "Texture" "Rust" { source = "rust.dds" }
resource "Texture" "Rust2" { source = "rust2.dds" }
resource "Texture" "Metal" { source = "metal.dds" }
resource "Texture" "MetalDark" { source = "metaldark.dds" }
resource "Texture" "Light" { source = "Light.dds" }
resource "Texture" "LightBeam" { source = "lightbeam.dds" }
resource "Texture" "Rainbow" { source = "Rainbow.dds" }
resource "Texture" "Leaf" { source = "Leaf.dds" }
resource "Texture" "Highway" { source = "highway.dds", filter = "aniso4" }
resource "Texture" "HighwayDirt" { source = "highwaydirt.dds" }
resource "Texture" "Skyline" { source = "skyline.dds", wrap=1 }
resource "Texture" "SkylineLevel2" { source = "SkylineLevel2.dds", wrap=1 }
resource "Texture" "SkylineLevel4" { source = "SkylineLevel4.dds", wrap=1 }
resource "Texture" "SkylineLevel5" { source = "SkylineLevel5.dds", wrap=1 }
resource "Texture" "Fog" { source = "Fog.dds" }
resource "Texture" "DarkPile" { source = "Darkpile.dds" }
resource "Texture" "DarkPile2" { source = "darkpile2.dds" }
resource "Texture" "SwingSet" { source = "swingset.dds" }
resource "Texture" "TowerVine" { source = "TowerVine.dds" }
resource "Texture" "MilkyWay" { source = "MilkyWay.dds" }
resource "Texture" "Stars" { source = "Stars.dds" }
resource "Texture" "Haystack" { source = "Haystack.dds" }
resource "Texture" "Wagon" { source = "Wagon.dds" }
resource "Texture" "Barbedwire" { source = "Barbedwire.dds" }
resource "Texture" "Snake" { source = "Snake.dds" }
resource "Texture" "Jelly" { source = "Jelly.dds" }
resource "Texture" "Armpit" { source = "Armpit.dds" }
resource "Texture" "Cloud1" { source = "Cloud1.dds" }
resource "Texture" "JapanLine" { source = "JapanLine.dds" }

-- Skyboxes
resource "Texture" "Sky_nocloud" { source = "sky_nocloud.dds" }
resource "Texture" "Sky_lightcloud" { source = "sky_lightcloud.dds" }
resource "Texture" "Sky_skyline" { source = "sky_skyline.dds" }
resource "Texture" "Sky_sunset" { source = "sky_sunset.dds" }
resource "Texture" "Sky_path" { source = "sky_path.dds" }
resource "Texture" "Sky_level1" { source = "Sky_level1.dds", cubemap=true }
resource "Texture" "Sky_level3" { source = "Sky_level3.dds", cubemap=true }
resource "Texture" "Sky_level3_3" { source = "Sky_level3_3.dds", cubemap=true }
resource "Texture" "Sky_level7" { source = "Sky_level7.dds", cubemap=true }
resource "Texture" "Sky_level9" { source = "Sky_level9.dds", cubemap=true }
resource "Texture" "Sky_level9_2" { source = "Sky_level9_2.dds", cubemap=true }

resource "Texture" "TerrainDetailLo" { source = "TerrainDetailLo.dds" }
resource "Texture" "TerrainDetailHi" { source = "TerrainDetailHi.dds" }
resource "Texture" "GrassBlade" { source = "GrassBlade.dds" }
resource "Texture" "GrassClump" { source = "GrassClump.dds" }
resource "Texture" "FlowerQuad" { source = "flowerquad2.dds" }
resource "Texture" "FlowerHalo" { source = "flowerhalo.dds" }
resource "Texture" "PetalHalo" { source = "petalhalo.dds" }
resource "Texture" "PetalBlur" { source = "FlowerBlur.dds" }
resource "Texture" "FillerFlower" { source = "FillerFlower.dds" }
resource "Texture" "PetalTexture" { source = "petalAtlas.dds", filter = "aniso16" }
resource "Texture" "ParticleTexture" { source = "particleAtlas.dds", filter = "aniso8" }
resource "Texture" "RibbonTexture" { source = "ribbon.dds" }
resource "Texture" "DD_Logo" { source = "DD_Logo.dds" }

resource "Texture" "DebugFont" { source = "DebugFont.dds" }

resource "FileTracker" "TrophyPack" { filename = "../sce_sys/trophy/NPWR04651_00/TROPHY.TRP", source="../sce_sys/trophy/PSP2/NPWR04651_00/TROPHY.TRP" }

resource "FileTracker" "Movie01" { filename = "Movies/cinematic01.m4v" }
resource "FileTracker" "Movie02" { filename = "Movies/cinematic02.m4v" }
resource "FileTracker" "Movie03" { filename = "Movies/cinematic03.m4v" }
resource "FileTracker" "Movie04" { filename = "Movies/cinematic04.m4v" }
resource "FileTracker" "Movie05" { filename = "Movies/cinematic05.m4v" }
resource "FileTracker" "Movie06" { filename = "Movies/cinematic06.m4v" }

if "JP" == REGION then
	resource "FileTracker" "Trailer" { filename = "Movies/TrailerJp.m4v" }
else
	resource "FileTracker" "Trailer" { filename = "Movies/Trailer.m4v" }
	if "EU" == REGION then
		resource "FileTracker" "TrailerPal" { filename = "Movies/TrailerPal.m4v" }
	end
end

end -- if TARGET=="PSP2"