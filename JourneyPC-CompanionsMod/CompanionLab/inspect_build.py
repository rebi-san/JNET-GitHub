import collections
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import tarfile


LAB = Path(__file__).resolve().parent
ROOT = LAB.parent.parent
SCRIPT_LIMIT = 8 * 1024 * 1024
EXPECTED_EXE = "16a7c176b9cce893e0ca72c5d0e4b2e7377c4fabec179f95a5e1331b6c2a788f"


def inside(path):
    path = path.resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError("Path leaves workspace")
    return path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def local_output(path):
    path = inside(path)
    if not path.is_relative_to(LAB / "work"):
        raise ValueError("Output leaves private working directory")
    return path


def gunzip_script(data):
    with gzip.GzipFile(fileobj=io.BytesIO(data)) as stream:
        result = stream.read(SCRIPT_LIMIT + 1)
    if len(result) > SCRIPT_LIMIT:
        raise ValueError("Script exceeds extraction limit")
    return result


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


def signature_hits(data, sections, signature):
    parts = signature.split()
    pattern = b"".join(b"." if p == "??" else re.escape(bytes.fromhex(p)) for p in parts)
    if not parts:
        raise ValueError("Empty signature")
    hits = []
    for section in sections:
        if not section["executable"]:
            continue
        base = section["raw_offset"]
        code = data[base:base + section["raw_size"]]
        for match in re.finditer(b"(?=(" + pattern + b"))", code, re.S):
            hits.append(dict(file_offset=base + match.start(), rva=section["rva"] + match.start()))
    return hits


def audit_signatures(data, sections):
    source = inside(ROOT / "journey_detour-main/journey_detour_v4")
    pattern = re.compile(r'SIGSCAN_(?:FUNC|HOOK)\s*\(\s*(\w+)\s*,\s*((?:"[^"\n]*"\s*)+)')
    results = []
    for path in sorted(source.rglob("*")):
        if path.suffix not in (".cpp", ".h") or "include" in path.parts:
            continue
        text = inside(path).read_text()
        # Remove comments without disturbing quoted patterns or line numbers.
        text = re.sub(r'("(?:\\.|[^"\\])*"|//[^\n]*|/\*[\s\S]*?\*/)',
                      lambda m: m[0] if m[0].startswith('"') else re.sub(r"[^\n]", " ", m[0]), text)
        for match in pattern.finditer(text):
            signature = "".join(re.findall(r'"([^"\n]*)"', match[2]))
            hits = signature_hits(data, sections, signature)
            results.append(dict(symbol=match[1], source=str(path.relative_to(ROOT)),
                                line=text[:match.start()].count("\n") + 1, hits=hits))
    return results


def write_local(path, data):
    path = local_output(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        if path.read_bytes() != data:
            raise ValueError("Existing output differs; choose a fresh working directory")
    else:
        with path.open("xb") as stream:
            stream.write(data)


def inspect():
    pc = inside(ROOT / "JourneyPC")
    executable = inside(pc / "Journey.exe").read_bytes()
    sections = pe_sections(executable)
    signatures = audit_signatures(executable, sections)
    report = dict(exe_sha256=digest(executable), architecture="PE32+ x86-64",
                  known_static_fingerprint=digest(executable) == EXPECTED_EXE,
                  runtime_compatibility="UNVERIFIED", sections=sections, signatures=signatures)
    report["libraries"] = {p.name: digest(inside(p).read_bytes()) for p in sorted(pc.glob("*.dll"))}
    report["working_exe_matches"] = executable == inside(LAB.parent / "Journey.exe").read_bytes()
    report["counts"] = {}
    for name in ("JourneyPC", "JourneyIOS"):
        report["counts"][name] = dict(collections.Counter(
            p.suffix.lower() for p in inside(ROOT / name).rglob("*") if p.is_file()))
    scripts = []
    for path in sorted((pc / "Data/Scripts").rglob("*.lua.gz")):
        relative = path.relative_to(pc / "Data/Scripts")
        decoded_name = relative.name.removeprefix("__").removesuffix(".gz")
        output_relative = relative.parent / decoded_name
        compressed = inside(path).read_bytes()
        decoded = gunzip_script(compressed)
        ios = inside(ROOT / "JourneyIOS/Data/Scripts" / output_relative)
        same = ios.exists() and decoded.replace(b"\r\n", b"\n") == ios.read_bytes().replace(b"\r\n", b"\n")
        entry = dict(source=str(path.relative_to(ROOT)), sha256=digest(compressed),
                     decoded_sha256=digest(decoded), ios_equal_normalized=same,
                     format="Lua bytecode" if decoded.startswith(b"\x1bLua") else "UTF-8 text")
        if entry["format"] == "UTF-8 text":
            decoded.decode("utf-8")
        copy = inside(LAB.parent / "Data/Scripts" / relative)
        entry["working_copy_matches"] = copy.exists() and copy.read_bytes() == compressed
        write_local(LAB / "work/pc_scripts" / output_relative, decoded)
        scripts.append(entry)
    report["scripts"] = scripts
    loose_scripts = []
    for path in sorted((pc / "Data/Scripts").rglob("*.lua")):
        relative = path.relative_to(pc / "Data/Scripts")
        decoded = inside(path).read_bytes()
        ios = inside(ROOT / "JourneyIOS/Data/Scripts" / relative)
        copy = inside(LAB.parent / "Data/Scripts" / relative)
        loose_scripts.append(dict(source=str(path.relative_to(ROOT)), sha256=digest(decoded),
                                  ios_equal_normalized=ios.exists() and decoded.replace(b"\r\n", b"\n") == ios.read_bytes().replace(b"\r\n", b"\n"),
                                  working_copy_matches=copy.exists() and copy.read_bytes() == decoded))
        write_local(LAB / "work/pc_scripts" / relative, decoded)
    report["loose_scripts"] = loose_scripts
    evidence = []
    allow = re.compile(r"ConnectionBarn::|ConnectionClient::|SynchVarBarn::|Lua 5\.3|^SteamNetworking|^GetRemoteDude$|^GetLocalDude$|^Player2?Chirp$")
    for match in re.finditer(rb"[ -~]{5,}", executable):
        value = match[0].decode("ascii")
        if allow.search(value):
            evidence.append(dict(file_offset=match.start(), text=value))
    report["binary_evidence"] = evidence
    sample = sorted((pc / "Data/Wads").glob("*.gz"))[0]
    with gzip.open(inside(sample), "rb") as stream:
        with tarfile.open(fileobj=stream, mode="r|") as archive:
            members = []
            for member in archive:
                members.append(dict(name=member.name, size=member.size))
                if len(members) == 3:
                    break
    report["wad_sample"] = dict(path=str(sample.relative_to(ROOT)), format="gzip containing tar", members=members)
    report["signature_check"] = "unique" if signatures and all(len(s["hits"]) == 1 for s in signatures) else "FAILED"
    local_output(LAB / "work/inventory.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(dict(scripts=len(scripts), ios_matches=sum(s["ios_equal_normalized"] for s in scripts),
                          signatures=len(signatures), signature_check=report["signature_check"],
                          known_static_fingerprint=report["known_static_fingerprint"],
                          runtime_compatibility=report["runtime_compatibility"]), indent=2))
    return report


if __name__ == "__main__":
    inspect()
