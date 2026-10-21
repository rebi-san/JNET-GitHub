-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Level6_A.bank" }
resource "ScreamBank" "FlrSfx" { source = "Level6_Flr.bank" }
resource "ScreamBank" "Music" { source = "Level5Music.bank" }

resource "Song" "SongA" { source = "EmptyMusic.lua" }
resource "Song" "SongB" { source = "MusicSwarmL61.lua" }
resource "Song" "SongC" { source = "MusicSwarmL62.lua" }
--resource "Song" "SongD" { source = "MusicSwarmL63.lua" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_6/"
	, srcHeight = "Level_6/Heightmap.dds"
	, srcGround = "Level_6/LandColor.dds"
	, srcGrass = "Level_6/GrassColor.dds"
	, srcFlower = "Level_6/FlowerColor.dds"
	, shader = "TerrainNone"		-- NO detail mask
	, vitaLODScale = 0.0			-- maximum possible LODing!!
	}
resource "FileTracker" "Border" { filename = "Terrain/Level_6/Border.dds" }

resource "Script" "CameraHullInstances" { source = "Level_6/CameraHullInstances.lua" }
resource "Script" "BoundaryHullInstances" { source = "Level_6/BoundaryHullInstances.lua" }
resource "Script" "PhysicalHullInstances" { source = "Level_6/PhysicalHullInstances.lua" }
resource "Script" "RegionHullInstances" { source = "Level_6/RegionHullInstances.lua" }
resource "Script" "WindHullInstances" { source = "Level_6/WindHullInstances.lua" }
resource "Script" "FlowerInstances" { source = "Level_6/FlowerInstances.lua" }
resource "Script" "DecorationMeshInstances" { source = "Level_6/DecorationMeshInstances.lua" }
resource "Script" "LandingZoneInstances" { source = "Level_6/LandingZoneInstances.lua" }
resource "Script" "EnvNodeInstances" { source = "Level_6/EnvNodeInstances.lua" }
resource "Script" "ShockwaveInstances" { source = "Level_6/ShockwaveInstances.lua" }
resource "Script" "WaypointInstances" { source = "Level_6/WaypointInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_6/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_6/SoundEmitterInstances.lua" }
resource "Script" "SoundDataInstances" { source = "Level_6/SoundDataInstances.lua" }
resource "Script" "SignInstances" { source = "Level_6/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_6/SignData.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_6/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_6/StartLocation.lua" }

resource "Script" "TriggerInstances" {source = "Level_6/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_6/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_6/RailInstances.lua" }

resource "Texture" "TerrainDetailIdx" { source = "Level_6/Detail.dds" }

if "PSP2" == TARGET then
	resource "SoundStream" "lvl6_00_16_loop" { source = "lvl6_00_16_loop.mp3", filename = "lvl6_00_16_loop.at9" }
	resource "SoundStream" "lvl6_01_16_loop" { source = "lvl6_01_16_loop.mp3", filename = "lvl6_01_16_loop.at9" }
else
	resource "FileTracker" "lvl6_00_16_loop" { filename = "Sounds/Streams/lvl6_00_16_loop.mp3" }
	resource "FileTracker" "lvl6_01_16_loop" { filename = "Sounds/Streams/lvl6_01_16_loop.mp3" }
end