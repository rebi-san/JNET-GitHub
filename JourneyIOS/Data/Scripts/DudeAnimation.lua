-------------------------------------------------------------------------------
-- Notes:  Controls the animation of the avatar
--
-- Author: Martin Middleton
-- Date C: 08-19-2009
-------------------------------------------------------------------------------

local function Clip( x, xmin, xmax )
	return math.min( xmax, math.max( xmin, x ) )
end

local function Delerp( t, x0, x1 )
	local diff = x1 - x0
	if diff > -0.00001 and diff < 0.00001 then return 1 end
	local val = ( t - x0 ) / diff
	return Clip( val, 0, 1 )
end

local function CalcValWithFade( val, fadeVals )
	local fadeInStart = fadeVals[ 1 ]
	local fadeInEnd = fadeVals[ 2 ]
	local fadeOutStart = fadeVals[ 3 ]
	local fadeOutEnd = fadeVals[ 4 ]
	if val < fadeInEnd then return Delerp( val, fadeInStart, fadeInEnd ) end
	if val > fadeOutStart then return 1 - Delerp( val, fadeOutStart, fadeOutEnd ) end
	return 1
end

-- The blendIn and blendOut values are mults of dt to control the max rate of change
-- of the final values of the weights, to prevent popping. The downside to this is the animation might not be as responsive to
-- the actual state of the world, there is always a tradeoff between responsiveness and smoothness.
-- maxBlendOutTime is useful for non-looping animations to make sure we finish blending out before the animation ends
local animDatas =
{
	Idle = { animSpeedMult = 0.5 }
,	IdleWeak = { animSpeedMult = 0.3 }
,	Walk = { isGroundMotion = true, stride = 0.6, animSpeedBounds = { 0, 1.3 } }
,	Walk2 = { isGroundMotion = true, stride = 0.6, animSpeedBounds = { 0, 1.3 } }
,	WalkWeak = { isGroundMotion = true, stride = 0.52, animSpeedBounds = { 0.0, 1 } }
,	WalkHurtL = { isGroundMotion = true, stride = 0.7, animSpeedBounds = { 0.0, 3 } }
,	WalkHurtL1 = { isGroundMotion = true, stride = 0.8, animSpeedBounds = { 0.0, 3 } }
,	WalkHurtR = { isGroundMotion = true, stride = 0.7, animSpeedBounds = { 0.0, 3 } }
,	WalkHurtR1 = { isGroundMotion = true, stride = 0.8, animSpeedBounds = { 0.0, 3 } }
,	WalkSnow = { isGroundMotion = true, stride = .96, animSpeedBounds = { 0.0, 2 } }
,	WalkTired = { isGroundMotion = true, stride = .96, animSpeedBounds = { 0.0, 2 } }
,	WalkUp = { isGroundMotion = true, stride = 0.6, animSpeedBounds = { 0, 1.3 } }
,	WalkDown = { isGroundMotion = true, stride = 1.0, animSpeedBounds = { 0, 3 }  }
,	PushBack = { isGroundMotion = true, stride = 0.2, animSpeedBounds = { 0.5, 3 } }
,	Slide = { animSpeedMult = 1 }
,	SlideB = { animSpeedMult = 1 }
,	SlideF = { animSpeedMult = 1 }
,	SlideL = { animSpeedMult = 1 }
,	SlideR = { animSpeedMult = 1 }
,	SlideShake = { animSpeedMult = 2.4 }
,	SlideX = { animSpeedMult = 1 }
,	SlideXB = { animSpeedMult = 1 }
,	SlideXF = { animSpeedMult = 1 }
,	SlideXL = { animSpeedMult = 1 }
,	SlideXR = { animSpeedMult = 1 }
,	SlideXShake = { animSpeedMult = 2.4 }
,	Jog = { isGroundMotion = true, stride = 1.1, animSpeedBounds = { 0, 3 } }
,	JogWeak = { isGroundMotion = true, stride = 1.1, animSpeedBounds = { 0.5, 2 } }
,	JogSnow = { isGroundMotion = true, stride = 1.2, animSpeedBounds = { 0.5, 2 } }
,	Run = { isGroundMotion = true, stride = 1.55, animSpeedBounds = { 0, 3 } }
,	RunWeak = { isGroundMotion = true, stride = 1.8, animSpeedBounds = { 0, 1 } }
,	Run1 = { isGroundMotion = true, stride = 1.55, animSpeedBounds = { 0, 3 } }
,	RunAltWeak = { isGroundMotion = true, stride = 1.8, animSpeedBounds = { 0, 1 } }
,	RunSnow = { isGroundMotion = true, stride = 1.2, animSpeedBounds = { 0, 3 } }
,	RunSnow1 = { isGroundMotion = true, stride = 1.2, animSpeedBounds = { 0, 3 } }
,   RunMJump = { isAction = true, animSpeedMult = 1.0 }
,	Boost = { isGroundMotion = true, stride = 2.5, animSpeedBounds = { 0.75, 3 } }
,	Glide = { }
,	GlideD = { }
,	GlideFD = { }
,	GlideL = { }
,	GlideR = { }
,	Dive = { }
,	DiveB = { }
,	SwimF = { isGroundMotion = true, stride = 5.5, animSpeedBounds = { 0.1, 1 } }
,	SwimL = { isGroundMotion = true, stride = 5.5, animSpeedBounds = { 0.1, 1 } }
,	SwimR = { isGroundMotion = true, stride = 5.5, animSpeedBounds = { 0.1, 1 } }
,	SwimTurnL = { isAction = true, animSpeedMult = 1.0 }
,	SwimTurnR = { isAction = true, animSpeedMult = 1.0 }
,   TurnBodyLeft = { animSpeedMult = 1.25 }
,   TurnBodyRight = { animSpeedMult = 1.25 }
,   LookBackL = { isAction = true, animSpeedMult = 1.0 }
,   LookBackR = { isAction = true, animSpeedMult = 1.0 }
, 	BlownBack = { isAction = true, animSpeedMult = 1} 
,	IdleStretch = { isAction = true, animSpeedMult = 1.0 }
,	IdleCheckFootL = { isAction = true, animSpeedMult = 1.0 }
,	IdleCheckFootR = { isAction = true, animSpeedMult = 1.0 }
,	Act01Hold = { isAction = true, animSpeedMult = 1.0 }
,	LiftA = { isAction = true, animSpeedMult = 1.0 }
,	LiftEL = { isAction = true, animSpeedMult = 1.0 }
,	LiftER = { isAction = true, animSpeedMult = 1.0 }
,	LiftF = { isAction = true, animSpeedMult = 1.0 }
,	Flap1 = { isAction = true, animSpeedMult = 1.0 }
,	Flap3 = { isAction = true, animSpeedMult = 1.0 }
,	HitBodyL = { isAction = true, animSpeedMult = 1.0 }
,	HitBodyR = { isAction = true, animSpeedMult = 1.0 }
,	HitBodyM = { isAction = true, animSpeedMult = 1.0 }
,	HitBodyLAir = { isAction = true, animSpeedMult = 1.0 }
,	HitBodyRAir = { isAction = true, animSpeedMult = 1.0 }
,	HitBodyMAir = { isAction = true, animSpeedMult = 1.0 }
,	ShoutL = { isAction = true, animSpeedMult = 1.0 }
,	ShoutR = { isAction = true, animSpeedMult = 1.0 }
,	ShoutM = { isAction = true, animSpeedMult = 1.0 }
,	ShoutBL = { isAction = true, animSpeedMult = 1.0 }
,	ShoutBR = { isAction = true, animSpeedMult = 1.0 }
,	Charge = { isAction = true, animSpeedMult = 1.0 }
,	ChargeAir = { isAction = true, animSpeedMult = 1.0 }
,	ChargeWalk = { isAction = true, animSpeedMult = 1.0 }
,	ChargeRun = { isAction = true, animSpeedMult = 1.0 }
,	ChargeWeak = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
,	ChargeCold = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
,	TurnLookBackL = { isAction = true, animSpeedMult = 1.0 }
,	TurnLookBackR = { isAction = true, animSpeedMult = 1.0 }
,	AirFlipR = { isAction = true, animSpeedMult = 1.0 }
,	AirFlipL = { isAction = true, animSpeedMult = 1.0 }
,	FallForward = { isAction = true, animSpeedMult = 1.0 }
,	RollF = { isAction = true, animSpeedMult = 1.0 }
,	RollB = { isAction = true, animSpeedMult = 1.0 }
,	RigTest = { isAction = true, animSpeedMult = 1.0 }
,	RollL = { isAction = true, animSpeedMult = 1.0 }
,	RollR = { isAction = true, animSpeedMult = 1.0 }
,	LieF = { isAction = true, animSpeedMult = 1.0 }
,	LieFSlide = { isAction = true, animSpeedMult = 1.0 }
,	LieB = { isAction = true, animSpeedMult = 1.0 }
,	GetUpF = { isAction = true, animSpeedMult = 1.0 }
,	GetUpB = { isAction = true, animSpeedMult = 1.0 }
,	TrippingL = { isAction = true, animSpeedMult = 1.0 }
,	TrippingR = { isAction = true, animSpeedMult = 1.0 }
,	TrippingL1 = { isAction = true, animSpeedMult = 1.0 }
,	TrippingR1 = { isAction = true, animSpeedMult = 1.0 }
,	StopL = { isAction = true, animSpeedMult = 1.0 }
,	StopR = { isAction = true, animSpeedMult = 1.0 }
,	StopBoostL = { isAction = true, animSpeedMult = 1.0 }
,	StopBoostR = { isAction = true, animSpeedMult = 1.0 }
,	StopTurnRunL = { isAction = true, animSpeedMult = 1.0 }
,	StopTurnRunR = { isAction = true, animSpeedMult = 1.0 }
,	StandTurnL = { isAction = true, animSpeedMult = 1.0 }
,	StandTurnR = { isAction = true, animSpeedMult = 1.0 }
,	WalkStopL = { isAction = true, animSpeedMult = 1.0 }
,	WalkStopR = { isAction = true, animSpeedMult = 1.0 }
,	JumpFL = { isAction = true, animSpeedMult = 1.0 }
,	JumpFR = { isAction = true, animSpeedMult = 1.0 }
,	JumpHalfL = { isAction = true, animSpeedMult = 1.0 }
,	JumpHalfR = { isAction = true, animSpeedMult = 1.0 }
,	JumpRoll = { isAction = true, animSpeedMult = 1.0 }
,	LeapL = { isAction = true, animSpeedMult = 1.0 }
,	LeapR = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpL = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpR = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpL1 = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpR1 = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpWeakL1 = { isAction = true, animSpeedMult = 1.1 }
,	ClimbUpWeakR1 = { isAction = true, animSpeedMult = 1.1 }
,	ClimbUpWeakHL = { isAction = true, animSpeedMult = 1.1 }
,	ClimbUpStepWL = { isAction = true, animSpeedMult = 1.1 }
,	ClimbUpStepWR = { isAction = true, animSpeedMult = 1.1 }
,	ClimbUpStepL = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpStepR = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpStepL1 = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpStepR1 = { isAction = true, animSpeedMult = 1.0 }
,	ClimbUpRunLR = { isGroundMotion = true, stride = 1.5, animSpeedBounds = { 0.0, 2 } }
,	ClimbUpRunLR1 = { isGroundMotion = true, stride = 1.5, animSpeedBounds = { 0.0, 2 } }
,	ClimbUpWalkSnow = { isGroundMotion = true, stride = 1.2, animSpeedBounds = { 0.0, 2 } }
,	ClimbUpRunSnow = { isGroundMotion = true, stride = 1.5, animSpeedBounds = { 0.0, 2 } }
,	ClimbUpRunSnow1 = { isGroundMotion = true, stride = 1.5, animSpeedBounds = { 0.0, 2 } }
,	SlideJump = { isAction = true, animSpeedMult = 1.0 }
,	SlideJump1 = { isAction = true, animSpeedMult = 1.0 }
,	SlideJump2 = { isAction = true, animSpeedMult = 1.0 }
,	SlideXJump = { isAction = true, animSpeedMult = 1.0 }
,	SlideXJump1 = { isAction = true, animSpeedMult = 1.0 }
,	SlideXJump2 = { isAction = true, animSpeedMult = 1.0 }
,	SlideAdjustL = { isAction = true, animSpeedMult = 1.0 }
,	SlideAdjustR = { isAction = true, animSpeedMult = 1.0 }
,	SlideAdjustF = { isAction = true, animSpeedMult = 1.0 }
,	SlideAdjustB = { isAction = true, animSpeedMult = 1.0 }
,	SlideAdjustL1 = { isAction = true, animSpeedMult = 0.4 }
,	SlideAdjustR1 = { isAction = true, animSpeedMult = 0.4 }
,	SlideAdjustF1 = { isAction = true, animSpeedMult = 1.0 }
,	SlideAdjustB1 = { isAction = true, animSpeedMult = 1.0 }
,	SlideXAdjustL = { isAction = true, animSpeedMult = 1.0 }
,	SlideXAdjustR = { isAction = true, animSpeedMult = 1.0 }
,	SlideXAdjustF = { isAction = true, animSpeedMult = 1.0 }
,	SlideXAdjustB = { isAction = true, animSpeedMult = 1.0 }
,	SlideXAdjustL1 = { isAction = true, animSpeedMult = 0.4 }
,	SlideXAdjustR1 = { isAction = true, animSpeedMult = 0.4 }
,	SlideXAdjustF1 = { isAction = true, animSpeedMult = 1.0 }
,	SlideXAdjustB1 = { isAction = true, animSpeedMult = 1.0 }
,   SlideGroundSpinL = { isAction = true, animSpeedMult = 1.0 }
,   SlideGroundSpinR = { isAction = true, animSpeedMult = 1.0 }
,	LandSoft = { isAction = true, animSpeedMult = 1.0 }
,	LandFast = { isAction = true, animSpeedMult = 1.0 }
,	SlideLand = { isAction = true, animSpeedMult = 1.0 }
,	SlideXLand = { isAction = true, animSpeedMult = 1.0 }   
,   Crouch = { isAction = true, animSpeedMult = 1.0 }
,   Crouching = { isAction = true, animSpeedMult = 1.0 }
,   CrouchWeak = { isAction = true, animSpeedMult = 1.0 }
,   CrouchWeak1 = { isAction = true, animSpeedMult = 1.0 }
,   CrouchingWeak = { isAction = true, animSpeedMult = 1.0 }
,   CrouchToIdle = { isAction = true, animSpeedMult = 1.0 }
,   CrouchToIdleW = { isAction = true, animSpeedMult = 1.0 }
,	Med = { isAction = true, animSpeedMult = 1.0 }
, 	Kneel = { isAction = true, animSpeedMult = 1.0 }
, 	Kneeling = { isAction = true, animSpeedMult = 1.0 }
,	ReadyToMed = { isAction = true, animSpeedMult = 1.0 }
,	StopMed = { isAction = true, animSpeedMult = 1.0 }
,	FixCloth = { isAction = true, animSpeedMult = 1.0 }
,	FixCloth2 = { isAction = true, animSpeedMult = 1.0 }
,	FixCloth2X= { isAction = true, animSpeedMult = 1.0 }
,	FixCloth3 = { isAction = true, animSpeedMult = 1.0 }
,	FixCloth4 = { isAction = true, animSpeedMult = 1.0 }
,	FixCloth4X = { isAction = true, animSpeedMult = 1.0 }
,	FixClothAir = { isAction = true, animSpeedMult = 1.0 }
,	MPAirSwing1L = { isAction = true, animSpeedMult = 1.0 }
,	MPAirSwing1R = { isAction = true, animSpeedMult = 1.0 }
,	MPAirSwing2L = { isAction = true, animSpeedMult = 1.0 }
,	MPAirSwing2R = { isAction = true, animSpeedMult = 1.0 }
,	MPRunFlip1L = { isAction = true, animSpeedMult = 1.0 }
,	MPRunFlip1R = { isAction = true, animSpeedMult = 1.0 }
,	MPFSpinL = { isAction = true, animSpeedMult = 1.0 }
,	MPFSpinR = { isAction = true, animSpeedMult = 1.0 }
,	SpinF = { isAction = true, animSpeedMult = 1.0 }
,	JumpRoll = { isAction = true, animSpeedMult = 1.0 }
,	LookUp = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
,	BoostSpiralL = { isAction = true, animSpeedMult = 1.0 }
,	BoostSpiralR = { isAction = true, animSpeedMult = 1.0 }
,	BoostFlap = { isAction = true, animSpeedMult = 1.0 }
,	BoostFlap2 = { isAction = true, animSpeedMult = 1.0 }
,	BoostFlap3 = { isAction = true, animSpeedMult = 1.0 }
,	Collapse = { isAction = true, animSpeedMult = 1.0 }
,	Collapsing = { isAction = true, animSpeedMult = 1.0 }
,	Collapse2 = { isAction = true, animSpeedMult = 1.0 }
,	Collapsing2 = { isAction = true, animSpeedMult = 1.0 }
,	Revive = { isAction = true, animSpeedMult = 1.0 }
,	Reviving = { isAction = true, animSpeedMult = 1.0 }
,	Revive2 = { isAction = true, animSpeedMult = 1.0 }
,	Reviving2 = { isAction = true, animSpeedMult = 1.0 }
,	PrepareToLaunch = { isAction = true, animSpeedMult = 1.0 }
,	ReadyToLaunch = { isAction = true, animSpeedMult = 1.0 }
,	HopL = { isAction = true, animSpeedMult = 1.0 }
,	NodHead = { isAction = true, animSpeedMult = 1.0 }
,	ShakeHead = { isAction = true, animSpeedMult = 1.0 }
,	LiftA1 = { isAction = true, animSpeedMult = 1.0 }
,	ShakeOff = { isAction = true, animSpeedMult = 1.0 }


-- Cinematic Cutscene
,   Act01Surprise = { isAction = true, animSpeedMult = 1.0 }
,   Act01LookDown = { isAction = true, animSpeedMult = 1.0 }
,   Act01LookAround = { isAction = true, animSpeedMult = 1.0 }
,   Act06LookAround = { isAction = true, animSpeedMult = 1.0 }
,   Act06LookLR = { isAction = true, animSpeedMult = 1.0 }

-- Additive
,	AddBreath = { isAdditive = true, animSpeedMult = 0.7  }
,	AddHeadL = { isAdditive = true, animSpeedMult = 1 }
,	AddHeadUp = { isAdditive = true, animSpeedMult = 1 }
--,	AddMultiplayer = { isAdditive = true, animSpeedMult = 0.4 }
,	AddNoise = { isAdditive = true, animSpeedMult = 0.05 }
,	AddNoiseHip = { isAdditive = true, animSpeedMult = 0.3 }
,	AddWindB = { isAdditive = true, animSpeedMult = 1.0 }
,	AddWindF = { isAdditive = true, animSpeedMult = 1.0 }
,	AddWindL = { isAdditive = true, animSpeedMult = 1.0 }
,	AddWindR = { isAdditive = true, animSpeedMult = 1.0 }
,	AddHipTurnL = { isAdditive = true, animSpeedMult = 1 }
,	AddHipTurnR = { isAdditive = true, animSpeedMult = 1 }
,	AddBend = { isAdditive = true, animSpeedMult = 1 }
,	AddBendL = { isAdditive = true, animSpeedMult = 1 }
,	AddBendR = { isAdditive = true, animSpeedMult = 1 }
,	AddFaceShake = { isAction = true, isAdditive = true, animSpeedMult = 1 }
,	Cough = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
,	AddFlap = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
,	AddFlap2 = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
,	AddFlap3 = { isAction = true, isAdditive = true, animSpeedMult = 1.0 }
--,	RunM = { isGroundMotion = true, stride = 4.0, animSpeedBounds = { 0, 100 } }
--,	RunML = { isGroundMotion = true, stride = 4.0 }
--,	RunMR = { isGroundMotion = true, stride = 4.0 }

--,	IdleBuried = { isAction = true, animSpeedMult = 1.0 }
--,	ShoutBuriedL = { isAction = true, animSpeedMult = 1.0 }
--,	ShoutBuriedR = { isAction = true, animSpeedMult = 1.0 }
--,	AdditiveBend = { isAdditive = true, animSpeedMult = 1.0 }
}

function LoadDudeStateAnimations( animBarn )
	for animName,animData in pairs( animDatas ) do
		if not animData.isAction then
			local anim = animBarn:AddStateAnimation()
			anim:SetName( animName )
			if animData.stride then anim:stride( animData.stride ) end
			if animData.animSpeedBounds then 
				anim:speedMin( animData.animSpeedBounds[ 1 ] ) 
				anim:speedMax( animData.animSpeedBounds[ 2 ] ) 
			end
			if animData.isGroundMotion then anim:isGroundMotion( true ) end
			if animData.animSpeedMult then anim:animSpeedMult( animData.animSpeedMult ) end
			if( animData.isAdditive ) then anim:isAdditive( true ) end
			if( animData.scurveBlend ) then anim:scurveBlend( true ) end
			if( animData.isDucker ) then anim:isDucker( true ) end
			if( animData.isDucked ) then anim:isDucked( true ) end
		end
	end
end

function LoadDudeActionAnimations( animBarn )
	for animName,animData in pairs( animDatas ) do
		if( animData.isAction ) then
			local anim = animBarn:AddActionAnimation()
			anim:SetName( animName )
			anim:isAction( true )
			if( animData.isAdditive ) then anim:isAdditive( true ) end
			if( animData.isDucker ) then anim:isDucker( true ) end
			if( animData.isDucked ) then anim:isDucked( true ) end
		end
	end
end

-- BankName,SoundName,FalloffDist,Volume
-- To play multiple sounds on the same frame you would do something like this
-- [ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 2.0, 1 }, { "SharSfx", "FootstepDeepLeft", 3.0, 1 } } }
local dudeAnimSoundMarkers =
{
	Boost = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } } }
	}
,	Run = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,   [ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } }	}
	}
,	Run1 = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,   [ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } }	}
	}
,	RunWeak = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,   [ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } }	}
	}
,	RunAltWeak = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,   [ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } }	}
	}
,	RunSnow = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,   [ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } }	}
	}	
,	RunSnow1 = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 } } }
	,   [ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 } }	}
	}		
,	Jog = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 2.0, 12.0, 1 } } }
	}	
,	JogSnow = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepRunLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepRunRight", 2.0, 12.0, 1 } } }
	}		
,	Walk =
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepWalkLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepWalkRight", 2.0, 12.0, 1 } } }	
	}	
,	Walk2 =
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepWalkLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepWalkRight", 2.0, 12.0, 1 } } }	
	}	
,	WalkDown = 
	{
		[ 0 ] = { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } 
	,	[ 15 ] = { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } 
	}
,	WalkUp = 
	{
		[ 0 ] = { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } 
	,	[ 15 ] = { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } 
	}	
,	WalkSnow =
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepWalkLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepWalkRight", 2.0, 12.0, 1 } } }	
	}
,	WalkTired =
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepWalkLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepWalkRight", 2.0, 12.0, 1 } } }	
	}			
,	WalkWeak =
	{
		[ 0 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 15 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	}	
,	WalkHurtL =
	{
		[ 15 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 25 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepWalkRight", 2.0, 12.0, 1 } }
		}
	}	
,	WalkHurtL1 =
	{
		[ 15 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 25 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepWalkRight", 2.0, 12.0, 1 } }
		}
	}
,	WalkHurtR =
	{
		[ 15 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 25 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepWalkLeft", 2.0, 12.0, 1 } }
		}
	}	
,	WalkHurtR1 =
	{
		[ 15 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 25 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepWalkLeft", 2.0, 12.0, 1 } }
		}
	}
,	ChargeWalk =
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 12.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 12.0, 1 } } }	
	}								
,	JogWeak =
	{
		[ 0 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 15 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	}	
,	PushBack = 
	{
		[ 0 ] = { rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.3 } }
	,	[ 15 ] = { rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.3 } }
	}	
,	ClimbUpRunLR = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } } }		
	}	
,	ClimbUpRunLR1 = 
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } } }
	,	[ 15 ] = { sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } } }		
	}		
,	ClimbUpWalkSnow = 
	{
		[ 0 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 15 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	}
,	ClimbUpRunSnow = 
	{
		[ 0 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.1 } 	
		}
	,	[ 15 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.1 } 	
		}
	}	
,	ClimbUpRunSnow1 = 
	{
		[ 0 ] =	
		{
			sounds = { { "SharSfx", "FootstepDeepLeft", 2.0, 14.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.1 } 	
		}
	,	[ 15 ] = 
		{ 
			sounds = { { "SharSfx", "FootstepDeepRight", 2.0, 14.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.1 } 	
		}
	}		
,	RollF = 
	{
		[ 0 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	}
,	RollB = 
	{
		[ 0 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	}	
,	RollL = 
	{
		[ 0 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	}	
,	RollR = 
	{
		[ 0 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 }
	}	
,	TrippingL = 
	{
		[ 0 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	}	
,	TrippingR = 
	{
		[ 0 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	}
,	TrippingL1 = 
	{
		[ 0 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	}	
,	TrippingR1 = 
	{
		[ 0 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 15 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	}
,	BlownBack =
	{
		[ 15 ] = { 
		    sounds = { { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.25, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.5 }
		}
	}
,	FallForward =
	{
		[ 0 ] = { "SharSfx", "2ndJump-Spin", 3.0, 12.0, 1 }
	,	[ 9 ] = { 
		    sounds = { { "SharSfx", "TripAndRoll", 3.0, 12.0, 1 } }
		,	rumble = { largeRumbleDuration = 0.25, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.5 }
		}
	}
,	Collapse =
	{
		[ 24 ] =	
		{
			rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 100 ] = 
		{ 
			rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	}
,	Collapse2 =
	{
		[ 24 ] =	
		{
			rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	,	[ 50 ] = 
		{ 
			rumble = { largeRumbleDuration = 0.125, largeRumbleFalloff = 0.125, largeRumbleIntensity = 0.2 } 	
		}
	}	
,   HopL =
	{
		[ 1 ] = { "SharSfx", "Jump-Moving", 3.0, 12.0, 1 }
	}
,   ReadyToMed	=
	{
		[ 1 ] = { "SharSfx", "SitDown", 3.0, 12.0, 1 }
	}
,   StopMed	=
	{
		[ 0 ] = { "SharSfx", "StandUp", 3.0, 12.0, 1 }
	}
,   GetUpF =	
	{
		[ 0 ] = { "SharSfx", "StandUp", 3.0, 12.0, 1 }
	}
,   GetUpB =	
	{
		[ 0 ] = { "SharSfx", "StandUp", 3.0, 12.0, 1 }
	}
,   CrouchToIdleW =	
	{
		[ 0 ] = { "SharSfx", "StandUp", 3.0, 12.0, 1 }
	}	
,   CrouchWeak =	
	{
		[ 16 ] = { "SharSfx", "JumpLand-InPlace", 3.0, 12.0, 1 }
	,   [ 29 ] = { "SharSfx", "JumpLand-InPlace", 3.0, 12.0, 1 }
	}	
,   CrouchWeak1 =	
	{
		[ 16 ] = { "SharSfx", "JumpLand-InPlace", 3.0, 12.0, 1 }
	,   [ 29 ] = { "SharSfx", "JumpLand-InPlace", 3.0, 12.0, 1 }
	}		
,   JumpFL =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "Jump-Moving", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }	
	}	
,   JumpFR =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "Jump-Moving", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }	
	}		
,   LeapL =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   LeapR =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   ClimbUpL =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   ClimbUpR =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}		
,   ClimbUpL1 =	
	{
		[ 0 ] = { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }
	,   [ 5 ] = { sounds = { { "SharSfx", "Jump-InPlace", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	,	[ 22 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	,	[ 25 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	}	
,   ClimbUpR1 =	
	{
		[ 0 ] = { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }
	,   [ 5 ] = { sounds = { { "SharSfx", "Jump-InPlace", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	,	[ 22 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 25 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	}
,   ClimbUpWeakL1 =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	,	[ 17 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 39 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }		
	}	
,   ClimbUpWeakR1 =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	,	[ 17 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }		
	,	[ 39 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }		
	}
,   ClimbUpStepWL =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   ClimbUpStepWR =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}			
,   JumpRoll =	
	{
		[ 0 ] = { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 } 
	,   [ 5 ] = { sounds = { { "SharSfx", "2ndJump-Spin", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	,	[ 22 ] = { "SharSfx", "FootstepRunLeft", 3.0, 12.0, 1 }
	,	[ 25 ] = { "SharSfx", "FootstepRunRight", 3.0, 12.0, 1 }
	}		
		
,   ClimbUpStepL =
	{
		[ 3 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }
	}
,   ClimbUpStepR =
	{
		[ 3 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }
	}
,   ClimbUpStepL1 =
	{
		[ 3 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }
	}
,   ClimbUpStepR1 =
	{
		[ 3 ] = { sounds = { { "SharSfx", "ClimbUp", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }
	}	
,   SlideJump =
	{		
		[ 0 ] = { sounds = { { "SharSfx", "Jump-Surfing", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}
,   SlideJump1 =
	{		
		[ 0 ] = { sounds = { { "SharSfx", "Jump-Surfing", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   SlideJump2 =
	{		
		[ 0 ] = { sounds = { { "SharSfx", "SurfFlip", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}
,   SlideJump3 =
	{		
		[ 0 ] = { sounds = { { "SharSfx", "SurfFlip", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}		
,   SlideXJump =
	{
		[ 0 ] = { sounds = { { "SharSfx", "Jump-Surfing", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}
,   SlideXJump1 =
	{
		[ 0 ] = { sounds = { { "SharSfx", "Jump-Surfing", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   SlideXJump2 =
	{
		[ 0 ] = { sounds = { { "SharSfx", "SurfFlip", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}	
,   SlideXJump3 =
	{
		[ 0 ] = { sounds = { { "SharSfx", "SurfFlip", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Jump", 3.0, 12.0, 1 } } }		
	}		
,   LandSoft =
	{
		[ 0 ] = { "SharSfx", "JumpLand-InPlace", 3.0, 12.0, 1 }
	}
,   LandFast =
	{
		[ 0 ] = { "SharSfx", "JumpLand-Surfing", 3.0, 12.0, 1 }
	}	
,   Charge =
	{
		[ 9 ] = { "SharSfx", "TurnSharp", 3.0, 12.0, 1 }
	}
,   ChargeRun =
	{
		[ 0 ] = { "SharSfx", "JumpLand-Moving", 3.0, 12.0, 1 }
	}
,   SlideLand =
	{
		[ 0 ] = { "SharSfx", "JumpLand-Surfing", 3.0, 12.0, 1 }
	}
,   SlideXLand =
	{
		[ 0 ] = { "SharSfx", "JumpLand-Surfing", 3.0, 12.0, 1 }
	}
,   StopBoostL =	
	{
		[ 0 ] = { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }
	}				
,   StopBoostR =	
	{
		[ 0 ] = { "SharSfx", "FootstepDeepRight", 3.0, 12.0, 1 }
	}						
,   StopL =	
	{
		[ 9 ] = { "SharSfx", "FootstepWalkRight", 3.0, 12.0, .75 }
	,	[ 15 ] = { "SharSfx", "FootstepWalkLeft", 3.0, 12.0, .5 }
	,   [ 21 ] = { "SharSfx", "FootstepWalkRight", 3.0, 12.0, .25 }
	}
,   StopR =	
	{
		[ 9 ] = { "SharSfx", "FootstepWalkLeft", 3.0, 12.0, .75 }
	,	[ 15 ] = { "SharSfx", "FootstepWalkRight", 3.0, 12.0, .5 }
	,   [ 21 ] = { "SharSfx", "FootstepWalkLeft", 3.0, 12.0, .25 }
	}
,   StopTurnRunL =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "TurnSharp", 3.0, 12.0, 1 }, { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Twist", 3.0, 12.0, 1 } } }
	}
,   StopTurnRunR =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "TurnSharp", 3.0, 12.0, 1 }, { "SharSfx", "FootstepDeepRight", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Twist", 3.0, 12.0, 1 } } }	
	}
,   StandTurnL =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "TurnSharp", 3.0, 12.0, 1 }, { "SharSfx", "FootstepDeepLeft", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Twist", 3.0, 12.0, 1 } } }
	}
,   StandTurnR =	
	{
		[ 0 ] = { sounds = { { "SharSfx", "TurnSharp", 3.0, 12.0, 1 }, { "SharSfx", "FootstepDeepRight", 3.0, 12.0, 1 }, { "SharSfx", "Cape1-Twist", 3.0, 12.0, 1 } } }	
	}
,   MPFSpinR =	
	{
		[ 1 ] = { "SharSfx", "Jump-Moving", 5.0, 20.0, 1 }
	}
,   MPFSpinL =	
	{
		[ 1 ] = { "SharSfx", "Jump-Moving", 5.0, 20.0, 1 }
	}	
,   SpinF =	
	{
		[ 1 ] = { "SharSfx", "Jump-Moving", 5.0, 20.0, 1 }
	}							
,	LiftEL =
	{
		[ 1 ] = { sounds = { { "SharSfx", "Jump-Moving", 5.0, 20.0, 1 }, { "SharSfx", "LadderClimbFabric", 5.0, 20.0, 1 } } }
	}
,	LiftER =
	{
		[ 1 ] = { sounds = { { "SharSfx", "Jump-Moving", 5.0, 20.0, 1 }, { "SharSfx", "LadderClimbFabric", 5.0, 20.0, 1 } } }
	}	
,   LiftA =
	{
		[ 1 ] = { "SharSfx", "2ndJump-Spin", 5.0, 20.0, 1 }
	}
,   LiftA1 =
	{
		[ 1 ] = { "SharSfx", "2ndJump-Spin", 5.0, 20.0, 1 }
	}
,   LiftF =
	{
		[ 1 ] = { "SharSfx", "2ndJump-Spin", 5.0, 20.0, 1 }
	}	
,   Flap1 =
	{
		[ 5 ] = { "SharSfx", "ClothEject", 5.0, 20.0, 1 }
	}	
,   Flap3 =
	{
		[ 5 ] = { "SharSfx", "ClothEject", 5.0, 20.0, 1 }
	}		
,   AddFlap =
	{
		[ 5 ] = { "SharSfx", "ClothEject", 5.0, 20.0, 1 }
	}	
,   MPFSpinR =	
	{
		[ 1 ] = { "SharSfx", "2ndJump-Spin", 5.0, 20.0, 1 }
	}	
,   MPFSpinL =	
	{
		[ 1 ] = { "SharSfx", "2ndJump-Spin", 5.0, 20.0, 1 }
	}	
,   SwimBoost =
	{
		[ 1 ] = { "SharSfx", "2ndJump-Spin", 5.0, 20.0, 1 }
	}
,   SwimBoost2 =
	{
		[ 1 ] = { "SharSfx", "ClothEject", 5.0, 20.0, 1 }
	}	
,   SwimBoost3 =
	{
		[ 1 ] = { "SharSfx", "ClothEject", 5.0, 20.0, 1 }
	}		
,   SwimBoostFlap =
	{
		[ 1 ] = { "SharSfx", "ClothEject", 5.0, 20.0, 1 }
	}		
,   FixCloth =
	{
		[ 1 ] = { "SharSfx", "FixClothSmall", 3.0, 12.0, 1 }
	}
,   FixCloth2 =
	{
		[ 1 ] = { "SharSfx", "FixClothSmall", 3.0, 12.0, 1 }
	}
,   FixCloth2X =
	{
		[ 1 ] = { "SharSfx", "FixClothSmall", 3.0, 12.0, 1 }
	}
,   FixCloth3 =
	{
		[ 1 ] = { "SharSfx", "FixClothMedium", 5.0, 20.0, 1 }
	}
,   FixCloth4 =
	{
		[ 1 ] = { "SharSfx", "FixClothShaky", 5.0, 20.0, 1 }
	}	
,   FixCloth4X =
	{
		[ 1 ] = { "SharSfx", "FixClothShaky", 5.0, 20.0, 1 }
	}						
,   FixClothAir =
	{
		[ 1 ] = { "SharSfx", "FixClothSmall", 5.0, 20.0, 1 }
	}
}

local function SortSoundMarkers()
	for name,markers in pairs( dudeAnimSoundMarkers ) do
		table.sort( markers )
	end
end
SortSoundMarkers()

-- This could be optimized, its set up right now to return the last marker passed
-- if we just want to return any marker passed then we can make this simpler
local function GetLastMarkerPassed( oldNum, nextNum, markers )
	local lastFrame = nil
	
	--have we looped
	if nextNum < oldNum then
		-- Check everything above oldNum
		for frame,marker in pairs( markers ) do
			if frame > oldNum then lastFrame = frame end
		end
		-- Check everything below nextNum
		for frame,marker in pairs( markers ) do
			if frame <= nextNum then lastFrame = frame end
		end
	else
		-- Check everything both above oldNum and below nextNum 
		for frame,marker in pairs( markers ) do
			if frame <= nextNum and frame > oldNum then lastFrame = frame end
		end
	end
	return lastFrame
end

local soundMarkerSet = {}
local terrainType = 0
local temprature = 0

function SetSoundMarkerSet( onSand, inAir, terrainNum, coldNum )
	soundMarkerSet = dudeAnimSoundMarkers
	terrainType = terrainNum
	temprature = coldNum
end

function TriggerFromDudeAnim( animName, oldFrameNum, frameNum, soundBarn, dudePos )
	local soundMarkers = soundMarkerSet
	local markers = soundMarkers[ animName ]
	if markers == nil then return false end
	local lastFrame = GetLastMarkerPassed( oldFrameNum, frameNum, markers )
	if lastFrame == nil then return false end
	local sounds = markers[ lastFrame ]
	
	if sounds.rumble then
		local rumbleEvent = game:eventBarn():MetaAddEvent( "Rumble" )
		local rumble = Rumble.cast( rumbleEvent )
		rumble:falloffMinDist( 20 )
		for k,v in pairs( sounds.rumble ) do
			rumble[ k ]( rumble, v )
		end
		rumble:useOriginPos( true )
		rumble:originPos( dudePos )
		rumble:Start()
	end
	if sounds.sounds then 
		for i,sound in ipairs( sounds.sounds ) do
			local maxDist = sound[ 3 ] + 50
			local vol = sound[ 4 ]
			if sound[ 5 ] then 
				maxDist = sound[ 4 ]
				vol = sound[ 5 ]  
			end
			soundBarn:SoundPlayLuaWithRegVal( sound[ 1 ], sound[ 2 ], dudePos, sound[ 3 ], maxDist, vol, terrainType, temprature )
		end
	end
	if sounds.sounds == nil and sounds.rumble == nil then
		local sound = sounds
		local maxDist = sound[ 3 ] + 50
		local vol = sound[ 4 ]
		if sound[ 5 ] then 
			maxDist = sound[ 4 ]
			vol = sound[ 5 ] 
		end
		soundBarn:SoundPlayLuaWithRegVal( sound[ 1 ], sound[ 2 ], dudePos, sound[ 3 ], maxDist, vol, terrainType, temprature )
	end
end
 

-- ***** NOTES *****
-- QueueAnimation( animName, blendIn, blendOut, animSpeed, numLoops, duration )
-- blendIn, blendOut, and duration are in seconds
-- to use duration, set numLoops = 0, otherwise it will be ignored
-- you can set numLoops < 0 to fit a certain number of complete loops in a duration
-- you can set duration to be a negative number if you want it to keep playing until you stop it
-- You still need to add the animation to the animDatas table at the top of this file, it will be this way until we totally
-- switch over to the new system
local animationSequences = {
	Act01LookAround = {
		{ animName = "Act01LookAround", blendIn = 0.4, blendOut = 0.3, animSpeed = 0.125, numLoops = 1, duration = 4 }
	}
,	Act01LookDown = {
		{ animName = "Act01LookDown", blendIn = 0.12, blendOut = 0.4, animSpeed = 0.1, numLoops = 1, duration = 4 }
	}
,	Act01Surprise = {
		{ animName = "StopMed", blendIn = 0, blendOut = 0.2, animSpeed = 0.8, numLoops = 1, duration = 4 }
	,	{ animName = "Act01Surprise", blendIn = 0.4, blendOut = 0.3, animSpeed = 0.24, numLoops = 1, duration = 4 }
	}
,	Act02Surprise = {
		{ animName = "Act01Surprise", blendIn = 0.4, blendOut = 0.9, animSpeed = 0.24, numLoops = 1, duration = 4 }
	,	{ animName = "Act01Hold", blendIn = .3, blendOut = 0.5, animSpeed = .3, numLoops = 0, duration = 8 }
	}
,	Act04Surprise = {
		{ animName = "Act01Surprise", blendIn = 0.4, blendOut = 0.9, animSpeed = 0.24, numLoops = 1, duration = 4 }
	,	{ animName = "Act01Hold", blendIn = .3, blendOut = 0.5, animSpeed = .3, numLoops = 0, duration = 8 }
	}
,	Act06LookAround = {
		{ animName = "Act06LookAround", blendIn = 0, blendOut = 0.4, animSpeed = 0.041666666666667, numLoops = 1, duration = 4 }
	,	{ animName = "LookUp", blendIn = 0.4, blendOut = 0.5, animSpeed = 1, numLoops = 0, duration = 30 }
	}
,	Act06LookLR = {
		{ animName = "LookUp", blendIn = 0.8, blendOut = 0.5, animSpeed = 1.0, numLoops = 0, duration = 4.2 }
	,	{ animName = "Act06LookLR", blendIn = 0.5, blendOut = 1.0, animSpeed = 0.32, numLoops = 1, duration = 4 }
	,	{ animName = "Act06LookAround", blendIn = 1.0, blendOut = 0.4, animSpeed = 0.041666666666667, numLoops = 1, duration = 4 }
	,	{ animName = "LookUp", blendIn = 0.4, blendOut = 0.5, animSpeed = 1, numLoops = 0, duration = 30 }
	}
,	AddFaceShake = {
		{ animName = "AddFaceShake", blendIn = 0.2, blendOut = 0.2, animSpeed = 1, numLoops = 1, duration = 0.6 }
	}
,	AddFlap = {
		{ animName = "AddFlap", blendIn = 0.4, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	AddFlap2 = {
		{ animName = "AddFlap2", blendIn = 0.4, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	AddFlap3 = {
		{ animName = "AddFlap3", blendIn = 0.4, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	AirFlipL = {
		{ animName = "AirFlipL", blendIn = 0.1, blendOut = 0.1, animSpeed = 1, numLoops = 9999999, duration = 0 }
	}
,	AirFlipR = {
		{ animName = "AirFlipR", blendIn = 0.1, blendOut = 0.1, animSpeed = 1, numLoops = 9999999, duration = 0 }
	}
,	Charge = {
		{ animName = "Charge", blendIn = 0.4, blendOut = .4, animSpeed = 0.5, numLoops = 1, duration = 4 }
	}
,	ChargeAir = {
		{ animName = "ChargeAir", blendIn = 0.4, blendOut = 0.2, animSpeed = 1.1, numLoops = 1, duration = 4 }
	}
,	ChargeCold = {
		{ animName = "ChargeCold", blendIn = 0.2, blendOut = 0.6, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	ChargeRunL = {
		{ animName = "ChargeRun", blendIn = 0.2, blendOut = 0.7, animSpeed = 1.5, numLoops = 1, duration = 4 }
	}
,	ChargeRunR = {
		{ animName = "ChargeRun", blendIn = 0.2, blendOut = 0.7, animSpeed = 1.5, numLoops = 1.5, duration = 4 }
	}
,	ChargeWalk = {
		{ animName = "ChargeWalk", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.5, numLoops = 0, duration = 0.25 }
	}
,	ChargeWeak = {
		{ animName = "ChargeWeak", blendIn = 0.2, blendOut = 0.6, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	ClimbUpL = {
		{ animName = "ClimbUpL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 0.5 }
	}
,	ClimbUpL1 = {
		{ animName = "ClimbUpL1", blendIn = 0.3, blendOut = 0.4, animSpeed = 0.8, numLoops = 0, duration = 0.7 }
	}
,	ClimbUpR = {
		{ animName = "ClimbUpR", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 0.5 }
	}
,	ClimbUpR1 = {
		{ animName = "ClimbUpR1", blendIn = 0.3, blendOut = 0.4, animSpeed = 0.8, numLoops = 0, duration = 0.7 }
	}
,	ClimbUpStepL = {
		{ animName = "ClimbUpStepL", blendIn = 0.2, blendOut = 0.3, animSpeed = 0.4, numLoops = 1, duration = 0.7 }
	}
,	ClimbUpStepL1 = {
		{ animName = "ClimbUpStepL1", blendIn = 0.2, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 0.7 }
	}
,	ClimbUpStepR = {
		{ animName = "ClimbUpStepR", blendIn = 0.2, blendOut = 0.3, animSpeed = 0.4, numLoops = 1, duration = 0.7 }
	}
,	ClimbUpStepR1 = {
		{ animName = "ClimbUpStepR1", blendIn = 0.2, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 0.7 }
	}
,	ClimbUpStepWL = {
		{ animName = "ClimbUpStepWL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.25, numLoops = 1, duration = 4 }
	}
,	ClimbUpStepWR = {
		{ animName = "ClimbUpStepWR", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.25, numLoops = 1, duration = 4 }
	}
,	ClimbUpWeakHL = {
		{ animName = "ClimbUpWeakHL", blendIn = 0.2, blendOut = 0.2, animSpeed = 1.5, numLoops = 1, duration = 4 }
	}
,	ClimbUpWeakL1 = {
		{ animName = "ClimbUpWeakL1", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	ClimbUpWeakR1 = {
		{ animName = "ClimbUpWeakR1", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	Collapse = {
		{ animName = "Collapse", blendIn = 1.0, blendOut = 0, animSpeed = 0.6, numLoops = 1, duration = 4 }
	,	{ animName = "Collapsing", blendIn = 0, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	CollapseFast = {
		{ animName = "Collapse", blendIn = 1.0, blendOut = 0.5, animSpeed = 1.5, numLoops = 1, duration = 4 }
	,	{ animName = "Collapsing", blendIn = 0.5, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	Collapse2 = {
		{ animName = "Collapse2", blendIn = 0.4, blendOut = 0, animSpeed = 0.75, numLoops = 1, duration = 4 }
	,	{ animName = "Collapsing2", blendIn = 0, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	CollapseFast2 = {
		{ animName = "Collapse2", blendIn = 1.0, blendOut = 0, animSpeed = 3.0, numLoops = 1, duration = 4 }
	,	{ animName = "Collapsing2", blendIn = 0, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	Collapsing2 = {
		{ animName = "Collapsing2", blendIn = 0.3, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	Cough = {
		{ animName = "Cough", blendIn = 1, blendOut = 1, animSpeed = 0.5, numLoops = 1, duration = 4 }
	}
,	Crouch = {
		{ animName = "Crouch", blendIn = 0.3, blendOut = 0.3, animSpeed = 0.6, numLoops = 1, duration = 4 }
	,	{ animName = "Crouching", blendIn = 0.3, blendOut = 0.1, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	CrouchToIdle = {
		{ animName = "CrouchToIdle", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.5, numLoops = 1, duration = 4 }
	}
,	CrouchToIdleW = {
		{ animName = "CrouchToIdleW", blendIn = 0.1, blendOut = 0.5, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	CrouchWeak = {
		{ animName = "CrouchWeak", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.4, numLoops = 1, duration = 4 }
	,	{ animName = "CrouchingWeak", blendIn = 0.4, blendOut = 0.3, animSpeed = 0.1, numLoops = 0, duration = 999999999 }
	}
,	CrouchWeak1 = {
		{ animName = "CrouchWeak1", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.4, numLoops = 1, duration = 4 }
	,	{ animName = "CrouchingWeak", blendIn = 0.4, blendOut = 0.3, animSpeed = 0.1, numLoops = 0, duration = 999999999 }
	}
,	CrouchingWeak = {
		{ animName = "CrouchingWeak", blendIn = 0.3, blendOut = 0.2, animSpeed = 0.1, numLoops = 0, duration = 999999999 }
	}
,	FallForward = {
		{ animName = "FallForward", blendIn = 0.1, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	,	{ animName = "LieF", blendIn = 0.5, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	FishBoostL = {
		{ animName = "MPFSpinL", blendIn = 0.25, blendOut = 0.5, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	FishBoostR = {
		{ animName = "SpinF", blendIn = 0.5, blendOut = 0.9, animSpeed = 1.5, numLoops = 1, duration = 4 }
	}
,	FixCloth = {
		{ animName = "FixCloth", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.6, numLoops = 0, duration = 0.6 }
	}
,	FixCloth2 = {
		{ animName = "FixCloth2", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.5, numLoops = 1, duration = 0.8 }
	}
,	FixCloth2X = {
		{ animName = "FixCloth2X", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.5, numLoops = 1, duration = 0.8 }
	}
,	FixCloth3 = {
		{ animName = "FixCloth3", blendIn = 0.5, blendOut = 0.5, animSpeed = 2, numLoops = 1, duration = 0.8 }
	}
,	FixCloth4 = {
		{ animName = "FixCloth4", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.75, numLoops = 1, duration = 0.8 }
	}
,	FixCloth4X = {
		{ animName = "FixCloth4X", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.75, numLoops = 1, duration = 0.8 }
	}
,	FixClothAir = {
		{ animName = "FixClothAir", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.6, numLoops = 0, duration = 0.6 }
	}
,	Flap1 = {
		{ animName = "Flap1", blendIn = 0.2, blendOut = 0.8, animSpeed = 1.1, numLoops = 1, duration = 4 }
	}
,	Flap3 = {
		{ animName = "Flap3", blendIn = 0.2, blendOut = 0.8, animSpeed = 1.1, numLoops = 1, duration = 4 }
	}
,	GetUpB = {
		{ animName = "GetUpB", blendIn = 0.15, blendOut = 0.2, animSpeed = 0.75, numLoops = 0, duration = 1.5 }
	,	{ animName = "CrouchToIdleW", blendIn = 0.1, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	GetUpF = {
		{ animName = "GetUpF", blendIn = 0, blendOut = 0.2, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	HitBodyL = {
		{ animName = "HitBodyL", blendIn = 0.2, blendOut = 0.6, animSpeed = 1.4, numLoops = 1.5, duration = 4 }
	}
,	HitBodyLAir = {
		{ animName = "HitBodyLAir", blendIn = 0.3, blendOut = 0.3, animSpeed = 1.0, numLoops = 1, duration = 4 }
	}
,	HitBodyM = {
		{ animName = "HitBodyM", blendIn = 0.1, blendOut = 0.6, animSpeed = 1.6, numLoops = 1.5, duration = 4 }
	}
,	HitBodyMAir = {
		{ animName = "HitBodyMAir", blendIn = 0.4, blendOut = 0.3, animSpeed = 1.0, numLoops = 1, duration = 4 }
	}
,	HitBodyR = {
		{ animName = "HitBodyR", blendIn = 0.2, blendOut = 0.6, animSpeed = 1.4, numLoops = 1.5, duration = 4 }
	}
,	HitBodyRAir = {
		{ animName = "HitBodyRAir", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.0, numLoops = 1, duration = 4 }
	}
,	IdleCheckFootL = {
		{ animName = "IdleCheckFootL", blendIn = 0.2, blendOut = 0.75, animSpeed = 0.3, numLoops = 1, duration = 4 }
	}
,	IdleCheckFootR = {
		{ animName = "IdleCheckFootR", blendIn = 0.2, blendOut = 0.75, animSpeed = 0.3, numLoops = 1, duration = 4 }
	}
,	IdleStretch = {
		{ animName = "IdleStretch", blendIn = 0.5, blendOut = 0.4, animSpeed = 0.15, numLoops = 1, duration = 4 }
	}
,	JumpFL = {
		{ animName = "JumpFL", blendIn = 0.1, blendOut = 0.45, animSpeed = 0.9, numLoops = 1, duration = 4 }
	}
,	JumpFR = {
		{ animName = "JumpFR", blendIn = 0.1, blendOut = 0.45, animSpeed = 0.9, numLoops = 1, duration = 4 }
	}
,	JumpHalfL = {
		{ animName = "JumpHalfL", blendIn = 0.2, blendOut = 0.3, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	JumpHalfR = {
		{ animName = "JumpHalfR", blendIn = 0.2, blendOut = 0.3, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	JumpRoll = {
		{ animName = "JumpRoll", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.42, numLoops = 0, duration = 1.1 }
	}
,	LandHard = {
		{ animName = "Charge", blendIn = 0.2, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	LandSoft = {
		{ animName = "LandSoft", blendIn = 0.2, blendOut = 0.3, animSpeed = 2, numLoops = 1, duration = 4 }
	}
,	LeapL = {
		{ animName = "LeapL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.2, numLoops = 1.4, duration = 1 }
	}
,	LeapR = {
		{ animName = "LeapR", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.2, numLoops = 1.4, duration = 1 }
	}
,	LiftA = {
		{ animName = "LiftA", blendIn = 0.1, blendOut = 0.8, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	LiftA1 = {
		{ animName = "LiftA1", blendIn = 0.1, blendOut = 0.8, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	LiftEL = {
		{ animName = "LiftEL", blendIn = 0.1, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	LiftER = {
		{ animName = "LiftER", blendIn = 0.1, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	LiftF = {
		{ animName = "LiftF", blendIn = 0.1, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	LookBackL = {
		{ animName = "LookBackL", blendIn = 0.5, blendOut = 0.4, animSpeed = 0.1, numLoops = 1, duration = 4 }
	}
,	LookBackR = {
		{ animName = "LookBackR", blendIn = 0.5, blendOut = 0.4, animSpeed = 0.1, numLoops = 1, duration = 4 }
	}
,	LookUp = {
		{ animName = "LookUp", blendIn = 0.8, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 10 }
	}
,	MPFSpinL = {
		{ animName = "MPFSpinL", blendIn = 0.2, blendOut = .3, animSpeed = 0.7, numLoops = 1, duration = 4 }
	}
,	MPFSpinR = {
		{ animName = "MPFSpinR", blendIn = 0.2, blendOut = .3, animSpeed = 0.7, numLoops = 1, duration = 4 }
	}
,	Act01Hold = {
		{ animName = "Act01Hold", blendIn = 1.0, blendOut = 1.0, animSpeed = 1, numLoops = 0, duration = 10 }
	}
,	ReadyToMed = {
		{ animName = "ReadyToMed", blendIn = 0.3, blendOut = 0, animSpeed = 0.2, numLoops = 1, duration = 4 }
	,	{ animName = "Med", blendIn = 0, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	Med = {
		{ animName = "Med", blendIn = 0.1, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	Kneel = {
		{ animName = "Kneel", blendIn = 0.4, blendOut = 0, animSpeed = .75, numLoops = 1, duration = 0 }
	,	{ animName = "Kneeling", blendIn = 0, blendOut = 0, animSpeed = .5, numLoops = 0, duration = 999999999 }
	}
,	Kneeling = {
		{ animName = "Kneeling", blendIn = 0.1, blendOut = 0, animSpeed = .5, numLoops = 0, duration = 999999999 }
	}
,	ResetJoints = {
		{ animName = "IdleCheckFootL", blendIn = 0.2, blendOut = 0.75, animSpeed = 1, numLoops = 0, duration = 0.25 }
	}
,	Revive = {
		{ animName = "Revive", blendIn = 0, blendOut = 0, animSpeed = 0.35, numLoops = 1, duration = 4 }
	,	{ animName = "Reviving", blendIn = 0, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}
,	Revive2 = {
		{ animName = "Revive2", blendIn = 0, blendOut = 0, animSpeed = 0.1, numLoops = 1, duration = 4 }
	,	{ animName = "Reviving2", blendIn = 0, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 999999999 }
	}	
,	RollB = {
		{ animName = "RollB", blendIn = 0.2, blendOut = 0.5, animSpeed = 2, numLoops = 1.5, duration = 0 }
	}
,	RollBToLieB = {
		{ animName = "RollB", blendIn = 0.2, blendOut = 0.2, animSpeed = 2.5, numLoops = 3, duration = 4 }
	,	{ animName = "LieB", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	RollF = {
		{ animName = "RollF", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.45, numLoops = 1, duration = 0 }
	--,	{ animName = "TrippingL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 0 }
	}
,	LandFast = { 
		{animName = "LandFast", blendIn = 0.2, blendOut = 0.6, animSpeed = .8, numLoops = 1, duration = 0 }
	}
,	RollF2 = {
		{ animName = "RollF", blendIn = 0.2, blendOut = 0.2, animSpeed = 2.5, numLoops = 3, duration = 0 }
	,	{ animName = "LieF", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	RollFToLieF = {
		{ animName = "RollF", blendIn = 0.2, blendOut = 0.2, animSpeed = 2.5, numLoops = 3, duration = 0 }
	,	{ animName = "LieF", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	RollLToLieB = {
		{ animName = "RollL", blendIn = 0.2, blendOut = 0.3, animSpeed = 2, numLoops = 2, duration = 4 }
	,	{ animName = "RollB", blendIn = 0.3, blendOut = 0.2, animSpeed = 1.5, numLoops = 0, duration = 0.3 }
	,	{ animName = "LieB", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	RollRToLieF = {
		{ animName = "RollR", blendIn = 0.2, blendOut = 0.3, animSpeed = 2, numLoops = 2, duration = 4 }
	,	{ animName = "RollF", blendIn = 0.3, blendOut = 0.2, animSpeed = 1.5, numLoops = 0, duration = 0.3 }
	,	{ animName = "LieF", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	ShoutBL = {
		{ animName = "ShoutBL", blendIn = 0.3, blendOut = 0.3, animSpeed = 1.15, numLoops = 1, duration = 4 }
	}
,	ShoutBR = {
		{ animName = "ShoutBR", blendIn = 0.3, blendOut = 0.3, animSpeed = 1.15, numLoops = 1, duration = 4 }
	}
,	ShoutL = {
		{ animName = "ShoutL", blendIn = .4, blendOut = 0.2, animSpeed = 1.3, numLoops = 1, duration = 4 }
	}
,	ShoutM = {
		{ animName = "ShoutL", blendIn = .4, blendOut = 0.2, animSpeed = 1.3, numLoops = 1, duration = 4 }
	}
,	ShoutR = {
		{ animName = "ShoutL", blendIn = .4, blendOut = 0.2, animSpeed = 1.3, numLoops = 1, duration = 4 }
	}
,	SlideAdjustB = {
		{ animName = "SlideAdjustB", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideAdjustB1 = {
		{ animName = "SlideAdjustB1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideAdjustF = {
		{ animName = "SlideAdjustF", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideAdjustF1 = {
		{ animName = "SlideAdjustF1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideAdjustL = {
		{ animName = "SlideAdjustL", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideAdjustL1 = {
		{ animName = "SlideAdjustL1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 0, duration = 1 }
	,	{ animName = "SlideGroundSpinL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideAdjustR = {
		{ animName = "SlideAdjustR", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideAdjustR1 = {
		{ animName = "SlideAdjustR1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 0, duration = 1 }
	,	{ animName = "SlideGroundSpinR", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideJump = {
		{ animName = "SlideJump", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.3, numLoops = 1, duration = 4 }
	}
,	SlideJump1 = {
		{ animName = "SlideJump1", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.3, numLoops = 1, duration = 4 }
	}
,	SlideJump2 = {
		{ animName = "SlideJump2", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.6, numLoops = 1, duration = 4 }
	}
,	SlideJump3 = {
		{ animName = "SlideJump2", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.9, numLoops = 1, duration = 4 }
	}	
,	SlideLand = {
		{ animName = "SlideLand", blendIn = 0.2, blendOut = 0.4, animSpeed = 1.2, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustB = {
		{ animName = "SlideXAdjustB", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustB1 = {
		{ animName = "SlideXAdjustB1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustF = {
		{ animName = "SlideXAdjustF", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustF1 = {
		{ animName = "SlideXAdjustF1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustL = {
		{ animName = "SlideXAdjustL", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustL1 = {
		{ animName = "SlideXAdjustL1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 0, duration = 1 }
	,	{ animName = "SlideGroundSpinL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustR = {
		{ animName = "SlideXAdjustR", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.4, numLoops = 1, duration = 4 }
	}
,	SlideXAdjustR1 = {
		{ animName = "SlideXAdjustR1", blendIn = 0.1, blendOut = 0.2, animSpeed = 1, numLoops = 0, duration = 1 }
	,	{ animName = "SlideGroundSpinR", blendIn = 0.2, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	SlideXJump = {
		{ animName = "SlideXJump", blendIn = 0.2, blendOut = 0.8, animSpeed = .3, numLoops = 1, duration = 4 }
	}
,	SlideXJump1 = {
		{ animName = "SlideXJump1", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.3, numLoops = 1, duration = 4 }
	}
,	SlideXJump2 = {
		{ animName = "SlideXJump2", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.6, numLoops = 1, duration = 4 }
	}
,	SlideXJump3 = {
		{ animName = "SlideXJump3", blendIn = 0.2, blendOut = 0.8, animSpeed = 0.9, numLoops = 1, duration = 4 }
	}
,	SlideXLand = {
		{ animName = "SlideXLand", blendIn = 0.2, blendOut = 0.4, animSpeed = 1.2, numLoops = 1, duration = 4 }
	}
,	HopL = {
		{ animName = "HopL", blendIn = 0.2, blendOut = 0.4, animSpeed = 1.6, numLoops = 0, duration = 0.6 }
	}
,	NodHead = {
		{ animName = "NodHead", blendIn = 0.2, blendOut = 0.4, animSpeed = 1.0, numLoops = 1, duration = 0.4 }
	}
,	ShakeHead = {
		{ animName = "ShakeHead", blendIn = 0.2, blendOut = 0.4, animSpeed = 1.0, numLoops = 1, duration = 0.4 }
	}
,	ShakeOff = {
		{ animName = "ShakeOff", blendIn = 0.2, blendOut = 0.4, animSpeed = 1.0, numLoops = 1, duration = 0.4 }
	}
,	Slip = {
		{ animName = "FallForward", blendIn = 0.2, blendOut = 0.3, animSpeed = 0.75, numLoops = 0, duration = 1.0 }
	,	{ animName = "LieF", blendIn = 0.2, blendOut = 0, animSpeed = 0.1, numLoops = 0, duration = 99999999 }
	}
,	BlownBack = {
		{ animName = "BlownBack", blendIn = 0.4, blendOut = 0.2, animSpeed = 1.3, numLoops = 1, duration = 0 }
	,	{ animName = "LieF", blendIn = 0.4, blendOut = 0, animSpeed = 1, numLoops = 0, duration = 99999999 }
	}
,	StopBoostL = {
		{ animName = "StopBoostL", blendIn = 0.2, blendOut = 0.4, animSpeed = 1, numLoops = 1, duration = 0 }
	}
,	StopBoostR = {
		{ animName = "StopBoostR", blendIn = 0.2, blendOut = 0.4, animSpeed = 1, numLoops = 1, duration = 0 }
	}
,	StopL = {
		{ animName = "StopL", blendIn = 0.3, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	StopMed = {
		{ animName = "StopMed", blendIn = 0, blendOut = 0.5, animSpeed = 0.5, numLoops = 0, duration = 1 }
	}
,	StopMedFast = {
		{ animName = "StopMed", blendIn = 0, blendOut = 0.2, animSpeed = 1.5, numLoops = 1, duration = 4 }
	}
,	StopR = {
		{ animName = "StopR", blendIn = 0.3, blendOut = 0.5, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	StopTurnRunL = {
		{ animName = "StopTurnRunL", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.75, numLoops = 1, duration = 4 }
	}
,	StopTurnRunR = {
		{ animName = "StopTurnRunR", blendIn = 0.2, blendOut = 0.3, animSpeed = 1.75, numLoops = 1, duration = 4 }
	}
,	StandTurnL = {
		{ animName = "StandTurnL", blendIn = 0.4, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	StandTurnR = {
		{ animName = "StandTurnR", blendIn = 0.4, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	WalkStopL = {
		{ animName = "WalkStopL", blendIn = 0.6, blendOut = 0.4, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	WalkStopR = {
		{ animName = "WalkStopR", blendIn = 0.6, blendOut = 0.4, animSpeed = 0.5, numLoops = 1, duration = 4 }
	}
,	BoostSpiralL = {
		{ animName = "BoostSpiralL", blendIn = 0.4, blendOut = 0.6, animSpeed = 1, numLoops = 1, duration = 0.75 }
	}
,	BoostSpiralR = {
		{ animName = "BoostSpiralR", blendIn = 0.4, blendOut = 0.6, animSpeed = 1.0, numLoops = 1, duration = 0.75 }
	}
,	BoostFlap = {
		{ animName = "BoostFlap", blendIn = 0.4, blendOut = 0.6, animSpeed = 1, numLoops = 1, duration = 0.75 }
	}
,	BoostFlap2 = {
		{ animName = "BoostFlap2", blendIn = 0.4, blendOut = 0.6, animSpeed = 1, numLoops = 1, duration = 0.75 }
	}
,	BoostFlap3 = {
		{ animName = "BoostFlap3", blendIn = 0.15, blendOut = 0.4, animSpeed = 0.8, numLoops = 1, duration = 0.75 }
	}
,	SwimTurnL = {
		{ animName = "SwimTurnL", blendIn = 0.4, blendOut = 0.5, animSpeed = 0.75, numLoops = 1, duration = 0 }
	}
,	SwimTurnR = {
		{ animName = "SwimTurnR", blendIn = 0.4, blendOut = 0.5, animSpeed = 0.75, numLoops = 1, duration = 0 }
	}
,	TrippingL = {
		{ animName = "TrippingL", blendIn = 0.1, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	TrippingL1 = {
		{ animName = "TrippingL1", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.8, numLoops = 0, duration = 1 }
	}
,	TrippingR = {
		{ animName = "TrippingR", blendIn = 0.1, blendOut = 0.3, animSpeed = 1, numLoops = 1, duration = 4 }
	}
,	TrippingR1 = {
		{ animName = "TrippingR1", blendIn = 0.5, blendOut = 0.5, animSpeed = 0.8, numLoops = 0, duration = 1 }
	}
,	TurnLookBackL = {
		{ animName = "TurnLookBackL", blendIn = 0.2, blendOut = 0.2, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	TurnLookBackR = {
		{ animName = "TurnLookBackR", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
,	TurnLookFastL = {
		{ animName = "TurnLookBackL", blendIn = 0.1, blendOut = 0.2, animSpeed = 0.75, numLoops = 1, duration = 4 }
	}
}

function LoadDudeAnimSequences( animBarn )
	for seqName,seqData in pairs( animationSequences ) do
		animBarn:AddAnimSequence( seqName )
	end
end

function QueueAnimationSequence( animBarn, sequenceName )
	local sequence = animationSequences[ sequenceName ]
	for _,anim in ipairs( sequence ) do
		animBarn:QueueAnimation( anim.animName, anim.blendIn, anim.blendOut, anim.animSpeed, anim.numLoops, anim.duration )
	end
end

function GetDudeAnimSequences()
	local seqList = {}
	for name,_ in pairs( animationSequences ) do
		table.insert( seqList, name )
	end
	return seqList
end

function GetDudeStateAnims()
	local animList = {}
	for name,anim in pairs( animDatas ) do
		if anim.isAction ~= true then
			table.insert( animList, name )
		end
	end
	return animList
end

function GetDudeActionAnims()
	local animList = {}
	for name,anim in pairs( animDatas ) do
		if anim.isAction == true then
			table.insert( animList, name )
		end
	end
	return animList
end

function PlayDudeAnimation( args )
	local name = args.name
	print( "PlayDudeAnimation "..name )
	
	game:playerBarn():GetLocalDude():GetAnimBarn():QueueAnimation( name, 0.1, 0.1, 1, 0, -1 )
end



