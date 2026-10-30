local creature, decorations, railBarn, levelEvents, assembly, spawn = ...
assert(creature and decorations and railBarn and levelEvents and assembly and spawn)
assert(not __JNETCharacterNative, 'Previous character must be retired first')
local skin = assembly.character == "War Machine" and (__JNETCharacterSkin or 0) or 0
assert(skin >= 0 and skin <= 2)
local objects, events, meshes, markers = {}, {}, {}, {}
local rows = assembly.objects
local names = {}
for name in pairs(rows) do names[#names + 1] = name end
table.sort(names)
local clumps = game:clumpBarn()
local markerBarn = game:markerBarn()
local resources = game:resources()
local root = rows[assembly.root]
assert(root and root.Type == 'Creature')
objects[assembly.root] = creature

local function resolve(value)
    if type(value) ~= 'table' then return value end
    if value.ref then return assert(objects[value.ref], 'Missing private creature reference') end
    local result = {}
    for key, item in pairs(value) do result[key] = resolve(item) end
    return result
end

local function headMarker(name)
    local row = assert(rows[name])
    if row.kind == 'Marker' then return row end
    assert(row.kind == 'Clump', 'Creature head must be a marker or clump')
    for _, child in ipairs(row.Objects) do
        if rows[child].kind == 'Marker' then return rows[child] end
    end
    error('Creature head marker missing')
end

local segments = assert(rows[root.Vars.segments.ref])
local head = headMarker(segments.Objects[1])
local delta = {}
for axis = 1, 3 do delta[axis] = spawn[axis] - head.Position[axis] end
local function position(old)
    return {old[1] + delta[1], old[2] + delta[2], old[3] + delta[3]}
end

for _, name in ipairs(names) do
    local row = rows[name]
    if row.kind == 'Trigger' then
        local event = game:eventBarn():MetaAddEvent(row.Type)
        objects[name] = assert(_G[row.Type].cast(event))
        events[#events + 1] = objects[name]
    end
end

for _, name in ipairs(names) do
    local row = rows[name]
    if row.kind == 'Marker' then
        local desc = resolve(row)
        desc.Position = position(row.Position)
        local marker = markerBarn:CreateMarker()
        MarkerInitialize(marker, desc)
        objects[name] = marker
        markers[#markers + 1] = marker
    elseif row.kind == 'Decoration' then
        local desc = resolve(row)
        for axis = 1, 3 do desc.Transformation[12 + axis] = desc.Transformation[12 + axis] + delta[axis] end
        if row.CoatingParams and skin ~= 0 then
            desc.Shader = "JNETCharacter_MeshSnowTopSide"
            desc.ShaderParams = resolve(row.CoatingParams)
            desc.ShaderParams[#desc.ShaderParams + 1] = {ParamType="texture", ParamName="duneTexture", ParamVal=skin == 2 and "White" or "DuneColorShadow"}
        end
        if row.Mesh == 'JNETCharacter_P_MechHeadGlowMask' then
            for _, param in ipairs(desc.ShaderParams) do
                if param.ParamName == 'alphaFade' then param.ParamVal = 0 end
            end
        end
        local matrix = desc.Transformation
        desc.Transformation = {
            {matrix[1], matrix[2], matrix[3], matrix[4]},
            {matrix[5], matrix[6], matrix[7], matrix[8]},
            {matrix[9], matrix[10], matrix[11], matrix[12]},
            {matrix[13], matrix[14], matrix[15], matrix[16]}
        }
        local mesh = decorations:AddDecoration(resources, desc.Mesh, desc.Shader, desc.Transformation)
        mesh:SetEnabled(false)
        DecorationInitialize(resources, mesh, desc)
        objects[name] = mesh
        meshes[#meshes + 1] = {object = mesh, visible = row.CharacterVisible, shader = row.Shader, params = row.ShaderParams, coating = row.CoatingParams}
    elseif row.kind == 'Clump' then
        objects[name] = clumps:CreateClump()
    elseif row.kind ~= 'DynamicNode' and row.kind ~= 'Trigger' then
        error('Unsupported character object type')
    end
end

for _, name in ipairs(names) do
    local row = rows[name]
    if row.kind == 'Clump' then
        local clump = objects[name]
        clumps:AllocateClumpStorage(clump, #row.Objects)
        for index, child in ipairs(row.Objects) do
            clump:SetAt(index - 1, Class.cast(assert(objects[child])))
        end
    end
end

objects[root.Vars.target.ref]:SetPosition(spawn)
for _, name in ipairs(names) do
    local row = rows[name]
    if row.kind == 'Trigger' or row.kind == 'DynamicNode' then
        local desc = {ObjectName = row.ObjectName, Vars = resolve(row.Vars)}
        TriggerInitialize(objects[name], desc)
    end
end
creature:gardenerName('JNET playable ' .. assembly.character)

local selfClump = clumps:CreateClump()
clumps:AllocateClumpStorage(selfClump, 1)
selfClump:SetAt(0, Class.cast(creature))
local cloth = clumps:CreateClump()
clumps:AllocateClumpStorage(cloth, #meshes)
for index, mesh in ipairs(meshes) do cloth:SetAt(index - 1, Class.cast(mesh.object)) end
local glow = ProtoClothGlow.cast(game:eventBarn():MetaAddEvent('ProtoClothGlow'))
glow:cloth(cloth)
glow:glowMax(1)
glow:glowMin(0)
events[#events + 1] = glow
local ride
if #segments.Objects == 1 then
    ride = ProtoClothMagnetize.cast(game:eventBarn():MetaAddEvent('ProtoClothMagnetize'))
    ride:cloth(cloth)
    ride:alignToDirection(0.5)
    ride:bridgeStiffness(0.5)
    ride:clothStiffness(1)
    ride:magnetizeToPlayer(1)
    ride:snapToCenter(0)
    ride:attractToCloth(1)
else
    ride = CreaB_WhaleRide.cast(game:eventBarn():MetaAddEvent('CreaB_WhaleRide'))
    ride:creatures(selfClump)
    ride:allignMult(1)
    ride:magnetizeMult(1)
    ride:pullMult(1)
    ride:disableHintRegions(nil)
end
events[#events + 1] = ride
if assembly.character == 'Longtail' then
    local grab = CreaB_Rideable.cast(game:eventBarn():MetaAddEvent('CreaB_Rideable'))
    grab:creatures(selfClump)
    grab:dropRegions(nil)
    grab:fxOnPickup(nil)
    grab:fxOnDrop(nil)
    grab:dirSpeedMult(0.25)
    grab:dudeSpeedMult(0.5)
    grab:nearestSegVelMax(2)
    grab:nearestSegVelMult(1)
    grab:rideYOffsetMultHorz(0.25)
    grab:rideYOffsetMultVert(1)
    grab:canJumpOff(false)
    grab:canMoveOff(false)
    grab:disableAfterDrop(false)
    grab:ignoreCreatureOwner(true)
    grab:enforcePickup(true)
    events[#events + 1] = grab
end
local attack, handler
if assembly.attack then
    local targets = clumps:CreateClump()
    clumps:AllocateClumpStorage(targets, 3)
    for index = 0, 2 do
        local marker = markerBarn:CreateMarker()
        MarkerInitialize(marker, {Position = spawn, Quaternion = {0, 0, 0, 1}, Scale = {1, 1, 1}, Enabled = false})
        targets:SetAt(index, Class.cast(marker))
    end
    local rails = {}
    for index, desc in ipairs(assembly.attack.rails) do
        local barn = railBarn
        local rail = barn:CreateRail()
        barn:AllocateRailPointStorage(rail, #desc.RailDataPoints)
        barn:AllocateRailArcParam(rail)
        RailInitialize(rail, desc)
        rails[index] = rail
    end
    attack = Crea_Attack.cast(levelEvents:MetaAddEvent('Crea_Attack'))
    attack:creature(creature)
    attack:guardianEyeSim(objects[assembly.eye])
    attack:attackTargets(targets)
    attack:attackRail(rails[1])
    attack:attackRailRecover(rails[2])
    attack:angryDuration(assembly.attack.angryDuration)
    attack:angrySpeed(assembly.attack.angrySpeed)
    attack:attackSpeed(assembly.attack.attackSpeed)
    attack:attackTargLerp(assembly.attack.attackTargLerp)
    attack:considerShouts(false)
    attack:useHintCam(false)
    local attacks = clumps:CreateClump()
    clumps:AllocateClumpStorage(attacks, 1)
    attacks:SetAt(0, Class.cast(attack))
    handler = Crea_AttackHandler.cast(game:eventBarn():MetaAddEvent('Crea_AttackHandler'))
    handler:creature(creature)
    handler:attackTriggers(attacks)
    events[#events + 1] = handler
end
local native = {
    creature = creature,
    selfClump = selfClump,
    ride = ride,
    target = objects[root.Vars.target.ref],
    head = objects[head.ObjectName],
    eye = assembly.eye and objects[assembly.eye],
    objects = objects,
    meshes = meshes,
    started = false
}
function native:setAppearance(variant)
    if assembly.character == 'Longtail' or assembly.character == 'Whale' then
        assert(variant == 0 or variant == 1, 'Invalid cloth appearance')
        local color = ClothSetColors.cast(game:eventBarn():MetaAddEvent('ClothSetColors'))
        color:meshes(cloth)
        color:duration(0)
        color:playerConversion(false)
        color:disableAfterInvis(false)
        color:toDefault(variant == 0)
        color:toWhite(variant == 1)
        self.colorEvent = color
        return
    end
    assert(assembly.character == 'War Machine' and variant == skin, 'Coating requires its initialized instance')
end
function native:finishAppearance()
    local color = assert(self.colorEvent)
    self.colorEvent = nil
    color:Start()
end
function native:teleport(position)
    self.target:SetPosition(position)
    local event = Crea_Teleport.cast(game:eventBarn():MetaAddEvent('Crea_Teleport'))
    event:creatures(self.selfClump)
    event:pos(self.target)
    event:effectsWhenDone(nil)
    event:network(false)
    self.teleportEvent = event
end
function native:setSpotlight(event, enabled)
    assert(self.eye and event, 'Native eye controller and switch event required')
    event:guardianEyeSim(self.eye)
    event:open(not not enabled)
    event:Start()
end
function native:startEvents()
    assert(not self.started, 'Character events already submitted')
    self.started = true
    local pending = events
    events = nil
    for _, name in ipairs(names) do
        if rows[name].kind == 'Trigger' then objects[name] = nil end
    end
    for index = 1, #pending do
        local event = pending[index]
        pending[index] = false
        event:Start()
    end
end
__JNETCharacterInstances[tostring(__JNETCharacterOwner) .. "|" .. assembly.character .. "|" .. tostring(skin)] = native
__JNETCharacterNative = native
