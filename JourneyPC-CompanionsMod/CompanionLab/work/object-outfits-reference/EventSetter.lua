function CreateTriggers( eventBarn, lua )
	Mod.DoLevelObjects("Trigger",lua) --Mod

	for i,TrigDesc in ipairs( TriggerInstances ) do
		local event = eventBarn:MetaAddEvent( TrigDesc.Type )
		local eventImplPtr = _G[ TrigDesc.Type ].cast( event )

		Names[ TrigDesc.ObjectName ] = eventImplPtr
	end
end

function ResolveTriggerNames( eventBarn, clumpBarn, lua )
	Mod.DoLevelObjects("Trigger",lua)--Mod

	TriggerShortcuts = {}
	for i,TrigDesc in ipairs( TriggerInstances ) do

		TriggerInitialize( Names[ TrigDesc.ObjectName ], TrigDesc )
	end

	TriggerInstances = {}
end

function TriggerInitialize( trig, desc )
	local clumpBarn = game:clumpBarn()

	local VarNameN = "ObjectName"
	if trig[ VarNameN ] == nil then
		print( "*** Warning loading Trigger "..desc.ObjectName..", attempting to set ObjectName" )
	else
		trig[ VarNameN ]( trig, desc.ObjectName )
	end

	for VarName, VarVal in pairs( desc.Vars ) do
		-- Check to see if this variable actually exists in the Trigger
		if trig[ VarName ] == nil then
			print( "*** Warning loading Trigger "..desc.ObjectName..", attempting to set variable "..VarName, VarVal )
		else
			-- If VarVal is a table of objects, we need to smash them into a clump.
			-- Watch out, though, because vars that return tables represent vectormath objects.
			if type( trig[ VarName ]( trig ) ) ~= "table" and type( VarVal ) == "table" then
				local clumpPtr = clumpBarn:CreateClump( )
				clumpBarn:AllocateClumpStorage( clumpPtr, #VarVal )
				for k,Member in ipairs( VarVal ) do
					local classPtr = Class.cast( Member )
					clumpPtr:SetAt( k-1, classPtr )
				end
				VarVal = clumpPtr
			end

			-- get the GardenerType of the variable using this nice hack
			local classinst = trig			
			local varfun = trig[ VarName ]
			if not varfun then
				print( "*** Error loading Trigger "..desc.ObjectName..":"..VarName )
				break
			end
			
			local varinst = varfun( classinst )
			local vartype = type( varinst )
			local gardenerType = ""
			if varinst == nil or vartype == "userdata" then

				-- precached the entire lot. fuck those loading times.
				if true then
					gardenerType = Lua__ScanForType(desc.ObjectName .. "_" .. VarName)
				end

				if gardenerType==nil or gardenerType=="" then

					local retcode,errmsg = pcall( varfun, classinst, "baddata" )
					if not retcode then
						vartype = errmsg:match( "Expected ([^,]+)" )
					else
						vartype = "class"
					end
	
					gardenerType = _G[ vartype ].__metadata.GardenerType
	
						MyPrint("    { \"" .. desc.ObjectName .. "_" .. VarName .. "\", \"" .. gardenerType .. "\", },\n")

				end


			end
			
			local isPtrType = gardenerType ~= ""
			local isClump = isPtrType and VarVal ~= 0 and _G.Clump.cast( VarVal ) 
			if gardenerType == "Clump" and isPtrType and not isClump then
				if VarVal ~= 0 then
					local clumpPtr = trig[ VarName ]( trig )
					if clumpPtr == nil or clumpPtr:Size() > 1 then
						-- Wrap single objects into clumps only if the trigger data calls for a clump and a size-1 clump doesn't already exist
						clumpPtr = clumpBarn:CreateClump( )	
						clumpBarn:AllocateClumpStorage( clumpPtr, 1 )
					end
					
					local classPtr = Class.cast( VarVal )
					clumpPtr:SetAt( 0, classPtr )
					trig[ VarName ]( trig, clumpPtr )
				else
					-- We got a nil value -- don't bother creating a wrapper clump, just set the clump pointer to be nil
					trig[ VarName ]( trig, nil )
				end
			elseif gardenerType ~= "Clump" and isPtrType and isClump then
				-- Assert if we are presenting a clump but the Trigger doesn't want one
				-- (Most likely because we combined a table of Names[] entries into a clump above)
				print( "Error with trigger "..desc.ObjectName .. " Variable: " .. VarName)
				assert( false )
			else
				local trigVal = VarVal
				if VarVal == 0 and isPtrType then
					trigVal = nil
				end
				trig[ VarName ]( trig, trigVal )
			end
		end
	end

	--Load in a Shortcut value, if available
	if desc.Shortcut and string.len( desc.Shortcut ) > 0 then
		-- Remove any existing shortcut from this trigger
		for k,v in pairs( TriggerShortcuts ) do
			for i,targ in ipairs( v ) do
				if targ == trig then table.remove( v, i ) end
			end
		end
		-- Create a new shortcut table for this key, if needed
		if not TriggerShortcuts[ desc.Shortcut ] then TriggerShortcuts[ desc.Shortcut ] = {} end
		table.insert( TriggerShortcuts[ desc.Shortcut ], trig )
	end
end

function CheckTriggerShortcuts( input )
	-- Fire any trigger shortcuts that have been activated.
	for key, triggerList in pairs( TriggerShortcuts ) do
		if input:IsKeyDown( key:byte() ) and not input:IsOldKeyDown( key:byte() ) then
			for i, trigger in ipairs( triggerList ) do
				trigger:Start()
			end
		end
	end
end
