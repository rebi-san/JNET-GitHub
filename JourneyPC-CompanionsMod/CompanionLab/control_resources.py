"""Extend the pinned PC menu while retaining its native mapping widgets."""
import copy
import struct
import hashlib
import xml.etree.ElementTree as ET
from inspect_build import ROOT


def menu_bytes():
    source=(ROOT/'JourneyPC/MenuSystem.xme').read_bytes()
    if hashlib.sha256(source).hexdigest()!="f060fb007a4928e954d65f8e3305709805b0229cb61f535aa3c0a52abe315909":
        raise ValueError("Unrecognized PC menu definition")
    key=0x6341f337
    plain=bytearray()
    for (value,) in struct.iter_unpack('<I',source):
        plain+=struct.pack('<I',value^key)
        key=(key+value)&0xffffffff
    root=ET.fromstring(plain.rstrip(b'\0'))
    menu=root.find('./Menu')
    menu.find('Start').set('Text','New Journey')
    at=list(menu).index(menu.find('Options'))
    for offset,label in enumerate(('Character','World')):
        menu.insert(at+offset,ET.Element('JNET'+label,Text=label,Action='%JNET_OPEN%'))
    for branch,before,name,label,options in [('Audio','Back','JNETCredits','Credits','Off;On'),('Graphics','Default','JNETFPS','FPS','Default;30;60')]:
        parent=menu.find('Options/'+branch)
        parent.insert(list(parent).index(parent.find(before)),ET.Element(name,Text=label,Options=options,Setting='%'+name+'%'))
    controls=root.find('./Menu/Options/Controls')
    keys=['None']+[f'Key {i:02X}' for i in range(1,256)]
    for i in range(65,91):keys[i]=chr(i)
    for i in range(48,58):keys[i]=chr(i)
    for i in range(112,136):keys[i]=f'F{i-111}'
    for i,name in {8:'Backspace',9:'Tab',13:'Enter',16:'Shift',17:'Ctrl',18:'Alt',19:'Break',20:'Caps Lock',27:'Escape',32:'Space',33:'Page Up',34:'Page Down',35:'End',36:'Home',37:'Left',38:'Up',39:'Right',40:'Down',45:'Insert',46:'Delete',160:'Left Shift',161:'Right Shift',162:'Left Ctrl',163:'Right Ctrl',164:'Left Alt',165:'Right Alt',186:'Semicolon',187:'Equals',188:'Comma',189:'Minus',190:'Period',191:'Slash',192:'Backtick',219:'Left Bracket',220:'Backslash',221:'Right Bracket',222:'Apostrophe'}.items():keys[i]=name
    options=[';'.join(keys),'None;Left;Right;Middle;X1;X2',
             'None;D-Pad Up;D-Pad Down;D-Pad Left;D-Pad Right;Start;Back;Left Stick;Right Stick;LB;RB;A;B;X;Y;LT;RT']
    for branch,device in [('Keyboard',0),('Mouse',1),('Pad/DualShock',2),('Pad/Input',2)]:
        parent=controls.find(branch)
        native={name:copy.deepcopy(parent.find(name) if parent.find(name) is not None else controls.find('Pad/'+name)) for name in ('Move','Look','Chime','Fly')}
        for name in native:
            if parent.find(name) is not None:parent.remove(parent.find(name))
        selector=ET.Element('JNETCharacter',Text='Character',Options='Wayfarer;Longtail;Whale;War Machine;fl0w',Setting='%JNET_CONTROL_CHARACTER%')
        parent.insert(0,selector)
        for index,kind in enumerate((0,1,3,2,5)):
            group=ET.SubElement(selector,'Character'+str(kind),Flags=f'Show.Never;Traverse.IfSelected.{index}')
            for name in ('Move','Look','Chime'):
                row=copy.deepcopy(native[name])
                if name=='Chime':row.set('Text','Chirp')
                group.append(row)
            if kind==0:
                group.append(copy.deepcopy(native['Fly']))
                actions=((0,'Crouch'),(4,'Frontflip'),(1,'Backflip'),(5,'SideflipL'),(6,'SideflipR'),(2,'Spin'))
            else:
                actions=[(0,'Fly Up'),(1,'Fly Down'),(2,'Level')]
                if kind in (2,3):actions.extend(((3,'TiltL'),(4,'TiltR')))
                if kind==2:actions.append((5,'Spotlight'))
                if kind==1:actions.append((5,'Grab'))
                actions.append((6,'Sprint'))
                if kind==5:actions.append((7,'Flower Trail'))
            for action,label in actions:
                setting=f'%JNET_BIND_{device}_{action}%' if kind==0 else f'%JNET_CREATURE_{kind}_{device}_{action}%'
                choices=options[device]
                if branch=='Pad/DualShock':choices=choices.replace('LB','L1').replace('RB','R1').replace(';A;B;X;Y;LT;RT',';Cross;Circle;Square;Triangle;L2;R2')
                ET.SubElement(group,'JNETAction'+str(action),Text=label,Options=choices,Setting=setting)
    for name in ('Move','Look'):controls.find('Pad').remove(controls.find('Pad/'+name))
    plain=ET.tostring(root,encoding='utf-8',xml_declaration=True)+b'\0'
    plain+=b'\0'*(-len(plain)%4)
    key=0x6341f337
    encrypted=bytearray()
    for (value,) in struct.iter_unpack('<I',plain):
        value^=key
        encrypted+=struct.pack('<I',value)
        key=(key+value)&0xffffffff
    return bytes(encrypted)
