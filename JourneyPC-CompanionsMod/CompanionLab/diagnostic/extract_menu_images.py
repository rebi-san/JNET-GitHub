from pathlib import Path
import hashlib
import json
import struct
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'JourneyPC-CompanionsMod/CompanionLab/work/menu-images'

def main():
    OUT.mkdir(exist_ok=True)
    previous=json.loads((OUT/'manifest.json').read_text())
    with Image.open(OUT/'atlas.png') as source_atlas:
        preserved=[(entry,source_atlas.crop((entry['rect'][0],entry['rect'][1],entry['rect'][0]+entry['rect'][2],entry['rect'][1]+entry['rect'][3]))) for entry in previous['entries'][12:42]]
    atlas = Image.new('RGBA', (1024, 2048))
    entries = []
    def add(name, im, x, y, size, source):
        im = im.convert('RGBA'); im.thumbnail(size, Image.Resampling.LANCZOS)
        atlas.paste(im, (x,y))
        entries.append(dict(name=name, rect=[x,y,*im.size], source=source))
    pictures=ROOT/'WorldLevelPictures'
    sources={int(p.name.split(' ',1)[0]):p for p in pictures.glob('*.png')}
    def preview(number):
        with Image.open(sources[number]) as im:
            if number in (15,16):
                a,b,tx,ty=(1.0092900666,-.0107121245,19.17633671,33.60859594) if number==16 else (1,0,0,0)
                scale=im.width/2048
                transform=(a*1.875*scale,-b*1.875*scale,(a*64-b*94+tx)*scale,
                           b*1.875*scale,a*1.875*scale,(b*64+a*94+ty)*scale)
                return im.convert('RGB').transform((1024,512),Image.Transform.AFFINE,transform,Image.Resampling.BICUBIC).resize((256,128),Image.Resampling.LANCZOS)
            return ImageOps.fit(im.convert('RGB'),(256,128),method=Image.Resampling.LANCZOS)
    names = ['Chapter Select','Broken Bridge','Pink Desert','Sunken City','Underground','Tower','Snow','Paradise','Credits','Level 09','Level Matt','Level Chris']
    days=[1,3,5,8,11,13,15,17,20,23,21,22]
    variants=[[i]*4 for i in range(12)]
    for i,(number,name) in enumerate(zip(days,names)):
        add(name,preview(number),(i%4)*256,(i//4)*128,(256,128),str(sources[number].relative_to(ROOT)))
    for entry,im in preserved:
        x,y,w,h=entry['rect']
        add(entry['name'],im,x,y,(w,h),entry['source'])
    mask=Image.new('L',(256,128))
    mask.putdata([round(255*max(0,min(1,(x-115.5)/24))) for y in range(128) for x in range(256)])
    def variant(level,mode,im,source):
        index=len(entries);slot=index-42
        add(names[level]+' '+['Day','Night','Cycle','Alternate'][mode],im,(slot%4)*256,1024+(slot//4)*128,(256,128),source)
        variants[level][mode]=index
    for level,number in enumerate(days[:8]):
        day,night=preview(number),preview(number+1)
        variant(level,1,night,str(sources[number+1].relative_to(ROOT)))
        variant(level,2,Image.composite(night,day,mask),'centered 24px blend: '+sources[number].name+' + '+sources[number+1].name)
    for level,number in [(2,7),(3,10),(7,19)]:
        variant(level,3,preview(number),str(sources[number].relative_to(ROOT)))
    atlas.save(OUT/'atlas.png')
    raw=atlas.tobytes();encoded=bytearray();i=0
    while i<len(raw):
        pixel=raw[i:i+4];n=1
        while n<65535 and raw[i+n*4:i+(n+1)*4]==pixel:n+=1
        encoded.extend(struct.pack('<H',n)+pixel);i+=n*4
    header=['#ifndef JNET_MENU_ASSETS_H','#define JNET_MENU_ASSETS_H','struct jnet_asset_rect { unsigned x,y,w,h; };','static const struct jnet_asset_rect jnet_assets[]={']
    header += ['{'+','.join(map(str,e['rect']))+'},' for e in entries]
    header += ['};','static const unsigned jnet_level_assets[12][4]={'+','.join('{'+','.join(map(str,row))+'}' for row in variants)+'};','static const char *const jnet_level_names[]={'+','.join(json.dumps(n) for n in names)+'};','static const unsigned char jnet_atlas_rle[]={']
    header += [','.join(map(str,encoded[i:i+128]))+',' for i in range(0,len(encoded),128)]
    header += ['};','#endif']
    (OUT/'jnet_menu_assets.h').write_text('\n'.join(header)+'\n')
    (OUT/'manifest.json').write_text(json.dumps(dict(entries=entries,sha256=hashlib.sha256(raw).hexdigest(),source_sha256=previous['source_sha256']),indent=2)+'\n')
    print(json.dumps(dict(images=len(entries),atlas=[1024,2048],encoded_bytes=len(encoded))))

if __name__=='__main__':main()
