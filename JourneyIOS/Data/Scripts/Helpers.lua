-------------------------------------------------------------------------------
-- Notes:  Helper functions for lua
--
-- Author: Michael Peddicord
-- Date C: 8-16-2010
-------------------------------------------------------------------------------

function GetDataFromName( name )
	return Names[name]
end

function PrintNameTableSize( )
	local sizeAccum = 0;
		for i, val in pairs( Names ) do
			sizeAccum = sizeAccum + 1
		end
	DebugPrint( sizeAccum )
end

-- Misc functions called from web pages
local levelLoadFuns = {}

function AddLevelLoadFun( fun )
	table.insert( levelLoadFuns, fun )
end

function RunLevelLoadFuns()
	for i,fun in ipairs( levelLoadFuns ) do
		fun()
	end
	levelLoadFuns = {}
end
