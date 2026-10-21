-------------------------------------------------------------------------------
-- Notes:  A script for rendering the opening control tutorial.
--
-- Author: Jenova Chen
-- Date C: 6-13-2008
-------------------------------------------------------------------------------

-- Tweakable
local textHoldTime = 0.5
local legalHoldTime = 0.5
local iconFadeSpeed = 0.75
local legalFadeSpeed = 1.5
local saveFadeSpeed = 1.5
local loadFadeSpeed = 1.5
local blackFadeSpeed = 0.4
local tutorialDone = false

local pressed = 0
local timer = 0
local legalTimer = 0

local textA = 0
local textAT = 0

local legalA = 0
local legalAT = 0

local loadA = 0
local loadAT = 0
local loadRot = 0

local blackA = 1
local blackAT = 1




function TutorialDoGui( context, Gui )
	function DoImage( tex, pos, dims, rot, alpha )
		local centerPos = { pos[1], pos[2], pos[3], 0 }
		dims = { dims[1], dims[2] * context:aspect(), 0 }
		pos = vec3_sub( pos, vec3_scalar_mult( dims, 0.5 ) )
		pos[1] = pos[1]
	--	dims[1] = dims[1] / context:aspect()
		local pass = Gui.DoImage( tex, pos, dims, rot, { 1, 1, 1, alpha } )
		pass:GetShader():SetParam4f( "CenterPos", centerPos )
		pass:GetShader():SetParamf( "Bias", 1 / context:aspect() )
        return pass
	end

--DoImage( "Black", { 0, 0, 0 }, { 2, 2 }, 0, blackA )
--DoImage( "Menu_controller_front", { -0.5, 0.0, 0 }, { 0.5, 0.25 }, 0, textA )
--DoImage( "Menu_controller_side", { 0.0, 0.0, 0 }, { 0.25, 0.25 }, 0, textA )
--DoImage( "Menu_controller_top", { 0.5, 0.0, 0 }, { 0.5, 0.25 }, 0, textA )
--DoImage( "Menu_positive", { -0.5, 0.2, 0 }, { 0.15, 0.15 }, 0, textA )
--DoImage( "Menu_positive", { 0, 0.2, 0 }, { 0.15, 0.15 }, 0, textA )
--DoImage( "Menu_negative", { 0.5, 0.2, 0 }, { 0.15, 0.15 }, 0, textA )
--DoImage( "Legal", { 0, -0.41, 0 }, { 1 * 1.3, 0.125 * 1.3 }, 0, legalA )
--DoImage( "LoadingIndicator", { 0, 0.0, 0.0 }, { 0.085, 0.085 }, loadRot, loadA )

-- Swapped round the tutorial images
--DoImage( "Black", { 0, 0, 0 }, { 2, 2 }, 0, blackA )
--DoImage( "Menu_controller_top", { 0.0, 0.0, 0 }, { 1.0, 0.6 }, 0, textA )


--DoImage( "Menu_controller_side", { 0.5, 0.0, 0 }, { 0.375, 0.375 }, 0, textA )
--DoImage( "Menu_controller_front", { 0.6, 0.0, 0 }, { 0.75, 0.375 }, 0, textA )
--DoImage( "Menu_positive", { -0.3, 0.5, 0 }, { 0.25, 0.25 }, 0, textA )
--DoImage( "Menu_positive", { 0.5, 0.5, 0 }, { 0.25, 0.25 }, 0, textA )
--DoImage( "Menu_negative", { 0.6, 0.5, 0 }, { 0.25, 0.25 }, 0, textA )

--local passString = DoImage( "allstrings", { 0, -0.5, 0 }, { 1.3 * 1, 1.3 * 0.0539 }, 0, legalA )
--passString:SetUv( 0, 0.71875, 1, 0.85546875 )

--local passString = DoImage( "allstrings", { 0, -0.8, 0 }, { 1.3 * 1, 1.3 * 0.1125 }, 0, legalA )
--passString:SetUv( 0, 0.44140625, 1, 0.71875 )



--DoImage( "LoadingIndicator", { 0, 0.0, 0.0 }, { 0.085, 0.085 }, loadRot, loadA )

	local pad = context:input():GetPad( 0 )
	pressed = math.max(pressed, CalculateMaxButtonPressure(pad,false))
	dt = context:dt()*(1+pressed*4)
	timer = timer + dt	

	if tutorialDone == false then
		textAT = 1		
		if timer > textHoldTime then
			textAT = 0
			if textA < 0.99 then
				legalTimer = legalTimer + context:dt()
				legalAT = 0.5
				if legalTimer > legalHoldTime then
					legalAT = 0
					timer = 0
					tutorialDone = true
				end
			end
		end
	else
		if context:loading() then
			loadAT = 1
		else
			loadAT = 0
			blackAT = 0
		end

		if legalA < 0.01 and blackA < 0.01 then
			blackA = 0
			legalA = 0
			context:done( true )
		end
	end

	-- update the alpha
	textA = textA + (textAT-textA)*context:dt()*iconFadeSpeed
	legalA = legalA + (legalAT-legalA)*context:dt()*legalFadeSpeed
	loadA = loadA + (loadAT-loadA)*context:dt()*loadFadeSpeed
	blackA = blackA + (blackAT-blackA)*context:dt()*blackFadeSpeed

	-- loading widget just constantly rotates to let the player know the game hasn't crashed
	loadRot = loadRot - 0.15 * math.pi * dt

	-- don't let the player control the menu until it is visible
	context:menuParalysis( math.min( math.max( 2 * (blackA - 0.1), 0 ), 1 ) )
end

