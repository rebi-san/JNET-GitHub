-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Credits-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Credits-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Matt/"
	, srcHeight = "Level_Matt/Heightmap.dds"
	, srcGround = "Level_Matt/LandColor.dds"
	, srcMask = "Level_Matt/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Matt/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Matt/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Matt/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Matt/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Matt/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Matt/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Matt/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Matt/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Matt/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Matt/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Matt/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Matt/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Matt/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Matt/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Matt/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Matt/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Matt/DuneColorShadow.dds", gammaRemap = true }