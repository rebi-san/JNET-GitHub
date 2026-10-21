-------------------------------------------------------------------------------
-- Notes:  We want a longer fadeout if we are in the middle of the movie
--
-- Author: Martin Middleton
-- Date C: 12-08-2008
-------------------------------------------------------------------------------

local titles =
{ { "Black", 1.0, 1.0, 0, { 0, 0, 0 } }
, { "Black", 2.5, 0, 2.5, { 0, 0, 0 } }
}

function FadeSkipMovie( ss )
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