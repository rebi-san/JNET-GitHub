-------------------------------------------------------------------------------
-- Notes:  Sets terrain data tweak vars and binds textures and texture color
--         channels to terrain attribute streams.
--
-- Author: John Edwards
-- Date C: 6-13-2007
-------------------------------------------------------------------------------
function InitTerrainData( data, resources )
	local height = data:LoadTexture( resources, "Heightmap" )
	local landColor = data:LoadTexture( resources, "LandColor" )
	local grassColor = data:LoadTexture( resources, "GrassColor" )
	local flowerColor = data:LoadTexture( resources, "FlowerColor" )
 
	data:SwizzleCells( 16 ) 
	data:SwizzleGrassDensity( 64 )
      
	-- The numbers 0 through 3 correspond to the texture color
	-- channels b, g, r, a.
	data:SetLandHeight( height, 1 )
	data:SetLandLight( height, 0 )
	data:SetGrassDensity( height, 2 )
	data:SetFlowerDensity( flowerColor, 3 )
	data:SetGrassHeight( height, 3 )
	data:SetLandColor( landColor, 2, 1, 0 )
	data:SetGrassColor( grassColor, 2, 1, 0 )
	data:SetFlowerColor( flowerColor, 2, 1, 0 )

	data:m_gridScale( 1.0 ) 
	data:m_heightScale( 25.0 )
end
 
