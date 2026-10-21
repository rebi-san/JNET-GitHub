-- Additive base meshes, the pose mesh needs to be built before the additive meshes
resource "MeshAnimation" "AdditivePose" { source = "C_ClothNinja_A_AdditivePose.PSSG", meshName = "BlendNinja" }
resource "MeshAnimation" "AdditivePose2" { source = "C_ClothNinja_A_AdditivePose2.PSSG", meshName = "BlendNinja" }

wad "MeshAnimWad" "Climb"
{ wadresource "MeshAnimation" "ClimbUpL" { source = "C_ClothNinja_A_ClimbUpL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpR" { source = "C_ClothNinja_A_ClimbUpR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpL1" { source = "C_ClothNinja_A_ClimbUpL1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpR1" { source = "C_ClothNinja_A_ClimbUpR1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpStepL" { source = "C_ClothNinja_A_ClimbUpStepL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpStepR" { source = "C_ClothNinja_A_ClimbUpStepR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpStepL1" { source = "C_ClothNinja_A_ClimbUpStepL1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpStepR1" { source = "C_ClothNinja_A_ClimbUpStepR1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpRunLR" { source = "C_ClothNinja_A_ClimbUpRunLR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpRunLR1" { source = "C_ClothNinja_A_ClimbUpRunLR1.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Jump"
{ wadresource "MeshAnimation" "JumpFL" { source = "C_ClothNinja_A_JumpFL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "JumpFR" { source = "C_ClothNinja_A_JumpFR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "JumpHalfL" { source = "C_ClothNinja_A_JumpHalfL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "JumpHalfR" { source = "C_ClothNinja_A_JumpHalfR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "JumpRoll" { source = "C_ClothNinja_A_MPJumpRollF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HopL" { source = "C_ClothNinja_A_HopL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "NodHead" { source = "C_ClothNinja_A_Nod.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ShakeHead" { source = "C_ClothNinja_A_ShakeHead.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LiftA1" { source = "C_ClothNinja_A_LiftA1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ShakeOff" { source = "C_ClothNinja_A_ShakeOff.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Slide0"
{ wadresource "MeshAnimation" "SlideJump" { source = "C_ClothNinja_A_SlideJump.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideJump1" { source = "C_ClothNinja_A_SlideJump1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideLand" { source = "C_ClothNinja_A_SlideLand.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideShake" { source = "C_ClothNinja_A_SlideShake.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustL" { source = "C_ClothNinja_A_SlideAdjustL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustR" { source = "C_ClothNinja_A_SlideAdjustR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustF" { source = "C_ClothNinja_A_SlideAdjustF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustB" { source = "C_ClothNinja_A_SlideAdjustB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustL1" { source = "C_ClothNinja_A_SlideAdjustHardL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustR1" { source = "C_ClothNinja_A_SlideAdjustHardR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustF1" { source = "C_ClothNinja_A_SlideAdjustHardF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideAdjustB1" { source = "C_ClothNinja_A_SlideAdjustHardB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideGroundSpinL" { source = "C_ClothNinja_A_SlideGroundSpinL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideGroundSpinR" { source = "C_ClothNinja_A_SlideGroundSpinR.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Slide1"
{ wadresource "MeshAnimation" "Slide" { source = "C_ClothNinja_A_Slide.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideB" { source = "C_ClothNinja_A_SlideB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideF" { source = "C_ClothNinja_A_SlideF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideL" { source = "C_ClothNinja_A_SlideL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideR" { source = "C_ClothNinja_A_SlideR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXJump" { source = "C_ClothNinja_A_SlideXJump.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXJump1" { source = "C_ClothNinja_A_SlideXJump1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXLand" { source = "C_ClothNinja_A_SlideXLand.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXShake" { source = "C_ClothNinja_A_SlideXShake.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustL" { source = "C_ClothNinja_A_SlideXAdjustL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustR" { source = "C_ClothNinja_A_SlideXAdjustR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustF" { source = "C_ClothNinja_A_SlideXAdjustF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustB" { source = "C_ClothNinja_A_SlideXAdjustB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustL1" { source = "C_ClothNinja_A_SlideXAdjustHardL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustR1" { source = "C_ClothNinja_A_SlideXAdjustHardR.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Slide2"
{ wadresource "MeshAnimation" "SlideXAdjustF1" { source = "C_ClothNinja_A_SlideXAdjustHardF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXAdjustB1" { source = "C_ClothNinja_A_SlideXAdjustHardB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideX" { source = "C_ClothNinja_A_SlideX.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXB" { source = "C_ClothNinja_A_SlideXB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXF" { source = "C_ClothNinja_A_SlideXF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXL" { source = "C_ClothNinja_A_SlideXL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXR" { source = "C_ClothNinja_A_SlideXR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideJump2" { source = "C_ClothNinja_A_SlideJump2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXJump2" { source = "C_ClothNinja_A_SlideXJump2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideJump3" { source = "C_ClothNinja_A_SlideJump3.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SlideXJump3" { source = "C_ClothNinja_A_SlideXJump3.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude0"
{ wadresource "MeshAnimation" "Crouch" { source = "C_ClothNinja_A_Crouch.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "CrouchWeak" { source = "C_ClothNinja_A_CrouchWeak.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "CrouchWeak1" { source = "C_ClothNinja_A_CrouchWeak1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Crouching" { source = "C_ClothNinja_A_Crouching.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "CrouchingWeak" { source = "C_ClothNinja_A_CrouchingWeak.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "CrouchToIdle" { source = "C_ClothNinja_A_CrouchToIdle.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "CrouchToIdleW" { source = "C_ClothNinja_A_CrouchToIdleWeak.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Dive" { source = "C_ClothNinja_A_Dive.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "DiveB" { source = "C_ClothNinja_A_DiveB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "AirFlipR" { source = "C_ClothNinja_A_AirFlipR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "AirFlipL" { source = "C_ClothNinja_A_AirFlipL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FallForward" { source = "C_ClothNinja_A_FallForward.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RollF" { source = "C_ClothNinja_A_RollF.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude1"
{ wadresource "MeshAnimation" "RollB" { source = "C_ClothNinja_A_RollB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RollL" { source = "C_ClothNinja_A_RollL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RollR" { source = "C_ClothNinja_A_RollR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LieF" { source = "C_ClothNinja_A_LieF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LieFSlide" { source = "C_ClothNinja_A_LieFSlide.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LieB" { source = "C_ClothNinja_A_LieB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "GetUpF" { source = "C_ClothNinja_A_GetUpF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "GetUpB" { source = "C_ClothNinja_A_GetUpB.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Glide" { source = "C_ClothNinja_A_Glide.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "GlideL" { source = "C_ClothNinja_A_GlideL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "GlideR" { source = "C_ClothNinja_A_GlideR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "GlideD" { source = "C_ClothNinja_A_GlideD.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "GlideFD" { source = "C_ClothNinja_A_JumpFLand.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude2"
{ wadresource "MeshAnimation" "LegGlide" { source = "C_ClothNinja_A_LegGlide.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LegGlideF" { source = "C_ClothNinja_A_LegGlideF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LegGlideL" { source = "C_ClothNinja_A_LegGlideL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LegGlideR" { source = "C_ClothNinja_A_LegGlideR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LegGlideU" { source = "C_ClothNinja_A_LegGlideU.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Idle" { source = "C_ClothNinja_A_Idle.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "IdleWeak" { source = "C_ClothNinja_A_IdleWeak.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "IdleStretch" { source = "C_ClothNinja_A_IdleStretch.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "IdleCheckFootL" { source = "C_ClothNinja_A_IdleCheckFootL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "IdleCheckFootR" { source = "C_ClothNinja_A_IdleCheckFootR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LookBackL" { source = "C_ClothNinja_A_LookBackL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LookBackR" { source = "C_ClothNinja_A_LookBackR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Jog" { source = "C_ClothNinja_A_Jog.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "JogWeak" { source = "C_ClothNinja_A_JogWeak.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude3"
{ wadresource "MeshAnimation" "LandSoft" { source = "C_ClothNinja_A_LandSoft.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LiftA" { source = "C_ClothNinja_A_LiftA.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LiftEL" { source = "C_ClothNinja_A_LiftEL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LiftER" { source = "C_ClothNinja_A_LiftER.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LiftF" { source = "C_ClothNinja_A_LiftF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Med" { source = "C_ClothNinja_A_Med.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Act01Hold" { source = "C_ClothNinja_A_Act01SurpriseHold.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ReadyToMed" { source = "C_ClothNinja_A_ReadyToMed.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Run" { source = "C_ClothNinja_A_Run.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Run1" { source = "C_ClothNinja_A_Run1.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude4"
{ wadresource "MeshAnimation" "RunMJump" { source = "C_ClothNinja_A_RunMJump.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RunWeak" { source = "C_ClothNinja_A_RunWeak.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RunAltWeak" { source = "C_ClothNinja_A_RunWeak1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Flap1" { source = "C_ClothNinja_A_Flap1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Flap3" { source = "C_ClothNinja_A_Flap3.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HitBodyL" { source = "C_ClothNinja_A_HitBodyL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HitBodyR" { source = "C_ClothNinja_A_HitBodyR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HitBodyM" { source = "C_ClothNinja_A_HitBodyM.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HitBodyLAir" { source = "C_ClothNinja_A_HitBodyLAir.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HitBodyRAir" { source = "C_ClothNinja_A_HitBodyRAir.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "HitBodyMAir" { source = "C_ClothNinja_A_HitBodyMAir.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ShoutL" { source = "C_ClothNinja_A_ShoutL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ShoutR" { source = "C_ClothNinja_A_ShoutR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ShoutM" { source = "C_ClothNinja_A_ShoutM.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude5"
{ wadresource "MeshAnimation" "ShoutBL" { source = "C_ClothNinja_A_ShoutBL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ShoutBR" { source = "C_ClothNinja_A_ShoutBR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Charge" { source = "C_ClothNinja_A_Charge.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ChargeAir" { source = "C_ClothNinja_A_ChargeAir.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ChargeWalk" { source = "C_ClothNinja_A_ChargeWalk.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ChargeRun" { source = "C_ClothNinja_A_ChargeRun.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ChargeWeak" { source = "C_ClothNinja_A_ChargeWeak.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "ChargeCold" { source = "C_ClothNinja_A_ChargeCold.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "Boost" { source = "C_ClothNinja_A_Boost.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LeapL" { source = "C_ClothNinja_A_LeapL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LeapR" { source = "C_ClothNinja_A_LeapR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "BlownBack" { source = "C_ClothNinja_A_BlownBack.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "LandFast" { source = "C_ClothNinja_A_LandFast.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude6"
{ wadresource "MeshAnimation" "StopL" { source = "C_ClothNinja_A_StopL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StopR" { source = "C_ClothNinja_A_StopR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StopBoostL" { source = "C_ClothNinja_A_StopBoostL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StopBoostR" { source = "C_ClothNinja_A_StopBoostR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StopMed" { source = "C_ClothNinja_A_StopMed.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StopTurnRunL" { source = "C_ClothNinja_A_StopTurnRunL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StopTurnRunR" { source = "C_ClothNinja_A_StopTurnRunR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TrippingL" { source = "C_ClothNinja_A_TrippingL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TrippingR" { source = "C_ClothNinja_A_TrippingR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TrippingL1" { source = "C_ClothNinja_A_TrippingL1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TrippingR1" { source = "C_ClothNinja_A_TrippingR1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TurnBodyLeft" { source = "C_ClothNinja_A_TurnBodyLeft.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TurnBodyRight" { source = "C_ClothNinja_A_TurnBodyRight.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TurnLookBackL" { source = "C_ClothNinja_A_TurnLookBackL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "TurnLookBackR" { source = "C_ClothNinja_A_TurnLookBackR.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude7"
{ wadresource "MeshAnimation" "Walk" { source = "C_ClothNinja_A_Walk.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkUp" { source = "C_ClothNinja_A_WalkUp.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkDown" { source = "C_ClothNinja_A_WalkDown.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkWeak" { source = "C_ClothNinja_A_WalkWeak.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkHurtL" { source = "C_ClothNinja_A_WalkHurtL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkHurtL1" { source = "C_ClothNinja_A_WalkHurtL1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPAirSwing1L" { source = "C_ClothNinja_A_MPAirSwing1L.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPAirSwing1R" { source = "C_ClothNinja_A_MPAirSwing1R.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPAirSwing2L" { source = "C_ClothNinja_A_MPAirSwing2L.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPAirSwing2R" { source = "C_ClothNinja_A_MPAirSwing2R.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPRunFlip1L" { source = "C_ClothNinja_A_MPGroundRunFlip1L.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPRunFlip1R" { source = "C_ClothNinja_A_MPGroundRunFlip1R.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkStopL" { source = "C_ClothNinja_A_WalkStopL.PSSG", meshName = "BlendNinja"}
, wadresource "MeshAnimation" "WalkStopR" { source = "C_ClothNinja_A_WalkStopR.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "DudeFixCloth"
{ wadresource "MeshAnimation" "FixCloth" { source = "C_ClothNinja_A_FixCloth.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FixCloth2" { source = "C_ClothNinja_A_FixCloth2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FixCloth2X" { source = "C_ClothNinja_A_FixCloth2X.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FixCloth3" { source = "C_ClothNinja_A_FixCloth3.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FixCloth4" { source = "C_ClothNinja_A_FixCloth4.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FixCloth4X" { source = "C_ClothNinja_A_FixCloth4X.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "FixClothAir" { source = "C_ClothNinja_A_FixClothAir.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Dude8"
{ wadresource "MeshAnimation" "MPFSpinL" { source = "C_ClothNinja_A_MPFSpinL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "MPFSpinR" { source = "C_ClothNinja_A_MPFSpinR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SwimF" { source = "C_ClothNinja_A_SwimF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SwimL" { source = "C_ClothNinja_A_SwimL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SwimR" { source = "C_ClothNinja_A_SwimR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "BoostSpiralL" { source = "C_ClothNinja_A_SwimBoost.PSSG", meshName = "BlendNinja" } --SwimBoost
, wadresource "MeshAnimation" "BoostSpiralR" { source = "C_ClothNinja_A_SwimBoost2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "BoostFlap" { source = "C_ClothNinja_A_BoostFlap.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "BoostFlap2" { source = "C_ClothNinja_A_BoostFlap2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SwimTurnL" { source = "C_ClothNinja_A_SwimTurnL.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SwimTurnR" { source = "C_ClothNinja_A_SwimTurnR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkHurtR" { source = "C_ClothNinja_A_WalkHurtR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkHurtR1" { source = "C_ClothNinja_A_WalkHurtR1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "StandTurnL" { source = "C_ClothNinja_A_StandTurnL.PSSG", meshName = "BlendNinja"}
, wadresource "MeshAnimation" "StandTurnR" { source = "C_ClothNinja_A_StandTurnR.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Mountain"
{ wadresource "MeshAnimation" "WalkSnow" { source = "C_ClothNinja_A_WalkSnow.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "JogSnow" { source = "C_ClothNinja_A_JogSnow.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RunSnow" { source = "C_ClothNinja_A_RunSnow.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RunSnow1" { source = "C_ClothNinja_A_RunSnowAlt.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "WalkTired" { source = "C_ClothNinja_A_WalkTired.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpWalkSnow" { source = "C_ClothNinja_A_ClimbUpWalkSnow.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpRunSnow" { source = "C_ClothNinja_A_ClimbUpRunSnow.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpRunSnow1" { source = "C_ClothNinja_A_ClimbUpRunSnow1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "PushBack" { source = "C_ClothNinja_A_PushBack.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpWeakL1" { source = "C_ClothNinja_A_ClimbUpWeakL1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpWeakR1" { source = "C_ClothNinja_A_ClimbUpWeakR1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpStepWL" { source = "C_ClothNinja_A_ClimbUpStepWeakL1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpStepWR" { source = "C_ClothNinja_A_ClimbUpStepWeakR1.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClimbUpWeakHL" { source = "C_ClothNinja_A_ClimbUpWeakHighL.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Summit" 
{ wadresource "MeshAnimation" "Collapse" { source = "C_ClothNinja_A_Collapse.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Collapsing" { source = "C_ClothNinja_A_Collapsing.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Collapse2" { source = "C_ClothNinja_A_Collapse2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Collapsing2" { source = "C_ClothNinja_A_Collapsing2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Revive" { source = "C_ClothNinja_A_Revive.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Reviving" { source = "C_ClothNinja_A_Reviving.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Revive2" { source = "C_ClothNinja_A_Revive2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Reviving2" { source = "C_ClothNinja_A_Reviving2.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "PrepareToLaunch" { source = "C_ClothNinja_A_PrepareToLaunch.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ReadyToLaunch" { source = "C_ClothNinja_A_ReadyToLaunch.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "BoostFlap3" { source = "C_ClothNinja_A_BoostFlap3.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "SpinF" { source = "C_ClothNinja_A_SpinF.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "RigTest" { source = "C_ClothNinja_A_RigTest.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Kneel" { source = "C_ClothNinja_A_Kneel.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Kneeling" { source = "C_ClothNinja_A_Kneeling.PSSG", meshName = "BlendNinja" }
}


--Dude Cinematics
wad "MeshAnimWad" "Cinematics"
{ wadresource "MeshAnimation" "Act01Surprise" { source = "C_ClothNinja_A_Act01Surprise.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Act01LookDown" { source = "C_ClothNinja_A_Act01LookDown.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Act01LookAround" { source = "C_ClothNinja_A_Act01LookAround.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "ClothBridgeIdle" { source = "P_ClothBridge_A_Idle.PSSG", meshName = "P_ClothBridge" }
, wadresource "MeshAnimation" "ClothBridgeLink" { source = "P_ClothBridge_A_Link.PSSG", meshName = "P_ClothBridge" }
, wadresource "MeshAnimation" "Act06LookAround" { source = "C_ClothNinja_A_Act06LookAround.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Act06LookLR" { source = "C_ClothNinja_A_Act06LookLR.PSSG", meshName = "BlendNinja" }
, wadresource "MeshAnimation" "Walk2" { source = "C_ClothNinja_A_Walk2.PSSG", meshName = "BlendNinja" }
--, wadresource "MeshAnimation" "LookBackUpL" { source = "C_ClothNinja_A_LookBackUpL.PSSG", meshName = "BlendNinja" }
--, wadresource "MeshAnimation" "LookingBackUpL" { source = "C_ClothNinja_A_LookingBackUpL.PSSG", meshName = "BlendNinja" }
}

wad "MeshAnimWad" "Spirit"
{ wadresource "MeshAnimation" "SpiritExamine" { source = "C_SpiritMesh_A_Act01Examine.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritMeshIdle" { source = "C_SpiritMesh_A_Idle.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritMeshMoveL" { source = "C_SpiritMesh_A_MoveToSideL.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritGroupL1" { source = "C_SpiritMesh_A_GroupMoveL1.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritGroupL2" { source = "C_SpiritMesh_A_GroupMoveL2.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritGroupR1" { source = "C_SpiritMesh_A_GroupMoveR1.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritGroupR2" { source = "C_SpiritMesh_A_GroupMoveR2.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritGroupM1" { source = "C_SpiritMesh_A_GroupMoveM1.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritGroupM2" { source = "C_SpiritMesh_A_GroupMoveM2.PSSG", meshName = "C_SpiritMesh" }

}

wad "MeshAnimWad" "Spirit01"
{ wadresource "MeshAnimation" "SpiritComeOut" { source = "C_SpiritMesh_A_Act01ComeOut.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritExit" { source = "C_SpiritMesh_A_Act01Exit.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritExitWait" { source = "C_SpiritMesh_A_Act01ExitWait.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritIdleDown" { source = "C_SpiritMesh_A_Act01IdelDown.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritIdleUp" { source = "C_SpiritMesh_A_SpiritIdleUp.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "LookRight" { source = "C_SpiritMesh_A_LookRight.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "LookRightDown" { source = "C_SpiritMesh_A_LookRightDown.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "LookRightUp" { source = "C_SpiritMesh_A_LookRightUp.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "01IdleToExit" { source = "C_SpiritMesh_A_Act01IdleToExit.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritRetreat" { source = "C_SpiritMesh_A_Act01Retreat.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "SpiritShout" { source = "C_SpiritMesh_A_Act01Shout.PSSG", meshName = "C_SpiritMesh" }
}

wad "MeshAnimWad" "Spirit03"
{ wadresource "MeshAnimation" "03Hesitation" { source = "C_SpiritMesh_A_Act03Hesitation.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "03Whisper" { source = "C_SpiritMesh_A_Act03Whisper.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "03Whispering" { source = "C_SpiritMesh_A_Act03Whispering.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "03WhisperToIdle" { source = "C_SpiritMesh_A_Act03WhisperToIdle.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "03IdleSad" { source = "C_SpiritMesh_A_Act05IdleSad.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "LookLeftDown" { source = "C_SpiritMesh_A_LookLeftDown.PSSG", meshName = "C_SpiritMesh" }

}

wad "MeshAnimWad" "Spirit06"
{ wadresource "MeshAnimation" "06FloatToIdleA" { source = "C_SpiritMesh_A_Act06BigFloatToIdleA.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleA" { source = "C_SpiritMesh_A_Act06BigIdleA.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleATurnToB" { source = "C_SpiritMesh_A_Act06BigIdleATurnToB.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleB" { source = "C_SpiritMesh_A_Act06BigIdleB.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleBLookToC" { source = "C_SpiritMesh_A_Act06BigIdleBLookUpToC.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleBRetreat" { source = "C_SpiritMesh_A_Act06BigIdleBRetreatExit.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleC" { source = "C_SpiritMesh_A_Act06BigIdleC.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleCShoutToD" { source = "C_SpiritMesh_A_Act06BigIdleCShoutToD.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleD" { source = "C_SpiritMesh_A_Act06BigIdleD.PSSG", meshName = "C_SpiritMesh" }
, wadresource "MeshAnimation" "06IdleDTurnToB" { source = "C_SpiritMesh_A_Act06BigIdleDTurnToB.PSSG", meshName = "C_SpiritMesh" }
}

wad "MeshAnimWad" "Misc0"
{ wadresource "MeshAnimation" "JellyFishIdle" { source = "P_JellyFish_A_Idle.PSSG", meshName = "P_JellyFish" }
, wadresource "MeshAnimation" "JellyFishShock" { source = "P_JellyFish_A_Shock.PSSG", meshName = "P_JellyFish" }
, wadresource "MeshAnimation" "JellyFishTwirl" { source = "P_JellyFish_A_Dash.PSSG", meshName = "P_JellyFish" }
, wadresource "MeshAnimation" "LJellyFishIdle" { source = "P_JellyFishLowRes_A_Idle.PSSG", meshName = "P_JellyFishLowRes" }
, wadresource "MeshAnimation" "LJellyFishShock" { source = "P_JellyFishLowRes_A_Shock.PSSG", meshName = "P_JellyFishLowRes" }
, wadresource "MeshAnimation" "LJellyFishTwirl" { source = "P_JellyFishLowRes_A_Dash.PSSG", meshName = "P_JellyFishLowRes" }
--, wadresource "MeshAnimation" "LJellyFishSwim" { source = "P_JellyFish_A_SwimUp.PSSG", meshName = "P_JellyFishLowRes" }
, wadresource "MeshAnimation" "MJellyFishIdle" { source = "P_MeshJellyfish_A_Idle.PSSG", meshName = "P_MeshJellyfish" }
, wadresource "MeshAnimation" "MJellyFishTwirl" { source = "P_MeshJellyfish_A_Dash.PSSG", meshName = "P_MeshJellyfish" }
, wadresource "MeshAnimation" "LanternIdle" { source = "P_ClothLantern_A_Idle.PSSG", meshName = "P_ClothLantern" }
, wadresource "MeshAnimation" "LanternSpin" { source = "P_ClothLantern_A_Spin.PSSG", meshName = "P_ClothLantern" }
, wadresource "MeshAnimation" "FishGrateStrug" { source = "P_DesertFinalTowerFishGrate_A_Struggle.PSSG", meshName = "P_DesertFinalTowerFishGrate" }
, wadresource "MeshAnimation" "FishGrateOpen" { source = "P_DesertFinalTowerFishGrate_A_Open.PSSG", meshName = "P_DesertFinalTowerFishGrate" }
}

wad "MeshAnimWad" "Fish"
{ wadresource "MeshAnimation" "FishDash" { source = "P_FishSmall_A_Dash.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "FishSwim" { source = "P_FishSmall_A_Swim.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "FishSwimBig" { source = "P_FishSmall_A_BigSwim.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "FishSpinFall" { source = "P_FishSmall_A_SpinFall.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "StuckIdle" { source = "P_FishSmall_A_StuckIdle.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "StuckWiggle" { source = "P_FishSmall_A_StuckWiggle.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "StuckStruggle" { source = "P_FishSmall_A_StuckStruggle.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "EscapeFail" { source = "P_FishSmall_A_EscapeFail.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "Escape" { source = "P_FishSmall_A_Escape.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "BindPose" { source = "P_FishSmall_A_BindPose.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "FishFlip" { source = "P_FishSmall_A_Flip.PSSG", meshName = "P_FishSmall" }
, wadresource "MeshAnimation" "LFishDash" { source = "P_FishSmallLowRes_A_Dash.PSSG", meshName = "P_FishSmallLowRes" }
, wadresource "MeshAnimation" "LFishSwim" { source = "P_FishSmallLowRes_A_Swim.PSSG", meshName = "P_FishSmallLowRes" }

}

wad "MeshAnimWad" "FlatFish"
{ wadresource "MeshAnimation" "FlatFishSwim" { source = "P_FishFlat_A_Swim.PSSG", meshName = "P_FishFlat_A_Test" }
, wadresource "MeshAnimation" "FlatFishFlip" { source = "P_FishFlat_A_Flip.PSSG", meshName = "P_FishFlat_A_Test" }
}

wad "MeshAnimWad" "Guardian"
{ wadresource "MeshAnimation" "SleepToSnap" { source = "P_Guardian_A_SleepToSnap.PSSG", meshName = "P_Guardian" }
, wadresource "MeshAnimation" "SwishTail" { source = "P_Guardian_A_SwishTail.PSSG", meshName = "P_Guardian" }
, wadresource "MeshAnimation" "Rear" { source = "P_Guardian_A_Rear.PSSG", meshName = "P_Guardian" }
, wadresource "MeshAnimation" "Awaken" { source = "P_Guardian_A_Awaken.PSSG", meshName = "P_Guardian" }
, wadresource "MeshAnimation" "Activation" { source = "P_Guardian_A_Activation.PSSG", meshName = "P_Guardian" }
-- These animations will need to be re-exported, but aren't used right now
--, wadresource "MeshAnimation" "WingRFlap" { source = "P_GuardianWingR_A_Flap.PSSG", meshName = "P_GuardianWingR" }
--, wadresource "MeshAnimation" "WingLFlap" { source = "P_GuardianWingL_A_Flap.PSSG", meshName = "P_GuardianWingL" }
}

wad "MeshAnimWad" "ClothGuardian"
{ wadresource "MeshAnimation" "NeckSpin" { source = "P_ClothGuardianNeck_A_Spin.PSSG", meshName = "P_ClothGuardianNeck" }
, wadresource "MeshAnimation" "NeckBankL" { source = "P_ClothGuardianNeck_A_BankL.PSSG", meshName = "P_ClothGuardianNeck" }
, wadresource "MeshAnimation" "HeadSpin" { source = "P_ClothGuardianHead_A_Spin.PSSG", meshName = "P_ClothGuardianHead" }
, wadresource "MeshAnimation" "HeadBankL" { source = "P_ClothGuardianHead_A_BankL.PSSG", meshName = "P_ClothGuardianHead" }
, wadresource "MeshAnimation" "WingSpin" { source = "P_ClothGuardianWing_A_Spin.PSSG", meshName = "P_ClothGuardianWing" }
, wadresource "MeshAnimation" "WingBankL" { source = "P_ClothGuardianWing_A_BankL.PSSG", meshName = "P_ClothGuardianWing" }
, wadresource "MeshAnimation" "WingLongSpin" { source = "P_ClothGuardianWingLong_A_Spin.PSSG", meshName = "P_ClothGuardianWingLong" }
, wadresource "MeshAnimation" "WingLongBankL" { source = "P_ClothGuardianWingLong_A_BankL.PSSG", meshName = "P_ClothGuardianWingLong" }
, wadresource "MeshAnimation" "TailSpin" { source = "P_ClothGuardianTail_A_Spin.PSSG", meshName = "P_ClothGuardianTail" }
, wadresource "MeshAnimation" "TailBankL" { source = "P_ClothGuardianTail_A_BankL.PSSG", meshName = "P_ClothGuardianTail" }
}

wad "MeshAnimWad" "Additive0"
{ wadresource "MeshAnimation" "AddBreath" { source = "C_ClothNinja_A_AddBreath.PSSG", meshName = "BlendNinja", isAdditive = true, baseMesh = "C_ClothNinja_A_AdditivePose.PSSG" }
, wadresource "MeshAnimation" "AddHeadL" { source = "C_ClothNinja_A_AddHeadL.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddHeadUp" { source = "C_ClothNinja_A_AddHeadUp.PSSG", meshName = "BlendNinja", isAdditive = true }
--, wadresource "MeshAnimation" "AddMultiplayer" { source = "C_ClothNinja_A_AddCapeOpen.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddNoise" { source = "C_ClothNinja_A_AddNoise.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddNoiseHip" { source = "C_ClothNinja_A_AddNoiseHip.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddWindB" { source = "C_ClothNinja_A_AddCapeBlowB.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddWindF" { source = "C_ClothNinja_A_AddCapeBlowF.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddWindL" { source = "C_ClothNinja_A_AddCapeBlowL.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddWindR" { source = "C_ClothNinja_A_AddCapeBlowR.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddHipTurnL" { source = "C_ClothNinja_A_AddHipTurnL.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddHipTurnR" { source = "C_ClothNinja_A_AddHipTurnR.PSSG", meshName = "BlendNinja", isAdditive = true }
}

wad "MeshAnimWad" "Additive1"
{ wadresource "MeshAnimation" "LookUp" { source = "C_ClothNinja_A_LookUp.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddBend" { source = "C_ClothNinja_A_AddBend.PSSG", meshName = "BlendNinja", isAdditive = true, baseMesh = "C_ClothNinja_A_AdditivePose.PSSG" }
, wadresource "MeshAnimation" "AddBendL" { source = "C_ClothNinja_A_AddBendL.PSSG", meshName = "BlendNinja", isAdditive = true, baseMesh = "C_ClothNinja_A_AdditivePose.PSSG" }
, wadresource "MeshAnimation" "AddBendR" { source = "C_ClothNinja_A_AddBendR.PSSG", meshName = "BlendNinja", isAdditive = true, baseMesh = "C_ClothNinja_A_AdditivePose.PSSG" }
, wadresource "MeshAnimation" "AddFaceShake" { source = "C_ClothNinja_A_AddFaceShake.PSSG", meshName = "BlendNinja", isAdditive = true, baseMesh = "C_ClothNinja_A_AdditivePose.PSSG" }
, wadresource "MeshAnimation" "Cough" { source = "C_ClothNinja_A_AddCough.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddFlap" { source = "C_ClothNinja_A_AddFlap.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddFlap2" { source = "C_ClothNinja_A_AddFlap2.PSSG", meshName = "BlendNinja", isAdditive = true }
, wadresource "MeshAnimation" "AddFlap3" { source = "C_ClothNinja_A_AddFlap3.PSSG", meshName = "BlendNinja", isAdditive = true }
}

--resource "MeshAnimation" "CutScene001" { source = "C_ClothNinja_A_CutScene001.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "GlideWalk" { source = "C_ClothNinja_A_GlideWalk.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "GlideWalkM" { source = "C_ClothNinja_A_GlideWalkM.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "IdleBuried" { source = "C_ClothNinja_A_IdleBuried.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "ShoutBuriedL" { source = "C_ClothNinja_A_ShoutBuriedL.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "ShoutBuriedR" { source = "C_ClothNinja_A_ShoutBuriedR.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "RunM" { source = "C_ClothNinja_A_RunM.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "RunML" { source = "C_ClothNinja_A_RunML.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "RunMR" { source = "C_ClothNinja_A_RunMR.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "StopTurnRun" { source = "C_ClothNinja_A_StopTurnRun.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "TurnHeadLeft" { source = "C_ClothNinja_A_TurnHeadLeft.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "TurnHeadRight" { source = "C_ClothNinja_A_TurnHeadRight.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "TurnHeadUp" { source = "C_ClothNinja_A_TurnHeadUp.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "TurnHeadDown" { source = "C_ClothNinja_A_TurnHeadDown.PSSG", meshName = "BlendNinja" }
--resource "MeshAnimation" "AdditiveBend" { source = "C_ClothNinja_A_AdditiveBend.PSSG", meshName = "BlendNinja" }
