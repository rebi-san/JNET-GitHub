local output = {}
local env = setmetatable({ print = function(line) output[#output + 1] = line end }, { __index = _G })
env._G = env
local localPos, remotePos = { 1, 2, 3 }, { 4, 5, 6 }
local remotePresent, level, getterCalls = true, 2, 0
local function callable(fn)
    return setmetatable({}, { __call = function(_, ...) return fn(...) end })
end
local wrapperMeta = { __eq = function() error('Equality must not be invoked in conservative mode') end }
local function traveler(slot)
    return setmetatable({ slot = slot, GetPos = callable(function()
        local pos = slot == 'local' and localPos or remotePos
        -- Fresh vector wrapper with indexed components, like a bound vector.
        return setmetatable({}, { __index = pos })
    end) }, wrapperMeta)
end
local barn = {
    GetLocalDude = callable(function() getterCalls = getterCalls + 1; return traveler('local') end),
    GetRemoteDude = callable(function() getterCalls = getterCalls + 1; return remotePresent and traveler('remote') or nil end)
}
local game = { playerBarn = callable(function() return barn end), GetCurrentLevelNumber = callable(function() return level end) }
local ticks = 0
local baseline = function(...)
    assert(select(1, ...) == game)
    ticks = ticks + 1
    return nil, 'preserved', select('#', ...), nil
end
env.Tick = baseline
assert(loadfile('CompanionLab/companion_probe.lua', 't', env))()
local p = env.scarfhelper
local function samples()
    local rows = {}
    for _, line in ipairs(output) do if line:match('^%[JNET%] sample=') then rows[#rows+1] = line end end
    return rows
end
local function latest() local rows = samples(); return rows[#rows] end
assert(p and env.Tick == baseline)
assert(loadfile('CompanionLab/companion_probe.lua', 't', env))()
assert(env.scarfhelper == p and env.Tick == baseline)
env.Game = { GetCurrentLevelNumber = function() return level end }
p.bindings()
assert(table.concat(output, '\n'):match('%[JNET%] binding=Game.GetCurrentLevelNumber'))
env.game = { playerBarn = function() error('wrong global') end }
assert(not p.sample())
assert(p.start())
assert(not p.start())
local values = table.pack(env.Tick(game, 'timing', nil))
assert(values.n == 4 and values[1] == nil and values[2] == 'preserved' and values[3] == 3 and values[4] == nil)
assert(not latest():match('repeat_eq='))
remotePos[1] = 8
for i=1,60 do env.Tick(game) end
assert(latest():match('local=episode1:zero_delta:pos_table:step_0'))
assert(latest():match('remote=episode1:nonzero_delta:pos_table:step_4'))
-- Deliberately tiny drift remains measured, not mislabeled as deliberate motion.
localPos[1] = localPos[1] + 0.000001
for i=1,60 do env.Tick(game) end
local tiny = tonumber(latest():match("local=.-:step_([^ ]+)"))
assert(tiny and tiny > 0.0000009 and tiny < 0.0000011)
assert(not latest():match("repeat_raw="))
-- A 3-4-5 displacement checks the norm, not one chosen coordinate.
localPos[1], localPos[2] = localPos[1] + 3, localPos[2] + 4
for i=1,60 do env.Tick(game) end
assert(tonumber(latest():match("local=.-:step_([^ ]+)")) == 5)
-- Invalid coordinates must not poison future measurements or leak values.
localPos[1] = math.huge
for i=1,60 do env.Tick(game) end
assert(latest():match("pos_invalid_components:step_unknown"))
localPos[1] = 1
for i=1,60 do env.Tick(game) end
assert(latest():match("local=episode1:unknown:pos_table:step_unknown"))
for _, line in ipairs(output) do assert(#line <= 480) end
assert(p.stop() and env.Tick == baseline)
assert(p.request())
local before = #samples()
env.Tick(game)
assert(#samples() == before + 1 and env.Tick == baseline)
-- Original gameplay errors propagate without observation.
env.Tick = function() error('baseline failure') end
assert(p.request())
before = #samples()
local ok, err = pcall(env.Tick, game)
assert(not ok and err:match('baseline failure') and #samples() == before)
assert(p.stop())
env.Tick = baseline
-- Observer exceptions stop observation and leave gameplay return values intact.
local sample = p.sample
p.sample = function() error('private exception') end
assert(p.request())
local a,b,c = env.Tick(game, 'timing', nil)
assert(a == nil and b == 'preserved' and c == 3 and env.Tick == baseline)
assert(not table.concat(output):match('private exception'))
p.sample = sample
-- A chapter change stops before any traveler getters in the new chapter.
assert(p.start())
env.Tick(game)
level = 3
local calls = getterCalls
for i=1,60 do env.Tick(game) end
assert(getterCalls == calls and env.Tick == baseline)
assert(table.concat(output):match('observation_state=8'))
-- Slot presence episodes reset on observed departure; no native refs retained.
assert(p.start())
env.Tick(game)
remotePresent = false
for i=1,60 do env.Tick(game) end
assert(latest():match('remote=absent'))
remotePresent = true
for i=1,60 do env.Tick(game) end
assert(latest():match('remote=episode%d+:unknown'))
local installed = env.Tick
env.Tick = function(...) return installed(...) end
assert(not p.uninstall())
before = #samples()
for i=1,120 do env.Tick(game) end
assert(#samples() == before)
env.Tick = installed
assert(p.stop() and env.Tick == baseline)
-- Windows finish and can be rearmed, while the whole-session cap stays bounded.
local windows = 0
while p.start() do
    windows = windows + 1
    before = #samples()
    for i=1,7201 do env.Tick(game) end
    assert(env.Tick == baseline and #samples() - before <= 15)
    assert(windows <= 12)
end
assert(#samples() <= 120)
local count, retained = #output, math.min(120, #samples())
p.dump()
assert(#output - count == retained)
assert(p.uninstall() and env.scarfhelper == nil)
-- A fresh session reaches the sample cap in eight windows, without invoking __eq.
output = {}
assert(loadfile('CompanionLab/companion_probe.lua', 't', env))()
p = env.scarfhelper
for w=1,8 do
    assert(p.start())
    local n = #samples()
    for i=1,901 do env.Tick(game) end
    assert(#samples() - n == 15 and env.Tick == baseline)
end
assert(#samples() == 120 and not p.start() and not p.sample(game))
assert(#output < 2048)
for _, line in ipairs(output) do assert(#line <= 480) end
assert(table.concat(output):match('sample_stage=15'))
assert(table.concat(output):match('sample_stage=16'))
assert(table.concat(output):match('sample_stage=18'))
assert(p.uninstall())
print('JNET harness passed: short windows, bounded stage output, no equality calls, slot deltas and callback preservation (mocks only).')
