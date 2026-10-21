-------------------------------------------------------------------------------
-- Notes:  Lua script for fading to black slideshow
--
-- Author: John
-- Date C: 8-22-2008
-------------------------------------------------------------------------------

local titles =
{ { "Black", 4, 3, 1, { 0, 0, 0 } }
, { "Black", 2, 0, 2, { 0, 0, 0 } }
}

function FadeOut( ss )
	for i,v in pairs( titles ) do
		ss:AddSlide( v[1], v[2], v[3], v[4], v[5] )
	end
end

