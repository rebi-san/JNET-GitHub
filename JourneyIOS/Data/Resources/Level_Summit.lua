-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Summit-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Summit-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Summit/"
	, srcHeight = "Level_Summit/Heightmap.dds"
	, srcGround = "Level_Summit/LandColor.dds"
	, srcMask = "Level_Summit/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Summit/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Summit/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Summit/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Summit/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Summit/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Summit/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Summit/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Summit/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Summit/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Summit/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Summit/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Summit/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Summit/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Summit/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Summit/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Summit/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Summit/DuneColorShadow.dds", gammaRemap = true }