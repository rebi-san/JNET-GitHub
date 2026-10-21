-------------------------------------------------------------------------------
-- Notes:  Contains all script functions for rendering a movie.
--
-- Author: Martin Middleton
-- Date C: 10-24-2007
-------------------------------------------------------------------------------

--pass variables
local pass_Movie
local pass_Background

--local image texture variables
local imageTex_Small
local blankTex

function MovieRenderInitialize( ren, resources )
	imageTex_Small = ren:AddImageTexture( resources, "NoSkybox" )
	blankTex = ren:AddImageTexture( resources, "Black" ) 

	pass_Background = ren:AddQuadPass( resources, "DownSampleSet" )
	pass_Background:colorTarget( nil )
	pass_Background:SetClear( true )
	pass_Background:SetPos( -1.0, -1.0, 1.0, 1.0 )
	pass_Background:SetUv( 0.0, 1.0, 1.0, 0.0 )
	
	pass_Movie = ren:AddQuadPass( resources, "PS4MovieRender" )
	pass_Movie:colorTarget( nil )
	pass_Movie:SetClear( true )
	pass_Movie:SetPos( -1.0, -1.0, 1.0, 1.0 )
	pass_Movie:SetUv( 0.0, 1.0, 1.0, 0.0 )
end

function MovieRenderInitializeContext( ren, renderContext )
end
function MovieRenderSelectField(ren,renderContext,field)
end


function MovieRenderUpdate( ren, renderContext )
	local movieBarn = renderContext:movieBarn()
	local movieTexture = movieBarn:GetMovieTexture()
	local lumaTexture = movieBarn:GetLuminanceTexture()

	-- todo: this presumes full screen
	pass_Movie:SetPos( -1, -1, 1, 1 )

	pass_Background:GetShader():SetTextureParam( "texture", blankTex:GetTexture() ) 
	pass_Background:colorTarget( rt_Tiny )

	if movieTexture then
		pass_Movie:GetShader():SetTextureParam( "chromaTexture", movieTexture ) 
		pass_Movie:colorTarget( nil )
	else
		pass_Movie:GetShader():SetTextureParam( "chromaTexture", imageTex_Small ) 
		pass_Movie:colorTarget( rt_Tiny )
	end

	if lumaTexture then
		pass_Movie:GetShader():SetTextureParam( "lumaTexture", lumaTexture ) 
		pass_Movie:colorTarget( nil )
	else
		pass_Movie:GetShader():SetTextureParam( "lumaTexture", imageTex_Small ) 
		pass_Movie:colorTarget( rt_Tiny )
	end
end

