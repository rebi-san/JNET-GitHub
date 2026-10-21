-- We will probably want to figure out a scheme for automatically generating level
-- resource scripts.  Once Gardener is set up, that shouldn't be too hard.

if "PublicBeta" == CONFIG then return end

resource "ScreamBank" "LvlSfx" { source = "Credits-SFX.bank" }
resource "ScreamBank" "LvlMus" { source = "Credits-MUS.bank" }

resource "TerrainData" "TerrainData"
	{ outDir = "Level_Credits/"
	, srcHeight = "Level_Credits/Heightmap.dds"
	, srcGround = "Level_Credits/LandColor.dds"
	, srcMask = "Level_Credits/MaskData.dds"
	}

resource "Binary" "HullInstances" { source = "Scripts/Level_Credits/HullInstances.lua", type = "Hull" }
resource "Binary" "DecorationMeshInstances" { source = "Scripts/Level_Credits/DecorationMeshInstances.lua", type = "Decoration" }

resource "Script" "EnvNodeInstances" { source = "Level_Credits/EnvNodeInstances.lua" }
resource "Script" "MarkerInstances" { source = "Level_Credits/MarkerInstances.lua" }
resource "Script" "ParticleEmitterInstances" { source = "Level_Credits/ParticleEmitterInstances.lua" }
resource "Script" "SoundEmitterInstances" { source = "Level_Credits/SoundEmitterInstances.lua" }
resource "Script" "SignInstances" { source = "Level_Credits/SignInstances.lua" }
resource "Script" "SignData" { source = "Level_Credits/SignData.lua" }
resource "Script" "JetInstances" { source = "Level_Credits/JetInstances.lua" }
resource "Script" "DynamicNodeInstances" { source = "Level_Credits/DynamicNodeInstances.lua" }

resource "Script" "CameraKeyFrameInstances" { source = "Level_Credits/CameraKeyFrameInstances.lua" }
resource "Script" "StartLocation" { source = "Level_Credits/StartLocation.lua" }

resource "Script" "TimelineInstances" {source = "Level_Credits/TimelineInstances.lua" }
resource "Script" "TriggerInstances" {source = "Level_Credits/TriggerInstances.lua" }
resource "Script" "ClumpInstances" { source = "Level_Credits/ClumpInstances.lua" }
resource "Script" "RailInstances" { source = "Level_Credits/RailInstances.lua" }

resource "Texture" "DuneColorShadow" { source = "Level_Credits/DuneColorShadow.dds", gammaRemap = true }