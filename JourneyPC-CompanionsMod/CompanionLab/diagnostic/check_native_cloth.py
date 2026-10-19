"""Exercise pinned PC cloth workers and indexed draws; never launch or modify Journey."""
import hashlib
import json
from pathlib import Path
import subprocess
import struct
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from inspect_build import ROOT, EXPECTED_EXE, pe_sections
from native_map import offset_for

CODE = ((0x1E2BF0, 0x1E64A7), (0x36D5B0, 0x36D792), (0x372580, 0x372614),
        (0x34E990, 0x34EB2B), (0x3E1B50, 0x3E1B80),
        (0x35D5D0, 0x35D815), (0x345A10, 0x345A17), (0x8A3A0, 0x8A792),
        (0x273200, 0x273324), (0x273400, 0x273540), (0x273A00, 0x273AFC),
        (0x2CCA00, 0x2CCA61), (0x85B70, 0x85BCC), (0x7A480, 0x7A4CF),
        (0x78A00, 0x78A01), (0x273640, 0x2736CB), (0x2DAE60, 0x2DB235),
        (0x270A80, 0x270AE2), (0x3630F0, 0x3630F3), (0x364F80, 0x364F8A),
        (0x1D9770, 0x1DB352), (0x3B91E0, 0x3B9269),
        (0x37F7B0, 0x37F7E3), (0x2D8CB0, 0x2D8DD3), (0xE94A0, 0xE9699),
        (0x8E9B0, 0x8EA0E), (0x91820, 0x91895))
DATA = ((0x5C1E60, 0x5C1F40), (0x5CD440, 0x5CD460),
        (0x5C24A0, 0x5C24B0), (0x677850, 0x6778B0),
        (0x68AE80, 0x68CD50), (0x731F60, 0x731F7C))


def main():
    data = (ROOT / "JourneyPC/Journey.exe").read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED_EXE:
        raise SystemExit("Pinned Journey executable mismatch")
    sections = pe_sections(data)
    # Native registration rows are (entry, name, sentinel), not name-first.
    registration = struct.pack("<QQQ", 0x1401DB1F0, 0x140677830, 0xFFFFFFFFFFFFFFFF)
    name_offset = offset_for(sections, 0x677830, 22)
    if data.count(registration) != 1 or data[name_offset:name_offset + 22] != b"SPUClothOcclusionCube\0":
        raise SystemExit("Native occlusion job registration mismatch")
    work = HERE.parent / "work/native-cloth"
    work.mkdir(exist_ok=True)
    header = ["/* Generated pinned worker code; fixture only, never packaged. */"]
    for index, (begin, end) in enumerate(CODE + DATA):
        offset = offset_for(sections, begin, end - begin)
        header.append(f"static const unsigned char piece{index}[]={{" +
                      ",".join(str(v) for v in data[offset:offset + end - begin]) + "};")
    header.append("static void load_pieces(unsigned char *image) {")
    for index, (begin, _) in enumerate(CODE + DATA):
        header.append(f"memcpy(image+0x{begin:x},piece{index},sizeof(piece{index}));")
    header.append("}")
    (work / "cloth_code.h").write_text("\n".join(header) + "\n")
    source = (HERE / "probe_dll.c").read_text()
    begin = source.index("struct cloth_finish_call {")
    end = source.index("static void cloth_worker_call(", begin)
    (work / "cloth_finish_code.h").write_text(source[begin:end])
    binary = work / "test_native_cloth_engine"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", str(work), str(HERE / "test_native_cloth_engine.c"),
                    "-o", str(binary)], check=True)
    result = subprocess.run([str(binary)], text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise SystemExit(f"Native cloth fixture failed ({result.returncode}):\n{result.stdout}{result.stderr}")
    receipt = {"journey_sha256": EXPECTED_EXE, "game_runtime_tested": False,
               "native_cloth_workers_executed": True,
               "native_indexed_draw_functions_executed": True, "gpu_executed": False,
               "native_command_builder_and_vertex_binding_executed": True,
               "native_publication_attribution_and_stream_redirection_executed": True,
               "native_cloth_finalizer_executed": True,
               "native_appearance_uniform_setters_executed": True,
               "native_atlas_subscriber_bind_unlink_executed": True,
               "native_texture_binding_setter_executed": True,
               "native_occlusion_cube_empty_scene_executed": True,
               "native_occlusion_cube_convex_fixture_executed": True,
               "native_cube_configuration_and_occluder_copy_executed": True,
               "production_probe_pipeline_executed": True,
               "probe_factory_query_and_gpu_services": "intercepted; no live GPU validation",
               "runtime_services": "fixture memory, allocation, resource factory, spatial query, GPU mapping and stack-probe services",
               "output": result.stdout.strip(),
               "sources": {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                           for name in ("native_environment_probe.h", "native_cloth_material_live.h", "native_cloth_commands.h", "native_cloth_publication.h", "native_cloth_partition.h", "native_cloth_draw.h", "native_cloth_frame.h", "test_native_cloth_engine.c", "check_native_cloth.py")}}
    (work / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(result.stdout, end="")


if __name__ == "__main__":
    main()
