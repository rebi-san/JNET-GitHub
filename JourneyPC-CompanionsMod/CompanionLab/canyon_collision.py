import gzip
import hashlib
import json
import math
from pathlib import Path
import re
import struct

LAB = Path(__file__).resolve().parent
ROOT = LAB.parent.parent
WORK = LAB / 'work'
THICKNESS = 0.01
COLLISION_LEVELS = {'Canyon': 3, 'Barrens': 1, 'Desert': 2, 'Cave': 4, 'Mountain': 6}


def source_instances(level='Canyon'):
    if level not in COLLISION_LEVELS:
        raise ValueError('Unknown collision level: ' + level)
    scene = json.loads((WORK / f'{level.lower()}-collision-scene.json').read_text())
    text = (ROOT / f'JourneyIOS/Data/Scripts/Level_{level}/DecorationMeshInstances.lua').read_text()
    binary = (ROOT / f'JourneyPC/Data/Scripts/Level_{level}/DecorationMeshInstances.lua.bin').read_bytes()
    rows = {m[1]:m[2] for m in re.finditer(r'ObjectName = "([0-9a-f]+)"(.*?)(?=ObjectName =|\Z)', text, re.S)}
    result = []
    for obj in scene['objects']:
        key = obj['props']['journey_source_id']
        row = rows[key]
        mesh = re.search(r'Mesh = "([^"]+)"', row)[1]
        if mesh != obj['props']['journey_source_mesh'] or binary.count(key.encode()) != 1:
            raise ValueError(level + ' decoration identity mismatch: ' + key)
        values = [float(v) for v in re.findall(r'-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?',
                  re.search(r'Transformation = (.*)', row)[1])]
        if len(values) != 16:
            raise ValueError('Invalid decoration transform')
        # PC serializes the same matrix with signed zeros and float rounding.
        candidates = []
        for offset in range(max(0,binary.index(key.encode())-0x840), binary.index(key.encode()), 4):
            pc = struct.unpack_from('<16f', binary, offset)
            if all(math.isfinite(v) and abs(v-values[i]) <= 0.0001 for i,v in enumerate(pc)):
                candidates.append((offset,pc))
        if len(candidates) != 1:
            raise ValueError(f'Ambiguous PC decoration matrix: {key}: {len(candidates)}')
        matrix = candidates[0][1]
        expected = (matrix[14],matrix[12],matrix[13])
        if any(abs(obj['matrix'][i][3]-expected[i]) > 0.002 for i in range(3)):
            raise ValueError('Blender/source placement differs: ' + obj['name'])
        if any(abs(obj['matrix'][i][j]-matrix[j*4+a]) > 0.0002
               for i,a in enumerate((2,0,1)) for j in range(3)):
            raise ValueError('Blender/source rotation or scale differs: ' + obj['name'])
        result.append(dict(name=obj['name'],id=key,mesh=mesh,matrix=matrix))
    if (not result or len(result) != len(scene['requested']) or
            len({o['id'] for o in result}) != len(result) or
            {o['name'] for o in result} != set(scene['requested'])):
        raise ValueError(f'Expected one unique {level} source instance per requested object')
    return result


def transform(matrix, v):
    return tuple(sum(matrix[j*4+a]*v[j] for j in range(3))+matrix[12+a] for a in range(3))


def triangle_area_squared(t):
    u = [t[1][a]-t[0][a] for a in range(3)]
    v = [t[2][a]-t[0][a] for a in range(3)]
    return sum((u[(a+1)%3]*v[(a+2)%3]-u[(a+2)%3]*v[(a+1)%3])**2 for a in range(3))


def build(level='Canyon'):
    if level not in COLLISION_LEVELS:
        raise ValueError('Unknown collision level: ' + level)
    with gzip.open(WORK / f'{level.lower()}-collision-meshes.json.gz','rt') as f:
        meshes = json.load(f)
    instances = source_instances(level)
    triangles, bounds, sources = [], [], []
    skipped = 0
    for owner,obj in enumerate(instances):
        mesh = meshes[obj['mesh']]
        vertices = [struct.unpack('<3f',struct.pack('<3f',*transform(obj['matrix'],v))) for v in mesh['vertices']]
        seen = set()
        start = len(triangles)
        for face in mesh['faces']:
            if len(face) != 3:
                raise ValueError('Reference mesh needs explicit source triangulation: ' + obj['name'])
            tri = tuple(vertices[i] for i in face)
            key = tuple(sorted(tri))
            if key in seen or triangle_area_squared(tri) < 1e-10:
                skipped += 1
                continue
            seen.add(key)
            triangles.append((tri,owner))
            bounds.append(tuple(min(v[a] for v in tri)-THICKNESS for a in range(3))+
                          tuple(max(v[a] for v in tri)+THICKNESS for a in range(3)))
        sources.append({**obj,'triangles':len(triangles)-start})
    nodes, ordered = [], []

    def node(indices, depth=0):
        if depth > 30:
            raise ValueError('Collision BVH too deep')
        box = tuple(min(bounds[i][a] for i in indices) for a in range(3)) + tuple(max(bounds[i][a] for i in indices) for a in range(3,6))
        index = len(nodes)
        nodes.append(None)
        if len(indices) <= 8:
            first = len(ordered)
            ordered.extend(indices)
            nodes[index] = (*box,first,len(indices))
        else:
            axis = max(range(3),key=lambda a:box[a+3]-box[a])
            indices.sort(key=lambda i:bounds[i][axis]+bounds[i][axis+3])
            middle = len(indices)//2
            left,right = node(indices[:middle],depth+1),node(indices[middle:],depth+1)
            nodes[index] = (*box,left,0x80000000|right)
        return index

    node(list(range(len(triangles))))
    payload = bytearray(struct.pack('<8sIIII',b'JNCANY1\0',len(nodes),len(triangles),len(instances),0))
    for n in nodes:
        payload.extend(struct.pack('<6fII',*n))
    for i in ordered:
        tri,owner = triangles[i]
        payload.extend(struct.pack('<9fI',*(v for p in tri for v in p),owner))
    report = dict(level='Level_'+level,level_index=COLLISION_LEVELS[level],
                  instances=sources,triangle_count=len(triangles),node_count=len(nodes),
                  duplicate_or_degenerate_triangles_skipped=skipped,thickness=THICKNESS,
                  geometry_source='Blender MCP scene snapshot; source IDs and placements matched to PC decoration records',
                  runtime_verified=False)
    return bytes(payload),report


def runtime_header(payloads):
    header = ('struct jnet_collision_resource {\n'
              'const wchar_t *filename; const char *name; int level;\n'
              'uint32_t bytes,nodes,triangles,instances; unsigned char hash[32];\n'
              '};\nstatic const struct jnet_collision_resource jnet_collision_resources[]={\n')
    for level,payload in payloads.items():
        nodes,triangles,instances,reserved = struct.unpack_from('<4I',payload,8)
        if (level not in COLLISION_LEVELS or payload[:8] != b'JNCANY1\0' or reserved or
                not nodes or not triangles or not instances or len(payload) != 24+nodes*32+triangles*40):
            raise ValueError('Invalid collision resource: ' + level)
        digest = ','.join(map(str,hashlib.sha256(payload).digest()))
        header += (f'{{L"JNET-{level.lower()}-collision.bin","{level}",{COLLISION_LEVELS[level]},'
                   f'{len(payload)}u,{nodes}u,{triangles}u,{instances}u,{{{digest}}}}},\n')
    if not payloads:
        raise ValueError('No collision resources')
    return header+'};\n'


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--level',choices=tuple(COLLISION_LEVELS),default='Canyon')
    level = parser.parse_args().level
    payload,report = build(level)
    (WORK / f'{level.lower()}-collision.bin').write_bytes(payload)
    (WORK / f'{level.lower()}-collision-audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='instances'},indent=2))
