-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level5_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level5_Flr.bank" }
resource "ScreamBank" "Music" { source = "Level4Music.bank" }

resource "Song" "SongA" { source = "EmptyMusic.lua" }
resource "Song" "SongB" { source = "MusicSwarm17.lua" }
resource "Song" "SongC" { source = "EmptyMusic.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_5/"
	, srcHeight = "Level_5/Heightmap.dds"
	, srcGround = "Level_5/LandColor.dds"
	, srcGrass = "Level_5/GrassColor.dds"
	, srcFlower = "Level_5/FlowerColor.dds"
	, shader = "TerrainRG"		-- Red/Green detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_5/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_5/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_5/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_5/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_5/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_5/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_5/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_5/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_5/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_5/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_5/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_5/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_5/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_5/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_5/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_5/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_5/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_5/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_5/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_5/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_5/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_5/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_5/Detail.dds" }
