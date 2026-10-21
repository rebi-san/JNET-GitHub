-------------------------------------------------------------------------------
-- Notes:  This serves two purposes.  The first is to test out a new immediate-
--         mode GUI system, and the second is to get some quick and dirty text
--         up on screen for the playtest.
--
-- Author: John Edwards
-- Date C: 3-27-2008
-------------------------------------------------------------------------------

HudElements = {}

function HudDoGui( context, Gui )
	-- Sort the elements based on z-depth
	local sortedElems = {}
	for name,elem in pairs( HudElements ) do
		table.insert( sortedElems, { name, elem } )
	end
	table.sort( sortedElems, function( t0, t1 ) return t0[2].pos[3] > t1[2].pos[3] end )

	for i,compoundElem in ipairs( sortedElems ) do
		local name = compoundElem[1]
		local elem = compoundElem[2]

		local pos = { elem.pos[1] / context:aspect(), elem.pos[2] }
		local uvs = { elem.uvs[1], elem.uvs[2], elem.uvs[3], elem.uvs[4] }
		local dim = { elem.dim[1] / context:aspect(), elem.dim[2] }
		local rot = elem.rot

		local alpha = 1.0
		if elem.ticker < elem.fadeIn then
			alpha = elem.ticker / elem.fadeIn
		elseif elem.duration - elem.ticker < elem.fadeOut then
			alpha = (elem.duration - elem.ticker) / elem.fadeOut
		end
		alpha = math.max( 0.0, math.min( 1.0, alpha ) )
		
		if not elem.isPauseElem then
			alpha = math.min( alpha, 1.0 - context:pauseSystemFader() )
		end
		
		local color = { elem.color[1], elem.color[2], elem.color[3], alpha * elem.color[4] }

		local pass = Gui.DoImage( elem.tex, pos, uvs, dim, rot, color )
		pass:GetShader():SetParamf( "Bias", context:aspect() )

		elem.ticker = elem.ticker + context:gameTiming():GetScaledDT()
		if elem.ticker > elem.duration then
			HudElements[ name ] = nil
		end
	end
end

function AddNamedHudElement( name, tex, pos, uvs, dim, color, fadeVals, isPauseElem )
	-- fadeVals = { duration, fadeIn, fadeOut }
	local elem =
	{ tex = tex
	, pos = pos
	, uvs = uvs
	, dim = dim
	, rot = 0
	, color = color
	, duration = fadeVals[ 1 ]
	, fadeIn = math.max( fadeVals[ 2 ], 0 )
	, fadeOut = math.max( fadeVals[ 3 ], 0 )
	, isPauseElem = isPauseElem
	, ticker = 0
	}
	HudElements[ name ] = elem
end

function AddHudElement( tex, pos, uvs, dim, color, fadeVals, isPauseElem )
	-- Put the element in a free slot.
	local key
	while true do
		key = tex..math.random()
		if not HudElements[ key ] then
			AddNamedHudElement( key, tex, pos, uvs, dim, color, fadeVals, isPauseElem )
			break
		end
	end
	return key
end

function RemoveHudElement( name )
	local elem = HudElements[ name ]
	if nil == elem then
		return
	end
	if elem.ticker < (elem.duration - elem.fadeOut) then
		elem.ticker = elem.duration - elem.fadeOut
	end
end

-- This is needed by the loading indicator, but we're too close to shipping to roll
-- it into the old AddHudElement functions.  JCE 1-9-2012
function SetHudElementRot( name, rot )
	local elem = HudElements[ name ]
	if elem then elem.rot = rot end
end
