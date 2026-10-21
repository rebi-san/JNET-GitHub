-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Ruins-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Ruins-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Ruins/"
	, srcHeight = "Level_Ruins/Heightmap.dds"
	, srcGround = "Level_Ruins/LandColor.dds"
	, srcMask = "Level_Ruins/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Ruins/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Ruins/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Ruins/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Ruins/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Ruins/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Ruins/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Ruins/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Ruins/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Ruins/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Ruins/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Ruins/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Ruins/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Ruins/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Ruins/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Ruins/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Ruins/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Ruins/DuneColorShadow.dds", gammaRemap = true }
