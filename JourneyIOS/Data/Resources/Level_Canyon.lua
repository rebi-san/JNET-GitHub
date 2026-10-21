-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Canyon-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Canyon-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Canyon/"
	, srcHeight = "Level_Canyon/Heightmap.dds"
	, srcGround = "Level_Canyon/LandColor.dds"
	, srcMask = "Level_Canyon/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Canyon/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Canyon/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Canyon/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Canyon/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Canyon/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Canyon/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Canyon/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Canyon/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Canyon/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Canyon/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Canyon/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Canyon/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Canyon/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Canyon/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Canyon/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Canyon/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Canyon/DuneColorShadow.dds", gammaRemap = true }