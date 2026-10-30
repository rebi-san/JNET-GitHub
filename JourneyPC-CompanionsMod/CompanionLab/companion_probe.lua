if _VERSION ~= "Lua 5.3" then
    print("[JNET] unsupported Lua runtime; probe not installed")
    return
end

if rawget(_G, "scarfhelper") ~= nil then
    print("[JNET] probe already loaded; uninstall before reloading")
    return
end

local probe = {}
local rows, total, frames, windowSamples, windows = {}, 0, 0, 0, 0
local LIMIT, WINDOW, MAX_WINDOWS = 120, 15, 12
local episodes, present, lastPositions = { local_slot = 0, remote_slot = 0 }, {}, {}
local lastLevel
local originalTick, wrapper
local enabled, single = false, false
local unpackValues = table.unpack
local function observation(state)
    print("[JNET] observation_state=" .. state)
end

local function call(object, name)
    if object == nil then return nil, "absent" end
    local ok, method = pcall(function() return object[name] end)
    if not ok then return nil, "lookup_error" end
    if method == nil then return nil, "unavailable" end
    -- Native bindings can be callable tables/userdata, not Lua functions.
    local kind = type(method)
    if kind ~= "function" and kind ~= "table" and kind ~= "userdata" then
        return nil, "not_callable"
    end
    local success, result = pcall(function() return method(object) end)
    if not success then return nil, "error" end
    if result == nil then return nil, "absent" end
    return result, "ok"
end

local function position(object)
    local value, status = call(object, "GetPos")
    if status ~= "ok" then return nil, status end
    local kind = type(value)
    if kind ~= "table" and kind ~= "userdata" then return nil, kind end
    local result = {}
    for i = 1, 3 do
        local ok, number = pcall(function() return value[i] end)
        if not ok or type(number) ~= "number" or number ~= number or math.abs(number) > 10000000 then
            return nil, "invalid_components"
        end
        result[i] = number
    end
    return result, kind
end

local function slot(object, label, status)
    if object == nil then
        present[label], lastPositions[label] = nil, nil
        return status
    end
    if not present[label] then episodes[label] = episodes[label] + 1 end
    present[label] = true
    print("[JNET] sample_stage=" .. (label == "local_slot" and "15" or "16"))
    local pos, format = position(object)
    local old = lastPositions[label]
    local movement, step = "unknown", "unknown"
    if pos and old then
        local dx, dy, dz = pos[1] - old[1], pos[2] - old[2], pos[3] - old[3]
        local distance = math.sqrt(dx * dx + dy * dy + dz * dz)
        movement = distance == 0 and "zero_delta" or "nonzero_delta"
        step = string.format("%.6g", distance)
    end
    -- Numeric slot history only: no native wrapper survives this callback.
    lastPositions[label] = pos
    return "episode" .. episodes[label] .. ":" .. movement .. ":pos_" .. format .. ":step_" .. step
end

function probe.stop()
    enabled, single = false, false
    present, lastPositions, lastLevel = {}, {}, nil
    observation(3)
    if wrapper and rawget(_G, "Tick") == wrapper then
        rawset(_G, "Tick", originalTick)
        wrapper, originalTick = nil, nil
        return true
    end
    if wrapper then
        -- Another hook may still call our wrapper; make it a pass-through.
        observation(7)
        return false
    end
    return true
end


function probe.sample(gameObject)
    if total >= LIMIT then
        enabled = false
        probe.stop()
        observation(5)
        return false
    end
    if gameObject == nil then
        print("[JNET] no Tick game argument; sample skipped")
        return false
    end
    print("[JNET] sample_stage=10")
    local gameValue = gameObject
    print("[JNET] sample_stage=11")
    local level = call(gameValue, "GetCurrentLevelNumber")
    if type(level) ~= "number" or level ~= math.floor(level) or level < 0 or level > 255 then
        level = "unknown"
    end
    if lastLevel ~= nil and level ~= lastLevel then
        probe.stop()
        observation(8)
        print("[JNET] chapter changed; observation stopped before traveler getters; rearm after loading")
        return true
    end
    lastLevel = level
    print("[JNET] sample_stage=12")
    local barn, barnStatus = call(gameValue, "playerBarn")
    print("[JNET] sample_stage=13")
    local localDude, localStatus = call(barn, "GetLocalDude")
    print("[JNET] sample_stage=14")
    local remoteDude, remoteStatus = call(barn, "GetRemoteDude")
    if barnStatus ~= "ok" then localStatus, remoteStatus = "unavailable", "unavailable" end
    total, windowSamples = total + 1, windowSamples + 1
    local row = "[JNET] sample=" .. total .. " window=" .. windows .. " level=" .. level .. " barn=" .. barnStatus
        .. " local=" .. slot(localDude, "local_slot", localStatus)
        .. " remote=" .. slot(remoteDude, "remote_slot", remoteStatus)

    print("[JNET] sample_stage=17")
    if #rows == 120 then table.remove(rows, 1) end
    rows[#rows + 1] = row
    print(row)
    if windowSamples == 1 then observation(2) end
    if total >= LIMIT or windowSamples >= WINDOW then
        probe.stop()
        observation(total >= LIMIT and 5 or 4)
        print("[JNET] observation window complete; use --status; rearm with --watch if budget remains")
    end
    print("[JNET] sample_stage=18")
    return true
end

local function arm(oneSample)
    if total >= LIMIT or windows >= MAX_WINDOWS then
        observation(5)
        print("[JNET] session observation budget exhausted; restart Journey for a fresh session")
        return false
    end
    if wrapper then
        print("[JNET] Tick wrapper already present; pause before starting another observation")
        return false
    end
    local tick = rawget(_G, "Tick")
    if type(tick) ~= "function" then
        print("[JNET] Tick is unavailable; no observation installed")
        return false
    end
    windows, windowSamples = windows + 1, 0
    originalTick, enabled, single, frames = tick, true, oneSample, 59
    wrapper = function(...)
        local returned = table.pack(tick(...))
        if enabled then
            frames = frames + 1
            if frames % 60 == 0 then
                -- Do not retain the borrowed game argument beyond this callback.
                local ok, sampled = pcall(probe.sample, select(1, ...))
                if not ok or not sampled then
                    probe.stop()
                    observation(6)
                    print("[JNET] Tick observation failed; sampling stopped; error text omitted")
                elseif single then
                    probe.stop()
                end
            end
        end
        return unpackValues(returned, 1, returned.n)
    end
    rawset(_G, "Tick", wrapper)
    observation(1)
    print("[JNET] observation armed; awaiting Tick game argument")
    return true
end

function probe.start()
    return arm(false)
end

function probe.request()
    return arm(true)
end

function probe.dump()
    for i = 1, #rows do print(rows[i]) end
end

function probe.bindings()
    for _, name in ipairs({ "Game", "PlayerBarn", "Dude", "ConnectionBarn", "Matchmaker", "SynchVarBarn" }) do
        local class = rawget(_G, name)
        local count = 0
        if type(class) == "table" then
            for key, value in next, class do
                count = count + 1
                if count > 256 then break end
                if type(key) == "string" and #key <= 64 and key:match("^[%a_][%w_]*$")
                    and type(value) == "function" then
                    print("[JNET] binding=" .. name .. "." .. key)
                end
            end
        end
        print("[JNET] class=" .. name .. " type=" .. type(class))
    end
end

function probe.uninstall()
    if not probe.stop() then
        print("[JNET] Tick ownership changed; sampling stopped; restart to remove hooks")
        return false
    end
    if rawget(_G, "scarfhelper") == probe then
        rawset(_G, "scarfhelper", nil)
    end
    return true
end

rawset(_G, "scarfhelper", probe)
print("[JNET] observational probe loaded; manual sampling; no private transport")
