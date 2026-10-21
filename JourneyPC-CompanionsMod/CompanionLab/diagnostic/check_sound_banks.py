import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
LAB = HERE.parent
sys.path.insert(0, str(LAB))
from inspect_build import pe_sections, EXPECTED_EXE, digest
from native_map import offset_for


def main():
    image = (LAB.parent.parent/'JourneyPC/Journey.exe').read_bytes()
    if digest(image) != EXPECTED_EXE:
        raise ValueError('Unrecognized executable')
    sections = pe_sections(image)
    work = LAB/'work/sound-bank-check'
    work.mkdir(exist_ok=True)
    lines = ['static void load_sound_bank_code(unsigned char *image){']
    for rva, length in ((0x2b6ff1, 0x73), (0x2b77ce, 0x75), (0x2b7c58, 0x61),
                        (0x8ed90, 0xa6), (0x8fb20, 8), (0x37c32e0, 5*0x30)):
        at = offset_for(sections, rva, length)
        lines.append('{static const unsigned char code[]={' + ','.join(map(str, image[at:at+length])) + '};')
        lines.append(f'memcpy(image+{rva},code,sizeof(code));}}')
    lines.append('for(unsigned i=0;i<5;++i){uintptr_t owner;memcpy(&owner,image+0x37c32e0+i*0x30+0x28,8);owner=owner-UINT64_C(0x140000000)+(uintptr_t)image;memcpy(image+0x37c32e0+i*0x30+0x28,&owner,8);}')
    lines.append('}')
    (work/'sound_bank_code.h').write_text('\n'.join(lines)+'\n')
    for name in ('test_jnet_sound_banks', 'test_jnet_sound_banks_prepare'):
        binary = work/name
        subprocess.run(['clang', '-arch', 'x86_64', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-I'+str(work), '-I'+str(HERE),
                        str(HERE/(name+'.c')), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=30)



if __name__ == '__main__':
    main()
