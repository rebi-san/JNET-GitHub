"""Inspect the pinned PC menu/font path without loading or modifying Journey."""
import hashlib
import json
import struct
import xml.etree.ElementTree as ET

from inspect_build import ROOT, LAB, EXPECTED_EXE, pe_sections
from native_map import offset_for


def menu_contract(data):
    if hashlib.sha256(data).hexdigest() != EXPECTED_EXE:
        raise ValueError("Unrecognized Journey executable")
    sections = pe_sections(data)
    sites = {
        0x1026e0: "MenuSystem.xml",
        0x102872: "Root.Config.Text",
        0x102991: "FontSize",
        0x102ebd: "Root.Config.Font.Name",
        0x10335b: "Root.Config.Font.Definitions",
        0x1033ee: "font.chars",
        0x10342d: "font.common.scaleW",
        0x1034b5: "font.common.scaleH",
        0x103828: "xadvance",
        0x103895: "xoffset",
        0x1038fc: "yoffset",
    }
    references = []
    for site, expected in sites.items():
        at = offset_for(sections, site, 7)
        instruction = data[at:at + 7]
        if instruction[0] not in (0x48, 0x4c) or instruction[1] != 0x8d or instruction[2] & 0xc7 != 0x05:
            raise ValueError(f"Unexpected menu reference instruction at {site:x}")
        target = site + 7 + struct.unpack_from("<i", instruction, 3)[0]
        encoded = expected.encode() + b"\0"
        at = offset_for(sections, target, len(encoded))
        if data[at:at + len(encoded)] != encoded:
            raise ValueError(f"Menu reference mismatch at {site:x}")
        references.append(dict(site=hex(site), target=hex(target), text=expected))
    return references


def font_contract(path):
    data = path.read_bytes()
    root = ET.fromstring(data)
    info, common = root.find("info"), root.find("common")
    width, height = int(common.attrib["scaleW"]), int(common.attrib["scaleH"])
    pages = {int(p.attrib["id"]): p.attrib["file"] for p in root.find("pages")}
    glyphs = {}
    for glyph in root.find("chars"):
        values = {k: int(v) for k, v in glyph.attrib.items()}
        code = values["id"]
        if code in glyphs or not 0 <= code <= 0x10ffff:
            raise ValueError("Invalid or duplicate font glyph")
        if (values["page"] not in pages or values["x"] < 0 or values["y"] < 0 or
                values["width"] < 0 or values["height"] < 0 or
                values["x"] + values["width"] > width or values["y"] + values["height"] > height):
            raise ValueError("Font glyph outside its atlas")
        glyphs[code] = values
    labels = ("Continue", "Settings", "Quit to Title", "Quit to Desktop", "JNET",
              "Host", "Mode", "Level", "Players", "Public", "Private", "Apply",
              "Join", "Leave", "Copy Invite", "Password", "Journey", "Rocket Death Match")
    missing = sorted({char for label in labels for char in label if ord(char) not in glyphs})
    if missing:
        raise ValueError("Required menu characters missing: " + repr(missing))
    return dict(sha256=hashlib.sha256(data).hexdigest(), face=info.attrib["face"],
                line_height=int(common.attrib["lineHeight"]), atlas=[width, height],
                pages=pages, glyph_count=len(glyphs), declared_glyph_count=int(root.find("chars").attrib["count"]),
                labels_supported=list(labels))


def main():
    data = (ROOT / "JourneyPC/Journey.exe").read_bytes()
    result = dict(exe_sha256=EXPECTED_EXE, references=menu_contract(data),
                  font_asset=font_contract(ROOT / "JourneyPC/font2.fnt"),
                  menu_initializer="0x1026a0", font_parser="0x103340",
                  runtime_verified=False,
                  unresolved=["Decoded PC MenuSystem font selection", "Text draw ABI and lifetime",
                              "Pause input focus and timing", "Live font resource identity"])
    output = LAB / "work/menu-contract.json"
    output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(dict(output=str(output.relative_to(ROOT)),
                          verified_references=len(result["references"]),
                          font_asset=result["font_asset"], runtime_verified=False), indent=2))


if __name__ == "__main__":
    main()
