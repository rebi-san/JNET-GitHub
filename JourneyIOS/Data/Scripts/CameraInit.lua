-------------------------------------------------------------------------------
-- Notes:  Camera data loading functions
--
-- Author: Martin Middleton
-- Date C: 9-20-2007
-------------------------------------------------------------------------------

function LoadCameraKeyFrames( cameraSys )
	for i, desc in ipairs( CameraKeyFrameInstances ) do
		local camKey = cameraSys:AllocateCameraKey()
		CameraKeyFrameInitialize( cameraSys, camKey, desc )

		-- Now triggers can refer to gardener-placed camera keyframes
		Names[ desc.ObjectName ] = camKey 
	end

    CameraKeyFrameInstances = {} 
end

function CameraKeyFrameInitialize( cameraSys, camKey, desc )
	camKey:rot(desc.Rotation)
	camKey:pos(desc.Position)
	camKey:fov(desc.Fov)
	--@HACK: We're changing over to new DoF params in Gardener, so we still need to support the old ones, for a while
	if desc.FocusDistance then camKey:dofDistance( desc.FocusDistance ) end
	if desc.FocusRadius then camKey:dofFocusRadius( desc.FocusRadius ) end
	-- And here are the new params
	if desc.DofFocusRadius then camKey:dofFocusRadius( desc.DofFocusRadius ) end
	if desc.DofTransitionMult then camKey:dofTransitionMult( desc.DofTransitionMult ) end
	if desc.DofDistance then camKey:dofDistance( desc.DofDistance ) end
	if desc.DofIntensity then camKey:dofIntensity( desc.DofIntensity ) end
	if desc.MotionBlurIntensity then camKey:motionBlurIntensity( desc.MotionBlurIntensity ) end

	-- Read metaData and load it
	local pauseMeta = cameraSys:GetPauseWayPointMetaDat( camKey )
	pauseMeta:panDirection( desc.PanDirection )
	pauseMeta:panSpeed( desc.PanSpeed )
	pauseMeta:groupId( desc.GroupID )
	pauseMeta:allowInPauseMode( desc.AllowInPauseMode )
end
