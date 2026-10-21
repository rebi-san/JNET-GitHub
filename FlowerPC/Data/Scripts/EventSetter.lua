-------------------------------------------------------------------------------
-- Notes:  A Simple function to attempt to send an event through Lua. This 
--         function should only be called on the first frame of the game.
--
-------------------------------------------------------------------------------

function CreateTriggers( eventBarn )

	-- This double loop is necessary so Triggers can refer to other Triggers by name

	for i,TrigDesc in ipairs( TriggerInstances ) do

		local event = eventBarn:MetaAddEvent( TrigDesc.TriggerType )
        -- grab the actual function out of the global variable table
		local eventImplPtr = _G[ TrigDesc.TriggerType ].cast( event )

		Names[ TrigDesc.ObjectName ] = eventImplPtr

	end

end


--reload the TriggerInstances.Lua file before this gets called
function ResolveTriggerNames( eventBarn, clumpBarn )
	TriggerShortcuts = {}

	for i,TrigDesc in ipairs( TriggerInstances ) do
		for VarName, VarVal in pairs( TrigDesc.Vars ) do
			-- Check to see if this variable actually exists in the Trigger
			if Names[ TrigDesc.ObjectName ][ VarName ] == nil then
				print( "*** Warning loading Trigger "..TrigDesc.ObjectName..", attempting to set variable "..VarName, VarVal )
			else
				-- If VarVal is a table of objects, we need to smash them into a clump.
				-- Watch out, though, because vars that return tables represent vectormath objects.
				if type( Names[ TrigDesc.ObjectName ][ VarName ]( Names[ TrigDesc.ObjectName ] ) ) ~= "table" and
				   type( VarVal ) == "table" then
					local clumpPtr = clumpBarn:CreateClump( )
					clumpBarn:AllocateClumpStorage( clumpPtr, #VarVal )
					for k,Member in ipairs( VarVal ) do
						local classPtr = Class.cast( Member )
						clumpPtr:SetAt( k-1, classPtr )
					end
					VarVal = clumpPtr
				end

				-- get the GardenerType of the variable using this nice hack
				local classinst = Names[ TrigDesc.ObjectName ]			
				local varfun = Names[ TrigDesc.ObjectName ][ VarName ]
				if not varfun then
					print( "*** Error loading Trigger "..TrigDesc.ObjectName..":"..VarName )
					break
				end
				local varinst = varfun( classinst )
				local vartype = type( varinst )
				local gardenerType = ""
				if varinst == nil or vartype == "userdata" then
					local retcode,errmsg = pcall( varfun, classinst, "baddata" )
					if not retcode then
						vartype = errmsg:match( "Expected ([^,]+)" )
					else
						vartype = "class"
					end

					gardenerType = _G[ vartype ].__metadata.GardenerType
				end

				-- Wrap single objects into clumps only if the trigger data calls for a clump
				if gardenerType == "Clump" and type( VarVal ) == "userdata" and not _G.Clump.cast( VarVal ) then
					local clumpPtr = clumpBarn:CreateClump( )	
					clumpBarn:AllocateClumpStorage( clumpPtr, 1 )
					local classPtr = Class.cast( VarVal )
					clumpPtr:SetAt( 0, classPtr )

					Names[ TrigDesc.ObjectName ][ VarName ]( Names[ TrigDesc.ObjectName ], clumpPtr )
				elseif gardenerType ~= "Clump" and type( VarVal ) == "userdata" and _G.Clump.cast( VarVal ) then
					-- Assert if we are presenting a clump but the Trigger doesn't want one
					-- (Most likely because we combined a table of Names[] entries into a clump above)
					assert( false )
				else
					Names[ TrigDesc.ObjectName ][ VarName ]( Names[ TrigDesc.ObjectName ], VarVal )
				end
			end
		end

		--@ADDED by Nick 3.10.08
		--Load in a Shortcut value, if available
		if TrigDesc.Shortcut and string.len( TrigDesc.Shortcut ) > 0 then
			if not TriggerShortcuts[ TrigDesc.Shortcut ] then
				TriggerShortcuts[ TrigDesc.Shortcut ] = {}
			end
			table.insert( TriggerShortcuts[ TrigDesc.Shortcut ], Names[ TrigDesc.ObjectName] )
		end
	end

	--To forcibly consume the Triggers
	TriggerInstances = {}

end

function printNameTableSize( )
	local sizeAccum = 0;
		for i, val in pairs( Names ) do
			sizeAccum = sizeAccum + 1
		end
	print( sizeAccum )
end

