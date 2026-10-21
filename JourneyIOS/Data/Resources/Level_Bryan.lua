-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Mountain-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Mountain-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Bryan/"
	, srcHeight = "Level_Bryan/Heightmap.dds"
	, srcGround = "Level_Bryan/LandColor.dds"
	, srcMask = "Level_Bryan/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Bryan/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Bryan/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Bryan/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Bryan/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Bryan/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Bryan/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Bryan/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Bryan/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Bryan/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Bryan/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Bryan/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Bryan/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Bryan/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Bryan/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Bryan/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Bryan/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Bryan/DuneColorShadow.dds", gammaRemap = true }