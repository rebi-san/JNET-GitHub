-------------------------------------------------------------------------------
-- Notes:  Loads all Creatures in the level. Creatures are a type of 
--         DynamicNode, and as such are stored in "DynamicNodeInstances" and
--         have a Type value of 'Creature'.
--
-- Author: Nick Clark
-- Date C: 09-24-2010
-------------------------------------------------------------------------------

function CreateCreatures( creatureBarn )
	for i, v in ipairs( DynamicNodeInstances ) do
		if v.Type == "Creature" then
			Names[ v.ObjectName ] = creatureBarn:CreateCreature()
		end
 	end
end

function ResolveCreatureNames( creatureBarn )
	for i, v in ipairs( DynamicNodeInstances ) do
		if v.Type == "Creature" then
			CreatureInitialize( Names[ v.ObjectName ], v )
			Names[ v.ObjectName ]:gardenerName( v.GardenerName )
		end
 	end
end

function CreatureInitialize( creature, desc )
	TriggerInitialize( creature, desc )
	--creature:Initialize()
end	
