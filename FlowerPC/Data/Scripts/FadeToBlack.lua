-------------------------------------------------------------------------------
-- Notes:  Lua script for fading to black slideshow
--
-- Author: Martin Middleton
-- Date C: 11-30-2007
-------------------------------------------------------------------------------

local titles =
{ { "Black", 0.8, 0.0, 0, { 0, 0, 0 } }
, { "Black", 3.0, 0.5, 2.5, { 0, 0, 0 } }
}

function FadeToBlack( ss )
	for i,v in pairs( titles ) do
		ss:AddSlide( v[1], v[2], v[3], v[4], v[5] )
	end
end

function GetSoundFadeInTime()
	return ( titles[ 1 ][ 2 ] + titles[ 2 ][ 2 ] )
end

function GetMovieFadeOutTime()
	return ( titles[ 1 ][ 3 ] )
end
