"""Extracts every Flower flower (meshes, textures, shaders, colours) from the FlowerPC data into open files.

Flower stores its petal meshes as Lua morph-target tables (FlowerMeshes.lua), the flower recipes as layer tables
(FlowerDefs.lua) and builds each flower in native code. This script reads the tables literally (no Lua is executed) and writes:

  work/flower/flowers.json            every flower type, layer, colour, atlas cell, stem and glow parameter
  work/flower/meshes/<mesh>_<frame>.obj   each petal/bud/stem-tendril mesh at every morph frame
  work/flower/flowers/<name>.obj      each flower assembled at full bloom (vertex colours as `v x y z r g b`)
  work/flower/textures/               petalAtlas.dds, which every petal and stem samples through its atlas cell
  work/flower/shaders/                the compiled D3D shaders Flower uses for petals, stems, glow, grass and swarms

The assembly transform order is an approximation of the native flower builder (not decompiled): per layer the petal is scaled, tilted
by rotBloom, offset, then spun around the vertical axis in equal steps. Verify against the original game before relying on exact poses.
"""
import hashlib
import json
import math
import re
import shutil
import struct
from pathlib import Path

from character_resources import LiteralReader

LAB = Path(__file__).resolve().parent
ROOT = LAB.parent.parent
FLOWER = ROOT / 'FlowerPC'
OUTPUT = LAB / 'work/flower'
SHADERS = ('PetalInstance', 'PetalInstanceSolid', 'StemInstance', 'FlowerBB', 'FlowerBlur', 'FlowerHalo', 'Glow',
           'FillerFlower', 'FillerFlowerRipple', 'Grass', 'GrassRipple', 'PetalSwarm', 'PetalSwarmBurn', 'PetalSwarmSolid')
ATLAS = 'petalAtlas.dds'


class FlowerReader(LiteralReader):
    allow_duplicates = True


def read_table(name, variable):
    """One top-level `variable = {...}` assignment; other assignments in the file are not read."""
    text = (FLOWER/'Data/Scripts'/name).read_text()
    start = re.search(r'^' + variable + r'\s*=', text, re.M).start()
    end = re.compile(r'^[A-Za-z_]\w*\s*=', re.M).search(text, text.index('\n', start))
    return FlowerReader(text[start:end.start() if end else len(text)]).assignment(variable)


def meshes():
    """{name: {'frames': {frame: [(pos, normal, uv)]}, 'indices': [..]}}; frames keep Flower's sorted order."""
    result = {}
    for entry in read_table('FlowerMeshes.lua', 'flowerMeshes'):
        name, indices, frames = entry[0], entry[-1], {}
        for frame in entry[1:-1]:
            frames[frame[0]] = [(tuple(v[0:3]), tuple(v[3:6]), tuple(v[6:8])) for v in frame[1:]]
        if len({len(v) for v in frames.values()}) != 1 or max(indices) >= len(next(iter(frames.values()))):
            raise ValueError('Inconsistent flower mesh ' + name)
        result[name] = dict(frames=dict(sorted(frames.items())), indices=indices)
    return result


def atlas():
    return read_table('FlowerDefs.lua', 'petalCatalog')


def flowers():
    return [dict(f, index=i) for i, f in enumerate(read_table('FlowerDefs.lua', 'flowerData'))]


def triple(value, default):
    if value is None: return tuple(default)
    return (value,)*3 if isinstance(value, (int, float)) else tuple(value)


def rotation(x, y, z):
    x, y, z = (math.radians(a) for a in (x, y, z))
    cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
    rx = ((1, 0, 0), (0, cx, -sx), (0, sx, cx))
    ry = ((cy, 0, sy), (0, 1, 0), (-sy, 0, cy))
    rz = ((cz, -sz, 0), (sz, cz, 0), (0, 0, 1))
    def mul(a, b): return tuple(tuple(sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)) for i in range(3))
    return mul(rz, mul(ry, rx))


def apply(matrix, vector):
    return tuple(sum(matrix[i][j]*vector[j] for j in range(3)) for i in range(3))


def atlas_uv(uv, cell, grid):
    return ((cell[0] + uv[0])*grid['gridWidth']/grid['atlasWidth'], (cell[1] + uv[1])*grid['gridWidth']/grid['atlasHeight'])


def bloom_frame(mesh):
    return next(reversed(mesh['frames']))


def closed_frame(mesh):
    return next(iter(mesh['frames']))


def layer_pose(layer, closed):
    """The layer's placement for one pose. Flower gives separate *UnBloom values for the closed bud; any it omits are shared."""
    def pick(bloom, unbloom):
        value = layer.get(unbloom) if closed else None
        return layer.get(bloom) if value is None else value
    if closed: tilt = layer.get('rotUnBloom') or layer.get('rot') or layer.get('rotBloom') or (0, 0, 0)
    else: tilt = layer.get('rotBloom') or layer.get('rot') or (0, 0, 0)
    return dict(scale=triple(pick('scale', 'scaleUnBloom'), (1, 1, 1)), tilt=tuple(tilt),
                offset=triple(pick('offset', 'offsetUnBloom'), (0, 0, 0)),
                base=tuple(c/255 for c in pick('color0', 'color0UnBloom')),
                tip=tuple(c/255 for c in pick('color1', 'color1UnBloom')))


def layer_geometry(layer, mesh, grid, closed=False):
    """Vertices (position, normal, colour, atlas uv) and triangles for one layer in full bloom (or as the closed bud), every petal copy."""
    frame = mesh['frames'][closed_frame(mesh) if closed else bloom_frame(mesh)]
    pose = layer_pose(layer, closed)
    tilt_matrix = rotation(*pose['tilt'])
    scale, offset, base, tip = pose['scale'], pose['offset'], pose['base'], pose['tip']
    heights = [v[0][1] for v in frame]
    low, high = min(heights), max(heights)
    cell = grid[layer.get('texture') or 'Default']
    count = layer['numPetals']
    vertices, triangles = [], []
    for petal in range(count):
        spin = rotation(0, 360*petal/count if count > 1 else 0, 0)
        first = len(vertices)
        for position, normal, uv in frame:
            local = tuple(a + b for a, b in zip(apply(tilt_matrix, tuple(p*s for p, s in zip(position, scale))), offset))
            t = (position[1] - low)/(high - low) if high > low else 0
            vertices.append((apply(spin, local), apply(spin, apply(tilt_matrix, normal)),
                             tuple(a + (b - a)*t for a, b in zip(base, tip)), atlas_uv(uv, cell, grid)))
        triangles += [tuple(first + i for i in mesh['indices'][n:n + 3]) for n in range(0, len(mesh['indices']), 3)]
    return vertices, triangles


def obj_text(groups, comment, material=None):
    lines = ['# ' + comment]
    if material: lines.append('mtllib ' + material)
    offset = 0
    for name, vertices, triangles in groups:
        lines.append('o ' + name)
        if material: lines.append('usemtl FlowerAtlas')
        lines += ['v %.6g %.6g %.6g' % v[0] + (' %.5g %.5g %.5g' % v[2] if len(v) > 3 else '') for v in vertices]
        lines += ['vt %.6g %.6g' % (v[-1][0], 1 - v[-1][1]) for v in vertices]
        lines += ['vn %.6g %.6g %.6g' % v[1] for v in vertices]
        for a, b, c in triangles:
            lines.append('f ' + ' '.join('%d/%d/%d' % ((i + offset + 1,)*3) for i in (a, b, c)))
        offset += len(vertices)
    return '\n'.join(lines) + '\n'


def mesh_obj(name, frame, mesh):
    vertices = mesh['frames'][frame]
    triangles = [tuple(mesh['indices'][i:i + 3]) for i in range(0, len(mesh['indices']), 3)]
    return obj_text([(name + '_' + frame, vertices, triangles)],
                    'Flower petal mesh %s, morph frame %s' % (name, frame))


def flower_obj(flower, mesh_table, grid):
    groups = []
    for number, layer in enumerate(flower['petalLayers']):
        vertices, triangles = layer_geometry(layer, mesh_table[layer['meshName']], grid)
        groups.append(('%s_layer%d_%s' % (flower['flowerName'], number, layer['meshName']), vertices, triangles))
    return obj_text(groups, 'Flower "%s" assembled at full bloom (approximate pose)' % flower['flowerName'], 'flowers.mtl')


def manifest(mesh_table, grid):
    result = dict(atlas=dict(file='textures/' + ATLAS, **grid), shaders=shader_files(), flowers=[], meshes={
        name: dict(frames=list(mesh['frames']), vertices=len(next(iter(mesh['frames'].values()))), triangles=len(mesh['indices'])//3)
        for name, mesh in mesh_table.items()})
    for flower in flowers():
        result['flowers'].append({k: v for k, v in flower.items()})
    return result


def shader_files():
    found = []
    for name in SHADERS:
        for stage in ('vs', 'fs'):
            path = FLOWER/'Data/Shaders/Release'/f'{name}_{stage}.cso'
            if path.exists():
                found.append(dict(name=name, stage=stage, file=f'shaders/{name}_{stage}.cso', bytes=path.stat().st_size,
                                  sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    return found


# Runtime model: the in-game flowers are far smaller than Flower's 3.6-unit heads, so geometry is scaled once here.
WORLD_UNIT = .15
STEM_HEIGHT = 3.2
STEM_SIDES = 6
STEM_RADII = (.16, .09)
# Vertex kinds, stored in the colour alpha: stems are solid atlas-cell colour, petals are cut out by the atlas alpha, grass is cut out
# by the grass texture alpha.
KIND_STEM, KIND_PETAL, KIND_GRASS = 0, 255, 128
GRASS_FILE = 'GrassClump.dds'
GRASS_CARDS = 7
GRASS_CELLS = ((0, 0), (2, 0), (0, 2), (2, 2), (1, 2))


def stem_geometry(flower, grid, height):
    """A six-sided tapered tube from stemBaseColor to stemTipColor, sampled from the stem's atlas cell."""
    cell = grid[flower.get('stemTexture') or 'StemDefault']
    uv = atlas_uv((.5, .5), cell, grid)
    base, tip = (tuple(c/255 for c in flower[k]) for k in ('stemBaseColor', 'stemTipColor'))
    vertices, triangles = [], []
    for ring, radius in enumerate(STEM_RADII):
        for side in range(STEM_SIDES):
            angle = 2*math.pi*side/STEM_SIDES
            direction = (math.cos(angle), 0, math.sin(angle))
            vertices.append(((direction[0]*radius, height*ring, direction[2]*radius), direction,
                             base if ring == 0 else tip, uv))
    for side in range(STEM_SIDES):
        a, b = side, (side + 1) % STEM_SIDES
        triangles += [(a, b, STEM_SIDES + b), (a, STEM_SIDES + b, STEM_SIDES + a)]
    return vertices, triangles


def grass_geometry():
    """A ring of crossed grass cards around the stem, cut from Flower's GrassClump sheet (a 3x3 grid of blade silhouettes)."""
    vertices, triangles = [], []
    for card in range(GRASS_CARDS):
        angle = 2*math.pi*card/GRASS_CARDS + .35*math.sin(card*2.7)
        radius = .35 + .3*((card*5) % 3)/2
        width, height = 1.0 + .35*((card*3) % 4)/3, 1.5 + .7*((card*7) % 5)/4
        centre = (math.cos(angle)*radius, 0, math.sin(angle)*radius)
        turn = angle + math.pi/2 + .5*math.sin(card*1.9)
        along = (math.cos(turn), 0, math.sin(turn))
        lean = (math.cos(angle)*.3, 0, math.sin(angle)*.3)
        cell = GRASS_CELLS[card % len(GRASS_CELLS)]
        u0, u1, v0, v1 = cell[0]/3 + .015, (cell[0] + 1)/3 - .015, cell[1]/3 + .015, (cell[1] + 1)/3 - .015
        shade = .85 + .15*math.sin(card*3.1)
        base = (.10*shade, .30*shade, .05*shade)
        tip = (.42*shade, .72*shade, .20*shade)
        first = len(vertices)
        for x, y, u, v in ((-1, 0, u0, v1), (1, 0, u1, v1), (1, 1, u1, v0), (-1, 1, u0, v0)):
            position = tuple(centre[i] + along[i]*x*width/2 + lean[i]*y for i in range(3))
            vertices.append(((position[0], height*y, position[2]), (0, 1, 0), tip if y else base, (u, v)))
        triangles += [(first, first + 1, first + 2), (first, first + 2, first + 3)]
    return vertices, triangles


def flower_model(flower, mesh_table, grid):
    """(vertices, triangles) of one flower standing on y=0, in world units. A vertex is (position, normal, colour, uv, kind, closed
    position, closed colour): the bloom pose plus the closed bud it morphs from. Stem, petal layers, then the grass ring."""
    height = STEM_HEIGHT*(flower.get('heightMult') or 1)
    vertices, triangles = [], []
    stem, stem_triangles = stem_geometry(flower, grid, height)
    vertices += [(p, n, c, uv, KIND_STEM, p, c) for p, n, c, uv in stem]
    triangles += stem_triangles
    for layer in flower['petalLayers']:
        mesh = mesh_table[layer['meshName']]
        opened, layer_triangles = layer_geometry(layer, mesh, grid)
        closed, _ = layer_geometry(layer, mesh, grid, closed=True)
        first = len(vertices)
        for (p, n, c, uv), (p0, _, c0, _) in zip(opened, closed):
            vertices.append(((p[0], p[1] + height, p[2]), n, c, uv, KIND_PETAL, (p0[0], p0[1] + height, p0[2]), c0))
        triangles += [tuple(first + i for i in t) for t in layer_triangles]
    grass, grass_triangles = grass_geometry()
    first = len(vertices)
    vertices += [(p, n, c, uv, KIND_GRASS, p, c) for p, n, c, uv in grass]
    triangles += [tuple(first + i for i in t) for t in grass_triangles]
    vertices = [(tuple(a*WORLD_UNIT for a in p), n, c, uv, kind, tuple(a*WORLD_UNIT for a in p0), c0)
                for p, n, c, uv, kind, p0, c0 in vertices]
    if len(vertices) > 65535: raise ValueError('Flower model exceeds 16-bit indices: ' + flower['flowerName'])
    return vertices, triangles


GRASS_SIZE = 256


def grass_dds():
    """JNET-grass.dds: GrassClump.dds halved to 256x256 with a full mip chain (the original has none), as BGRA32."""
    data = (FLOWER/'Data/Textures'/GRASS_FILE).read_bytes()
    height, width = struct.unpack_from('<II', data, 12)
    if (width, height, struct.unpack_from('<I', data, 88)[0]) != (512, 512, 32): raise ValueError('Unexpected grass texture')
    level, size = bytes(data[128:128 + 512*512*4]), 512
    levels = []
    while size > 1:
        half = size//2
        out = bytearray(half*half*4)
        for y in range(half):
            for x in range(half):
                for channel in range(4):
                    i = ((2*y)*size + 2*x)*4 + channel
                    out[(y*half + x)*4 + channel] = (level[i] + level[i + 4] + level[i + size*4] + level[i + size*4 + 4] + 2)//4
        level, size = bytes(out), half
        levels.append(level)
    header = bytearray(128)
    header[0:4] = b'DDS '
    struct.pack_into('<IIIIIII', header, 4, 124, 0x1007 | 0x20000 | 0x80000, GRASS_SIZE, GRASS_SIZE, GRASS_SIZE*GRASS_SIZE*4, 0, len(levels))
    struct.pack_into('<IIII', header, 76, 32, 0x41, 0, 32)
    struct.pack_into('<IIII', header, 92, 0xff0000, 0xff00, 0xff, 0xff000000)
    struct.pack_into('<I', header, 108, 0x401008)
    return bytes(header) + b''.join(levels)


def runtime_header():
    """C header with every flower's mesh as one shared vertex/index list. The 11 MB petal atlas and the grass sheet ship as loose files."""
    mesh_table, grid = meshes(), atlas()
    vertices, indices, models = [], [], []
    for flower in flowers():
        model_vertices, triangles = flower_model(flower, mesh_table, grid)
        models.append((len(vertices), len(model_vertices), len(indices), len(triangles)*3))
        vertices += model_vertices
        indices += [i for triangle in triangles for i in triangle]
    lines = [
        '#ifndef JNET_FLOWER_ASSETS_H', '#define JNET_FLOWER_ASSETS_H',
        '/* Generated from FlowerPC by flower_resources.py: every Flower flower in bloom and as a closed bud, with stem and grass ring. */',
        '#define JNET_FLOWER_ATLAS_FILE L"JNET-flowers.dds"',
        '#define JNET_FLOWER_ATLAS_WIDTH %d' % grid['atlasWidth'], '#define JNET_FLOWER_ATLAS_HEIGHT %d' % grid['atlasHeight'],
        '#define JNET_FLOWER_GRASS_FILE L"JNET-grass.dds"', '#define JNET_FLOWER_GRASS_SIZE %d' % GRASS_SIZE,
        '#define JNET_FLOWER_MODEL_COUNT %d' % len(models),
        '/* color[3] and closed_color[3] hold the vertex kind: 0 stem, 255 petal (atlas cutout), 128 grass (grass sheet cutout). */',
        'struct jnet_flower_vertex { float position[3],normal[3],uv[2]; unsigned char color[4]; float closed[3]; unsigned char closed_color[4]; };',
        'struct jnet_flower_model { unsigned first_vertex,vertex_count,first_index,index_count; const char *name; };',
        'static const struct jnet_flower_vertex jnet_flower_vertices[%d]={' % len(vertices)]
    def byte_color(color, kind):
        return ','.join(str(max(0, min(255, round(a*255)))) for a in color) + ',%d' % kind
    rows = []
    for position, normal, color, uv, kind, closed_position, closed_color in vertices:
        rows.append('{{%s},{%s},{%s},{%s},{%s},{%s}}' % (
            ','.join('%.5g' % a for a in position), ','.join('%.4g' % a for a in normal), ','.join('%.5g' % a for a in uv),
            byte_color(color, kind), ','.join('%.5g' % a for a in closed_position), byte_color(closed_color, kind)))
    lines.append(',\n'.join(rows) + '};')
    lines.append('static const unsigned short jnet_flower_indices[%d]={%s};' % (len(indices), ','.join(map(str, indices))))
    lines.append('static const struct jnet_flower_model jnet_flower_models[JNET_FLOWER_MODEL_COUNT]={%s};' % ','.join(
        '{%d,%d,%d,%d,"%s"}' % (*model, flower['flowerName']) for model, flower in zip(models, flowers())))
    lines.append('#endif')
    return '\n'.join(lines) + '\n'


def export(output=OUTPUT):
    output = Path(output)
    mesh_table, grid = meshes(), atlas()
    for folder in ('meshes', 'flowers', 'textures', 'shaders'):
        (output/folder).mkdir(parents=True, exist_ok=True)
    for name, mesh in mesh_table.items():
        for frame in mesh['frames']:
            (output/'meshes'/f'{name}_{frame}.obj').write_text(mesh_obj(name, frame, mesh))
    for flower in flowers():
        (output/'flowers'/f'{flower["flowerName"]}.obj').write_text(flower_obj(flower, mesh_table, grid))
    (output/'flowers/flowers.mtl').write_text('newmtl FlowerAtlas\nmap_Kd ../textures/' + ATLAS + '\n')
    shutil.copyfile(FLOWER/'Data/Textures'/ATLAS, output/'textures'/ATLAS)
    for entry in shader_files():
        shutil.copyfile(FLOWER/'Data/Shaders/Release'/Path(entry['file']).name, output/entry['file'])
    (output/'flowers.json').write_text(json.dumps(manifest(mesh_table, grid), indent=1, sort_keys=True))
    return output


if __name__ == '__main__':
    print(export())
