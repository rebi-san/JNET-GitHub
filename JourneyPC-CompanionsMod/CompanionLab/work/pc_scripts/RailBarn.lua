-------------------------------------------------------------------------------
-- Notes:  Loads rails from RailInstances.lua
--
-- Author: Rick Nelson
-------------------------------------------------------------------------------

function LoadRails( railBarn )
	for i, desc in ipairs( RailInstances ) do
		local rail = railBarn:CreateRail()

		railBarn:AllocateRailPointStorage( rail, #desc.RailDataPoints )
		railBarn:AllocateRailArcParam( rail )

		RailInitialize( rail, desc )

        --Fill in the name table so triggers and clumps can refer to rails
		Names[ desc.ObjectName ] = rail
	end
end

--Copied from Environment.lua
local function GammaToLinearVec3_Div255( v )
	return GammaToLinearVec3( vec3_scalar_mult( v, 1 / 255 ) )
end

function RailInitialize( rail, desc )
	rail:looped( desc.Closed )

	if desc.RailColor ~= nil then
		rail:color( GammaToLinearVec3_Div255( desc.RailColor ) )
	end
	

	-- fill in the rail point data
	if rail:GetPointCount() == #desc.RailDataPoints then
		for i, node in ipairs( desc.RailDataPoints ) do
			local position = node.Position or node
			rail:SetPoint( i-1, position ) 
		end
	end

	rail:InitArcParam( 32 )
end
