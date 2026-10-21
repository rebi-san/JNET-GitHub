-------------------------------------------------------------------------------
-- Notes:  Stuff related to music
--
-- Author: Martin Middleton
-- Date C: 12-14-2007
-------------------------------------------------------------------------------
--[[
local threshData = 
{
	["Level_1.lua"] = { build = 25, fade = 35 }
,	["Level_2.lua"] = { build = 25, fade = 35 }
,	["Level_3.lua"] = { build = 25, fade = 35 }
,	["Level_4.lua"] = { build = 25, fade = 35 }
,	["Level_5.lua"] = { build = 25, fade = 15 }
,	["Level_6.lua"] = { build = 25, fade = 15 }
,	["Level_7.lua"] = { build = 25, fade = 15 }
}

function LoadMusicThresholds( musician, levelName )
	local build = 250
	local fade = 15
	if threshData[ levelName ] then
		build = threshData[ levelName ].build
		fade = threshData[ levelName ].fade
	end
	musician:m_fadeThreshold( fade )
	musician:m_buildThreshold( build )
end
]]--
