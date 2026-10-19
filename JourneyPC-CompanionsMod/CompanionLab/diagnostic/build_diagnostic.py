import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zipfile
import argparse


HERE = Path(__file__).resolve().parent
LAB = HERE.parent
VERSION = "0.283"
ROOT = LAB.parent.parent
sys.path.insert(0, str(LAB))
from inspect_build import EXPECTED_EXE, audit_signatures, pe_sections
from daynight_resources import runtime_header as daynight_runtime_header
from startup_resources import extract_persistent, prepare_candidate, SCRIPT_RVA, SCRIPT_SIZE
from control_resources import menu_bytes
from character_resources import character_runtime_header, character_resource_script, wayfarer_resource_script, character_animation_files, character_animation_file_header, character_mesh_files


def sha(data):
    return hashlib.sha256(data).hexdigest()


def array(data):
    return "{" + ",".join(str(b) for b in data) + "}"


def scarf_banner_header():
    source = ROOT / "JourneyPC/Data/Textures/Bin"
    header = "#define JC_SCARF_WIDTH 2048\n#define JC_SCARF_HEIGHT 80\n"
    for label, name, left in (
        ("red", "ClothPlayerScarfFilledClothPlayerScarfFilled.dds", 40),
        ("white", "ClothPlayerScarfFilledClothPlayerScarfFilled.dds", 424),
    ):
        data = (source / name).read_bytes()
        if (data[:4] != b"DDS " or struct.unpack_from("<II", data, 12) != (2048, 512) or
                struct.unpack_from("<7I", data, 80) != (65, 0, 32, 0xff0000, 0xff00, 0xff, 0xff000000) or
                len(data) < 128 + 2048 * 512 * 4):
            raise ValueError("Unexpected scarf texture format: " + name)
        pixels = bytearray()
        for y in range(80):
            for x in range(2048):
                offset = 128 + (x * 512 + left + 79 - y) * 4
                pixels.extend(data[offset:offset + 4])
        header += "static const unsigned char jc_scarf_" + label + "[] = " + array(pixels) + ";\n"
    return header


def chat_font_header():
    import xml.etree.ElementTree as ET
    data = (ROOT / "JourneyPC/Data/Textures/Bin/Font2Font2.dds").read_bytes()
    root = ET.parse(ROOT / "JourneyPC/font2.fnt").getroot()
    if data[:4] != b"DDS " or struct.unpack_from("<II", data, 12) != (2048, 2048) or struct.unpack_from("<7I", data, 80) != (65, 0, 32, 0xff0000, 0xff00, 0xff, 0xff000000):
        raise ValueError("Unexpected native font texture")
    glyphs = {int(g.attrib['id']): {k:int(v) for k,v in g.attrib.items()} for g in root.find('chars')}
    raw = bytearray(); rows = []
    for code in range(32,127):
        g=glyphs[code]
        rows.append('{'+','.join(str(v) for v in (len(raw),g['width'],g['height'],g['xoffset'],g['yoffset'],g['xadvance']))+'}')
        for y in range(g['height']):
            for x in range(g['width']):
                raw.append(data[128+((g['y']+y)*2048+g['x']+x)*4+3])
    encoded=bytearray();i=0
    while i<len(raw):
        n=1
        while n<255 and i+n<len(raw) and raw[i+n]==raw[i]:n+=1
        encoded.extend((n,raw[i]));i+=n
    return ('struct jnet_chat_glyph { unsigned offset; int w,h,x,y,advance; };\n'
            'static const struct jnet_chat_glyph jnet_chat_glyphs[]={'+','.join(rows)+'};\n'
            '#define JNET_CHAT_FONT_BYTES '+str(len(raw))+'\n'
            'static const unsigned char jnet_chat_font_rle[]='+array(encoded)+';\n')


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


def imports(data):
    sections = pe_sections(data)
    rva, size = directory(data, 1)
    if not rva:
        return []
    base = file_offset(sections, rva, size)
    result = []
    for offset in range(base, base + size - 19, 20):
        descriptor = struct.unpack_from("<IIIII", data, offset)
        if not any(descriptor):
            return result
        result.append(c_string(data, file_offset(sections, descriptor[3])))
    raise ValueError("Unterminated imports")


def verify_reference(data):
    if sha(data) != EXPECTED_EXE:
        raise ValueError("Unrecognized reference executable; refuse to generate native addresses")
    return pe_sections(data)


def build(source_check=False):
    work = LAB / "work/diagnostic-build"
    package = LAB / ("work/production-check" if source_check else f"work/package-{VERSION}/JNET")
    deps = LAB / "work/deps"
    zig = deps / "zig-macos-aarch64-0.13.0/zig"
    minhook = deps / "minhook-1.3.4"
    if not zig.is_file() or not (minhook / "src/hook.c").is_file():
        raise ValueError("Approved workspace toolchain is missing; no downloads are performed by this script")
    for path in (work, package, LAB / "work/zig-cache", LAB / "work/zig-local-cache", LAB / "work/tmp"):
        if not path.resolve().is_relative_to(LAB / "work"):
            raise ValueError("Output path leaves workspace work directory")
        path.mkdir(parents=True, exist_ok=True)
    data = (ROOT / "JourneyPC/Journey.exe").read_bytes()
    sections = verify_reference(data)
    audio_game = (ROOT / "Journey-sound-fix-0.3.1.exe").read_bytes()
    if sha(audio_game) != "0a73c2fb3a2e5b33f6c38082b966160cd54882fac9efbe09c3ea8dde6fc037ca":
        raise ValueError("Unexpected optional sound-fix reference executable")
    (work / "jnet_chat_font.h").write_text(chat_font_header())
    selected = ("GameUpdate", "lua_gettop", "lua_settop", "luaL_loadbufferx", "lua_pcallk", "lua_tolstring", "lua_pushcclosure")
    signatures = {item["symbol"]: item for item in audit_signatures(data, sections)}
    pe = struct.unpack_from("<I", data, 60)[0]
    image_size = struct.unpack_from("<I", audio_game, pe + 24 + 56)[0]
    header = "#ifndef JC_TARGET_BUILD_H\n#define JC_TARGET_BUILD_H\n#include <stdint.h>\n#include <string.h>\n"
    header += f"#define JC_IMAGE_SIZE {image_size}u\n"
    header += f"static const unsigned char jc_game_hash[32] = {array(hashlib.sha256(data).digest())};\n"
    original_size = struct.unpack_from("<I", data, pe + 24 + 56)[0]
    header += f"static const unsigned char jc_audio_hash[32] = {array(hashlib.sha256(audio_game).digest())};\n"
    header += f"static inline unsigned jc_executable_size(const unsigned char *hash) {{ return !memcmp(hash,jc_game_hash,32) ? {original_size}u : !memcmp(hash,jc_audio_hash,32) ? {image_size}u : 0; }}\n"
    header += f"#define JC_ORIGINAL_IMAGE_SIZE {original_size}u\n"
    entries = []
    resolved = {}
    for name in selected:
        candidates = signatures[name]["hits"]
        if len(candidates) != 1:
            raise ValueError("Required entry point is missing or ambiguous: " + name)
        hit = candidates[0]
        resolved[name] = hit
        rva = hit["rva"]
        offset = file_offset(sections, rva, 16)
        entries.append("{" + str(rva) + "u," + array(data[offset:offset + 16]) + "}")
        header += f"#define JC_RVA_{name} {rva}u\n"
    # Static native lifecycle evidence is recorded in README; no signature guessing.
    for name, rva in {"CharacterMeshUpdate": 0x9dad0, "CharacterShoutInit": 0x13f6c0, "CharacterDecorationUpdate": 0x9f5f0, "CharacterDecorationDraw": 0x9f820, "CharacterDecorationCleanup": 0x9ea20, "CharacterInteract": 0x218e50, "IntroUpdate": 0x143490, "TextureNullGuard": 0x9e3a8, "CharacterBank": 0x2c8b20, "CharacterBodyInput": 0xa54a0, "CharacterLinkEmitter": 0x23cf70, "CharacterWhaleRide": 0x1c2c50, "CharacterClothTouch": 0x179630, "CharacterClothRide": 0x235210, "CharacterEye": 0x1c53e0, "CharacterJump": 0x9a890, "CharacterCamera": 0x22ed60, "CharacterSkeletonBuild": 0x2d4530, "CharacterAnimationLoad": 0x284930, "CharacterAnimationFinish": 0x284d40, "CharacterAssetLoad": 0x2d0610, "CreatureReset": 0x9b820, "MenuInput": 0x105ed0, "MenuPop": 0x107db0, "MenuFrame": 0x21aa70, "MenuPresent": 0x36d330, "MenuBuild": 0x105840, "MenuNodes": 0x104870, "MenuAppend": 0x1024a0, "MenuFrameDrawSite": 0x21ac61,
                      "MenuMap": 0x267d10, "MenuMapImpl": 0x345de0, "MenuFinish": 0x102310,
                      "ClothRender": 0x8a7a0, "ActorInput": 0x118030, "ActorFinish": 0x118df0,
                      "ActorNativeInput": 0xa54a0, "ActorNativeFinish": 0xa9d20,
                      "AsyncGeometryDispatch": 0x352060, "GraphicsMutex": 0xc0840,
                      "GraphicsEnter": 0x374b80, "GraphicsLeave": 0x374c00, "NativeThread": 0x34bff0,
                      "TextureSamplerCreate": 0x362db0, "EnvironmentRefRelease": 0x8e9b0,
                      "ClothDependency": 0x8e210,
                      "ClothUpdate": 0x89a00, "ClothFinish": 0x8a3a0, "MeshClothDefinition": 0x2610a0,
                      "DudeClothInit": 0x246230, "DudeClothAttach": 0x246470, "DudeClothCollide": 0x246d40,
                      "DudeClothAppearance": 0xa9470, "ClothRelease": 0x87880, "NativeShout": 0x13fbb0, "EvalSubmit": 0x37aed0, "EvalWait": 0x37b1b0, "EvalWorker": 0x37a570,
                      "EvalRootWrite": 0x395d10, "EvalSamplerPrepare": 0x398040, "EvalSamplerCompute": 0x3975f0,
                      "EvalNormalPrepare": 0x38ea80, "EvalAddPrepare": 0x391280, "EvalBlendCompute": 0x38fbb0,
                      "EvalNormalBlend": 0x38ed20, "EvalAddBlend": 0x391520, "EvalClock": 0x25fab0,
                      "EvalNormalFlags": 0x38e950, "EvalAddFlags": 0x391130, "EvalLeafFlags": 0x3f9870,
                      "EvalInactivePolicy": 0x3f9850,
                      "EvalSingleSubmit": 0x411780, "EvalThreadedSubmit": 0x412020, "EvalThreadedImmediate": 0x412040,
                      "EvalSingleFlush": 0x4117d0, "EvalSingleWait": 0x4117e0,
                      "EvalThreadedFlush": 0x412060, "EvalThreadedWait": 0x412130,
                      "EvalTempAlloc": 0x3de240, "EvalTempReserve": 0x3de260, "EvalScheduler": 0x39c870,
                      "RemoteReceive": 0x977a0, "RemoteApply": 0x118730, "PlayerUpdate": 0x118690,
                      "DudeUpdate": 0xa6350, "DudeSimulation": 0x24ab80, "PlayerAppearance": 0x2177c0,
                      "ClothAppearance": 0x24c010, "ChestSymbol": 0xa8d70, "ChirpSymbol": 0xa8930,
                      "PlayerShout": 0x24e290, "ShoutUpdate": 0x13e6b0, "RemoteDeparture": 0x24fa70,
                      "SandInline": 0x245e10, "SandInlineResume": 0x246125, "SandAppend": 0x10f0a0, "SandConsume": 0x10e360, "DudePrepare": 0xa2ec0, "DudeFullInit": 0xa32c0, "DudeFactory": 0xabcd0, "DudeDelete": 0xa2470,
                      "DudeConstructor": 0xa1e80, "DudeDestructor": 0xa24b0, "DudeCleanup": 0xa3fe0, "DudeResourceInit": 0xa4a60,
                      "ResourceAttach": 0x2cca00, "MeshInit": 0x263190, "MeshCleanup": 0x263090, "MeshRegister": 0x262b00,
                      "PropertyClear": 0x273000, "CloneReference": 0x91550, "PropertyAssign": 0x272580,
                      "ReferenceRelease": 0x91820, "GeometryBind": 0x266420, "GeometryBuild": 0x2710d0, "MaterialCommit": 0x274d70, "MaterialSamplerInit": 0x272580, "MeshUpdate": 0x263aa0, "AnimationSubmit": 0x262680, "AnimationFinish": 0x2628d0, "TravelerSubmit": 0x117790, "RenderConsume": 0x2d16c0, "RenderCull": 0x2d1490, "GeometryDispatch": 0x34ee80, "CommandBuild": 0x34e990, "RenderFlush": 0x352de0, "RenderHandoff": 0x378810, "BaseFlush": 0x352540, "BaseHandoff": 0x3786b0, "CommandExecute": 0x3770b0, "SavePoll": 0x1398b0, "SaveLoad": 0x138730, "SaveGame": 0xd7c00, "ScriptExecute": 0x26a8b0, "CurrentLevelString": 0xda810, "MaterialScalarLookup": 0x273a00, "MaterialVectorSet": 0x273200, "MaterialTextureSet": 0x273640}.items():
        offset = file_offset(sections, rva, 16)
        resolved[name] = dict(rva=rva, file_offset=offset)
        entries.append("{" + str(rva) + "u," + array(data[offset:offset + 16]) + "}")
        header += f"#define JC_RVA_{name} {rva}u\n"
    for name, target in resolved.items():
        offset = target['file_offset']
        if data[offset:offset+16] != audio_game[offset:offset+16]:
            raise ValueError("Sound-fix overlaps JNET target: " + name)
    header += "struct jc_target { uint32_t rva; unsigned char prologue[16]; };\n"
    header += "static const struct jc_target jc_targets[] = {" + ",".join(entries) + "};\n"
    reloc_rva, reloc_size = directory(data, 5)
    if reloc_size:
        start = file_offset(sections, reloc_rva, reloc_size)
        end = start + reloc_size
        while start < end:
            page, block_size = struct.unpack_from("<II", data, start)
            if block_size < 8 or block_size % 2 or start + block_size > end:
                raise ValueError("Invalid relocation block")
            for at in range(start + 8, start + block_size, 2):
                entry = struct.unpack_from("<H", data, at)[0]
                if entry >> 12:
                    location = page + (entry & 4095)
                    if any(location < h["rva"] + 16 and location + 8 > h["rva"] for h in resolved.values()):
                        raise ValueError("An entry-point fingerprint needs relocation normalization")
            start += block_size
    probe = (LAB / "companion_probe.lua").read_text()
    bootstrap = "local print = ...\nassert(type(print) == 'function')\nassert(_VERSION == 'Lua 5.3')\nassert(rawget(_G, 'JourneyCompanionProbe') == nil)\n"
    bootstrap += probe + "\nassert(JourneyCompanionProbe)\nJourneyCompanionProbe.bindings()\n"
    embedded = "static const char jc_embedded_probe[] =\n" + "\n".join(json.dumps(line + "\n") for line in bootstrap.splitlines()) + ";\n"
    (work / "embedded_probe.h").write_text(embedded)
    receipt = json.loads((deps / "receipt.json").read_text())
    startup_source = extract_persistent(data)
    startup_candidate, startup_changes = prepare_candidate(startup_source, active_traveler=True)
    character_startup = character_resource_script(data, participants=3)
    wayfarer_startup = wayfarer_resource_script()
    startup_candidate = wayfarer_startup + startup_candidate + character_startup
    startup_header = f"#define JC_STARTUP_SOURCE_RVA {SCRIPT_RVA}u\n#define JC_STARTUP_SIZE {SCRIPT_SIZE}u\n"
    startup_header += f"#define JC_STARTUP_CANDIDATE_SIZE {len(startup_candidate)}u\n"
    startup_header += '#define JC_NATIVE_CHARACTERS_ENABLED 1\nstatic const char jc_character_startup[] = ' + json.dumps(character_startup.decode('ascii')) + ';\n'
    startup_header += "static const char jc_wayfarer_startup[] = " + json.dumps(wayfarer_startup.decode("ascii")) + ";\n"
    startup_header += "static const unsigned char jc_startup_source_hash[32] = " + array(hashlib.sha256(startup_source).digest()) + ";\n"
    startup_header += "static const unsigned char jc_startup_candidate_hash[32] = " + array(hashlib.sha256(startup_candidate).digest()) + ";\n"
    startup_header += "static const unsigned jc_startup_offsets[] = {" + ",".join(str(c['buffer_offset']) for c in startup_changes) + "};\n"
    for field in ("before", "after"):
        startup_header += f"static const unsigned char jc_startup_{field}[] = " + array(bytes(48+c[field] for c in startup_changes)) + ";\n"
    (work / "startup_candidate.h").write_text(startup_header)
    source_parts = [header.encode(), bootstrap.encode(), json.dumps(receipt, sort_keys=True).encode()]
    source_parts.append(startup_header.encode())
    night = daynight_runtime_header()
    (work / "jnet_daynight_payload.h").write_text(night)
    source_parts.append(night.encode())
    characters = character_runtime_header(data)
    animation_bundles = character_animation_files(data)
    animation_bundles.update(character_mesh_files(data))
    characters += character_animation_file_header(animation_bundles)
    source_parts.extend(animation_bundles.values())
    (work / "jnet_character_payloads.h").write_text(characters)
    source_parts.append(characters.encode())
    banners = scarf_banner_header()
    (work / "scarf_banners.h").write_text(banners)
    source_parts.append(banners.encode())
    source_parts.append((work / "jnet_chat_font.h").read_bytes())
    source_parts.append((LAB / "work/menu-images/jnet_menu_assets.h").read_bytes())
    for file in sorted(HERE.glob("*")):
        if file.suffix in (".c", ".h", ".py"):
            source_parts.append(file.read_bytes())
    build_id = hashlib.sha256(b"\0".join(source_parts)).digest()
    header += "#define JC_BUILD_ID " + array(build_id) + "\n#endif\n"
    (work / "target_build.h").write_text(header)
    env = dict(os.environ)
    env.update(ZIG_GLOBAL_CACHE_DIR=str(LAB / "work/zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(LAB / "work/zig-local-cache"), TMPDIR=str(LAB / "work/tmp"))
    flags = [str(zig), "cc", "-target", "x86_64-windows-gnu", "-O2", "-std=c11", "-fms-extensions", "-DUNICODE", "-D_UNICODE",
             "-Wall", "-Wextra", "-Werror", "-Wno-unused-const-variable", "-I" + str(work), "-I" + str(minhook / "include")]
    hook_sources = [minhook / "src" / file for file in ("buffer.c", "hook.c", "trampoline.c", "hde/hde64.c")]
    subprocess.run(flags + ["-shared", str(HERE / "probe_dll.c"), *map(str, hook_sources), "-lwinmm", "-lbcrypt", "-lws2_32", "-lgdi32", "-o", str(package / "JNET.dll")],
                   env=env, check=True, cwd=work)
    # Compile the exact observer shims against a standalone canary target, never Journey.
    fixture = LAB / "work/companion-components/test_native_lifecycle_abi.exe"
    fixture.parent.mkdir(exist_ok=True)
    subprocess.run(flags + [str(HERE / "test_native_lifecycle_abi.c"), "-o", str(fixture)], env=env, check=True, cwd=work)
    dll = (package / "JNET.dll").read_bytes()
    exported = exports(dll)
    live_exports = {'JNETSteamGame', 'JNETSteamP2P', 'JNETSteamP2PText', 'JNETSteamLobby', 'JNETSteamLobbyText', 'JNETSteamServiceText', 'JNETSteam', 'JNETMenu', 'JNETNetwork', 'JNETPeerLifecycle', 'JNETActorTrial', 'JNETActorTrialStart', 'JNETActorTrialStop', 'JNETClothAdoptMappedPass', 'JNETClothBindNativeMaps', 'JNETClothPublish', 'JNETActorActivate', 'JNETClothInitialize', 'JNETActorStop', 'JNETClothApplyAnimatedAppearance', 'JNETActorPrepare', 'JNETActor', 'JNETActorReceive', 'JNETClothStageStockBatch'}
    if set(exported) != (live_exports | {"JNETTraveler", "JNETLifecycle", "JNETStatus", "JNETCommand", "JNETBuildId", "JNETInitialize", "JNETReaderCheck", "JNETControl", "JNETObservation", "JNETPhase", "JNETFrames", "JNETGame", "JNETTrial", "JNETLifetime", "JNETResource", "JNETPool", "JNETMesh", "JNETMaterial", "JNETGeometry", "JNETBuilder", "JNETStartup", "JNETCommit", "JNETCallbacks", "JNETPose", "JNETPoseWrites", "JNETAnimation", "JNETIdle", "JNETRender", "JNETMeshLife", "JNETAppearance", "JNETLink", "JNETRoom", "JNETMotion", "JNETBodies", "JNETDual", "JNETSkeleton", "JNETEvaluation", "JNETNativePlayback", "JNETNativeOutputs", "JNETCompanionFeatures", "JNETFlight", "JNETLinkConfig", "JNETPerformance"}):
        raise ValueError("Unexpected DLL control exports")
    dll_sections = pe_sections(dll)
    id_offset = file_offset(dll_sections, exported["JNETBuildId"], 32)
    if dll[id_offset:id_offset + 32] != build_id:
        raise ValueError("Exported build identifier mismatch")
    built_header = '#define JC_HELPER_VERSION L"' + ("development" if source_check else VERSION) + '"\n'
    built_header += f"static const unsigned char jc_dll_hash[32] = {array(bytes.fromhex(sha(dll)))};\n"
    for macro, symbol in (("STEAM_GAME", "JNETSteamGame"), ("STEAM_P2P", "JNETSteamP2P"), ("STEAM_P2P_TEXT", "JNETSteamP2PText"), ("STEAM_LOBBY", "JNETSteamLobby"), ("STEAM_LOBBY_TEXT", "JNETSteamLobbyText"), ("STEAM_TEXT", "JNETSteamServiceText"), ("STEAM", "JNETSteam"), ("MENU", "JNETMenu"), ("PEER_LIFECYCLE", "JNETPeerLifecycle"), ("CONTROL", "JNETControl"), ("ACTOR_TRIAL", "JNETActorTrial"), ("ACTOR", "JNETActor"), ("TRAVELER", "JNETTraveler"), ("LIFECYCLE", "JNETLifecycle"), ("STATUS", "JNETStatus"), ("COMMAND", "JNETCommand"), ("BUILD", "JNETBuildId"), ("INIT", "JNETInitialize"), ("OBSERVATION", "JNETObservation"), ("PHASE", "JNETPhase"), ("FRAMES", "JNETFrames"), ("GAME", "JNETGame"), ("TRIAL", "JNETTrial"), ("LIFETIME", "JNETLifetime"), ("RESOURCE", "JNETResource"), ("POOL", "JNETPool"), ("MESH", "JNETMesh"), ("MATERIAL", "JNETMaterial"), ("GEOMETRY", "JNETGeometry"), ("BUILDER", "JNETBuilder"), ("STARTUP", "JNETStartup"), ("COMMIT", "JNETCommit"), ("CALLBACKS", "JNETCallbacks"), ("POSE", "JNETPose"), ("POSE_WRITES", "JNETPoseWrites"), ("ANIMATION", "JNETAnimation"), ("IDLE", "JNETIdle"), ("RENDER", "JNETRender"), ("MESH_LIFE", "JNETMeshLife"), ("APPEARANCE", "JNETAppearance"), ("LINK", "JNETLink"), ("ROOM", "JNETRoom"), ("MOTION", "JNETMotion"), ("BODIES", "JNETBodies"), ("DUAL", "JNETDual"), ("SKELETON", "JNETSkeleton"), ("EVALUATION", "JNETEvaluation"), ("NATIVE_PLAYBACK", "JNETNativePlayback"), ("NATIVE_OUTPUTS", "JNETNativeOutputs"), ("COMPANION_FEATURES", "JNETCompanionFeatures"), ("FLIGHT", "JNETFlight"), ("LINK_CONFIG", "JNETLinkConfig"), ("NETWORK", "JNETNetwork"), ("PERFORMANCE", "JNETPerformance")):
        built_header += f"#define JC_{macro}_RVA {exported[symbol]}u\n"
    (work / "built_dll.h").write_text(built_header)
    subprocess.run(flags + ["-municode", "-Wl,--subsystem,windows", str(HERE / "probe_helper.c"), "-lbcrypt", "-luser32", "-lgdi32", "-lcomdlg32", "-o", str(package / "JNET.exe")],
                   env=env, check=True, cwd=work)
    helper = (package / "JNET.exe").read_bytes()
    pe_sections(helper)
    helper_pe = struct.unpack_from("<I", helper, 60)[0]
    if struct.unpack_from("<H", helper, helper_pe + 24 + 68)[0] != 2:
        raise ValueError("JNET must use the Windows GUI subsystem")
    allowed_imports = {"comdlg32.dll", "winmm.dll", "kernel32.dll", "msvcrt.dll", "user32.dll", "gdi32.dll", "bcrypt.dll", "ntdll.dll", "ws2_32.dll"}
    allowed_imports.update("api-ms-win-crt-" + name + "-l1-1-0.dll" for name in
                           ("private", "runtime", "stdio", "string", "environment", "heap", "time", "math", "utility"))
    dll_imports, helper_imports = imports(dll), imports(helper)
    if (set(map(str.lower, dll_imports + helper_imports)) - allowed_imports):
        raise ValueError("Unexpected non-system runtime dependency")
    if source_check:
        result = dict(compile_link="passed", runtime_tested=False, installed=False,
                      release_package=False, artifact=str((package / "JNET.dll").relative_to(LAB)),
                      sha256=sha(dll), helper_sha256=sha(helper), build_id=build_id.hex(), startup_changes=startup_changes,
                      sources={str(p.relative_to(LAB)): sha(p.read_bytes())
                               for p in sorted(HERE.iterdir()) if p.suffix in (".c", ".h", ".py")})
        (LAB / "work/production-paths-verification.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps({k: result[k] for k in ("compile_link", "artifact", "sha256", "installed")}, indent=2))
        return
    (package / "JNETMenu.xme").write_bytes(menu_bytes())
    (package / "README.txt").write_text((HERE / "README.txt").read_text().replace("@VERSION@", VERSION))
    for name, content in animation_bundles.items():
        (package / name).write_bytes(content)
    manifest = dict(version=VERSION, automatic_diagnostic_logs=False, session_report_export=True, meet_everywhere_always=True, portable_windows_package=True, game_sha256=EXPECTED_EXE, optional_sound_fix_sha256=sha(audio_game), sound_fix_version="0.3.1", optional_drop_in_executable=True, build_id=build_id.hex(),
                    runtime_verified=False, three_player_supported=False, local_diagnostic_only=False, steam_lobby_transport_test=True, gameplay_over_steam=True,
                    group_travel_candidate=True, all_levels_runtime_verified=False,
                    native_character_candidates=["Longtail", "Whale", "War Machine", "fl0w 1", "fl0w 2"], native_character_preload=True,
                    native_character_asset_mapping=False, native_character_resident_textures=True, character_asset_load_trace=True,
                    native_character_animation_bundles=False, native_character_animation_loose_files=True, isolated_animation_probe=False, private_animation_asset_namespace=True, private_mesh_asset_namespace=True, character_skeleton_stage_trace=True,
                    character_animation_stage_trace=True, native_characters_runtime_verified=False, war_machine_available=True, war_machine_attacks=False, war_machine_spotlight=True, creature_camera_candidate=True, war_machine_resources_packaged=True, character_visibility_snapshots=True,
                    creature_appearance_replication=True, rocket_character=True, rocket_relay_participants=3, rocket_runtime_verified=False, character_resource_participants=3,
                    private_character_decoration_pool=True, native_chirps_in_private_pool=True,
                    creature_jump_ascent=True, spotlight_input="interact/sit", creature_flight_audio_muted=False, creature_body_physics_bypassed=True, creature_motion_smoothed=True, creature_speed={"Longtail":10.8,"Whale":6.9,"War Machine":6.9,"fl0w 1":10.8,"fl0w 2":10.8},
                    menu_option_wrap=True, intro_skip_visible=False, creature_vortex_decay=True, creature_cached_teleport_completion=True, creature_cached_resume_before_teleport_wait=True, intro_skip_any_input_twice=True, fps_choices=[30,60,"Default"], intro_mapped_loading_input=True, longtail_chirp_head=True, missing_texture_null_guard=True, bryan_tapestry_diagnostic=True, bryan_texture_replaced=False, creature_private_sound_banks=["Summit", "Cave"],
                    startup_changes=startup_changes, compiler="Zig 0.13.0 x86_64-windows-gnu",
                    minhook="1.3.4", dll_imports=dll_imports, helper_imports=helper_imports,
                    exports=exported, source_entry_points=resolved,
                    artifacts={name: sha((package / name).read_bytes()) for name in ("JNET.dll", "JNET.exe", "README.txt", "JNETMenu.xme", *animation_bundles)})
    (package / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    archive = LAB / f"work/JNET-{VERSION}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
        for name in sorted([*manifest["artifacts"], "manifest.json"]):
            output.write(package / name, "JNET/" + name)
    print(json.dumps(dict(package=str(archive.relative_to(ROOT)), sha256=sha(archive.read_bytes()),
                          size=archive.stat().st_size, runtime_verified=False), indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-check", action="store_true", help="Build the live source DLL separately; no helper package or installation")
    build(parser.parse_args().source_check)
