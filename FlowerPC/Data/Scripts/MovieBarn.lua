-------------------------------------------------------------------------------
-- Notes:  Data file for Movie Barn
--
-- Author: Martin Middleton
-- Date C: 04-10-2008
-------------------------------------------------------------------------------

local movieData = 
{
	["Level_1.lua"] = { start = "Movie01", finish = false, vol = 0.85 }
,	["Level_2.lua"] = { start = "Movie02", finish = false, vol = 0.65 }
,	["Level_3.lua"] = { start = "Movie03", finish = false, vol = 0.6 }
,	["Level_4.lua"] = { start = "Movie04", finish = false, vol = 0.55 }
,	["Level_5.lua"] = { start = "Movie05", finish = false, vol = 0.6 }
,	["Level_6.lua"] = { start = false, finish = "Movie06", vol = 0.6 }
,	["Level_7.lua"] = { start = false, finish = false, vol = 1.0 }
}

function PlayLevelMovie( movieBarn, levelName, isOutro, resources )
	local levelMovies = movieData[ levelName ]
	local movieName = {}
	if isOutro then movieName = levelMovies.finish
	else movieName = levelMovies.start end
	local audioVol = levelMovies.vol
	if movieName then 
		local audioName = movieName.."Audio"
		movieBarn:PlayMovie( movieName, audioName, resources, audioVol ) 
		return true
	end
	return false
end
