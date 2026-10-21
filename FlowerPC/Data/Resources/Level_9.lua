-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level1_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level1_Flr.bank" }

resource "Song" "SongA" { source = "MusicSwarm19.lua" }
resource "Song" "SongB" { source = "MusicSwarm11.lua" }
resource "Song" "SongC" { source = "MusicSwarm12.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_9/"
	, srcHeight = "Level_9/Heightmap.dds"
	, srcGround = "Level_9/LandColor.dds"
	, srcGrass = "Level_9/GrassColor.dds"
	, srcFlower = "Level_9/FlowerColor.dds"
	, shader = "TerrainNone"		-- NO detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_9/Border.dds" }

resource "Script" "BoundaryHullInstances" { source = "Level_9/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_9/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_9/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_9/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_9/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_9/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_9/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_9/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_9/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_9/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_9/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_9/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_9/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_9/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_9/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_9/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_9/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_9/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_9/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_9/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_9/Detail.dds" }