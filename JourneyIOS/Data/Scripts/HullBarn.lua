-------------------------------------------------------------------------------
-- Notes:  Defines a function for filling up a HullBarn with Hull instances.
--
-- Author: John Edwards
-- Date C: 1-29-2007
-------------------------------------------------------------------------------

local function GammaToLinearVec3_Div255( v )
	return GammaToLinearVec3( vec3_scalar_mult( v, 1 / 255 ) )
end

function PutHullsInNamesTable( hullBarn )
	local hull = hullBarn:GetFirstHull()
	while hull ~= nil do
		-- For Events/Triggers.
		-- If we want multiple hulls to exist at the same location in the Names
		-- table, we will need to create a table and insert them into it. 
		local hullName = hull:GetName()
		if Names[ hullName ] ~= nil then
			if type( Names[ hullName ] ) ~= "table" then
				Names[ hullName ] = { Names[ hullName ] }
			end
			table.insert( Names[ hullName ], hull )
		else
			Names[ hullName ] = hull
		end
		hull = hull:GetNextHull()
	end
end

function HullReinitialize( hull, desc )
	-- If the numbers of planes/edges/verts have changed, we have to reallocate memory.
	-- Reallocating hull memory is currently too complex, so instead we just bail.
	if hull:GetTriCount() ~= #desc.Planes / 3 or
	   hull:GetPlaneCount() ~= #desc.TriCounts or
	   hull:GetEdgeCount() ~= #desc.Edges / 2 or
	   hull:GetVertCount() ~= #desc.Verts / 3 then
		DebugPrintError( "Failed to reinitialize hull "..desc.ObjectName..", plane/edge/vert count changed." )
		return
	end

	hull:SetStrength( desc.Strength )
	hull:SetDir( desc.Direction[ 1 ], desc.Direction[ 2 ], desc.Direction[ 3 ] )
	local lightColor = GammaToLinearVec3_Div255( desc.LightColor )
	hull:SetLightColor( lightColor[ 1 ], lightColor[ 2 ], lightColor[ 3 ] )
	hull:SetActive( desc.Enabled )

	hull:ResetAabb()

	for i = 1, #desc.Planes, 3 do
		hull:SetTri( (i-1)/3, desc.Planes[ i + 0 ], desc.Planes[ i + 1 ], desc.Planes[ i + 2 ] )
	end
	for i = 1, #desc.TriCounts, 1 do
		hull:SetPlane( (i-1), desc.TriCounts[ i ] )
	end
	for i = 1, #desc.Edges, 2 do
		hull:SetEdge( (i-1)/2, desc.Edges[ i + 0 ], desc.Edges[ i + 1 ] )
	end
	for i = 1, #desc.Verts, 3 do
		hull:SetVert( (i-1)/3, { desc.Verts[ i + 0 ], desc.Verts[ i + 1 ], desc.Verts[ i + 2 ] } )
	end
end

function HullValidate( hullBarn, pweb3 )
	DebugPrint( "Printing invalid hulls:" )
	local anyInvalid = false
	local hull = hullBarn:GetFirstHull()
	while hull ~= nil do
		if not hull:TryValidate() then
			DebugPrint( hull:GetName() )
			if not anyInvalid then
				local gardenerCmd = "Mel( [[ SelectHullByHash( \""..hull:GetName().."\" ); ]] )"
				for i = 0, pweb3:GetBoundDeviceCount() - 1 do
					local device = pweb3:GetBoundDevice( i )
					if device:GetType() == "gardener" then
						pweb3:Send( device:GetUniqueId(), gardenerCmd )
					end
				end
			end
			anyInvalid = true
		end
		hull = hull:GetNextHull()
	end
	if not anyInvalid then
		DebugPrint( "All hulls are valid!" )
	end
end
