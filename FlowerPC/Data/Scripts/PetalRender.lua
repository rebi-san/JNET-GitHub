-------------------------------------------------------------------------------
-- Notes: Data for various swarm glow colors
--
-- Author: Martin Middleton
-- Date C: 09-24-2008
-------------------------------------------------------------------------------

local petalGlowProfiles =
{
	Default	= 
	{   glowColor = { 255, 235, 225 }
	,   glowMult = 0.04
	,	hueCycleSpeed = 5
	,   hueIntensityGlow = 0.3
	,   hueIntensityPetal = 0.2 
	}
,	Red = 
	{	glowColor = { 255, 0, 0 }
	,	glowMult = 0.05
	,	hueCycleSpeed = 0.02
	,	hueIntensityGlow = 0.1
	,	hueIntensityPetal = 0.01
	}
,	Blue = 
	{	glowColor = { 0, 0, 255 }
	,	glowMult = 0.075
	,	hueCycleSpeed = 0.00
	,	hueIntensityGlow = 0.00
	,	hueIntensityPetal = 0.001
	}
,	Orange = 
	{	glowColor = { 250, 255, 150 }
	,	glowMult = 0.05
	,	hueCycleSpeed = 0.02
	,	hueIntensityGlow = 0.1
	,	hueIntensityPetal = 0.001
	}
,	Cyan = 
	{	glowColor = { 0, 200, 255 }
	,	glowMult = 0.05
	,	hueCycleSpeed = 0.02
	,	hueIntensityGlow = 0.0
	,	hueIntensityPetal = 0.001
	}
,	Pink = 
	{	glowColor = { 255, 150, 244 }
	,	glowMult = 0.05
	,	hueCycleSpeed = 0.02
	,	hueIntensityGlow = 0.1
	,	hueIntensityPetal = 0.001
	}
,	Star = 
	{	glowColor = { 200, 200, 255 }
	,	glowMult = 0.035
	,	hueCycleSpeed = 20
	,	hueIntensityGlow = 0.5
	,	hueIntensityPetal = 0.0
	}
,	Credits = 
	{	glowColor = { 200, 200, 255 }
	,	glowMult = 0.035
	,	hueCycleSpeed = 20
	,	hueIntensityGlow = 0.5
	,	hueIntensityPetal = 0.0
	}
}

function SetPetalGlowContext( glowContext, profileName )
	local profile = petalGlowProfiles[ profileName ]
	glowContext:name( profileName )
	glowContext:glowColor( profile.glowColor )
	glowContext:glowMult( profile.glowMult )
	glowContext:hueCycleSpeed( profile.hueCycleSpeed )
	glowContext:hueIntensityGlow( profile.hueIntensityGlow )
	glowContext:hueIntensityPetal( profile.hueIntensityPetal )
end
