-------------------------------------------------------------------------------
-- Notes:  Some functions or data or something for controlling how the titles
--         are displayed.
--
-- Author: John Edwards
-- Date C: 8-17-2007
-------------------------------------------------------------------------------

-- NOTE: BECAUSE OF FADE INS AND OUTS, THE TIME A TITLE IS DISPLAYED NEEDS TO 
-- BE 1 SECOND LONGER THAN IS INTENDED!

local titles = {}

table.insert( titles, { "Black", 0.25, 0, 0, { 0, 0, 0 }, false } )

if "JP" == REGION then
	--table.insert( titles, { "Title_Warning", 4.0, 1, 1, { 0, 0, 0 }, false, 1 } )
	table.insert( titles, { "Title_SonyLogo", 4.0, 1, 1, { 0, 0, 0 }, false, 1 } )
	table.insert( titles, { "Title_SmLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_TgcLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_ControlScheme", 6.0, 1, 1, { 0, 0, 0 }, true, 1 } )
end
if "UA" == REGION or "AS" == REGION then
	table.insert( titles, { "Title_SonyLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_SmLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_TgcLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_ControlScheme", 6.0, 1, 1, { 0, 0, 0 }, false, 1 } )
end
if "EU" == REGION then
	--table.insert( titles, { "Title_Epilepsy", 6.0, 1, 1, { 0, 0, 0 }, false } )
	table.insert( titles, { "Title_SonyLogo", 4.0, 1, 1, { 0, 0, 0 }, false, 1 } )
	table.insert( titles, { "Title_SmLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_TgcLogo", 4.0, 1, 1, { 0, 0, 0 }, true, 1 } )
	table.insert( titles, { "Title_ControlScheme", 6.0, 1, 1, { 0, 0, 0 }, true, 1 } )
end

table.insert( titles, { "Black", 7, 0, 6, { 0, 0, 0 }, false, 1 } )

function OpeningTitles( ss )
	for i,v in pairs( titles ) do
		local slide = ss:AddSlide( v[1], v[2], v[3], v[4], v[5], v[6] )
		if v[7] then slide:flags( v[7] ) end
	end
end

