-------------------------------------------------------------------------------
-- Notes:  Contains all script functions for rendering the menu.
--         Look at Render.lua for a more complete description of the purpose of
--         this file.
--
-- Author: Jenova Chen and John Edwards
-- Date C: 11-9-2007
-------------------------------------------------------------------------------

--constants
local kButtonIds =
{ Channel_Button_Square
, Channel_Button_Cross
, Channel_Button_Circle
, Channel_Button_Triangle
, Channel_Button_L1
, Channel_Button_R1
, Channel_Button_L2
, Channel_Button_R2
, Channel_Button_Select
, Channel_Button_Start
, Channel_Button_L3
, Channel_Button_R3
--, Channel_Button_Up
--, Channel_Button_Right
--, Channel_Button_Down
--, Channel_Button_Left
}

--pass variables
local TOTAL_SCENE = 8
local pass_Background
local imageTex_Background
local dt
local cheatsEnabled

--camera variables
local aspectRatio = 1
local sensor = {0,0,0}
local actualTrigger = 0
local obeyTilt = 1
local autoZoom = 0
local attractMult = 0
local focus = 0
local zoomTheta = 0
local zoomThetaOld = zoomTheta

--GUI variables
local alphas =
{ GUI = 0
, GUIT = 1
, helpObj = 0
, helpObjT = 1
, blackObjT = 0
, intObjT = 0
, anyBTObj = 0
, anyBTObjT = 0
, resumeObj = 0
, resumeObjT = 0
, glowObjT = 1
, waveObjT = 1
}
local glowIdx = 0
local waveObjScale = 0
local waveObjScaleT = 0

local minZoom = 1
local minZoomT = 1
local maxZoom = 3
local maxZoomT = 3
local vol = 1.777
local timer = 0
local resumeTimer = 0
local fadeTime = 1.5
local checkTime = 0.75
local resumeTime = 0.5
local safeWaitTime = 0.5

local lb1, lb2

--we have to group some of these params in tables because we were running out of upvalues.
local rewardState =
{ bloomTimer = 0
, playSound = true
, unlockedTrophy = false
, allFullyBloomed = false
, timer = 0
, state = "bloom"
, bloomTime = 2.0
, zoomTime = 2.2
, trophyTime = 1.0	-- MB: Arbitrary small wait time - trophy toast now detected and waited for
, delayTime = 0.8
, glowTime = 1.5
, doneTime = 2.0
}

local pauseState =
{ timer = 0
, delay = 0.333
, startReleased = false
}

local idleState =
{ enabled = false
, triggerTimer = 0
, fadeGuiTriggerTime = 7
, zoomOutTriggerTime = 4.5
, realSensor = { 0, 0, 0 }
, prevSensor = { 0, 0, 0 }
, controllerIdle = false
, controllerIdleTime = 0
, camLerp = 0
, camLerpOld = 0
}

local S3DState =
{	enabled = false,
	available = false,
	enableObj = nil,
	disableObj = nil,
	crossFade = 1.0,
	fade = 0.0
}

local infoState =
{ enabled = false --"JP" == REGION -- don't enable on PS4
, instructionsBeginObj = nil
, instructionsContObj = nil
, instructionsTicker = 0
, objs = {}
, curObj = 0
, alpha = 0
}

local demo =
{ endObj = nil
, endAlphaT = 0
, blackObj = nil
, blackAlphaT = 0
, loadObj = nil
, loadAlphaT = 0
, promptObj = nil
, promptObjT = 0
, movieObj = nil
, runUpsell = true
, forceFinish = false
, movieTimer = 0
, movieBlankTime = 0.75 -- A bug in the movie player forces us to let it play blanked out for a little bit.
, movieFadeTime = 2
, movieAllowSkip = false
, waitForNpConnection = true
, waitToDoNpConnectionFrames = 10
, launchInGameCommerce = true
, loadUiTimer = 0
, loadUiTime = 0.3
, productId = ""
, jumpToStore = false

}

local snappedPot = -1
local camPos = {0,0,0}
local levelsBeaten = 0
local currentLevel = 0
local currentSound = -1

local roomTrans =
{ fade = 0.0
, cur = 0
, nex = 0
}

local isLoading = false
local hasReward = false
local fromPause = false
local isSnaped = false
local allowResume = false
local noSnapYet = true

local potDepth = 3.55

local roomObjs = {}
local flowerObjs = {}
local animObjs = {}
local petalEmitters = {}

local levelNameMap =
{ [0] = "Level_1.lua"
, [1] = "Level_2.lua"
, [2] = "Level_3.lua"
, [3] = "Level_4.lua"
, [4] = "Level_5.lua"
, [5] = "Level_6.lua"
, [6] = "Level_7.lua"
}

function MenuRenderInitialize( ren, resources )
	--background shader initialization
	pass_Background = ren:AddQuadPass( resources, "MenuObject" )
	pass_Background:SetBlend( false, PE_SHADER_BLEND_ZERO, PE_SHADER_BLEND_DST_COLOR )
	pass_Background:SetPos( 0.0, 0.0, 0.0, 0.0 )
	pass_Background:SetUv( 0.0, 0.0, 1.0, 1.0 )
	imageTex_Background = ren:AddImageTexture( resources, "Black" )

	demo.productId = NP_GetStoreProductLabel()
end

function MenuRenderInitializeContext( ren, renderContext )
end
function MenuRenderSelectField(ren,renderContext,field)

	local menu = renderContext:menu()
	local offset

	if field==0 then
		offset = -1.0
	else
		offset = 1.0
	end

	menu:UpdateSceneObjects(offset)

end


function MenuReset( menu, resources )
	levelsBeaten = -1
	alphas.glowObjT = 0
	autoZoom = 0
	camPos = {0,0,0}
	menu:levelsBeaten(0)
	menu:currentScene(0)
	for flowerIdx, flower in ipairs( flowerObjs ) do
		flower:SetBloom( resources, 0 )
		for petalIdx, petal in ipairs( flower.petals ) do
			petal.obj:Alpha( 0.0 )
			petal.obj:Pos( petal.path[ 1 ] )
			petal.fall = 0
		end
	end
end

-- check for the beginning trophy
-- also sets up for dream trophy
local function CheckBeginningTrophy()
	local curBloomed = 0
	rewardState.allFullyBloomed = true
	for flowerIdx, flowerInner in ipairs( flowerObjs ) do
		if flowerInner.bloom < 2 and #flowerInner.petals > 0 and
		   (flower ~= flowerInner or secretPetalsComplete < 3) then
			rewardState.allFullyBloomed = false
		elseif flowerInner.bloom >= 2 then
			curBloomed = curBloomed + 1
		end
	end
	if curBloomed ~= 0 then
		-- Beginning Trophy
		Trophy_QueueUnlockTrophy( 12 )
	end

--	-- MB: Hack - unlock a ton of trophies for testing
--	for tidx = 0, 12, 1 do
--		Trophy_QueueUnlockTrophy( tidx )
--	end

end


function MenuRenderOnLoadFromFile( menu, resources, saveData )
	levelsBeaten = math.min( menu:levelsBeaten(), #flowerObjs - 1 )

	local allFullyBloomed = true
	for i, flower in ipairs( flowerObjs ) do
		local bloomLevel = 0
		local secrets = saveData:GetLevelData( i - 1 ):GetNumSecretFlowersCollected()
		if i <= levelsBeaten then bloomLevel = 1 end
		if secrets >= 3 then bloomLevel = 2 end
		flower:SetBloom( resources, bloomLevel )

		if #flower.petals > secrets then
			allFullyBloomed = false
		end

		for petalIdx, petal in ipairs( flower.petals ) do
			if saveData:GetLevelData( i - 1 ):IsSecretFlowerCollected( petalIdx - 1 ) then
				petal.obj:Alpha( 1.0 )
				petal.obj:Pos( petal.path[ #petal.path ] )
				petal.fall = 1
			end
		end
	end

	local levelIdx = levelsBeaten
	if allFullyBloomed then levelIdx = TOTAL_SCENE - 1 end

	roomTrans.cur = levelIdx
	roomTrans.nex = levelIdx
	menu:SelectPotImmediate( levelIdx )
	MoveGlow( math.min( #flowerObjs, levelIdx + 1 ) )
end

function MenuRenderUpdate( ren, renderContext )
	---------------------
	--read basic data
	---------------------
	--update background
	pass_Background:GetShader():SetTextureParam( "texture", imageTex_Background:GetTexture() )

	--read dt and total time
	dt = renderContext:gameTiming():GetDT() * 1.7                 --!MP! hack!
	local time = renderContext:gameTiming():GetTotalTime()

	-- to adjust things that don't take dt into account
	-- eg: attractMult
	local fpsAdjust = Clip(dt * 30, 0, 1)

	--did a special event just happen?
	if renderContext:menu():m_isVictorious() or renderContext:menu():m_isFromPause() then
		if renderContext:menu():m_isVictorious() then
			renderContext:menu():m_isVictorious(false)
			hasReward = true
		end
		if renderContext:menu():m_isFromPause() then
			renderContext:menu():m_isFromPause(false)
			fromPause = true
		end

		renderContext:menu():SelectPotImmediate( currentLevel )
		camPos = CalcLookAtFlowerCamPos( currentLevel + 1 )
		autoZoom = 1
	end

	--read cheat mode
	cheatsEnabled = Vars.Game.cheatsEnabled()



	aspectRatio = renderContext:camNode():GetAspect()
	local letterBoxY = 1.0/aspectRatio
	local letterBoxYSub = letterBoxY-1/1.777

	if letterBoxYSub < 0 then
		letterBoxYSub = 0
	end
	local letterBoxHeight = 2.0 * letterBoxYSub
	lb1:Pos( {0, letterBoxY, 0} )
	lb1:Scale( {2, letterBoxHeight, 0.0} )
	lb2:Pos( {0, -letterBoxY, 0} )
	lb2:Scale( {2, letterBoxHeight, 0.0} )


	---------------------
	--read input data
	---------------------
	local pad = renderContext:input():GetPad( 0 )
	local menu = renderContext:menu()
	local menuVisible = not menu:m_isTitleCardsRunning()

	local effectiveParalysis = menu:m_paralysis()
	if infoState.enabled and not demo.forceFinish and (infoState.curObj > 0 or pad:IsButtonDown( Channel_Button_Triangle )) then effectiveParalysis = 1 end

	-- S3D toggle
	if S3DState.enabled and menuVisible then

		local speed
		local available = S3DState.available and (effectiveParalysis<0.1)

		if available then
			speed = dt
		else
			speed = -dt
		end
		S3DState.fade = Clip(S3DState.fade+speed,0,1)

		if menu:GetS3DEnabled() then
			speed = dt*0.5
		else
			speed = -dt*0.5
		end
		S3DState.crossFade = Clip(S3DState.crossFade+speed,0,1)


		S3DState.enableObj:Alpha((1 - S3DState.crossFade)*S3DState.fade)
		S3DState.disableObj:Alpha(S3DState.crossFade*S3DState.fade)
	
		if available then
			if pad:WasButtonPressed( Channel_Button_Triangle ) then
				menu:ToggleS3D()
			end
		end

		-- default to not available again
		S3DState.available = false
	end

	--detect if the controller has gone idle.
	idleState.prevSensor = idleState.realSensor
	if menuVisible then
		idleState.realSensor = LerpV3( 0.15, idleState.realSensor, pad:Sensor() )
	else
		idleState.realSensor = {0, 0, 0}
	end


    --!MP! this is where we dump the controller state

    if false then

        local tttx = idleState.realSensor[1];
        local ttty = idleState.realSensor[2];
        local tttz = idleState.realSensor[3];
        DebugPrint("realSensor: " .. tttx .. ":" .. ttty .. ":" .. tttz .. "\n");

    end


	idleState.controllerIdle =
		Length( VectorAdd( idleState.realSensor, ScalarMult( idleState.prevSensor, -1 ) ) ) < 0.001 and
		not pad:IsAnyButtonDown() and
		not demo.forceFinish
	if idleState.controllerIdle  then
		idleState.controllerIdleTime = Clip( idleState.controllerIdleTime + dt, 0, 8 )
	else
		idleState.controllerIdleTime = Clip( idleState.controllerIdleTime - 5.0 * dt, 0, 8 )
	end
	local nullSensorLerp = Clip( math.max( math.pow( idleState.camLerp, 1/3 ), (idleState.controllerIdleTime - 4) / 4 ), 0, 1 )
	nullSensorLerp = Lerp( effectiveParalysis, nullSensorLerp, 1 )

	--lerp the sensor with previous data to reduce noises from the motion sensor
	if not demo.forceFinish then
		sensor = LerpV3( nullSensorLerp, idleState.realSensor, { 0, 0, 0 } )
	end

	--by default the tilt sensor returns a value around -0.2 to 0.2.  We want to
	--add a dead zone and get it in the proper range.
	local function FilterInput( input, max, dead )
		if input < 0 then return Clip( (input + dead) / (max - dead), -1, 0 )
		else              return Clip( (input - dead) / (max - dead), 0, 1 )
		end
	end

	local inputX = FilterInput( 0.6 * sensor[1], 0.2, 0.025 )
	local inputY = FilterInput( 0.6 * sensor[3], 0.2, 0.0 )

	---------------------
	--read trigger
	---------------------
	--lerp trigger to get a delay effect in the input		
	if menuVisible then
		if not demo.forceFinish then
			if cheatsEnabled then
				actualTrigger = CalculateMaxButtonPressureMenu(pad,true)
			else
				actualTrigger = CalculateMaxButtonPressureMenu(pad,false)
			end
		end
	else
		actualTrigger = 0
	end
	if effectiveParalysis > 0.1 then
		actualTrigger = 0
	else
		actualTrigger = actualTrigger * (1 - 10 * renderContext:menu():m_paralysis())
	end

	---------------------
	--when it's not loading level
	---------------------
	if isLoading == false then
		---------------------
		--cheat
		---------------------
		--@HACK: We want to let QA cheat in the menu.
		local forceCheatsEnabled = false
		--hold down L1 and R1 and press Left and Right to cheat level access
		if cheatsEnabled or forceCheatsEnabled then
			if pad:WasButtonPressed( Channel_Button_Right ) then
				levelsBeaten = math.min(renderContext:menu():levelsBeaten()+1,TOTAL_SCENE-1)
			end
			if pad:WasButtonPressed( Channel_Button_Left ) then
				levelsBeaten = math.max(renderContext:menu():levelsBeaten()-1,0)
			end
			if pad:WasButtonPressed( Channel_Button_Left ) or pad:WasButtonPressed( Channel_Button_Right ) then
				alphas.glowObjT = 0
				timer=0
				renderContext:menu():levelsBeaten( levelsBeaten )
				renderContext:menu():SelectPot( levelsBeaten )
			end
			if pad:WasButtonPressed( Channel_Button_Up ) then
				MenuReset( renderContext:menu(), renderContext:resources() )
			end
			if pad:WasButtonPressed( Channel_Button_Down ) then
				renderContext:menu():m_isVictorious( true )
			end
		end

		-- We have fairly different logic in the different menu states.
		if hasReward then
			---------------------
			--when it's waiting for reward (showing new flower and scene)
			---------------------
			minZoom = 1
			minZoomT = 1
			alphas.anyBTObjT = 0
			alphas.blackObjT = 0
			allowResume = false

			rewardState.timer = rewardState.timer + dt

			if "bloom" == rewardState.state then
				isSnaped = true
				if rewardState.timer > rewardState.bloomTime then
					if rewardState.bloomTimer > 0 then
						rewardState.bloomTimer = rewardState.bloomTimer - dt
					else
						-- Flowers bloom upon completing the level for the first time, and then they bloom again
						-- when the player collects all the secret flowers.
						local flower = flowerObjs[ math.min( currentLevel + 1, #flowerObjs ) ]
						-- Figure out the status of the secret flower UI.
						local lvlData = renderContext:saveData():GetLevelData( currentLevel )
						local secretPetalsComplete = 0
						local activePetals = {}
						for petalIdx, petal in ipairs( flower.petals ) do
							if lvlData:IsSecretFlowerCollected( petalIdx - 1 ) then
								if petal.fall >= 1.0 then
									secretPetalsComplete = secretPetalsComplete + 1
								else
									table.insert( activePetals, petal )
									if petal.fall < 0.25 then break end
								end
							end
						end
						if flower.bloom < 1 or (flower.bloom < 2 and secretPetalsComplete >= 3) then
							if rewardState.playSound then
								if flower.bloom < 1 then
									if currentLevel < 6 then
										SoundPlay( "MenuSfx", "RegularBloom"..(currentLevel + 1), 1 )
									end
								else
									SoundPlay( "MenuSfx", "SecretBloom", 1 )
								end
								rewardState.playSound = false
							end
							local newBloom = flower.bloom + 0.025
							if math.floor( flower.bloom ) ~= math.floor( newBloom ) then
								newBloom = math.floor( newBloom )
								rewardState.bloomTimer = 0.75
								rewardState.playSound = true
							end
							flower:SetBloom( renderContext:resources(), newBloom )
							camPos = ScalarMult( flower:LookAt(), potDepth )
							camPos[3] = 0
						elseif #activePetals > 0 then
							for petalIdx, curPetal in ipairs( activePetals ) do
								if curPetal.obj:Alpha() < 1.0 then
									SoundPlay( "MenuSfx", "PetalDrop", 1 )
									curPetal.playLandSound = true
								end
								if curPetal.playLandSound and curPetal.fall > 0.5 then
									SoundPlay( "MenuSfx", "PetalLand", 1 )
									curPetal.playLandSound = false
								end
								curPetal.obj:Alpha( 1.0 )
								curPetal.fall = Clip( curPetal.fall + 0.005 * fpsAdjust, 0, 1 )

								--@HACK: Time to give in and just play back Mattie's curves.
								local pos = { curPetal.path[2][1], curPetal.path[2][2], curPetal.path[2][3] }
								local idxMin = math.floor( curPetal.fall * #petalAnimData_translateX ) + 1
								idxMin = math.min( idxMin, #petalAnimData_translateX - 1 )
								local idxMax = idxMin + 1
								local t = Clip( 1 + curPetal.fall * #petalAnimData_translateX - idxMin, 0, 1 )
								pos[1] = pos[1] + Lerp( t, petalAnimData_translateX[ idxMin ], petalAnimData_translateX[ idxMax ] )
								pos[2] = pos[2] + Lerp( t, petalAnimData_translateY[ idxMin ], petalAnimData_translateY[ idxMax ] )
								curPetal.obj:Pos( pos )
								curPetal.obj:Orient( Lerp( t, petalAnimData_rotateZ[ idxMin ], petalAnimData_rotateZ[ idxMax ] ) )
							end
						else

							CheckBeginningTrophy()

							--it takes a while to unlock a trophy, so we queue it up, here.
							rewardState.unlockedTrophy = Trophy_UnlockNextTrophyInQueue()
							rewardState.timer = 0
							rewardState.state = "zoomOut"
						end
					end
				end
			elseif "zoomOut" == rewardState.state then
				local t = 1 - 0.5 * (1 + math.cos( math.pi * rewardState.timer / rewardState.zoomTime ))
				t = Clip( t, 0, 1 )
				camPos = LerpV3( t, CalcLookAtFlowerCamPos( currentLevel + 1 ), { -0.025, CalcIdealY( { -0.025, 0, 0 } ), 0 } )
				autoZoom = Lerp( t, 1, 0 )
				if rewardState.timer > rewardState.zoomTime then
					if not rewardState.unlockedTrophy then
						rewardState.timer = rewardState.trophyTime + 1
					end
					rewardState.state = "trophy"
				end
			elseif "trophy" == rewardState.state then
				if rewardState.timer > rewardState.trophyTime then
					rewardState.timer = 0

					if not (Trophy_UnlockInProgress() or Trophy_ToastInProgress()) then
						if Trophy_UnlockNextTrophyInQueue() then
							rewardState.unlockedTrophy = true
						else
							if rewardState.unlockedTrophy then
								rewardState.unlockedTrophy = false
							else
								if renderContext:saveDataUtil():IsOwned() then
									renderContext:saveData():SaveToFile( renderContext:saveDataUtil() )
								end
								rewardState.state = "change"
							end
						end
					end
				end
			elseif "change" == rewardState.state then
				if rewardState.timer > rewardState.delayTime then
					levelsBeaten = renderContext:menu():levelsBeaten()
					if rewardState.allFullyBloomed then
						renderContext:menu():SelectPot( TOTAL_SCENE - 1 )
						-- DREAM trophy
						Trophy_UnlockTrophy( 0 )
					else
						renderContext:menu():SelectPot( math.min( #flowerObjs, currentLevel + 1, TOTAL_SCENE - 2 ) )
					end

					rewardState.allFullyBloomed = false
					rewardState.timer = 0
					rewardState.state = "glow"
				end
			elseif "glow" == rewardState.state then
				if rewardState.timer > rewardState.glowTime then
					SoundPlay( "MenuSfx","PotAppear", 1 )
					MoveGlow( currentLevel + 2 )

					rewardState.timer = 0
					rewardState.state = "done"
				end
			elseif "done" == rewardState.state then
				if rewardState.timer > rewardState.doneTime / 2 then
					isSnaped = false
				end
				if rewardState.timer > rewardState.doneTime then
					zoomTheta = 0
					zoomThetaOld = 0

					rewardState.timer = 0
					rewardState.state = "bloom"
					hasReward = false
				end
			else
				error( "Invalid reward state!\n" )
			end
		elseif fromPause then
			---------------------
			--when we're coming back from pause
			---------------------
			allowResume = true
			zoomTheta = 1
			zoomThetaOld = 1
			alphas.anyBTObjT = 0
			alphas.blackObjT = 0
			helpObj:Alpha( 0 )
			blackObj:Alpha( 1 - pauseState.timer / pauseState.delay )
			pauseState.timer = pauseState.timer + dt
			if pauseState.timer > pauseState.delay then
				pauseState.timer = 0
				fromPause = false
				DebugPrint("back from pause")
			end
		else
			---------------------
			--normal menu control
			---------------------
			S3DState.available = true

			--how quickly we move the camera is based on its postion ...
			local moveMult = CalcMoveMult()*fpsAdjust
			moveMult = moveMult * 0.06
			--... as well as the trigger state
			if actualTrigger < 0.1 then
				obeyTilt = obeyTilt + (0.05*fpsAdjust)
			else
				obeyTilt = obeyTilt - (0.05*fpsAdjust)
			end
			minZoomT = 1+actualTrigger
			obeyTilt = Clip( obeyTilt, 0, 1 )
			moveMult = moveMult * obeyTilt

			--get new camera position suggestion based on tilting
			local oldCamPos = camPos
			camPos = { camPos[1]+inputX*moveMult, camPos[2]+inputY*moveMult, 0 }

			--sometimes we give the player a little help pulling the camera to the closest pot
			if actualTrigger > 0.1 or math.abs( inputX ) < 0.001 then
				if actualTrigger > 0.1 then
					attractMult = attractMult + (0.075*fpsAdjust)
				else
					attractMult = attractMult + (0.025*fpsAdjust)
				end
			else
				attractMult = attractMult - (0.075*fpsAdjust)
			end
			attractMult = Clip( attractMult, 0, 1 )

			local closePot, secondPot = CalcClosestPots( camPos )
			if not secondPot then secondPot = { 0, 0, 0 } end
			local toClosePot = closePot[1] - camPos[1] / potDepth
			local toSecondPot = secondPot[1] - camPos[1] / potDepth
			local potDir = 1 ; if toClosePot < 0 then potDir = -1 end
			local potAttraction = math.min( 0.02, 0.5 * math.abs( toClosePot ), 0.333 * (math.abs( toSecondPot ) - math.abs( toClosePot )) )
			camPos[1] = camPos[1] + potAttraction * attractMult * potDir * fpsAdjust

			--we constrain the camera to a rail that connects all the pots.
			camPos[2] = CalcIdealY( camPos )

			ApplyBoundary( zoom )

			--if the controller goes idle, zoom out and show the full menu
			if idleState.controllerIdle then
				idleState.triggerTimer = idleState.triggerTimer + dt
				if idleState.triggerTimer > idleState.fadeGuiTriggerTime then
					alphas.GUIT = 0
				end
				if idleState.triggerTimer > idleState.zoomOutTriggerTime and
				   idleState.camLerp < 0.001 and
				   autoZoom > 0.5 then
					idleState.enabled = true
				end
			else
				idleState.enabled = false
				idleState.triggerTimer = 0
				alphas.GUIT = 1
			end
			if idleState.enabled then
				zoomTheta = Clip( zoomTheta - 0.05 * dt, 0, 1 )
				idleState.camLerp = Clip( idleState.camLerp + 0.022 * dt, 0, 1 )
				if zoomTheta < 0.0001 and idleState.camLerp > 0.999 then
					camPos = { 0, 0, 0 }
					idleState.enabled = false
					idleState.triggerTimer = 0
					idleState.camLerp = 0
					idleState.camLerpOld = 0
				end
			else
				idleState.camLerp = Clip( idleState.camLerp - 0.05 * dt, 0, 1 )
			end
			idleState.camLerpOld, idleState.camLerp = idleState.camLerp, Clip( idleState.camLerp + 0.95 * (idleState.camLerp - idleState.camLerpOld), 0, 1 )

			--this code communicates with the old menu system.  It handles pot selection,
			--among other things
			local maxFocusDist = 0.07
			local dfocus = 0.05
			local dzoom = 0.001 * fpsAdjust
			--see if the camera's in the center of the screen
			if math.abs( camPos[1] / potDepth ) < maxFocusDist then
				focus = focus + dfocus * math.max( 0, 0.3 - math.abs( inputX ) ) / 0.3
				if focus > 1 then
					snappedPot = -1
					isSnaped = false
					zoomTheta = zoomTheta - dzoom
				end
			else
				if math.abs( toClosePot ) < maxFocusDist then
					focus = focus + dfocus
					if focus > 1 then snappedPot = CalcClosestPotIdx( camPos ) - 1 end
				else
					focus = 0
					snappedPot = -1
				end
				isSnaped = true
				zoomTheta = zoomTheta + dzoom
			end
			focus = Clip( focus, 0, 1 )
			zoomThetaOld, zoomTheta = zoomTheta, Clip( zoomTheta + 0.95 * (zoomTheta - zoomThetaOld), 0, 1 )
			autoZoom = 0.5 * (1 - math.cos( math.pi * zoomTheta ))

			---------------------
			--update gameplay interaction
			---------------------
			--if camera is focused on the same pot, trigger timer
			if autoZoom>0.9 and snappedPot>-1 and allowResume==false then
				alphas.anyBTObjT = 1
			else
				alphas.anyBTObjT = 0
			end

			if autoZoom>0.9 then
				local maxButtonPressure = CalculateMaxButtonPressureMenu( pad, false )
				if effectiveParalysis > 0.1 then maxButtonPressure = 0 end
				if maxButtonPressure>0.95 or demo.forceFinish then
					--increase timer when player is still on top of the same pot
					if renderContext:menu():currentScene() == snappedPot and noSnapYet==false then
						timer = timer + dt
					elseif snappedPot > -1 then
						--Do some stuff when we've selected a pot
						renderContext:menu():SelectPot( snappedPot )

						--reset sound and wave and timer
						--SoundStopLua(currentSound)
						SoundUpdateVolLua(currentSound,0,1)
						waveObjScale = 0
						waveObj:Alpha(1)
						timer = 0
						noSnapYet = false

						--start transition sound and wave effect
						currentSound = SoundPlay("MenuSfx","Rollover_Level"..(snappedPot+1),1)
						local wavePos = flowerObjs[ snappedPot + 1 ]:LookAt()
						wavePos[3]=wavePos[3]+0.01
						waveObj:Pos(wavePos)
						waveObjScaleT = 0.5
						alphas.waveObjT = 0
					end
				else
					timer = 0
				end
			else
				noSnapYet = true
			end

			---------------------
			--Japanese "additional info" menu
			---------------------
			if infoState.enabled and not demo.forceFinish then
				if renderContext:menu():m_paralysis() < 0.5 then
					infoState.instructionsTicker = infoState.instructionsTicker + dt
					local infoInstructionsAlpha = Clip( infoState.instructionsTicker, 0, 1 )
					infoInstructionsAlpha = infoInstructionsAlpha * (0.75 + 0.25 * math.sin( 3 * infoState.instructionsTicker ))
					infoInstructionsAlpha = infoInstructionsAlpha * Clip( 7 - infoState.instructionsTicker, 0, 1 )
					infoInstructionsAlpha = infoInstructionsAlpha * (1 - infoState.alpha)
					infoState.instructionsBeginObj:Alpha( infoInstructionsAlpha )

					if infoState.curObj == 0 and pad:WasButtonPressed( Channel_Button_Triangle ) then
						infoState.curObj = 1
					elseif infoState.curObj > 0 then
						local buttonPressed = false
						for i, bid in ipairs( kButtonIds ) do
							if pad:WasButtonPressed( bid ) then
								buttonPressed = true
								break
							end
						end

						if buttonPressed then
							infoState.curObj = infoState.curObj + 1
							if infoState.curObj > #infoState.objs then
								infoState.curObj = 0
							end
						end
					end
				end
				if infoState.curObj > 0 then
					infoState.alpha = Clip( infoState.alpha + dt, 0, 1 )
				else
					infoState.alpha = Clip( infoState.alpha - dt, 0, 1 )
				end

				for objIdx, obj in ipairs( infoState.objs ) do
					if objIdx == infoState.curObj or (objIdx == #infoState.objs and infoState.curObj == 0) then
						obj:Alpha( 0.9 * infoState.alpha )
					else
						obj:Alpha( 0 )
					end
				end
				infoState.instructionsContObj:Alpha( infoState.alpha )
			end
		end

		--immediate mode demo UI elements
		demo.endAlphaT = 0
		--only reset the runUpsell flag if we've completely exited out of the upsell
		if demo.endObj:Alpha() < 0.01 and demo.blackObj:Alpha() < 0.01 then
			demo.runUpsell = true
		end

		--background fade to black as a confirmation mechanics, player can cancel it by releaseing the trigger
		if timer>fadeTime then

			S3DState.available = false

			--fade in black background behind the flower first
			intObj:Alpha((timer-fadeTime)/checkTime)
			--fade out sound first
			renderContext:menu():m_vol((1-autoZoom*0.6)*(1-(timer-fadeTime)/checkTime))
			--fade out R2 button
			anyBTObj:Alpha(1-(timer-fadeTime)/checkTime)

			--confirmed selection, now fade to black
			if timer>checkTime+fadeTime then
				if snappedPot < 1 or Menu_Mode_Full == renderContext:menu():GetMode() then
					isLoading = true
					SoundPlay("MenuSfx","EnterLevelWhoosh",1)
					timer = 0
					if snappedPot == levelsBeaten then
						alphas.glowObjT = 0
					end
				else
					alphas.anyBTObjT = 0
					demo.endAlphaT = 1

					--if the user selects the pot while in demo mode, we force him or her to sit through an upsell.
					if demo.endObj:Alpha() < 0.01 and demo.runUpsell then
						demo.forceFinish = true
					end

					if demo.endObj:Alpha() > 0.99 and demo.runUpsell then
						demo.blackAlphaT = 1

						if demo.blackObj:Alpha() > 0.95 then

							if demo.movieTimer <= 0 then
								local movieName = "Trailer"
								local audioName = "TrailerAudio"
								-- This is kind of hacky, but we divide total frames by total time to see if we
								-- are running at about 25Hz, and therefore need the PAL video.
								--local averageFps = renderContext:gameTiming():GetCurTickNum() / renderContext:gameTiming():GetTotalTime()
								--if "EU" == REGION and averageFps < 26 then
								--	movieName = "TrailerPal"
								--	audioName = "TrailerPalAudio"
								--end
								local audioVol = 1
								renderContext:movieBarn():PlayMovie( movieName, audioName, renderContext:resources(), audioVol )
							end
							local movieTimeLeft = 0 --renderContext:movieBarn():GetTimeLeft()
							local movieFade = math.min( math.max( 0, demo.movieTimer - demo.movieBlankTime ), movieTimeLeft ) / demo.movieFadeTime
							if renderContext:movieBarn():GetIsFading() then
								movieFade = math.min( movieFade, 1 - renderContext:movieBarn():GetFadeLevel() )
							end
							movieFade = math.max( 0, movieFade )
							movieFade = math.min( 1, movieFade )
							demo.movieObj:SetVideoTexture( renderContext:movieBarn():GetMovieTexture(), renderContext:movieBarn():GetLuminanceTexture() )
							demo.movieObj:Alpha( movieFade )
							demo.movieTimer = demo.movieTimer + dt

							if demo.movieTimer > 4.0 and
							   demo.movieAllowSkip and
							   not renderContext:movieBarn():GetIsFading() then
								for i, bid in ipairs( kButtonIds ) do
									if pad:WasButtonPressed( bid ) then
										renderContext:movieBarn():FadeOut( demo.movieFadeTime )
										break
									end
								end
							end

							if demo.jumpToStore == false then
								if movieTimeLeft <= 0 or not renderContext:movieBarn():GetIsPlaying() then
									demo.movieObj:Alpha( 0 )
									demo.promptObjT = 1
									demo.loadAlphaT = 0
									if demo.promptObj:Alpha() == 1 then
										demo.loadAlphaT = 1
									end

									local goToStore = false
									local goToMainMenu = false
									if "JP" == REGION then
										if pad:WasButtonPressed( Channel_Button_Cross ) then
											goToMainMenu = true
										end
										if pad:WasButtonPressed( Channel_Button_Circle ) then
											goToStore = true
										end
									else
										if pad:WasButtonPressed( Channel_Button_Circle ) then
											goToMainMenu = true
										end
										if pad:WasButtonPressed( Channel_Button_Cross ) then
											goToStore = true
										end
									end

									-- Go To Store
									if goToStore then
										DebugPrint("Going to PlayStation store from upsell screen")
										
										demo.promptObjT = 0
										demo.loadAlphaT = 1
										demo.jumpToStore = true
										isLoading = false
									end
									
									-- Go To Main Menu
									if goToMainMenu then
										DebugPrint("Returning to Main Menu from Upsell screen")
									
										demo.blackAlphaT = 0
										demo.runUpsell = false
										demo.forceFinish = false
										demo.movieTimer = 0
										demo.movieSkip = false
										demo.movieAllowSkip = true
										demo.waitForNpConnection = true
										demo.launchInGameCommerce = true
										demo.loadUiTimer = 0
										demo.loadAlphaT = 0
										demo.loadObj:Alpha( 0 )
										demo.jumpToStore = false
										demo.endAlphaT = 0
										demo.endObj:Alpha( 0 )
										demo.promptObjT = 0
										alphas.blackObjT = 0
										blackObj:Alpha( 0 )
										isLoading = false
											
										-- Return from Zoomed in
										MoveGlow(0)
										autoZoom = 0
										actualTrigger = 0
										idleState.controllerIdle = true
										idleState.triggerTimer = idleState.zoomOutTriggerTime
									end
								end 
							end
							
							if demo.jumpToStore then
								isLoading = false
								if demo.waitForNpConnection then
									demo.loadAlphaT = 1
									if demo.waitToDoNpConnectionFrames > 0 then
										demo.waitToDoNpConnectionFrames = demo.waitToDoNpConnectionFrames - 1
									else
										if NetworkConnection_kDialogStatus_Inactive == renderContext:networkConnection():GetNetworkStartDialogStatus() then
											if NetworkConnection_kDialogResult_None == renderContext:networkConnection():GetNetworkStartDialogResult() then
												renderContext:networkConnection():LaunchNetworkStartDialog()
											else
												renderContext:networkConnection():ClearNetworkStartDialogResult()
												demo.waitForNpConnection = false
												demo.launchInGameCommerce = renderContext:commerce():IsAvailable()
											end
										end
									end
								else
									demo.movieObj:Alpha( 0 )
									if demo.launchInGameCommerce then
										renderContext:commerce():LaunchProductBrowser( demo.productId )
										demo.launchInGameCommerce = false
									end

									if Commerce_kProdBrowse_Active == renderContext:commerce():GetProductBrowserStatus() then
										if demo.loadObj:Alpha() < 0.51 then
											demo.loadAlphaT = 1
										end
										if demo.loadObj:Alpha() > 0.99 then
											demo.loadAlphaT = 0.5
										end
									end

									if Commerce_kProdBrowse_Active ~= renderContext:commerce():GetProductBrowserStatus() then
										demo.blackAlphaT = 0
										demo.runUpsell = false
										demo.forceFinish = false
										demo.movieTimer = 0
										demo.movieSkip = false
										demo.movieAllowSkip = true
										demo.waitForNpConnection = true
										demo.launchInGameCommerce = true
										demo.loadUiTimer = 0
										demo.loadAlphaT = 0
										demo.jumpToStore = false
										
										-- If they purchased the game, let them continue playing.
										if Commerce_kProdBrowse_Success == renderContext:commerce():GetProductBrowserReturnStatus() and VerifyEntitlement( renderContext:resources() ) then
											renderContext:commerce():Terminate()
											renderContext:menu():SetMode( Menu_Mode_Full )
											Trophy_Initialize( 0 )

											-- Instantly load up the next level.
											isLoading = true
											SoundPlay("MenuSfx","EnterLevelWhoosh",1)
											timer = 0
											if snappedPot == levelsBeaten then
												alphas.glowObjT = 0
											end

											demo.endAlphaT = 0
											demo.endObj:Alpha( 0 )
											alphas.blackObjT = 1
											blackObj:Alpha( 1 )
										end
									end
								end
							end
						end
					end
				end
			end
		else
			--reset variables after cancelling the confirmation
			alphas.intObj = 0
			maxZoomT = 2
			vol = 1.0
			renderContext:menu():lvlTarget("")
			--fade out sound based on zoom
			renderContext:menu():m_vol(1-autoZoom*0.6)
			
			--if player pressed trigger, the help img fades out
			if isSnaped then 
				alphas.helpObjT = 0
			else
				alphas.helpObjT = 1
			end
		end

		---------------------
		--hide and unhide pots based on level complete
		---------------------
		for i = 0, #flowerObjs - 1 do
			local alpha = flowerObjs[i+1].obj:Alpha()
			if i <= levelsBeaten then
				alpha = alpha + (1 - flowerObjs[i+1].obj:Alpha()) * dt * 1.5
			else
				alpha = alpha + (0 - flowerObjs[i+1].obj:Alpha()) * dt * 1.5
			end
			flowerObjs[i+1].obj:Alpha( alpha )
			if flowerObjs[i+1].potObj then
				flowerObjs[i+1].potObj:Alpha( alpha )
			end
		end

		---------------------
		--transition room objects to the current level
		---------------------
		local curScene = renderContext:menu():currentScene()
		if roomTrans.fade < 1 then
			roomTrans.fade = math.min( 1, roomTrans.fade + dt * 0.75 )
		elseif curScene > -1 and curScene < TOTAL_SCENE and curScene ~= roomTrans.nex then
			roomTrans.cur = roomTrans.nex
			roomTrans.nex = curScene
			roomTrans.fade = 0
		end
		for i, room in ipairs( roomObjs ) do
			room.obj:SetTextureState( renderContext:resources(), room.tex..roomTrans.cur, room.tex..roomTrans.nex, roomTrans.fade )
		end

        --flowerPotTopRight:Alpha(0)
        --returnToGame:Alpha(0)

		if allowResume and not demo.forceFinish then
            --flowerPotTopRight:Alpha(0.75)
            --returnToGame:Alpha(0.75)

            setReturnToGame(true)

			alphas.resumeObjT = 1
			renderContext:menu():m_vol( 1 - resumeTimer / resumeTime )
			if not pad:IsButtonDown( Channel_Button_Start ) then pauseState.startReleased = true end
			if pauseState.startReleased and pad:IsButtonDown( Channel_Button_Start ) or resumeTimer > 0 then
				resumeTimer = resumeTimer + dt
				blackObj:Alpha( Clip( resumeTimer / resumeTime, 0, 1 ) )
				if resumeTimer > resumeTime then
					DebugPrint("start pressed")
					allowResume = false
					pauseState.startReleased = false
					renderContext:menu():ResumeGame()
					resumeTimer = 0
				end
			end
		else
			alphas.resumeObjT = 0

            setReturnToGame(false)
		end	
	else
		---------------------
		--when it's loading level
		---------------------
		--menu fades to black transition, camera zoom in, player can no longer cancel this process
		alphas.intObj = 1
		alphas.anyBTObjT = 0
		alphas.blackObjT = 1
		maxZoomT = 4
		vol = 0

		--when menu.cpp confirm loading level, we will cancel the isLoading process
		if renderContext:menu():m_isLaunching() == true then
			timer=timer+dt
			if timer>safeWaitTime then
				isLoading = false
				timer = 0
				DebugPrint("loading complete")
			end
		else
			if blackObj:Alpha()>0.99 and timer==0 then
				blackObj:Alpha(1)
				anyBTObj:Alpha(0)
				glowObj:Alpha(0)
				alphas.glowObjT = 0
				renderContext:menu():m_vol(0)
				currentLevel = renderContext:menu():currentScene()
				renderContext:menu():lvlTarget( levelNameMap[ currentLevel ] )
			end
		end
	end

	--------------------
	--tell C++ about the new camera values
	--------------------
	--update min and max zooms
	minZoom = minZoom + (minZoomT - minZoom)*dt
	maxZoom = maxZoom + (maxZoomT - maxZoom)*dt
	local zoom = minZoom+autoZoom*maxZoom
	renderContext:menu():camZoom(zoom)

	--add perlin noise for the camera
	local idleCam = LerpV3( 0.5 * (1 - math.cos( math.pi * idleState.camLerp )), camPos, { 0, 0, 0 } )
	local perlinX = 0.3*PerlinRandom(time*.1,4)
	local perlinY = 0.2*PerlinRandom(time*.1+1.0,4)
	local perlinZ = PerlinRandom(time*.1+2.0,4)
	local perlin = {perlinX,perlinY,perlinZ}
	local noisyCamPos = VectorAdd( idleCam, ScalarMult( perlin, (1-autoZoom*0.5)*0.5 ) )
	renderContext:menu():camPos( noisyCamPos )

	--------------------
	--update animated objects
	--------------------
	for i, animObj in ipairs( animObjs ) do
		animObj:Update( renderContext:resources(), dt )
	end

	--------------------
	--update snapiness interpolation
	--------------------
	--rotate menu help
	helpObj:Orient( -sensor[1] * 135 )

	--update UI alphas
	glowObj:Alpha( glowObj:Alpha() + (alphas.glowObjT-glowObj:Alpha())*dt*2 )
	waveObj:Alpha( waveObj:Alpha() + (alphas.waveObjT-waveObj:Alpha())*dt*2 )
	blackObj:Alpha( blackObj:Alpha() + (alphas.blackObjT-blackObj:Alpha())*dt )
	intObj:Alpha( intObj:Alpha() + (alphas.intObj-intObj:Alpha())*dt*3 )
	demo.endObj:Alpha( demo.endObj:Alpha() + (demo.endAlphaT - demo.endObj:Alpha())*dt*20 ) -- The fade speed is set to 20 for a special test build that doesn't include the "End of Demo" card.
	demo.blackObj:Alpha( demo.blackObj:Alpha() + (demo.blackAlphaT - demo.blackObj:Alpha())*dt*2 )
	demo.loadObj:Alpha( demo.loadObj:Alpha() + (demo.loadAlphaT - demo.loadObj:Alpha())*dt*2 )
	demo.promptObj:Alpha( demo.promptObj:Alpha() + (demo.promptObjT - demo.promptObj:Alpha())*dt*2 )

	--apply alpha blinking for GUI message
	alphas.GUI = alphas.GUI + (alphas.GUIT-alphas.GUI)*dt
	local minAlpha = 0.4
	local blinkSpd = 1.5
	alphas.helpObj = alphas.helpObj + (alphas.helpObjT-alphas.helpObj)*dt*3
		helpObj:Alpha( alphas.GUI*alphas.helpObj*(minAlpha+(1+math.sin(time*blinkSpd))*.5*(1-minAlpha)) )
	alphas.anyBTObj = alphas.anyBTObj + (alphas.anyBTObjT-alphas.anyBTObj)*dt*3
		anyBTObj:Alpha( alphas.GUI*alphas.anyBTObj*(minAlpha+(1+math.sin(time*blinkSpd))*.5*(1-minAlpha)) )
	alphas.resumeObj = alphas.resumeObj + (alphas.resumeObjT-alphas.resumeObj)*dt*3
		resumeObj:Alpha( alphas.GUI*alphas.resumeObj*(minAlpha+(1+math.sin(time*blinkSpd))*.5*(1-minAlpha)) )

	--update other UI properties
	local dhelpFade = dt * 2
	if glowObj:Pos()[1] < 0 then dhelpFade = -dhelpFade end
	local helpFade = Clip( helpObj:texFade() + dhelpFade, 0, 1 )
	helpObj:SetTextureState( renderContext:resources(), "Menu_help_left", "Menu_help_right", helpFade )

	local pulse = .05*PerlinRandom(time*1,2)+1
	glowObj:Scale({.2*pulse,.2*pulse,0})
	glowObj:Pos( VectorAdd( flowerObjs[ glowIdx ]:LookAt(), { 0, 0, -0.01 } ) )

	waveObjScale = waveObjScale + (waveObjScaleT-waveObjScale)*dt*2
	waveObj:Scale({waveObjScale,waveObjScale,waveObjScale})

	--update sound
	local curVol = renderContext:menu():m_vol() + (vol-renderContext:menu():m_vol())*dt
	renderContext:menu():m_vol(curVol)		

	--update rain
	if time%0.1<=0.033333 then
		RainObj:Pos( {Random( -0.25, -0.15 ),Random( -0.2, 0.2 ),7} )
		RainObj2:Pos( {Random( -0.3, -0.2 ),Random( 0, 0.2 ),14} )
		RainObj:Orient( (PerlinRandom(time*.2,4)+1)*.5*5 )
		RainObj2:Orient( (PerlinRandom(time*.2,4)+0.5)*.5*30 )
	end
	
	--update godray
	GodrayObj:Scale( {1.637*(1+PerlinRandom(time*.2,4)*.1), 1.162*(1+PerlinRandom(time*.2,4)*.1), 0.0} )

	--update dust	
	DustObj:Pos( {PerlinRandom(time*.01,4)*.1,PerlinRandom(time*0.01,2)*.1,4} )	
	DustObj2:Pos( {PerlinRandom(time*.02,4)*.15,PerlinRandom(time*0.01,2)*.15,5} )	
	DustObj3:Pos( {-PerlinRandom(time*.03,4)*.3,PerlinRandom(time*0.01,2)*.3,2} )	
	
	--update credit lvl falling petals
	for emitterIdx, emitter in ipairs( petalEmitters ) do
		emitter:Update( dt, renderContext:menu():currentScene() == 6 or renderContext:menu():currentScene() == 7)
	end
end

function CalcLookAtFlowerCamPos( fidx )
	local pos = { flowerObjs[ fidx ]:LookAt()[1] * potDepth, 0, 0 }
	pos[2] = CalcIdealY( { pos[1], 0, 0 } )
	return pos
end

function CalcIdealY( pos )
	local pots = {}
	for i=1,#flowerObjs do
		pots[i] = flowerObjs[i]:LookAt()
		pots[i][3] = 0
	end
	pots[ #flowerObjs + 1 ] = { 0, -0.125 * autoZoom, 0 }
	table.sort( pots, function( p1, p2 ) return p1[1] < p2[1] end )

	local x = pos[1] / potDepth

	local crossIdx = 1
	while crossIdx < #pots - 1 and x > pots[ crossIdx + 1 ][1] do
		crossIdx = crossIdx + 1
	end

	local pot1, pot2 = pots[ crossIdx ], pots[ crossIdx + 1 ]

	local t = (pos[1] / potDepth - pot1[1]) / (pot2[1] - pot1[1])

	return SmoothStepV1( t, pot1[2], pot2[2] ) * potDepth
end

function CalcClosestPots( pos )
	local pots = {}

	--caculate parallel distance from camera to the pot without considering the depth
	local maxPots = math.min( #flowerObjs, levelsBeaten + 1 )
	for i=1,maxPots do
		local potPos = flowerObjs[i]:LookAt()
		potPos[3] = 0
		--only caculate X distance
		pots[i] = { idx = i, dist = math.abs(pos[1]/potDepth-potPos[1]), pos = potPos }
	end
	pots[ maxPots + 1 ] = { idx = 0, dist = math.abs(pos[1]/potDepth), pos = { 0, -0.125 * autoZoom, 0 } }

	table.sort( pots, function( d1, d2 ) return d1.dist < d2.dist end )

	local posTbl = {}
	for i, d in ipairs( pots ) do posTbl[i] = d.pos end

	return unpack( posTbl )
end

function CalcClosestPotIdx( pos )
	local minDist = math.huge
	local idx = -1
	local maxPots = math.min( #flowerObjs, levelsBeaten + 1 )
	for i=1,maxPots do
		local dist = math.abs( pos[1] / potDepth - flowerObjs[i]:LookAt()[1] )
		if minDist > dist then
			minDist = dist
			idx = i
		end
	end
	return idx
end

function CalcMoveMult()
	local closePot = CalcClosestPots( camPos )
	local dist = math.abs( closePot[1] - camPos[1] / potDepth )

	local slowDist = 0.1
	return Lerp( math.max( 0, math.min( 1, 1 - (slowDist - dist) / slowDist ) ), 0.3, 1 )
end

function CalcBoundary()
	--calculate the minimal boundary to reach all the flowers
	local boundary = { -0.01, 0.0, -0.01, 0.0 }

	local maxPots = math.min( #flowerObjs, levelsBeaten + 1 )
	for i = 1,maxPots do
		local flower = flowerObjs[i]:LookAt()
		boundary[1] = math.min( boundary[1], flower[1] )
		boundary[2] = math.min( boundary[2], flower[2] )
		boundary[3] = math.max( boundary[3], flower[1] )
		boundary[4] = math.max( boundary[4], flower[2] )
	end

	return boundary
end

function ApplyBoundary(zoom)
	local lerpV = 0.825
	local boundary = CalcBoundary()
	if camPos[1]<boundary[1]*potDepth then
		camPos[1]=Lerp(lerpV,boundary[1]*potDepth,camPos[1])
	end
	if camPos[2]<boundary[2]*potDepth then
		camPos[2]=Lerp(lerpV,boundary[2]*potDepth,camPos[2])
	end
	if camPos[1]>boundary[3]*potDepth then
		camPos[1]=Lerp(lerpV,boundary[3]*potDepth,camPos[1])
	end
	if camPos[2]>boundary[4]*potDepth then
		camPos[2]=Lerp(lerpV,boundary[4]*potDepth,camPos[2])
	end
end

function MoveGlow(i)
	if i > 0 and i <= #flowerObjs then
		alphas.glowObjT = 1.0
		glowIdx = i
	else
		alphas.glowObjT = 0.0
	end
end

function MenuInitialize( menu, resources, aspectR, ipxDeltaX, ipxDeltaY )
	--read aspectRatio
	aspectRatio = aspectR

	--initialize all default room objects
	local potA = menu:AddSceneObject( resources, "Menu_pota0", -1 )
	potA:Pos( {-0.2, -0.29, potDepth-0.02} )
	potA:Scale( {0.14, 0.14, 0.0} )	
	potA:Orient( 0.0 ) 
	potA:Alpha( 1.0 )

	local potB = menu:AddSceneObject( resources, "Menu_potb0", -1 )
	potB:Pos( {-0.425, -0.26, potDepth-0.02} )
	potB:Scale( {0.2, 0.2, 0.0} )
	potB:Orient( 0.0 ) 
	potB:Alpha( 1.0 )

	local potC = menu:AddSceneObject( resources, "Menu_potc0", -1 )
	potC:Pos( {-.65, -0.29, potDepth-0.02} )
	potC:Scale( {0.13, 0.13, 0.0} )
	potC:Orient( 0.0 ) 
	potC:Alpha( 1.0 )

	local potD = menu:AddSceneObject( resources, "Menu_potd0", -1 )
	potD:Pos( {0.2, -0.28, potDepth-0.02} )
	potD:Scale( {0.15, 0.15, 0.0} )
	potD:Orient( 0.0 ) 
	potD:Alpha( 1.0 )

	local potE = menu:AddSceneObject( resources, "Menu_pote0", -1 )
	potE:Pos( {.4, -0.29, potDepth-0.02} )
	potE:Scale( {0.14, 0.14, 0.0} )
	potE:Orient( 0.0 )
	potE:Alpha( 1.0 )

	local potF = menu:AddSceneObject( resources, "Menu_potf0", -1 )
	potF:Pos( {.6, -0.285, potDepth-0.02} )
	potF:Scale( {0.14, 0.14, 0.0} )
	potF:Orient( 0.0 ) 
	potF:Alpha( 1.0 )

	local frame = menu:AddSceneObject( resources, "Menu_frame0", -1 )
	frame:Pos( {0.82, -0.255, potDepth-0.02} )
	frame:Scale( {0.20, 0.21, 0.0} )
	frame:Orient( 0.0 ) 
	frame:Alpha( 1.0 )

	local sky = menu:AddSceneObject( resources, "Menu_sky0", -1 )
	sky:Pos( {-0.12, 0.0, 100.0} )
	sky:Scale( {1.536, 1.024, 0.0} )
	sky:Orient( 0.0 ) 
	sky:Alpha( 1.0 )

	local citytall = menu:AddSceneObject( resources, "Menu_citytall0", -1 )
	citytall:Pos( {-0.2, 0.10, 30.0} )
	citytall:Scale( {1.307, .778, 0.0} )
	citytall:Orient( 0.0 )
	citytall:Alpha( 1.0 )

	local city = menu:AddSceneObject( resources, "Menu_city0", -1 )
	city:Pos( {-0.1, -.18, 15.0} )
	city:Scale( {1.394, .434, 0.0} )
	city:Orient( 0.0 )
	city:Alpha( 1.0 )

	local wall = menu:AddSceneObject( resources, "Menu_wall0", -1 )
	wall:Pos( {0.0, 0.0, 5.8} )
	wall:Scale( {2.232, 1.265, 0.0} )
	wall:Orient( 0.0 )
	wall:Alpha( 1.0 )

	local mtable = menu:AddSceneObject( resources, "Menu_table0", -1 )
	mtable:Pos( {0.15, -0.51, 3.4} )
	mtable:Scale( {2.048, .512, 0.0} )
	mtable:Orient( 0.0 )
	mtable:Alpha( 1.0 )

	local chair = menu:AddSceneObject( resources, "Menu_chair0", -1 )
	chair:Pos( {0.6, -0.55, 2.0} )
	chair:Scale( {.7, .7, 0.0} )
	chair:Orient( 0.0 ) 
	chair:Alpha( 1.0 )

	function MakeRoom( obj, tex )
		return
			{ obj = obj
			, tex = tex
			}
	end
	roomObjs =
	{ MakeRoom( potA, "Menu_pota" )
	, MakeRoom( potB, "Menu_potb" )
	, MakeRoom( potC, "Menu_potc" )
	, MakeRoom( potD, "Menu_potd" )
	, MakeRoom( potE, "Menu_pote" )
	, MakeRoom( potF, "Menu_potf" )
	, MakeRoom( frame, "Menu_frame" )
	, MakeRoom( sky, "Menu_sky" )
	, MakeRoom( citytall, "Menu_citytall" )
	, MakeRoom( city, "Menu_city" )	
	, MakeRoom( wall, "Menu_wall" )
	, MakeRoom( mtable, "Menu_table" )
	, MakeRoom( chair, "Menu_chair" )
	}

	--flowers
	local flowerA = menu:AddSceneObject( resources, "Menu_flowera0", -1 )
	flowerA:Pos( {-0.195, -0.14, potDepth} )
	flowerA:Scale( {0.15, 0.3, 0.0} )
	flowerA:Orient( 0.0 )
	flowerA:Alpha( 0.0 )

	local flowerB = menu:AddSceneObject( resources, "Menu_flowerb0", -1 )
	flowerB:Pos( {-0.425 , -0.1, potDepth} )
	flowerB:Scale( {0.15, 0.3, 0.0} )
	flowerB:Orient( 0.0 )
	flowerB:Alpha( 0.0 )

	local flowerC = menu:AddSceneObject( resources, "Menu_flowerc0", -1 )
	flowerC:Pos( {-0.65, -0.125, potDepth} )
	flowerC:Scale( {0.15, 0.3, 0.0} )
	flowerC:Orient( 0.0 )
	flowerC:Alpha( 0.0 )

	local flowerD = menu:AddSceneObject( resources, "Menu_flowerd0", -1 )
	flowerD:Pos( {0.2, -0.13, potDepth} )
	flowerD:Scale( {0.15, 0.3, 0.0} )
	flowerD:Orient( 0.0 )
	flowerD:Alpha( 0.0 )

	local flowerE = menu:AddSceneObject( resources, "Menu_flowere0", -1 )
	flowerE:Pos( {0.41, -0.149, potDepth} )
	flowerE:Scale( {0.15, 0.3, 0.0} )
	flowerE:Orient( 0.0 )
	flowerE:Alpha( 0.0 )

	local flowerF = menu:AddSceneObject( resources, "Menu_flowerf0", -1 )
	flowerF:Pos( {0.595, -0.185, potDepth} )
	flowerF:Scale( {0.15, 0.3, 0.0} )
	flowerF:Orient( 0.0 )
	flowerF:Alpha( 0.0 )

	local flowerFrame = menu:AddSceneObject( resources, "Blank", -1 )
	flowerFrame:Pos( {0.82, -0.255, potDepth} )
	flowerFrame:Scale( {0.0, 0.0, 0.0} )
	flowerFrame:Orient( 0.0 )
	flowerFrame:Alpha( 0.0 )

	function FlowerCalcBloomIdxs( self )
		local base = math.floor( self.bloom )
		local frac = self.bloom - base
		local idx1 = math.min( base + 1, #self.offsets )
		local idx2 = math.min( idx1 + 1, #self.offsets )
		local t = SmoothStepV1( frac, 0, 1 )
		return idx1, idx2, t
	end
	function FlowerLookAt( self )
		local idx1, idx2, t = FlowerCalcBloomIdxs( self )
		local offset = LerpV3( t, self.offsets[ idx1 ], self.offsets[ idx2 ] )
		return VectorAdd( self.obj:Pos(), offset )
	end
	function FlowerSetBloom( self, resources, bloom )
		self.bloom = bloom
		if self.tex ~= "" then
			local idx1, idx2, t = FlowerCalcBloomIdxs( self )
			self.obj:SetTextureState( resources, self.tex..(idx1-1), self.tex..(idx2-1), t )
		end
	end
	function MakeFlower( obj, potObj, tex, offsets, secretPetalOffset, secretPetalPacking )
		local flower =
			{ obj = obj
			, potObj = potObj
			, petals = {}
			, tex = tex
			, offsets = offsets
			, bloom = 0
			, LookAt = FlowerLookAt
			, SetBloom = FlowerSetBloom
			}

		if secretPetalOffset and secretPetalPacking then
			for petalIdx = 1,3 do
				local petalObj = menu:AddSceneObject( resources, "Menu_special_leaf", -1 )
				petalObj:Pos( VectorAdd( obj:Pos(), { 0.035 * (petalIdx - 2), -0.105, 0.0 } ) )
				petalObj:Scale( {0.035, 0.035, 0.0} )
				petalObj:Orient( 0.0 )
				petalObj:Alpha( 0.0 )

				local petalPath = {}
				petalPath[2] = VectorAdd( obj:Pos(), { secretPetalOffset[1] + secretPetalPacking * (petalIdx - 2), secretPetalOffset[2], 0.0 } )
				petalPath[2] = VectorAdd( petalPath[2], { Random( -0.004, 0.004 ), Random( -0.002, 0.002 ), 0.0 } )
				petalPath[1] = { petalPath[2][1], petalPath[2][2] + 0.335, petalPath[2][3] }
				flower.petals[ petalIdx ] =
					{ obj = petalObj
					, path = petalPath
					, fall = 0
					}
			end
		end

		return flower
	end
	flowerObjs =
	{ MakeFlower( flowerA, potA, "Menu_flowera", { {0.035,-0.04,0}, {0.005,0.045,0}, {-0.01,0.1,0} }, { -0.007, -0.104 }, 0.035 )
	, MakeFlower( flowerB, potB, "Menu_flowerb", { {0.020,0,0}, {0.03,0.015,0}, {0.0,0.09,0} }, { -0.005, -0.099 }, 0.040 )
	, MakeFlower( flowerC, potC, "Menu_flowerc", { {0.02,-0.01,0}, {0.015,0.03,0}, {0.015,0.055,0} }, { -0.003, -0.105 }, 0.026 )
	, MakeFlower( flowerD, potD, "Menu_flowerd", { {0.005,-0.005,0}, {0.005,0.04,0}, {-0.01,0.08,0} }, { -0.002, -0.099 }, 0.021 )
	, MakeFlower( flowerE, potE, "Menu_flowere", { {0.005,-0.015,0}, {0.03,0.0,0}, {0.038,0.01,0} }, { -0.013, -0.090 }, 0.032 )
	, MakeFlower( flowerF, potF, "Menu_flowerf", { {0.01,-0.04,0}, {0.01,-0.01,0}, {0.01,0.01,0} }, { 0.002, -0.098 }, 0.028 )
	, MakeFlower( flowerFrame, frame, "", { {0.01,0.04,0}, {0.01,0.04,0}, {0.01,0.04,0} } )
	}

	--add level special obj
	--cloth line
	local a = menu:AddSceneObject( resources, "Menu_cloth", 0 )
	a:Pos( {-.25, 0, 7.0} )
	a:Scale( {1.75, 0.75, 0.0} )
	a:Orient( 0.0 )
	a:Alpha( 0.0 )
	--fan
	local a = menu:AddSceneObject( resources, "Menu_fan", 2 )
	a:Pos( {-1, -0.54, 2.9} )
	a:Scale( {0.55, 0.55, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--rain
	RainObj = menu:AddSceneObject( resources, "Blank", 4 )
	RainObj:Pos( {-0.2, 0, 7} )
	RainObj:Scale( {1, 1, 0.0} )
	RainObj:Orient( 0.0 )
	RainObj:Alpha( 1.0 )

	--rain2
	RainObj2 = menu:AddSceneObject( resources, "Blank", 4 )
	RainObj2:Pos( {-0.2, 0, 14} )
	RainObj2:Scale( {.95, .95, 0.0} )
	RainObj2:Orient( 10.0 )
	RainObj2:Alpha( 1.0 )

	--grassfront
	local a = menu:AddSceneObject( resources, "Menu_grassfront", 7 )
	a:Pos( {0.0, -.55, 10} )
	a:Scale( {2.232, .144, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--grassmid
	local a = menu:AddSceneObject( resources, "Menu_grassmid", 7 )
	a:Pos( {0.0, -.525, 15} )
	a:Scale( {2.232, .248, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--grassfield
	local a = menu:AddSceneObject( resources, "Menu_grassfield", 7 )
	a:Pos( {0.0, -.475, 20} )
	a:Scale( {2.232, .65, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--grassfar
	local a = menu:AddSceneObject( resources, "Menu_grassfar", 7 )
	a:Pos( {0.0, -.375, 40} )
	a:Scale( {2.232, .481, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--grassmountain
	local a = menu:AddSceneObject( resources, "Menu_grassmountain", 7 )
	a:Pos( {-0.2, -.15, 90} )
	a:Scale( {2.048, .256, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--grasssky
	local a = menu:AddSceneObject( resources, "Menu_grasssky", 7 )
	a:Pos( {0.0, 0.0, 98} )
	a:Scale( {2.232, 1.265, 0.0} )
	a:Orient( 0.0 ) 
	a:Alpha( 1.0 )

	--dust
	DustObj = menu:AddSceneObject( resources, "Menu_dust", 5 )
	DustObj:Pos( {0.0, 0.0, 3.3} )
	DustObj:Scale( {3, 1.6, 0.0} )
	DustObj:Orient( 0.0 ) 
	DustObj:Alpha( 1.0 )

	DustObj2 = menu:AddSceneObject( resources, "Menu_dust", 5 )
	DustObj2:Pos( {0.0, 0.0, 4.5} )
	DustObj2:Scale( {2.232, 1.265, 0.0} )
	DustObj2:Orient( 0.0 ) 
	DustObj2:Alpha( 1.0 )

	DustObj3 = menu:AddSceneObject( resources, "Menu_dust", 5 )
	DustObj3:Pos( {0.0, 0.0, 2.0 } )
	DustObj3:Scale( {4.464, 2.53, 0.0} )
	DustObj3:Orient( 0.0 ) 
	DustObj3:Alpha( 1.0 )

	--godray
	GodrayObj = menu:AddSceneObject( resources, "Menu_godray", 5 )
	GodrayObj:Pos( {-.2, 0.0, 5} )
	GodrayObj:Scale( {1.637, 1.162, 0.0} )
	GodrayObj:Orient( 0.0 ) 
	GodrayObj:Alpha( 1.0 )

	--curtain
	local Curtain = menu:AddSceneObject( resources, "Blank", 7 )
	Curtain:Pos( {0.74, -0.1, 5.5} )
	Curtain:Scale( {.344, 1.286, 0.0} )
	Curtain:Orient( 0.0 ) 
	Curtain:Alpha( 1.0 )

	local Curtain2 = menu:AddSceneObject( resources, "Blank", 7 )
	Curtain2:Pos( {-.9, -0.1, 5.5} )
	Curtain2:Scale( {-.344, 1.286, 0.0} )
	Curtain2:Orient( 0.0 ) 
	Curtain2:Alpha( 1.0 )

	--visual white noise
	WNObj = menu:AddSceneObject( resources, "Blank", -1 )
	WNObj:Pos( {0, 0, 0} )
	WNObj:Scale( {2, 2, 0.0} )
	WNObj:Orient( 0.0 )
	WNObj:Alpha( 0.05 )

	function AnimObjUpdate( self, resources, dt )
		self.timer = self.timer + dt
		if self.timer > self.flipTime then
			self.timer = 0
			self.animIdx = self.animIdx + 1
			if self.animIdx > #self.texList then self.animIdx = 1 end
		end
		local idx1 = self.animIdx
		local idx2 = idx1 + 1
		if idx2 > #self.texList then idx2 = 1 end
		self.obj:SetTextureState( resources, self.texList[ idx1 ], self.texList[ idx2 ], self.timer / self.flipTime )
	end
	function MakeAnimObj( obj, texList, flipTime )
		return
			{ obj = obj
			, texList = texList
			, flipTime = flipTime
			, timer = 0
			, animIdx = 1
			, Update = AnimObjUpdate
			}
	end
	animObjs =
	{
		MakeAnimObj( WNObj, { "Grain1", "Grain2", "Grain3", "Grain2", "Grain1", "Grain3", "Grain2", "Grain3" }, 0.3 ),
		MakeAnimObj( RainObj, { "Menu_rain", "Menu_rain2" }, 0.1 ),
		MakeAnimObj( RainObj2, { "Menu_rain2", "Menu_rain" }, 0.1 ),
		MakeAnimObj( Curtain, { "Menu_curtain", "Menu_curtain2", "Menu_curtain3", "Menu_curtain2" }, 1.5 ),
		MakeAnimObj( Curtain2, { "Menu_curtain", "Menu_curtain2", "Menu_curtain3", "Menu_curtain2" }, 1.5 ),
	}

	--petal emitters
	local function MakePetalEmitter( maxPetals, scaleRange, speed, densityPow, densityFreq )
		local emitter = {}

		emitter.petals = {} 
		for i = 1,maxPetals do
			local petal = {}
			petal.obj = menu:AddSceneObject( resources, "Menu_tower_petal", -1 )
			petal.obj:Pos( { -5, -5, Random( 10, 15 ) } )
			local scale = Lerp( math.pow( Random( 0, 1 ), 2 ), scaleRange[1], scaleRange[2] )
			petal.obj:Scale( { scale, scale, 0.0 } )
			petal.obj:Orient( 0.0 )
			petal.obj:Alpha( 0 )
			petal.ticker = i / maxPetals
			petal.height = Random( 0.0, 0.5 )
			petal.offset = Random( 0, 1000 )
			table.insert( emitter.petals, petal )
		end

		emitter.ticker = 0

		emitter.Update = function( self, dt, drawPetals )
			emitter.ticker = emitter.ticker + dt
			local density = math.pow( 0.5 + PerlinRandom( emitter.ticker * densityFreq, 0 ), densityPow )

			for petalIdx, petal in ipairs( self.petals ) do
				petal.ticker = petal.ticker + speed * dt
				local t = petal.ticker
				local o = petal.offset
				local newPos =
				{ -0.95 + t * 1.95 + 0.3 * PerlinRandom( o + t * 2, 2 )
				, petal.height + t * -0.35 + 0.4 * PerlinRandom( o + 10 + t * 2, 2 )
				, petal.obj:Pos()[3]
				}
				petal.obj:Pos( newPos )
				petal.obj:Orient( 1 * 360 * t + 2*360 * PerlinRandom( o - 0.5 + t * 2, 2 ) )

				if petal.ticker > 1 then
					petal.ticker = 0
					petal.height = Random( 0.0, 0.5 )
					petal.offset = Random( 0, 1000 )
					if not drawPetals or Random( 0, 1 ) > density then
						petal.obj:Alpha( 0 )
					else
						petal.obj:Alpha( Random( 0.3, 1.0 ) )
					end
				end
			end
		end

		return emitter
	end

	petalEmitters =
	{ MakePetalEmitter( 30, { 0.0075, 0.02 }, 0.065, 0.5, 0.12 )
	, MakePetalEmitter( 20, { 0.005, 0.015 }, 0.18, 2, 0.3 )
	}

	--adding gameplay objects

local lowerLogoHack = aspectRatio - 1.77

if(lowerLogoHack<0) then
    lowerLogoHack = 0.0
end

lowerLogoHack = lowerLogoHack * 0.15

	logoObj = menu:AddSceneObject( resources, "Menu_logo", -1 )
	logoObj:Pos( {0.64, 0.35-lowerLogoHack, 3.0} )
	logoObj:Scale( {.5,.5, 0.0} )
	logoObj:Orient( 0.0 )
	logoObj:Alpha( 1.0 )

	--intensify flowers by blacken everything behind them
	intObj = menu:AddSceneObject( resources, "Black", -1 )
	intObj:Pos( {0.0, 0.0, 3.6} )
	intObj:Scale( {20.0, 20.0, 0.0} )	
	intObj:Orient( 0.0 )  
	intObj:Alpha( 0.0 )

	--glow behind newly born flower
	glowObj = menu:AddSceneObject( resources, "Menu_glow", -1 )
	glowObj:Pos( {0,0,0} )
	glowObj:Scale( {.2,.2, 0} )	
	glowObj:Orient( 0.0 )
	glowObj:Alpha( 0.0 )
	MoveGlow(1)

	--vignette that darken the border area of the screen
	ViObj = menu:AddSceneObject( resources, "Menu_vignette", -1 )
	ViObj:Pos( {0, 0, 0} )
	ViObj:Scale( {2.0, 1.125, 0.0} )
	ViObj:Orient( 0.0 )
	ViObj:Alpha( 0.0 )

	--wave effect
	waveObj = menu:AddSceneObject( resources, "Menu_glow", -1 )
	waveObj:Pos( {0,0,0} )
	waveObj:Scale( {0,0,0} )
	waveObj:Orient( 0.0 )
	waveObj:Alpha( 1.0 )

	--tilt instruction
	helpObj = menu:AddSceneObject( resources, "Menu_help_left", -1 )
	helpObj:Pos( {0.0, -0.0, 0} )
	helpObj:Scale( {0.4, 0.4, 0.0} )
	helpObj:Orient( 0.0 )
	helpObj:Alpha( 0.0 )

	--Start button icon, using dimensions from Menu_Any for the moment...
	--resumeObj = menu:AddSceneObject( resources, "Menu_pressstart", -1 )
	--resumeObj:Pos( {0.60, -0.45, 0} )
    --resumeObj:Scale( {0.8, 0.2, 0.0} )
    --resumeObj:Orient( 0.0 )
    --resumeObj:Alpha( 0.0 )

    --Start button icon
    resumeObj = menu:AddSceneObject( resources, "Menu_pressstart", -1 )
    resumeObj:Pos( {0.2, -0.45, 0} )
    resumeObj:Scale( {1.5, 0.1875, 0.0} )
    resumeObj:Orient( 0.0 )
    resumeObj:Alpha( 0.0 )

	--Any button
	--anyBTObj = menu:AddSceneObject( resources, "Menu_any", -1 )
	--anyBTObj:Pos( {0.65, -0.45, 0} )
	--anyBTObj:Scale( {0.4, 0.1, 0.0} )
	--anyBTObj:Orient( 0.0 )
	--anyBTObj:Alpha( 0.0 )

    --Any button
    anyBTObj = menu:AddSceneObject( resources, "Menu_any", -1 )
    anyBTObj:Pos( {0, 0.25, 0} )
    anyBTObj:Scale( {1, 0.25, 0.0} )
    anyBTObj:Orient( 0.0 )
    anyBTObj:Alpha( 0.0 )

	-- 3d toggle
	--S3DState.enabled = menu:GetS3DAvailable()
	if S3DState.enabled then
		local prompt3D = menu:AddSceneObject( resources, "3D_Enable", -1 )
		prompt3D:Pos( {-0.30, -0.453, 0.0} )
		prompt3D:Scale( {1.0, 0.125, 0.0} )
		prompt3D:Orient( 0.0 )
		prompt3D:Alpha( 1.0 )

		S3DState.enableObj = prompt3D

		prompt3D = menu:AddSceneObject( resources, "3D_Disable", -1 )
		prompt3D:Pos( {-0.30, -0.453, 0.0} )
		prompt3D:Scale( {1.0, 0.125, 0.0} )
		prompt3D:Orient( 0.0 )
		prompt3D:Alpha( 0.0 )

		S3DState.disableObj = prompt3D
	end


--letter boxes
local letterBoxY = 1.0/aspectRatio
local letterBoxYSub = letterBoxY-1/1.777

if letterBoxYSub < 0 then
letterBoxYSub = 0
end


local letterBoxHeight = 2.0 * letterBoxYSub
--    DebugPrint(aspectRatio)

lb1 = menu:AddSceneObject( resources, "Black", -1 )
lb1:Pos( {0, letterBoxY, 0} )
lb1:Scale( {10, letterBoxHeight, 0.0} )
lb1:Orient( 0.0 )
lb1:Alpha( 1.0 )

lb2 = menu:AddSceneObject( resources, "Black", -1 )
lb2:Pos( {0, -letterBoxY, 0} )
lb2:Scale( {10, letterBoxHeight, 0.0} )
lb2:Orient( 0.0 )
lb2:Alpha( 1.0 )


	--fade to black screen
	blackObj = menu:AddSceneObject( resources, "Black", -1 )
	blackObj:Pos( {0.0, 0.0, -1} )
	blackObj:Scale( {20.0, 20.0, 0.0} )
	blackObj:Orient( 0.0 )
	blackObj:Alpha( 0.0 ) 


	--demo-related messages
	demo.endObj = menu:AddSceneObject( resources, "Menu_demo_end", -1 )
	demo.endObj:Pos( {0.0, 0.14, -1} )
--	demo.endObj:Scale( {0.5, 0.125, 0.0} ) -- Randall asked for a build with this card removed.  I have a feeling he may want it back ...
	demo.endObj:Scale( {0.0, 0.0, 0.0} )
	demo.endObj:Orient( 0.0 )
	demo.endObj:Alpha( 0.0 )

	demo.blackObj = menu:AddSceneObject( resources, "Black", -1 )
	demo.blackObj:Pos( {0.0, 0.0, -1} )
	demo.blackObj:Scale( {10.0, 10.0, 0.0} )
	demo.blackObj:Orient( 0.0 )
	demo.blackObj:Alpha( 0.0 )

	demo.loadObj = menu:AddSceneObject( resources, "Menu_demo_load", -1 )
	demo.loadObj:Pos( {0.0, 0.0, -1} )
	demo.loadObj:Scale( {1.0, 0.125, 0.0} )
	demo.loadObj:Orient( 0.0 )
	demo.loadObj:Alpha( 0.0 )

	demo.promptObj = menu:AddSceneObject( resources, "Menu_demo_prompt", -1 )
	demo.promptObj:Pos( {0.0, 0.0, -1} )
	demo.promptObj:Scale( {2.0, 1.125, 0.0} )
	demo.promptObj:Orient( 0.0 )
	demo.promptObj:Alpha( 0.0 )

	demo.movieObj = menu:AddSceneObject( resources, "Black", -1 )
	demo.movieObj:Pos( {0.0, 0.0, -1} )
	demo.movieObj:Scale( {2.0, -2.0 * 9.0/16.0, 0.0} )
	demo.movieObj:Orient( 0.0 )
	demo.movieObj:Alpha( 0.0 )

	if infoState.enabled then
		local infoInstructionsBegin = menu:AddSceneObject( resources, "InfoJp_InstructionsBegin", -1 )
		infoInstructionsBegin:Pos( {0.0, -0.40, -1} )
		infoInstructionsBegin:Scale( {0.5, 0.25, 0.0} )
		infoInstructionsBegin:Orient( 0.0 )
		infoInstructionsBegin:Alpha( 0.0 )

		local infoHealth = menu:AddSceneObject( resources, "InfoJp_Health", -1 )
		infoHealth:Pos( {0.0, 0.0, -1} )
		infoHealth:Scale( {2.0, 1.125, 0.0} )
		infoHealth:Orient( 0.0 )
		infoHealth:Alpha( 0.0 )

		local infoUsage = menu:AddSceneObject( resources, "InfoJp_Usage", -1 )
		infoUsage:Pos( {0.0, 0.0, -1} )
		infoUsage:Scale( {2.0, 1.125, 0.0} )
		infoUsage:Orient( 0.0 )
		infoUsage:Alpha( 0.0 )

		local infoController = menu:AddSceneObject( resources, "InfoJp_Controller", -1 )
		infoController:Pos( {0.0, 0.0, -1} )
		infoController:Scale( {2.0, 1.125, 0.0} )
		infoController:Orient( 0.0 )
		infoController:Alpha( 0.0 )

		local infoInstructionsCont = menu:AddSceneObject( resources, "InfoJp_InstructionsCont", -1 )
		infoInstructionsCont:Pos( {0.75, -0.45, -1} )
		infoInstructionsCont:Scale( {0.4, 0.2, 0.0} )
		infoInstructionsCont:Orient( 0.0 )
		infoInstructionsCont:Alpha( 0.0 )

		infoState.instructionsBeginObj = infoInstructionsBegin
		infoState.instructionsContObj = infoInstructionsCont
		infoState.objs = { infoHealth, infoUsage, infoController }
	end
end

----------------------------------
--Tools function
----------------------------------
function SmoothStep(x,min,max)
	if x<min then
		return 0
	end
	if x>=max then
		return 1
	end
	return -2*((x-min)/(max-min))*((x-min)/(max-min))*((x-min)/(max-min))+3*((x-min)/(max-min))*((x-min)/(max-min))
end

function SmoothStepV1(x,v1,v2)
	return Lerp(SmoothStep(x,0,1),v1,v2)
end

function SmoothStepV3(x,v1,v2)
	return LerpV3(SmoothStep(x,0,1),v1,v2)
end

function Lerp( alpha, x0, x1 )
	return x0 + alpha * ( x1 - x0 )
end

function LerpV3( alpha, vec0, vec1 ) 
	local ret = { 0, 0, 0 }
	ret[ 1 ] = Lerp( alpha, vec0[ 1 ], vec1[ 1 ] )
	ret[ 2 ] = Lerp( alpha, vec0[ 2 ], vec1[ 2 ] )
	ret[ 3 ] = Lerp( alpha, vec0[ 3 ], vec1[ 3 ] )
	return ret
end

local function Clip( x, min, max )
	if min > max then min,max = max,min end
	return math.max( min, math.min( max, x ) )
end

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


-------------------------------------------------------------------------------
-- Animation data
-------------------------------------------------------------------------------
petalAnimData_translateX =
{ 0.15894645546207, 0.12080124971646, 0.029381195897085, -0.11257505530071, -0.30079518063849, -0.52946189124349, -0.79135768253681
, -1.0781726675548, -1.3809461762767, -1.6905794946667, -1.9983402520767, -2.2962852730411, -2.5775543191206, -2.8365204738279, -3.0688117261121
, -3.2712354548109, -3.4416423221903, -3.5787620677387, -3.6820354619359, -3.7514578667493, -3.7874424747725, -3.7895733205794, -3.7475452562869
, -3.6577780959983, -3.5194268865665, -3.3320910352455, -3.0959507305733, -2.8119195265818, -2.4818056343808, -2.108469131446, -1.6959563457205
, -1.2495873974139, -0.77597032927247, -0.28291783381965, 0.22074786618305, 0.72549993731239, 1.2214990724179, 1.6990312352199, 2.1489382027039
, 2.5629889149367, 2.934152358216, 3.2567528856434, 3.526509997358, 3.7404810133505, 3.8969339506081, 3.9951794547329, 4.0353869500994
, 4.0197532967736, 3.9524884391744, 3.8355212149174, 3.6710321973501, 3.4615556147035, 3.209995160474, 2.919635953352, 2.5941516919556
, 2.2376060498972, 1.8544474083929, 1.4494961356293, 1.0279238001428, 0.59522395050708, 0.15717440044015, -0.28020868523394, -0.71072422195338
, -1.1280460815488, -1.5257879753555, -1.897571597958, -2.2370960619549, -2.5382065386456, -2.7949600249645, -3.0016862850589, -3.1530422555398
, -3.2440585359389, -3.271310846255, -3.2684500280756, -3.2508185190549, -3.2150883579323, -3.156999732322, -3.0712228973566, -2.9515012383455
, -2.7915986154297, -2.5878385388545, -2.3432264108628, -2.0700887263594, -1.786625994362, -1.5092096864705, -1.2477541230149, -1.0137383946484
, -0.82873744396856, -0.67992248838386, -0.55768910051849, -0.45599704704678, -0.37074827164193, -0.29900505152732, -0.23857053906809, -0.18774517427549
, -0.14517623429494, -0.10976012973823, -0.080576186505581, -0.056840007144723, -0.03786937817243, -0.023058353949685, -0.011856652712409, -0.003752355522282
, 0.0017436433799478, 0.0051128092064971, 0.0068452101558683, 0.0074586543847694, 0.0075233843327154
}
petalAnimData_translateY =
{ 19.992592803893, 19.387047640508, 18.812076085784, 18.318438766042, 17.91185094738, 17.580164833324, 17.30911115313
, 17.086721504591, 16.903747371019, 16.753158711314, 16.62958270314, 16.528859450711, 16.448583368775, 16.393757272624, 16.347750301789
, 16.305588364029, 16.26585308762, 16.227947446428, 16.191624399049, 16.156896362988, 16.124227409316, 16.092950255606, 16.032684078639
, 15.928690647901, 15.772858559625, 15.558219278633, 15.283083033473, 14.956916597217, 14.602858467682, 14.250635854296, 13.924320551319
, 13.636356208236, 13.389667220403, 13.182084175588, 13.009525122476, 12.867603528458, 12.752276714912, 12.660037820946, 12.587920184904
, 12.547927599267, 12.531381929236, 12.521683874584, 12.515629254634, 12.511945077698, 12.509962713568, 12.509252663068, 12.509379488923
, 12.495826416002, 12.435531861905, 12.316939562685, 12.127860382063, 11.857565123401, 11.503792176681, 11.082686697904, 10.631211238458
, 10.19220400112, 9.7953459289195, 9.45245069878, 9.1636284807271, 8.9239458291092, 8.7271771961482, 8.5673717434529, 8.4393478788142
, 8.3387541769764, 8.2617703024797, 8.1980575453485, 8.1432684846529, 8.0968838525456, 8.0602991717684, 8.0378412178683, 8.0323274727416
, 8.0140379935286, 7.9773989700891, 7.920741867288, 7.8426635368764, 7.7424337868503, 7.6206250535915, 7.4799194368649, 7.3258239741857
, 7.1667442981025, 7.0128918237767, 6.8741876026564, 6.7582113830639, 6.6692491647808, 6.608575551258, 6.575370853042, 6.5636261253117
, 6.5552529867284, 6.5481329900199, 6.5420641538496, 6.5368896957548, 6.5324846662621, 6.5287472210804, 6.5255926957621, 6.5229494467822
, 6.5207558453278, 6.5189580449086, 6.5175082801585, 6.5163635361022, 6.5154844777905, 6.5148345620932, 6.5143792736196, 6.5140854392019
, 6.5139205823889, 6.5138522820767, 6.5138474980701, 6.513871820562, 6.5138885888526
}
petalAnimData_rotateZ =
{ 47.627745560281, 45.120354168898, 42.460490659866, 39.667537083038, 36.760875488269, 33.759887925413, 30.683956444325
, 27.552463094858, 24.384789926868, 21.200318990207, 18.018432334731, 14.858512010294, 11.73994006675, 8.6820985539526, 5.7043695217571
, 2.8261350200174, 0.066777098587698, -2.5543221926777, -5.0177808039245, -7.3042166852986, -9.3942477869455, -11.268492059011, -12.907567451641
, -14.292091914981, -15.402683399177, -16.219959854375, -16.72453923072, -16.897039478358, -16.73499956312, -16.266210289621, -15.516667366191
, -14.512366501155, -13.279303402841, -11.843473779575, -10.230873339686, -8.4674977914995, -6.579342843343, -4.5924042035437, -2.5326775804286
, -0.42615868232478, 1.7011567824407, 3.8232731055408, 5.9141945786484, 7.9479254934364, 9.8984701415778, 11.739832814746, 13.446017804612
, 14.991029402851, 16.348871901136, 17.493549591138, 18.399066764531, 19.039427712988, 19.388636728181, 19.412500576427, 19.030637365902
, 18.260304314912, 17.141141203097, 15.712787810096, 14.014883915548, 12.087069299092, 9.9689837403677, 7.7002670190143, 5.320558914671
, 2.8694992069769, 0.38672767557153, -2.088115899906, -4.5153917398165, -6.8554600645206, -9.0686810943791, -11.115415049753, -12.956022151002
, -14.550862618488, -15.860296672572, -16.844684533613, -17.464386421974, -17.679762558014, -17.595916820086, -17.312586609398, -16.768684089881
, -15.882512219012, -14.558425836154, -12.730320805904, -10.463906953673, -8.0136382724764, -5.6709030797358, -3.5902988886215, -1.7983290874305
, -0.26687862779245, 1.0439265231311, 2.1708358430879, 3.1441875994885, 3.9882192535047, 4.7221867270648, 5.3614675329375, 5.918457884034
, 6.4032593490595, 6.8241944048765, 7.1881925525024, 7.5010809410244, 7.7678049662481, 7.992597380438, 8.1791092802965, 8.3305126175318
, 8.4495812279634, 8.5387554939702, 8.6001944136782, 8.6358178872568, 8.6473413335809
}

function MassageAnimData( tbl, offset, scale )
	for k,v in ipairs( tbl ) do
		tbl[ k ] = (v + offset) * scale
	end
end

MassageAnimData( petalAnimData_translateX, 0.0, 1 / 30 )
MassageAnimData( petalAnimData_translateY, -6.51, 1 / 40 )
MassageAnimData( petalAnimData_rotateZ, 0.0, 1.0 )

assert( #petalAnimData_translateX == #petalAnimData_translateY )
assert( #petalAnimData_translateX == #petalAnimData_rotateZ )
assert( #petalAnimData_translateY == #petalAnimData_rotateZ )

