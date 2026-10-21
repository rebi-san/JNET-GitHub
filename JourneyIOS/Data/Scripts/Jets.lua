-------------------------------------------------------------------------------
-- Notes:  Loads all Jets in the level. Jets are stored in a
--		   lua table called "JetInstances"
--
-- Author: Nick Clark
-- Date C: 1-11-2010
-------------------------------------------------------------------------------

function LoadJets( JetBarn )
	for i, v in ipairs( JetInstances ) do
		local jetPtr = JetBarn:CreateJet()

		JetInitialize( jetPtr, v )
		
        --for Events/Triggers
		Names[ v.ObjectName ] = jetPtr 

	end
	
	JetInstances = {}
	
end

function JetInitialize( jetPtr, jetData )
	jetPtr:pos( jetData.Position )
	jetPtr:SetPushDir( jetData.Quaternion )
	
	jetPtr:enabled( jetData.Enabled )
	jetPtr:pulseFreq( jetData.PulseFrequency )
	jetPtr:radius( jetData.Radius )
	jetPtr:push( jetData.Push )
	jetPtr:sink( jetData.Sink )
	jetPtr:mass( jetData.Mass )
end