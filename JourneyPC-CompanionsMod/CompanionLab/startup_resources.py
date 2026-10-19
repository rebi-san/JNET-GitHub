"""Prepare a private, in-memory startup-resource candidate; never patch a file."""
import hashlib
import json
import re
import struct

from inspect_build import ROOT, EXPECTED_EXE, inside, pe_sections
from native_map import offset_for


SCRIPT_NAME = b'Data/Resources/Persistent.lua'
SCRIPT_RVA = 0x372eed0
SCRIPT_SIZE = 73572
SCRIPT_SHA256 = '586e140fd5de2a0af36b09c84138e1510fb3fcd846e9659c2d2833691f7d7b37'
DESCRIPTOR_RVA = 0x376baa0
IMAGE_BASE = 0x140000000
TARGETS = (b'MeshDudeRemote', b'MeshDudeRemoteFrozen')
CLOTH_TARGETS = (b'ClothRemote', b'ClothRemoteScarf')


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def extract_persistent(image):
    if sha256(image) != EXPECTED_EXE:
        raise ValueError('Unsupported executable hash')
    sections = pe_sections(image)
    descriptor = offset_for(sections, DESCRIPTOR_RVA, 24)
    name, source, size = struct.unpack_from('<QQQ', image, descriptor)
    if source != IMAGE_BASE + SCRIPT_RVA or size != SCRIPT_SIZE:
        raise ValueError('Unexpected embedded script descriptor')
    name_offset = offset_for(sections, name - IMAGE_BASE, len(SCRIPT_NAME) + 1)
    if image[name_offset:name_offset + len(SCRIPT_NAME) + 1] != SCRIPT_NAME + b'\0':
        raise ValueError('Unexpected embedded script name')
    start = offset_for(sections, SCRIPT_RVA, SCRIPT_SIZE + 1)
    if image[start + SCRIPT_SIZE] != 0:
        raise ValueError('Missing embedded script terminator')
    source = image[start:start + SCRIPT_SIZE]
    if sha256(source) != SCRIPT_SHA256:
        raise ValueError('Unexpected embedded script hash')
    return source


def prepare_candidate(source, *, active_traveler=False):
    if len(source) != SCRIPT_SIZE or sha256(source) != SCRIPT_SHA256:
        raise ValueError('Unsupported startup resource buffer')
    candidate = bytearray(source)
    changes = []
    targets = [(b'Shader', name, b'2', b'4') for name in TARGETS]
    if active_traveler:
        # Stock material plus four independently leased frame materials.
        targets.extend((b'Shader', name, b'1', b'5') for name in CLOTH_TARGETS)
    targets.append((b'Mesh', b'BlendNinja', b'2', b'4'))
    for kind, name, before, after in targets:
        pattern = rb'(?m)^resource "' + kind + rb'" "' + re.escape(name) + rb'" \{[^\r\n{}]*\}'
        declarations = list(re.finditer(pattern, source))
        if len(declarations) != 1:
            raise ValueError('Ambiguous resource declaration')
        declaration = declarations[0]
        limits = list(re.finditer(rb'\binstanceLimit\s*=\s*(\d+)\b', declaration[0]))
        if len(limits) != 1 or limits[0][1] != before:
            raise ValueError('Unexpected resource capacity')
        offset = declaration.start() + limits[0].start(1)
        candidate[offset] = after[0]
        changes.append(dict(resource=name.decode('ascii'), buffer_offset=offset,
                            rva=SCRIPT_RVA + offset, before=int(before), after=int(after)))
    changes.sort(key=lambda change: change['buffer_offset'])
    if [i for i, (a, b) in enumerate(zip(source, candidate)) if a != b] != [c['buffer_offset'] for c in changes]:
        raise ValueError('Unexpected changes outside capacity fields')
    return bytes(candidate), changes


def main():
    source = extract_persistent(inside(ROOT / 'JourneyPC/Journey.exe').read_bytes())
    candidate, changes = prepare_candidate(source)
    print(json.dumps(dict(status='offline candidate only; no runtime integration',
                          executable_sha256=EXPECTED_EXE, script=SCRIPT_NAME.decode(),
                          source_sha256=sha256(source), candidate_sha256=sha256(candidate),
                          size=len(candidate), changes=changes), indent=2))


if __name__ == '__main__':
    main()
