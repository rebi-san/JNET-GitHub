-- Press Z to update this file from level data
-- Press spacebar to reload this file

soundData =
{	Emitters = 
	{	SoundEmitter1Node = 
		{	AutoPitch = true
		,	BankName = "LvlSfx"
		,	Enabled = false
		,	Is3D = true
		,	MinDist = 30
		,	ObjectName = "SoundEmitter1Node"
		,	Position = { 127.39, 3.04279, 111.195 }
		,	SoundName = "BuildingRise"
		,	Volume = 1
		}
	,	SoundEmitter2Node = 
		{	AutoPitch = true
		,	BankName = "LvlSfx"
		,	Enabled = false
		,	Is3D = true
		,	MinDist = 30
		,	ObjectName = "SoundEmitter2Node"
		,	Position = { 127.39, 3.04279, 145.922 }
		,	SoundName = "BuildingRise"
		,	Volume = 1
		}
	,	SoundEmitterNode = 
		{	AutoPitch = true
		,	BankName = "SharSfx"
		,	Enabled = false
		,	Is3D = false
		,	MinDist = 10
		,	ObjectName = "SoundEmitterNode"
		,	Position = { 212.141, 8.49291, 41.7514 }
		,	SoundName = "LandingZone_L"
		,	Volume = 1
		}
	}
,	Triggers = 
	{	PlaySFXTrigger1Node = 
		{	ObjectName = "PlaySFXTrigger1Node"
		,	Shortcut = ""
		,	TriggerType = "PlaySFXTrigger"
		,	Vars = 
			{	bankName = "LvlSfx"
			,	soundName = "VineSmash"
			,	vol = 1
			}
		}
	,	PlaySFXTriggerNode = 
		{	ObjectName = "PlaySFXTriggerNode"
		,	Shortcut = ""
		,	TriggerType = "PlaySFXTrigger"
		,	Vars = 
			{	bankName = "LvlSfx"
			,	soundName = "VineSmash"
			,	vol = 1
			}
		}
	}
}