-------------------------------------------------------------------------------
-- Notes:  Definitions for particles
--
-- Author: Martin Middleton
-- Date C: 8-20-2007
-------------------------------------------------------------------------------


-- Texture is represented by a grid number starting at lower left of textureAtlas
local gridWidth = 128
local atlasWidth = 512

-- To allocate the worst-case would be wasteful, so here's a guess at the bell curve
local particleAllocMult = 2.0

local textureMap =
{
	cloud   = 0
,	sphere  = 1
,   egg     = 2
,	halo    = 3
,	dirt    = 4
,   ripple  = 5
,   evil    = 6
,   flare   = 7
,   lightning = 8
,	grass   = 9
,	debris	= 10
,	storm	= 11
,	petal	= 12
,	petal2  = 13
,	petal3  = 14
,   ring    = 15
}

-- Emitters are tied to particle types
local emitterMap =
{
	[1] = "LandingZone"
,	[2] = "LandingZoneDisabled"
,	[3] = "Dirt"
,	[4] = "Filler"
,	[5] = "LeadFlower"
,	[6] = "SwarmDeposit"
,	[7] = "FlowerHit"
,	[8] = "SwarmFront"
,	[9] = "SwarmFront2"
,	[10] = "WindGuide"
,	[11] = "SwarmPaint"
,	[12] = "WindWallRibbon"
,	[13] = "Lvl2_Blue"
,	[14] = "Lvl2_Yellow"
,	[15] = "Lvl2_Purple"
,	[16] = "Lvl2_Red"
,	[17] = "WindWall"
,	[18] = "Boost"
,	[19] = "Dust"
,	[20] = "Windmill"
,	[21] = "Rain"
,	[22] = "LandingZoneRibbon"
,	[23] = "WindRiver"
,	[24] = "RedLight"
,	[25] = "HeavyRain"
,	[26] = "FlowerRibbon"
,	[27] = "WindShockwave"
,	[28] = "Spark"
,	[29] = "FlowerShower"
,	[30] = "LandImpact"
,	[31] = "Grass"
,	[32] = "Godray"
,	[33] = "VineExplode"
,	[34] = "DustTrail"
,	[35] = "Smoke"
,	[36] = "BuildingRise"
,	[37] = "WindmillLarge"
,	[38] = "WindmillRibbon"
,	[39] = "ElectricFlare"
,	[40] = "FlowerSmoke"
,	[41] = "ZapFlower"
,	[42] = "BrokenLight"
,	[43] = "LandSpark"
,	[44] = "LandImpact2"
,	[45] = "PetalBreak"
,	[46] = "Storm"
,	[47] = "Lightning"
,	[48] = "Lvl2_Green"
,	[49] = "Leaf"
,	[50] = "VineBlueExplode"
,	[51] = "VineRedExplode"
,	[52] = "VineYellowExplode"
,	[53] = "PetalStorm"
,	[54] = "SwarmHalo"
,	[55] = "ColorHaloPink"
,	[56] = "ColorHaloBlue"
,	[57] = "ColorHaloYellow"
,	[58] = "LightHalo"
,	[59] = "LightCone"
,	[60] = "FreewayWind"
,	[61] = "TowerFlower"
,	[62] = "PetalStorm2"
,	[63] = "PetalStorm3"
,	[64] = "Firefly"
,	[65] = "RockGate"
,	[66] = "Ripple"
,	[67] = "SmallLight"
,	[68] = "StoneCircleDust"
,	[69] = "PaintCompleteFirefly"
,	[70] = "PinkGlow"
,	[71] = "CyanGlow"
,	[72] = "OrangeGlow"
,	[73] = "Spiral"
,	[74] = "Pond"
,	[75] = "WaterSplash"
,	[76] = "Revival"
,	[77] = "RevivalRibbon"
,	[78] = "Debris"
,	[79] = "SmallFlare"
,	[80] = "RainRipple"
,	[81] = "HeavyRainRipple"
,	[82] = "CaveWind"
,	[83] = "SecretFlowerGlow"
,	[84] = "Stars"
,	[85] = "ShootingStars"
,	[86] = "Lvl2_BluePaint"
,	[87] = "Lvl2_YellowPaint"
,	[88] = "Lvl2_PurplePaint"
,	[89] = "WindmillStart"
,	[90] = "TowerTree"
,	[91] = "BigGodray"
,	[92] = "LandingZoneRibbonPostDof"
}
  
local shaderMap =
{
	Default  = 0
,	Additive = 1
,	Ribbon = 2
,	Soft = 3
,	LowResSubtractive = 4
,   AdditivePostDOF = 5
}

-- Note: there is a particle limit of 8000 total and 1000 per emitter
-- Each emitter will have at most ( max spawn rate * max lifetime ) particles at once 
-- In addition there is a limit of 32 total emitters existing at once
-- Spread  0 - line, 1 - hemisphere, 2 - sphere 
local emitterData =
{
	SecretFlowerGlow = { particle = "FlowerGlowP", spawnRate = { 1, 5 }, spd = { 0.8, 1.8 }, spread = 1, radius = { 0.0, 0.0 }, shader = "Additive" }

,	LandingZone = { particle = "LandingZoneP", spawnRate = { 40, 100 }, spd = { -0.5, -1.5 }, spread = 0.0, radius = { 4, 5 } }
,	LandingZoneRibbon = { particle = "LandingZoneRibbonP", spawnRate = { 10, 15 }, spd = { 0.0, 0.0 }, spread = 0.0, radius = { 6, 7 }, shader="Ribbon", ribbonLength = { 20, 30 } }
,	LandingZoneRibbonPostDof = { particle = "LandingZoneRibbonP", spawnRate = { 10, 15 }, spd = { 0.0, 0.0 }, spread = 0.0, radius = { 6, 7 }, shader="Ribbon", ribbonLength = { 20, 30 }, ribbonPostDof = true }
,	LandingZoneDisabled = { particle = "LandingZoneDisabledP", spawnRate = { 30, 40 }, spd = { 0.0, 1.0 }, spread = 1, radius = { 0.5, 1 }, shader = "Additive" }

,   Dirt = { particle = "DirtP", spawnRate = { 25, 30 }, spd = { 4, 7 }, spread = 0.3, radius = { 0, 0.5 }, spawnExtra=0 }
,   Filler = { particle = "FillerFlowerP", spawnRate = { 120, 130 }, spd = { 0, 0.25 }, spread = 0.7, radius = { 0, 0.15 }, shader = "Additive" }
,   DustTrail = { particle = "DustTrailP", spawnRate = { 15, 18 }, spd = { 3, 5 }, spread = 0.5, radius = { 0, 0 }, panRadius = 0.5, shader = "Soft", spawnExtra=0 }

,   LeadFlower = { particle = "LeadFlowerP", spawnRate = { 10, 15 }, spd = { 0.4, 0.8 }, spread = 2, radius = { 0.0, 0.0 }, shader = "AdditivePostDOF" }
,   SwarmDeposit = { particle = "DepositP", spawnRate = { 8, 10 }, spd = { 0.06, 0.1 }, spread = 0, radius = { 2.0, 2.51 } }
,   LandImpact = { particle = "LandImpactP", spawnRate = { 18, 20 }, spd = {1,1}, spread = 0.6, radius = { 0,0 }, panRadius = 0.2 }

,   FlowerHit = { particle = "FlowerHitP", spawnRate = { 10, 20 }, spd = { 1.5, 1.6 }, spread = 0.3, radius = { 0, 0 } }

,   SwarmFront = { particle = "SwarmP", spawnRate = { 5, 12 }, spd = { -2, -4 }, spread = 0.5, radius = { 0.5, 0.75 } }
,   SwarmFront2 = { particle = "SwarmP2", spawnRate = { 2, 4 }, spd = { -2, -4 }, spread = 0.5, radius = { 0, 0.1 }, shader = "Ribbon" }

,   WindGuide = { particle = "WindP", spawnRate = { 2, 3 }, spd = { 0.0,0.0 }, spread = 2, radius = { 0, 0.1 }, shader = "Ribbon" }
,   SwarmPaint = { particle = "FillerFlowerPaint", spawnRate = { 50, 50 }, spd = { 0, 2 }, spread = 2, radius = { 0.15, 0.25 }, shader = "Additive" }

,   LandSpark = { particle = "LandSparkP", spawnRate = { 5, 5 }, spd = { 1, 5 }, spread = 0.5, radius = { 0, 0.01 }, shader="Ribbon", ribbonLength = { 10, 30 } }
,   LandImpact2 = { particle = "LandImpact2P", spawnRate = { 80, 90 }, spd = {2,2}, spread = 0.5, radius = { 0, 0 }, panRadius = 0.5, shader = "Additive" }

--level 1
,   WindWallRibbon = { particle = "WindStreakP", spawnRate = { 5, 10 }, spd = { 12.0, 15.0 }, spread = 0.0, radius = { 4, 8 }, shader = "Ribbon", ribbonLength = { 10, 12 }  }
,   Leaf = { particle = "LeafP", spawnRate = { 0, 3 }, spd = { 0, 0 }, spread = 0.0, radius = { 0,0 }, panRadius = 8}

--level 2
,   Lvl2_Blue = { particle = "Lvl2_BlueP", spawnRate = { 10, 15 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 6, shader = "Additive" }
,   Lvl2_Yellow = { particle = "Lvl2_YellowP", spawnRate = { 10, 15 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 6, shader = "Additive" }
,   Lvl2_Purple = { particle = "Lvl2_PurpleP", spawnRate = { 10, 15 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 6, shader = "Additive"  }
,   Lvl2_BluePaint = { particle = "Lvl2_BluePaintP", spawnRate = { 20, 25 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 7, shader = "Additive" }
,   Lvl2_YellowPaint = { particle = "Lvl2_YellowPaintP", spawnRate = { 20, 25 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 7, shader = "Additive" }
,   Lvl2_PurplePaint = { particle = "Lvl2_PurplePaintP", spawnRate = { 20, 25 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 7, shader = "Additive"  }
,   Lvl2_Red = { particle = "Lvl2_RedP", spawnRate = { 10, 15 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 6, shader = "Additive"}
,   Lvl2_Green = { particle = "Lvl2_GreenP", spawnRate = { 10, 15 }, spd = { 0, 0 }, spread = 0.5, radius = {0,0}, panRadius = 6, shader = "Additive"}
,   Dust = { particle = "DustP", spawnRate = { 4, 6 }, spd = { 3, 5 }, spread = 0.5, radius = { 0, 1.5 }, shader = "Additive", spawnExtra=0}
,   WindWall = { particle = "WindWallP", spawnRate = { 50, 80 }, spd = { 12.0, 15.0 }, spread = 0.0, radius = { 4, 8 }, shader = "Additive"  }
,   Boost = { particle = "BoostP", spawnRate = { 3, 5 }, spd = { 0, 0 }, spread = 0, radius = { 0.1, 0.3 }, shader = "Ribbon", ribbonLength = { 10, 20 }}
,   ColorHaloPink = { particle = "ColorHaloPinkP", spawnRate = { 4, 16 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader="Additive", relativeToEmitter = true, spawnExtra=0 }
,   ColorHaloBlue = { particle = "ColorHaloBlueP", spawnRate = { 4, 16 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader="Additive", relativeToEmitter = true, spawnExtra=0 }
,   ColorHaloYellow = { particle = "ColorHaloYellowP", spawnRate = { 4, 16 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader="Additive", relativeToEmitter = true, spawnExtra=0 }
,   RockGate = { particle = "RockGateP", spawnRate = { 50, 60 }, spd = { 4, 7 }, spread = 0.3, radius = { 0.3, 1.5 }, spawnExtra=0 }
,   StoneCircleDust = { particle = "StoneCircleDustP", spawnRate = { 70, 60 }, spd = { 0.5, 2 }, spread = 1.0, radius = { 1, 3}, shader = "Additive", spawnExtra=0}
,   PinkGlow = { particle = "PinkGlowP", spawnRate = { 4, 6 }, spd = { 1, -1 }, spread = 0.0, radius = { 0, .25}, shader = "Additive"}
,   CyanGlow = { particle = "CyanGlowP", spawnRate = { 4, 6 }, spd = { 1, -1 }, spread = 0.0, radius = { 0, .25}, shader = "Additive"}
,   OrangeGlow = { particle = "OrangeGlowP", spawnRate = { 4, 6 }, spd = { 1, -1 }, spread = 0.0, radius = { 0, .25}, shader = "Additive"}
--level 3
,   Windmill = { particle = "WindmillP", spawnRate = { 10, 10 }, spd = { 10, 10 }, spread = 0.0, radius = { 0,0 }, panRadius = 3}
,   WindmillRibbon = { particle = "WindmillRibbonP", spawnRate = { 1, 2 }, spd = { 10, 10 }, spread = 0.0, radius = { 0,0 }, panRadius = 1, shader = "Ribbon", ribbonLength = { 10, 20 }}
,   WindmillLarge = { particle = "WindmillLargeP", spawnRate = { 40, 40 }, spd = { 10, 15 }, spread = 0.2, radius = { 0,0 }, panRadius = 8}
,   WindRiver = { particle = "WindRiverP", scaleInTime = 0.4, spawnRate = { 5, 8 }, spd = { 30.0, 50.0 }, spread = 0.0, radius = { 4, 8 }, shader = "Ribbon" , ribbonLength = { 18, 25 } }
,   FlowerRibbon = { particle = "FlowerRibbonP", spawnRate = { 15, 20 }, spd = { 30, 50 }, spread = 0.1, radius = { 0, 0 }, panRadius = 3.5}
,   WindShockwave = { particle = "WindShockwaveP", spawnRate = { 30, 40 }, spd = { 20, 40 }, spread = 1, radius = { 0, 0 }, panRadius = 1, shader = "Ribbon",ribbonLength = { 10, 20 }}
,   FlowerShower = { particle = "FlowerShowerP", spawnRate = { 70, 80 }, spd = { 10, 15 }, spread = 0.0, radius = { 0, 0 }, panRadius = 20}
,   CaveWind = { particle = "CaveWindP", spawnRate = { 10, 10 }, spd = { 30.0, 35.0 }, spread = 0.0, radius = { 1, 2 }, shader = "Ribbon", ribbonLength = { 15, 20 }}
,   WindmillStart = { particle = "WindmillStartP", spawnRate = { 10, 15 }, spd = { 2, 3 }, spread = 1, radius = { 0.1, 0.3 } }
--level 4
,   BrokenLight = { particle = "BrokenLightP", spawnRate = { 0.1, 5 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0.05 }, shader = "Additive", spawnExtra=0 }
,   Storm = { particle = "StormP", spawnRate = { 0.1, 0.2 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, panRadius = 128, shader = "Additive" }
,   Lightning = { particle = "LightningP", spawnRate = { 0.1, 0.2 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, panRadius = 128, shader = "Additive" }
,   LightHalo = { particle = "LightHaloP", spawnRate = { 0.1, 8 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader="Additive", relativeToEmitter = true, spawnExtra=0 }
,   LightCone = { particle = "LightConeP", spawnRate = { 0.1, 3 }, spd = { 0, 0 }, spread = 0, radius = { 0, 2 }, shader="Additive"}
,   Firefly = { particle = "FireflyP", spawnRate = { 20, 30 }, spd = { 0, 0 }, spread = 2, radius = { 2, 3 }, shader="AdditivePostDOF"}
,   Grass = { particle = "GrassP", spawnRate = { 50, 60 }, spd = { 0.2, 1.0}, spread = 2, radius = { 0, 0.5 } }
,   Ripple = {particle = "RippleP", spawnRate = { 8, 12 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader="Additive", billboardDirection = { 0, 1, 0 }, spawnExtra=0}
,   SmallLight = { particle = "SmallLightP", spawnRate = { 0.5, 0.5 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader = "Additive", spawnExtra=0 }
,   PaintCompleteFirefly = { particle = "PaintCompleteFireflyP", spawnRate = { 50, 75 }, spd = { 0.5, 0.8 }, spread = 0.2, radius = { 0, 0 }, panRadius = 10, shader="AdditivePostDOF" }
,   Spiral = { particle = "SpiralP", spawnRate = { 75, 150 }, spd = { 0.5, 0.8 }, spread = 0.2, radius = { 0, 0 }, panRadius = 25, shader="AdditivePostDOF" }
,   Pond = { particle = "PondP", spawnRate = { 50, 100 }, spd = { 0.5, 0.8 }, spread = 0.2, radius = { 0, 0 }, panRadius = 10, shader="Additive" }
,	WaterSplash = { particle = "WaterSplashP", spawnRate = { 20, 30 }, spd = { 1, 2 }, spread = 0.1, radius = { 0, 0.15 }, shader = "Additive" }
,   Debris = { particle = "DebrisP", spawnRate = { 1, 5 }, spd = { 14, 27 }, spread = 0.3, radius = { 0.5, 1.5 } }
,   ElectricFlare = { particle = "ElectricFlareP", spawnRate = { 80, 110 }, spd = { 0, 0 }, spread = 0, radius = { 0, .25 }, shader="Additive"}
--level 5
,   Rain = { particle = "RainP", spawnRate = { 60, 80 }, spd = { 5, 10 }, spread = 0, radius = { 0, 0 }, panRadius = 20, shader = "Ribbon", ribbonLength = { 3, 5 }}
,   HeavyRain = { particle = "HeavyRainP", spawnRate = { 200, 250 }, spd = { 20, 30 }, spread = 0.0, radius = { 0, 0 }, panRadius = 20, shader = "Ribbon", ribbonLength = { 4, 8 }}
,   RainRipple = { spawnOnGround = true, heightAboveGround = 0.25, particle = "RainRippleP", spawnRate = { 200, 300 }, spd = { 0.05, 0.1 }, spread = 0.4, radius = { 0, 0 }, panRadius = 20, shader="Additive"}
,   HeavyRainRipple = { spawnOnGround = true, heightAboveGround = 0.25, particle = "RainRippleP", spawnRate = { 400, 600 }, spd = { 0.1, 0.2 }, spread = 0.8, radius = { 0, 0 }, panRadius = 20, shader="Additive"}
,   RedLight = { particle = "RedLightP", spawnRate = { 0.5, 0.5 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader = "Additive", spawnExtra=0 }
,   Spark = { particle = "SparkP", spawnRate = { 1, 20 }, spd = { 4, 8 }, spread = 0.5, radius = { 0, 0.1 }, shader="Ribbon", ribbonLength = { 4, 8 } }
,   SmallFlare = { particle = "SmallFlareP", spawnRate = { 1, 10 }, spd = { 0, 0 }, spread = 0, radius = { 0,0 }, shader="Additive"}
,   ZapFlower = { particle = "ElectricFlareP", spawnRate = { 80, 110 }, spd = { 0, 0 }, spread = 0, radius = { 0, .1 }, shader="Additive"}
,   FlowerSmoke = { particle = "FlowerSmokeP", spawnRate = { 20, 30 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0.02 }, shader="LowResSubtractive", spawnExtra=0 }
,   PetalBreak = { particle = "PetalBreakP", spawnRate = { 4, 6 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0.01 } }
--level 6
,   VineExplode = { particle = "VineP", spawnRate = { 100, 150 }, spd = { -30, -40 }, spread = 1.2, radius = { 0, 0.2 } }
,   VineBlueExplode = { particle = "VineBlueP", spawnRate = { 40, 40 }, spd = { 30, 40 }, spread = 0.1, radius = { 0, 0.2 } }
,   VineRedExplode = { particle = "VineRedP", spawnRate = { 40, 40 }, spd = { 30, 40 }, spread = 0.1, radius = { 0, 0.2 } }
,   VineYellowExplode = { particle = "VineYellowP", spawnRate = { 40, 40 }, spd = { 30, 40 }, spread = 0.1, radius = { 0, 0.2 } }
,   Smoke = { particle = "SmokeP", spawnRate = { 15, 25 }, spd = { 50, 80 }, spread = 0.05, radius = { 0, 0.2 } }
,   BuildingRise = { particle = "DustP", spawnRate = {12, 15 }, spd = { 10, 20 }, spread = 1, radius = { 0, 0 }, panRadius = 6, shader="Additive", spawnExtra=0 }
,   Godray = { particle = "GodrayP", spawnRate = { 40, 80 }, spd = { 5, 7 }, spread = 2, radius = { 0, 0.01 }, shader = "Ribbon", ribbonLength = { 10, 20 } }
,   BigGodray = { particle = "BigGodrayP", scaleInTime = 0.4, spawnRate = { 10, 12 }, spd = { 0, 0 }, spread = 2, radius = { 7, 8 }, shader = "Ribbon" }
,   Revival = { particle = "RevivalP", spawnRate = { 1, 40 }, spd = { 0.2, 1 }, spread = 1, radius = { 0.02, 0.04 }}
,   RevivalRibbon = { particle = "RevivalP", spawnRate = { 1, 40 }, spd = { 0.2, 1 }, spread = 1, radius = { 0.02, 0.04 }, shader = "Ribbon", ribbonLength = { 8, 15 }, ribbonPostDof = true }
,   PetalStorm = { particle = "PetalStormP", scaleInTime = 0.4, spawnRate = { 50, 80 }, spd = { 10, 15 }, spread = 0.0, radius = { 0, 0 }, panRadius = 20, spawnExtra=0.5}
,   PetalStorm2 = { particle = "PetalStorm2P", scaleInTime = 0.4, spawnRate = { 50, 80 }, spd = { 10, 15 }, spread = 0.0, radius = { 0, 0 }, panRadius = 20, spawnExtra=0.5}
,   PetalStorm3 = { particle = "PetalStorm3P", scaleInTime = 0.4, spawnRate = { 40, 60 }, spd = { 10, 15 }, spread = 0.0, radius = { 0, 0 }, panRadius = 20, spawnExtra=0.5}
,   SwarmHalo = { particle = "SwarmHaloP", spawnRate = { 0.1, 8 }, spd = { 0, 0 }, spread = 0, radius = { 0, 0 }, shader="Additive", relativeToEmitter = true, spawnExtra=0 }
,   FreewayWind = { particle = "FreewayWindP", spawnRate = { 10, 15 }, spd = { 30.0, 35.0 }, spread = 0.0, radius = { 5, 8 }, shader = "Ribbon", ribbonLength = { 10, 12 }}
,   TowerFlower = { particle = "TowerFlowerP", spawnRate = { 250, 350 }, spd = { 30, 40 }, spread = 2.0, radius = { 2, 4 }}
,   TowerTree = { particle = "TowerTreeP", spawnRate = { 0, 500 }, spd = { 0, 0 }, spread = 2.0, radius = { 0,0 }, panRadius = 120}
--level 7
,	Stars = { particle = "StarsP", scaleInTime = 3, spawnRate = { 1, 500 }, spd = { 0, 0 }, spread = 0.0, radius = { 700,700 }, shader = "Additive", relativeToEmitter = true}
,	ShootingStars = { particle = "ShootingStarsP", spawnRate = { 0.1, 4 }, spd = { 750, 1000 }, spread = 1.5, radius = { 665,665 }, shader = "Ribbon", ribbonLength = { 10, 15 }, relativeToEmitter = true}
}

-- forceMlt = { gravity, wind, noise, attraction }
local particleData =
{
	FlowerGlowP = { scale = { 0.02, 0.05 }, lifetime = { 2.5, 3.2 }, spin = { 0,1 }, decay = {0.12, 0.15}, forceMlt = {-0.05, 1.5, 0.1, 0}, 
	texture = "egg", cStart = {225,255,55}, cEnd = {155,255,188}, forceVortex = 1 }
,   LandingZoneP = { scale = { 0.3, 0.6 }, lifetime = { 1.5, 2.0 }, spin = { 2,3 }, decay = {0.23, 0.25}, forceMlt = {0.9, 1, 1, 3}, 
	texture = "petal", cStart = {255,150,150}, cEnd = {255,75,75}, forceVortex = 30.0, forceCentripetal = 10 }
,   LandingZoneRibbonP = { scale = { 0, 0 }, scaleEnd = {0.1,0.15}, lifetime = { 2, 3 }, spin = { 2,3 }, decay = {0.1, 0.2}, forceMlt = {0, 0, 1, 3}, 
	texture = "petal", cStart = {255,255,255}, cEnd = {255,0,0}, forceVortex = 30.0, forceCentripetal = 10 }
,   LandingZoneDisabledP = { scale = { 0.9, 1.2 }, lifetime = { 2.0, 2.7 }, spin = { 1, 4 }, decay = {0.2, 0.3}, forceMlt = {-0.25, 1, 0.5, 0},	
	texture = "evil", cStart = {13,12,12}, cEnd = {0,0,0}, forceVortex = 13.0 }
,   DirtP = { scale = { 0.05, 0.1 }, lifetime = { 1.0, 2.0 }, spin = { -4, 4 }, decay = {0.1, 0.3}, forceMlt = {2, 0, 0, 0}, 
	texture = "dirt", cStart = {50,30,20}, cEnd = {20,10,0} }
,   FillerFlowerP = { scale = { 0.05, 0.07 }, lifetime = { 4, 6 }, spin = { 5, 10 }, decay = {0.05, 0.11}, forceMlt = {-0.05, 10, 5, 0},
	texture = "egg", cStart = {128,255,255}, cEnd = {255,255,255}}
,   DustTrailP = { scale = {0,0}, scaleEnd = { 15, 20 }, lifetime = { 4.0, 6.0 }, spin = { -0.1, 0.1 }, decay = {0.1, 0.3}, forceMlt = {0.25, 3, 0.5, 0},
	texture = "cloud", cStart = {12,12,12}, cEnd = {0,0,0}}
,   LeadFlowerP = { scale = { 0.006, 0.01 }, lifetime = { 0.5, 1.2 }, spin = { 0,1 }, decay = {0.16, 0.2}, forceMlt = {-0.025, 5, 0.1, 0}, 
	texture = "egg", cStart = {255,255,255}, cEnd = {255,188,188}, forceVortex = 1 }
,   DepositP = { scale = { 0.3, 0.4 }, lifetime = { 0.8, 0.9 }, spin = { 0,1 }, decay = {0.1, 0.1}, forceMlt = {0, 0, 0, 8}, 
	texture = "petal", cStart = {255,155,155}, cEnd = {255,255,255}}
,   LandImpactP = { scale = { 0.1, 0.15 }, lifetime = { 0.5, 1 }, spin = { 0,1 }, decay = {0.2, 0.3}, forceMlt = {-0.1, 0, 0, 0}, 
	texture = "sphere", cStart = {255,155,155}, cEnd = {0,0,0}}
,   FlowerHitP = { scale = { .03, 0.05}, lifetime = { 0.5, 0.5 }, spin = { 1, 2 }, decay = {0.1, 0.15}, forceMlt = {0,1,0,0},
	texture = "egg", cStart = {255,255,100}, cEnd = {255,255,255}}
,   SwarmP = { scale = { 0.2, 0.4 }, lifetime = { 3.0, 3.5 }, spin = { 1,3 }, decay = {0.1, 0.1}, forceMlt = {-0.1, 1, 1, 0},
	texture = "petal", cStart = {255,64,0}, cEnd = {255,64,0} , forceVortex = 1.0 }
,   SwarmP2 = { scale = { 0.1,0.15 }, scaleEnd = {0.1,0.15 }, lifetime = { 2.8, 3.5 }, spin = { 0.5,1.5 }, decay = {0.001, 0.01}, forceMlt = {0, 0, 2, 25},
	texture = "grass", cStart = {255,55,205}, cEnd = {0,0,0}, forceVortex = 15.0 }
,   WindP = { scale = { 0.0,0.0 }, scaleEnd = {0.15,0.20 }, lifetime = { 1.8, 2.5 }, spin = { 0.5,1.5 }, decay = {0.001, 0.01}, forceMlt = {0, 0, 2, 5},
	texture = "grass", cStart = {255,255,255}, cEnd = {0,0,0}, forceVortex = 5.0 }
,   FillerFlowerPaint = { scale = { 0.1, 0.15 }, lifetime = { 2, 3 }, spin = { 5, 10 }, decay = {0.05, 0.11}, forceMlt = {-0.05, 2.5, 1.5, 0},
	texture = "sphere", cStart = {128,255,255}, cEnd = {255,255,255} }
,   LandSparkP = { scale = { 0, 0 }, scaleEnd = {0.03,0.03}, lifetime = { 1.5, 2.0 }, spin = { 2,3 }, decay = {0.1, 0.3}, forceMlt = {-.75, 0, 1, 3}, texture = "petal", cStart = {255,0,0}, cEnd = {255,255,255}, forceVortex = 10.0, forceCentripetal = 10 }
,	LandImpact2P = { scale = { .2, .3 }, scaleEnd = {0.0,0.1}, lifetime = { 1.5, 2.0 }, spin = { 2,3 }, decay = {0.2, .3}, forceMlt = {-1, 0, 1, 3}, texture = "petal", cStart = {255,255,255}, cEnd = {255,0,0}, forceVortex = 10.0, forceCentripetal = 5 }
--level 1
,   WindStreakP = { scale = {0,0}, scaleEnd = { 0.2,0.25 }, lifetime = { 1.8, 2.5 }, spin = { 0.5,1.5 }, decay = {0.001, 0.01}, forceMlt = {0, 3, 3, 0},
	texture = "petal", cStart = {155,155,155}, cEnd = {0,0,0}}
,   LeafP = { scale = { 0.2, 0.4 }, scaleEnd = {0.2,0.4}, lifetime = { 5, 8 }, spin = { 2,3 }, decay = {0.15, 0.2}, forceMlt = {0.9, 10, 5, 0}, 
	texture = "petal", cStart = {7,9,5}, cEnd = {100,155,50}, forceVortex = 3.0}
--level 2
,   Lvl2_BlueP = { scale = { 0.12, 0.15 }, lifetime = { 4.0, 6.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.5, 10, 5, 0 },
	texture = "egg", cStart = { 0, 100, 255 }, cEnd = { 128, 128, 255 }}
,   Lvl2_YellowP = { scale = { 0.12, 0.15 }, lifetime = { 4.0, 6.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.5, 10, 5, 0 },
	texture = "egg", cStart = { 255, 255, 0 }, cEnd = { 255, 255, 0 }}
,   Lvl2_PurpleP = { scale = { 0.12, 0.15 }, lifetime = { 4.0, 6.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.5, 10, 5, 0 },
	texture = "egg", cStart = { 255, 0, 240 }, cEnd = { 255, 128, 240 }}
,   Lvl2_BluePaintP = { scale = { 0.15, 0.17 }, lifetime = { 6.0, 8.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.4, 5, 5, 0 },
	texture = "egg", cStart = { 0, 100, 255 }, cEnd = { 128, 128, 255 }, forceVortex = 5.0, forceCentripetal = 5}
,   Lvl2_YellowPaintP = { scale = { 0.15, 0.17 }, lifetime = { 6.0, 8.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.4, 5, 5, 0 },
	texture = "egg", cStart = { 255, 255, 0 }, cEnd = { 255, 255, 0 }, forceVortex = 5.0, forceCentripetal = 5}
,   Lvl2_PurplePaintP = { scale = { 0.15, 0.17 }, lifetime = { 6.0, 8.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.4, 5, 5, 0 },
	texture = "egg", cStart = { 255, 0, 240 }, cEnd = { 255, 128, 240 }, forceVortex = 5.0, forceCentripetal = 5}	
,   Lvl2_RedP = { scale = { 0.12, 0.15 }, lifetime = { 4.0, 6.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.5, 10, 5, 0 },
	texture = "egg", cStart = { 255, 0, 0 }, cEnd = { 255, 128, 128 }}
,   Lvl2_GreenP = { scale = { 0.12, 0.15 }, lifetime = { 4.0, 6.0 }, spin = { 1, 5 }, decay = { 0.1, 0.2 }, forceMlt = { -0.5, 10, 5, 0 },
	texture = "egg", cStart = { 0, 255, 0 }, cEnd = { 128, 255, 128 }}
,   DustP = { scale = {0,0}, scaleEnd = { 15, 20 }, lifetime = { 4.0, 6.0 }, spin = { -0.1, 0.1 }, decay = {0.1, 0.3}, forceMlt = {0.25, 3, 0.5, 0},
	texture = "cloud", cStart = {25,25,25}, cEnd = {0,0,0}}
,   WindWallP = { scale = { 0.2,0.25 }, lifetime = { 1.8, 2.5 }, spin = { 0.5,1.5 }, decay = {0.001, 0.01}, forceMlt = {0, 0, 4, 0}, 
	texture = "petal", cStart = {255,255,255}, cEnd = {0,0,0}}	
,   BoostP = { scale = { 0.2,0.4 }, scaleEnd = {0.2,0.4 }, lifetime = { 1.0, 2.0 }, spin = { 0,0 }, decay = {0.01, 0.01}, forceMlt = {0, 0, 2, 18}, 
	texture = "sphere", cStart = {50,50,50}, cEnd = {0,0,0}, forceVortex = 28.0, forceCentripetal = 20 }		
,   ColorHaloPinkP = { scale = {0,0}, scaleEnd = {1,1.5}, lifetime = { 1.6, 1.6 }, spin = { -1.5,1.5 }, decay = {0.0, 0.0}, forceMlt = {0,0,0,0},
	texture = "flare", cStart = {255,150,150}, cEnd = {0,0,0}}		
,   ColorHaloBlueP = { scale = {0,0}, scaleEnd = {1,1.5}, lifetime = { 1.6, 1.6 }, spin = { -1.5,1.5 }, decay = {0.0, 0.0}, forceMlt = {0,0,0,0},
	texture = "flare", cStart = {50,50,255}, cEnd = {0,0,0}}		
,   ColorHaloYellowP = { scale = {0,0}, scaleEnd = {1,1.5}, lifetime = { 1.6, 1.6 }, spin = { -1.5,1.5 }, decay = {0.0, 0.0}, forceMlt = {0,0,0,0},
	texture = "flare", cStart = {255,255,50}, cEnd = {0,0,0}}	
,   RockGateP = { scale = { 0.05, 0.2 }, lifetime = { 2.0, 3.0 }, spin = { -8, 8 }, decay = {0.1, 0.3}, forceMlt = {2, 0, 0, 0}, 
	texture = "dirt", cStart = {20,10,0}, cEnd = {20,10,0}}
,   StoneCircleDustP = { scale = {2,2}, scaleEnd = { 3, 5 }, lifetime = { 0.5, 2.0 }, spin = { -0.1, 0.1 }, decay = {0.01, 0.02}, forceMlt = {0.01, 0, 0.0, 0},
	texture = "cloud", cStart = {25,25,25}, cEnd = {0,0,0}}
,   PinkGlowP = { scale = {0,0}, scaleEnd = { 3, 4 }, lifetime = { 7.0, 15.0 }, spin = { -0.3, 0.1 }, decay = {0.1, 0.2}, forceMlt = {-0.05, 0, 0.0, 0},
	texture = "cloud", cStart = {255, 150, 244}, cEnd = {255, 150, 244}}
,   OrangeGlowP = { scale = {0,0}, scaleEnd = { 3, 4 }, lifetime = { 7.0, 15.0 }, spin = { -0.3, 0.1 }, decay = {0.1, 0.2}, forceMlt = {-0.05, 0, 0.0, 0},
	texture = "cloud", cStart = {250, 255, 150}, cEnd = {250, 255, 150}}
,   CyanGlowP = { scale = {0,0}, scaleEnd = { 3, 4 }, lifetime = { 7.0, 15.0 }, spin = { -0.3, 0.1 }, decay = {0.1, 0.2}, forceMlt = {-0.05, 0, 0.0, 0},
	texture = "cloud", cStart = {0, 200, 255}, cEnd = {0, 200, 255}}
--level 3
,   WindmillP = { scale = { 0.2,0.25 }, scaleEnd = { 0.2,0.25 }, lifetime = { 1.0, 1.5 }, spin = { 0.5,1.5 }, decay = {0.01, 0.1}, forceMlt = {0, 0, 0, 0},
	texture = "petal", cStart = {255,155,205}, cEnd = {255,55,205}, forceVortex = 8.0, forceCentripetal = 8 }	
,   WindmillRibbonP = { scale = { 0,0 }, scaleEnd = { 0.2,0.25 }, lifetime = { 1.0, 1.5 }, spin = { 0.5,1.5 }, decay = {0.01, 0.01}, forceMlt = {0, 0, 0, 0},
	texture = "petal", cStart = {255,55,205}, cEnd = {0,0,0}, forceVortex = 8.0, forceCentripetal = 8 }	
,   WindmillLargeP = { scale = { 0.3,0.4 }, scaleEnd = { 0.3,0.4 }, lifetime = { 1.0, 1.5 }, spin = { 0.5,1.5 }, decay = {0.01, 0.1}, forceMlt = {0, 0, 0, 0},
	texture = "petal", cStart = {255,155,205}, cEnd = {255,55,205}, forceVortex = 8.0, forceCentripetal = 8 }		
,   WindRiverP = { scale = { 0,0 }, scale = { 0.3,0.35 }, lifetime = { 2.0, 2.0 }, spin = { 0.5,1.5 }, decay = {0.05, 0.06}, forceMlt = {0, 0, 6, 0.4}, 
	texture = "petal", cStart = {155,155,155}, cEnd = {0,0,0}}	
,   FlowerRibbonP = { scale = { 0.2,0.3 }, scaleEnd = { 0.2,0.3 }, lifetime = { 3.0, 4.5 }, spin = { 0.5,1.5 }, decay = {0.1, 0.2}, forceMlt = {2.25, 1, 1, -1},
	texture = "petal", cStart = {185,155,155}, cEnd = {255,155,205}, forceVortex = 15.0, forceCentripetal = 10 }	
,   WindShockwaveP = { scale = { 0,0 }, scaleEnd = { 0.2,0.3 }, lifetime = { 3.0, 4.5 }, spin = { 0.5,1.5 }, decay = {0.1, 0.15}, forceMlt = {-0.1, 1, 1, 0.3},
	texture = "petal", cStart = {255,55,205}, cEnd = {255,155,205}, forceVortex = 15.0, forceCentripetal = 10 }	
,   FlowerShowerP = { scale = { 0.3,0.4 }, lifetime = { 5.0, 5.5 }, spin = { -5,5 }, decay = {0.01, 0.02}, forceMlt = {-0.1, 10, 1, 0},
	texture = "petal", cStart = {85,25,25}, cEnd = {255,155,205}}	
,   CaveWindP = { scale = {0,0}, scaleEnd = { 0.4,0.45 }, lifetime = { 1.0, 1.5 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 1, 1},
	texture = "petal", cStart = {255,255,255,0}, cEnd = {0,0,0}}		
,   WindmillStartP = { scale = { 0.1, 0.2 }, lifetime = { 2.0, 3.0 }, spin = { -8, 8 }, decay = {0.1, 0.3}, forceMlt = {1.5, 0, 0, 0}, 
	texture = "dirt", cStart = {150,130,120}, cEnd = {50,30,20}}
--level 4	
,   BrokenLightP = { scale = { 0.25, 1 }, scaleEnd = { 0.5, 1 }, lifetime = { 0.2, 0.8 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 0, 0}, 
	texture = "sphere", cStart = {255,0,0}, cEnd = {255,75,0}}
,   StormP = { scale = { 100, 100 }, scaleEnd = {100,100}, lifetime = { 1.0, 2.0 }, spin = { -0.01,0.01 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 0, 0}, 
	texture = "storm", cStart = {255,255,255}, cEnd = {0,0,0}}
,   LightningP = { scale = { 100, 100 }, scaleEnd = {100,100}, lifetime = { 1.0, 2.0 }, spin = { -0.01,0.01 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 0, 0}, 
	texture = "lightning", cStart = {255,255,255}, cEnd = {0,0,0} }
,   LightHaloP = { scale = {0,0}, scaleEnd = {1.5,1.5}, lifetime = { 1.6, 1.6 }, spin = { -0.5,0.5 }, decay = {0.0, 0.0}, forceMlt = {0,0,0,0},
	texture = "halo", cStart = {255,235,255}, cEnd = {0,0,0}}		
,   LightConeP = { scale = {0.05,0.15}, lifetime = { 1.6, 3.6 }, spin = { -5,5 }, decay = {0.1, 0.15}, forceMlt = {0,0.5,1,0},
	texture = "sphere", cStart = {200,255,255}, cEnd = {200,255,255}}		
,   GrassP = { scale = { 0.5, 1.5 }, lifetime = { 1.5, 4.0 }, spin = { -1, 1 }, decay = {0.2, 0.3}, forceMlt = {2, 10, 5, 0}, 
	texture = "grass", cStart = {55,55,85}, cEnd = {25,35,45}}
,   FireflyP = { scale = { 0.12, 0.15 }, lifetime = { 4.0, 6.0 }, spin = { 1, 3 }, decay = { 0.1, 0.2 }, forceMlt = { -0.2, 3, 1, -0.5 },
	texture = "egg", cStart = { 100, 255, 255 }, cEnd = { 100, 200, 255 }, forceVortex = -4.0, forceCentripetal = 1}
,   RippleP = { scale = {0,0}, scaleEnd = {5,6}, lifetime = { 1.5, 1.6 }, spin = { -0.5,0.5 }, decay = {0.0, 0.0}, forceMlt = {0.01,0,0,0},
	texture = "ripple", cStart = {215,225,255}, cEnd = {0,0,0}}
,   SmallLightP = { scale = { 0.5, 1 }, lifetime = { 4.0, 4.0 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 0, 0}, 
	texture = "sphere", cStart = {255,120,0}, cEnd = {0,0,0}}
,   PaintCompleteFireflyP = { scale = { 0.12, 0.15 }, lifetime = { 0.7, 20.0 }, spin = { 0, 0 }, decay = { 0.1, 0.2 }, forceMlt = { -0.1, 5, 2, 0 },
	texture = "egg", cStart = { 100, 255, 255 }, cEnd = { 100, 200, 255 }}
,   SpiralP = { scale = { 0.12, 0.15 }, lifetime = { 0.7, 20.0 }, spin = { 0, 0 }, decay = { 0.1, 0.2 }, forceMlt = { -0.3, 5, 2, 0 },
	texture = "egg", cStart = { 100, 255, 255 }, cEnd = { 100, 200, 255 }, forceVortex = -5.0, forceCentripetal = 1}
,   PondP = { scale = { 0.1, 0.12 }, lifetime = { 0.7, 20.0 }, spin = { 0, 0 }, decay = { 0.1, 0.2 }, forceMlt = { -0.05, 5, 2, 0 },
	texture = "egg", cStart = { 100, 255, 255 }, cEnd = { 255, 205, 0 }}
,   WaterSplashP = { scale = { 0.01, 0.02}, lifetime = { 1.0, 2.0 }, spin = { -10,10 }, decay = {0.1, 0.1}, forceMlt = {-0.1, 0.5, 0.1, 0}, 
	texture = "egg", cStart = {255,255,255}, cEnd = {255,255,255}}
,   DebrisP = { scale = { 0.05, 0.8}, lifetime = { 1.0, 2.0 }, spin = { -25,25 }, decay = {0.1, 0.3}, forceMlt = {2, 0, 0, 0}, 
	texture = "debris", cStart = {0,0,0}, cEnd = {0,0,0}}
,   ElectricFlareP = { scale = { 0.5, 2 }, scaleEnd = { 1, 2 }, lifetime = { 0.2, 0.4 }, spin = { 0,0 }, decay = {0.03, 0.1}, forceMlt = {0, 0, 0, 0}, 
	texture = "flare", cStart = {0,0,255}, cEnd = {255,128,0}}	
--level 5
,   RainP = { scale = { 0.05, 0.1 }, lifetime = { 1.0, 2.0 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {3, 7, 1, 0}, 
	texture = "sphere", cStart = {25,25,25}, cEnd = {120,120,120}}
,   HeavyRainP = { scale = { 0.05, 0.15 }, lifetime = { 1.0, 2.0 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {3, 20, 2, 0}, 
	texture = "sphere", cStart = {25,25,25}, cEnd = {120,120,120} }
,   RainRippleP = { scale = {0.1,0.16}, lifetime = { 0.2, 0.4 }, spin = { 0,0 }, decay = {0.0, 0.0}, forceMlt = {0.1,0.2,0,0},
	texture = "ring", cStart = {255,255,255}, cEnd = {0,0,0}}
,   RedLightP = { scale = { 0.5, 1 }, lifetime = { 4.0, 4.0 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 0, 0}, 
	texture = "sphere", cStart = {255,0,0}, cEnd = {0,0,0}}
,   SparkP = { scale = { 0.06, 0.08 }, scaleEnd = { 0.03, 0.04 }, lifetime = { 0.5, 1.5 }, spin = { 0,0 }, decay = {0.06, 0.1}, forceMlt = {0.5, 1, 0, 0}, 
	texture = "sphere", cStart = {255,128,0}, cEnd = {255,0,0}}
,   SmallFlareP = { scale = { 0.1, 0.2 }, scaleEnd = { 0.2, 0.4 }, lifetime = { 0.2, 0.4 }, spin = { 0,0 }, decay = {0.03, 0.1}, forceMlt = {0, 0, 0, 0}, 
	texture = "flare", cStart = {155,0,0}, cEnd = {155,64,0}}	
,   ZapFlowerP = { scale = { 0.5, 2 }, scaleEnd = { 0.5, 2 }, lifetime = { 0.2, 0.4 }, spin = { 0,0 }, decay = {0.03, 0.1}, forceMlt = {0, 0, 0, 0}, 
	texture = "flare", cStart = {0,0,255}, cEnd = {255,128,0}}
,   FlowerSmokeP = { scale = { 0.2, 0.4 }, scaleEnd = { 2, 3.2 }, lifetime = { 2.0, 4.0 }, spin = { -0.5, 0.5 }, decay = {0.1, 0.2}, forceMlt = {-0.9, 3, 1, 0.0}, 
	texture = "cloud", cStart = {5,5,5}, cEnd = {0,0,0} }
,   PetalBreakP = { scale = { 0.03, 0.07 }, lifetime = { 0.5, 1.0 }, spin = { -0.5, 0.5 }, decay = {0.1, 0.2}, forceMlt = {0.5, 3, 1, 0}, 
	texture = "dirt", cStart = {0,0,0}, cEnd = {50,50,50}}
--level 6
,   VineP = { scale = { 0.3, 0.4 }, lifetime = { 4.0, 6.0 }, spin = { -10, 10 }, decay = {0.1, 0.2}, forceMlt = {-0.6, 10, 3, 0}, 
	texture = "petal", cStart = {255,200,200}, cEnd = {255,255,255}}
,   VineBlueP = { scale = { 0.5, 0.7 }, lifetime = { 4.0, 6.0 }, spin = { -10, 10 }, decay = {0.05, 0.1}, forceMlt = {-0.25, 1, 1, 0}, 
	texture = "petal", cStart = {50,50,200}, cEnd = {155,155,255}}
,   VineRedP = { scale = { 0.5, 0.7 }, lifetime = { 4.0, 6.0 }, spin = { -10, 10 }, decay = {0.05, 0.1}, forceMlt = {-0.25, 1, 1, 0}, 
	texture = "petal", cStart = {200,50,50}, cEnd = {255,155,155} }
,   VineYellowP = { scale = { 0.5, 0.7 }, lifetime = { 4.0, 6.0 }, spin = { -10, 10 }, decay = {0.05, 0.1}, forceMlt = {-0.25, 1, 1, 0}, 
	texture = "petal", cStart = {200,200,50}, cEnd = {255,255,155}}
,   SmokeP = { scale = { 25.0, 30.0 }, lifetime = { 5.0, 6.0 }, spin = { -0.5, 0.5 }, decay = {0.03, 0.05}, forceMlt = {-0.7, 3, 1, 0.0}, 
	texture = "cloud", cStart = {25,25,25}, cEnd = {0,0,0}}
,   GodrayP = { scale = { 0.3,0.5 }, lifetime = { 0.8, 1.0 }, spin = { 0,0 }, decay = {0.05, 0.07}, forceMlt = {0, 0, 0, 0},
	texture = "petal", cStart = {255,255,255}, cEnd = {255,255,255}, forceVortex = 3.0, forceCentripetal = 3 }		
,   BigGodrayP = { scale = { 0,0 }, scaleEnd = {0.8,1.0}, lifetime = { 2, 2.2 }, spin = { 0,0 }, decay = {0.18, 0.2}, forceMlt = {0, 0, 0, -8},
	texture = "petal", cStart = {200,200,200}, cEnd = {0,0,0}, forceVortex = 13.0, forceCentripetal = 13 }		
,   RevivalP = { scale = { 0,0 }, scaleEnd = {0.01,0.02}, lifetime = { 0.8, 1.0 }, spin = { 0,0 }, decay = {0.05, 0.07}, forceMlt = {-0.1, 2, 0, -1},
	texture = "petal", cStart = {255,255,255}, cEnd = {255,0,0}, forceVortex = 3.0, forceCentripetal = 3 }
,   RevivalRibbonP = { scale = { 0,0 }, scaleEnd = {0.01,0.02}, lifetime = { 0.8, 1.0 }, spin = { 0,0 }, decay = {0.05, 0.07}, forceMlt = {-0.1, 2, 0, -1},
	texture = "petal", cStart = {255,255,255}, cEnd = {255,0,0}, forceVortex = 3.0, forceCentripetal = 3 }
,   PetalStormP = { scale = { 0.3,0.4 }, lifetime = { 5.0, 5.5 }, spin = { -5,5 }, decay = {0.01, 0.02}, forceMlt = {-0.1, 10, 1, 0},
	texture = "petal", cStart = {105,45,45}, cEnd = {255,155,205}}	
,   PetalStorm2P = { scale = { 0.25,0.35 }, lifetime = { 5.0, 5.5 }, spin = { -3,3 }, decay = {0.02, 0.03}, forceMlt = {-0.1, 10, 1, 0},
	texture = "petal2", cStart = {85,25,25}, cEnd = {255,155,205}}	
,   PetalStorm3P = { scale = { 0.25,0.35 }, lifetime = { 5.0, 5.5 }, spin = { -1,1 }, decay = {0.03, 0.04}, forceMlt = {-0.1, 10, 1, 0},
	texture = "petal3", cStart = {85,25,25}, cEnd = {255,155,205}}	
,   SwarmHaloP = { scale = {0,0}, scaleEnd = {5,5}, lifetime = { 1.6, 1.6 }, spin = { -0.5,0.5 }, decay = {0.0, 0.0}, forceMlt = {0,0,0,0},
	texture = "halo", cStart = {255,235,225}, cEnd = {0,0,0}}
,   FreewayWindP = { scale = {0,0}, scaleEnd = { 0.4,0.45 }, lifetime = { 1.5, 2.0 }, spin = { 0,0 }, decay = {0.03, 0.03}, forceMlt = {0, 0, 1, 1},
	texture = "petal", cStart = {255,255,255}, cEnd = {0,0,0}}		
,   TowerFlowerP = { scale = {2.0,2.2}, lifetime = { 7.0, 10.0 }, spin = { -0.5, 0.5 }, decay = {0.03, 0.05}, forceMlt = {-2, 3, 3, 0},
	texture = "petal", cStart = {155,55,55}, cEnd = {255,255,255}}
,   TowerTreeP = { scale = {1.5,2.7}, scaleEnd = {1.5,2.7}, lifetime = { 10.0, 25.0 }, spin = { 2, 3 }, decay = {0.05, 0.12}, forceMlt = {1, 30, 10, 0},
	texture = "petal", cStart = {255,130,150}, cEnd = {255,180,200}, forceVortex = -3.0,forceCentripetal = -1}		
--level 7
,   StarsP = { scale = {2,8}, lifetime = { 8, 20 }, spin = { -1,1 }, decay = {0.1, 0.1}, forceMlt = {0,0,0,0},
	texture = "ring", cStart = {255,255,255}, cEnd = {255,255,255}}
,   ShootingStarsP = { scale = {5,10}, lifetime = { 0.5, 1.2 }, spin = { 10,10 }, decay = {0.01, 0.03}, forceMlt = {0,0,0,0},
	texture = "sphere", cStart = {255,255,255}, cEnd = {255,255,255}, forceVortex = 20.0, forceCentripetal = 1}
}

-- Init functions 
function InitParticleRender( particleRender )
	particleRender:m_atlasGridWidth( atlasWidth / gridWidth )
end

function LoadEmitterType( particleBarn, emitter )
	local typeNum = emitter:type()
	--print("typenum: ", typeNum )
	local typeName = emitterMap[ typeNum ]
	local type = emitterData[ typeName ]
	
	-- Set particle info
	local particle = particleData[ type.particle ]
	
	-- Optional fields
	local scaleEnd = { 0,0 }
	if ( particle.scaleEnd ) then
		scaleEnd[ 1 ] = particle.scaleEnd[ 1 ]
		scaleEnd[ 2 ] = particle.scaleEnd[ 2 ]
	end
	
	if ( type.shader ) then emitter:shaderType( shaderMap[ type.shader ] )
	else emitter:shaderType( 0 ) end
	
	
	local blueprint = emitter:GetBlueprint()
	blueprint:SetScale( particle.scale[ 1 ], particle.scale[ 2 ] )
	blueprint:SetScaleEnd( scaleEnd[ 1 ], scaleEnd[ 2 ] )
	blueprint:SetLifetime( particle.lifetime[ 1 ], particle.lifetime[ 2 ] )
	blueprint:SetSpin( particle.spin[ 1 ], particle.spin[ 2 ] )
	blueprint:SetDecay( particle.decay[ 1 ], particle.decay[ 2 ] )
	blueprint:SetForceMlt( particle.forceMlt[ 1 ], particle.forceMlt[ 2 ], particle.forceMlt[ 3 ], particle.forceMlt[ 4 ] )
	blueprint:forceVortex( 0.0 )
	blueprint:forceCentripetal( 0.0 )
	if ( particle.forceVortex ) then blueprint:forceVortex( particle.forceVortex ) end
	if ( particle.forceCentripetal ) then blueprint:forceCentripetal( particle.forceCentripetal ) end
	
	blueprint:texture( textureMap[ particle.texture ] )
	--blueprint:SetColor( particle.cStart[ 1 ], particle.cStart[ 2 ], particle.cStart[ 3 ], particle.cEnd[ 1 ], particle.cEnd[ 2 ], particle.cEnd[ 3 ] )
	if ( particle.cStart[ 4 ] ) then blueprint:SetColorStart( particle.cStart[ 1 ], particle.cStart[ 2 ], particle.cStart[ 3 ], particle.cStart[ 4 ] )
	else blueprint:SetColorStart( particle.cStart[ 1 ], particle.cStart[ 2 ], particle.cStart[ 3 ], 255 ) end
	if ( particle.cEnd[ 4 ] ) then blueprint:SetColorEnd( particle.cEnd[ 1 ], particle.cEnd[ 2 ], particle.cEnd[ 3 ], particle.cEnd[ 4 ] )
	else blueprint:SetColorEnd( particle.cEnd[ 1 ], particle.cEnd[ 2 ], particle.cEnd[ 3 ], 255 ) end
	
	-- Set emitter info
	--local spawnTime = { 1 / type.spawnRate[ 2 ], 1 / type.spawnRate[ 1 ] }
	--local minSpawnTime = spawnTime[ 1 ]
	--if type.maxSpawnRate then minSpawnTime = 1 / type.maxSpawnRate end
	
	--local constSpawnTime = spawnTime[ 1 ] + math.random() * ( spawnTime[ 2 ] - spawnTime[ 1 ] )
	
	--emitter:minSpawnTime( minSpawnTime )
	--emitter:SetSpawntime( spawnTime[ 1 ], spawnTime[ 2 ] )
	--emitter:SetSpawntime( constSpawnTime, constSpawnTime )
	emitter:SetSpawnRate( type.spawnRate[ 1 ], type.spawnRate[ 2 ] )
	emitter:SetSpd( type.spd[ 1 ], type.spd[ 2 ] )
	emitter:SetSpread( type.spread )
	emitter:SetRadius( type.radius[ 1 ], type.radius[ 2 ] )
	
	if ( type.ribbonLength ) then emitter:SetRibbonLength( type.ribbonLength[ 1 ], type.ribbonLength[ 2 ] )
	else emitter:SetRibbonLength( 12, 12 ) end
	
	if ( type.panRadius ) then emitter:panRadius( type.panRadius )
	else emitter:panRadius( 0 ) end
	
	if( type.relativeToEmitter ) then blueprint:relativeToEmitter( 1 )
	else blueprint:relativeToEmitter( 0 ) end

	-- MB: for platform specific particle scaling
	--		spawnExtra lets the system spawn extra particles
	--		defaults to 1.0 (spawn as many extra as we want)
	--		set to 0.0 to not spawn any extra
	if( type.spawnExtra ) then emitter:spawnExtra(type.spawnExtra)
	else emitter:spawnExtra(1) end
	
	if( type.billboardDirection ) then emitter:bbDir( type.billboardDirection )
	else emitter:bbDir( { 0, 0, 0 } ) end
	
	if( type.spawnOnGround ) then emitter:spawnOnGround( 1 )
	else emitter:spawnOnGround( 0 ) end
	
	if( type.heightAboveGround ) then emitter:heightAboveGround( type.heightAboveGround )
	else emitter:heightAboveGround( 0.0 ) end
	
	if( type.scaleInTime ) then blueprint:scaleInTime( type.scaleInTime )
	else blueprint:scaleInTime( 0.0 ) end
	
	if( type.ribbonPostDof ) then emitter:isRibbonPostDof( 1 )
	else emitter:isRibbonPostDof( 0 ) end
	
end
 
function LoadEmitterTypes( particleBarn )
	-- Since we are gonna reallocate, we can change the mult safely
	particleBarn:m_partAllocMult( particleAllocMult )

	-- First fill in the names so they can be referred to in the code
	particleBarn:ClearEmitterTypes()
	for typeNum,typeName in ipairs( emitterMap ) do
		particleBarn:AddEmitterType( typeNum, typeName )
		--print("Adding type #", typeNum, typeName )
	end
	
	local numEmitters = particleBarn:GetNumEmitters()
	--print( "numEmitters: ", numEmitters )
	for i = 0, numEmitters - 1 do
		local emitter = particleBarn:GetEmitter( i )
		LoadEmitterType( particleBarn, emitter )		
	end
end

