-------------------------------------------------------------------------------
-- Notes:  List the resources that need to be loaded to get basic system
--         functionality up and running.
--
-- Author: John Edwards
-- Date C: 12-9-2008
-------------------------------------------------------------------------------

resource "Script" "LocalizedStrings" { source = "LocalizedStrings.lua" }
resource "Script" "Helpers" { source = "Helpers.lua" }
resource "Script" "Tick" { source = "Tick.lua" }
resource "Script" "LevelUid" { source = "LevelUid.lua" }
resource "Script" "DebugKeys" { source = "DebugKeys.lua" }
resource "Script" "PWeb3" { source = "PWeb3.lua" }
resource "Script" "Garnet" { source = "Garnet.lua" }

resource "FileTracker" "Vars" { filename = "Scripts/Vars.lua", script = true } -- Special global vars script.
resource "FileTracker" "AnimationHack64" { filename = "Animation/Anim.P64" }

--if not DEMO or 0 == DEMO then
--	resource "FileTracker" "NsbEntitlement" { filename = "Guard/Guard.txt.edat" }
--end


-- needed for the Initialization coroutine
resource "Texture" "NoSkybox" { source = "none.dds" }
resource "Script" "CameraInit" { source = "CameraInit.lua" }



if "PublicBeta" == CONFIG then
	resource "FileTracker" "SaveIcon" { filename = "Textures/BlankIcon.PNG" }
else
	if "JP" == REGION then
		resource "FileTracker" "SaveIcon" 
			"english" { filename = "Save/ICON0.PNG" }
			{ filename = "Save/ICON0_00.PNG" }
	elseif "EU" == REGION then
		resource "FileTracker" "SaveIcon"
			"russian" { filename = "Save/ICON0_08.PNG" }
			"polish" { filename = "Save/ICON0_16.PNG" }
			{ filename = "Save/ICON0.PNG" }
	elseif "CH" == REGION then
		resource "FileTracker" "SaveIcon"
			"chinese_s" { filename = "Save/ICON0_11.PNG" }
			{ filename = "Save/ICON0.PNG" }
	else
		resource "FileTracker" "SaveIcon" { filename = "Save/ICON0.PNG" }
	end
end
