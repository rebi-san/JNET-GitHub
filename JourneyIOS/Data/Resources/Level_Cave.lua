-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Cave-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Cave-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Cave/"
	, srcHeight = "Level_Cave/Heightmap.dds"
	, srcGround = "Level_Cave/LandColor.dds"
	, srcMask = "Level_Cave/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Cave/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Cave/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Cave/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Cave/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Cave/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Cave/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Cave/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Cave/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Cave/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Cave/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Cave/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Cave/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Cave/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Cave/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Cave/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Cave/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Cave/DuneColorShadow.dds", gammaRemap = true }
