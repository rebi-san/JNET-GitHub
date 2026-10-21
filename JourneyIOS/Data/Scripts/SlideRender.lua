-------------------------------------------------------------------------------
-- Notes:  Contains all script functions for rendering the slide show.
--         Look at Render.lua for a more complete description of the purpose of
--         this file.
--
-- Author: John Edwards
-- Date C: 10-24-2007
-------------------------------------------------------------------------------

--pass variables
local pass_SlideShow
local pass_SlideShowLeft
local pass_SlideShowRight

--local image texture variables
local imageTex_Small

function SlideRenderInitialize( ren, resources )
	imageTex_Small = ren:AddImageTexture( resources, "NoSkybox" )

	pass_SlideShow = ren:AddQuadPass( resources, "SlideShow" )
	pass_SlideShow:colorTarget( nil )
	pass_SlideShow:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE_MINUS_SRC_ALPHA )
	pass_SlideShow:SetPos( 100100, 100101, 100102, 100103 )
	pass_SlideShow:SetUv( 0.0, 0.0, 1.0, 1.0 )

	pass_SlideShowLeft = ren:AddQuadPass( resources, "SlideShow" )
	pass_SlideShowLeft:colorTarget( nil )
	pass_SlideShowLeft:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE_MINUS_SRC_ALPHA )
	pass_SlideShowLeft:SetPos( 100104, 100101, 100100, 100103 )
	pass_SlideShowLeft:SetUv( 0.0, 0.0, 0.0, 1.0 )

	pass_SlideShowRight = ren:AddQuadPass( resources, "SlideShow" )
	pass_SlideShowRight:colorTarget( nil )
	pass_SlideShowRight:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE_MINUS_SRC_ALPHA )
	pass_SlideShowRight:SetPos( 100102, 100101, 100105, 100103 )
	pass_SlideShowRight:SetUv( 1.0, 0.0, 1.0, 1.0 )
end

function SlideRenderInitializeContext( ren, resources, renderContext )
end

function SlideRenderUpdate( ren, resources, renderContext )
	-- Cache some values from the render context so we don't have to do as
	-- much typing.
	local cam = renderContext:camera()
	local slideShow = renderContext:slideShow()

	--@HACK: This should happen in C++, not Lua.
	local slide = slideShow:GetCurSlide()
	local nextSlide = slideShow:GetNextSlide()

	local gameInt = 0
	local slideInt = 1

	if slideShow:m_curSlideIdx() + 1 == slideShow:m_slideCount() then
		if slide and slideShow:m_ticker() > slide:duration() - slide:fadeOut() then
			local elap = slideShow:m_ticker() - (slide:duration() - slide:fadeOut())
			gameInt = elap / slide:fadeOut()
			if gameInt > 1 then gameInt = 1 end
		end
	end
	if slideShow:m_curSlideIdx() == 0 then
		if slide and slideShow:m_ticker() < slide:fadeIn() then
			gameInt = 1 - slideShow:m_ticker() / slide:fadeIn()
		end
	end

	if slide then
		if slideShow:m_ticker() > slide:duration() - slide:fadeOut() then
			local elap = slideShow:m_ticker() - (slide:duration() - slide:fadeOut())
			slideInt = 1 - elap / slide:fadeOut()
			if slideInt < 0 then slideInt = 0 end
		end
		if slideShow:m_ticker() < slide:fadeIn() then
			slideInt = slideShow:m_ticker() / slide:fadeIn()
		end
	else
		gameInt = 1
		slideInt = 0
	end

    -- write out the current slide intensity so C++ can have it
	slideShow:m_curSlideInt( slideInt )

	local slideTex = imageTex_Small
	local slideTex2 = imageTex_Small
	local slideFade = 0.0
	if slide then
		slideTex = slide:tex()
		slideTex2 = slide:tex2()
		slideFade = slide:texFade()
	end

	local passes = { pass_SlideShow, pass_SlideShowLeft, pass_SlideShowRight }
	for i, pass in ipairs( passes ) do
		pass:GetShader():SetParam4f( "bgColor", {0,0,0,0} )
		pass:GetShader():SetParamf( "gameInt", gameInt )
		pass:GetShader():SetParamf( "slideInt", slideInt )
		pass:GetShader():SetParamf( "stretch", (9.0/16.0) * cam:GetAspect() )
		pass:GetShader():SetTextureParam( "slideTex", slideTex )

		-- to enable fading
		pass:GetShader():SetTextureParam( "slideTex2", slideTex2 )
		pass:GetShader():SetParamf( "slideFade", slideFade )
		
	end
end
