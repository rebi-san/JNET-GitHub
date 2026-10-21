
-------------------------------------------------------------------------------
-- Notes:  Code for controlling the Petal Swarm
--
-- Author: Martin Middleton
-- Date C: 7-25-2007
-------------------------------------------------------------------------------

-- Tweakables
local startVel = { 0, 0, 0 }
local startSize = 0.25
local startMass = 2
local startBumpForce = 2
local currentG = 0

local forceMult = 5.0
local forceGravity = 5
local minHeight = 0.25
local minSpd = 0.05
local idealHeight = 1.2
local dampen = 0.5
local triggerR = 0.0

--local heightLimit = 10
local kSwarmSmallHeight = 2
local kSwarmLargeHeight = 10
local kNumPetalsForMaxHeight = 200

-- Called by SwarmLead::Initialize()
function InitSwarmLead( swarmLead )	
	-- Init tweakable member variables here instead of in the constructor
	local startRel = { 0.0, 0.0, -2.0 }
	local rotRel = vec3_quat_mult( startRel, StartLocationInstances[1].Rotation )
	
	local startPos = 
	{ StartLocationInstances[ 1 ].Position[ 1 ] + rotRel[1]
	, StartLocationInstances[ 1 ].Position[ 2 ] + rotRel[2]
	, StartLocationInstances[ 1 ].Position[ 3 ] + rotRel[3] }
	
--	local startPos = 
--	{ StartLocationInstances[ 1 ].Position[ 1 ]
--	, StartLocationInstances[ 1 ].Position[ 2 ]
--	, StartLocationInstances[ 1 ].Position[ 3 ] }
	swarmLead:SetPos( startPos )
	swarmLead:m_vel( startVel )
	swarmLead:m_size( startSize )
	swarmLead:m_spdBonus( 1 )
	swarmLead:m_colBonus( 1 )
	swarmLead:m_mass( startMass )
	swarmLead:m_bumpForce( startBumpForce )
	swarmLead:m_minSpd( minSpd )
	swarmLead:m_forceMult( forceMult )
end

-- Called by SwarmLead::Update()
function UpdateSwarmLead( swarmLead, dt, input, cameraNode, terrain, hullBarn, landPos, isLandable )
	swarmLead:m_mass( startMass )
	swarmLead:m_bumpForce( startBumpForce )
	-- Load from c++
	local pos = swarmLead:GetPos()
	local vel = swarmLead:m_vel()
	local exp = swarmLead:m_numActivePetals()
	--print(exp)
	-- experience grows size
	local size = (1+math.sqrt(exp)*0.15)*startSize


	-- Read pad data
	local pad = input:GetPad( 0 )
	local cheatsEnabled = Vars.Game.cheatsEnabled()
	
	-- @Hack prototype how wind volume force might affect controls
	if cheatsEnabled == true and (pad:Button( Channel_Button_Right )>0) then
		local windBoost = ScalarMult(Normalize(vel),15)
		vel = LerpV3(0.2, vel, windBoost)
	end
	
	local sensor = pad:Sensor()
	--triggerR = Lerp(0.2,triggerR,CalculateMaxButtonPressureRight(pad,cheatsEnabled))	
	--triggerR = 1 --Lerp(0.2,triggerR,CalculateMaxButtonPressure(pad,cheatsEnabled))
	triggerR = Lerp(0.2,triggerR,CalculateMaxButtonPressure(pad,cheatsEnabled))
	
	-- Switch the axis of input vector
	local swap = sensor[2]
	sensor[2] = sensor[3]
	sensor[3] = swap

	-- Rotate to align with camera
	local camMatrix = cameraNode:GetWorldMatrix()
	local dir = CastV3V4( sensor )
	dir = mat4_vec4_mult( camMatrix, dir )

    -- Alternate possible swarm controls (more sensitive)
-- local baseSpd = 0.5 
-- local xSens = 15
-- local ySens = 20 
--    startDir = { 0, 0, -baseSpd }    
--    startDir = vec3_rotY( startDir, -sensor[1]*xSens )
--    startDir = vec3_rotX( startDir, sensor[2]*ySens )
--	local camMatrix = cameraNode:GetWorldMatrix()
--	local dir = CastV3V4( startDir )
--	dir = mat4_vec4_mult( camMatrix, dir )

	-- apply min speed
	vel = VectorAdd( vel, ScalarMult(dir, swarmLead:m_minSpd()) )

	-- boost
	local totalForce = 0
	totalForce = triggerR * triggerR * swarmLead:m_forceMult() * size * math.sqrt(swarmLead:m_spdBonus())
	--totalForce = triggerR * triggerR * swarmLead:m_forceMult() * size * swarmLead:m_comboBonus()

	-- Calc the new speed
	local speed = Length( vel )
	speed = speed + totalForce * dt * swarmLead:m_spdMult()

	-- Steer
	vel = vec3_slerp( 0.25, Normalize( vel ), Normalize( dir ) )
	vel = ScalarMult( Normalize( vel ), speed )	

	-- Air friction
	vel = VectorAdd(vel, ScalarMult(vel, -1*dt*dampen))

	-- Avoid the ground outside landing zone
	local newMinHeight = minHeight
	if isLandable then
		newMinHeight = 0.05
		local toLand = Normalize(VectorAdd(landPos,ScalarMult(pos,-1)))
		vel = VectorAdd(vel, ScalarMult(VectorAdd( ScalarMult(toLand,3), ScalarMult(vel,-1)),dt*20))
	end
	
	swarmLead:m_boostDuration( swarmLead:m_boostDuration() - dt )
	if swarmLead:m_boostDuration() > 0 then
		local windBoost = ScalarMult( Normalize(vel), swarmLead:m_boostStrength() )
		vel = LerpV3( 0.2, vel, windBoost )
	end

	-- Integrate position
	pos = VectorAdd( pos, ScalarMult( vel, dt ) )

	-- Settle at an ideal height which affects the maximum height player can go
	if not swarmLead:IsLanded() then
		local groundHeight = swarmLead:GetHeight( pos[ 1 ], pos[ 3 ], terrain )
		if pos[2] < groundHeight+idealHeight then
			pos[ 2 ] = Lerp( 0.01, pos[ 2 ], groundHeight + idealHeight )
		else
			-- Apply gravity
			if swarmLead:m_applyGravity() and isLandable == false then
				--pos[ 2 ] = Lerp( 0.01, pos[ 2 ], groundHeight + idealHeight )
				
				-- High limit
				local swarmSize = exp
				local heightLerp = Clip( swarmSize / kNumPetalsForMaxHeight, 0, 1 )
				local heightLimit = Lerp( heightLerp, kSwarmSmallHeight, kSwarmLargeHeight )
				--print( heightLimit, swarmSize, pos[ 2 ] )
				if pos[2]>groundHeight+heightLimit then
					--pos[ 2 ] = Lerp( 0.01, pos[ 2 ], heightLimit )
					-- gravity
					local G = Lerp(pos[2]/(groundHeight+heightLimit)-1,0,forceGravity)
					currentG = Lerp(0.005,currentG,G)
					--vel[ 2 ] = vel[ 2 ] - G * dt						
					pos[2] = pos[2] - currentG * dt
				else
					currentG = 0
				end
			end
		end
	end	

	-- Set in c++
	swarmLead:SetPos( pos )
	swarmLead:m_vel( vel )	
	swarmLead:m_size( size )
end
	
-- Helper Functions

-- Some vector math functions
function VectorAdd( vec0, vec1 )
	local ret = { vec0[1] + vec1[1], vec0[2] + vec1[2], vec0[3] + vec1[3] }
	return ret
end

function ScalarMult( vec0, scalar )
	local ret = { vec0[1] * scalar, vec0[2] * scalar, vec0[3] * scalar }
	return ret
end

function Normalize( vec )
	local weight = math.sqrt (vec[1]*vec[1]+vec[2]*vec[2]+vec[3]*vec[3])
	local ret = { 0, 0, 0 }
	if weight>0 then
		ret = {vec[1]/weight,vec[2]/weight,vec[3]/weight}
	end
	return ret
end

function Length( vec )
	local ret = math.sqrt (vec[1]*vec[1]+vec[2]*vec[2]+vec[3]*vec[3])
	return ret
end

function LerpV3( alpha, vec0, vec1 )
	local ret = { 0, 0, 0 }
	ret[ 1 ] = Lerp( alpha, vec0[ 1 ], vec1[ 1 ] )
	ret[ 2 ] = Lerp( alpha, vec0[ 2 ], vec1[ 2 ] )
	ret[ 3 ] = Lerp( alpha, vec0[ 3 ], vec1[ 3 ] )
	return ret
end

-- this is needed because you can only multiply a matrix4 by a vector4
function CastV3V4( vec3 )
	local vec4 = { vec3[1], vec3[2], vec3[3], 0.0 }
	return vec4
end

function Lerp( alpha, x0, x1 )
	return x0 + alpha * ( x1 - x0 )
end

function Clip( val, min, max )
	local ret = val
	if ret < min then ret = min end
	if ret > max then ret = max end
	return ret
end

function Abs( value )
	local ret = value
	if ret < 0 then ret = ret * -1 end
	return ret
end

function Dot( v0, v1 )
	return v0[1]*v1[1] + v0[2]*v1[2] + v0[3]*v1[3]
end

