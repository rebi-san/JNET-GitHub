from pathlib import Path
import hashlib
import json
import struct
import UnityPy
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'JourneyPC-CompanionsMod/CompanionLab/work/menu-images'
SOURCE = ROOT / 'Journey-Save-Editor-Windows-main/Journey Save Editor_Data'

def main():
    OUT.mkdir(exist_ok=True)
    env = UnityPy.load(str(SOURCE / 'sharedassets0.assets'))
    objects = {o.path_id: o for o in env.objects}
    atlas = Image.new('RGBA', (1024, 1024))
    entries = []
    def add(name, im, x, y, size, source):
        im = im.convert('RGBA'); im.thumbnail(size, Image.Resampling.LANCZOS)
        atlas.paste(im, (x,y))
        entries.append(dict(name=name, rect=[x,y,*im.size], source=source))
    level_ids = [30,47,35,46,49,28,43,45,52,37,29,36]
    names = ['Chapter Select','Broken Bridge','Pink Desert','Sunken City','Underground','Tower','Snow','Paradise','Credits','Level Bryan','Level Matt','Level Chris']
    for i, (path_id, name) in enumerate(zip(level_ids,names)):
        add(name, objects[path_id].read().image, (i%4)*256, (i//4)*128, (256,128), f'sharedassets0.assets:{path_id}')
    for i, path_id in enumerate([38,33,39,34,44,51,41]):
        im=objects[path_id].read().image.crop((0,0,512,256))
        add(('Red ' + str(i)) if i<4 else ('White '+str(i-3)), im, (i%4)*256,384+(i//4)*128,(256,128),f'sharedassets0.assets:{path_id}:cloth half')
    symbols=objects[53].read().image.convert('RGBA')
    for i in range(21):
        x,y=(i%8)*32,(3-i//8)*32
        im=symbols.crop((x,y,x+32,y+32));alpha=im.getchannel('A')
        im=Image.new('RGBA',im.size,'white');im.putalpha(alpha)
        add('Symbol '+str(i), im, i*32,640,(32,32),'sharedassets0.assets:53:bottom-first 8-column atlas')
    add('Red scarf',objects[40].read().image,0,688,(1024,49),'sharedassets0.assets:40')
    # Use the native white scarf strip already consumed by the helper build.
    source=ROOT/'JourneyPC/Data/Textures/Bin/ClothPlayerScarfFilledClothPlayerScarfFilled.dds'
    data=source.read_bytes()
    if data[:4]!=b'DDS ' or struct.unpack_from('<II',data,12)!=(2048,512):raise ValueError('Scarf source changed')
    pixels=bytearray()
    for y in range(80):
        for x in range(2048):
            at=128+(x*512+424+79-y)*4
            b,g,r,a=data[at:at+4];pixels.extend((r,g,b,a))
    add('White scarf',Image.frombytes('RGBA',(2048,80),bytes(pixels)),0,752,(1024,40),'JourneyPC/Data/Textures/Bin/ClothPlayerScarfFilledClothPlayerScarfFilled.dds:white strip')
    atlas.save(OUT/'atlas.png')
    raw=atlas.tobytes();encoded=bytearray();i=0
    while i<len(raw):
        pixel=raw[i:i+4];n=1
        while n<65535 and raw[i+n*4:i+(n+1)*4]==pixel:n+=1
        encoded.extend(struct.pack('<H',n)+pixel);i+=n*4
    header=['#ifndef JNET_MENU_ASSETS_H','#define JNET_MENU_ASSETS_H','struct jnet_asset_rect { unsigned x,y,w,h; };','static const struct jnet_asset_rect jnet_assets[]={']
    header += ['{'+','.join(map(str,e['rect']))+'},' for e in entries]
    header += ['};','static const char *const jnet_level_names[]={'+','.join(json.dumps(n) for n in names)+'};','static const unsigned char jnet_atlas_rle[]={']
    header += [','.join(map(str,encoded[i:i+128]))+',' for i in range(0,len(encoded),128)]
    header += ['};','#endif']
    (OUT/'jnet_menu_assets.h').write_text('\n'.join(header)+'\n')
    (OUT/'manifest.json').write_text(json.dumps(dict(entries=entries,sha256=hashlib.sha256(raw).hexdigest(),source_sha256=hashlib.sha256((SOURCE/'sharedassets0.assets').read_bytes()).hexdigest()),indent=2)+'\n')
    print(json.dumps(dict(images=len(entries),atlas=[1024,1024],encoded_bytes=len(encoded))))

if __name__=='__main__':main()
