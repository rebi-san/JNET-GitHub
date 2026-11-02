import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess


def pe_sections(data):
    if data[:2] != b"MZ":
        raise ValueError("Not a PE image")
    pe = struct.unpack_from("<I", data, 60)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Invalid PE header")
    machine, count = struct.unpack_from("<HH", data, pe + 4)
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    if machine != 0x8664 or struct.unpack_from("<H", data, pe + 24)[0] != 0x20b:
        raise ValueError("Expected x86-64 PE32+")
    result = []
    for i in range(count):
        pos = pe + 24 + optional_size + 40 * i
        name, virtual_size, rva, size, offset = struct.unpack_from("<8sIIII", data, pos)
        flags = struct.unpack_from("<I", data, pos + 36)[0]
        if offset + size > len(data):
            raise ValueError("Truncated PE section")
        result.append(dict(name=name.rstrip(b"\0").decode("ascii"),
                           rva=rva, raw_offset=offset, raw_size=size,
                           virtual_size=virtual_size, executable=bool(flags & 0x20000000)))
    return result


def directory(data, index):
    pe = struct.unpack_from("<I", data, 60)[0]
    return struct.unpack_from("<II", data, pe + 24 + 112 + 8 * index)


def file_offset(sections, rva, size=1):
    if size < 1:
        raise ValueError("Empty RVA range")
    for section in sections:
        relative = rva - section["rva"]
        if relative >= 0 and relative + size <= section["raw_size"]:
            return section["raw_offset"] + relative
    raise ValueError("RVA is outside file-backed sections")


def c_string(data, offset):
    end = data.find(b"\0", offset, offset + 512)
    if offset < 0 or end < 0:
        raise ValueError("Invalid PE string")
    return data[offset:end].decode("ascii")


def exports(data):
    sections = pe_sections(data)
    rva, size = directory(data, 0)
    offset = file_offset(sections, rva, 40)
    functions, names, function_rva, name_rva, ordinal_rva = struct.unpack_from("<IIIII", data, offset + 20)
    if not 0 < names <= functions <= 4096:
        raise ValueError("Unexpected export count")
    function_offset = file_offset(sections, function_rva, 4 * functions)
    name_offset = file_offset(sections, name_rva, 4 * names)
    ordinal_offset = file_offset(sections, ordinal_rva, 2 * names)
    result = {}
    for i in range(names):
        name_ptr = struct.unpack_from("<I", data, name_offset + 4 * i)[0]
        name = c_string(data, file_offset(sections, name_ptr))
        ordinal = struct.unpack_from("<H", data, ordinal_offset + 2 * i)[0]
        if ordinal >= functions or name in result:
            raise ValueError("Invalid export ordinal or duplicate name")
        location = struct.unpack_from("<I", data, function_offset + 4 * ordinal)[0]
        if not location or rva <= location < rva + size:
            raise ValueError("Missing or forwarded data export")
        result[name] = location
    return result


EXPORTS = (('STEAM_GAME', 'JNETSteamGame'), ('STEAM_P2P', 'JNETSteamP2P'), ('STEAM_P2P_TEXT', 'JNETSteamP2PText'), ('STEAM_LOBBY', 'JNETSteamLobby'), ('STEAM_LOBBY_TEXT', 'JNETSteamLobbyText'), ('STEAM_TEXT', 'JNETSteamServiceText'), ('STEAM', 'JNETSteam'), ('MENU', 'JNETMenu'), ('PEER_LIFECYCLE', 'JNETPeerLifecycle'), ('CONTROL', 'JNETControl'), ('ACTOR_TRIAL', 'JNETActorTrial'), ('ACTOR', 'JNETActor'), ('TRAVELER', 'JNETTraveler'), ('LIFECYCLE', 'JNETLifecycle'), ('STATUS', 'JNETStatus'), ('COMMAND', 'JNETCommand'), ('BUILD', 'JNETBuildId'), ('INIT', 'JNETInitialize'), ('OBSERVATION', 'JNETObservation'), ('PHASE', 'JNETPhase'), ('FRAMES', 'JNETFrames'), ('GAME', 'JNETGame'), ('TRIAL', 'JNETTrial'), ('LIFETIME', 'JNETLifetime'), ('RESOURCE', 'JNETResource'), ('POOL', 'JNETPool'), ('MESH', 'JNETMesh'), ('MATERIAL', 'JNETMaterial'), ('GEOMETRY', 'JNETGeometry'), ('BUILDER', 'JNETBuilder'), ('STARTUP', 'JNETStartup'), ('COMMIT', 'JNETCommit'), ('CALLBACKS', 'JNETCallbacks'), ('POSE', 'JNETPose'), ('POSE_WRITES', 'JNETPoseWrites'), ('ANIMATION', 'JNETAnimation'), ('IDLE', 'JNETIdle'), ('RENDER', 'JNETRender'), ('MESH_LIFE', 'JNETMeshLife'), ('APPEARANCE', 'JNETAppearance'), ('LINK', 'JNETLink'), ('ROOM', 'JNETRoom'), ('MOTION', 'JNETMotion'), ('BODIES', 'JNETBodies'), ('DUAL', 'JNETDual'), ('SKELETON', 'JNETSkeleton'), ('EVALUATION', 'JNETEvaluation'), ('NATIVE_PLAYBACK', 'JNETNativePlayback'), ('NATIVE_OUTPUTS', 'JNETNativeOutputs'), ('COMPANION_FEATURES', 'JNETCompanionFeatures'), ('FLIGHT', 'JNETFlight'), ('LINK_CONFIG', 'JNETLinkConfig'), ('NETWORK', 'JNETNetwork'), ('PERFORMANCE', 'JNETPerformance'))


def main():
    parser = argparse.ArgumentParser(description="Compile the reviewed JNET DLL and its matching launcher; no installation or launch.")
    parser.add_argument("--zig", default="zig", help="Zig 0.13.0 executable")
    args = parser.parse_args()
    zig = shutil.which(args.zig)
    if not zig or subprocess.check_output([zig, "version"], text=True).strip() != "0.13.0":
        raise SystemExit("Zig 0.13.0 is required; dependencies are not installed automatically.")
    lab = Path(__file__).resolve().parent
    generated = lab / "work/diagnostic-build"
    source = lab / "diagnostic"
    minhook = lab / "work/deps/minhook-1.3.4"
    output = lab / "work/moderation-build"
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env.update(ZIG_GLOBAL_CACHE_DIR=str(output / "zig-cache"), ZIG_LOCAL_CACHE_DIR=str(output / "zig-local-cache"))
    flags = [zig, "cc", "-target", "x86_64-windows-gnu", "-O2", "-std=c11", "-fms-extensions",
             "-DUNICODE", "-D_UNICODE", "-Wall", "-Wextra", "-Werror", "-Wno-unused-const-variable",
             "-I" + str(output), "-I" + str(generated), "-I" + str(minhook / "include")]
    hooks = [str(minhook / "src" / name) for name in ("buffer.c", "hook.c", "trampoline.c", "hde/hde64.c")]
    subprocess.run(flags + ["-shared", str(source / "probe_dll.c"), *hooks,
                           "-lwinmm", "-lbcrypt", "-lws2_32", "-lgdi32", "-o", str(output / "JNET.dll")],
                   check=True, env=env, cwd=output)
    dll = (output / "JNET.dll").read_bytes()
    symbols = exports(dll)
    def array(data):
        return "{" + ",".join(str(byte) for byte in data) + "}"
    header = '#define JC_HELPER_VERSION L"0.357"\n#define JC_HELPER_HEADER_VERSION L"0.357"\n'
    header += "static const unsigned char jc_dll_hash[32] = " + array(hashlib.sha256(dll).digest()) + ";\n"
    for macro, symbol in EXPORTS:
        header += f"#define JC_{macro}_RVA {symbols[symbol]}u\n"
    (output / "built_dll.h").write_text(header)
    subprocess.run(flags + ["-municode", "-Wl,--subsystem,windows", str(source / "probe_helper.c"),
                           "-lbcrypt", "-luser32", "-lgdi32", "-lcomdlg32", "-o", str(output / "JNET.exe")],
                   check=True, env=env, cwd=output)
    helper = (output / "JNET.exe").read_bytes()
    pe_sections(helper)
    print(json.dumps({"version": "0.357", "dll_sha256": hashlib.sha256(dll).hexdigest(),
                      "exe_sha256": hashlib.sha256(helper).hexdigest(), "installed": False, "launched": False}, indent=2))


if __name__ == "__main__":
    main()
