-------------------------------------------------------------------------------
-- Notes:  Handles loading waypoints into the barn
--
-- Author: Martin Middleton
-- Date C: 01-30-2008
-------------------------------------------------------------------------------

local kDefaultVisitRadius = 5.0

function ReloadWaypoints( waypointBarn )
	
	waypointBarn:ResetWaypoints()
	
	for i,poi in ipairs( WaypointInstances ) do
		local pos = poi.Position
		local waypoint = waypointBarn:AddWaypoint()
		waypoint:SetPos( pos[ 1 ], pos[ 2 ], pos[ 3 ] )
		waypoint:SetVisited( not poi.Enabled )
		waypoint:SetRadius( poi.Radius )
		Names[ poi.ObjectName ] = waypoint
	end

	WaypointInstances = {}
	
end
