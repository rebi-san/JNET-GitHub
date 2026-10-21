-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Credits-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Credits-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Chris/"
	, srcHeight = "Level_Chris/Heightmap.dds"
	, srcGround = "Level_Chris/LandColor.dds"
	, srcMask = "Level_Chris/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Chris/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Chris/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Chris/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Chris/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Chris/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Chris/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Chris/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Chris/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Chris/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Chris/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Chris/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Chris/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Chris/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Chris/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Chris/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Chris/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Chris/DuneColorShadow.dds", gammaRemap = true }