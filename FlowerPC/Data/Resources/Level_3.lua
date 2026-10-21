-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level3_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level3_Flr.bank" }

resource "Song" "SongA" { source = "MusicSwarm15.lua" }
resource "Song" "SongB" { source = "MusicSwarm14.lua" }
resource "Song" "SongC" { source = "MusicSwarm16.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_3/"
	, srcHeight = "Level_3/Heightmap.dds"
	, srcGround = "Level_3/LandColor.dds"
	, srcGrass = "Level_3/GrassColor.dds"
	, srcFlower = "Level_3/FlowerColor.dds"
	, shader = "TerrainNone"		-- NO detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_3/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_3/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_3/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_3/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_3/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_3/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_3/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_3/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_3/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_3/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_3/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_3/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_3/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_3/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_3/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_3/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_3/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_3/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_3/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_3/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_3/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_3/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_3/Detail.dds" }