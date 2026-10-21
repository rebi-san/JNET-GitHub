-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level4_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level4_Flr.bank" }
resource "ScreamBank" "Music" { source = "Level35Music.bank" }

resource "Song" "SongA" { source = "MusicSwarm19.lua" }
resource "Song" "SongB" { source = "MusicSwarm11.lua" }
resource "Song" "SongC" { source = "MusicSwarm12.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_4/"
	, srcHeight = "Level_4/Heightmap.dds"
	, srcGround = "Level_4/LandColor.dds"
	, srcGrass = "Level_4/GrassColor.dds"
	, srcFlower = "Level_4/FlowerColor.dds"
	, shader = "TerrainNone"		-- NO detail mask
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_4/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_4/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_4/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_4/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_4/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_4/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_4/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_4/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_4/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_4/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_4/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_4/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_4/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_4/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_4/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_4/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_4/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_4/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_4/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_4/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_4/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_4/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_4/Detail.dds" }