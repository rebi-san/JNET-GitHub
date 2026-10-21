-------------------------------------------------------------------------------
-- Notes:  Defines a global Lua function "Tick" is called every frame from
--         within the game code.  Tick is a repository for various hacky debug
--         keys or code we don't want cluttering up actual C++.
--
-- Author: John Edwards
-- Date C: 5-10-2007
-------------------------------------------------------------------------------

-- KeyPressed()
-- A handy shortcut for seeing if a keyboard key has just been pressed.
local g_input
local function KeyPressed( c )
	return g_input:IsKeyDown( c:byte() ) and not g_input:IsOldKeyDown( c:byte() )
end

-- PrintMeta()
-- Prints out all the classes that have been exposed to Lua, as well as their
-- member variables.
function PrintMeta( out )
	local metasystem = {}

	for classname,classtbl in pairs( _G ) do
		if type( classtbl ) == "table" and classtbl.__vars then
			local class = {}

			-- abstract
			class.abstract = ( type( classtbl.new ) ~= "function" )

			-- vars
			local vars = {}
			class.vars = vars;
			if not class.abstract then
				local classinst = classtbl.new()

				for varname,varfun in pairs( classtbl.__vars ) do
					local vartable = {}

					local varinst = varfun( classinst )
					local vartype = type( varinst )
					-- See if the var is a Class pointer.
					if varinst == nil or vartype == "userdata" then
						local retcode,errmsg = pcall( varfun, classinst, "baddata" )
						if not retcode then
							vartype = errmsg:match( "Expected ([^,]+)" )
						else
							vartype = "class"
						end
					end
					-- See if the var is a Vectormath object.
					if vartype == "table" then
						local tbllen = #varinst
						local basetype = nil
						for i,v in ipairs( varinst ) do
							if type( v ) == "number" then
								if basetype and basetype ~= "Vector" then basetype = nil break end
								basetype = "Vector"
							elseif type( v ) == "table" then
								local allNumbers = true
								for ii,vv in ipairs( v ) do
									if type( vv ) ~= "number" then allNumbers = false break end
								end
								if (#v ~= tbllen or not allNumbers) or
									(basetype and basetype ~= "Matrix") then basetype = nil break end
								basetype = "Matrix"
							end
						end
						if basetype then vartype = basetype..tbllen end
					end

					local metadatatable = {}
					for metaname,metadata in pairs( varfun.__metadata ) do
						metadatatable[ metaname ] = metadata
					end

					vartable.type = vartype
					vartable.metadata = metadatatable

					vars[ varname ] = vartable
				end
			end	

			-- functions

			-- metadata
			-- ...I hope no one is dyslexic! metatadatatabatable
			local metadatatable = {}
			for metaname,metadata in pairs( classtbl.__metadata ) do
				metadatatable[ metaname ] = metadata
			end
			class.metadata = metadatatable


			--delete classinst
			--@HACK: delete isn't implemented, so we just leak a bunch of memory.

			metasystem[ classname ] = class
		end
	end

	print( "\n\n\n" )
	out:write( "-- A table of all non-abstract classes registered with the meta system.\n" )
    out:write( "Metasystem =" )
	PrintTable( out, metasystem )

	print( "\n\n\n" )
	print( "Dumping the metasystem." )
	print( "-- Warning: The current implementation of the metasystem dump has a serious memory leak." )
	print( "-- Your game may become unstable after this point." )
	print( "\n\n\n" )
end

-- DumpMetaSys()
-- moved out here so it can be called directly from the native code 
function DumpMetaSys()
	out = io.open( "/app_home/Metasystem.lua", "w+" )
	PrintMeta( out )
	out:close()
	
	LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"..\\Tools\\Gardener\\Perforce_OpenMetasystem.bat &&"
	.."move /Y Metasystem.lua ..\\Tools\\Gardener\\Metasystem.lua &&"
	.."cd ..\\Tools\\Gardener && BuildGardener.bat\"" )
end

-- DumpClassSizes()
-- Prints the sizes of all the classes registered with the metasystem to stdout.
function DumpClassSizes()
	local classSizes = {}
	for name,var in pairs( _G ) do
		if type( var ) == "table" and var.__sizeof then
			table.insert( classSizes, { name = name, size = var.__sizeof } )
		end
	end
	table.sort( classSizes, function( t0, t1 ) return t0.name < t1.name end )

	for i,classSize in ipairs( classSizes ) do
		print( "sizeof( "..classSize.name.." ): "..classSize.size )
	end
end

-- Tick()
-- Called once per frame at the end of the frame.  Gives us a hook into the
-- game.  We could potentially use this to display diagnostic info, or
-- something.
function Tick( gameTiming, input, listener, curCamKeyFrame )
	g_input = input

	if Vars.Game.cheatsEnabled() then
		if KeyPressed( 'V' ) then
			print( "Saving out tweak vars." )
			SaveVars( "/app_home/Data/Scripts/Vars.lua" )
		end

		if KeyPressed( 'T' ) then
			DumpMetaSys()
		end

		if KeyPressed( 'A' ) then
			DumpClassSizes()
		end

		-- Try to tell Gardener about the current camera attributes.
		if input:GetPad( 0 ):WasButtonPressed( Channel_Button_L1 ) then
			local pos = curCamKeyFrame:pos()
			local rot = curCamKeyFrame:rot()
			local fov = curCamKeyFrame:fov()
			local fDist = curCamKeyFrame:focusDistance()
			local fRad = curCamKeyFrame:focusRadius()

			local strpos = string.format("%.3f", pos[1])..","..string.format("%.3f", pos[2])..","..string.format("%.3f", pos[3])
			local strrot = string.format("%.3f", rot[1])..","..string.format("%.3f", rot[2])..","..string.format("%.3f", rot[3])..","..string.format("%.3f", rot[4])

			local cmd1 = "SetCamTrans( {"..strpos.."}, {"..strrot.."} );"
			local cmd2 = "SetCamVars( "..fov..", "..fDist..", "..fRad.." );"

			LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"..\\Tools\\Gardener\\Sprinkler.exe 127.0.0.1 5000 "..cmd1 )
			LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"..\\Tools\\Gardener\\Sprinkler.exe 127.0.0.1 5000 "..cmd2 )
		end	

		-- Fire any trigger shortcuts that have been activated.
		for key,tbl in pairs( TriggerShortcuts ) do
			if KeyPressed( key ) then
				for i,trigger in ipairs( TriggerShortcuts[ key ] ) do
					trigger:Start()
				end
			end
		end
	end
end

