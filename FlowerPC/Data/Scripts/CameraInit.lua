-------------------------------------------------------------------------------
-- Notes:  Camera related data driven stuff
--
-- Author: Martin Middleton
-- Date C: 9-20-2007
-------------------------------------------------------------------------------



function InitCamera( camera )
	local startPos = 
	{ StartLocationInstances[ 1 ].Position[ 1 ]
	, StartLocationInstances[ 1 ].Position[ 2 ]
	, StartLocationInstances[ 1 ].Position[ 3 ] }
	local rot =
	{ StartLocationInstances[ 1 ].Rotation[ 1 ]
	, StartLocationInstances[ 1 ].Rotation[ 2 ]
	, StartLocationInstances[ 1 ].Rotation[ 3 ]
	, StartLocationInstances[ 1 ].Rotation[ 4 ] }
	local dist = 10
	
	camera:SetPos( startPos )
	camera:SetStartOrientation( rot, dist )
end

function LoadCameraKeyFrames( cameraSys )

	for i, v in ipairs( CameraKeyFrameInstances ) do

		local camKeyPtr = cameraSys:AllocateCameraKey( )
		camKeyPtr:rot(v.Rotation)
		camKeyPtr:pos(v.Position)
		camKeyPtr:fov(v.Fov)

--@HACK: just a workaround before reExporting camera key frames with the extra parms
		if v.FocusDistance then 
		camKeyPtr:focusDistance( v.FocusDistance ) else
		camKeyPtr:focusDistance( 50.0 ) end
		
		if v.FocusRadius then
		camKeyPtr:focusRadius( v.FocusRadius ) else
		camKeyPtr:focusRadius( 40.0 ) end
		
		-- read metaData and load it
        local pauseMeta = cameraSys:GetPauseWayPointMetaDat( camKeyPtr )
        pauseMeta:panDirection( v.PanDirection )
        pauseMeta:panSpeed( v.PanSpeed )
        pauseMeta:groupID( v.GroupID )
        pauseMeta:allowInPauseMode( v.AllowInPauseMode )
        
		--now triggers can refer to gardener-placed camera keyframes
		Names[ v.ObjectName ] = camKeyPtr 
	end

    CameraKeyFrameInstances = {} 

end

