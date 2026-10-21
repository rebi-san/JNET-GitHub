-------------------------------------------------------------------------------
-- Notes:  Contains all script functions for rendering the main scene.
--
-- RenderInitialize()
--     Called once at startup.  You should allocate all render targets and
--     textures in here.
-- 
-- RenderInitializeContext()
--     Called once towards the beginning of every frame.  Lets you set up a
--     RenderContext which is passed to all objects that want to be rendered.
--
-- RenderUpdate()
--     Called once a frame right before actual rendering.  Lets you setup
--     passes and profiles.
--
-- Author: John Edwards
-- Date C: 6-1-2007
-------------------------------------------------------------------------------

function RenderInitialize( ren, resources )
	PrxRenderInitialize( ren, resources )
end

function RenderInitializeContext( ren, resources, renderContext )
	PrxRenderInitializeContext( game, ren, resources, renderContext )
end

function RenderUpdate( ren, resources, renderContext )
	PrxRenderUpdate( ren, resources, renderContext )
end
