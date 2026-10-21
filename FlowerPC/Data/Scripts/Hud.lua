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

		local alpha = 1.0
		if elem.ticker < elem.fadeIn then
			alpha = elem.ticker / elem.fadeIn
		elseif elem.duration - elem.ticker < elem.fadeOut then
			alpha = (elem.duration - elem.ticker) / elem.fadeOut
		end
		alpha = math.max( 0.0, math.min( 1.0, alpha ) )
		local color = { elem.color[1], elem.color[2], elem.color[3], alpha * elem.color[4] }

		local pass = Gui.DoImage( elem.tex, elem.pos, elem.dim, 0, color )
		pass:GetShader():SetParamf( "Bias", 1 / context:aspect() )

		elem.ticker = elem.ticker + context:gameTiming():GetDT()
		if elem.ticker > elem.duration then
			HudElements[ name ] = nil
		end
	end

end

-- MB:	Changing Gui.lua to add SelectField is complicated, due to it being embedded
--		We lost the ability to have embedded files auto-compile somewhere along the way
function HudSelectField( ren, context, field )
end

function AddHudElement( tex, pos, dim, color, duration, fadeIn, fadeOut )
	local elem =
	{ tex = tex
	, pos = pos
	, dim = dim
	, color = color
	, duration = duration
	, fadeIn = math.max( fadeIn, 0 )
	, fadeOut = math.max( fadeOut, 0 )
	, ticker = 0
	}

	-- Put the element in a free slot.
	while true do
		local key = tex..math.random()
		if not HudElements[ key ] then
			HudElements[ key ] = elem
			break
		end
	end
end

