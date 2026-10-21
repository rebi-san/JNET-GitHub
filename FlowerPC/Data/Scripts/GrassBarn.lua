-------------------------------------------------------------------------------
-- Notes:  Function for initializing tweak vars in the grass barn.
--
-- Author: John Edwards
-- Date C: 12-12-2007
-------------------------------------------------------------------------------

function InitializeGrassBarn( resources, grassBarn )
	local high = grassBarn:AddGrassPass( resources, "GrassBlade" )
	high:type( GrassType_Blade )
	high:maxGrass( 128 * 1024 )
	high:maxGenJobs( 384 )
	high:cellSize( 1.85 )
	high:dither( 1.0 )
	high:skew( 0.4 )
	high:widthMult( 1.2 )
	high:heightMult( 1.0 )
	high:windMult( 1.0 )
	high:additive( false )
	high:alphaToDither( true )

	local low = grassBarn:AddGrassPass( resources, "GrassClump" )
	low:type( GrassType_Debris )
	low:maxGrass( 76 * 1024 )
	low:maxGenJobs( 384 )
	low:cellSize( 1.333 )
	low:dither( 1.0 )
	low:skew( 4.0 )
	low:widthMult( 25.0 )
	low:heightMult( 1.0 )
	low:windMult( 0.1 )
	low:additive( false )
	low:alphaToDither( true )

	local fill = grassBarn:AddGrassPass( resources, "FillerFlower" )
	fill:type( GrassType_Filler )
	fill:maxGrass( 96 * 1024 )
	fill:maxGenJobs( 384 )
	fill:cellSize( 3.333 )
	fill:dither( 1.0 )
	fill:skew( 0.4 )
	fill:widthMult( 0.0 )
	fill:heightMult( 1.0 )
	fill:windMult( 1.0 )
	fill:additive( true )
	fill:alphaToDither( false )
end

