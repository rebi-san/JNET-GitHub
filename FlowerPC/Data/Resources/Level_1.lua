-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level1_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level1_Flr.bank" }

resource "Song" "SongA" { source = "MusicSwarm18.lua" }
resource "Song" "SongB" { source = "MusicSwarm06.lua" }
resource "Song" "SongC" { source = "Level1Section3.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_1/"
	, srcHeight = "Level_1/Heightmap.dds"
	, srcGround = "Level_1/LandColor.dds"
	, srcGrass = "Level_1/GrassColor.dds"
	, srcFlower = "Level_1/FlowerColor.dds"
	, shader = "TerrainR"		-- Red detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_1/Border.dds" } 

resource "Script" "CameraHullInstances" { source = "Level_1/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_1/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_1/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_1/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_1/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_1/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_1/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_1/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_1/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_1/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_1/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_1/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_1/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_1/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_1/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_1/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_1/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_1/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_1/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_1/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_1/RailInstances.lua" }


resource "Texture" "TerrainDetailIdx" { source = "Level_1/Detail.dds" }