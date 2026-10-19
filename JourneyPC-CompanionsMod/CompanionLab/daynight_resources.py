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
    output = []
    moon_mesh = decorations('Level_Chris')['484482c9a85f8204dae25eeee89cdf8f']
    moon = params(moon_mesh)
    for target, source, offset, anchors in MAPPINGS:
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
        source_meshes = list(decorations('Level_' + source).values())
        clouds, suns = {}, {}
        for key, row in decorations('Level_' + target).items():
            day = params(row)
            if day.get('tex') == 'Sun':
                night = moon if source == 'Chris' else day
                suns[key] = dict(day=day, night=night)
            if any(word in row['Mesh'].lower() for word in ('cloud', 'dust', 'mist', 'wind', 'steam', 'storm')):
                meshes = {row['Mesh']}
                if row['Mesh'] in ('P_Level1MountainClouds', 'P_Level1MountainClouds2'):
                    meshes.update(('P_Level1MountainClouds', 'P_Level1MountainClouds2'))
                matches = [r for r in source_meshes if r['Mesh'] in meshes
                           and params(r).get('tex') == day.get('tex') and 'mltColor' in params(r)]
                if matches:
                    position = [row['Transformation'][12+i]*.5+offset[i] for i in range(3)]
                    match = min(matches, key=lambda r: sum((r['Transformation'][12+i]-position[i])**2 for i in (0,2)))
                    night = params(match)
                    values = {k: dict(day=day[k], night=night[k]) for k in ('mltColor', 'color') if k in day and k in night}
                    if 'cloud' in row['Mesh'].lower() and row['Shader'].startswith('Scattering'):
                        values.update({k: dict(day=day[k], night=night[k]) for k in ('alpha', 'scatter') if k in day and k in night})
                    if values: clouds[key] = values
            if ((target == 'Graveyard' and row['Mesh'] in ('P_FlatMist', 'P_DomeMist', 'Level3_M_GraveyardDust', 'P_MistCardSphere'))
                    or (target == 'Barrens' and row['Mesh'] in ('Level3_M_GraveyardDust', 'P_MistCardSphere'))):
                clouds.setdefault(key, {})['alpha'] = dict(day=day['alpha'], night=0)
            if target == 'Graveyard' and row['Mesh'] in ('P_ShadowPlane', 'P_GraveyardGateRay', 'P_GraveyardGateSphinxRay'):
                tint = params(decorations('Level_Chris')['666793bf1c8c211598a3e391a1d5efb8'])['mltColor']
                color = day['mltColor']
                luminance = sum(a*b for a,b in zip(color, (.2126, .7152, .0722)))
                night = tuple(luminance*c/max(tint[:3]) for c in tint[:3]) + (color[3],)
                clouds.setdefault(key, {})['mltColor'] = dict(day=color, night=night)
        emitters = [copy.deepcopy(r) for r in records('Level_'+source,'ParticleEmitter').values()
                    if r['EmitterType'] in ('Stars', 'StarsBright', 'StarsDim', 'ShootingStars')]
        if len(records('Level_'+target,'ParticleEmitter'))+len(emitters)>512:
            raise ValueError('Native environment emitter capacity exceeded')
        for r in emitters:
            if not r['FollowCamera']: raise ValueError('Night emitter needs a verified world-space mapping')
            r['ObjectName'] = 'JNETNight_' + r['EmitterType']
        output.append(dict(nodes=nodes, nodeCount=len(nodes), clouds=clouds, suns=suns, emitters=emitters, moon=source == 'Chris' and bool(suns)))
    return output


def runtime_header():
    source = 'local profiles=' + lua_literal(profiles()) + '\n' + (LAB/'diagnostic/jnet_daynight.lua').read_text()
    return 'static const char jnet_daynight_source[]=\n' + '\n'.join(json.dumps(line+'\n') for line in source.splitlines()) + ';\n'
