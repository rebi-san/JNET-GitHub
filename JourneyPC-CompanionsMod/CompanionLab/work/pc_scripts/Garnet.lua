-------------------------------------------------------------------------------
-- Notes:  Helper functions for Garnet
--
-- Author: Michael Peddicord
-- Date C: 10-26-2010
-------------------------------------------------------------------------------

function ParseServerCommand( jsonString )
	--print( "ParseServerCommand "..jsonString )
	for matches in string.gmatch(jsonString, "%b{}") do	
		local dataIn = Json.Decode( matches )
		if dataIn.type == "uniqueId" then
			game:pweb3():SetSessionId(dataIn.data)
		elseif dataIn.type == "lua" then
			assert(loadstring(dataIn.data))()
		elseif dataIn.type == "json" then
			local cmdName = dataIn.data.cmd
			local cmd = _G[ cmdName ]
			local args = dataIn.data.args
			cmd( unpack( args ) )
		end			
	end
end