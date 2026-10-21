import base64
import hashlib
import json
from pathlib import Path
import sys

WORK = Path(__file__).resolve().parent
RECORD = WORK / 'temporary-network-settings.json'
PAYLOAD = b'[Diagnostics]\r\nScans=0\r\nObservationLogs=0\r\n[Transport]\r\nWorker=0\r\n[Motion]\r\nSmoothMs=0\r\n'
NAMES = ('devishaz', 'rebishaz', 'smooviepuffs')

def checked(path):
    if path.is_symlink() or path.resolve() != path:
        raise RuntimeError('Redirected path: ' + str(path))
    return path

def main():
    mode = sys.argv[1] if len(sys.argv) == 2 else ''
    if mode not in ('test-b', 'test-c', 'baseline-b', 'restore'):
        raise RuntimeError('Specify test-b, test-c or restore')
    if mode in ('test-c', 'baseline-b'):
        saved_record = RECORD.read_bytes()
        record = json.loads(saved_record)
        if record['status'] != 'active' or record['phase'] != ('B' if mode == 'test-c' else 'C'):
            raise RuntimeError('Expected active Test B settings')
        before = base64.b64decode(record['temporary'])
        old = b'Worker=0' if mode == 'test-c' else b'Worker=1'
        new = b'Worker=1' if mode == 'test-c' else b'Worker=0'
        if before.count(old) != 1:
            raise RuntimeError('Unexpected worker setting')
        after = before.replace(old, new)
        for row in record['targets']:
            path = checked(Path(row['path']))
            if path.read_bytes() != before:
                raise RuntimeError('Settings changed: ' + row['name'])
            receipt = json.loads((WORK / ('install-' + row['name'] + '.json')).read_text())
            if receipt['version'] != '0.178' or str(path.parent) != receipt['directory']:
                raise RuntimeError('Installation identity mismatch')
            if hashlib.sha256(checked(path.parent / 'JNET.dll').read_bytes()).hexdigest() != receipt['files']['JNET.dll']:
                raise RuntimeError('Installed DLL changed')
        changed = []
        try:
            for row in record['targets']:
                path = checked(Path(row['path']))
                changed.append(path)
                path.write_bytes(after)
                if path.read_bytes() != after:
                    raise RuntimeError('Test C verification failed')
            record['phase'] = 'C' if mode == 'test-c' else 'B'
            record['temporary'] = base64.b64encode(after).decode()
            RECORD.write_text(json.dumps(record, indent=2) + '\n')
        except Exception:
            for path in reversed(changed): path.write_bytes(before)
            RECORD.write_bytes(saved_record)
            raise
        print('Settings verified in all three bottles: phase ' + record['phase'] + '. Original restore data retained. No applications launched.')
        return
    if mode == 'restore':
        record = json.loads(RECORD.read_text())
        if record['status'] != 'active':
            raise RuntimeError('No active temporary settings')
        for row in record['targets']:
            path = checked(Path(row['path']))
            if path.read_bytes() != base64.b64decode(record['temporary']):
                raise RuntimeError('Settings changed; refusing to overwrite: ' + row['name'])
        for row in record['targets']:
            path = checked(Path(row['path']))
            if row['original'] is None:
                path.unlink()
                assert not path.exists()
            else:
                original = base64.b64decode(row['original'])
                path.write_bytes(original)
                assert path.read_bytes() == original
        record['status'] = 'restored'
        RECORD.write_text(json.dumps(record, indent=2) + '\n')
        print('Restored original settings in all three bottles.')
        return
    if RECORD.exists():
        raise RuntimeError('Restore record already exists; inspect it before changing settings')
    rows = []
    for name in NAMES:
        receipt = json.loads((WORK / ('install-' + name + '.json')).read_text())
        if receipt['version'] != '0.178':
            raise RuntimeError('Unexpected installed version: ' + name)
        folder = checked(Path(receipt['directory']))
        dll = checked(folder / 'JNET.dll')
        if hashlib.sha256(dll.read_bytes()).hexdigest() != receipt['files']['JNET.dll']:
            raise RuntimeError('Installed DLL changed: ' + name)
        path = checked(folder / 'JNET-network.ini')
        original = path.read_bytes() if path.exists() else None
        rows.append(dict(name=name, path=str(path), original=None if original is None else base64.b64encode(original).decode()))
    record = dict(status='active', phase='B', temporary=base64.b64encode(PAYLOAD).decode(), targets=rows)
    RECORD.write_text(json.dumps(record, indent=2) + '\n')
    changed = []
    try:
        for row in rows:
            path = checked(Path(row['path']))
            changed.append(row)
            path.write_bytes(PAYLOAD)
            if path.read_bytes() != PAYLOAD:
                raise RuntimeError('Settings verification failed: ' + row['name'])
    except Exception:
        for row in reversed(changed):
            path = Path(row['path'])
            if row['original'] is None:
                if path.exists(): path.unlink()
            else:
                path.write_bytes(base64.b64decode(row['original']))
        record['status'] = 'rolled-back'
        RECORD.write_text(json.dumps(record, indent=2) + '\n')
        raise
    for row in rows:
        print(row['name'] + ': Test B verified; ' + ('temporary file created' if row['original'] is None else 'original configuration backed up'))
    print('Settings apply on next Journey launch. No game or helper launched.')

if __name__ == '__main__':
    main()
