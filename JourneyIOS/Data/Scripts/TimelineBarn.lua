-------------------------------------------------------------------------------
-- Notes:  Loads timelines into the timebarn
--		   
--
-- Author: Michael Peddicord
-- Date C: 8-16-2010
-------------------------------------------------------------------------------

function LoadTimelines( timelineBarn )
	for i, info in ipairs( TimelineInstances ) do
		local timeline = timelineBarn:CreateTimeline()
		TimelineInitialize( timeline, info )
		Names[ info.ObjectName ] = timeline
	end
end

function TimelineInitialize( timeline, info )
	timeline:Initialize()
	for i, triggerInfo in ipairs( info.Triggers ) do
		timeline:PushTrigger(triggerInfo.TriggerName, triggerInfo.Time)
	end
	timeline:SetLength(info.Length)
	timeline:SetStartTime(info.LoopStart)
	timeline:SetEndTime(info.LoopEnd)
	timeline:SetPauseTime(info.PauseTime)
	if info.NonGracefulLooping == nil then
		info.NonGracefulLooping = false
	end
	timeline:SetLooping(info.Looping, info.NonGracefulLooping)
	if info.Name ~= nil then timeline:SetName(info.Name) end
end