import json
import copy
from collections import Counter, defaultdict
import math
import re
import struct
import gzip
import io
import tarfile
from pathlib import Path


LAB = Path(__file__).resolve().parent
ROOT = LAB.parent.parent
CHARACTERS = {
    'Longtail': ('Level_Desert', '5da695ecc5eb6a4a28824613d7c85590'),
    'War Machine': ('Level_Cave', 'd2598dc141f1e41f159bbb7ef05a9de7'),
    'Whale': ('Level_Summit', 'd143a774751633d414e4e8729a58dfba'),
    'fl0w 1': ('Level_Ruins', 'a1d05a9dcd548ccf82c7baf437c17e09'),
    'fl0w 2': ('Level_Ruins', '017c4db3317950fb9cfc77f97f298a14'),
}
CHARACTER_IDS = {'Longtail': 1, 'War Machine': 2, 'Whale': 3, 'fl0w 1': 5, 'fl0w 2': 6}
TOKEN = re.compile(r'\s+|--[^\n]*|"(?:\\.|[^"\\])*"|[A-Za-z_][A-Za-z_0-9]*|[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?|[{}\[\]=,;]')


class LiteralReader:
    def __init__(self, text):
        self.tokens = []
        at = 0
        for match in TOKEN.finditer(text):
            if match.start() != at:
                raise ValueError(f"Unsupported Lua syntax at {at}")
            at = match.end()
            value = match[0]
            if not value.isspace() and not value.startswith('--'):
                self.tokens.append(value)
        if at != len(text):
            raise ValueError("Unparsed Lua suffix")
        self.at = 0

    def take(self, expected=None):
        if self.at >= len(self.tokens):
            raise ValueError("Unexpected end of Lua table")
        token = self.tokens[self.at]
        self.at += 1
        if expected is not None and token != expected:
            raise ValueError(f"Expected {expected}, found {token}")
        return token

    def peek(self):
        if self.at >= len(self.tokens):
            raise ValueError("Unexpected end of Lua table")
        return self.tokens[self.at]

    def value(self):
        token = self.take()
        if token == '{':
            fields, items = {}, []
            while self.peek() != '}':
                keyed = self.peek() == '[' or (self.at + 1 < len(self.tokens) and self.tokens[self.at + 1] == '=')
                if keyed:
                    if self.peek() == '[':
                        self.take('[')
                        key = self.value()
                        self.take(']')
                        if not isinstance(key, str):
                            raise ValueError('Expected string table key')
                    else:
                        key = self.take()
                    self.take('=')
                    if key in fields:
                        raise ValueError(f"Duplicate field {key}")
                    fields[key] = self.value()
                else:
                    items.append(self.value())
                if self.peek() in (',', ';'):
                    self.take()
                elif self.peek() != '}':
                    raise ValueError("Missing Lua field separator")
            self.take('}')
            if fields and items:
                fields['items'] = items
                return fields
            return fields if fields else items
        if token == 'Names':
            self.take('[')
            name = self.value()
            self.take(']')
            if not isinstance(name, str):
                raise ValueError("Non-string native object reference")
            return {'ref': name}
        if token.startswith('"'):
            return json.loads(token)
        if token in ('true', 'false'):
            return token == 'true'
        if token == 'nil':
            return None
        try:
            return float(token) if any(c in token for c in '.eE') else int(token)
        except ValueError:
            raise ValueError(f"Nonliteral Lua value {token}") from None

    def assignment(self, expected):
        self.take(expected)
        self.take('=')
        result = self.value()
        if self.at != len(self.tokens):
            raise ValueError("Trailing executable Lua is not permitted")
        return result


def records(level, kind):
    path = LAB / 'work/pc_scripts' / level / f'{kind}Instances.lua'
    result = LiteralReader(path.read_text()).assignment(f'{kind}Instances')
    rows = result.get('items', []) if isinstance(result, dict) else result
    output = {}
    for row in rows:
        name = row['ObjectName']
        if name in output:
            raise ValueError(f"Duplicate object {name}")
        output[name] = row
    return output


def decorations(level):
    data = (ROOT / 'JourneyPC/Data/Scripts' / level / 'DecorationMeshInstances.lua.bin').read_bytes()
    version, count = struct.unpack_from('<II', data)
    if version != 1 or len(data) != 16 + count * 0x840:
        raise ValueError('Unexpected native decoration record format')
    result = {}
    for index in range(count):
        record = data[16 + index * 0x840:16 + (index + 1) * 0x840]
        def name(offset):
            field = record[offset:offset + 64]
            end = field.find(b'\0')
            if end < 0:
                raise ValueError('Unterminated native decoration name')
            return field[:end].decode('ascii')
        key = name(0x70)
        if key in result:
            raise ValueError(f'Duplicate decoration {key}')
        matrix = struct.unpack_from('<16f', record, 0x10)
        if not all(math.isfinite(value) for value in matrix):
            raise ValueError(f'Nonfinite decoration transform {key}')
        count_params = struct.unpack_from('<I', record, 0x830)[0]
        if count_params > 16:
            raise ValueError('Native decoration shader parameter overflow')
        params = []
        for slot in range(count_params):
            at = 0x130 + slot * 0x70
            param_name = record[at + 0x50:at + 0x60].split(b'\0', 1)[0].decode('ascii')
            param_type = struct.unpack_from('<I', record, at + 0x60)[0]
            if param_type == 2:
                value = name(at + 0x10)
            elif param_type in (0, 1):
                value = struct.unpack_from('<4f', record, at)
                if not all(math.isfinite(item) for item in value):
                    raise ValueError('Nonfinite native shader parameter')
                value = value[0] if param_type == 0 else value
            else:
                raise ValueError(f'Unsupported native shader parameter type {param_type}')
            params.append(dict(ParamName=param_name, ParamType=('float', 'float4', 'texture')[param_type], ParamVal=value))
        result[key] = dict(ObjectName=key, Mesh=name(0xb0), Shader=name(0xf0),
                           Transformation=matrix,
                           FadeMin=struct.unpack_from('<f', record, 0x50)[0],
                           FadeMax=struct.unpack_from('<f', record, 0x54)[0],
                           ShaderLODMin=struct.unpack_from('<f', record, 0x58)[0],
                           ShaderLODMax=struct.unpack_from('<f', record, 0x5c)[0],
                           Brightness=struct.unpack_from('<f', record, 0x60)[0],
                           Saturation=struct.unpack_from('<f', record, 0x64)[0],
                           Hue=struct.unpack_from('<f', record, 0x68)[0],
                           ShaderParams=params,
                           NativeFlags=struct.unpack_from('<I', record, 0x6c)[0],
                           record=index)
    return result


def members(value):
    if isinstance(value, dict):
        if set(value) == {'ref'}:
            yield value['ref']
        else:
            for item in value.values():
                yield from members(item)
    elif isinstance(value, list):
        for item in value:
            yield from members(item)


def creature_graph(level, root):
    kinds = {kind: records(level, kind) for kind in ('DynamicNode', 'Marker', 'Clump', 'Trigger', 'SoundEmitter', 'EnvNode', 'ParticleEmitter')}
    kinds['Decoration'] = decorations(level)
    nodes = {}
    for kind, rows in kinds.items():
        for name, row in rows.items():
            if name in nodes:
                raise ValueError(f'Object has multiple native types: {name}')
            nodes[name] = (kind, row)
    if root not in kinds['DynamicNode'] or kinds['DynamicNode'][root].get('Type') != 'Creature':
        raise ValueError('Root must be a native Creature')
    selected = set()
    def include(name):
        if name in selected:
            return
        if name not in nodes:
            raise ValueError(f'Missing native dependency {name}')
        selected.add(name)
        kind, row = nodes[name]
        for dependency in (row.get('Objects', []) if kind == 'Clump' else members(row.get('Vars', {}))):
            include(dependency)
    include(root)
    changed = True
    while changed:
        changed = False
        for name, row in kinds['Trigger'].items():
            if name in selected or row.get('Type') != 'AttachObjectToMarker':
                continue
            source = row.get('Vars', {}).get('src')
            source = source.get('ref') if isinstance(source, dict) else None
            if source in selected:
                include(name)
                changed = True
    for name, row in kinds['Trigger'].items():
        if row.get('Type') == 'CreaA_PlayAnimation':
            refs = set(members(row.get('Vars', {})))
            if root in refs and refs.issubset(selected):
                include(name)
    return {name: {'kind': nodes[name][0], **nodes[name][1]} for name in sorted(selected)}


def clump_leaves(clumps, name, visiting=None):
    visiting = set() if visiting is None else visiting
    if name in visiting:
        raise ValueError(f'Cyclic native clump {name}')
    if name not in clumps:
        return {name}
    visiting.add(name)
    result = set()
    for child in clumps[name]['Objects']:
        result.update(clump_leaves(clumps, child, visiting))
    visiting.remove(name)
    return result


def character_assembly(character):
    level, root = CHARACTERS[character]
    graph = creature_graph(level, root)
    visible = {name for name, row in graph.items() if row['kind'] == 'Decoration'}
    if character == 'Whale':
        triggers = records(level, 'Trigger')
        reveal = triggers['e7085fa190be1fff1ce18deadaeef3c9']
        if reveal['Type'] != 'Enable' or reveal['Vars']['enableFlag'] is not True:
            raise ValueError('Native Whale reveal definition changed')
        visible &= clump_leaves(records(level, 'Clump'), reveal['Vars']['objects']['ref'])
        expected = {'P_ClothGuardianHead', 'P_ClothGuardianNeck',
                    'P_ClothGuardianWing', 'P_ClothGuardianTail'}
        if {graph[name]['Mesh'] for name in visible} != expected:
            raise ValueError('Incomplete native Whale reveal assembly')
    for name, row in graph.items():
        if row['kind'] == 'Decoration':
            row['CharacterVisible'] = name in visible
    return {'character': character, 'level': level, 'root': root, 'objects': graph}


def relocate_assembly(assembly, position):
    if len(position) != 3 or not all(isinstance(v, (int, float)) and math.isfinite(v) for v in position):
        raise ValueError('Character spawn needs a finite three-dimensional position')
    result = copy.deepcopy(assembly)
    graph = result['objects']
    root = graph[result['root']]
    segments = root['Vars']['segments']['ref']
    clumps = {key: row for key, row in graph.items() if row['kind'] == 'Clump'}
    head = graph[clumps[segments]['Objects'][0]]
    if head['kind'] == 'Clump':
        markers = [graph[key] for key in clump_leaves(clumps, head['ObjectName'])
                   if graph[key]['kind'] == 'Marker']
        if len(markers) != 1:
            raise ValueError('Ambiguous native creature head marker')
        head = markers[0]
    if head['kind'] != 'Marker':
        raise ValueError('Native creature has no head marker')
    delta = [position[i] - head['Position'][i] for i in range(3)]
    for row in graph.values():
        if 'Position' in row:
            row['Position'] = [row['Position'][i] + delta[i] for i in range(3)]
        if row['kind'] == 'Decoration':
            matrix = list(row['Transformation'])
            for i in range(3):
                matrix[12 + i] += delta[i]
            row['Transformation'] = matrix
    target = graph[root['Vars']['target']['ref']]
    if target['kind'] != 'Marker':
        raise ValueError('Native creature movement target is not a marker')
    target['Position'] = list(position)
    return result


def character_resources(image, participants=1):
    from startup_resources import extract_persistent
    persistent = extract_persistent(image)
    if type(participants) is not int or not 1 <= participants <= 3:
        raise ValueError('Unsupported character participant count')
    definitions = defaultdict(set)
    pattern = rb'(?:wadresource|resource)\s*"(Mesh|Shader|MeshAnimation|Texture)"\s*"([^"\r\n]+)"\s*(\{[^{}]*\})'
    for match in re.finditer(pattern, image):
        definitions[match[1].decode(), match[2].decode()].add(match[3].decode())
    counts = {'Mesh': Counter(), 'Shader': Counter()}
    textures = set()
    for character in CHARACTERS:
        for row in character_assembly(character)['objects'].values():
            if row['kind'] == 'Decoration':
                for kind in counts:
                    counts[kind][row[kind]] += participants
                textures.update(param['ParamVal'] for param in row['ShaderParams'] if param['ParamType'] == 'texture')
    selected = {}
    for kind, needs in counts.items():
        for name, capacity in sorted(needs.items()):
            bodies = definitions[kind, name]
            if len(bodies) != 1:
                raise ValueError(f'Ambiguous native resource {kind}:{name}')
            desc = LiteralReader('T=' + next(iter(bodies))).assignment('T')
            if not isinstance(desc, dict) or type(desc.get('instanceLimit')) is not int:
                raise ValueError(f'Native resource has no explicit capacity: {name}')
            desc['instanceLimit'] = capacity
            selected[kind, name] = desc
    for name in sorted(textures):
        bodies = definitions['Texture', name]
        if len(bodies) != 1:
            raise ValueError(f'Ambiguous native texture {name}')
        declarations = list(re.finditer(rb'(?m)^\s*(?:,\s*)?(?:resource|wadresource)\s+"Texture"\s+"' + re.escape(name.encode()) + rb'"\s*(\{[^{}]*\})', persistent))
        if len(declarations) != 1 or declarations[0][1].decode() not in bodies:
            raise ValueError(f'Creature texture is not uniquely resident at startup: {name}')
        selected['Texture', name] = LiteralReader('T=' + next(iter(bodies))).assignment('T')
    animated_meshes = {row['Mesh'] for character in ('Longtail', 'Whale')
                       for row in character_assembly(character)['objects'].values()
                       if row['kind'] == 'Decoration'}
    for (kind, name), bodies in sorted(definitions.items()):
        if kind != 'MeshAnimation':
            continue
        for body in bodies:
            desc = LiteralReader('T=' + body).assignment('T')
            if desc.get('meshName') in animated_meshes:
                if len(bodies) != 1:
                    raise ValueError(f'Ambiguous native animation {name}')
                desc['meshName'] = 'JNETCharacter_' + desc['meshName']
                selected[kind, name] = desc
    return selected


def lua_literal(value):
    if value is None:
        return 'nil'
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, str):
        if not value.isascii():
            raise ValueError('Native character identifiers must be ASCII')
        return json.dumps(value, ensure_ascii=True)
    if isinstance(value, (int, float)) and math.isfinite(value):
        return repr(value)
    if isinstance(value, (list, tuple)):
        return '{' + ','.join(lua_literal(item) for item in value) + '}'
    if isinstance(value, dict):
        return '{' + ','.join('[' + lua_literal(key) + ']=' + lua_literal(item)
                              for key, item in sorted(value.items())) + '}'
    raise ValueError('Unsupported native character literal')


def character_resource_name(kind, name):
    alias = ('JC_' if kind == 'MeshAnimation' else 'JNETCharacter_') + name
    if kind == 'MeshAnimation' and (not alias.isascii() or len(alias) >= 16):
        raise ValueError('Private animation name exceeds native 16-byte slot')
    return alias


WAYFARER_ACTION_CLIPS = {"JNETSpin3": "SlideJump3", "JNETFlipL": "RollL", "JNETFlipR": "RollR"}


def wayfarer_resource_script():
    return ("\n" + "".join('resource "MeshAnimation" "' + name + '" { source="C_ClothNinja_A_' + clip +
                            '.PSSG", meshName="BlendNinja" }\n'
                            for name, clip in WAYFARER_ACTION_CLIPS.items())).encode("ascii")


def character_resource_script(image, participants=1):
    resources = character_resources(image, participants)
    return ('\nresource "ScreamBank" "JNTBio" { source="Summit-SFX.bank" }\nresource "ScreamBank" "JNTMech" { source="Cave-SFX.bank" }\n' + '\n'.join('resource ' + lua_literal(kind) + ' ' +
            lua_literal(character_resource_name(kind, name)) + ' ' + lua_literal(desc)
            for (kind, name), desc in sorted(resources.items(), key=lambda item:
                (('Texture', 'Shader', 'MeshAnimation', 'Mesh').index(item[0][0]), item[0][1])) if kind != 'Texture') + '\n').encode('ascii')


def character_animation_namespace(data, source):
    stem = Path(source).stem
    if not re.fullmatch(r'[A-Za-z0-9_]+', stem):
        raise ValueError('Invalid animation source name')
    prefix = ('Data/Meshes/' + stem + '.dae#').encode('ascii')
    names = list(re.finditer(rb'Data/Meshes/[^\x00]+\x00', data))
    if not names or any(not match[0].startswith(prefix) for match in names):
        raise ValueError('Unexpected animation asset-name namespace')
    if not any(match[0] == prefix + b'VisualSceneNode\0' for match in names):
        raise ValueError('Missing animation scene asset')
    output = bytearray(data)
    for match in names:
        output[match.start():match.start()+4] = b'JNET'
    return bytes(output)


def character_animation_bundles(image):
    resources = character_resources(image)
    native = {}
    for stem in ('FlatFish_6348', 'ClothGuardian_39196'):
        with tarfile.open(ROOT / 'JourneyPC/Data/Wads' / (stem + '.Phyre.D3D11x64.gz')) as archive:
            listings = [m for m in archive.getmembers() if m.name.endswith('.lst')]
            if len(listings) != 1:
                raise ValueError('Ambiguous native animation bundle listing')
            lines = archive.extractfile(listings[0]).read().decode('ascii').splitlines()
            if len(lines) != 2 or len(lines[0].split()) != len(lines[1].split()):
                raise ValueError('Invalid native animation bundle listing')
            for name, path in zip(lines[0].split(), lines[1].split()):
                if name in native:
                    raise ValueError('Duplicate native animation bundle key')
                native[name] = (path, archive.extractfile(path).read())
    bundles = {}
    for (kind, name), desc in sorted(resources.items()):
        if kind != 'MeshAnimation':
            continue
        path, data = native[name]
        filename = Path(desc['source']).stem + '.D3D11x64'
        if Path(path).name != filename or data != (ROOT / 'JourneyPC/Data/Meshes' / filename).read_bytes():
            raise ValueError('Animation bundle/source mismatch: ' + name)
        data = character_animation_namespace(data, desc['source'])
        key = character_resource_name(kind, name)
        listing = (key + '\r\n' + path + '\r\n').encode('ascii')
        output = io.BytesIO()
        with tarfile.open(fileobj=output, mode='w', format=tarfile.USTAR_FORMAT) as archive:
            for member, content in (('Data/Wads/' + key + '.Phyre.tar.tmp.lst', listing), (path, data)):
                info = tarfile.TarInfo(member)
                info.size = len(content)
                info.mode = 0o644
                archive.addfile(info, io.BytesIO(content))
        bundles[key + '.Phyre.D3D11x64.gz'] = gzip.compress(output.getvalue(), mtime=0)
    return bundles


def character_animation_bundle_header(image):
    names = character_animation_bundles(image)
    return ('static const struct { const char *key, *file; } jnet_character_animation_bundles[] = {\n' +
            ''.join('{' + json.dumps(name.removesuffix('.Phyre.D3D11x64.gz')) + ',' +
                    json.dumps(name) + '},\n' for name in names) + '};\n')


def character_animation_files(image):
    files = {}
    for name, clip in WAYFARER_ACTION_CLIPS.items():
        source = 'C_ClothNinja_A_' + clip
        data = (ROOT / 'JourneyPC/Data/Meshes' / (source + '.D3D11x64')).read_bytes()
        files[name + '.D3D11x64'] = character_animation_namespace(data, source + '.PSSG')
    for name, data in character_animation_bundles(image).items():
        key = name.removesuffix('.Phyre.D3D11x64.gz')
        with tarfile.open(fileobj=io.BytesIO(data)) as archive:
            listing = archive.extractfile(archive.getmembers()[0]).read().decode('ascii').splitlines()
            files['JNET_' + key.removeprefix('JC_') + '.D3D11x64'] = archive.extractfile(listing[1]).read()
    return files


def character_mesh_files(image):
    files = {}
    for (kind, name), desc in character_resources(image).items():
        if kind != 'Mesh':
            continue
        path = ROOT / 'JourneyPC/Data/Meshes' / (Path(desc['source']).stem + '.D3D11x64')
        files['JNETCharacter_' + name + '.D3D11x64'] = character_animation_namespace(path.read_bytes(), desc['source'])
    return files


def character_animation_file_header(files):
    return ('static const struct { const char *key, *file; } jnet_character_animation_files[] = {\n' +
            ''.join('{' + json.dumps(('JC_' + name[5:] if name.startswith('JNET_') else name).removesuffix('.D3D11x64')) + ',' +
                    json.dumps(name) + '},\n' for name in files) + '};\n')


def character_visual_assembly(character, resources, include_eye=True):
    assembly = character_assembly(character)
    graph = assembly['objects']
    if character == 'War Machine' and include_eye:
        eyes = [(name, row) for name, row in records(assembly['level'], 'Trigger').items()
                if row['Type'] == 'GuardianEyeSim' and
                row['Vars'].get('creature') == {'ref': assembly['root']}]
        if len(eyes) != 1:
            raise ValueError('Ambiguous native War Machine eye controller')
        name, row = eyes[0]
        for kind in ('Marker', 'Clump'):
            source = records(assembly['level'], kind)
            for ref in members(row['Vars']):
                if ref not in graph and ref in source:
                    graph[ref] = dict(source[ref], kind=kind)
        if any(ref not in graph for ref in members(row['Vars'])):
            raise ValueError('Incomplete native War Machine eye assembly')
        graph[name] = dict(row, kind='Trigger')
        assembly['eye'] = name
    if character == 'Whale':
        visible = {name for name, row in graph.items()
                   if row['kind'] == 'Decoration' and row['CharacterVisible']}
        animated = set()
        for name, row in records(assembly['level'], 'Trigger').items():
            if row['Type'] != 'DecorationPlayAnimation' or row['Vars'].get('speed') != 0.07:
                continue
            refs = set(members(row['Vars']))
            if not refs or not refs <= visible:
                continue
            if animated & refs:
                raise ValueError('Conflicting native Whale animations')
            animated.update(refs)
            graph[name] = dict(row, kind='Trigger')
        if animated != visible:
            raise ValueError('Incomplete native Whale swimming animations')
    graph = {name: row for name, row in graph.items()
             if row['kind'] in ('Marker', 'Clump', 'Decoration', 'DynamicNode', 'Trigger')}
    changed = True
    while changed:
        changed = False
        for name, row in list(graph.items()):
            if row['kind'] == 'Clump':
                row['Objects'] = [key for key in row['Objects'] if key in graph]
                if not row['Objects']:
                    del graph[name]
                    changed = True
            elif row['kind'] == 'Trigger' and any(key not in graph for key in members(row.get('Vars', {}))):
                del graph[name]
                changed = True
    for row in graph.values():
        if row['kind'] == 'Decoration':
            for kind in ('Mesh', 'Shader'):
                if (kind, row[kind]) not in resources:
                    raise ValueError('Missing private character resource')
                row[kind] = 'JNETCharacter_' + row[kind]
            for param in row['ShaderParams']:
                if param['ParamType'] == 'texture' and ('Texture', param['ParamVal']) not in resources:
                    raise ValueError('Missing resident character texture')
        if row.get('Type') in ('CreaA_PlayAnimation', 'DecorationPlayAnimation'):
            name = row['Vars']['targetAnimName']
            if ('MeshAnimation', name) not in resources:
                raise ValueError('Missing private character animation')
            row['Vars']['targetAnimName'] = character_resource_name('MeshAnimation', name)
    for row in graph.values():
        for ref in members(row.get('Vars', {})):
            if ref not in graph:
                raise ValueError(f'Unresolved playable character dependency {ref}')
    assembly['objects'] = graph
    return assembly


def character_runtime_header(image):
    resources = character_resources(image)
    factory = (LAB / 'diagnostic/jnet_character_factory.lua').read_text()
    lines = ['static const char jnet_character_factory_source[] =',
             *[json.dumps(line + '\n') for line in factory.splitlines()], ';',
             'struct jnet_character_mesh_spec { const char *mesh,*shader; unsigned flags,visible; };',
             'struct jnet_character_spec { const char *source; struct jnet_character_budget budget; '
             'const struct jnet_character_mesh_spec *meshes; unsigned mesh_count; };']
    specs = ['{0}'] * (max(CHARACTER_IDS.values()) + 1)
    for character in CHARACTERS:
        index = CHARACTER_IDS[character]
        assembly = character_visual_assembly(character, resources)
        rows = [row for _, row in sorted(assembly['objects'].items())]
        counts = Counter(row['kind'] for row in rows)
        wrappers = sum(isinstance(value, (dict, list))
                       for row in rows for value in row.get('Vars', {}).values())
        budget = (counts['Decoration'], counts['Marker'], counts['Clump'] + wrappers + 2, counts['Trigger'] + 2)
        prefix = 'jnet_character_' + str(index)
        source = 'return ' + lua_literal(assembly)
        lines += ['static const char ' + prefix + '_source[] = ' + json.dumps(source) + ';',
                  'static const struct jnet_character_mesh_spec ' + prefix + '_meshes[] = {']
        for row in rows:
            if row['kind'] == 'Decoration':
                lines.append('{' + json.dumps(row['Mesh']) + ',' + json.dumps(row['Shader']) + ',' +
                             str(row['NativeFlags']) + ',' + str(int(row['CharacterVisible'])) + '},')
        lines += ['};']
        specs[index] = ('{' + prefix + '_source,{' + ','.join(map(str, budget)) + '},' +
                        prefix + '_meshes,' + str(counts['Decoration']) + '}')
    lines += ['static const struct jnet_character_spec jnet_character_specs[] = {',
              ',\n'.join(specs), '};']
    mesh_users = defaultdict(int)
    for character in CHARACTERS:
        for row in character_assembly(character)['objects'].values():
            if row['kind'] == 'Decoration':
                mesh_users['JNETCharacter_' + row['Mesh']] |= 1 << CHARACTER_IDS[character]
    lines.append('static struct jnet_character_animation_dependency jnet_character_animation_dependencies[]={')
    for (kind, name), desc in sorted(resources.items()):
        if kind == 'MeshAnimation':
            lines.append('{' + json.dumps(character_resource_name(kind, name)) + ',' +
                         str(mesh_users[desc['meshName']]) + ',0},')
    lines.append('};')
    character_asset_map_header(resources)
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('level', choices=('Level_Desert', 'Level_Cave', 'Level_Summit', 'Level_Ruins'))
    parser.add_argument('creature')
    args = parser.parse_args()
    graph = creature_graph(args.level, args.creature)
    print(json.dumps(graph, indent=2))


def character_asset_map_header(resources):
    paths = {}
    for (kind, name), desc in sorted(resources.items()):
        if kind == 'Mesh':
            path = ROOT / 'JourneyPC/Data/Meshes' / (Path(desc['source']).stem + '.D3D11x64')
            data = path.read_bytes()
            if (Path(desc['source']).stem + '.dae#').encode() not in data:
                raise ValueError('Compiled mesh does not contain native name: ' + name)
        elif kind == 'Texture':
            basename = name + desc['source']
            relative = '/Data/Textures/Bin/' + basename + '.D3D11x64'
            data = (ROOT / 'JourneyPC' / relative.lstrip('/')).read_bytes()
            if basename.encode() + b'\0' not in data:
                raise ValueError('Compiled texture does not contain native name: ' + name)
            paths['/Data/Textures/Bin/JNETCharacter_' + basename + '.D3D11x64'] = relative
        elif kind == 'MeshAnimation':
            (ROOT / 'JourneyPC/Data/Meshes' / (Path(desc['source']).stem + '.D3D11x64')).read_bytes()
    lines = ['struct jnet_character_asset_alias { const char *alias, *native; };']
    for label, mapping in (('paths', paths),):
        lines.append('static const struct jnet_character_asset_alias jnet_character_asset_' + label + '[]={')
        lines.extend('{' + json.dumps(a) + ',' + json.dumps(n) + '},' for a, n in sorted(mapping.items()))
        lines.append('};')
    return '\n'.join(lines)
