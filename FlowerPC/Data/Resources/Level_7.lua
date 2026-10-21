-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level7_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level7_Flr.bank" }
resource "ScreamBank" "Music" { source = "Level7Music.bank" }

resource "Song" "SongA" { source = "EmptyMusic.lua" }
resource "Song" "SongB" { source = "MusicSwarm11.lua" }
resource "Song" "SongC" { source = "MusicSwarm12.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_7/"
	, srcHeight = "Level_7/Heightmap.dds"
	, srcGround = "Level_7/LandColor.dds"
	, srcGrass = "Level_7/GrassColor.dds"
	, srcFlower = "Level_7/FlowerColor.dds"
	, shader = "TerrainR"		-- Red detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_7/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_7/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_7/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_7/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_7/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_7/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_7/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_7/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_7/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_7/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_7/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_7/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_7/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_7/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_7/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_7/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_7/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_7/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_7/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_7/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_7/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_7/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_7/Detail.dds" }
