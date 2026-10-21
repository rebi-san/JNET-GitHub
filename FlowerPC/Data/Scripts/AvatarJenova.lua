-------------------------------------------------------------------------------
-- Notes:  Code for manipulating the Swarm Snake
--
-- Author: George Weising
-- Date C: 8-06-2007
-------------------------------------------------------------------------------

--Right now we can have up to 8 profiles( 0-7 )
--The blend is 1/2 the number of petals across which a transition takes place
--So at kNumPetals the profile is half the previous profile, and kNumPetals + kBlend the profile is 100% the new profile
--Only two profiles are ever blended, so if your blend amount bleeds into previous profile blends you might get weird results
function SetSwarmProfiles( swarm )
	
--baby 0-36 petals
	local profile = swarm:GetProfile( 0 )
	profile:kNumPetals( 0 )
	profile:kBlend( 0 )	-- never set a blend for the first profile

	profile:kTailRadiusMin( 0.3 )
	profile:kTailRadiusMax( 0.4 )
	profile:kHeadRadiusMin( 0.3 )
	profile:kHeadRadiusMax( 0.4 )
	profile:kHeadRadiusMinDiff( 0.01 )
	profile:kHeadRadiusMaxDiff( 0.015 )
	profile:kMaxSpinVel( 10.0 )
	profile:kFirstSegLength( 0.01 )
	profile:kFirstSegLengthVariance( 0.01 )
	profile:kFirstSegLengthVarianceSlow( 0.1 )
	profile:kSegLengthMax( 5.2 )
	profile:kSegLengthMaxVariance( 2.1 )

	profile:kHeadRandRotSegVel( 1.25 )
	profile:kHeadRandRotSpeed( 0.5 )
	profile:kHeadRandRotSpeedSlow( 0.00001 )
	profile:kMinStrandLength( 2 )
	profile:kPetalBlendRate( 0.09 )

	profile:kSpineSnappiness( 0.4 )
	profile:kSpineSnappinessSlow( 0.2 )
	profile:kFirstSegSnappiness( 0.4 )
	profile:kSpineSegfollowDistance( 0.14 )
	profile:kSpinMult( 0.0025 )
	profile:kSpinMultSlow( 0.002 )
	profile:kHeadSpinMlt( 0.013 )
	
	profile:kSegVelRelativeMax( 2.0 )
	profile:kSpeedBlendLow( 3.0 )
	profile:kSpeedBlendHigh( 4.0 )
	
	
--child 36-100 petals
	profile = swarm:GetProfile( 1 )
	profile:kNumPetals( 36 )
	profile:kBlend( 0 )

	profile:kTailRadiusMin( 0.2 )
	profile:kTailRadiusMax( 0.4 )
	profile:kHeadRadiusMin( 0.1 )
	profile:kHeadRadiusMax( 0.1 )
	profile:kHeadRadiusMinDiff( 0.01 )
	profile:kHeadRadiusMaxDiff( 0.015 )
	profile:kMaxSpinVel( 10.0 )
	profile:kFirstSegLength( 0.015 )
	profile:kFirstSegLengthVariance( 0.0 )
	profile:kFirstSegLengthVarianceSlow( 0.1 )
	profile:kSegLengthMax( 10 )
	profile:kSegLengthMaxVariance( 0.1 )

	profile:kHeadRandRotSegVel( 1.25 )
	profile:kHeadRandRotSpeed( 0.5 )
	profile:kHeadRandRotSpeedSlow( 0.01 )
	profile:kMinStrandLength( 5 )
	profile:kPetalBlendRate( 0.09 )

	profile:kSpineSnappiness( 0.4 )
	profile:kSpineSnappinessSlow( 0.06 )
	profile:kFirstSegSnappiness( 0.5 )
	profile:kSpineSegfollowDistance( 0.05 )
	profile:kSpinMult( 0.0035 )
	profile:kSpinMultSlow( 0.002 )
	profile:kHeadSpinMlt( 0.02 )
	
	profile:kSegVelRelativeMax( 2.0 )
	profile:kSpeedBlendLow( 3.5 )
	profile:kSpeedBlendHigh( 4.5 )
	

-- Teenager 100 - 500 petals
	profile = swarm:GetProfile( 2 )
	profile:kNumPetals( 100 )
	profile:kBlend( 0 )

	profile:kTailRadiusMin( 0.2 )
	profile:kTailRadiusMax( 0.4 )
	profile:kHeadRadiusMin( 0.1 )
	profile:kHeadRadiusMax( 0.1 )
	profile:kHeadRadiusMinDiff( 0.01 )
	profile:kHeadRadiusMaxDiff( 0.015 )
	profile:kMaxSpinVel( 10.0 )
	profile:kFirstSegLength( 0.02 )
	profile:kFirstSegLengthVariance( 0.1 )
	profile:kFirstSegLengthVarianceSlow( 0.1 )
	profile:kSegLengthMax( 4.0 )
	profile:kSegLengthMaxVariance( 0.3 )

	profile:kHeadRandRotSegVel( 1.5 )
	profile:kHeadRandRotSpeed( 1.0 )
	profile:kHeadRandRotSpeedSlow( 0.000001 )
	profile:kMinStrandLength( 25 )
	profile:kPetalBlendRate( 0.05 )

	profile:kSpineSnappiness( 0.3 )
	profile:kSpineSnappinessSlow( 0.05 )
	profile:kFirstSegSnappiness( 0.4 )
	profile:kSpineSegfollowDistance( 0.08 )
	profile:kSpinMult( 0.0025 )
	profile:kSpinMultSlow( 0.002 )
	profile:kHeadSpinMlt( 0.03 )
	
	profile:kSegVelRelativeMax( 2.0 )
	profile:kSpeedBlendLow( 3.5 )
	profile:kSpeedBlendHigh( 7.0 )
	
-- Adult 500 - 1000 petals
	profile = swarm:GetProfile( 3 )
	profile:kNumPetals( 500 )
	profile:kBlend( 0 )

	profile:kTailRadiusMin( 0.25 )
	profile:kTailRadiusMax( 0.5 )
	profile:kHeadRadiusMin( 0.1 )
	profile:kHeadRadiusMax( 0.1 )
	profile:kHeadRadiusMinDiff( 0.01 )
	profile:kHeadRadiusMaxDiff( 0.015 )
	profile:kMaxSpinVel( 10.0 )
	profile:kFirstSegLength( 0.01 )
	profile:kFirstSegLengthVariance( 0.2 )
	profile:kFirstSegLengthVarianceSlow( 0.1 )
	profile:kSegLengthMax( 4.0 )
	profile:kSegLengthMaxVariance( 0.3 )

	profile:kHeadRandRotSegVel( 1.5 )
	profile:kHeadRandRotSpeed( 0.00001 )
	profile:kHeadRandRotSpeedSlow( 0.000001 )
	profile:kMinStrandLength( 30 )
	profile:kPetalBlendRate( 0.05 )

	profile:kSpineSnappiness( 0.35 )
	profile:kSpineSnappinessSlow( 0.03 )
	profile:kFirstSegSnappiness( 0.35 )
	profile:kSpineSegfollowDistance( 0.08 )
	profile:kSpinMult( 0.002 )
	profile:kSpinMultSlow( 0.002 )
	profile:kHeadSpinMlt( 0.02 )
	
	profile:kSegVelRelativeMax( 2.0 )
	profile:kSpeedBlendLow( 5.0 )
	profile:kSpeedBlendHigh( 10.0 )

-- Land Profile
	profile = swarm:GetLandProfile()
	profile:kNumPetals( 500 )
	profile:kBlend( 0 )

	profile:kTailRadiusMin( 0.00025 )
	profile:kTailRadiusMax( 0.0005 )
	profile:kHeadRadiusMin( 0.0001 )
	profile:kHeadRadiusMax( 0.0001 )
	profile:kHeadRadiusMinDiff( 0.0001 )
	profile:kHeadRadiusMaxDiff( 0.00015 )
	profile:kMaxSpinVel( 10.0 )
	profile:kFirstSegLength( 0.001 )
	profile:kFirstSegLengthVariance( 0.002 )
	profile:kFirstSegLengthVarianceSlow( 0.0001 )
	profile:kSegLengthMax( 0.001 )
	profile:kSegLengthMaxVariance( 0.00003 )

	profile:kHeadRandRotSegVel( 1.5 )
	profile:kHeadRandRotSpeed( 0.00001 )
	profile:kHeadRandRotSpeedSlow( 0.000001 )
	profile:kMinStrandLength( 30 )
	profile:kPetalBlendRate( 0.05 )

	profile:kSpineSnappiness( 0.35 )
	profile:kSpineSnappinessSlow( 0.3 )
	profile:kFirstSegSnappiness( 0.35 )
	profile:kSpineSegfollowDistance( 0.0 )
	profile:kSpinMult( 0.00 )
	profile:kSpinMultSlow( 0.00 )
	profile:kHeadSpinMlt( 0.0 )
	
	profile:kSegVelRelativeMax( 2000.0 )
	profile:kSpeedBlendLow( 5.0 )
	profile:kSpeedBlendHigh( 10.0 )
	
--etc...
	
	swarm:SetNumProfiles( 4 )
end


