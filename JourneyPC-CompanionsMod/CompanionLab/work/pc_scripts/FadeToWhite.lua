-------------------------------------------------------------------------------
-- Notes:  Lua script for fading to white slideshow
--
-- Author: Martin Middleton
-- Date C: 11-05-2007
-------------------------------------------------------------------------------

local titles =
{ { "Title_White", 4, 3, 1, { 1, 1, 1 } }
, { "Title_White", 2, 0, 2, { 1, 1, 1 } }
--, { "Title_Black", 4, 3, 1, { 0, 0, 0 } }
}

function FadeToWhite( ss )
	for i,v in pairs( titles ) do
		ss:AddSlide( v[1], v[2], v[3], v[4], v[5] )
	end
end
