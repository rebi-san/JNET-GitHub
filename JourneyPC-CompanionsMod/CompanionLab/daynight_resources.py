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


MAPPINGS = (
    ('Graveyard', 'Chris', (0, 0, 0), ('37cc2e82e213ce8eedbf88961ed6325c', '99440de91d6ef6ee642f18ad6089fcb8')),
    ('Barrens', 'Chris', (256, 0, 512), ('b4ad6f584115d4b81fb5b91c022aa353', 'c673990a6fbd41b3cffb104e0d0e9b4a')),
    ('Desert', 'Matt', (0, 0, 0), ('31f04f52bbfdd3610458c25d5ec5bb87', '4a573e76ffd62db7001bb35504220458', '76a4e0c7aede7544a493a442d1cc0637', 'ecb314f38fdc7e2df7cb0a58e2091e94')),
    ('Canyon', 'Matt', (256, -200, 512), ('f6f6c406c04d3c1339585aa9444e9a70', 'e26584b87c4ffef7c0cb1b0c6cbf07d4', '06d51a05b13030319bd6c12e7d5d8317', 'c01c13593c03f46a3e464ba722226169')),
    ('Cave', None, None, ()), ('Ruins', None, None, ()),
    ('Mountain', None, None, ()), ('Summit', None, None, ()),
)
FIELDS = ('FogCloseColor', 'FogFarColor', 'FogCloseAlpha', 'FogFarAlpha',
          'SkyMinColor', 'SkyLowColor', 'SkyMidColor', 'SkyHighColor', 'SkyMaxColor',
          'SkyLowHeight', 'SkyMidHeight', 'SkyHighHeight', 'ShadowLength', 'ShadowAlpha',
          'BloomSize', 'BloomIntensity', 'BloomOverbright', 'ChromaticAberration', 'VelvetRatio', 'VelvetInt',
          'Env_LightDir', 'Env_LightColor', 'Env_LightInt', 'Env_AmbientColor', 'Env_AmbientInt',
          'Env_SpecularColor', 'Env_SpecularInt', 'Env_SpecularPower')


def params(row):
    return {p['ParamName']: p['ParamVal'] for p in row['ShaderParams']}


def profiles():
    source = json.loads((LAB/'daynight_profiles.json').read_text())
    output = copy.deepcopy(source['profiles'])
    for index, profile in enumerate(output):
        level = 'Level_' + MAPPINGS[index][0]
        nodes = records(level, 'EnvNode')
        meshes = decorations(level)
        if set(profile['nodes']) != set(nodes):
            raise ValueError('Night profile environment mismatch: ' + level)
        profile['nodeCount'] = len(nodes)
        profile['emitters'] = copy.deepcopy(source['emitters'])
        for variant in (profile, profile.get('alternate')):
            if variant is None: continue
            if not set(variant['clouds']).union(variant['suns']).issubset(meshes):
                raise ValueError('Night profile mesh mismatch: ' + level)
            if variant is not profile: variant['emitters'] = []
        existing = {r['EmitterType'] for r in records(level, 'ParticleEmitter').values()}
        if len(records(level, 'ParticleEmitter')) + sum(e['EmitterType'] not in existing for e in profile['emitters']) > 512:
            raise ValueError('Native environment emitter capacity exceeded')
    summit = output[7]
    summit['suns'] = {}
    for name, row in decorations('Level_Summit').items():
        values = params(row)
        if values.get('tex') in ('Sun', 'Moon'):
            summit['suns'][name] = dict(day=values, night=values, separate=values['tex'])
    for name, phase in (('6138d402a31a69498e14158615877fc5', 'Sun'),
                        ('50806e6c9116663ca29ca8ba955813ef', 'Moon')):
        row = decorations('Level_Summit')[name]
        if row['Mesh'] != 'P_SkyRamp':
            raise ValueError('Summit sky ramp mismatch')
        values = params(row)
        summit['clouds'].pop(name, None)
        summit['suns'][name] = dict(day=values, night=values, separate=phase)
    for name in ('c8a3854cb941f5b7bd82df63e0142f5a',
                 'f281fa80e26c5507b8be57bb6c447200',
                 '5fd6b7b0c8eef88daaeda898ce399893',
                 '570c221cd2f9048548590784dd5b92ff',
                 'd0bcda556118028a6ff1dd52de6ea5b1',
                 '34b9219071a7c1eaa85c2bae204f2e98',
                 '9467edcaf435a69e22e2d37d2a189076'):
        row = decorations('Level_Summit')[name]
        if row['Mesh'] != 'P_ShadowPlane':
            raise ValueError('Summit shadow plane mismatch')
        values = params(row)
        summit['clouds'].pop(name, None)
        summit['suns'][name] = dict(day=values, night=values, separate='Sun')
    return output


def runtime_source():
    data = profiles()
    index = [dict(nodeCount=p['nodeCount'], nodes={k: True for k in p['nodes']}, emitters=p['emitters']) for p in data]
    return 'local profiles=' + lua_literal(index) + '\n' + (LAB/'diagnostic/jnet_daynight.lua').read_text()


def profile_parts():
    result = []
    for level, profile in enumerate(profiles()):
        parts = []
        for alt, variant in enumerate((profile, profile.get('alternate'))):
            if variant is None: continue
            parts.append(f'__JNETDayNightPart({level},{alt},"meta",' + lua_literal(dict(moon=variant['moon'])) + ')')
            for kind in ('nodes', 'clouds', 'suns'):
                for key, value in variant[kind].items():
                    parts.append(f'__JNETDayNightPart({level},{alt},"{kind}",' + lua_literal({key: value}) + ')')
        result.append(parts)
    return result


def runtime_header():
    source = runtime_source()
    header = 'static const char jnet_daynight_source[]=\n' + '\n'.join(json.dumps(line+'\n') for line in source.splitlines()) + ';\n'
    parts = profile_parts()
    for level, rows in enumerate(parts):
        header += f'static const char *const jnet_daynight_parts_{level}[]={{\n' + ',\n'.join(json.dumps(row) for row in rows) + '};\n'
    header += 'static const struct {const char *const *parts; unsigned count;} jnet_daynight_data[]={\n'
    header += ',\n'.join(f'{{jnet_daynight_parts_{level},{len(rows)}}}' for level, rows in enumerate(parts)) + '};\n'
    return header
