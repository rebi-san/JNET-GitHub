"""Offline, hash-pinned native cross-reference map; never opens a game process."""
import bisect
import argparse
import collections
import json
from pathlib import Path
import re
import struct
import subprocess

from inspect_build import ROOT, LAB, EXPECTED_EXE, digest, inside, local_output, pe_sections


def offset_for(sections, rva, length=1):
    if length < 1 or rva < 0:
        raise ValueError('Invalid range')
    for section in sections:
        delta = rva - section['rva']
        if 0 <= delta and delta + length <= section['raw_size']:
            return section['raw_offset'] + delta
    raise ValueError('RVA is not file-backed')


def runtime_functions(data, sections):
    pe = struct.unpack_from('<I', data, 60)[0]
    directory = pe + 24 + 112 + 3 * 8
    rva, size = struct.unpack_from('<II', data, directory)
    if size % 12 or size > 4 * 1024 * 1024:
        raise ValueError('Invalid exception directory size')
    offset = offset_for(sections, rva, size)
    result = []
    for pos in range(offset, offset + size, 12):
        begin, end, unwind = struct.unpack_from('<III', data, pos)
        if begin >= end:
            raise ValueError('Invalid runtime function range')
        offset_for(sections, begin, end - begin)
        result.append((begin, end, unwind))
    if result != sorted(result) or any(a[1] > b[0] for a,b in zip(result, result[1:])):
        raise ValueError('Overlapping or unsorted runtime ranges')
    return result


def containing(functions, starts, rva):
    index = bisect.bisect_right(starts, rva) - 1
    return functions[index][0] if index >= 0 and rva < functions[index][1] else None


def unwind_roots(data, sections, functions):
    """Resolve PE chained unwind fragments; reject malformed or ambiguous chains."""
    entries = {entry[0]: tuple(entry) for entry in functions}
    roots = {}
    for begin in entries:
        current = begin
        seen = set()
        while current not in roots:
            if current in seen or len(seen) >= 32:
                raise ValueError('Cyclic or excessive unwind chain')
            seen.add(current)
            unwind = entries[current][2]
            offset = offset_for(sections, unwind, 4)
            version_flags, _, count, _ = data[offset:offset+4]
            version, flags = version_flags & 7, version_flags >> 3
            if version not in (1, 2) or flags & ~7 or (flags & 4 and flags & 3):
                raise ValueError('Unsupported unwind header')
            tail = unwind + 4 + ((count + 1) // 2) * 4
            offset_for(sections, unwind, tail - unwind)
            if not flags & 4:
                roots[current] = current
                break
            offset = offset_for(sections, tail, 12)
            parent = struct.unpack_from('<III', data, offset)
            if entries.get(parent[0]) != parent:
                raise ValueError('Unwind chain target is not a runtime entry')
            current = parent[0]
        root = roots[current]
        for fragment in seen:
            roots[fragment] = root
    return roots


def parse_instruction(line, base):
    match = re.match(r'^([0-9a-fA-F]+):\s+([a-zA-Z][a-zA-Z0-9]*)\s*(.*)$', line)
    if not match:
        return None
    address, mnemonic, operand = match.groups()
    address = int(address,16) - base
    direct = re.match(r'0x([0-9a-fA-F]+)(?:\s|$)', operand) if mnemonic in ('call', 'jmp') else None
    rip = re.search(r'#\s+0x([0-9a-fA-F]+)', operand) if '[rip' in operand else None
    return dict(rva=address, mnemonic=mnemonic, operand=operand,
                direct=int(direct[1],16)-base if direct else None,
                rip=int(rip[1],16)-base if rip else None)


def query_cached(report, data, rva, limit=12):
    """Read a bounded owning-function view; never disassemble or modify the cache."""
    if type(rva) is not int or not 0 <= rva <= 0xffffffff:
        raise ValueError('RVA must be an unsigned 32-bit integer')
    if type(limit) is not int or not 1 <= limit <= 100:
        raise ValueError('Limit must be between 1 and 100 per edge direction')
    if not isinstance(report, dict) or report.get('exe_sha256') != EXPECTED_EXE or digest(data) != EXPECTED_EXE:
        raise ValueError('Stale native map or unsupported executable; rebuild the map from the pinned executable')
    sections = pe_sections(data)
    pe = struct.unpack_from('<I', data, 60)[0]
    base = struct.unpack_from('<Q', data, pe+24+24)[0]
    image_size = struct.unpack_from('<I', data, pe+24+56)[0]
    if type(report.get('image_base')) is not int or report['image_base'] != base:
        raise ValueError('Native map image base does not match the executable')
    if rva >= image_size:
        raise ValueError('RVA is outside the executable image')
    functions = runtime_functions(data, sections)
    cached_functions = report.get('runtime_functions')
    if (not isinstance(cached_functions, list) or
            any(not isinstance(row, list) or len(row) != 3 or any(type(v) is not int for v in row)
                for row in cached_functions) or cached_functions != [list(row) for row in functions]):
        raise ValueError('Native map runtime ranges do not match the executable')
    roots = unwind_roots(data, sections, functions)
    cached_roots = report.get('unwind_roots')
    if (not isinstance(cached_roots, dict) or any(type(v) is not int for v in cached_roots.values()) or
            cached_roots != {str(k):v for k,v in roots.items()}):
        raise ValueError('Native map chained unwind roots do not match the executable')
    if not isinstance(report.get('incoming'), dict) or not isinstance(report.get('outgoing'), dict):
        raise ValueError('Native map edge indexes are missing or malformed')
    starts = [row[0] for row in functions]
    fragment = containing(functions, starts, rva)
    owner = roots.get(fragment)
    target = owner if owner is not None else rva

    def edges(direction):
        rows = report[direction].get(str(target), []) if direction == 'incoming' or owner is not None else []
        if not isinstance(rows, list):
            raise ValueError('Malformed native map edge list')
        for edge in rows:
            if (not isinstance(edge, dict) or edge.get('kind') not in ('call', 'jmp') or
                    any(type(edge.get(k)) is not int or not 0 <= edge[k] < image_size for k in ('site', 'target')) or
                    any(k not in edge or (edge[k] is not None and type(edge[k]) is not int) for k in ('owner', 'fragment'))):
                raise ValueError('Malformed native map edge')
            edge_fragment = containing(functions, starts, edge['site'])
            if (edge['fragment'] != edge_fragment or edge['owner'] != roots.get(edge_fragment) or
                    (direction == 'incoming' and edge['target'] != target) or
                    (direction == 'outgoing' and edge['owner'] != owner)):
                raise ValueError('Native map edge ownership or index does not match its range')
        selected = sorted(rows, key=lambda edge: (edge['site'], edge['target'], edge['kind']))[:limit]
        return dict(total=len(rows), truncated=len(rows)>limit, edges=[
            {k:hex(edge[k]) if edge[k] is not None else None for k in ('site', 'target', 'owner', 'fragment')} |
            {'kind':edge['kind']} for edge in selected])

    current = functions[bisect.bisect_left(starts, fragment)] if fragment is not None else None
    return dict(rva=hex(rva), image_base=hex(base), owner=hex(owner) if owner is not None else None,
                fragment=dict(begin=hex(current[0]), end=hex(current[1]), unwind=hex(current[2])) if current else None,
                incoming_to=hex(target), outgoing_from=hex(owner) if owner is not None else None,
                limit=limit, incoming=edges('incoming'), outgoing=edges('outgoing'),
                limits='Static direct edges only. Queries inside a chained fragment use its owning function. '
                       'Without an unwind range, only exact-address incoming edges are shown; no leaf owner is inferred.')


def query(rva, limit=12):
    cache = inside(LAB/'work/native/map.json')
    if not cache.is_file():
        raise ValueError('Native map cache is missing; run native_map.py without --at to build it')
    if cache.stat().st_size > 64*1024*1024:
        raise ValueError('Native map cache exceeds the 64 MiB inspection limit')
    report = json.loads(cache.read_text())
    data = inside(ROOT/'JourneyPC/Journey.exe').read_bytes()
    return query_cached(report, data, rva, limit)


def build():
    exe = inside(ROOT/'JourneyPC/Journey.exe')
    data = exe.read_bytes()
    if digest(data) != EXPECTED_EXE:
        raise ValueError('Unsupported executable; no analysis output generated')
    sections = pe_sections(data)
    pe = struct.unpack_from('<I',data,60)[0]
    base = struct.unpack_from('<Q',data,pe+24+24)[0]
    functions = runtime_functions(data, sections)
    roots = unwind_roots(data, sections, functions)
    starts = [x[0] for x in functions]
    work = local_output(LAB/'work/native')
    work.mkdir(exist_ok=True)
    assembly = local_output(work/'text.asm')
    # Regenerate the stream from the verified image, never trust a stale cache.
    with assembly.open('w') as output:
        subprocess.run(['objdump','-d','--no-show-raw-insn','--x86-asm-syntax=intel',str(exe)], stdout=output, check=True)
    outgoing = collections.defaultdict(list)
    incoming = collections.defaultdict(list)
    strings = collections.defaultdict(list)
    fields = collections.defaultdict(list)
    interesting = ('ConnectionBarn', 'ConnectionClient', 'ConnClient', 'PlayerBarn', 'SynchVarBarn', 'Dude::')
    for line in assembly.open():
        instruction = parse_instruction(line, base)
        if instruction is None:
            continue
        rva = instruction['rva']
        fragment = containing(functions, starts, rva)
        owner = roots.get(fragment)
        target = instruction['direct']
        if target is not None:
            edge = dict(site=rva, owner=owner, fragment=fragment, target=target, kind=instruction['mnemonic'])
            outgoing[owner].append(edge)
            incoming[target].append(edge)
        target = instruction['rip']
        if target is not None:
            try:
                pos = offset_for(sections,target)
                end = data.find(b'\0',pos,pos+384)
                value = data[pos:end].decode('ascii') if end >= 0 else ''
                if value and all(32 <= ord(c) < 127 or c in "\r\n\t" for c in value) and any(x in value for x in interesting):
                    strings[value].append(dict(site=rva,owner=owner,string_rva=target))
            except (ValueError,UnicodeDecodeError):
                pass
        for displacement in ('0x146f0','0x117b4','0x117b5','0x1670c'):
            if re.search(r'\[.*\+ '+displacement+r'\]',instruction['operand']):
                fields[displacement].append(dict(site=rva,owner=owner,instruction=instruction['mnemonic']+' '+instruction['operand']))
    report = dict(exe_sha256=EXPECTED_EXE, image_base=base,
                  limits='Static decoded direct edges only; owner follows validated chained unwind records, not inferred proximity. Leaf code may lack an owner; indirect calls, inlining and data flow are not resolved. Matching displacements do not establish object type.',
                  runtime_functions=functions, unwind_roots=roots, strings=dict(strings), field_references=dict(fields),
                  incoming={str(k):v for k,v in incoming.items()}, outgoing={str(k):v for k,v in outgoing.items()})
    (work/'map.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(runtime_ranges=len(functions),interesting_strings=len(strings),direct_edges=sum(map(len,incoming.values())),output=str(work.relative_to(ROOT)))))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--at', type=lambda value: int(value, 0), metavar='RVA',
                        help='query the existing map at a decimal or 0x-prefixed RVA without rebuilding')
    parser.add_argument('--limit', type=int, metavar='N', help='maximum incoming/outgoing edges, each 1..100 (default 12)')
    args = parser.parse_args(argv)
    if args.at is None and args.limit is not None:
        parser.error('--limit requires --at; no map was rebuilt')
    try:
        if args.at is None:
            build()
        else:
            print(json.dumps(query(args.at, 12 if args.limit is None else args.limit), indent=2))
    except (OSError, ValueError, struct.error) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
