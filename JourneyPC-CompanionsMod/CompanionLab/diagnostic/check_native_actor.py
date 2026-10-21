"""Differentially execute the pinned actor render dispatcher outside Journey."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from inspect_build import ROOT, EXPECTED_EXE, pe_sections
from native_map import offset_for


def main():
    data = (ROOT / "JourneyPC/Journey.exe").read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED_EXE:
        raise SystemExit("Pinned Journey executable mismatch")
    sections = pe_sections(data)
    work = HERE.parent / "work/native-actor"
    work.mkdir(exist_ok=True)
    ranges = ((0x117790, 0x1179df), (0x1183f4, 0x11865a), (0x117fd0, 0x11802a),
              (0x7a1b0, 0x7a2dd), (0x5c1e60, 0x5c1f40), (0x68ae80, 0x68c020),
              (0x118730, 0x118de8), (0xb1730, 0xb18f4), (0xa9d20, 0xaa481),
              (0x273200,0x273324),(0x273400,0x273540),(0x273640,0x2736cb),
              (0xa9c8e,0xa9ce5),(0x272580,0x272ff3),(0x2464f8,0x24652d),
              (0xa7bb6,0xa7bfb),(0x7aa40,0x7ab9f),(0x5c24c0,0x5c24d0),
              (0x217a70,0x217c2d),(0x10f0a0,0x10f1b0),(0x13e6b0,0x13f354),(0x9eed0,0x9f036),
              (0x5cf750,0x5cf780),(0x5cec28,0x5cec48),
              (0xae650,0xaeb44),(0xaedd0,0xaefe3),(0x146870,0x1468d1),
              (0x245e10,0x246125),(0x10e43b,0x10e5a8),(0x5defa8,0x5df030),(0x8ab10,0x8ad97),(0x117770,0x117785),
              (0x21d980,0x21d9ab),(0x24fa70,0x24fd1c),(0x140790,0x140a0d),(0x5cfa90,0x5cfaa0),
              (0xab830,0xab920),(0xc4d10,0xc4dc5),(0x24bdf0,0x24c002),(0x24c010,0x24c49b),(0x118e57,0x118e7c),(0x677870,0x677880))
    runtime = (HERE / "traveler_actor_runtime.h").read_text()
    begin = runtime.index("static void traveler_actor_visibility(")
    end = runtime.index("static void traveler_actor_render(", begin)
    runtime = runtime[:begin] + "static void traveler_actor_visibility(uintptr_t render) {(void)render;}\n" + runtime[end:]
    (work / "actor_runtime_fixture.h").write_text(runtime)
    backend = (HERE / "traveler_live_backend.h").read_text()
    begin = backend.index("static int traveler_live_material_rebuild(")
    end = backend.index("static int traveler_live_workers(", begin)
    (work / "material_reset_fixture.h").write_text(backend[begin:end])
    header = []
    for n, (start, end) in enumerate(ranges):
        offset = offset_for(sections, start, end - start)
        header.append(f"static const unsigned char piece{n}[]={{" +
                      ",".join(map(str, data[offset:offset + end - start])) + "};")
    header.append("static void load_code(unsigned char *image) {")
    for n, (start, _) in enumerate(ranges):
        header.append(f"memcpy(image+0x{start:x},piece{n},sizeof(piece{n}));")
    header.append("}")
    header.append("static inline void load_runtime_constants(unsigned char *image) {")
    for rva in (0x28427f8,0x2887514,0x3601f84,0x35b65f8,0x35b7734):
        offset = offset_for(sections,rva,4)
        values = ",".join(map(str,data[offset:offset+4]))
        header.append(f"memcpy(image+0x{rva:x},(const unsigned char[]){{{values}}},4);")
    header.append("}")
    (work / "actor_code.h").write_text("\n".join(header) + "\n")
    binary = work / "test_native_actor_engine"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-I", str(work), "-I", str(HERE), str(HERE / "test_native_actor_engine.c"), "-o", str(binary)], check=True)
    result = subprocess.run([str(binary)], text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise SystemExit(f"Native actor fixture failed ({result.returncode}):\n{result.stdout}{result.stderr}")
    glyph_binary=work / "test_native_chirp_glyph"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(work), "-I", str(HERE),
                    str(HERE / "test_native_chirp_glyph.c"), "-o", str(glyph_binary)], check=True)
    glyph_result=subprocess.run([str(glyph_binary)], text=True, capture_output=True, timeout=60)
    if glyph_result.returncode:
        raise SystemExit(f"Native glyph fixture failed: {glyph_result.stdout}{glyph_result.stderr}")
    print(glyph_result.stdout,end="")
    departure_binary=work / "test_native_actor_departure"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(work), "-I", str(HERE),
                    str(HERE / "test_native_actor_departure.c"), "-o", str(departure_binary)], check=True)
    departure_result=subprocess.run([str(departure_binary)], text=True, capture_output=True, timeout=60)
    if departure_result.returncode:
        raise SystemExit(f"Native departure fixture failed: {departure_result.stdout}{departure_result.stderr}")
    print(departure_result.stdout,end="")
    sand_binary=work / "test_native_sand_consumer"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(work), "-I", str(HERE),
                    str(HERE / "test_native_sand_consumer.c"), "-o", str(sand_binary)], check=True)
    sand_result=subprocess.run([str(sand_binary)], text=True, capture_output=True, timeout=60)
    if sand_result.returncode:
        raise SystemExit(f"Native sand consumer fixture failed: {sand_result.stdout}{sand_result.stderr}")
    print(sand_result.stdout,end="")
    local_binary=work / "test_native_local_peer"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(work), "-I", str(HERE),
                    str(HERE / "test_native_local_peer.c"), "-o", str(local_binary)], check=True)
    local_result=subprocess.run([str(local_binary)], text=True, capture_output=True, timeout=60)
    if local_result.returncode:
        raise SystemExit(f"Local peer fixture failed: {local_result.stdout}{local_result.stderr}")
    print(local_result.stdout,end="")
    charge_binary=work / "test_native_chirp_charge"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(work), "-I", str(HERE),
                    str(HERE / "test_native_chirp_charge.c"), "-o", str(charge_binary)], check=True)
    charge_result=subprocess.run([str(charge_binary)],text=True,capture_output=True,timeout=60)
    if charge_result.returncode:
        raise SystemExit(f"Chirp charging fixture failed: {charge_result.stdout}{charge_result.stderr}")
    print(charge_result.stdout,end="")
    growth_binary=work / "test_native_scarf_growth"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(work), "-I", str(HERE),
                    str(HERE / "test_native_scarf_growth.c"), "-o", str(growth_binary)], check=True)
    growth_result=subprocess.run([str(growth_binary)],text=True,capture_output=True,timeout=60)
    if growth_result.returncode:
        raise SystemExit(f"Scarf growth fixture failed: {growth_result.stdout}{growth_result.stderr}")
    print(growth_result.stdout,end="")
    stream_binary=work / "test_companion_actor_stream"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-I", str(HERE), str(HERE / "test_companion_actor_stream.c"),
                    "-o",str(stream_binary)],check=True)
    stream_result=subprocess.run([str(stream_binary)],text=True,capture_output=True,timeout=60)
    if stream_result.returncode:
        raise SystemExit(f"Actor stream fixture failed: {stream_result.stdout}{stream_result.stderr}")
    print(stream_result.stdout,end="")
    packet_binary=work / "test_companion_actor_packet"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-I", str(HERE), str(HERE / "test_companion_actor_packet.c"),
                    "-o",str(packet_binary)],check=True)
    packet_result=subprocess.run([str(packet_binary)],text=True,capture_output=True,timeout=60)
    if packet_result.returncode:
        raise SystemExit(f"Actor packet fixture failed: {packet_result.stdout}{packet_result.stderr}")
    print(packet_result.stdout,end="")
    admission_binary=work / "test_traveler_stream_admission"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-I", str(HERE), str(HERE / "test_traveler_stream_admission.c"),
                    "-o",str(admission_binary)],check=True)
    admission_result=subprocess.run([str(admission_binary)],text=True,capture_output=True,timeout=60)
    if admission_result.returncode:
        raise SystemExit(f"Admission fixture failed: {admission_result.stdout}{admission_result.stderr}")
    print(admission_result.stdout,end="")
    receipt = {"admission_output": admission_result.stdout.strip(), "actor_packet_output": packet_result.stdout.strip(), "journey_sha256": EXPECTED_EXE, "game_runtime_tested": False, "actor_stream_output": stream_result.stdout.strip(), "scarf_growth_output": growth_result.stdout.strip(),
               "local_peer_output": local_result.stdout.strip(), "chirp_charge_output": charge_result.stdout.strip(),
               "native_sand_consumer_output": sand_result.stdout.strip(),
               "native_departure_leaves_executed": True, "native_departure_output": departure_result.stdout.strip(),
               "native_render_dispatcher_executed": True, "render_leaf_calls_intercepted": True,
               "native_remote_input_numeric_branch_executed": True,
               "native_normalization_and_clock_executed": True,
               "actor_input_and_update_intercepted": True,
               "native_actor_end_frame_executed": True,
               "native_body_scale_transform_executed": True,
               "native_actor_hsv_update_executed": True,
               "visibility_logger_tested": False,
               "native_sand_append_capacity_guard_executed": True,
               "native_sand_inline_prewrite_guard_executed": True,
               "native_chirp_glyph_update_executed": True,
               "native_cloth_allocation_gate_executed": True,
               "native_empty_material_setters_executed": True,
               "adapter_and_fixture_sanitizers": ["address", "undefined"],
               "original_machine_code_instrumented": False,
               "output": result.stdout.strip(), "sources": {
                   name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                   for name in ("native_actor_live.h", "native_actor_runtime.h", "native_actor_material.h", "traveler_actor_runtime.h", "traveler_glyph_init.h", "native_geometry_pool.h",
                                "native_actor_departure.h", "native_peer_lifecycle.h", "traveler_peer_lifecycle.h", "traveler_live_backend.h", "test_native_actor_departure.c", "traveler_chirp_glyph.h", "traveler_glyph_trace.h", "test_native_chirp_glyph.c", "traveler_sand_guard.h", "test_native_sand_consumer.c", "test_native_actor_engine.c", "native_local_peer.h", "traveler_local_peer.h", "test_native_local_peer.c", "native_chirp_charge.h", "traveler_chirp_charge.h", "test_native_chirp_charge.c", "native_scarf_growth.h", "test_native_scarf_growth.c", "companion_actor_stream.h", "companion_script_peer.h", "traveler_stream_test.h", "test_companion_actor_stream.c", "companion_actor_packet.h", "companion_chirp_packet.h", "traveler_stream_udp.h", "test_companion_actor_packet.c", "test_traveler_stream_admission.c", "check_native_actor.py")}}
    (work / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(result.stdout, end="")


if __name__ == "__main__":
    main()
