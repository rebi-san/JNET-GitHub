-------------------------------------------------------------------------------
-- Notes:  Contains the master DebugKeys table, as well as a helper function
--         for Game to call every frame.
--        
--         Key can either be a string, number, or function. If it's a string,
--         Key() is used to parse. If a number, Pad() is used, and if a function,
--         that function is evaluated to determine if that action should be fired.
--
-- Author: Nick Clark
-- Date C: 8-25-2010
-------------------------------------------------------------------------------
local g_input

local function Key( c, ignoreCheat )
	return (ignoreCheat or Vars.Game.cheatsEnabled()) and g_input:IsKeyDown( c:byte() ) and not g_input:IsOldKeyDown( c:byte() )
end

local function Pad( p, ignoreCheat )
	return (ignoreCheat or Vars.Game.cheatsEnabled()) and g_input:GetPad( g_input:GetDebugPadIndex() ):WasButtonPressed( p )
end

local function PadDown( p, ignoreCheat )
	return (ignoreCheat or Vars.Game.cheatsEnabled()) and g_input:GetPad( g_input:GetDebugPadIndex() ):IsButtonDown( p )
end

local function PadUp( p, ignoreCheat )
	return (ignoreCheat or Vars.Game.cheatsEnabled()) and not g_input:GetPad( g_input:GetDebugPadIndex() ):IsButtonDown( p )
end

DebugKeys = 
{   { Key = '1'
	, Desc = "Load Graveyard"
	, Action = function( game ) game:QueueLevel( "Level_Graveyard", true, true ) end }
,	{ Key = '2'
	, Desc = "Load Barrens"
	, Action = function( game ) game:QueueLevel( "Level_Barrens", true, true ) end }
,	{ Key = '3'
	, Desc = "Load Desert"
	, Action = function( game ) game:QueueLevel( "Level_Desert", true, true ) end }
,	{ Key = '4'
	, Desc = "Load Canyon"
	, Action = function( game ) game:QueueLevel( "Level_Canyon", true, true ) end }
,	{ Key = '5'
	, Desc = "Load Cave"
	, Action = function( game ) game:QueueLevel( "Level_Cave", true, true ) end }
,	{ Key = '6'
	, Desc = "Load Ruins"
	, Action = function( game ) game:QueueLevel( "Level_Ruins", true, true ) end }
,	{ Key = '7'
	, Desc = "Load Mountain"
	, Action = function( game ) game:QueueLevel( "Level_Mountain", true, true ) end }
,	{ Key = '8'
	, Desc = "Load Summit"
	, Action = function( game ) game:QueueLevel( "Level_Summit", true, true ) end }
,	{ Key = '9'
	, Desc = "Load Credits"
	, Action = function( game ) game:QueueLevel( "Level_Credits", true, true ) end }
,	{ Key = '0'
	, Desc = "Load Matts's test level"
	, Action = function( game ) game:QueueLevel( "Level_Desert", true, true ) end }
,	{ Key = '\\'
	, Desc = "Load Bryan's test level"
	, Action = function( game ) game:QueueLevel( "Level_Bryan", true, true ) end }
,	{ Key = ' '
	, Desc = "Attempt resource hotload"
	, Action = function( game ) game:AttemptHotload() end }
,	{ Key = 'A'
	, Desc = "Print class sizes"
	, Action = function( game ) DumpClassSizes() end }
,	{ Key = 'B'
	, Desc = "Spawn strands for the player"
	, Action = function( game ) game:playerBarn():GetLocalDude():FillScarf( 99, game:soundBarn() ) end }
--,   Key = 'C' -- Used for cheat toggling
,	{ Key = 'D'
	, Desc = "Toggle debug line rendering"
	, Action = function( game ) game:debugLines():SetEnabled( not game:debugLines():GetEnabled() ) end }
,	{ Key = 'E'
	, Desc = "Toggle link mode enabled"
	, Action = function( game ) ForceGlowEnabled( game ) end }
,	{ Key = 'G'
	, Desc = "Toggle camera angle lock for WalkmanCam"
	, Action = function( game ) Vars.WalkmanCam.kEnableSixaxisTilt( not Vars.WalkmanCam.kEnableSixaxisTilt() ) end }
,	{ Key = 'H'	
	, Desc = "Cycle through debug HUD render modes"
	, Action = function( game ) game:debugHud():CycleMode() end }
,	{ Key = 'I'
	, Desc = "Toggle cam/dude pos"
	, Action = function( game ) ToggleCamDude() end }
,	{ Key = 'J'
	, Desc = "Toggle CPU Profiler display"
	, Action = function( game ) ToggleCPUProfilerDisplay() end }
,	{ Key = 'L'
	, Desc = "Force disconnect Garnet"
	, Action = function( game ) game:pweb3():ResetSocket() end }
,	{ Key = 'M'
	, Desc = "Print memory usage"
	, Action = function( game ) game:PrintMemUsage() end }
,	{ Key = 'N'
	, Desc = "Toggle net gui"
	, Action = function( game ) print( "NetGUI Activated" ) game:netGui():ToggleEnabled() end }
,	{ Key = 'O'
	, Desc = "Load Chris's test level"
	, Action = function( game ) game:QueueLevel( "Level_Chris", true, true ) end }
,	{ Key = 'P'
	, Desc = "Toggle pause mode"
	, Action = 
		function( game )
			game:m_forcePause( not game:m_forcePause() )
			if game:m_forcePause() then game:gameTiming():SetPauseSpdMult( 0.0001 ) else game:gameTiming():SetPauseSpdMult( 1.0 ) end
		end 
	}
,	{ Key = 'Q'
	, Desc = "Toggle DM mode"
	, Action = function( game ) ToggleDMActive( game ) end }
,	{ Key = 'R'
	, Desc = "Reload dune barn and reset Zephyrs"
	, Action = function( game ) game:duneBarn():OnLevelUnLoad() game:duneBarn():OnLevelLoad( game:resources() ) game:zephyrBarn():Reset() end }
,	{ Key = 'S'
	, Desc = "Print sounds playing and sound memory usage"
	, Action = function( game ) SoundDebugPrintMemory() game:soundBarn():DebugPrint() SoundPrintDebug() game:reverbBarn():DebugPrint() end }
,	{ Key = 'T'
	, Desc = "Print Metasystem"
	, Action = function( game ) DumpMetaSys() end }
,	{ Key = 'U'
	, Desc = "Validate Hulls"
	, Action = function( game ) HullValidate( game:physicalHullBarn(), game:pweb3() ) end }
,	{ Key = 'V'
	, Desc = "Save Vars"
	, Action = 
		function( game )
			print( "Saving out tweak vars." )
			LuaCallWindowsCmd( "/app_home/EXEC:cmd /c \"p4 edit Data/Scripts/Vars.lua\"" )
			SaveVars( "/app_home/Data/Scripts/Vars.lua" ) 
		end 
	}
,	{ Key = 'W'
	, Desc = "Toggle Debug Particle Composite"
	, Action = function( game ) game:renderContext():particleCompositeLod( 1 - game:renderContext():particleCompositeLod() ) end }
,	{ Key = 'X'
	, Desc = "Clear save game data"
	, Action = function( game ) game:saveData():Clear() game:playerBarn():GetLocalDude():SetOutfit( 0 ) SpawnEvent{ SaveGameTrigger = { } } end }
,	{ Key = 'Y'
	, Desc = "Mute the music for trailer people to capture sfx only"
	, Action = function( game ) game:soundBarn():ToggleMuteMusic() end }
,	{ Key = '-'
	, Desc = "Reduce max cape"
	, Action = function( game ) SpawnEvent{ SetMaxCloth = { changeAmount = -3 } } end }
,	{ Key = '='
	, Desc = "Increase max cape"
	, Action = function( game ) SpawnEvent{ SetMaxCloth = { changeAmount = 3 } } end }
}

DebugActions =
{	{ Key = function() return Vars.Game.allowCheatToggling() and ( Key( 'C', true ) or ( PadDown( Channel_Button_L3, true ) and PadDown( Channel_Button_R3, true ) and Pad( Channel_Button_Start, true ) ) ) end
	, Desc = "Toggle cheats enabled"
	, Action = function( game ) game:ToggleCheatsEnabled() end }
,	{ Key = function() return PadDown( Channel_Button_Circle ) end
	, Desc = "DoF mode"
	, Action =
		function( game )
			if game:cameraSystem():UsingDebugCam() then
				game:cameraSystem():GetDebugCam():UpdateDof( game:gameTiming():GetDT(), game:input(), game:playerBarn():GetLocalDude():GetPos() )
			end
		end
	}
,	{ Key = Channel_Button_Square
	, Desc = "Toggle debug camera"
	, Action =
		function( game )
			local cs = game:cameraSystem()
			local psnName = LuaGetPsnName()
			local isCool = psnName == "BeefyCakes" or psnName == "DrJessie"
			if not cs:UsingDebugCam() then
				cs:StartDebugCam()
				cs:GetDebugCam():SetAngleLock( false )

				if isCool then
					AddNamedHudElement( "JohnFace", "JohnFace", {-1.7,-0.875,1}, {0,0,1,1}, {0.3,0.2,0}, {1,1,1,1}, {9999999, 0.5, 0.5}, true )
					SpawnEvent{ DisplayText = { text = "Activating AngleLock'd DebugCam", x = -1.315, y = -0.65, duration = 3, fadeTime = 0.5 } }
					cs:GetDebugCam():SetAngleLock( true )
				end
			else
				cs:StopDebugCam()

				if isCool then RemoveHudElement( "JohnFace" ) end
			end
		end
	}
,	{ Key = function() return ( PadDown( Channel_Button_L3, true) and PadDown( Channel_Button_R1, true) ) end
	, Desc = "Save Debug Camera Position"
	, Action = 
		function( game )
			local cs = game:cameraSystem()
			if cs:UsingDebugCam() then
				cs:GetDebugCam():SaveCameraPosition()
			end
		end
	}
,	{ Key = function() return ( PadDown( Channel_Button_R3, true) and PadDown( Channel_Button_R1, true) ) end
	, Desc = "Restore Saved Debug Camera Position"
	, Action = 
		function( game )
			local cs = game:cameraSystem()
			if cs:UsingDebugCam() then
				cs:GetDebugCam():RestoreSavedCameraPosition()
			end
		end
	}
--,	{ Key = Channel_Button_Triangle
--	, Desc = "Execute selected NetGui item, or start debug cam with angle lock, or toggle debug cam angle lock"
--	, Action =
--		function( game )
--			game:netGui():ExecuteSelectedItem( game )
--			local cs = game:cameraSystem()
--			if not cs:UsingDebugCam() then
--				cs:StartDebugCam()
--				cs:GetDebugCam():SetAngleLock( true )
--			else
--				if cs:GetDebugCam():GetCinematicMode() then
--					cs:GetDebugCam():SetCinematicMode( false )
--				elseif cs:GetDebugCam():GetAngleLock() then
--					cs:GetDebugCam():SetCinematicMode( true )
--					cs:GetDebugCam():SetAngleLock( false )
--				else
--					cs:GetDebugCam():SetAngleLock( true )
--				end
--			end
--		end
--	}
,	{ Key = function() return PadDown( Channel_Button_L1 ) end
	, Desc = "Set Maya camera to match game camera"
	, Action = 
		function( game )
			local curCamKeyFrame = game:cameraSystem():GetBlendState()
			local pos = curCamKeyFrame:pos()
			local rot = curCamKeyFrame:rot()
			local fov = curCamKeyFrame:fov()
			local dofDist = curCamKeyFrame:dofDistance()
			local dofRad = curCamKeyFrame:dofFocusRadius()
			local dofTrans = curCamKeyFrame:dofTransitionMult()
			local dofInt = curCamKeyFrame:dofIntensity()
			local mbInt = curCamKeyFrame:motionBlurIntensity()

			local strpos = string.format("%.3f", pos[1])..","..string.format("%.3f", pos[2])..","..string.format("%.3f", pos[3])
			local strrot = string.format("%.3f", rot[1])..","..string.format("%.3f", rot[2])..","..string.format("%.3f", rot[3])..","..string.format("%.3f", rot[4])

			local gardenerCmd = "Mel( [[ SetCamTrans( {"..strpos.."}, {"..strrot.."} ); SetCamVars( "..fov..", "..dofDist..", "..dofRad..", "..dofTrans..", "..dofInt..", "..mbInt.." ); ]] )"

			for i = 0, game:pweb3():GetBoundDeviceCount() - 1 do
				local device = game:pweb3():GetBoundDevice( i )
				if device:GetType() == "gardener" then
					game:pweb3():Send( device:GetUniqueId(), gardenerCmd )
				end
			end
		end
	}
,	{ Key = function() return PadDown( Channel_Button_R2, true ) end
	, Desc = "Speed up the game"
	, Action = 
		function( game )
			if not game:cameraSystem():UsingDebugCam() then
				local throttle = g_input:GetPad( g_input:GetDebugPadIndex() ):Button( Channel_Button_R2 )
				if not game:playerBarn():IsMultiplayer() then
					game:gameTiming():SetDebugContThrottle( 1 + throttle * 4 )
				else
					game:gameTiming():SetDebugContThrottle( 1 )
				end
			end
		end
	}
,	{ Key = function() return PadDown( Channel_Button_L2, true ) end
	, Desc = "Slow down the game"
	, Action = 
		function( game )
			if not game:cameraSystem():UsingDebugCam() then
				local throttle = g_input:GetPad( g_input:GetDebugPadIndex() ):Button( Channel_Button_L2 )
				if not game:playerBarn():IsMultiplayer() then
					local eps = 0.000001;
					local newSpeed = 1 - throttle;
					if newSpeed < eps then
						newSpeed = eps
					end
					game:gameTiming():SetDebugContThrottle( newSpeed )
				else
					game:gameTiming():SetDebugContThrottle( 1 )
				end
			end
		end
	}
,	{ Key = function() return PadUp( Channel_Button_R2 ) and PadUp( Channel_Button_L2 ) end
	, Desc = "Return the game to normal speed"
	, Action = 
		function( game )
			game:gameTiming():SetDebugContThrottle(1)
		end
	}
,	{ Key = Channel_Button_Up
	, Desc = "Select previous NetGui item"
	, Action = 
		function( game ) 
			if game:netGui():IsEnabled() then
				game:netGui():SelectPreviousItem()
			else
				--game:m_forcePause( not game:m_forcePause() )
				--if game:m_forcePause() then game:gameTiming():SetPauseSpdMult( 0.0001 ) else game:gameTiming():SetPauseSpdMult( 1.0 ) end 
			end
		end 
	}
,	{ Key = Channel_Button_Down
	, Desc = "Select next NetGui item"
	, Action = function( game ) game:netGui():SelectNextItem() end }
,	{ Key = function() return Pad(Channel_Button_Select, true) end
	, Desc = "Start pause mode"
	, Action = 
		function( game ) 
			game:pauseSystem():Toggle( game ) 
		end
	}
,	{ Key = Channel_Button_Left
	, Desc = "Activate previous pause camera"
	, Action = function( game ) game:pauseSystem():DebugPrevKeyFrame(game:cameraSystem()) end }
,	{ Key = Channel_Button_Right
	, Desc = "Activate next pause camera"
	, Action = function( game ) game:pauseSystem():DebugNextKeyFrame(game:cameraSystem()) end }
,	{ Key = function() return PadDown( Channel_Button_L3, false ) end
	, Desc = "Activate wind edit mode"
	, Action = 
		function( game ) 
			if game:cameraSystem():UsingDebugCam() then		
				game:cameraSystem():GetDebugCam():Suppress()
				game:zephyrBarn():Suppress()
				
				local camForward = game:cameraSystem():GetCameraNode():GetForward()
				local camRight = game:cameraSystem():GetCameraNode():GetRight()
				
				local rightStick = game:input():GetPad( g_input:GetDebugPadIndex() ):Right()
				local windForward = vec3_scalar_mult( vec3_normalize( { camForward[1], 0, camForward[3] } ), rightStick[2] )
				local windRight = vec3_scalar_mult( vec3_normalize( { camRight[1], 0, camRight[3] } ), -rightStick[1] )
				
				local wind = vec3_scalar_mult( vec3_add( windForward, windRight ), 50 )
				game:surfaceWindBarn():SetVelocity( wind )
			end
		end 
	}
}

function ProcessDebugKeys( game, input )
	g_input = input

	if input:IsAnyKeyActive() then
		for i,v in ipairs( DebugKeys ) do
			assert( type( v.Key ) == "string" )
			if Key( v.Key ) then
				v.Action( game )
			end
		end
	end

	for i,v in ipairs( DebugActions ) do
		if ( type( v.Key ) == "string" and Key( v.Key ) ) or
		   ( type( v.Key ) == "number" and Pad( v.Key ) ) or
		   ( type( v.Key ) == "function" and v.Key() ) then
			v.Action( game )
		end
	end

	if Vars.Game.cheatsEnabled() then
		CheckTriggerShortcuts( input )
	end
end
