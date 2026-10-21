-------------------------------------------------------------------------------
-- Notes:  Environmental Particle Effects
--
-- Author: Martin Middleton
-- Date C: 12-06-2007
-------------------------------------------------------------------------------
 
function LoadEnvFXParticles( envFXBarn, particleBarn )
	for i,emitter in ipairs( ParticleEmitterInstances ) do
		local curEffect = envFXBarn:AddEffect( particleBarn, emitter.EmitterType )
		curEffect:SetPos( emitter.Position[ 1 ], emitter.Position[ 2 ], emitter.Position[ 3 ] )
		if ( emitter.TargetPos[ 1 ] == 0 and emitter.TargetPos[ 2 ] == 0 and emitter.TargetPos[ 3 ] == 0 )
			then curEffect:SetTargetPos( emitter.Position[ 1 ], emitter.Position[ 2 ], emitter.Position[ 3 ] )
			else curEffect:SetTargetPos( emitter.TargetPos[ 1 ], emitter.TargetPos[ 2 ], emitter.TargetPos[ 3 ] )
		end
		curEffect:SetDir( -emitter.Transformation[ 3 ][ 1 ], -emitter.Transformation[ 3 ][ 2 ], -emitter.Transformation[ 3 ][ 3 ] )
		curEffect:SetEnabled( emitter.Enabled )
		curEffect:SetFollowCamera( emitter.FollowCamera )
		if( emitter.CameraOffset ) then
			curEffect:SetOffsetPos( emitter.CameraOffset[ 1 ], emitter.CameraOffset[ 2 ], emitter.CameraOffset[ 3 ] )
		end
		if( emitter.ForceCache ) then curEffect:SetForceCache() end
		if( emitter.WindHullBlendVel ) then curEffect:windHullBlendVel( emitter.WindHullBlendVel ) else curEffect:windHullBlendVel( 0.0 ) end
		if( emitter.WindHullBlendDir ) then curEffect:windHullBlendDir( emitter.WindHullBlendDir ) else curEffect:windHullBlendDir( 0.0 ) end
		if( emitter.FollowSwarm ) then curEffect:followSwarm( true ) else curEffect:followSwarm( false ) end
		if( emitter.CamLeadMult ) then curEffect:camLeadMult( emitter.CamLeadMult ) else curEffect:camLeadMult( 1.0 ) end
		Names[ emitter.ObjectName ] = curEffect
	end
	
	ParticleEmitterInstances = {}
end
