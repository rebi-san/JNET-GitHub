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
    work = LAB/'work/chirp-audio-check'
    work.mkdir(exist_ok=True)
    lines = ['static void load_chirp_audio_code(unsigned char *image){']
    for rva, length in ((0x13e760, 0x11e), (0x5e4020, 0xa0)):
        at = offset_for(sections, rva, length)
        lines.append('{static const unsigned char code[]={' + ','.join(map(str, image[at:at+length])) + '};')
        lines.append(f'memcpy(image+{rva},code,sizeof(code));}}')
    lines.append('}')
    (work/'chirp_audio_code.h').write_text('\n'.join(lines)+'\n')
    binary = work/'test_native_chirp_audio'
    subprocess.run(['clang', '-arch', 'x86_64', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(work), '-I'+str(HERE),
                    str(HERE/'test_native_chirp_audio.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == '__main__':
    main()
