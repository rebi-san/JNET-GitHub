function LoadMarkers( MarkerBarn, lua )
	print("LoadMarkers")
	Mod.DoLevelObjects("Marker",lua)--Mod

	for i, v in ipairs( MarkerInstances ) do
		local markerPtr = MarkerBarn:CreateMarker()
		MarkerInitialize( markerPtr, v )
		Names[ v.ObjectName ] = markerPtr
	end
	MarkerInstances = { }
end

function MarkerInitialize( marker, desc )
	marker:SetPosition( { desc.Position[ 1 ], desc.Position[ 2 ], desc.Position[ 3 ] } )
	marker:SetQuat( desc.Quaternion[ 1 ], desc.Quaternion[ 2 ], desc.Quaternion[ 3 ], desc.Quaternion[ 4 ] )
	if desc.Scale ~= nil then
		marker:SetScale( { desc.Scale[ 1 ], desc.Scale[ 2 ], desc.Scale[ 3 ] } )
	end
	if desc.Enabled ~= nil then
		marker:enabled( desc.Enabled )
	end
end