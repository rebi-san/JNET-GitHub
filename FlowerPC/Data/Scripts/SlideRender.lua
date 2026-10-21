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

--local image texture variables
local imageTex_Small


function SlideRenderInitialize( ren, resources )
	imageTex_Small = ren:AddImageTexture( resources, "NoSkybox" )

	pass_SlideShow = ren:AddQuadPass( resources, "SlideShow" )
	pass_SlideShow:colorTarget( nil )
	pass_SlideShow:SetBlend( true, PE_SHADER_BLEND_SRC_ALPHA, PE_SHADER_BLEND_ONE_MINUS_SRC_ALPHA )
	pass_SlideShow:SetPos( -1, -1, 1, 1 )
	pass_SlideShow:SetUv( 0.0, 0.0, 1.0, 1.0 )	
end

function SlideRenderInitializeContext( ren, renderContext )
end

function SlideRenderSelectField(ren,renderContext,field)
end

function SlideRenderUpdate( ren, renderContext )
	-- Cache some values from the render context so we don't have to do as
	-- much typing.
	local cam = renderContext:camera()
	local slideShow = renderContext:slideShow()

    local u0 = 0.0 + slideShow:m_deltaU()
    local v0 = 0.0 + slideShow:m_deltaV()
    local u1 = 1.0 - slideShow:m_deltaU()
    local v1 = 1.0 - slideShow:m_deltaV()

    pass_SlideShow:SetUv( u0, v0, u1, v1 )


	--@HACK: This should happen in C++, not Lua.
    local prevSlide = slideShow:GetPrevSlide()
	local slide = slideShow:GetCurSlide()
	local nextSlide = slideShow:GetNextSlide()

	local gameInt = 0
	local slideInt = 1
    local underlayInt = 1

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
            underlayInt = slideInt;
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
    local underlayTex = imageTex_Small

	if slide then
        slideTex = slide:tex()
        underlayTex = slideTex
    end

    if slide:isOverlay() then underlayTex = prevSlide:tex() end

	local bgColor = { 0, 0, 0, 0 }
	if slide then
		if slideShow:m_ticker() > slide:fadeIn() and nextSlide then
			bgColor = nextSlide:color()
		else
			bgColor = slide:color()
		end
		table.insert( bgColor, 0 )
	end

	pass_SlideShow:GetShader():SetParam4f( "bgColor", bgColor )
	pass_SlideShow:GetShader():SetParamf( "gameInt", gameInt )
	pass_SlideShow:GetShader():SetParamf( "slideInt", slideInt )
    pass_SlideShow:GetShader():SetParamf( "underlayInt", underlayInt )
	pass_SlideShow:GetShader():SetParamf( "stretch", (9.0/16.0) * cam:GetAspect() )
    pass_SlideShow:GetShader():SetParamf( "isOverlay", ( slide:isOverlay() and 1.0 or 0.0 ) );
	pass_SlideShow:GetShader():SetTextureParam( "slideTex", slideTex )
    pass_SlideShow:GetShader():SetTextureParam( "underlayTex", underlayTex )
end

