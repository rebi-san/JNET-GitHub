-------------------------------------------------------------------------------

-- Notes:  A list of resources that are loaded before the opening titles are

--         displayed, and then unloaded afterwards (ideally).

--

-- Author: John Edwards

-- Date C: 8-17-2007

-------------------------------------------------------------------------------



resource "Shader" "SlideShow" { vs = "Resample_vs.cg", fs = "SlideShow_fs.cg", instanceLimit = 1 }



resource "Script" "SlideRender" { source = "SlideRender.lua" }



resource "Script" "OpeningTitles" { source = "OpeningTitles.ios64.lua" }



if "UA" == REGION or "AS" == REGION then

	resource "FileTracker" "Title_Epilepsy"

		"french" { filename = "Textures/Title_Epilepsy_french.dds" }

		"italian" { filename = "Textures/Title_Epilepsy_italian.dds" }

		"german" { filename = "Textures/Title_Epilepsy_german.dds" }

		"spanish" { filename = "Textures/Title_Epilepsy_spanish.dds" }

		"dutch" { filename = "Textures/Title_Epilepsy_dutch.dds" }

		"portuguese" { filename = "Textures/Title_Epilepsy_portuguese.dds" }

		"russian" { filename = "Textures/Title_Epilepsy_russian.dds" }

		{ filename = "Textures/Title_Epilepsy.dds" }

end



if "UA" == REGION or "AS" == REGION then

	resource "FileTracker" "Title_Esrb" { filename = "Textures/Title_Esrb.dds" }

end



if "UA" == REGION or "EU" == REGION or "AS" == REGION then

	resource "FileTracker" "Title_SaveMessage"

		"french" { filename = "Textures/Title_SaveMessage_french.dds" }

		"italian" { filename = "Textures/Title_SaveMessage_italian.dds" }

		"german" { filename = "Textures/Title_SaveMessage_german.dds" }

		"spanish" { filename = "Textures/Title_SaveMessage_spanish.dds" }

		"dutch" { filename = "Textures/Title_SaveMessage_dutch.dds" }

		"portuguese" { filename = "Textures/Title_SaveMessage_portuguese.dds" }

		"russian" { filename = "Textures/Title_SaveMessage_russian.dds" }

		{ filename = "Textures/Title_SaveMessage.dds" }

end

if "JP" == REGION then

	resource "FileTracker" "Title_Warning" { filename = "Textures/Title_WarningJp.dds" }

end



if "UA" == REGION or "AS" == REGION then

	resource "FileTracker" "Title_AILogo"

		"french" { filename = "Textures/Title_AILogo.dds" }

		"spanish" { filename = "Textures/Title_AILogo.dds" }

		"portuguese" { filename = "Textures/Title_AILogo.dds" }

		{ filename = "Textures/Title_AILogo.dds" }

elseif "EU" == REGION then

	resource "FileTracker" "Title_AILogo"

		"french" { filename = "Textures/Title_AILogo.dds" }

		"italian" { filename = "Textures/Title_AILogo.dds" }

		"german" { filename = "Textures/Title_AILogo.dds" }

		"spanish" { filename = "Textures/Title_AILogo.dds" }

		"dutch" { filename = "Textures/Title_AILogo.dds" }

		"portuguese" { filename = "Textures/Title_AILogo.dds" }

		"russian" { filename = "Textures/Title_AILogo.dds" }

		{ filename = "Textures/Title_AILogo.dds" }

elseif "JP" == REGION then

	resource "FileTracker" "Title_AILogo" { filename = "Textures/Title_AILogo.dds" }

end



resource "FileTracker" "Title_SonyLogo" { filename = "Textures/Title_SonyLogo.dds" }

resource "FileTracker" "Title_TgcGraphic" { filename = "Textures/Title_TgcGraphic.dds" }

resource "FileTracker" "Title_TgcLabel" { filename = "Textures/Title_TgcLabel.dds" }



-- Japanese PS4 control scheme has legal text on it

if "JP" == REGION and "PS4"==TARGET then

	resource "FileTracker" "Title_ControlScheme" { filename = "Textures/Title_ControlScheme_JP.dds" }

else

	resource "FileTracker" "Title_ControlScheme" { filename = "Textures/Title_ControlScheme.dds" }

end



resource "FileTracker" "Black" { filename = "Textures/Black.dds" }

resource "FileTracker" "White" { filename = "Textures/White.dds" }


resource "Texture" "allstrings"
    "arabic" { source = "alltext_arabic.dds", vram=false , nomipmap=true}
    "chinesesimp" { source = "alltext_chinesesimp.dds", vram=false , nomipmap=true}
    "chinesetrad" { source = "alltext_chinesetrad.dds", vram=false , nomipmap=true}
    "dutch" { source = "alltext_dutch.dds", vram=false , nomipmap=true}
    "french" { source = "alltext_french.dds", vram=false , nomipmap=true}
    "german" { source = "alltext_german.dds", vram=false , nomipmap=true}
    "italian" { source = "alltext_italian.dds", vram=false , nomipmap=true}
    "portuguesebr" { source = "alltext_ptbr.dds", vram=false , nomipmap=true}
    "polish" { source = "alltext_polish.dds", vram=false , nomipmap=true}
    "russian" { source = "alltext_russian.dds", vram=false , nomipmap=true}
    "spanish" { source = "alltext_spanish.dds", vram=false , nomipmap=true}
    "spanishla" { source = "alltext_spanishla.dds", vram=false , nomipmap=true}
    "swedish" { source = "alltext_swedish.dds", vram=false , nomipmap=true}
    "turkish" { source = "alltext_turkish.dds", vram=false , nomipmap=true}
    "thai" { source = "alltext_thai.dds", vram=false , nomipmap=true}
    "japanese" { source = "alltext_japanese.dds", vram=false , nomipmap=true}
{ source = "alltext_english.dds", vram=false , nomipmap=true}

