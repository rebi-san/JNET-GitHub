import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXTENT = 0.001
TARGETS = {
    'Canyon': '''342a0d4342c87a06589408315ca1a6d6 Hull12830_G
86bc4178b8ee58708f741f1b5b761f0c pCube954HullNode''',
    'Desert': '30bab2f719f1a4ffa32f3e393cb6ad0d Hull23250_G1',
    'Mountain': '''837060bea3c083202676a26a867d7e80 Hull77151_G
dbdc4b54e1bb6477975ec5a57a58584c Hull77152_G
82d86af4fc87e3f0410833d6020a2ebb Hull77153_G
ef0a772f54ee0cad749758f35f0e5827 Hull77149_G
8ab629df30728f6acaa9c75b5d863991 Hull77150_G
e0c4f6febb8a8b48d55acf4a6d20c4a7 Hull77148_G
1e8cfd7b423fbf4f59b7aa86aa45af79 Hull77121_G
7aa3d37457578ae0b04394982daf98da Hull77122_G
b37a9212dfd204f4be2f0aa7e3aff5ee Hull77120_G
c91d7d5add6a5b28772569094419b5c6 Hull77123_G
a189fc906b50f96ffa6e5dc4042ef89b Hull77124_G
6c7018e42d1d11871a85f145f0bed5a5 Hull77133_G
95ffeaf049208f733e68332361d34b7f Hull77129_G
28e24eac434395ce520142c0ac5db346 Hull77131_G
5dc2d725a78f4a4f3d452f294c7873c8 Hull77130_G
46395ddf514c063445c167071b9cc98a Hull77132_G
95e0124101b0551572a8d79f6bf0a889 Hull77128_G
5a474d4f6f10c898dacc0e89deaa29e5 Hull77103_G
796e18feb8900843b786dd463add3c32 Hull77102_G
11e3bd345a444908aaa35b044c415f66 Hull77154_G
02bc2db6a75a57ad340108bb639c0e62 Hull771254_G
4177537eca6da7d865309ee8d42964e0 Hull77169_G1
f44a5312bbfcf9c5f21e769154660a6c Hull76885_G1
52d5190fda4096defd34f4f4490c945c Hull76884_G
63238ada7e975b3bd30307e8d50d3c69 Hull76882_G
301ca1d315f9cb5563b95c6f368ceb0a Hull421_G''',
    'Barrens': '''62ee2608fb353b369b0f9ecd6ea629fc Hull16197_G
22310576f64779804370186cd5904e76 Hull16196_G
10ed6bc2b9f2162c153abb455b72608b Hull16195_G
c12a91d75606658f0319263cfd118bd0 Hull16194_G
43fbc16f7d3dd8f8ac2c48873d3b98c4 Hull16193_G
0e82dfe89e539aaac481b880da914ee6 Hull16192_G
18a6d31fd52ec3bc2de3ad4d32f92b9e Hull16187_G
04f7f890515f41d0da07b3da274b7492 Hull16186_G
28a27539300c167a16a2906b307b2364 Hull16188_G
4d9db8b09d995e484ec1242af61bc3f0 Hull16189_G
9da5c78a25143ca0554483e53df20599 Hull16180_G
1889ef9af5d2a052e1544a6a7e74c7ff Hull16179_G
27afff61dfbc8437cb967259ddadda28 Hull16178_G
645b4d64ca06df5ae1d15809b9d262d4 Hull16173_G
e753497495eb0c465ad668908aa8d228 Hull16174_G
3b7d4be91ea336d80c788171e0ac05c7 Hull16175_G
80942d051a18dc88af6b13e97d21626c Hull16176_G''',
}


def records():
    result = {}
    for level, targets in TARGETS.items():
        source = (ROOT / f'JourneyIOS/Data/Scripts/Level_{level}/HullInstances.lua').read_text()
        binary = (ROOT / f'JourneyPC/Data/Scripts/Level_{level}/HullInstances.lua.bin').read_bytes()
        rows = {m[1]: m[2] for m in re.finditer(
            r'ObjectName = "([0-9a-f]+)"(.*?)(?=ObjectName =|\Z)', source, re.S)}
        for entry in targets.splitlines():
            key, name = entry.split()
            row = rows[key]
            gardener = re.search(r'GardenerName = "([^"]+)"', row)[1]
            if gardener.split('|')[-1] != name or key in result:
                raise ValueError(f'Unexpected hull identity: {level}/{key}')
            arrays = {}
            for field in ('Planes', 'TriCounts', 'Edges', 'Verts'):
                body = re.search(field + r'\s*=\s*\{([^}]+)\}', row)[1]
                arrays[field] = [float(v) for v in re.findall(r'-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?', body)]
            vertices = arrays['Verts']
            packed = b''.join(struct.pack('<4f', *vertices[i:i+3], 0) for i in range(0, len(vertices), 3))
            if binary.count(key.encode()) != 1:
                raise ValueError(f'PC hull source mismatch: {level}/{key}')
            start = binary.index(key.encode())
            offset = binary.find(packed, start, start + 512 + len(packed))
            if offset <= start or offset - start > 512:
                raise ValueError(f'PC hull topology mismatch: {level}/{key}')
            cursor = start + len(key) + 1
            for field in ('Planes', 'TriCounts', 'Edges'):
                topology = bytes(int(v) for v in arrays[field])
                found = binary.find(topology, cursor, offset)
                if found < 0:
                    raise ValueError(f'PC hull {field} mismatch: {level}/{key}')
                cursor = found + len(topology)
            bounds = [(min(vertices[a::3]), max(vertices[a::3])) for a in range(3)]
            scale = EXTENT / max(hi-lo for lo, hi in bounds)
            center = [(lo+hi)/2 for lo, hi in bounds]
            shrunk = [(v-center[i % 3])*scale for i, v in enumerate(vertices)]
            result[key] = dict(level=level, name=name, vertices=shrunk,
                               counts=[len(arrays['Planes'])//3, len(arrays['TriCounts']),
                                       len(arrays['Edges'])//2, len(vertices)//3])
    return result


def lua_chunks():
    chunks = ['__JNETHullVertices={}']
    for key, row in records().items():
        vectors = ['{' + ','.join(format(v, '.12g') for v in row['vertices'][i:i+3]) + '}'
                   for i in range(0, len(row['vertices']), 3)]
        chunks.append('__JNETHullVertices["' + key + '"]={counts={' +
                      ','.join(map(str, row['counts'])) + '},verts={' + ','.join(vectors) + '}}')
    chunks.append('''function __JNETShrinkHulls()
        for name,row in pairs(__JNETHullVertices) do
            local hull=Names[name]
            if hull then
                local function resize(h)
                    local c=row.counts
                    if h:GetTriCount()~=c[1] or h:GetPlaneCount()~=c[2] or
                       h:GetEdgeCount()~=c[3] or h:GetVertCount()~=c[4] then
                        error('Unexpected hull topology: '..name)
                    end
                    h:ResetAabb()
                    for i,v in ipairs(row.verts) do h:SetVert(i-1,v) end
                end
                local ok,err=pcall(function()
                    if type(hull)=='table' then for _,h in ipairs(hull) do resize(h) end
                    else resize(hull) end
                end)
                if not ok then print('[JNET] hull resize failed: '..tostring(err)) end
            end
        end
    end''')
    return chunks


def runtime_header():
    return ('static const char *const jnet_hull_chunks[]={\n' +
            ',\n'.join(json.dumps(chunk) for chunk in lua_chunks()) + '\n};\n')
