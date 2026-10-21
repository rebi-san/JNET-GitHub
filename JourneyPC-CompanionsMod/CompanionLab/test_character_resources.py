import unittest
import json
import io
import tarfile
import re
from unittest.mock import patch
from pathlib import Path

from character_resources import (CHARACTERS, CHARACTER_IDS, LiteralReader, character_assembly,
                                 creature_graph, members, relocate_assembly,
                                 character_resources, character_resource_name, character_visual_assembly, lua_literal,
                                 character_asset_map_header, character_runtime_header, character_resource_script,
                                 character_animation_bundles, character_animation_bundle_header, character_animation_namespace,
                                 character_animation_files, character_animation_file_header, character_mesh_files)


class CharacterResourcesTest(unittest.TestCase):
    def test_flow_ancestors_preserve_ruins_assemblies(self):
        from character_resources import CHARACTER_IDS
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        resources = character_resources(image, participants=3)
        for character, root, prefix, count, identifier in (
                ('fl0w 1', 'a1d05a9dcd548ccf82c7baf437c17e09', 'P_FlowCreature_', 9, 5),
                ('fl0w 2', '017c4db3317950fb9cfc77f97f298a14', 'P_FlowHunter_', 10, 6)):
            with self.subTest(character=character):
                self.assertEqual(CHARACTERS[character], ('Level_Ruins', root))
                self.assertEqual(CHARACTER_IDS[character], identifier)
                native = creature_graph('Level_Ruins', root)
                assembly = character_visual_assembly(character, resources)['objects']
                self.assertEqual(set(assembly), set(native))
                meshes = [row for row in assembly.values() if row['kind'] == 'Decoration']
                self.assertEqual(len(meshes), count)
                self.assertTrue(all(row['CharacterVisible'] for row in meshes))
                for name, row in assembly.items():
                    original = native[name]
                    if row['kind'] == 'Decoration':
                        self.assertTrue(original['Mesh'].startswith(prefix))
                        self.assertEqual(row['Mesh'], 'JNETCharacter_' + original['Mesh'])
                        self.assertEqual(row['Shader'], 'JNETCharacter_MeshCham')
                        self.assertEqual(row['Transformation'], original['Transformation'])
                        self.assertEqual(row['ShaderParams'], original['ShaderParams'])
                    else:
                        self.assertEqual(row, original)
        header = character_runtime_header(image)
        self.assertIn('jnet_character_5_source', header)
        self.assertIn('jnet_character_6_source', header)
        self.assertNotIn('jnet_character_4_source', header)

    def test_three_participant_capacity_and_crowded_level_headroom(self):
        from character_resources import decorations, records
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        single = character_resources(image)
        triple = character_resources(image, participants=3)
        for key, desc in single.items():
            if key[0] in ('Mesh', 'Shader'):
                self.assertEqual(triple[key]['instanceLimit'], desc['instanceLimit'] * 3)
        private = 16 + 3 * sum((3 if c == 'War Machine' else 1) * sum(row['kind'] == 'Decoration' for row in
                                 character_visual_assembly(c, triple)['objects'].values()) for c in CHARACTERS)
        self.assertLessEqual(private, 1024)
        for suffix in ('Graveyard', 'Barrens', 'Desert', 'Canyon', 'Cave', 'Ruins',
                       'Mountain', 'Summit', 'Credits', 'Bryan', 'Matt', 'Chris'):
            level = 'Level_' + suffix
            self.assertLessEqual(len(decorations(level)) + 3, 1024)
            native_creatures = sum(row.get('Type') == 'Creature' for row in records(level, 'DynamicNode').values())
            self.assertLessEqual(native_creatures + 3 * (len(CHARACTERS) + 2), 112)
            private_markers = 3 * sum((3 if c == 'War Machine' else 1) * sum(row['kind'] == 'Marker' for row in
                                        character_visual_assembly(c, triple)['objects'].values()) for c in CHARACTERS)
            self.assertLessEqual(len(records(level, 'Marker')) + private_markers, 1024)
        script = character_resource_script(image, participants=3).decode()
        self.assertIn('resource "Mesh"', script)
        self.assertEqual(script.count('resource "ScreamBank"'), 3)
        self.assertIn('resource "ScreamBank" "JNTFlow"', script)

    def test_sound_bank_capacity_rejects_startup_crash_regression(self):
        from character_resources import character_sound_bank_script, CHARACTER_SOUND_BANKS
        from inspect_build import pe_sections
        from native_map import offset_for
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        script = character_sound_bank_script(image)
        self.assertEqual(script.count('resource "ScreamBank"'), 3)
        with self.assertRaisesRegex(ValueError, 'capacity exceeded'):
            character_sound_bank_script(image, CHARACTER_SOUND_BANKS + tuple(('Extra'+str(i), 'Ruins-SFX.bank') for i in range(3)))
        sections = pe_sections(image)
        at = offset_for(sections, 0x8ede0, 3)
        self.assertEqual(image[at:at+3], bytes.fromhex('488b01'))
        at = offset_for(sections, 0x2b783a, 9)
        self.assertEqual(image[at:at+9], bytes.fromhex('488d4818e89d75ddff'))

    def test_flow_chirp_aliases_preserve_original_creature_calls(self):
        import ast
        from lupa import LuaRuntime
        source = (Path(__file__).resolve().parent/'diagnostic/jnet_freeplay_live.h').read_text()
        chunks = source.split('static const char *const chunks[]={')[1].split('    };')[0]
        script = ''.join(ast.literal_eval(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', chunks))
        lua = LuaRuntime()
        lua.execute('soundEffectData={PlayerCoo={bankName="LvlSfx",soundName="PlayerCoo",minDist=9,autoPitchBend=false}}')
        lua.execute(script + ' __JNETApplySounds() __JNETApplySounds()')
        rows = lua.globals().soundEffectData
        self.assertEqual(len(list(rows.items())), 12)
        for name,call in (('JNTFlowSnake','FlowSnakeCall'),('JNTFlowHunter','FlowHunterCall')):
            self.assertEqual(rows[name].bankName, 'JNTFlow')
            self.assertEqual(rows[name].soundName, call)
        self.assertTrue(lua.eval('soundEffectData.JNTFlowSnake~=soundEffectData.PlayerCoo'))
        self.assertTrue(lua.eval('soundEffectData.JNTFlowHunter~=soundEffectData.JNTFlowSnake'))
        self.assertEqual(rows.JNTWhale.bankName, 'JNTBio')
        self.assertEqual(rows.JNTWarVoice.bankName, 'JNTMech')

    def test_factory_passes_native_vector_tables_and_preserves_spawn_transform(self):
        from lupa import LuaRuntime
        lab = Path(__file__).resolve().parent
        image = (lab.parent.parent / 'JourneyPC/Journey.exe').read_bytes()
        resources = character_resources(image)
        factory = (lab / 'diagnostic/jnet_character_factory.lua').read_text()
        header = character_runtime_header(image)
        for character, skin in [(c, s) for c in CHARACTERS for s in (range(3) if c == "War Machine" else (0,))]:
            with self.subTest(character=character, skin=skin):
                assembly = character_visual_assembly(character, resources)
                spawn = [123.25, 604.5, -78.75]
                moved = relocate_assembly(assembly, spawn)
                expected = [row['Transformation'] for _, row in sorted(moved['objects'].items())
                            if row['kind'] == 'Decoration']
                lua = LuaRuntime()
                lua.execute('''
                    created, initialized, rides, glows, rideValues, colors, materials, attacks = {}, {}, {}, {}, {}, {}, {}, {}
                    eventCount, persistentCount = 0, 0
                    local function matrixCopy(matrix)
                        assert(#matrix == 4, 'Expected four vector tables')
                        local copy = {}
                        for i = 1, 4 do
                            assert(type(matrix[i]) == 'table' and #matrix[i] == 4,
                                'Expected a vector table')
                            for j = 1, 4 do copy[#copy + 1] = matrix[i][j] end
                        end
                        return copy
                    end
                    local object
                    object = function()
                        return setmetatable({}, {__index = function() return function() return object() end end})
                    end
                    creature, decorations, game, railBarn = object(), object(), {}, object()
                    function game:clumpBarn() return object() end
                    function game:markerBarn() return object() end
                    function game:resources() return object() end
                    function decorations:AddDecoration(resources, mesh, shader, matrix)
                        created[#created + 1] = matrixCopy(matrix)
                        local result = object()
                        result.material = {shader = shader, params = {}, changes = 0}
                        materials[#materials + 1] = result.material
                        function result:SetShader(_, value)
                            self.material.shader = value
                            self.material.params = {}
                            self.material.changes = self.material.changes + 1
                        end
                        function result:SetShaderTextureParam(_, name, value) self.material.params[name] = value end
                        function result:SetShaderParamf(name, value) self.material.params[name] = value end
                        function result:SetShaderParam4f(name, value) self.material.params[name] = value end
                        return result
                    end
                    function DecorationInitialize(resources, mesh, desc)
                        initialized[#initialized + 1] = matrixCopy(desc.Transformation)
                        for _, p in ipairs(desc.ShaderParams) do mesh.material.params[p.ParamName] = p.ParamVal end
                    end
                    function game:eventBarn()
                        return {MetaAddEvent = function(_, kind)
                            eventCount = eventCount + 1
                            local event = object()
                            if kind == 'Crea_Attack' or kind == 'Crea_AttackHandler' then
                                attacks[kind] = event
                                event.starts = 0
                                for _, field in ipairs({'creature','guardianEyeSim','attackTargets','attackRail','attackRailRecover','considerShouts','useHintCam','attackTriggers'}) do
                                    event[field] = function(self, value) self[field .. 'Value'] = value end
                                end
                                function event:Start() self.starts = self.starts + 1 end
                            end
                            if kind == 'ClothSetColors' then
                                colors[#colors + 1] = event
                                for _, field in ipairs({'meshes','duration','toWhite','toDefault','playerConversion','disableAfterInvis'}) do
                                    event[field] = function(self, value) assert(not rawget(self, 'expired'), 'Expired colour event'); self[field .. 'Value'] = value end
                                end
                                function event:Start() assert(not rawget(self, 'expired'), 'Expired colour event'); self.starts = (rawget(self, 'starts') or 0) + 1; self.expired = true end
                            end
                            if kind == 'ProtoClothGlow' then
                                glows[#glows + 1] = event
                                function event:glowMax(value) self.maximum = value end
                                function event:glowMin(value) self.minimum = value end
                            end
                            if kind == 'CreaB_WhaleRide' or kind == 'ProtoClothMagnetize' then
                                rides[#rides + 1] = kind
                                for _, field in ipairs({'allignMult','magnetizeMult','pullMult','attractToCloth'}) do
                                    event[field] = function(_, value) rideValues[field] = value end
                                end
                            end
                            return event
                        end}
                    end
                    levelEvents = {MetaAddEvent = function(_, kind)
                        local event = game:eventBarn():MetaAddEvent(kind)
                        eventCount = eventCount - 1
                        persistentCount = persistentCount + 1
                        return event
                    end}
                    function RailInitialize(rail, desc) assert(#desc.RailDataPoints >= 2) end
                    function MarkerInitialize() end
                    function TriggerInitialize() end
                    Class = {cast = function(value) return value end}
                    CreaB_WhaleRide, ProtoClothMagnetize, ProtoClothGlow, ClothSetColors = Class, Class, Class, Class
                    Crea_Attack, Crea_AttackHandler = Class, Class
                    __JNETCharacterInstances = {}
                ''')
                for row in assembly['objects'].values():
                    if row['kind'] == 'Trigger':
                        lua.globals()[row['Type']] = lua.globals().Class
                lua.globals()['__JNETCharacterSkin'] = skin
                lua.globals()['__JNETCharacterOwner'] = 0
                lua.execute(factory, lua.globals().creature, lua.globals().decorations, lua.globals().railBarn, lua.globals().levelEvents,
                            lua.execute('return ' + lua_literal(assembly)), lua.table_from(spawn))
                self.assertTrue(lua.execute('return __JNETCharacterInstances[...] == __JNETCharacterNative', f'0|{character}|{skin}'))
                budget = re.search(r'\{jnet_character_' + str(CHARACTER_IDS[character]) + r'_source,\{([0-9,]+)\}', header)
                self.assertEqual(lua.globals().eventCount, int(budget[1].split(',')[3]))
                self.assertEqual(lua.globals().persistentCount, int(character == 'War Machine'))
                self.assertEqual(list(lua.globals().rides.values()),
                                 ['ProtoClothMagnetize','CreaB_WhaleRide'] if character == 'Longtail' else ['CreaB_WhaleRide'])
                values = dict(lua.globals().rideValues.items())
                self.assertEqual(values, {'attractToCloth': 1, 'allignMult': 1, 'magnetizeMult': 1, 'pullMult': 1} if character == 'Longtail' else
                                 {'allignMult': 1, 'magnetizeMult': 1, 'pullMult': 1})
                if character in ('Longtail', 'Whale'):
                    self.assertEqual(len(lua.globals().colors), 0)
                    for index, variant in enumerate((1, 0, 1), 1):
                        lua.execute('__JNETCharacterNative:setAppearance(...)', variant)
                        color = lua.globals().colors[index]
                        self.assertIsNone(lua.eval("rawget")(color, "starts"))
                        lua.execute('__JNETCharacterNative:finishAppearance()')
                        self.assertEqual(color.starts, 1)
                        self.assertEqual(color.toWhiteValue, bool(variant))
                        self.assertEqual(color.toDefaultValue, not variant)
                        self.assertFalse(color.playerConversionValue)
                    self.assertEqual(len(lua.globals().colors), 3)
                else:
                    self.assertEqual(len(lua.globals().colors), 0)
                if character == 'War Machine':
                    native_rows = [r for _, r in sorted(assembly['objects'].items()) if r['kind'] == 'Decoration']
                    lua.execute('__JNETCharacterNative:setAppearance(...)', skin)
                    ok, _ = lua.execute('local variant=...; return pcall(function() __JNETCharacterNative:setAppearance(variant) end)', (skin + 1) % 3)
                    self.assertFalse(ok)
                    for index, row in enumerate(native_rows, 1):
                        material = lua.globals().materials[index]
                        self.assertEqual(material.changes, 0)
                        coated = 'CoatingParams' in row and skin != 0
                        self.assertEqual(material.shader, 'JNETCharacter_MeshSnowTopSide' if coated else row['Shader'])
                        if coated:
                            self.assertEqual(material.params.duneTexture, 'White' if skin == 2 else 'DuneColorShadow')
                        params = row['CoatingParams'] if coated else row['ShaderParams']
                        for param in params:
                            actual = material.params[param['ParamName']]
                            expected_value = param['ParamVal']
                            if param['ParamType'] == 'float4':
                                actual = list(actual.values())
                                expected_value = list(expected_value)
                            self.assertEqual(actual, expected_value)
                if character == 'War Machine':
                    attack = lua.globals().attacks.Crea_Attack
                    handler = lua.globals().attacks.Crea_AttackHandler
                    self.assertFalse(attack.considerShoutsValue)
                    self.assertFalse(attack.useHintCamValue)
                    self.assertEqual(attack.starts, 0)
                    self.assertEqual(handler.starts, 0)
                    lua.execute('__JNETCharacterNative:startEvents()')
                    self.assertEqual(attack.starts, 0)
                    self.assertEqual(handler.starts, 1)
                else:
                    self.assertEqual(len(list(lua.globals().attacks.items())), 0)
                self.assertEqual(len(lua.globals().glows), 1)
                self.assertEqual(lua.globals().glows[1].maximum, 1)
                self.assertEqual(lua.globals().glows[1].minimum, 0)
                for field in ('created', 'initialized'):
                    actual = lua.globals()[field]
                    self.assertEqual(len(actual), len(expected))
                    for index, matrix in enumerate(expected, 1):
                        self.assertEqual(list(actual[index].values()), list(matrix))

    def test_private_meshes_isolate_every_asset_name_without_changing_geometry(self):
        root = Path(__file__).resolve().parents[1].parent
        image = (root / 'JourneyPC/Journey.exe').read_bytes()
        resources = character_resources(image)
        files = character_mesh_files(image)
        self.assertEqual(len(files), 25)
        private_names = set()
        for (kind, name), desc in resources.items():
            if kind != 'Mesh':
                continue
            native = (root / 'JourneyPC/Data/Meshes' / (Path(desc['source']).stem + '.D3D11x64')).read_bytes()
            payload = files['JNETCharacter_' + name + '.D3D11x64']
            self.assertEqual(len(payload), len(native))
            restored = bytearray(payload)
            matches = list(re.finditer(rb'Data/Meshes/[^\x00]+\x00', native))
            self.assertTrue(matches)
            for match in matches:
                alias = payload[match.start():match.end()]
                self.assertEqual(alias, b'JNET' + match[0][4:])
                self.assertNotIn(alias, private_names)
                private_names.add(alias)
                restored[match.start():match.start()+4] = b'Data'
            self.assertEqual(bytes(restored), native)
            self.assertNotIn(b'Data/Meshes/', payload)

    def test_loose_animation_payloads_preserve_isolated_bundle_content(self):
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        files = character_animation_files(image)
        bundles = character_animation_bundles(image)
        self.assertEqual(len(files), 13)
        for name,clip in (('JNETSpin3','SlideJump3'),('JNETFlipL','RollL'),('JNETFlipR','RollR')):
            payload=files.pop(name+'.D3D11x64')
            native=(Path(__file__).resolve().parents[1].parent/('JourneyPC/Data/Meshes/C_ClothNinja_A_'+clip+'.D3D11x64')).read_bytes()
            self.assertEqual(payload,character_animation_namespace(native,'C_ClothNinja_A_'+clip+'.PSSG'))
        header = character_animation_file_header(files)
        self.assertNotIn('.gz', header)
        for name, payload in files.items():
            self.assertTrue(name.startswith('JNET_'))
            key = 'JC_' + name.removeprefix('JNET_').removesuffix('.D3D11x64')
            self.assertLess(len(key),16)
            self.assertIn('"' + key + '","' + name + '"', header)
            with tarfile.open(fileobj=io.BytesIO(bundles[key + '.Phyre.D3D11x64.gz'])) as archive:
                listing = archive.extractfile(archive.getmembers()[0]).read().decode('ascii').splitlines()
                self.assertEqual(payload, archive.extractfile(listing[1]).read())
            self.assertIn(b'JNET/Meshes/', payload)
            self.assertFalse(payload.startswith(b'\x1f\x8b'))

    def test_missing_resident_texture_is_rejected(self):
        from startup_resources import extract_persistent
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        source = extract_persistent(image)
        altered = source.replace(b'"Texture" "DesertRamp"', b'"Texture" "MissingRamp"')
        self.assertNotEqual(source, altered)
        with patch('startup_resources.extract_persistent', return_value=altered):
            with self.assertRaisesRegex(ValueError, 'not uniquely resident'):
                character_resources(image)

    def test_war_visual_candidate_has_no_eye_or_attack_events(self):
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        assembly = character_visual_assembly('War Machine', character_resources(image), include_eye=False)
        self.assertNotIn('eye', assembly)
        triggers = [row['Type'] for row in assembly['objects'].values() if row['kind'] == 'Trigger']
        self.assertTrue(triggers)
        self.assertEqual(set(triggers), {'AttachObjectToMarker'})

    def test_native_animation_slot_does_not_overwrite_index(self):
        old = b'JNETCharacter_WingSpin\0'
        self.assertEqual(int.from_bytes(old[16:20], 'little'), 0x7053676e)
        alias = character_resource_name('MeshAnimation', 'WingSpin').encode() + b'\0'
        slot = bytearray(16) + b'\xff' * 4
        slot[:len(alias)] = alias
        self.assertEqual(slot[16:20], b'\xff' * 4)
        with self.assertRaisesRegex(ValueError, '16-byte slot'):
            character_resource_name('MeshAnimation', 'A' * 13)

    def test_private_resources_and_animation_references(self):
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        resources = character_resources(image)
        aliases = {(kind, character_resource_name(kind, name)) for kind, name in resources}
        for character in CHARACTERS:
            assembly = character_visual_assembly(character, resources)
            graph = assembly['objects']
            restored = LiteralReader('T=' + lua_literal(assembly)).assignment('T')
            self.assertEqual(restored, json.loads(json.dumps(assembly)))
            animated = []
            for row in graph.values():
                if row['kind'] == 'Decoration':
                    for kind in ('Mesh', 'Shader'):
                        self.assertIn((kind, row[kind]), aliases)
                    for param in row['ShaderParams']:
                        if param['ParamType'] == 'texture':
                            self.assertIn(('Texture', param['ParamVal']), resources)
                            self.assertFalse(param['ParamVal'].startswith('JNETCharacter_'))
                if row.get('Type') in ('CreaA_PlayAnimation', 'DecorationPlayAnimation'):
                    self.assertIn(('MeshAnimation', row['Vars']['targetAnimName']), aliases)
                    animated.append(row)
                for ref in members(row.get('Vars', {})):
                    self.assertIn(ref, graph)
                for ref in row.get('Objects', []):
                    self.assertIn(ref, graph)
            if character == 'Whale':
                self.assertEqual(len(animated), 12)
                self.assertTrue(all(row['Vars']['loopAnim'] for row in animated))
            if character == 'Longtail':
                self.assertEqual(len(animated), 1)

    def test_compiled_assets_and_runtime_scope(self):
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        resources = character_resources(image)
        mapping = character_asset_map_header(resources)
        self.assertEqual(mapping.count('/Data/Textures/Bin/JNETCharacter_'), 7)
        self.assertIn('WhiteWhite.dds.D3D11x64', mapping)
        self.assertIn('GreyGrey.dds.D3D11x64', mapping)
        self.assertNotIn('JNETCharacter_P_MechHeadEye', mapping)
        script = character_resource_script(image).decode()
        self.assertNotIn('resource \"Texture\"', script)
        self.assertIn('resource \"Mesh\"', script)
        declarations = re.findall(r'resource "(\w+)" "([^"]+)"', script)
        for (kind, name), desc in resources.items():
            if kind == 'MeshAnimation':
                self.assertLess(declarations.index(('MeshAnimation', character_resource_name(kind, name))),
                                declarations.index(('Mesh', desc['meshName'])))
        header = character_runtime_header(image)
        self.assertNotIn('JNETCharacter_DesertRamp', header)
        self.assertNotIn('JNETCharacter_White', header)
        self.assertIn('jnet_character_1_source', header)
        self.assertIn('jnet_character_3_source', header)
        self.assertIn('jnet_character_2_source', header)
        self.assertEqual({name for kind, name in resources if kind == 'Texture'},
                         {'Blank', 'DesertRamp', 'GuardianMech', 'GuardianWing', 'SandMask', 'White', 'Grey'})
        war = character_visual_assembly('War Machine', resources)
        eye = war['objects'][war['eye']]
        self.assertEqual(eye['Type'], 'GuardianEyeSim')
        self.assertEqual(eye['Vars']['creature'], {'ref': war['root']})
        self.assertTrue(all(ref in war['objects'] for ref in members(eye['Vars'])))

    def test_animation_bundles_isolate_asset_names_and_preserve_motion_bytes(self):
        image = (Path(__file__).resolve().parents[1].parent / 'JourneyPC/Journey.exe').read_bytes()
        resources = character_resources(image)
        bundles = character_animation_bundles(image)
        header = character_animation_bundle_header(image)
        expected = {character_resource_name(kind, name) for kind, name in resources if kind == 'MeshAnimation'}
        self.assertEqual({name.removesuffix('.Phyre.D3D11x64.gz') for name in bundles}, expected)
        self.assertEqual(bundles, character_animation_bundles(image))
        for name, data in bundles.items():
            key = name.removesuffix('.Phyre.D3D11x64.gz')
            self.assertIn('"' + name + '"', header)
            with tarfile.open(fileobj=io.BytesIO(data)) as archive:
                self.assertEqual(len(archive.getmembers()), 2)
                listing = archive.extractfile(archive.getmembers()[0]).read().decode('ascii').splitlines()
                self.assertEqual(listing[0], key)
                animation = archive.extractfile(listing[1]).read()
                source = resources['MeshAnimation', key.removeprefix('JC_')]['source']
                original = Path(__file__).resolve().parents[1].parent / 'JourneyPC/Data/Meshes' / (Path(source).stem + '.D3D11x64')
                native = original.read_bytes()
                self.assertEqual(len(animation), len(native))
                prefix = ('Data/Meshes/' + Path(source).stem + '.dae#').encode()
                positions = []
                at = 0
                while (at := native.find(prefix, at)) != -1:
                    positions.append(at)
                    at += len(prefix)
                self.assertTrue(positions)
                restored = bytearray(animation)
                for at in positions:
                    self.assertEqual(animation[at:at+4], b'JNET')
                    restored[at:at+4] = b'Data'
                self.assertEqual(bytes(restored), native)
                self.assertNotIn(prefix, animation)
                self.assertIn(prefix.replace(b'Data/', b'JNET/') + b'VisualSceneNode\0', animation)

    def test_animation_namespace_rejects_unknown_or_missing_names(self):
        for data in (b'', b'Data/Meshes/Other.dae#VisualSceneNode\0',
                     b'Data/Meshes/Fish.dae#AnimationSet\0'):
            with self.assertRaises(ValueError):
                character_animation_namespace(data, 'Fish.PSSG')

    def test_longtail_uses_native_cloth_and_speed_animation(self):
        root = '5da695ecc5eb6a4a28824613d7c85590'
        graph = creature_graph('Level_Desert', root)
        mesh = graph['83bbe0cac68bdc9077c3494422d9710e']
        self.assertEqual(mesh['Mesh'], 'P_FishFlat_A_Test')
        self.assertEqual(mesh['Shader'], 'Simple')
        self.assertEqual(len(mesh['Transformation']), 16)
        self.assertAlmostEqual(mesh['Transformation'][0], 0.4)
        animation = graph['ee04719712623217a644b03ef3b63a3c']['Vars']
        self.assertEqual(animation['targetAnimName'], 'FlatFishSwim')
        self.assertEqual(animation['creatureClump'], {'ref': root})
        self.assertEqual(animation['animSpeedMin'], 0.5)
        self.assertEqual(animation['animSpeedMax'], 1)
        self.assertNotIn('Crea_FollowRail', [row.get('Type') for row in graph.values()])

    def test_native_assemblies_have_closed_references(self):
        for level, root, mesh in (
            ('Level_Cave', 'd2598dc141f1e41f159bbb7ef05a9de7', 'P_MechHead'),
            ('Level_Summit', 'd143a774751633d414e4e8729a58dfba', 'P_ClothGuardianHead'),
        ):
            graph = creature_graph(level, root)
            self.assertIn(mesh, [row.get('Mesh') for row in graph.values()])
            for row in graph.values():
                refs = row.get('Objects', []) if row['kind'] == 'Clump' else members(row.get('Vars', {}))
                for ref in refs:
                    self.assertIn(ref, graph)
            self.assertEqual(sum(row.get('Type') == 'Creature' for row in graph.values()), 1)
            self.assertTrue(any(row.get('Type') == 'AttachObjectToMarker' for row in graph.values()))

    def test_literal_only_and_duplicate_refusal(self):
        for source in ('T={x=os.execute("anything")}', 'T={x=1,x=2}', 'T={} print("anything")',
                       'T={', 'T={x={}', 'T={x=1,'):
            with self.assertRaises(ValueError):
                LiteralReader(source).assignment('T')
        self.assertEqual(LiteralReader('T={x=Names["native-id"],y=false}').assignment('T'),
                         {'x': {'ref': 'native-id'}, 'y': False})

    def test_whale_uses_native_reveal_visibility(self):
        graph = character_assembly('Whale')['objects']
        visible = [row['Mesh'] for row in graph.values()
                   if row['kind'] == 'Decoration' and row['CharacterVisible']]
        self.assertEqual(visible.count('P_ClothGuardianHead'), 1)
        self.assertEqual(visible.count('P_ClothGuardianNeck'), 7)
        self.assertEqual(visible.count('P_ClothGuardianWing'), 3)
        self.assertEqual(visible.count('P_ClothGuardianTail'), 1)
        self.assertEqual(len(visible), 12)

    def test_spawn_rebases_without_changing_source_or_targeting_old_level(self):
        for character in CHARACTERS:
            original = character_assembly(character)
            spawned = relocate_assembly(original, [1, 600, -20])
            target = original['objects'][original['root']]['Vars']['target']['ref']
            self.assertEqual(spawned['objects'][target]['Position'], [1, 600, -20])
            self.assertNotEqual(original['objects'][target]['Position'], [1, 600, -20])
            meshes = [key for key, row in original['objects'].items() if row['kind'] == 'Decoration']
            shifts = []
            for key in meshes:
                before = original['objects'][key]['Transformation']
                after = spawned['objects'][key]['Transformation']
                self.assertEqual(list(before[:12]), after[:12])
                shifts.append([after[12+i] - before[12+i] for i in range(3)])
            for shift in shifts:
                for i in range(3):
                    self.assertAlmostEqual(shift[i], shifts[0][i])
            with self.assertRaises(ValueError):
                relocate_assembly(original, [0, float('nan'), 0])


if __name__ == '__main__':
    unittest.main()
