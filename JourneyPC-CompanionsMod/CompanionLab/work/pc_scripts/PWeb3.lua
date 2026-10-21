-------------------------------------------------------------------------------
-- Notes:  Defines several utility functions used by the PWeb3 protocol.
--
--         SpawnEvent:
--         Creates and runs an event in the event barn. Parameters can be 
--         set optionally by passing in a table of key, value pairs for each
--         parameter. For example:
--         SpawnEvent( "DebugOut", { stdoutMessage = "Hello, world!" } )
--
--         PostMetasystem:
--         Sends a version of the current metasystem.lua to the server.  Does
--         *not* do a metadump of the runtime, since that leaks memory.
--
--         HotDate:
--         Receives object updates delivered from Gardener by PWeb3. Responsible
--         for decoding the type of object and calling the corresponding 
--         initialization function. 
--
--         UpdatePWeb3Bindings:
--         Updates the bound device list inside of PWeb3.  Bound devices are
--         other PS3s and computers logged into PWeb3 that this PS3 can send
--         commands to.
--
-- Author: Nick Clark and John Edwards
-- Date C: 10-10-2009
-------------------------------------------------------------------------------
function SpawnEvent( eventTable )
	local eventBarn = _G[ "game" ]:eventBarn()
	
	for eventType, eventParams in pairs( eventTable ) do		
		if _G[ eventType ] == nil then
			print( "SpawnEvent(): invalid event type '"..eventType.."'\n" )
			return
		end
	
		print( "Spawning event '"..eventType.."'" )
		
		local event = eventBarn:MetaAddEvent( eventType )
		local eventImplPtr = _G[ eventType ].cast( event )
	
		for key,val in pairs( eventParams ) do
			if eventImplPtr[ key ] == nil then
				print( "SpawnEvent(): invalid param '"..key.."' for event '"..eventType.."'" )
			else
				eventImplPtr[ key ]( eventImplPtr, val )
			end
		end
		
		eventImplPtr:Start()
	end	
end

function PostMetasystem()
	dofile( "/app_home/Metasystem.lua" )

	local stringBuilder = { "{ " }
	local comma = ""
	for name, class in pairs( Metasystem ) do
		local fieldArray = {}
		for fieldName, field in pairs( class.vars ) do
			table.insert( fieldArray, '"'..fieldName..'"' )
		end
		table.insert( stringBuilder, comma..'"'..name..'" : ['..table.concat( fieldArray, ',' )..']' )
		comma = ", "
	end
	table.insert( stringBuilder, " }" )

	game:pweb3():PostMetasystem( table.concat( stringBuilder ) )
end

function ActivateTriggerByName( triggerName )
	print( "Trying to activiate" .. triggerName )
	Names[ triggerName ]:Start()
end

function ReturnLobbyFlagNames( uid )
	--[[
	local lobbyFlagNameCollector = {}
	for i = 0, 31 do
		local flagName = game:lobby():GetFlagName( i )
		if flagName ~= "" then
			lobbyFlagNameCollector[ #lobbyFlagNameCollector + 1 ] = flagName;
		end
	end
	]]--
	
	local selectionInfoData = { 
		["psnname"] = LuaGetPsnName(),
		["lobbyFlagNames"] = lobbyFlagNameCollector
	}
	
	local succeed, jString = pcall( Json.Encode, selectionInfoData )
	if succeed then
		game:pweb3():SendJson( uid, jString )
	else
		print( "Failed to send lobby flag names" );
	end
end

function SendBackCamPosForEnvCreation()
	local curCamKeyFrame = game:cameraSystem():GetBlendState()
	local pos = curCamKeyFrame:pos()

	local strpos = string.format("%.6f", pos[1])..","..string.format("%.6f", pos[2])..","..string.format("%.6f", pos[3])

	local gardenerCmd = "Mel( [[ EnvCreateNewLerpedNode( {"..strpos.."} ); ]] )"

	for i = 0, game:pweb3():GetBoundDeviceCount() - 1 do
		local device = game:pweb3():GetBoundDevice( i )
		if device:GetType() == "gardener" then
			game:pweb3():Send( device:GetUniqueId(), gardenerCmd )
		end
	end
end

function SetNewCamPosition( rot, pos, fov, lvlName )
	print(game:GetCurrentLevelString())
	local fullName = "Level_" .. lvlName;
	if fullName == game:GetCurrentLevelString() then	
		game:cameraSystem():GetDebugCam():SetXForm( rot, pos, fov )
	end
end

local function HookUpPointers( objTable )
	for k,v in pairs( objTable ) do
		if "table" == type( v ) then 
			if v.NamesTablePtr then objTable[ k ] = Names[ v.NamesTablePtr ]
			else HookUpPointers( v ) end
		end
	end
end

-- For when we are moving around stuff, we don't want to repopulate the entire object
function HotDateTransform( objectType, objectTable, object )
	SetObjectMatrix( object, objectTable.Transformation )	
end

function HotDate( objectType, objectName, objectTable, levelName )
	-- Make sure the update we're getting is for the level we're currently on.
	local curLevel = game:GetCurrentLevelString()	
	if levelName ~= curLevel then return end
	
	-- Make sure the specified object still exists.
	local object = Names[ objectName ]
	if not object then return end
	
	if objectTable.partial then
		HotDateTransform( objectType, objectTable, object )
		return
	end
	
	-- We need to hook up the pointers in the objectTable
	HookUpPointers( objectTable )

	--PrintTable( objectTable )
		
	-- Call the correct reinitialize routine.
	if "CameraKeyFrame" == objectType then
		CameraKeyFrameInitialize( game:cameraSystem(), object, objectTable )
	elseif "EnvNode" == objectType then
		EnvNodeInitialize( object, objectTable )
	elseif "Hull" == objectType then
		-- Hull are special in the Names table, in that each entry can actually be a table containing multiple Hulls.
		if "table" == type( object ) then
			for hullIdx, hull in ipairs( object ) do
				HullReinitialize( hull, objectTable )
			end
		else
			HullReinitialize( object, objectTable )
		end
	elseif "Marker" == objectType then
		MarkerInitialize( object, objectTable )
	elseif "MeshNode" == objectType then
		DecorationInitialize( game:resources(), object, objectTable )
	elseif "ParticleEmitter" == objectType then
		ParticleEmitterInitialize( object, objectTable )
	elseif "Rail" == objectType then
		RailInitialize( object, objectTable )
	elseif "SoundEmitter" == objectType then
		SoundEmitterInitialize( object, objectTable )
	elseif "Timeline" == objectType then
		TimelineInitialize( object, objectTable )
	elseif "Trigger" == objectType then
		TriggerInitialize( object, objectTable )
	elseif "Sign" == objectType then
		SignInitialize( object, objectTable )
	elseif "Jet" == objectType then
		JetInitialize( object, objectTable )
	end
end

function UpdatePWeb3Bindings( hash, jsonBindings )
	game:pweb3():ClearBoundDevices()
	game:pweb3():m_boundDevicesHash( hash )

	local bindings = Json.Decode( jsonBindings )
	for i, b in ipairs( bindings ) do
		game:pweb3():AddBoundDevice( b.unique_id, b.type, b.name, b.hot_date )
	end
end

function RemoteFunHandler( argsTxt )
	--print( "received "..argsTxt )
	local args = Json.Decode( argsTxt )

	local function ReturnFun( data )
		local retData = { cbId = args.cbId, data = data }
		local retTxt = Json.Encode( retData )
		--print( "returning "..retTxt )
		game:pweb3():Send( args.returnId, retTxt )
	end
	
	local fun = _G[ args.funName ]
	if fun == nil then 
		ReturnFun( "ERROR: No function named "..args.funName )
		return
	end
	local funArgs = args.funArgs;
	
	local retData = fun( funArgs )
	
	ReturnFun( retData )
end

function AddBookmark()
	local keyframe = game:cameraSystem():GetBlendState()
	local pos = keyframe:pos()
	local rot = keyframe:rot()
	local fov = keyframe:fov()
	local levelName = game:GetCurrentLevelString()
	levelName = string.gsub( levelName, "Level_", "" )
	return { pos = pos, rot = rot, fov = fov, levelName = levelName }
end

function GoToBookmark( data )
	function moveCameraToBookmark()
		local cs = game:cameraSystem()
		if not cs:UsingDebugCam() then cs:StartDebugCam() end
		SetNewCamPosition( data.rot, data.pos, data.fov, data.levelName )
		if data.player == true then
			game:playerBarn():GetLocalDude():DebugTeleport( data.pos, cs )
		end
	end
	
	local fullName = "Level_"..data.levelName
	if data.forceLevelRestart or fullName ~= game:GetCurrentLevelString() then 
		game:QueueLevel( fullName, true, true )
		AddLevelLoadFun( moveCameraToBookmark )
	else
		moveCameraToBookmark() 
	end
end

function GetSaveData()
	local saveData = game:saveData()
	local retData = {}
	retData.levelData = {}
	for i = 0,12 do
		local levelData = saveData:GetLevelData( i )
		local levelDataLua = { timeSpentWithPartner = levelData:timeSpentWithPartner() }
		table.insert( retData.levelData, levelDataLua )
	end
	return retData	
end

function UpdateSaveData( data )
	local saveData = game:saveData()
	for i = 0,12 do
		local levelData = saveData:GetLevelData( i )
		levelData:timeSpentWithPartner( data.levelData[ i + 1 ].timeSpentWithPartner )
	end
	game:OnLoadFromFile()
end