-------------------------------------------------------------------------------
-- Notes:  Definitions for flower types
--
-- Author: Martin Middleton
-- Date C: 6-12-2007
-------------------------------------------------------------------------------



-- flower definition is
-- 	,{	flowerName = "Flower Type 1 Name"
--	 ,	petalLayers =
--		{
--			  { meshName = "MeshName1", numPetals = #petals }
--			 ,{ meshName = "MeshName1", numPetals = #petals }
--			  ...
--		}
--	 }


-- the mesh names are the same ones you give the mesh in maya
flowerData =
{

	{	flowerName = "Yellow"
	,	petalLayers =
		{
            { meshName = "PetalYellow", numPetals = 3, numPetalsHigh = 6, offset = { 0, 0, 0 }, offsetUnBloom = { 0, 0, 0 }, texture = "Yellow", randMultBloom = 1.0, randMultUnBloom = 0.0
				,scale = { 0.8, 0.8, 0.8 }, rotBloom = { 2, 45, 0 }, color0 = { 250, 220, 10 }, color1 = { 230, 100, 10 }
				,scaleUnBloom = { 0.8, 0.8, 0.8 }, rotUnBloom = { 2, 45, 0 }, color0UnBloom = { 250, 190, 10 }, color1UnBloom = { 230, 150, 10 } }
			,{ meshName = "PetalYellow", numPetals = 3, numPetalsHigh = 6, scale = 0.7, rot = { -13, 90, 0 }, color0UnBloom = { 250, 190, 10 }, color1UnBloom = { 230, 150, 10 }, color0 = { 250, 200, 10 }, color1 = { 230,100,11 }, texture = "Yellow"  }
			,{ meshName = "Bud_001", numPetals = 5, numPetalsHigh = 8, scale = { 1.4, 2.3, 1.8 }, offset = { 0, 0, -.15 }, offsetUnBloom = { 0, 0, -.15 }, randMultBloom = 1.0, randMultUnBloom = 0.0, rotBloom = { 30, 0, 0 }, rotUnBloom = { 15, 0, 0 }
                                , color0 = { 0, 32, 45 }, color1 = { 205, 205, 40 }, texture = "Bud"  }
			,{ meshName = "Bud_003", numPetals = 1, scale = 1.5, rot = { 0, 0, 0 }, randMultUnBloom = 0, randMultBloom = 0, color0 = {  17, 60, 11 }, color1 = { 250, 150, 0 } }
			,{ meshName = "Banana", numPetals = 3, numPetalsHigh = 6, scale = 0.4, scaleUnBloom = { .1, .1, .1 }, rot = { 0, 0, -25 }, offset = { .02, .1, 0 }, offsetUnBloom = { 0, .3, 0 }, color0 = { 250, 250, 100 }, color1 = { 255, 250, 250 }, texture = "Tendril" }
		}
	,	stemBaseColor = { 10, 23, 2 }
	,	stemTipColor = {  10, 45, 7 }
	,	glowBottomColor = { 255, 80, 0, 155 }
	,	glowTopColor = { 200, 90, 0, 0 }
	,	stemTexture = "StemDefault"
	,	gardener_RenderColor = { 255, 255, 0 }
	,	gardener_Icon = "YellowFlower.bmp"
	,	heightMult = 1.0
	,	outerLayer = { 0, 1 }
	}

	,{	flowerName = "Red"
	,	petalLayers =
		{
			{ meshName = "PetalRed1", numPetals = 16, randMultBloom = 1.0, randMultUnBloom = 0.1, scale = 0.8 , scaleUnBloom = { .5, .8, .8 }, rotUnBloom = { 20, 0, 0 }
                                   , rotBloom = { 17, 0, 10 },  offset = { .2, -.2, 0 }, color0UnBloom = { 100, 0, 0 }
                                   ,color1UnBloom = { 255, 75, 0 }, color0 = { 100, 0, 0 }, color1 = { 255, 75, 0 }, texture = "Red"  }
			,{ meshName = "PetalRed", numPetals = 10, numPetalsHigh = 12, randMultBloom = 1.0, randMultUnBloom = 0.0, scale = { 0.4, 0.6, 0.6 }, scaleUnBloom = { .5,1,1 }
                                   ,rotBloom = { 35, 30, 0 } , rotUnBloom = { 0, 0, 0 }, offset = { 0, -.2, 0 } ,offsetUnBloom = { 0, -.4, .5 }
                                   , color0 = { 50, 0, 0 }, color1 = { 255, 75, 0 }, texture = "Red"  }
			,{ meshName = "Bud_005", numPetals = 11, numPetalsHigh = 13, scale = .6, scaleUnBloom = { 0, 0, 0 }, rotBloom = { 0, 0, 0 }, rotUnBloom = { 0, 0, 0 }
                                   , color0 = { 255, 200, 0 }, color1 = {255, 100, 0 }, offset = { 0, .3, .3 },  offsetUnBloom = { 0, .3, 0 } }

			,{ meshName = "Bud_001", numPetals = 7, numPetalsHigh = 10, scale = { 1.3, 2.3, 1.8 }, offset = { 0, .1, .01 }, offsetUnBloom = { 0, .1, .01 }, randMultBloom = 0.0, randMultUnBloom = 0.0, rotBloom = { 30, 0, 0 }, rotUnBloom = { 4, 0, 0 }
                                , color0 = { 75, 10, 0 }, color1 = { 255, 30, 0 }, texture = "Bud"  }

			,{ meshName = "Bud_003", numPetals = 1, scale = 2, rot = { 0, 0, 0 }, color0 = { 160, 0, 0 }, color1 = { 155, 100, 0} }
		}
	,	stemBaseColor = { 5, 0, 0 }
	,	stemTipColor = { 125, 25, 0 }
	,	glowBottomColor = { 255, 30, 0, 180 }
	,	glowTopColor = { 255, 85, 0, 0}
	,	gardener_RenderColor = { 255, 0, 0 }
	,	gardener_Icon = "RedFlower.bmp"
	,	outerLayer = { 0, 0 }
	}
	
	
	,{	flowerName = "White"
	,	petalLayers =
		{
			{ meshName = "PetalWhite", numPetals = 5, addPetals=false, scale = { .5, .4, .4 }, randMultBloom = 1.0, randMultUnBloom = 0
                                ,scaleUnBloom = { .4, 0.5, 0.5 }, rotBloom = { 0, 0, 0 }, rotUnBloom = { 2, 45, -20 }
                               , offset = { -.05, .0, 0 }, color0 = { 150, 56, 96  }, color1 = {   255, 255, 255   }, texture = "White"  }
			,{ meshName = "Bud_004", numPetals = 5, scale = 1.2, scaleUnBloom = 3 , rotBloom = { 25, 0, 0 }, rotUnBloom = { -20, 20, 15 }
                               ,randMultBloom = 1.0, randMultUnBloom = 0
                               , color0 = { 32, 10, 41 }, color1 = {32, 10, 41 } , offset = { 0, .1 , 0 },  offsetUnBloom = { .2, 0, 0 }
                               ,color0UnBloom = { 132, 110, 141 }, color1UnBloom = { 255, 255, 255 }, texture = "Bud2"  }
			,{ meshName = "Bud_005", numPetals = 5, scale = 1.2, scaleUnBloom = 0, rotBloom = { 0, 0, 0 }, rotUnBloom = { 0, 0, 0 }
                               , color0 = { 12, 50, 87 }, color1 = {12, 10, 41 }, offset = { 0, .2, .3 },  offsetUnBloom = { 0, .3, 0 } }
			,{ meshName = "Bud_003", numPetals = 1, scale = 1.2,  scaleUnBloom = 1.6, rot = { 0, 0, 0 }, color0 = {32, 10, 41}, color1 = { 150, 56, 96 } }

		}
	,	stemBaseColor = { 0, 26, 11 }
	,	stemTipColor = { 30, 0, 30 }
	,	glowBottomColor = { 255, 255, 255, 180 }
	,	glowTopColor = { 255, 255, 255, 0}
	,	gardener_RenderColor = { 255, 255, 255 }
	,	gardener_Icon = "WhiteFlower.bmp"
	,	outerLayer = { 0, 0 }
	}

	,{	flowerName = "Blue"
	,	petalLayers =
		{
			{ meshName = "PetalBlue", numPetals = 3, numPetalsHigh = 6, scale = .8, scaleUnBloom = { 0.3, 0.2, 0.2 }, rotUnBloom = { 20, 0, 0 }, rotBloom = { 45, 0, 0 }, offset = { 0, -.1, -.4 }
                                , offsetUnBloom = { 0, .2, -.2 }, color0 = { 20, 200, 230 }, color1 = { 2, 50, 80 }, randMultBloom = 1.0, randMultUnBloom = 0.0, texture = "Blue"  }
			
			,{ meshName = "PetalBlue", numPetals = 5, numPetalsHigh = 8, scale = .5, scaleUnBloom = { .65, .9, .9 }, rotUnBloom = { 16, 0, 0 }, rotBloom = { -30, 0, 0 }, offset = { 0, -.1, -.1 },  texture = "Blue"
                                    , randMultBloom = 1.0, randMultUnBloom = 0.0, color0 = {0, 0, 40 }, color1 = { 0, 150, 60 }, color0UnBloom = { 0, 75, 100 }, color1UnBloom = { 0, 255, 255 } }

			,{ meshName = "Tendril_003", numPetals = 1, offset = { 0, .4, 0 }, offsetUnBloom = { 0, .4, 0 }, color0 = { 0, 255, 255 }, color1 = { 0, 25, 100 }, texture = "Blue2"
				,scale = .7,  rotBloom = { 0, 0, 0 }
				,scaleUnBloom = .5, rotUnBloom = { 0, 0, 0 }, randMultBloom = 1.0, randMultUnBloom = 0.0, color0UnBloom = { 0, 255, 255 }, color1UnBloom = { 0, 100, 255 } }

			,{ meshName = "PetalBlue", numPetals = 3, numPetalsHigh = 6, scale = { 1.1, .8, .8 }, scaleUnBloom = { .4, .3, .3 }, rotUnBloom = { 20, 0, 0 }, rotBloom = { 5, 60, 0 }, offset = { 0, -.1, -.1 }
                                , offsetUnBloom = { 0, .1, -.2 }, color0 = { 0, 150, 200 }, color1 = { 0, 0, 40 }, randMultBloom = 1.0, randMultUnBloom = 0.0, texture = "Blue"  }

			
			,{ meshName = "Bud_003", numPetals = 1, scale = 1.8, offset = { 0, 0, 0 }, rot = { 0, 0, 0 }
                                    , randMultBloom = 1.0, randMultUnBloom = 0.0, color0 = { 0, 10, 75 }, color1 = { 0, 255, 255 } }

		}
	,	stemBaseColor = { 0, 10, 30 }
	,	stemTipColor = { 0, 75, 200 }
	,	glowBottomColor = { 20, 105, 255, 155 }
	,	glowTopColor = { 20, 205, 255, 0 }
	,	gardener_RenderColor = { 0, 0, 255 }
	,	gardener_Icon = "BlueFlower.bmp"
	,	outerLayer = { 1, 1 }
	}
	
	,{	flowerName = "Pink"
	,	petalLayers =
		{
			{ meshName = "PetalPink", numPetals = 5, numPetalsHigh = 8, scale = 1.6, scaleUnBloom = 2, rotUnBloom = { 25, 360, -10 }, randMultUnBloom = 0, randMultBloom = .10, rotBloom = { 50, 0, 10 }
                                , color0 = { 130, 35, 70 }, color1 = {  255, 120, 175 }, texture = "Pink", offset = { -.1, .1, 0 }, offsetUnBloom = { 0, .1, -.1 } }

			,{ meshName = "PetalBlue", numPetals = 5, numPetalsHigh = 8, randMultUnBloom = 0, randMultBloom = 1.0, rotUnBloom = { -27, 0, 0 }, rotBloom = { -20, 0, 0 },scale = .4, scaleUnBloom = {.4, .4,.4}, offset = { 0, -.1, 0 }
                                , offsetUnBloom = { 0, .1, 0 }, color0UnBloom = { 0, 30, 10 }, color1UnBloom = {10,100,50 } , color0 = { 0, 30, 10 }, color1 = { 10,100,50 }, texture = "Pink2"  }

			,{ meshName = "Bud_001", numPetals = 5, numPetalsHigh = 8, scale = 1.8, scaleUnBloom = 1.8, rotBloom = { 70, 0, 1 }, rotUnBloom = { 80, 0, 1 },offset = { 0, .1, 0 }
                                , offsetUnBloom = { 0, .6, -.23 }, color0 = { 0, 30, 10 }
                                , color1 = { 10,100,50 }, texture = "Pink2"}

                        ,{ meshName = "Banana", numPetals = 5, scale = { .3, .3, .3 }, scaleUnBloom = 0, rotBloom = { 13, 0, -30 }, rotUnBloom = { 0, 0, 0 }
                                , color0 = { 130, 20, 75}, color1 = {255, 255, 255 }, offset = { 0, .1, .06 },  offsetUnBloom = { 0, .3, 0 }, }

			,{ meshName = "Bud_003", numPetals = 1, scale = {1,1.5,1}, rot = { 0, 0, 0 }, color0 = { 0, 30, 10 }, color1 = { 255, 90, 200} }

		}
	,	stemBaseColor = { 0, 25, 5 }
	,	stemTipColor = { 200,75,100 }
	,	glowBottomColor = { 255, 89, 255, 155 }
	,	glowTopColor = { 255, 162, 100, 0 }
	,	gardener_RenderColor = { 255, 100, 100 }
	,	gardener_Icon = "PinkFlower.bmp"
	,	outerLayer = { 0, 1 }
	}
	
	,{	flowerName = "Purple"
	,	petalLayers =
		{
			{ meshName = "PetalPurple", numPetals = 0, scale = {2,1,1}, randMultUnBloom = 0, randMultBloom = .5, offsetUnBloom = { 0, .8,0 }, offset = { 0, 1, 0 }
                                , rotBloom = { -10, 0, 0 }, rotUnBloom = { -2, 0, 0 }, color0UnBloom = {  20, 0, 150 }, color1UnBloom = { 50, 50, 91  }, color0 = { 209, 209, 245 }, color1 = { 56, 50, 91 }, texture = "Purple"  }

            ,{ meshName = "PetalPurple", numPetals = 3, numPetalsHigh = 6, scale = {1.7,1.4,1.7}, scaleUnBloom = {2.5,1.0,1.3,}, randMultUnBloom = 0, randMultBloom = .5, offsetUnBloom = { 0, .4,0 }, offset = { 0, .5, 0 }
                                , rotBloom = { 10, 60, 0 }, rotUnBloom = { -8, 60, 0 }, color0UnBloom = {  0, 12, 30 }, color1UnBloom = { 209, 209, 245 }, color0 = { 209, 209, 245 }, color1 = { 0, 12, 30 }, texture = "Purple"  }

             ,{ meshName = "PetalPurple", numPetals = 3, numPetalsHigh = 6, scale = {1.7,1.4,1.7}, scaleUnBloom = {2.3,1.0,1.3,}, randMultUnBloom = 0, randMultBloom = .5, offsetUnBloom = { 0, .4,0 }, offset = { 0, .5, 0 }
                                , rotBloom = { 20, 0, 0 }, rotUnBloom = { -1, 0, 0 }, color0UnBloom = {  0, 12, 30}, color1UnBloom = { 209, 209, 245 }, color0 = { 209, 209, 245 }, color1 = { 0, 12, 30 }, texture = "Purple"  }
            
			,{ meshName = "BudPurple", numPetals = 1, randMultUnBloom = 0, randMultBloom = 0, scale = {.35,.3,.35}, scaleUnBloom = {.6,.4,.6}, offset = { 0, .12, 0 }
			                    , rot = { 0, 0, 0 }, color0UnBloom = {  60, 60, 120 }, color1UnBloom = { 0, 0, 0 }, color0 = { 0, 0, 0 }, color1 = { 60, 60, 120 }, texture = "Purple2"  }                        
			                    
			,{ meshName = "Bud_003", numPetals = 1, scale = 1, offset = { 0, 0, 0 }, rot = { 0, 0, 0 }, color0 = { 10, 0, 40 }, color1 = { 10, 0, 50 } }
		}
	,	stemBaseColor = { 0, 5, 15 }
	,	stemTipColor = { 42,42,100 }
	,	glowBottomColor = { 111, 109, 255, 255 }
	,	glowTopColor = { 111, 109, 255, 0 }
	,	gardener_RenderColor = { 192, 0, 255 }
	,	gardener_Icon = "PurpleFlower.bmp"
	,	sadness = 1
	,	heightMult = 1.5
	,	outerLayer = { 1, 1 }
	}
	
	,{	flowerName = "Special"
	,	petalLayers =
		{
			{ meshName = "PetalBlue", numPetals = 3, numPetalsHigh = 6, scale = {.5,.5,.5}, scaleUnBloom = {.1,.2,.2,}, offset = { 0, -.2, 0 }, offsetUnBloom = { -.12, .0, 0 }, randMultUnBloom = 0, randMultBloom = 0
				, rotBloom = { 0, 22.5, 0 }, rotUnBloom = { 20, 90, 0 }, color0 = { 255, 255, 255  }, color1 = { 8, 79, 51  }, color0UnBloom = { 255, 255, 255  }, color1UnBloom = { 255, 255, 255  }, texture = "Leaf" }

			,{ meshName = "PetalBlue", numPetals = 3, numPetalsHigh = 6, scale = {.65,.5,.5}, scaleUnBloom = {.1,.1,.1}, offset = { -.1, -.3, 0 }, offsetUnBloom = { -.1, .1, 0 }, randMultUnBloom = 0, randMultBloom = 0
				, rotBloom = { 40, 45, 0 }, rotUnBloom = { 20, 90, 0 }, color0 = { 200, 255, 255  }, color1 = { 8, 79, 51  }, color0UnBloom = { 255, 255, 255  }, color1UnBloom = { 255, 255, 255  }, texture = "Special"}

			,{ meshName = "PetalBlue", numPetals = 3, numPetalsHigh = 6, scale = .2, scaleUnBloom = { .45, .9, 1 }, offset = { 0, -.2, 0 }, randMultUnBloom = .1, randMultBloom = 0
				, rotBloom = { 0, 45, 0 }, rotUnBloom = { 3, 0, 0 }, color0UnBloom = { 200, 255, 255  }, color1UnBloom = { 8, 79, 51  }, color0 = { 200, 255, 255  }, color1 = { 88, 159, 131  }, color0UnBloom = { 200, 255, 255 }, color1UnBloom = { 8, 79, 51  }, texture = "Leaf"}

            ,{ meshName = "PetalBlue", numPetals = 3, numPetalsHigh = 6, scale = .2, scaleUnBloom = { .4, .9, 1 }, offset = { 0, -.2, 0 }, offsetUnBloom = { .1, -.1, 0 }, randMultUnBloom = .1, randMultBloom = 0
				, rotBloom = { 0, 100, 0 }, rotUnBloom = { 6, 65, 0 }, color0UnBloom = { 200, 255, 255  }, color1UnBloom = { 8, 79, 51  }, color0 = { 200, 255, 255  }, color1 = { 88, 159, 131  }, color0UnBloom = { 200, 255, 255 }, color1UnBloom = { 8, 79, 51  }, texture = "Leaf"}

			,{ meshName = "Bud_004", numPetals = 3, numPetalsHigh = 6, scale = 1.9, scaleUnBloom = {.3,.3,.3}, offset = { 0, -.2, -.06 }, offsetUnBloom = { 0, .1, 0 }, randMultUnBloom = 0, randMultBloom = 0
				, rotBloom = { -50, 0, 0 }, rotUnBloom = { 0, 0, 0 }, color0 = { 255, 255, 255 }, color1 = { 255, 255, 255 }, texture = "Special"}

			,{ meshName = "Bud_005", numPetals = 7, numPetalsHigh = 10, scale = .4, scaleUnBloom = .3, offset = { 0, .15, -.13 }, offsetUnBloom = { 0, .15, .03 }, randMultUnBloom = 0, randMultBloom = 0
				, rotBloom = { -20, 0, 0 }, rotUnBloom = { 0, 0, 0 }, color0 = { 255, 255, 255  }, color1 = { 255, 255, 255  }, }
                
            ,{ meshName = "Bud_005", numPetals = 3, numPetalsHigh = 6, scale = .4, scaleUnBloom = .3, offset = { 0, .15, -.06 }, offsetUnBloom = { 0, .15, .03 }, randMultUnBloom = 0, randMultBloom = 0
				, rotBloom = { -20, 0, 0 }, rotUnBloom = { 0, 0, 0 }, color0 = { 255, 255, 255  }, color1 = { 255, 255, 255  }, }
                
            ,{ meshName = "Bud_001", numPetals = 5, numPetalsHigh = 8, scale = 1.1, scaleUnBloom = { 1.4, 2.3, 1.8 }, offset = { 0, -.1, -.05 }, offsetUnBloom = { 0, 0, -.15 }, randMultBloom = 1.0, randMultUnBloom = 0.0, rotBloom = { 50, 0, 0 }, rotUnBloom = { 25, 0, 0 }
                                , color0 = { 0, 32, 45 }, color1 = { 8, 79, 51  }, texture = "Bud"  }

			,{ meshName = "Bud_003", numPetals = 1, scale = 1, scaleUnBloom = 1.5, rotBloom = { 0, 0, 0 }, randMultUnBloom = 0, randMultBloom = 0, rotUnBloom = { 0, 0, 0 }
                               , color0 = { 255, 255, 255 }, color1 = {22, 91, 81 }, color0UnBloom = { 255, 255, 255  }, color1UnBloom = { 255, 255, 255  }, offset = { 0, 0, 0 },  offsetUnBloom = { 0, .15, 0 } }
		}
	,	stemBaseColor = { 8, 79, 51 }
	,	stemTipColor = { 255, 255, 255 }
	,	glowBottomColor = { 100, 255, 225, 150 }
	,	glowTopColor = { 255, 255, 255, 0 }
	,	gardener_RenderColor = { 192, 0, 255 }
	,	gardener_Icon = "SecretFlower.bmp"
	,	sadness = .4
	,	heightMult = 1
	,	outerLayer = { 3, 3 }
	}

 ,{	flowerName = "Wheat"
	,	petalLayers =
		{
			{ meshName = "Wheat", numPetals = 1, scale = 1, rot = { 0,0,0 }, color0 = { 0, 5, 55  }, color1 = { 0, 50, 155  }, texture = "Wheat"  }
			,{ meshName = "Bud_003", numPetals = 1, scale = 0, rot = { 0,0,0 }, color0 = { 0, 5, 55  }, color1 = { 0, 100, 255  }, texture = "StemDefault"  }
  }
	,	stemBaseColor = { 0, 0, 0 }
	,	stemTipColor = { 0, 5, 55 }
	,	glowBottomColor = { 192, 0, 255, 250 }
	,	glowTopColor = { 192, 0, 255, 50 }
	,	gardener_RenderColor = { 0, 255, 0 }
	,	gardener_Icon = "NoFlower.bmp"
	,	heightMult = 2.0
	}
	--[[]]--
}

petalCatalog =
{
	gridWidth = 256
	, atlasWidth = 1024
	, atlasHeight = 2048
	, Default = { 3, 0 }
	, Yellow = { 0, 7 }
	, Red = { 0, 5 }
	, White = { 0, 3 }
	, Blue = { 0, 2 }
	, Blue2 = { 2, 2 }
	, Pink = { 1, 6 }
	, Pink2 = { 2, 6 }
	, Purple = { 0, 4 }
	, Purple2 = { 2, 4 }
	, Tendril = { 1, 0 }
	, Bud = { 0, 0 } 
	, Bud2 = { 0, 1 }
	, StemDefault = { 3, 0 }
	, Wheat = {3,1}
	, Leaf = {1,1}
	, Special = {2,1}
}

flowerHeight = { 0.4, 0.6 }

