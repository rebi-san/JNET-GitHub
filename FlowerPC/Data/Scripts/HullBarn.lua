-------------------------------------------------------------------------------
-- Notes:  Defines a function for filling up a HullBarn with Hull instances.
--
-- Author: John Edwards
-- Date C: 1-29-2007
-------------------------------------------------------------------------------

function HullBarnInitialize( hullBarn, instanceTblName, hullType )

	local hulls = _G[ instanceTblName ]

	-- We are changing to only load one bin per barn
	local binIdx = 1

	-- Separate all the hulls in the desc table into groups with total size
	-- of 16kb or less (so they can be shipped to the SPUs).
	-- We check against 15kb to give the allocator some slop room.
	local hullGroups = {}
	local currentGroup = {}
	local currentSize = 0
	for i,desc in ipairs( hulls ) do
		local hullSize = SizeOfHull( #desc.Planes / 3, #desc.Edges / 2, #desc.Verts / 3 )
		assert( hullSize < 15 * 1024 )

		local newSize = currentSize + hullSize
		if newSize >= 15 * 1024 then
			table.insert( hullGroups, currentGroup )
			currentGroup = {}
			currentSize = 0
		end

		currentSize = currentSize + hullSize
		table.insert( currentGroup, desc )
	end
	if currentSize > 0 then table.insert( hullGroups, currentGroup ) end

	-- Make sure we aren't putting too many hulls in a single bin.
	assert( #hullGroups <= HULL_BARN_ARRAYS_PER_BIN )

	-- Start filling the bins.
	for groupIdx,group in ipairs( hullGroups ) do
		hullBarn:AllocateHullArray( groupIdx - 1, #group )

		for hullIdx,hullDesc in ipairs( group ) do

			local hull = hullBarn:InitializeHull(
				groupIdx - 1,
				#hullDesc.Planes / 3,
				#hullDesc.Edges / 2,
				#hullDesc.Verts / 3 )

			if "Boundary" == hullType then hull:SetType( BoundaryHullType ) end

			hull:SetActive( hullDesc.Enabled )
			hull:SetStrength( hullDesc.Strength )
			hull:SetDir( hullDesc.Direction[ 1 ], hullDesc.Direction[ 2 ], hullDesc.Direction[ 3 ] )

			for j = 1,#hullDesc.Planes,3 do
				hull:SetPlane( (j-1)/3, hullDesc.Planes[ j + 0 ], hullDesc.Planes[ j + 1 ], hullDesc.Planes[ j + 2 ] )
			end

			for j = 1,#hullDesc.Edges,2 do
				hull:SetEdge( (j-1)/2, hullDesc.Edges[ j + 0 ], hullDesc.Edges[ j + 1 ] )
			end

			local vert = {}
			for j = 1,#hullDesc.Verts,3 do
				vert[1] = hullDesc.Verts[ j + 0 ]
				vert[2] = hullDesc.Verts[ j + 1 ]
				vert[3] = hullDesc.Verts[ j + 2 ]
				hull:SetVert( (j-1)/3, vert )
			end

			-- For Events/Triggers.
			-- If we want multiple hulls to exist at the same location in the Names
			-- table, we will need to create a table and insert them into it. 
			if Names[ hullDesc.ObjectName ] ~= nil then
				if type( Names[ hullDesc.ObjectName ] ) ~= "table" then
					Names[ hullDesc.ObjectName ] = { Names[ hullDesc.ObjectName ] }
				end
				table.insert( Names[ hullDesc.ObjectName ], hull )
			else
				Names[ hullDesc.ObjectName ] = hull
			end

		end
	end

	-- This is necessary, as the hull instances table can get quite large,
	-- and possibly cause a memory overflow.
	_G[ instanceTblName ] = nil
	hulls = nil
	collectgarbage( "collect" )
end

