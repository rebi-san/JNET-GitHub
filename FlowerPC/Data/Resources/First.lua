-------------------------------------------------------------------------------
-- Notes:  List the resources that need to be loaded to get basic system
--         functionality up and running.
--
-- Author: John Edwards
-- Date C: 12-9-2008
-------------------------------------------------------------------------------

resource "ScreamBank" "MenuSfx" { source = "Menu.bank" }

resource "Script" "ResourceFiles" { source = "ResourceFiles"..REGION..".lua" } -- This script is generated automatically.
resource "Script" "LocalizedStrings" { source = "LocalizedStrings.lua" }
resource "Script" "Tick" { source = "Tick.lua" }

resource "FileTracker" "Vars" { filename = "Scripts/Vars.lua", script = true } -- Special global vars script.

resource "FileTracker" "NsbEntitlement" { filename = "Guard/Guard.txt.edat" }


-- needed for the Initialization coroutine
resource "Texture" "NoSkybox" { source = "none.dds" }
resource "Script" "Tutorial" { source = "Tutorial.lua" }
resource "Script" "CameraInit" { source = "CameraInit.lua" }
if "PSP2" == TARGET then
	resource "FileTracker" "ReverbPresets" { filename = "Sounds/reverbNGS.ini" }
else
	resource "FileTracker" "ReverbPresets" { filename = "Sounds/Presets.ini" }
end

if "JP" == REGION then
	resource "FileTracker" "SaveIcon" { filename = "Save/ICON0_00.PNG" }
else
	resource "FileTracker" "SaveIcon" { filename = "Save/ICON0.PNG" }
end
