-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Desert-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Desert-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Desert/"
	, srcHeight = "Level_Desert/Heightmap.dds"
	, srcGround = "Level_Desert/LandColor.dds"
	, srcMask = "Level_Desert/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Desert/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Desert/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Desert/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Desert/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Desert/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Desert/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Desert/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Desert/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Desert/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Desert/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Desert/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Desert/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Desert/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Desert/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Desert/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Desert/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Desert/DuneColorShadow.dds", gammaRemap = true }