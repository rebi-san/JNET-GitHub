-------------------------------------------------------------------------------
-- Notes:  Tweak data for sound effects placed in code
--
-- Author: Martin Middleton
-- Date C: 02-03-2010
-------------------------------------------------------------------------------

soundEffectData =
{
	NoSound = { bankName = "LvlSfx", soundName = "", minDist = 0.0, autoPitchBend = false }
,	AddStrand = { bankName = "SharSfx", soundName = "CapeClothAdded", minDist = 5.0, maxDist = 50, autoPitchBend = true }
,	RemoveStrand = { bankName = "SharSfx", soundName = "CapeClothLost", minDist = 5.0, maxDist = 50, autoPitchBend = true }
,	AddStrandFull = { bankName = "SharSfx", soundName = "CapeClothFull", minDist = 5.0, maxDist = 50, autoPitchBend = true }
,	AddStrandFail = { bankName = "SharSfx", soundName = "CapeClothRejected", minDist = 5.0, maxDist = 50, autoPitchBend = true }
,	Cloth = { bankName = "SharSfx", soundName = "Cloth", minDist = 10.0, autoPitchBend = true }
,	ClothSing = { bankName = "LvlSfx", soundName = "ClothSing", minDist = 1.0, maxDist = 15, autoPitchBend = true }
,	Combo = { bankName = "SharSfx", soundName = "ClothEject", minDist = 10.0, autoPitchBend = true }
,	Shout = { bankName = "SharSfx", soundName = "Shout", minDist = 5.0, maxDist = 100.0, autoPitchBend = true }
,	ClothGuardianShout = { bankName = "LvlSfx", soundName = "ClothGuardianShout", minDist = 15.0, maxDist = 200.0, autoPitchBend = true }
,	Sing = { bankName = "LvlSfx", soundName = "Sing", minDist = 10.0, autoPitchBend = false }
,	SingBurst = { bankName = "LvlSfx", soundName = "SingBurst", minDist = 15.0, autoPitchBend = false }
,	SingTap = { bankName = "LvlSfx", soundName = "SingTap", minDist = 5.0, autoPitchBend = false }
,	Sing1 = { bankName = "LvlSfx", soundName = "Sing-Area1-Tier1", minDist = 10.0, autoPitchBend = false }
,	PlayerChirp = { bankName = "LvlSfx", soundName = "PlayerChirp", minDist = 10.0, autoPitchBend = false }
,	PlayerCoo = { bankName = "LvlSfx", soundName = "PlayerCoo", minDist = 9.0, autoPitchBend = false }
,	PlayerShout = { bankName = "LvlSfx", soundName = "PlayerShout", minDist = 16.0, autoPitchBend = false }
,	PlayerCall = { bankName = "LvlSfx", soundName = "PlayerCall", minDist = 15.0, autoPitchBend = false }
,	Player2Chirp = { bankName = "LvlSfx", soundName = "Player2Chirp", minDist = 10.0, autoPitchBend = false }
,	Player2Coo = { bankName = "LvlSfx", soundName = "Player2Coo", minDist = 9.0, autoPitchBend = false }
,	Player2Shout = { bankName = "LvlSfx", soundName = "Player2Shout", minDist = 16.0, autoPitchBend = false }
,	Player2Call = { bankName = "LvlSfx", soundName = "Player2Call", minDist = 15.0, autoPitchBend = false }
,	PlayerFailedGlow = { bankName = "SharSfx", soundName = "PlayerFailedGlow", minDist = 1.0, maxDist = 9.0, autoPitchBend = false }
,	SingCenterSP = { bankName = "LvlSfx", soundName = "Sing-Center-SP", minDist = 10.0, autoPitchBend = true }
,	SingCenterMP = { bankName = "LvlSfx", soundName = "Sing-Center-MP1", minDist = 10.0, autoPitchBend = true }
,	SingCenterMP2 = { bankName = "LvlSfx", soundName = "Sing-Center-MP2", minDist = 10.0, autoPitchBend = true }
,	SingUpSP = { bankName = "LvlSfx", soundName = "Sing-Up-SP", minDist = 10.0, autoPitchBend = true }
,	SingUpMP = { bankName = "LvlSfx", soundName = "Sing-Up-MP1", minDist = 10.0, autoPitchBend = true }
,	SingUpMP2 = { bankName = "LvlSfx", soundName = "Sing-Up-MP2", minDist = 10.0, autoPitchBend = true }
,	SingDownSP = { bankName = "LvlSfx", soundName = "Sing-Down-SP", minDist = 10.0, autoPitchBend = true }
,	SingDownMP = { bankName = "LvlSfx", soundName = "Sing-Down-MP1", minDist = 10.0, autoPitchBend = true }
,	SingDownMP2 = { bankName = "LvlSfx", soundName = "Sing-Down-MP2", minDist = 10.0, autoPitchBend = true }
,	SingLeftSP = { bankName = "LvlSfx", soundName = "Sing-Left-SP", minDist = 10.0, autoPitchBend = true }
,	SingLeftMP = { bankName = "LvlSfx", soundName = "Sing-Left-MP1", minDist = 10.0, autoPitchBend = true }
,	SingLeftMP2 = { bankName = "LvlSfx", soundName = "Sing-Left-MP2", minDist = 10.0, autoPitchBend = true }
,	SingRightSP = { bankName = "LvlSfx", soundName = "Sing-Right-SP", minDist = 10.0, autoPitchBend = true }
,	SingRightMP = { bankName = "LvlSfx", soundName = "Sing-Right-MP1", minDist = 10.0, autoPitchBend = true }
,	SingRightMP2 = { bankName = "LvlSfx", soundName = "Sing-Right-MP2", minDist = 10.0, autoPitchBend = true }
,	SingUpRightSP = { bankName = "LvlSfx", soundName = "Sing-UpRight-SP", minDist = 10.0, autoPitchBend = true }
,	SingUpRightMP = { bankName = "LvlSfx", soundName = "Sing-UpRight-MP1", minDist = 10.0, autoPitchBend = true }
,	SingUpRightMP2 = { bankName = "LvlSfx", soundName = "Sing-UpRight-MP2", minDist = 10.0, autoPitchBend = true }
,	SingDownLeftSP = { bankName = "LvlSfx", soundName = "Sing-DownLeft-SP", minDist = 10.0, autoPitchBend = true }
,	SingDownLeftMP = { bankName = "LvlSfx", soundName = "Sing-DownLeft-MP1", minDist = 10.0, autoPitchBend = true }
,	SingDownLeftMP2 = { bankName = "LvlSfx", soundName = "Sing-DownLeft-MP2", minDist = 10.0, autoPitchBend = true }
,	SingUpLeftSP = { bankName = "LvlSfx", soundName = "Sing-UpLeft-SP", minDist = 10.0, autoPitchBend = true }
,	SingUpLeftMP = { bankName = "LvlSfx", soundName = "Sing-UpLeft-MP1", minDist = 10.0, autoPitchBend = true }
,	SingUpLeftMP2 = { bankName = "LvlSfx", soundName = "Sing-UpLeft-MP2", minDist = 10.0, autoPitchBend = true }
,	SingDownRightSP = { bankName = "LvlSfx", soundName = "Sing-DownRight-SP", minDist = 10.0, autoPitchBend = true }
,	SingDownRightMP = { bankName = "LvlSfx", soundName = "Sing-DownRight-MP1", minDist = 10.0, autoPitchBend = true }
,	SingDownRightMP2 = { bankName = "LvlSfx", soundName = "Sing-DownRight-MP2", minDist = 10.0, autoPitchBend = true }
,	SandStorm = { bankName = "LvlSfx", soundName = "SandStorm_L", minDist = 30.0, autoPitchBend = true }
,	SandStormMusic = { bankName = "LvlMus", soundName = "M_Desert_SandStorm_L", minDist = 50.0, autoPitchBend = true }
,	SnowStorm = { bankName = "LvlSfx", soundName = "A_SnowStormWind_L", minDist = 30.0, autoPitchBend = true }
,	Meteor = { bankName = "LvlSfx", soundName = "AncestorEmitter_L", minDist = 20.0, autoPitchBend = true }
,	Falling = { bankName = "SharSfx", soundName = "Falling_L", minDist = 5.0, autoPitchBend = false }
,	Surfing = { bankName = "SharSfx", soundName = "SandSurf_L", minDist = 10.0, maxDist = 50, autoPitchBend = false }
,	SurfingS = { bankName = "SharSfx", soundName = "StoneSurf_L", minDist = 10.0, maxDist = 50, autoPitchBend = false }
,	SurfingL = { bankName = "SharSfx", soundName = "SandSurf-LeftSweetener_L", minDist = 10.0, maxDist = 50, autoPitchBend = false }
,	SurfingR = { bankName = "SharSfx", soundName = "SandSurf-RightSweetener_L", minDist = 10.0, maxDist = 50, autoPitchBend = false }
,	ClothSlide = { bankName = "SharSfx", soundName = "BridgeGlide_L", minDist = 5.0, maxDist = 50, autoPitchBend = true }
,   VortexHover = { bankName = "SharSfx", soundName = "VortexHover_L", minDist = 5.0, maxDist = 50, autoPitchBend = true }
,	CapeEnd = { bankName = "SharSfx", soundName = "Cape-End_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,	CapeMid = { bankName = "SharSfx", soundName = "Cape-Middle_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,	CapeEndRemote = { bankName = "SharSfx", soundName = "Cape-End-Remote_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,	CapeMidRemote = { bankName = "SharSfx", soundName = "Cape-Middle-Remote_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,	Cloak = { bankName = "SharSfx", soundName = "Cloak_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,   Dust = { bankName = "SharSfx", soundName = "DustSpray_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,   DustRemote = { bankName = "SharSfx", soundName = "DustSpray-Remote_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,   Dirty = { bankName = "SharSfx", soundName = "DirtyFeetSpray_L", minDist = 1,  maxDist = 10.0, autoPitchBend = true }
,   DirtyRemote = { bankName = "SharSfx", soundName = "DirtyFeetSpray-Remote_L", minDist = 1,  maxDist = 10.0, autoPitchBend = true }
,	Glow = { bankName = "SharSfx", soundName = "PlayerSparkle_L", minDist = 5.0,  maxDist = 10.0, autoPitchBend = true }
,	LinkLocal = { bankName = "SharSfx", soundName = "LocalPlayerSymbolPartnerGlow_L", minDist = 2.0, maxDist = 15.0, autoPitchBend = true }
,	LinkRemote = { bankName = "SharSfx", soundName = "RemotePlayerSymbolPartnerGlow_L", minDist = 2.0, maxDist = 15.0, autoPitchBend = true }
,	RollingDuneMedium = { bankName = "SharSfx", soundName = "RollingDuneMedium_L", minDist = 3.0,  maxDist = 15.0, autoPitchBend = true, dopplerMult = 0.4 }
,	RollingDuneTall = { bankName = "SharSfx", soundName = "RollingDuneTall_L", minDist = 3.0,  maxDist = 15.0, autoPitchBend = true, dopplerMult = 0.4 }
,	DuneHitRocksLg = { bankName = "SharSfx", soundName = "RollingDuneHitRocksLg", minDist = 2.0, maxDist = 50.0, autoPitchBend = true, dopplerMult = 0.4 }
,	DuneHitRocksSm = { bankName = "SharSfx", soundName = "RollingDuneHitRocksSm", minDist = 1.0, maxDist = 30.0, autoPitchBend = true, dopplerMult = 0.4 }
,	SnowTumble = { bankName = "LvlSfx", soundName = "SnowTumble_L", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,	Tombstone = { bankName = "SharSfx", soundName = "TombstoneActivated_L", minDist = 1.5, maxDist = 8.0, autoPitchBend = true }
,	BreadcrumbSymbol = { bankName = "SharSfx", soundName = "BreadcrumbSymbol", minDist = 3, maxDist = 50.0, autoPitchBend = true }
,	WaterfallCol = { bankName = "SharSfx", soundName = "PlayerCrashIntoWaterfallLg", minDist = 3.0, autoPitchBend = true }
,	SummitWaterfallCol = { bankName = "LvlSfx", soundName = "SummitPlayerHitWaterfall", minDist = 3.0, autoPitchBend = true }
,	GuardianIntoSand = { bankName = "LvlSfx", soundName = "GuardianIntoSand", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,	GuardianOutOfSand = { bankName = "LvlSfx", soundName = "GuardianOutOfSand", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,	GuardianInSand_L = { bankName = "LvlSfx", soundName = "GuardianInSand_L", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,	Guardian_VisorDown = { bankName = "LvlSfx", soundName = "Guardian-VisorDown", minDist = 15.0, maxDist = 35.0, autoPitchBend = true }
,	Guardian_VisorUp = { bankName = "LvlSfx", soundName = "Guardian-VisorUp", minDist = 15.0, maxDist = 35.0, autoPitchBend = true }
,	Guardian_LockOn = { bankName = "LvlSfx", soundName = "Guardian-LockOn", minDist = 15.0, maxDist = 50.0, autoPitchBend = true }
,	Guardian_LeapUp = { bankName = "LvlSfx", soundName = "Guardian-LeapUp", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	Guardian_IntoSnowEnter = { bankName = "LvlSfx", soundName = "Guardian-IntoSnowEnter", minDist = 15.0, maxDist = 50.0, autoPitchBend = true }
,	Guardian_DiveBomb = { bankName = "LvlSfx", soundName = "Guardian-DiveBomb", minDist = 50.0, maxDist = 70.0, autoPitchBend = true }
,	Guardian_OutOfSnowExit = { bankName = "LvlSfx", soundName = "Guardian-OutOfSnowExit", minDist = 15.0, maxDist = 50.0, autoPitchBend = true }
,	Guardian_OutOfSnowVO = { bankName = "LvlSfx", soundName = "Guardian-OutOfSnowVO", minDist = 15.0, maxDist = 50.0, autoPitchBend = true }
,   Guardian_TearPlayerCape = { bankName = "LvlSfx", soundName = "Guardian-TearPlayerCape", minDist = 15.0, maxDist = 50.0, autoPitchBend = true } 
,	DistantGuardian_LockOn = { bankName = "LvlSfx", soundName = "DistantGuardian-LockOn", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	DistantGuardian_LeapUp = { bankName = "LvlSfx", soundName = "DistantGuardian-LeapUp", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	DistantGuardian_IntoSnowEnter = { bankName = "LvlSfx", soundName = "DistantGuardian-IntoSnowEnter", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	DistantGuardian_DiveBomb = { bankName = "LvlSfx", soundName = "DistantGuardian-DiveBomb", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	DistantGuardian_OutOfSnowExit = { bankName = "LvlSfx", soundName = "DistantGuardian-OutOfSnowExit", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	DistantGuardian_OutOfSnowVO = { bankName = "LvlSfx", soundName = "DistantGuardian-OutOfSnowVO", minDist = 40.0, maxDist = 90.0, autoPitchBend = true }
,	GuardianHitPlayer = { bankName = "LvlSfx", soundName = "GuardianHitPlayer", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,	PlayerHitByGuardian = { bankName = "LvlSfx", soundName = "PlayerHitByGuardian", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,   ClothPlatformBloom = { bankName = "LvlSfx", soundName = "ClothPlatformBloom", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,   CollisionSmash = { bankName = "SharSfx", soundName = "CollisionSmash", minDist = 3.0, maxDist = 50.0, autoPitchBend = true }
,   FishAwake = { bankName = "LvlSfx", soundName = "FishAwake", minDist = 2.0, maxDist = 12.0, autoPitchBend = true } 
,   FishInSand_L = { bankName = "SharSfx", soundName = "FishInSand_L", minDist = 3.0, maxDist = 30.0, autoPitchBend = true }
,   FishMovement_L = { bankName = "SharSfx", soundName = "FishMovement_L", minDist = 3.0, maxDist = 25.0, autoPitchBend = true }
,   FishCry = { bankName = "LvlSfx", soundName = "StandardFish-FrozenCry", minDist = 2.0, maxDist = 12.0, autoPitchBend = true }
,   FrozenFishLandInSnow = { bankName = "LvlSfx", soundName = "FrozenFishLandInSnow", minDist = 3.0, maxDist = 50.0, autoPitchBend = true }
,	FishFreedCelebrate    = { bankName = "LvlSfx", soundName = "fish-freed-celebrate"     , minDist = 10.0, autoPitchBend = false }
,	FishIdleMoveShort     = { bankName = "LvlSfx", soundName = "fish-idle-move-short"     , minDist = 10.0, autoPitchBend = false }
,	FishIdleMoveMedium    = { bankName = "LvlSfx", soundName = "fish-idle-move-medium"    , minDist = 10.0, autoPitchBend = false }
,	FishIdleMoveLong      = { bankName = "LvlSfx", soundName = "fish-idle-move-long"      , minDist = 10.0, autoPitchBend = false }
,	FishRespondToPlayer   = { bankName = "LvlSfx", soundName = "fish-respond-to-palayer"  , minDist = 10.0, autoPitchBend = false }
,	FishFoundSomething    = { bankName = "LvlSfx", soundName = "fish-found-something"     , minDist = 10.0, autoPitchBend = false }
,	FishComeHere          = { bankName = "LvlSfx", soundName = "fish-come-here"           , minDist = 10.0, autoPitchBend = false }
,	FishThereYouGo        = { bankName = "LvlSfx", soundName = "fish-there-you-go"        , minDist = 10.0, autoPitchBend = false }
,	FishTrapped           = { bankName = "LvlSfx", soundName = "fish-trapped"             , minDist = 10.0, autoPitchBend = false }
,	FishWantsToLiftPlayer = { bankName = "LvlSfx", soundName = "fish-wants-to-lift-player", minDist = 10.0, autoPitchBend = false }
,	FishLaugh             = { bankName = "LvlSfx", soundName = "fish-laugh"               , minDist = 10.0, autoPitchBend = false }
-- ,	FishMutter            = { bankName = "LvlSfx", soundName = "fish-mutter"              , minDist = 10.0, autoPitchBend = false }
-- ,	FishQuestion          = { bankName = "LvlSfx", soundName = "fish-question"            , minDist = 10.0, autoPitchBend = false }
-- ,	FishUhOh              = { bankName = "LvlSfx", soundName = "fish-uh-oh"               , minDist = 10.0, autoPitchBend = false }
,   FlowSnakeCall         = { bankName = "LvlSfx", soundName = "FlowSnakeCall"           , minDist = 10.0, autoPitchBend = false }
,   FlowHunterCall        = { bankName = "LvlSfx", soundName = "FlowHunterCall"           , minDist = 10.0, autoPitchBend = false }
,   CapeGrow_L = { bankName = "SharSfx", soundName = "CapeGrow_L", minDist = 10.0, maxDist = 20.0, autoPitchBend = true }
,   SecretIdle_L = { bankName = "SharSfx", soundName = "CollectionCapeIdle_L", minDist = 1.0, maxDist = 5.0, autoPitchBend = true }
,   StrandFlap = { bankName = "SharSfx", soundName = "FlyingStrandFlap", minDist = 1.0, maxDist = 12.0, autoPitchBend = false }
,   BrickMovement = { bankName = "SharSfx", soundName = "BrickMovement", minDist = 1.0, maxDist = 12.0, autoPitchBend = true }
,   WhistleStone = { bankName = "LvlSfx", soundName = "WhistleStone_L", minDist = 1.0, maxDist = 8, autoPitchBend = true }
,   WindBattleFirst = { bankName = "LvlSfx", soundName = "A_Windswept_L", minDist = 1.0, maxDist = 3.5, autoPitchBend = false }
,   WindBattleStrong = { bankName = "LvlSfx", soundName = "A_StrongWind_L", minDist = 1.0, maxDist = 3.5, autoPitchBend = false }
,   WindBattleFinal = { bankName = "LvlSfx", soundName = "A_SnowStormWind_L", minDist = 1.0, maxDist = 3.5, autoPitchBend = false }
,   WindBattleDudePush = { bankName = "LvlSfx", soundName = "SnowTumble_L", minDist = 2.0, maxDist = 12.0, autoPitchBend = false }
,   PlayerShakeClean_Sm = { bankName = "SharSfx", soundName = "PlayerShakeClean_Sm", minDist = 1.0, maxDist = 3.5, autoPitchBend = true }
,   PlayerShakeClean_Lg = { bankName = "SharSfx", soundName = "PlayerShakeClean_Lg", minDist = 1.0, maxDist = 3.5, autoPitchBend = true }
,   PlayerFreezing_Sm_L = { bankName = "LvlSfx", soundName = "PlayerFreezing_Sm_L", minDist = 1.0, maxDist = 12, autoPitchBend = true }
,   PlayerFreezing_Lg_L = { bankName = "LvlSfx", soundName = "PlayerFreezing_Lg_L", minDist = 1.0, maxDist = 12, autoPitchBend = true }
,   GuardianExplode = { bankName = "LvlSfx", soundName = "GuardianDestroyed", minDist = 60.0, maxDist = 200, autoPitchBend = true }
,   GuardianSearchlight = { bankName = "LvlSfx", soundName = "Searchlight_L", minDist = 3.0, maxDist = 15, autoPitchBend = true }
,   JellySpinVocal = { bankName = "LvlSfx", soundName = "JellySpinVocal", minDist = 2.0, maxDist = 200, autoPitchBend = true }
,   HappyWhale = { bankName = "LvlSfx", soundName = "HappyWhale", minDist = 2.0, maxDist = 100, autoPitchBend = true }
,   WhooshBySmall = { bankName = "LvlSfx", soundName = "WhooshBy-Small", minDist = 1.0, maxDist = 10, autoPitchBend = true }
,   WhooshByMedium = { bankName = "LvlSfx", soundName = "WhooshBy-Medium", minDist = 1.0, maxDist = 10, autoPitchBend = true }
,   WhooshByLarge = { bankName = "LvlSfx", soundName = "WhooshBy-Large", minDist = 1.0, maxDist = 10, autoPitchBend = true }
,   Flower = { bankName = "LvlSfx", soundName = "FlowerRespond", minDist = 1.0, maxDist = 10, autoPitchBend = true }
,   OpeningUberShout = { bankName = "LvlSfx", soundName = "OpeningUberShout", minDist = 1.0, maxDist = 10, autoPitchBend = true }
}
