-------------------------------------------------------------------------------
-- Notes:  A list of resources that are loaded before the opening titles are
--         displayed, and then unloaded afterwards (ideally).
--
-- Author: John Edwards
-- Date C: 8-17-2007
-------------------------------------------------------------------------------

resource "Shader" "SlideShow" { vs = "Resample_vs.cg", fs = "SlideShow_fs.cg", instanceLimit = 4 }

resource "Script" "SlideRender" { source = "SlideRender.lua" }

resource "Script" "OpeningTitles" { source = "OpeningTitles.lua" }

if "CH" == REGION then
resource "FileTracker" "Title_Medical" { filename = "Textures/Title_Medical_Ch.dds" }
end

--	if "JP" == REGION then
--		resource "FileTracker" "Title_Warning" { filename = "Textures/Title_WarningJp.dds.gz" }
--		resource "FileTracker" "Title_Medical" { filename = "Textures/Title_MedicalJp.dds.gz" }
--		resource "FileTracker" "Title_Controller" { filename = "Textures/Title_ControllerJp.dds.gz" }
--		resource "FileTracker" "Title_Save" { filename = "Textures/Title_SaveMessage_japanese.dds.gz" }
--		resource "FileTracker" "Title_Dolby" { filename = "Textures/Title_Dolby_jp.dds.gz" }
--	elseif "AS" == REGION then
--		resource "FileTracker" "Title_Warning"
--			"chinese_t" { filename = "Textures/Title_Warning_tc.dds.gz" }
--			"korean" { filename = "Textures/Title_Warning_korean.dds.gz" }
--			{ filename = "Textures/Title_Warning_as.dds.gz" }
--		resource "FileTracker" "Title_Medical"
--			"chinese_t" { filename = "Textures/Title_Medical_tc.dds.gz" }
--			"korean" { filename = "Textures/Title_Medical_korean.dds.gz" }
--			{ filename = "Textures/Title_Medical_as.dds.gz" }
--		resource "FileTracker" "Title_Controller"
--			"chinese_t" { filename = "Textures/Title_Controller_tc.dds.gz" }
--			"korean" { filename = "Textures/Title_Controller_korean.dds.gz" }
--			{ filename = "Textures/Title_Controller_as.dds.gz" }
--		resource "FileTracker" "Title_Save"
--			"chinese_t" { filename = "Textures/Title_SaveMessage_chinese_t.dds.gz" }
--			"korean" { filename = "Textures/Title_SaveMessage_korean.dds.gz" }
--			{ filename = "Textures/Title_SaveMessage_AS_english.dds.gz" }
--	elseif "UA" == REGION then
--		resource "FileTracker" "Title_ESRB" { filename = "Textures/Title_ESRB.dds.gz" }
--		resource "FileTracker" "Title_Epilepsy"
--			"french" { filename = "Textures/Title_Epilepsy_french_ua.dds.gz" }
--			"portuguese" { filename = "Textures/Title_Epilepsy_portuguese.dds.gz" }
--			"spanish" { filename = "Textures/Title_Epilepsy_spanish_ua.dds.gz" }
--			{ filename = "Textures/Title_Epilepsy.dds.gz" }
--		resource "FileTracker" "Title_Save"
--			"french" { filename = "Textures/Title_SaveMessage_UA_french.dds.gz" }
--			"portuguese" { filename = "Textures/Title_SaveMessage_UA_portuguese.dds.gz" }
--			"spanish" { filename = "Textures/Title_SaveMessage_UA_spanish.dds.gz" }
--			{ filename = "Textures/Title_SaveMessage_UA.dds.gz" }
--	elseif "EU" == REGION then
--		resource "FileTracker" "Title_Save"
--			"danish" { filename = "Textures/Title_SaveMessage_danish.dds.gz" }
--			"dutch" { filename = "Textures/Title_SaveMessage_dutch.dds.gz" }
--			"finnish" { filename = "Textures/Title_SaveMessage_finnish.dds.gz" }
--			"french" { filename = "Textures/Title_SaveMessage_french.dds.gz" }
--			"german" { filename = "Textures/Title_SaveMessage_german.dds.gz" }
--			"italian" { filename = "Textures/Title_SaveMessage_italian.dds.gz" }
--			"norwegian" { filename = "Textures/Title_SaveMessage_norwegian.dds.gz" }
--			"polish" { filename = "Textures/Title_SaveMessage_polish.dds.gz" }
--			"portuguese" { filename = "Textures/Title_SaveMessage_portuguese.dds.gz" }
--			"russian" { filename = "Textures/Title_SaveMessage_russian.dds.gz" }
--			"spanish" { filename = "Textures/Title_SaveMessage_spanish.dds.gz" }
--			"swedish" { filename = "Textures/Title_SaveMessage_swedish.dds.gz" }
--			"turkish" { filename = "Textures/Title_SaveMessage_turkish.dds.gz" }
--			"arabic" { filename = "Textures/Title_SaveMessage_arabic.dds.gz" }
--			{ filename = "Textures/Title_SaveMessage.dds.gz" }
--	end

if "JP" == REGION then
	resource "FileTracker" "Title_SonyLogo" { filename = "Textures/Title_SonyLogo.dds.gz" }
elseif "AS" == REGION then
	-- It appears this logo isn't translated in Asia.
	resource "FileTracker" "Title_SonyLogo" { filename = "Textures/Title_SonyLogo.dds.gz" }
elseif "CH" == REGION then
	-- It appears this logo isn't translated in China.
	resource "FileTracker" "Title_SonyLogo" { filename = "Textures/Title_SonyLogo.dds.gz" }
elseif "UA" == REGION then
	resource "FileTracker" "Title_SonyLogo"
		"french" { filename = "Textures/Title_SonyLogo_french.dds.gz" }
		"portuguesebr" { filename = "Textures/Title_SonyLogo_portuguese.dds.gz" }
		"spanishla" { filename = "Textures/Title_SonyLogo_spanish.dds.gz" }
		{ filename = "Textures/Title_SonyLogo.dds.gz" }
elseif "EU" == REGION then
	resource "FileTracker" "Title_SonyLogo"
		"danish" { filename = "Textures/Title_Eu_SonyLogo_danish.dds.gz" }
		"dutch" { filename = "Textures/Title_Eu_SonyLogo_dutch.dds.gz" }
		"finnish" { filename = "Textures/Title_Eu_SonyLogo_finnish.dds.gz" }
		"french" { filename = "Textures/Title_Eu_SonyLogo_french.dds.gz" }
		"german" { filename = "Textures/Title_Eu_SonyLogo_german.dds.gz" }
		"norwegian" { filename = "Textures/Title_Eu_SonyLogo_norwegian.dds.gz" }
		"italian" { filename = "Textures/Title_Eu_SonyLogo_italian.dds.gz" }
		"polish" { filename = "Textures/Title_Eu_SonyLogo_polish.dds.gz" }
		"portuguese" { filename = "Textures/Title_Eu_SonyLogo_portuguese.dds.gz" }
		"russian" { filename = "Textures/Title_Eu_SonyLogo_russian.dds.gz" }
		"spanish" { filename = "Textures/Title_Eu_SonyLogo_spanish.dds.gz" }
		"swedish" { filename = "Textures/Title_Eu_SonyLogo_swedish.dds.gz" }
		"turkish" { filename = "Textures/Title_Eu_SonyLogo_turkish.dds" }
		"arabic" { filename = "Textures/Title_Eu_SonyLogo_arabic.dds" }
		{ filename = "Textures/Title_Eu_SonyLogo.dds.gz" }
end
resource "FileTracker" "Title_ApiLogo" { filename = "Textures/Title_ApiLogo.pvr" }
resource "FileTracker" "Title_SMS_OROPS" { filename = "Textures/Title_SMS_OROPS.pvr" }
resource "FileTracker" "Title_SmLogo" { filename = "Textures/Title_SmLogo.pvr" }
resource "FileTracker" "Title_TgcGraphic" { filename = "Textures/Title_TgcGraphic.pvr" }
resource "FileTracker" "Title_TgcLabel" { filename = "Textures/Title_TgcLabel.pvr" }

resource "FileTracker" "Title_Black" { filename = "Textures/Title_Black.pvr" }
resource "FileTracker" "Title_White" { filename = "Textures/Title_White.pvr" }



if "CH" == REGION then
resource "Texture" "Title_Medical" { source = "Title_Medical_Ch.dds" }
end


--	if "JP" == REGION then
--		resource "Texture" "Title_Warning" { source = "Title_WarningJp.dds" }
--		resource "Texture" "Title_Medical" { source = "Title_MedicalJp.dds" }
--		resource "Texture" "Title_Controller" { source = "Title_ControllerJp.dds" }
--		resource "Texture" "Title_Save" { source = "Title_SaveMessage_japanese.dds" }
--		resource "Texture" "Title_Dolby" { source = "Title_Dolby_jp.dds" }
--	elseif "AS" == REGION then
--		resource "Texture" "Title_Warning"
--			"chinese_t" { source = "Title_Warning_tc.dds" }
--			"korean" { source = "Title_Warning_korean.dds" }
--			{ source = "Title_Warning_as.dds" }
--		resource "Texture" "Title_Medical"
--			"chinese_t" { source = "Title_Medical_tc.dds" }
--			"korean" { source = "Title_Medical_korean.dds" }
--			{ source = "Title_Medical_as.dds" }
--		resource "Texture" "Title_Controller"
--			"chinese_t" { source = "Title_Controller_tc.dds" }
--			"korean" { source = "Title_Controller_korean.dds" }
--			{ source = "Title_Controller_as.dds" }
--		resource "Texture" "Title_Save"
--			"chinese_t" { source = "Title_SaveMessage_chinese_t.dds" }
--			"korean" { source = "Title_SaveMessage_korean.dds" }
--			{ source = "Title_SaveMessage_AS_english.dds" }
--	elseif "UA" == REGION then
--		resource "Texture" "Title_ESRB" { source = "Title_ESRB.dds" }
--		resource "Texture" "Title_Epilepsy"
--			"french" { source = "Title_Epilepsy_french_ua.dds" }
--			"portuguese" { source = "Title_Epilepsy_portuguese.dds" }
--			"spanish" { source = "Title_Epilepsy_spanish_ua.dds" }
--			{ source = "Title_Epilepsy.dds" }
--		resource "Texture" "Title_Save"
--			"french" { source = "Title_SaveMessage_UA_french.dds" }
--			"portuguese" { source = "Title_SaveMessage_UA_portuguese.dds" }
--			"spanish" { source = "Title_SaveMessage_UA_spanish.dds" }
--			{ source = "Title_SaveMessage_UA.dds" }
--	elseif "EU" == REGION then
--		resource "Texture" "Title_Save"
--			"danish" { source = "Title_SaveMessage_danish.dds" }
--			"dutch" { source = "Title_SaveMessage_dutch.dds" }
--			"finnish" { source = "Title_SaveMessage_finnish.dds" }
--			"french" { source = "Title_SaveMessage_french.dds" }
--			"german" { source = "Title_SaveMessage_german.dds" }
--			"italian" { source = "Title_SaveMessage_italian.dds" }
--			"norwegian" { source = "Title_SaveMessage_norwegian.dds" }
--			"polish" { source = "Title_SaveMessage_polish.dds" }
--			"portuguese" { source = "Title_SaveMessage_portuguese.dds" }
--			"russian" { source = "Title_SaveMessage_russian.dds" }
--			"spanish" { source = "Title_SaveMessage_spanish.dds" }
--			"swedish" { source = "Title_SaveMessage_swedish.dds" }
--			"turkish" { source = "Title_SaveMessage_turkish.dds" }
--			"arabic" { source = "Title_SaveMessage_arabic.dds" }
--			{ source = "Title_SaveMessage.dds" }
--	end

if "JP" == REGION then
	resource "Texture" "Title_SonyLogo" { source = "Title_SonyLogo.dds" }
elseif "AS" == REGION then
	-- It appears this logo isn't translated in Asia.
	resource "Texture" "Title_SonyLogo" { source = "Title_SonyLogo.dds" }
elseif "CH" == REGION then
	-- It appears this logo isn't translated in China.
	resource "Texture" "Title_SonyLogo" { source = "Title_SonyLogo.dds" }
elseif "UA" == REGION then
	resource "Texture" "Title_SonyLogo"
		"french" { source = "Title_SonyLogo_french.dds" }
		"portuguesebr" { source = "Title_SonyLogo_portuguese.dds" }
		"spanishla" { source = "Title_SonyLogo_spanish.dds" }
		{ source = "Title_SonyLogo.dds" }
elseif "EU" == REGION then
	resource "Texture" "Title_SonyLogo"
		"danish" { source = "Title_Eu_SonyLogo_danish.dds" }
		"dutch" { source = "Title_Eu_SonyLogo_dutch.dds" }
		"finnish" { source = "Title_Eu_SonyLogo_finnish.dds" }
		"french" { source = "Title_Eu_SonyLogo_french.dds" }
		"german" { source = "Title_Eu_SonyLogo_german.dds" }
		"norwegian" { source = "Title_Eu_SonyLogo_norwegian.dds" }
		"italian" { source = "Title_Eu_SonyLogo_italian.dds" }
		"polish" { source = "Title_Eu_SonyLogo_polish.dds" }
		"portuguese" { source = "Title_Eu_SonyLogo_portuguese.dds" }
		"russian" { source = "Title_Eu_SonyLogo_russian.dds" }
		"spanish" { source = "Title_Eu_SonyLogo_spanish.dds" }
		"swedish" { source = "Title_Eu_SonyLogo_swedish.dds" }
		"turkish" { source = "Title_Eu_SonyLogo_turkish.dds" }
		"arabic" { source = "Title_Eu_SonyLogo_arabic.dds" }
		{ source = "Title_Eu_SonyLogo.dds" }
end

resource "Texture" "Title_ApiLogo" { source = "Title_ApiLogo.dds" }
resource "Texture" "Title_SMS_OROPS" { source = "Title_SMS_OROPS.dds" }
resource "Texture" "Title_SmLogo" { source = "Title_SmLogo.dds" }
resource "Texture" "Title_TgcGraphic" { source = "Title_TgcGraphic.dds" }
resource "Texture" "Title_TgcLabel" { source = "Title_TgcLabel.dds" }

resource "Texture" "Title_Black" { source = "Title_Black.dds" }
resource "Texture" "Title_White" { source = "Title_White.dds" }

resource "Texture" "ffwdicon" { source = "ffwdicon.pvr" }

