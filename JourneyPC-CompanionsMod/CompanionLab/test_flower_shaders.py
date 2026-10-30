"""Compiles the flower HLSL embedded in the C headers with the Windows shader compiler, so a syntax error is caught here and not on
the first frame in the game (where it would only appear as a log line and no flowers). Skipped where d3dcompiler_47.dll is absent."""
import ctypes
import re
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
DIAGNOSTIC = HERE/'diagnostic'
LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')


def c_source(header, variable):
    text = (DIAGNOSTIC/header).read_text()
    start = text.index('static const char ' + variable + '[]=')
    body = text[start:text.index(';\n', start)]
    # the C compiler joins adjacent string literals; the only escape in them is a line break
    return ''.join(piece.replace('\\n', '\n') for piece in LITERAL.findall(body))


def compile_shader(source, target, entry='main'):
    compiler = ctypes.WinDLL('d3dcompiler_47.dll')
    code, errors = ctypes.c_void_p(), ctypes.c_void_p()
    data = source.encode()
    result = compiler.D3DCompile(data, len(data), b'test', None, None, entry.encode(), target.encode(), 0, 0,
                                 ctypes.byref(code), ctypes.byref(errors))
    message = ''
    if errors.value:
        # ID3DBlob vtable: QueryInterface, AddRef, Release, GetBufferPointer, GetBufferSize
        vtable = ctypes.cast(ctypes.cast(errors, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
        pointer = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(vtable[3])(errors)
        size = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(vtable[4])(errors)
        message = ctypes.string_at(pointer, size).decode(errors='replace')
    return result & 0xffffffff, message


@unittest.skipUnless(sys.platform == 'win32', 'needs the Windows shader compiler')
class FlowerShaderTest(unittest.TestCase):
    def test_flower_vertex_shader_compiles(self):
        result, message = compile_shader(c_source('jnet_flower_render.h', 'vertex_source'), 'vs_4_0')
        self.assertEqual(result, 0, message)

    def test_flower_pixel_shader_compiles(self):
        result, message = compile_shader(c_source('jnet_flower_render.h', 'pixel_source'), 'ps_4_1')
        self.assertEqual(result, 0, message)

    def test_compatible_shaders_compile(self):
        for variable, target in (("vertex_fallback", "vs_4_0"), ("pixel_fallback", "ps_4_0")):
            result, message = compile_shader(c_source("jnet_flower_render.h", variable), target)
            self.assertEqual(result, 0, message)

    def test_depth_probe_shaders_compile(self):
        for variable, target in (('vertex_source', 'vs_4_0'), ('pixel_source', 'ps_4_1')):
            result, message = compile_shader(c_source('jnet_flower_scene.h', variable), target)
            self.assertEqual(result, 0, message)

    def test_a_broken_shader_is_reported(self):
        result, message = compile_shader('float4 main(:SV_POSITION', 'vs_4_0')
        self.assertNotEqual(result, 0)
        self.assertTrue(message)


if __name__ == '__main__':
    unittest.main()
