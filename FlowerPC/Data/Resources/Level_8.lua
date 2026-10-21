-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "Song" "SongA" { source = "EmptyMusic.lua" }
resource "Song" "SongB" { source = "EmptyMusic.lua" }
resource "Song" "SongC" { source = "EmptyMusic.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_8/"
	, srcHeight = "Level_8/Heightmap.dds"
	, srcGround = "Level_8/LandColor.dds"
	, srcGrass = "Level_8/GrassColor.dds"
	, srcFlower = "Level_8/FlowerColor.dds"
	, shader = "TerrainNone"		-- NO detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_8/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_8/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_8/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_8/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_8/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_8/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_8/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_8/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_8/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_8/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_8/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_8/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_8/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_8/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_8/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_8/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_8/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_8/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_8/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_8/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_8/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_8/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_8/Detail.dds" }
