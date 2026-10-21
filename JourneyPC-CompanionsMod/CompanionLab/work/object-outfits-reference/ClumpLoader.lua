function LoadClumps( clumpBarn, lua )
	Mod.DoLevelObjects("Clump",lua)

	for i,ClumpDesc in ipairs( ClumpInstances ) do
		local clumpPtr = clumpBarn:CreateClump(  )
		--clumpBarn:AllocateClumpStorage( clumpPtr, #ClumpDesc.Objects )

        --Fill in the name table so triggers can refer to clumps
		Names[ ClumpDesc.ObjectName ] = clumpPtr

	end
end

--build clumps out of the resolved name table
function SetClumpMembers( clumpBarn, lua )
	print("SetClumpMembers")
	Mod.DoLevelObjects("Clump",lua)

	for i,ClumpDesc in ipairs( ClumpInstances ) do
	
		-- Traverse the clump table and compute how many objects will be in the clump
		local clumpSize = 0
		for j,ClumpMember in ipairs( ClumpDesc.Objects ) do
			if type( Names[ ClumpMember ] ) == "table" then
				clumpSize = clumpSize + #Names[ ClumpMember ]
			else
				clumpSize = clumpSize + 1
			end
		end
		
		clumpBarn:AllocateClumpStorage( Names[ ClumpDesc.ObjectName ], clumpSize )
	
		local idx = 0
	    for j,ClumpMember in ipairs( ClumpDesc.Objects ) do
			-- @Added by Nick on 3.17.08 to support multiple objects at one Names table entry
			if type( Names[ ClumpMember ] ) == "table" then
				for k,SubMember in ipairs( Names[ ClumpMember] ) do
					local classPtr = Class.cast( SubMember )
					Names[ ClumpDesc.ObjectName ]:SetAt( idx, classPtr )
					idx = idx + 1
				end
			else
				local classPtr = Class.cast( Names[ ClumpMember ] )
				Names[ ClumpDesc.ObjectName ]:SetAt( idx, classPtr )
				idx = idx + 1
			end	   
		end

    end

    --Free the memory taken by the ClumpInstances table
    ClumpInstances = {}
end
