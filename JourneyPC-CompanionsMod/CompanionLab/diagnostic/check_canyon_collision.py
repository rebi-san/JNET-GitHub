from pathlib import Path
import hashlib
import subprocess
import sys

HERE = Path(__file__).resolve().parent
LAB = HERE.parent
sys.path.insert(0,str(LAB))
from inspect_build import EXPECTED_EXE, pe_sections
from native_map import offset_for


def main():
    image = (LAB.parent.parent/'JourneyPC/Journey.exe').read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_EXE
    sections = pe_sections(image)
    work = LAB/'work/canyon-collision-check'
    work.mkdir(exist_ok=True)
    parts = [(0xe82c0,0xe8990),(0x5c1e00,0x5c2000),(0x68ae00,0x68c100)]
    header = ''
    for i,(start,end) in enumerate(parts):
        data = image[offset_for(sections,start):offset_for(sections,end)]
        header += f'static const unsigned char canyon_native_{i}[]={{'+','.join(map(str,data))+'};\n'
    header += 'static const struct {unsigned rva,size;const unsigned char *data;} canyon_native_parts[]={'+','.join(
        f'{{{start},{end-start},canyon_native_{i}}}' for i,(start,end) in enumerate(parts))+'};\n'
    (work/'canyon_native_fixture.h').write_text(header)
    binary=work/'test_jnet_canyon_collision'
    subprocess.run(['clang','-arch','x86_64','-std=c11','-O2','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I',str(HERE),'-I',str(work),
                    str(HERE/'test_jnet_canyon_collision.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(LAB/'work/canyon-collision.bin')],check=True,timeout=60)


if __name__ == '__main__':
    main()
