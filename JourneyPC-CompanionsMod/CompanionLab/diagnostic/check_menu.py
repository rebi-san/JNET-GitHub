from pathlib import Path
import subprocess
import struct
import re
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from inspect_menu import menu_contract, font_contract
from inspect_build import ROOT, LAB, pe_sections
from native_map import offset_for, runtime_functions


def main():
    data = (ROOT / "JourneyPC/Journey.exe").read_bytes()
    sections = pe_sections(data)
    source = (HERE / "probe_dll.c").read_text()
    signature = re.search(r"queue_entry\[\]=\{([^}]+)\}", source)
    if signature is None:
        raise ValueError("QueueLevel startup signature missing")
    expected = bytes(int(value.strip(), 16) for value in signature.group(1).split(","))
    at = offset_for(sections, 0xda6c0, len(expected))
    if not expected or data[at:at + len(expected)] != expected:
        raise ValueError("Production QueueLevel startup signature does not match Journey")
    print("Production QueueLevel startup signature matches pinned Journey executable.")
    at = offset_for(sections, 0x21b19f, 5)
    if data[at:at + 5] != bytes.fromhex("e8 1c f5 eb ff"):
        raise ValueError("Native Chapter Select QueueLevel caller changed")
    header = (HERE / "jnet_character_native.h").read_text()
    glow_type = int(re.search(r"#define JNET_CHARACTER_GLOW_VTABLE_RVA (0x[0-9a-f]+)u", header)[1], 16)
    for ctor in (0x17a390, 0x17a412):
        at = offset_for(sections, ctor, 7)
        instruction = data[at:at+7]
        if instruction[:3] not in (b"\x48\x8d\x05", b"\x48\x8d\x0d") or ctor+7+struct.unpack_from("<i", instruction, 3)[0] != glow_type:
            raise ValueError("Cloth glow type does not match native constructor")
    print("Cloth glow type matches both native constructors.")
    creature = {
        0x179630: "4c8bdc534154415641574881ecb8000000",
        0x179300: "48895c24084889742410574883ec20",
        0x664178: "a093174001000000",
        0x6641b0: "0093174001000000",
        0x1c2b70: "48895c24084889742410574883ec2048",
        0x1c2c50: "488bc44889581048897018574881ecc0000000",
        0x235210: "488bc4488958084889701855574156",
        0x23cf70: "4885d27507889194000000c344888194000000",
        0x23d0e4: "e887feffff",
        0x23d0f1: "e87afeffff",
        0x23d0fa: "e871feffff",
        0x1b85a4: "33d2488bc8488bd8448d4268e8d3342e00",
        0x1b813e: "898150120000",
        0x2387c4: "c7815c12000002000000",
        0x23a0f7: "89875c120000",
        0x23a182: "88880d070000",
        0x1b823b: "398350120000",
        0x1b8247: "4489a350120000",
        0x1b8260: "e81b050800",
        0x1b8436: "4585ff498b6b300f95c0884609",

        0x23de9a: "f30f1096f8000000410f28c6f30f5986441c0000f30f5cd0410f2fd4f30f1196f8000000",
        0xab068: "f30f10aff8000000",
        0x117b50: "66c741080101",
        0x1b80c0: "c6410800",
        0x1b8440: "884609",
        0x66fd50: "70811b4001000000",

        0x9f890: "ff8688050000",
        0x9f896: "83bc1ff406000000",
        0x9f8a4: "80bc1f1c07000000",
        0x9f8b2: "80bc1f1e07000000",
        0x9f905: "4863841f400f0000",
        0x9dc72: "440fb66330",
        0x9dc89: "80bb600f000000",
        0x9dc92: "488b4b18",
        0x9dc9b: "488b4910",
        0x9dca4: "488b81a8000000",
        0x9dcab: "4883781800",
        0x9dc65: "8983e4060000",
        0x9ddc6: "488b83400f0000",
        0x9ddd2: "8b88b0910000",
        0x9dddc: "488b93580f0000",
        0x9dde3: "3b4a10",
        0x9de22: "f30f11bb500f0000",
        0x9db17: "0f109370050000",
        0x65ab90: "c090154001000000",
        0x65abc8: "7089154001000000",
        0x158970: "4c8bdc555357498dabe8feffff4881ec00020000",
        0x1598c9: "488b43784c8d45104c8b8b80000000",
        0x236e3e: "488b03ff5048488b03488bcbff5028",
        0x9cb80: "8991300f0000",
        0x68b3a8: "0000000000002240",
        0x82340: "40534883ec700f29742460498bd84d8b40100f297c24",
        0xa5bc9: "498b87d00000004889442430",
        0xaa4fc: "488bac2400010000",
        0xaa57b: "4c8bc5",
        0x1403af: "488b8424280100004803cb4889442448488b842418",

        0x2d0610: "4889542410535741544155415641574883ec68",
        0x9b660: "488d05e12f530041b870000000488d5110",
        0x9bea0: "8b91101808008d4201898110180800488d41104869d2801200004803c2c3",
        0x9abc0: "488d0579395300488901488d421048894108488bc1c3",
        0xdc6e8: "498b8dc8010000",
        0x238fed: "488b4f10",
        0x81bf0: "488d4108c3",
        0x1182f5: "488b4740488b4808488b8160020000",
        0x11830d: "0f108080000000",
        0x118344: "488d45c7f30f1045bb0f57d20f5705b99b4a000f28d8f30f10000f57c90f14da450f57db0f14c10f14c3",
        0x7f2b0: "488b41280f288070010000488bc2660f7f02c3",
        0x7f2f0: "488b41280f2805152c54000f578090010000488bc20f1102c3",
        0x9b820: "33d20f57c0899110180800",
        0x236e80: "488bc45341574881ec08010000",
        0x5c1f10: "00000080000000800000008000000080",
    }
    for rva, expected in creature.items():
        expected = bytes.fromhex(expected)
        at = offset_for(sections, rva, len(expected))
        if data[at:at + len(expected)] != expected:
            raise ValueError(f"Native creature contract changed at {rva:x}")
    print("Native creature contract: 112 slots, 0x1280 stride, unchecked allocator, Game barn and movement target verified offline.")
    isolation = {
        0x1382f6: "488d057bae4a00488bd9488901",
        0x14dd0a: "488bd5e89ecb1100",
        0x187f67: "66c741080101", 0x170e22: "66c742090001", 0x170e9c: "c6430a01", 0x17daba: "884309488b5c2430",
        0x376ff58: "d8855d4001000000689d5d4001000000c0a60d4001000000",
        0xfaa9f: "418bd6488d0d47334e00488b18e8df851e004c8bc0488bcbe804411b00",
        0x2aee76: "498b4e10488d15afbd32004533c94d8bc7488b01ff5028498b4e10488b01ff5020",
        0x2af1fb: "498b4f10458bc433d2488b01ff5068",
        0x2b062e: "498b4d10498b5518488b01ff90a0000000498b4d1041b802000000498b5518488b01ff9008010000",
        0x29b82f: "488b0b4c8d0dd77d3e00448bc78bd6488b01ff908000000048894340",
        0x2a6886: "ff158cd82a00488b484049894c2410",
        0x2b3c6c: "488b4810c744242800000000c744242002000000488b01ff10",
        0x2b4513: "498b0424498b5500488b4810488b01ff5018",
        0x15b240: "4055535741554157488d6c24c94881ec",
        0xaa239: "0fb68200010000888100010000", 0x184920: "f30f114314f30f114b18",
        0x5dac30: "6c6576656c00", 0x5dddf0: "256400",
    }
    for rva, expected in isolation.items():
        expected = bytes.fromhex(expected)
        at = offset_for(sections, rva, len(expected))
        if data[at:at + len(expected)] != expected:
            raise ValueError(f"Native matchmaking isolation contract changed at {rva:x}")
    print("Pinned native isolation: decimal level search, private-first publication, legacy P2P send/accept call sites verified offline. Live isolation not exercised.")
    menu_contract(data)
    font_contract(ROOT / "JourneyPC/font2.fnt")
    sections = pe_sections(data)
    depth = struct.unpack_from("<I", data, offset_for(sections, 0x68b248, 4))[0]
    if depth != 0x3f7ff972:
        raise ValueError("Native menu text depth changed")
    work = LAB / "work/menu"
    work.mkdir(exist_ok=True)
    travel_source = (HERE / "jnet_travel_live.h").read_text()
    chapter_start = travel_source.index("static uintptr_t jnet_chapter_menu,")
    chapter_end = travel_source.index("static int jnet_travel_level_ready(", chapter_start)
    (work / "chapter_code.h").write_text(travel_source[chapter_start:chapter_end])
    patch_start = travel_source.index("static int jnet_mepal_site(")
    patch_end = travel_source.index("\n#endif", patch_start)
    (work / "mepal_code.h").write_text(travel_source[patch_start:patch_end])
    lines = ["static void load_menu_code(unsigned char *image) {"]
    for rva, length in ((0x372ad0, 0x50), (0x1024a0, 0x1fe), (0x1042b5, 0xd1), (0x68b24c, 4), (0xda6c0, 0x14e), (0xd7fc34, 4), (0x1468e0, 0x94)):
        at = offset_for(sections, rva, length)
        code = ",".join(str(b) for b in data[at:at + length])
        lines.append("{ static const unsigned char code[]={" + code + "};")
        lines.append(f"memcpy(image+{rva},code,sizeof(code));}}")
    lines.append("image[0x104386]=0xc3;}")
    (work / "menu_code.h").write_text("\n".join(lines) + "\n")
    functions={start:end-start for start,end,_ in runtime_functions(data,sections)}
    lines=["static void load_environment_code(unsigned char *image) {"]
    spans=[(0xc5880,0x5e70-0x5880),(0xc6140,0x67d0-0x6140)]
    spans += [(rva,functions[rva]) for rva in (0xc69b0,0x7a1b0)]
    spans += [(0x5c1e00,0x200),(0x677870,16),(0x68ae80,0x400),(0x68c000,16),(0x7a20cc,4)]
    for rva,length in spans:
        at=offset_for(sections,rva,length)
        code=",".join(str(b) for b in data[at:at+length])
        lines.append("{ static const unsigned char code[]={"+code+"};")
        lines.append(f"memcpy(image+{rva},code,sizeof(code));}}")
    lines.append("image[0xc84d0]=0xc3;image[0x274d70]=0xc3;}")
    (work/"environment_code.h").write_text("\n".join(lines)+"\n")

    appearance_source=(HERE / "jnet_appearance_live.h").read_text()
    save_start=appearance_source.index("static int jnet_appearance_local_saved(")
    save_end=appearance_source.index("static int jnet_appearance_boot(",save_start)
    (work / "save_poll_code.h").write_text(appearance_source[save_start:save_end])
    character_source=(HERE / "jnet_character_live.h").read_text()
    eye_start=character_source.index("static void (*jnet_character_eye_original)")
    eye_end=character_source.index("static unsigned char (*jnet_character_jump_original)",eye_start)
    (work / "character_eye_code.h").write_text(character_source[eye_start:eye_end])
    restore_start=character_source.index("static int jnet_character_restore_wayfarer(")
    restore_end=character_source.index("static void jnet_character_fail(",restore_start)
    body_start=character_source.index("static struct jnet_character_state *jnet_character_creature_for_dude(")
    body_end=character_source.index("static unsigned char (*jnet_character_touch_original)",body_start)
    (work / "character_body_code.h").write_text(character_source[restore_start:restore_end]+character_source[body_start:body_end])
    for name in ("test_jnet_resource_view", "test_jnet_character_body", "test_jnet_character_eye", "test_jnet_save_status", "test_jnet_frame_pacing", "test_jnet_texture_guard", "test_jnet_intro", "test_jnet_character_volume", "test_jnet_character_assets", "test_jnet_character_native", "test_jnet_character", "test_jnet_chat_input", "test_jnet_menu_draw", "test_jnet_menu_live", "test_jnet_menu_model", "test_jnet_lobby_runtime", "test_jnet_steam_probe", "test_jnet_steam_lobby", "test_jnet_rocket", "test_jnet_group_travel", "test_jnet_travel_native", "test_jnet_appearance_live", "test_jnet_freeplay_live", "test_jnet_flower_trail", "test_jnet_flower_view", "test_jnet_flower_dds"):
        binary = work / name
        subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-I", str(work), "-I", str(HERE),
                    str(HERE / (name + ".c")), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
