-------------------------------------------------------------------------------
-- Notes:  A list of resources that are loaded before the opening titles are
--         displayed, and then unloaded afterwards (ideally).
--
-- Author: John Edwards
-- Date C: 8-17-2007
-------------------------------------------------------------------------------

resource "Shader" "SlideShow" { vs = "Resample_vs.cg", fs = "SlideShow_fs.cg", instanceLimit = 1 }

resource "Script" "SlideRender" { source = "SlideRender.lua" }

resource "Script" "OpeningTitles" { source = "OpeningTitles.lua" }

if "UA" == REGION or "AS" == REGION then
	resource "FileTracker" "Title_Epilepsy"
		"french" { filename = "Textures/Title_Epilepsy_french.pvr" }
		"italian" { filename = "Textures/Title_Epilepsy_italian.pvr" }
		"german" { filename = "Textures/Title_Epilepsy_german.pvr" }
		"spanish" { filename = "Textures/Title_Epilepsy_spanish.pvr" }
		"dutch" { filename = "Textures/Title_Epilepsy_dutch.pvr" }
		"portuguese" { filename = "Textures/Title_Epilepsy_portuguese.pvr" }
		"russian" { filename = "Textures/Title_Epilepsy_russian.pvr" }
		{ filename = "Textures/Title_Epilepsy.pvr" }
end

if "UA" == REGION or "AS" == REGION then
	resource "FileTracker" "Title_Esrb" { filename = "Textures/Title_Esrb.pvr" }
end

if "UA" == REGION or "EU" == REGION or "AS" == REGION then
	resource "FileTracker" "Title_SaveMessage"
		"french" { filename = "Textures/Title_SaveMessage_french.pvr" }
		"italian" { filename = "Textures/Title_SaveMessage_italian.pvr" }
		"german" { filename = "Textures/Title_SaveMessage_german.pvr" }
		"spanish" { filename = "Textures/Title_SaveMessage_spanish.pvr" }
		"dutch" { filename = "Textures/Title_SaveMessage_dutch.pvr" }
		"portuguese" { filename = "Textures/Title_SaveMessage_portuguese.pvr" }
		"russian" { filename = "Textures/Title_SaveMessage_russian.pvr" }
		{ filename = "Textures/Title_SaveMessage.pvr" }
end
if "JP" == REGION then
	resource "FileTracker" "Title_Warning" { filename = "Textures/Title_WarningJp.pvr" }
end

if "UA" == REGION or "AS" == REGION then
	resource "FileTracker" "Title_SonyLogo"
		"french" { filename = "Textures/Title_SonyLogoUA_french.pvr" }
		"spanish" { filename = "Textures/Title_SonyLogoUA_spanish.pvr" }
		"portuguese" { filename = "Textures/Title_SonyLogoUA_portuguese.pvr" }
		{ filename = "Textures/Title_SonyLogoUA.pvr" }
elseif "EU" == REGION then
	resource "FileTracker" "Title_SonyLogo"
		"french" { filename = "Textures/Title_SonyLogo_french.pvr" }
		"italian" { filename = "Textures/Title_SonyLogo_italian.pvr" }
		"german" { filename = "Textures/Title_SonyLogo_german.pvr" }
		"spanish" { filename = "Textures/Title_SonyLogo_spanish.pvr" }
		"dutch" { filename = "Textures/Title_SonyLogo_dutch.pvr" }
		"portuguese" { filename = "Textures/Title_SonyLogo_portuguese.pvr" }
		"russian" { filename = "Textures/Title_SonyLogo_russian.pvr" }
		{ filename = "Textures/Title_SonyLogo.pvr" }
elseif "JP" == REGION then
	resource "FileTracker" "Title_SonyLogo" { filename = "Textures/Title_SonyLogoJP.pvr" }
end
resource "FileTracker" "Title_SmLogo" { filename = "Textures/Title_SmLogo.pvr" }
resource "FileTracker" "Title_TgcLogo" { filename = "Textures/Title_TgcLogo.pvr" }

-- Japanese PS4 control scheme has legal text on it
if "JP" == REGION and "PS4"==TARGET then
	resource "FileTracker" "Title_ControlScheme" { filename = "Textures/Title_ControlScheme_JP.pvr" }
else
	resource "FileTracker" "Title_ControlScheme" { filename = "Textures/Title_ControlScheme.pvr" }
end

resource "FileTracker" "Black" { filename = "Textures/Black.pvr" }
resource "FileTracker" "White" { filename = "Textures/White.pvr" }
