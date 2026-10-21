-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level2_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level2_Flr.bank" }

resource "Song" "SongA" { source = "MusicSwarm09.lua" }
resource "Song" "SongB" { source = "MusicSwarm11.lua" }
resource "Song" "SongC" { source = "MusicSwarm12.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_2/"
	, srcHeight = "Level_2/Heightmap.dds"
	, srcGround = "Level_2/LandColor.dds"
	, srcGrass = "Level_2/GrassColor.dds"
	, srcFlower = "Level_2/FlowerColor.dds"
	, shader = "TerrainR"		-- Red detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_2/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_2/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_2/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_2/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_2/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_2/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_2/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_2/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_2/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_2/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_2/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_2/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_2/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_2/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_2/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_2/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_2/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_2/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_2/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_2/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_2/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_2/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_2/Detail.dds" }
