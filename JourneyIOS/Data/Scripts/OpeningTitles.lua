-------------------------------------------------------------------------------
-- Notes:  Some functions or data or something for controlling how the titles
--         are displayed.
--
-- Author: John Edwards
-- Date C: 8-17-2007
-------------------------------------------------------------------------------

local titles = {}

table.insert( titles, {	"Title_Black",		"Title_Black",		1, 0, 0, { 0, 0, 0 } } )
table.insert( titles, {	"Title_SMS_OROPS",	"Title_SMS_OROPS",	4, 1.5, 1.5, { 0, 0, 0 }, 1 } )
table.insert( titles, {	"Title_ApiLogo",	"Title_ApiLogo",	4, 1.5, 1.5, { 0, 0, 0 }, 1 } )
table.insert( titles, {	"Title_TgcGraphic",	"Title_TgcLabel",	6.0, 1, 1, { 0, 0, 0 }, 1 } )


--table.insert( titles, { "Title_SonyLogo",	"Title_SonyLogo",	4, 1.5, 1.5, { 0, 0, 0 }, 1 } )
--table.insert( titles, { "Title_SmLogo",	"Title_SmLogo",		4, 1.5, 1.5, { 0, 0, 0 }, 1 } )

if "CH" == REGION then
	local languageName = SystemLanguageName()
	if (languageName == "chinese_s")  then
		table.insert( titles, { "Title_Medical", "Title_Medical", 8, 2, 2, { 0, 0, 0 }, 1 } )
	end
end
if "UA" == REGION and DEMO and game:IsDemo() then
	table.insert( titles, { "Title_Black", "Title_Black", 2.1, 0.1, 2, { 0, 0, 0 }, 1 } )
else
table.insert( titles, { "Title_Black", "Title_Black", 1.01, 0.01, 1.0, { 0, 0, 0 }, 1 } )
end

function OpeningTitles( ss )
	for i,v in pairs( titles ) do
		local slide = ss:AddSlide( v[1], v[2], v[3], v[4], v[5], v[6] )
		if v[7] then slide:flags( v[7] ) end
	end
end
