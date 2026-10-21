"""Software preview of the extracted Flower models: DDS decoding, a small z-buffered rasteriser and a PNG writer (standard library only).

Used to check model changes offline: `python flower_preview.py [output.png]` writes a contact sheet of every flower, closed and in bloom,
with the atlas alpha cutout, vertex colours and simple lighting that the in-game shader applies.
"""
import math
import struct
import sys
import zlib
from pathlib import Path

import flower_resources as fr

LAB = Path(__file__).resolve().parent


def write_png(path, width, height, rgb):
    """rgb: bytes of width*height*3."""
    raw = b''.join(b'\x00' + rgb[y*width*3:(y + 1)*width*3] for y in range(height))
    def chunk(kind, data):
        body = kind + data
        return struct.pack('>I', len(data)) + body + struct.pack('>I', zlib.crc32(body) & 0xffffffff)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) +
                           chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def _rgb565(value):
    return ((value >> 11 & 31)*255//31, (value >> 5 & 63)*255//63, (value & 31)*255//31)


def _dxt_color(block):
    c0, c1, bits = struct.unpack_from('<HHI', block, 0)
    a, b = _rgb565(c0), _rgb565(c1)
    if c0 > c1:
        palette = [a, b, tuple((2*x + y)//3 for x, y in zip(a, b)), tuple((x + 2*y)//3 for x, y in zip(a, b))]
    else:
        palette = [a, b, tuple((x + y)//2 for x, y in zip(a, b)), (0, 0, 0)]
    return [palette[bits >> (2*i) & 3] for i in range(16)]


def _dxt5_alpha(block):
    a0, a1 = block[0], block[1]
    bits = int.from_bytes(block[2:8], 'little')
    if a0 > a1: table = [a0, a1] + [((7 - i)*a0 + i*a1)//7 for i in range(1, 7)]
    else: table = [a0, a1] + [((5 - i)*a0 + i*a1)//5 for i in range(1, 5)] + [0, 255]
    return [table[bits >> (3*i) & 7] for i in range(16)]


def read_dds(path, level=0):
    """(width, height, rgba bytes) of one mip level of an uncompressed BGRA or DXT1/DXT5 DDS."""
    data = Path(path).read_bytes()
    height, width = struct.unpack_from('<II', data, 12)
    mips = struct.unpack_from('<I', data, 28)[0] or 1
    flags, fourcc, bits = struct.unpack_from('<I4sI', data, 80)
    at = 128
    compressed = fourcc in (b'DXT1', b'DXT5')
    for n in range(level):
        w, h = max(1, width >> n), max(1, height >> n)
        at += (max(1, (w + 3)//4)*max(1, (h + 3)//4)*(8 if fourcc == b'DXT1' else 16)) if compressed else w*h*4
    w, h = max(1, width >> level), max(1, height >> level)
    out = bytearray(w*h*4)
    if compressed:
        size = 8 if fourcc == b'DXT1' else 16
        for by in range((h + 3)//4):
            for bx in range((w + 3)//4):
                block = data[at:at + size]; at += size
                alpha = _dxt5_alpha(block) if fourcc == b'DXT5' else [255]*16
                colors = _dxt_color(block[size - 8:])
                for i in range(16):
                    x, y = bx*4 + i % 4, by*4 + i//4
                    if x < w and y < h:
                        out[(y*w + x)*4:(y*w + x)*4 + 4] = bytes((*colors[i], alpha[i]))
    else:
        raw = data[at:at + w*h*4]
        for i in range(0, w*h*4, 4):
            out[i:i + 4] = bytes((raw[i + 2], raw[i + 1], raw[i], raw[i + 3]))
    return w, h, bytes(out)


class Canvas:
    def __init__(self, width, height, background=(60, 70, 80)):
        self.width, self.height = width, height
        self.color = [list(background) for _ in range(width*height)]
        self.depth = [1e30]*(width*height)

    def triangle(self, vertices, shade):
        """vertices: three (x, y, depth, payload); shade(payload tuple) -> (r, g, b) or None to discard."""
        xs = [v[0] for v in vertices]; ys = [v[1] for v in vertices]
        x0, x1 = max(0, int(math.floor(min(xs)))), min(self.width - 1, int(math.ceil(max(xs))))
        y0, y1 = max(0, int(math.floor(min(ys)))), min(self.height - 1, int(math.ceil(max(ys))))
        (ax, ay, az, ap), (bx, by, bz, bp), (cx, cy, cz, cp) = vertices
        area = (bx - ax)*(cy - ay) - (by - ay)*(cx - ax)
        if abs(area) < 1e-9: return
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                px, py = x + .5, y + .5
                w0 = ((bx - px)*(cy - py) - (by - py)*(cx - px))/area
                w1 = ((cx - px)*(ay - py) - (cy - py)*(ax - px))/area
                w2 = 1 - w0 - w1
                if w0 < 0 or w1 < 0 or w2 < 0: continue
                z = w0*az + w1*bz + w2*cz
                if z >= self.depth[y*self.width + x]: continue
                payload = tuple(w0*p + w1*q + w2*r for p, q, r in zip(ap, bp, cp))
                rgb = shade(payload)
                if rgb is None: continue
                self.depth[y*self.width + x] = z
                self.color[y*self.width + x] = rgb

    def paste(self, other, left, top):
        for y in range(other.height):
            for x in range(other.width):
                self.color[(top + y)*self.width + left + x] = other.color[y*other.width + x]

    def bytes(self):
        return bytes(min(255, max(0, int(c))) for pixel in self.color for c in pixel)


def lerp(a, b, t):
    return tuple(x + (y - x)*t for x, y in zip(a, b))


def render(vertices, triangles, atlas, grass, size=192, yaw=.6, pitch=.45, bloom=1.0):
    """Orbit camera looking at the flower's middle. Vertices follow flower_model(); the alpha-cut kinds match the in-game shader."""
    canvas = Canvas(size, size)
    ys = [v[0][1] for v in vertices]
    centre = (0, (max(ys) + min(ys))/2, 0)
    radius = max(max(abs(v[0][0]), abs(v[0][2])) for v in vertices)
    extent = max(radius*2.4, (max(ys) - min(ys))*1.15)
    cy, sy, cp, sp = math.cos(yaw), math.sin(yaw), math.cos(pitch), math.sin(pitch)
    def project(p):
        x, y, z = p[0] - centre[0], p[1] - centre[1], p[2] - centre[2]
        x, z = x*cy + z*sy, z*cy - x*sy
        y, z = y*cp - z*sp, z*cp + y*sp
        return (size/2 + x/extent*size, size/2 - y/extent*size, z)
    projected = []
    for position, normal, colour, uv, kind, closed_position, closed_colour in vertices:
        position = lerp(closed_position, position, bloom); colour = lerp(closed_colour, colour, bloom)
        x, y, z = project(position)
        nx, nz = normal[0]*cy + normal[2]*sy, normal[2]*cy - normal[0]*sy
        projected.append((x, y, z, (*colour, uv[0], uv[1], normal[1]*cp - nz*sp, kind)))
    def sample(texture, u, v):
        w, h, pixels = texture
        t = (min(h - 1, max(0, int(v*h)))*w + min(w - 1, max(0, int(u*w))))*4
        return pixels[t:t + 4]
    def shade(p):
        r, g, b, u, v, ny, kind = p
        if kind > 200:
            texel = sample(atlas, u, v)
            if texel[3] < 128: return None
            mix = tuple(1 + (texel[i]/255*1.8 - 1)*.65 for i in range(3))
        elif kind > 60:
            texel = sample(grass, u, v)
            if texel[3] < 128: return None
            mix = (1, 1, 1)
        else:
            mix = (1, 1, 1)
        light = .55 + .2*ny + .5*max(0, ny*.8)
        return tuple(255*min(1, (max(ch, 0)**.55)*m*light*1.15) for ch, m in zip((r, g, b), mix))
    for a, b, c in triangles:
        canvas.triangle([projected[a], projected[b], projected[c]], shade)
    return canvas


def load_textures():
    folder = LAB/'work/flower'
    folder.mkdir(parents=True, exist_ok=True)
    (folder/'JNET-grass.dds').write_bytes(fr.grass_dds())
    return read_dds(fr.FLOWER/'Data/Textures'/fr.ATLAS, 1), read_dds(folder/'JNET-grass.dds', 0)


def contact_sheet(path, blooms=(1.0, 0.0), size=160):
    atlas, grass = load_textures()
    table, grid = fr.meshes(), fr.atlas()
    flowers = fr.flowers()
    sheet = Canvas(size*len(flowers), size*len(blooms))
    for column, flower in enumerate(flowers):
        vertices, triangles = fr.flower_model(flower, table, grid)
        for row, bloom in enumerate(blooms):
            sheet.paste(render(vertices, triangles, atlas, grass, size, bloom=bloom), column*size, row*size)
    write_png(path, sheet.width, sheet.height, sheet.bytes())
    return path


if __name__ == '__main__':
    print(contact_sheet(sys.argv[1] if len(sys.argv) > 1 else LAB/'work/flower/preview.png'))
