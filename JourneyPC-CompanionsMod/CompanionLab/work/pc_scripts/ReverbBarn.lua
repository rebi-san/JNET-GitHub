-------------------------------------------------------------------------------
-- Notes:  Reverb presets
--
-- Author: Martin Middleton
-- Date C: 06-08-2010
-------------------------------------------------------------------------------

-- Data
local reverbNodes =
{
	DefaultSfxReverb =
	{
		azimuth = 0
	,	focus = 180
	,	gain = 1
	,	lfeGain = 0
	,	presetName = "Desert"
	}
,	DefaultMusicReverb =
	{
		azimuth = 0
	,	focus = 180
	,	gain = 1
	,	lfeGain = 0
	,	presetName = "JourneyMusic"
	}	
}

function SetReverbNode( node, nodeName )
	local nodeData = reverbNodes[ nodeName ]
	if nodeData == nil then print( "Error SetReverbNode() can't find "..nodeName ) return end
	node:azimuth( nodeData.azimuth )
	node:focus( nodeData.focus )
	node:gain( nodeData.gain )
	node:lfeGain( nodeData.lfeGain )
	node:SetPresetName( nodeData.presetName )
	node:SetNodeName( nodeName )
end