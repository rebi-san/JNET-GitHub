-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Mountain-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Mountain-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Mountain/"
	, srcHeight = "Level_Mountain/Heightmap.dds"
	, srcGround = "Level_Mountain/LandColor.dds"
	, srcMask = "Level_Mountain/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Mountain/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Mountain/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Mountain/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Mountain/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Mountain/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Mountain/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Mountain/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Mountain/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Mountain/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Mountain/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Mountain/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Mountain/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Mountain/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Mountain/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Mountain/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Mountain/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Mountain/DuneColorShadow.dds", gammaRemap = true }