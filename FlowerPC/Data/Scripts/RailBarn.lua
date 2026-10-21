-------------------------------------------------------------------------------
-- Notes:  Loaded by the ClumpBarn - this is a function which loads clumps from  
--         ClumpInstances.lua
--
-------------------------------------------------------------------------------

function LoadRails( railBarn )

	--print("LoadRails")
	--print( #RailInstances )
	for i, RailDesc in ipairs( RailInstances ) do
	    --print( "CreateRail" )
		local railPtr = railBarn:CreateRail(  )
		
		--print( #RailDesc.RailDataPoints ) 
		railBarn:AllocateRailPointStorage( railPtr, #RailDesc.RailDataPoints )
		railPtr:looped( RailDesc.Closed )
		
		-- fill in the rail point data
		for j, RNode in ipairs( RailDesc.RailDataPoints ) do
			railPtr:SetPoint( j-1, RNode.Position ) 
		end
		
		railBarn:SetupRailArcParam( railPtr )
   
        --Fill in the name table so triggers and clumps can refer to rails
		Names[ RailDesc.ObjectName ] = railPtr

	end

end

-- this kind of thing will only be necessary if rail nodes refer to other elements
function SetRailMembers( railBarn )

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

