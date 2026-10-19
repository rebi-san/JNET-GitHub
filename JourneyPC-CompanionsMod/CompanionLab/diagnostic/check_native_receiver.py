"""Execute pinned numeric PC functions and the actor dispatcher in an isolated fixture.

Never opens Journey, installs anything, or includes game code in a release package.
macOS uses the existing x86-64 execution support; no runtime is installed.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from inspect_build import ROOT, EXPECTED_EXE, pe_sections
from native_map import offset_for

# All direct callees and RIP-relative constants used by these leaf paths.
CODE = ((0x118730, 0x118DE8), (0x117FD0, 0x11802A), (0xB1730, 0xB18F4), (0x118690, 0x118723))
DATA = ((0x5C1E60, 0x5C1F20), (0x68AE80, 0x68B940))


def main():
    data = (ROOT / "JourneyPC/Journey.exe").read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED_EXE:
        raise SystemExit("Pinned Journey executable mismatch")
    sections = pe_sections(data)
    work = HERE.parent / "work/native-receiver"
    work.mkdir(exist_ok=True)
    header = ["/* Generated from the hash-pinned workspace executable; fixture only. */"]
    for index, (begin, end) in enumerate(CODE + DATA):
        offset = offset_for(sections, begin, end - begin)
        chunk = data[offset:offset + end - begin]
        header.append(f"static const unsigned char piece{index}[]={{" +
                      ",".join(str(v) for v in chunk) + "};")
    header.append("static void load_pieces(unsigned char *image) {")
    for index, (begin, _) in enumerate(CODE + DATA):
        header.append(f"memcpy(image+0x{begin:x},piece{index},sizeof(piece{index}));")
    header.append("}")
    (work / "receiver_code.h").write_text("\n".join(header) + "\n")
    binary = work / "test_native_receiver_engine"
    subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", str(work), str(HERE / "test_native_receiver_engine.c"),
                    "-o", str(binary)], check=True)
    result = subprocess.run([str(binary)], text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise SystemExit(f"Native receiver fixture failed ({result.returncode}):\n{result.stdout}{result.stderr}")
    receipt = {"journey_sha256": EXPECTED_EXE, "game_runtime_tested": False,
               "native_numeric_code_executed": True, "native_actor_dispatcher_executed": True,
               "actor_update_intercepted": True, "output": result.stdout.strip(),
               "sources": {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                           for name in ("native_receiver.h", "test_native_receiver_engine.c", "check_native_receiver.py")}}
    (work / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(result.stdout, end="")


if __name__ == "__main__":
    main()
