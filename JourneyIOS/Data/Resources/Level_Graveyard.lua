-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Graveyard-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Graveyard-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Graveyard/"
	, srcHeight = "Level_Graveyard/Heightmap.dds"
	, srcGround = "Level_Graveyard/LandColor.dds"
	, srcMask = "Level_Graveyard/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Graveyard/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Graveyard/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Graveyard/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Graveyard/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Graveyard/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Graveyard/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Graveyard/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Graveyard/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Graveyard/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Graveyard/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Graveyard/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Graveyard/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Graveyard/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Graveyard/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Graveyard/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Graveyard/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Graveyard/DuneColorShadow.dds", gammaRemap = true }