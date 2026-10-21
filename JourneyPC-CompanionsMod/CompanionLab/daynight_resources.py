import copy
import json
import gzip
from pathlib import Path

from character_resources import LiteralReader, decorations, lua_literal

LAB = Path(__file__).resolve().parent
def records(level, kind):
    data = gzip.decompress((LAB.parent.parent/"JourneyPC/Data/Scripts"/level/("__"+kind+"Instances.lua.gz")).read_bytes()).decode()
    rows = LiteralReader(data).assignment(kind+"Instances")["items"]
    return {r["ObjectName"]: r for r in rows}


NIGHT_ANCHOR = ('b4ad6f584115d4b81fb5b91c022aa353',)
MAPPINGS = (
    ('Graveyard', 'Chris', (0, 0, 0), ('37cc2e82e213ce8eedbf88961ed6325c', '99440de91d6ef6ee642f18ad6089fcb8')),
    ('Barrens', 'Chris', (256, 0, 512), ('b4ad6f584115d4b81fb5b91c022aa353', 'c673990a6fbd41b3cffb104e0d0e9b4a')),
    ('Desert', 'Chris', (0, 0, 0), NIGHT_ANCHOR),
    ('Canyon', 'Chris', (0, 0, 0), NIGHT_ANCHOR),
    ('Cave', 'Chris', (0, 0, 0), NIGHT_ANCHOR),
    ('Ruins', 'Chris', (0, 0, 0), NIGHT_ANCHOR),
    ('Mountain', 'Chris', (0, 0, 0), NIGHT_ANCHOR),
    ('Summit', 'Chris', (0, 0, 0), NIGHT_ANCHOR),
)
# Graveyard and Barrens are half-scale copies inside Level_Chris, so their nodes and clouds map spatially. Every other
# level has no counterpart there and takes Chris's night palette instead, kept no brighter than its own day values.
MINIATURES = ('Graveyard', 'Barrens')
# Pink Desert and Sunken City keep their Level_Matt Dusk and Dawn looks (lighting modes 1 and 2); Night is mode 3.
ALTERNATES = {
    'Desert': ('Matt', (0, 0, 0), ('31f04f52bbfdd3610458c25d5ec5bb87', '4a573e76ffd62db7001bb35504220458', '76a4e0c7aede7544a493a442d1cc0637', 'ecb314f38fdc7e2df7cb0a58e2091e94')),
    'Canyon': ('Matt', (256, -200, 512), ('f6f6c406c04d3c1339585aa9444e9a70', 'e26584b87c4ffef7c0cb1b0c6cbf07d4', '06d51a05b13030319bd6c12e7d5d8317', 'c01c13593c03f46a3e464ba722226169')),
}
FIELDS = ('FogCloseColor', 'FogFarColor', 'FogCloseAlpha', 'FogFarAlpha',
          'SkyMinColor', 'SkyLowColor', 'SkyMidColor', 'SkyHighColor', 'SkyMaxColor',
          'SkyLowHeight', 'SkyMidHeight', 'SkyHighHeight', 'ShadowLength', 'ShadowAlpha',
          'BloomSize', 'BloomIntensity', 'BloomOverbright', 'ChromaticAberration', 'VelvetRatio', 'VelvetInt',
          'Env_LightDir', 'Env_LightColor', 'Env_LightInt', 'Env_AmbientColor', 'Env_AmbientInt',
          'Env_SpecularColor', 'Env_SpecularInt', 'Env_SpecularPower')


def params(row):
    return {p['ParamName']: p['ParamVal'] for p in row['ShaderParams']}


def luminance(color):
    return sum(a*b for a, b in zip(color[:3], (.2126, .7152, .0722)))


def cap_to_day(night, day):
    night = copy.deepcopy(night)
    for key in ('FogCloseColor', 'FogFarColor', 'SkyMinColor', 'SkyLowColor', 'SkyMidColor', 'SkyHighColor', 'SkyMaxColor'):
        ratio = luminance(day[key])/luminance(night[key]) if luminance(night[key]) else 1
        if ratio < 1: night[key] = [c*ratio for c in night[key][:3]] + list(night[key][3:])
    for color, intensity in (('Env_AmbientColor', 'Env_AmbientInt'), ('Env_LightColor', 'Env_LightInt'), ('Env_SpecularColor', 'Env_SpecularInt')):
        energy = luminance(night[color])*night[intensity]
        if energy and luminance(day[color])*day[intensity] < energy:
            night[intensity] *= luminance(day[color])*day[intensity]/energy
    return night


def dim_to_day(night, day):
    factor = min(1, luminance(day)/luminance(night)) if luminance(night) else 1
    return tuple(c*factor for c in night[:3]) + tuple(night[3:])


def cool_cloud(color, tint):
    factor = min(luminance(color), luminance(tint))/luminance(tint)
    return tuple(c*factor for c in tint[:3]) + (color[3],)


def profile(target, source, offset, anchors, moon, tint):
    palette = source == 'Chris' and target not in MINIATURES
    source_nodes = records('Level_' + source, 'EnvNode')
    selected = sorted((source_nodes[key] for key in anchors), key=lambda r: r['ZPos'])
    nodes = {}
    for key, row in records('Level_' + target, 'EnvNode').items():
        z = row.get('ZPos', row.get('MinBoundingBox', {}).get('Position', [0, 0, 0])[2]) * .5 + offset[2]
        lo = max((r for r in selected if r['ZPos'] <= z), key=lambda r: r['ZPos'], default=selected[0])
        hi = min((r for r in selected if r['ZPos'] >= z), key=lambda r: r['ZPos'], default=selected[-1])
        t = 0 if hi is lo else (z-lo['ZPos'])/(hi['ZPos']-lo['ZPos'])
        def blend(a, b):
            if isinstance(a, list): return [blend(x,y) for x,y in zip(a,b)]
            return a+(b-a)*t
        nodes[key] = {k: blend(lo[k], hi[k]) for k in FIELDS}
        if palette: nodes[key] = cap_to_day(nodes[key], row)
    source_meshes = list(decorations('Level_' + source).values())
    clouds, suns = {}, {}
    for key, row in decorations('Level_' + target).items():
        day = params(row)
        if day.get('tex') == 'Sun' and day.get('alpha', 1) > 0:
            night = moon if source == 'Chris' else day
            suns[key] = dict(day=day, night=night)
        if any(word in row['Mesh'].lower() for word in ('cloud', 'dust', 'mist', 'wind', 'steam', 'storm')):
            meshes = {row['Mesh']}
            if row['Mesh'] in ('P_Level1MountainClouds', 'P_Level1MountainClouds2'):
                meshes.update(('P_Level1MountainClouds', 'P_Level1MountainClouds2'))
            matches = [r for r in source_meshes if r['Mesh'] in meshes
                       and params(r).get('tex') == day.get('tex') and 'mltColor' in params(r)]
            if palette:
                # Chris has one warm, lamp-lit mist; it is local to that spot and must not spread across a whole level.
                matches = [r for r in matches if params(r)['mltColor'][2] >= params(r)['mltColor'][0]]
            if matches:
                position = [row['Transformation'][12+i]*.5+offset[i] for i in range(3)]
                if not palette:
                    match = min(matches, key=lambda r: sum((r['Transformation'][12+i]-position[i])**2 for i in (0,2)))
                else:
                    match = min(matches, key=lambda r: abs(params(r).get('alpha', 0)-day.get('alpha', 0)))
                night = params(match)
                values = {k: dict(day=day[k], night=night[k]) for k in ('mltColor', 'color') if k in day and k in night}
                if 'cloud' in row['Mesh'].lower() and row['Shader'].startswith('Scattering'):
                    values.update({k: dict(day=day[k], night=night[k]) for k in ('alpha', 'scatter') if k in day and k in night})
                if palette and 'mltColor' in values:
                    values['mltColor']['night'] = dim_to_day(values['mltColor']['night'], day['mltColor'])
                if values: clouds[key] = values
            elif palette and 'mltColor' in day and 'lightning' not in row['Mesh'].lower():
                clouds[key] = dict(mltColor=dict(day=day['mltColor'], night=cool_cloud(day['mltColor'], tint)))
        if ((target == 'Graveyard' and row['Mesh'] in ('P_FlatMist', 'P_DomeMist', 'Level3_M_GraveyardDust', 'P_MistCardSphere'))
                or (target == 'Barrens' and row['Mesh'] in ('Level3_M_GraveyardDust', 'P_MistCardSphere'))):
            clouds.setdefault(key, {})['alpha'] = dict(day=day['alpha'], night=0)
        if target == 'Graveyard' and row['Mesh'] in ('P_ShadowPlane', 'P_GraveyardGateRay', 'P_GraveyardGateSphinxRay'):
            tint = params(decorations('Level_Chris')['666793bf1c8c211598a3e391a1d5efb8'])['mltColor']
            color = day['mltColor']
            ray_luminance = sum(a*b for a,b in zip(color, (.2126, .7152, .0722)))
            night = tuple(ray_luminance*c/max(tint[:3]) for c in tint[:3]) + (color[3],)
            clouds.setdefault(key, {})['mltColor'] = dict(day=color, night=night)
    emitters = [copy.deepcopy(r) for r in records('Level_'+source,'ParticleEmitter').values()
                if r['EmitterType'] in ('Stars', 'StarsBright', 'StarsDim', 'ShootingStars')]
    if len(records('Level_'+target,'ParticleEmitter'))+len(emitters)>512:
        raise ValueError('Native environment emitter capacity exceeded')
    for r in emitters:
        if not r['FollowCamera']: raise ValueError('Night emitter needs a verified world-space mapping')
        r['ObjectName'] = 'JNETNight_' + r['EmitterType']
    return dict(nodes=nodes, nodeCount=len(nodes), clouds=clouds, suns=suns, emitters=emitters, moon=source == 'Chris')


def profiles():
    moon = params(decorations('Level_Chris')['484482c9a85f8204dae25eeee89cdf8f'])
    tint = params(decorations('Level_Chris')['666793bf1c8c211598a3e391a1d5efb8'])['mltColor']
    output = []
    for target, source, offset, anchors in MAPPINGS:
        entry = profile(target, source, offset, anchors, moon, tint)
        if target in ALTERNATES:
            entry['alternate'] = profile(target, *ALTERNATES[target], moon, tint)
        output.append(entry)
    return output


# Every Lua allocation in the game comes from fixed pools (Journey.exe table at RVA 0x5C38F0) of which the large classes hold only 8 to 16
# blocks, and a full class is fatal. Compiling every level's profile as one chunk needed blocks of 128 KB and 256 KB, so the runtime now
# installs only a small index and later loads one level's data as many parts, each small enough for the 4096-byte class (128 blocks).
PART_LIMIT = 1200


def split_parts(entries):
    parts, current, size = [], {}, 0
    for key, value in sorted(entries.items()):
        weight = len(lua_literal(value)) + len(key) + 8
        if current and size + weight > PART_LIMIT:
            parts.append(current)
            current, size = {}, 0
        current[key] = value
        size += weight
    if current: parts.append(current)
    return parts


def level_parts(level, profile):
    parts = []
    for alt, data in ((0, profile), (1, profile.get('alternate'))):
        if data is None: continue
        parts.append('__JNETDayNightPart(%d,%d,"meta",%s)' % (level, alt, lua_literal(dict(moon=data['moon']))))
        for kind in ('nodes', 'clouds', 'suns'):
            for part in split_parts(data[kind]):
                parts.append('__JNETDayNightPart(%d,%d,"%s",%s)' % (level, alt, kind, lua_literal(part)))
    return parts


def runtime_payload(built=None):
    built = profiles() if built is None else built
    emitters = built[0]['emitters']
    if any(profile['emitters'] != emitters for profile in built):
        raise ValueError('Night emitters differ between levels; the shared star definition would be wrong')
    index = ','.join('{nodeCount=%d,nodes={%s},emitters=shared}' % (
        profile['nodeCount'], ','.join('[%s]=true' % lua_literal(key) for key in sorted(profile['nodes']))) for profile in built)
    source = 'local shared=' + lua_literal(emitters) + '\nlocal profiles={' + index + '}\n' + (LAB/'diagnostic/jnet_daynight.lua').read_text()
    return dict(source=source, parts=[level_parts(level, profile) for level, profile in enumerate(built)])


def runtime_header():
    payload = runtime_payload()
    header = 'static const char jnet_daynight_source[]=\n' + '\n'.join(json.dumps(line+'\n') for line in payload['source'].splitlines()) + ';\n'
    for level, parts in enumerate(payload['parts']):
        header += 'static const char *const jnet_daynight_parts_%d[]={\n' % level + ''.join(json.dumps(part) + ',\n' for part in parts) + '0};\n'
    header += '#define JNET_DAYNIGHT_LEVELS %d\nstatic const char *const *const jnet_daynight_parts[JNET_DAYNIGHT_LEVELS]={%s};\n' % (
        len(payload['parts']), ','.join('jnet_daynight_parts_%d' % level for level in range(len(payload['parts']))))
    return header
