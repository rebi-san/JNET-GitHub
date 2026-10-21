-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

resource "ScreamBank" "LvlSfx" { source = "Barrens-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Barrens-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Barrens/"
	, srcHeight = "Level_Barrens/Heightmap.dds"
	, srcGround = "Level_Barrens/LandColor.dds"
	, srcMask = "Level_Barrens/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Barrens/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Barrens/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Barrens/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Barrens/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Barrens/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Barrens/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Barrens/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Barrens/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Barrens/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Barrens/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Barrens/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Barrens/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Barrens/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Barrens/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Barrens/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Barrens/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Barrens/DuneColorShadow.dds", gammaRemap = true }