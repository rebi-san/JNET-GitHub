import re
import ast
from lupa import LuaRuntime
import struct
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from control_resources import menu_bytes
from inspect_build import ROOT, LAB, pe_sections
from native_map import offset_for


def decode(data):
    key=0x6341f337
    plain=bytearray()
    for (value,) in struct.iter_unpack('<I',data):
        plain+=struct.pack('<I',value^key)
        key=(key+value)&0xffffffff
    text=plain.rstrip(b'\0').decode('utf-8')
    text=re.sub(r'&#0*(1[6-9]|2[0-3]);',lambda m:chr(0xe000+int(m[1])),text)
    root=ET.fromstring(text)
    for node in root.iter():
        for key,value in node.attrib.items():
            node.set(key,''.join(chr(ord(c)-0xe000) if 0xe010<=ord(c)<=0xe017 else c for c in value))
    return root


class ControlsTests(unittest.TestCase):
    def test_native_widgets_and_unrelated_rows(self):
        original=decode((ROOT/'JourneyPC/MenuSystem.xme').read_bytes())
        edited=decode(menu_bytes())
        self.assertEqual(edited.find('./Menu/Options/Graphics/JNETFPS').get('Options'),'Default;30;55')
        controls=edited.find('./Menu/Options/Controls')
        for branch,device,count in [('Keyboard',0,256),('Mouse',1,6),('Pad/DualShock',2,17),('Pad/Input',2,17)]:
            parent=controls.find(branch)
            selector=parent.find('JNETCharacter')
            self.assertEqual(selector.get('Options'),'Wayfarer;Longtail;Whale;War Machine;fl0w')
            self.assertEqual(len(selector),5)
            self.assertIsNone(selector.find('Character6'))
            expected=[['Move','Look','Chirp','Fly','Crouch','Frontflip','Backflip','Spin','SideflipL','SideflipR'],
                      ['Move','Look','Chirp','Fly Up','Fly Down','Level','Dash','Grab'],
                      ['Move','Look','Chirp','Fly Up','Fly Down','Level','Dash'],
                      ['Move','Look','Chirp','Fly Up','Fly Down','Level','Dash','Spotlight','Attack']]
            for index,group in enumerate(selector):
                labels=[row.get('Text') for row in group]
                labels[:2]=['Move','Look']
                if index==0:labels[3]='Fly'
                self.assertEqual(labels,expected[index] if index<4 else ['Move','Look','Chirp','Fly Up','Fly Down','Level','Dash'])
                self.assertEqual(group.get('Flags'),f'Show.Never;Traverse.IfSelected.{index}')
                for row in group:
                    if row.tag.startswith('JNETAction'):
                        choices=row.get('Options').split(';')
                        self.assertEqual(len(choices),count)
                        if device==2:
                            codes=(18,17,19,16) if branch=='Pad/DualShock' else (22,23,20,21)
                            self.assertEqual(choices[11:15],[chr(code) for code in codes])
                            font=ET.parse(ROOT/'JourneyPC/font2.fnt')
                            for code in codes:self.assertIsNotNone(font.find(f'.//char[@id="{code}"]'))
                for name in ('Move','Look','Chime'):
                    native=original.find('./Menu/Options/Controls/'+branch+'/'+name)
                    if native is None:native=original.find('./Menu/Options/Controls/Pad/'+name)
                    self.assertEqual(group.find(name).get('Options'),native.get('Options'))
                    self.assertEqual(group.find(name).get('Setting'),native.get('Setting'))
            parent.remove(selector)
            original_parent=original.find('./Menu/Options/Controls/'+branch)
            for index,row in enumerate(original_parent):
                if row.tag in ('Move','Look','Chime','Fly'):parent.insert(index,ET.fromstring(ET.tostring(row)))
        for index,row in enumerate(original.find('./Menu/Options/Controls/Pad')):
            if row.tag in ('Move','Look'):controls.find('Pad').insert(index,ET.fromstring(ET.tostring(row)))
        menu=edited.find('./Menu')
        tags=[row.tag for row in menu]
        at=tags.index('Options')
        self.assertEqual(tags[at-2:at],['JNETCharacter','JNETWorld'])
        for name in ('JNETCharacter','JNETWorld'):
            self.assertEqual(menu.find(name).get('Action'),'%JNET_OPEN%')
            menu.remove(menu.find(name))
        for branch,before,name,above in [('Audio','Back','JNETCredits','MusicVol'),('Graphics','Default','JNETFPS','Anisotrophy')]:
            parent=menu.find('Options/'+branch)
            tags=[row.tag for row in parent]
            at=tags.index(name)
            self.assertEqual(tags[at-1:at+2],[above,name,before])
            self.assertEqual(parent.find(name).get('Setting'),'%'+name+'%')
            parent.remove(parent.find(name))
        menu.find('Start').set('Text',original.find('./Menu/Start').get('Text'))
        ET.indent(original);ET.indent(edited)
        self.assertEqual(ET.tostring(original),ET.tostring(edited))

    def test_native_entry_guards(self):
        source=(LAB/'diagnostic/probe_dll.c').read_text()
        binary=(ROOT/'JourneyPC/Journey.exe').read_bytes()
        sections=pe_sections(binary)
        for name,rva in [('xml',0x2ecf60),('setting',0x1084f0),('action',0xaea60),('dispatch',0x101e30),('push',0x107c70)]:
            values=re.search(r'controls_'+name+r'_entry\[\]=\{([^}]+)\}',source)[1]
            expected=bytes(int(v,16) for v in values.split(','))
            at=offset_for(sections,rva,len(expected))
            self.assertEqual(binary[at:at+len(expected)],expected)

    def test_creature_sound_aliases_exist_and_fit_native_table(self):
        lua=LuaRuntime()
        lua.execute((LAB/'work/pc_scripts/SoundEffectData.lua').read_text())
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        wrapper=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper=''.join(ast.literal_eval(part) for part in re.findall(r'"(?:\\.|[^"\\])*"',wrapper))
        lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()")
        sounds=lua.globals().soundEffectData
        bank=(ROOT/'JourneyPC/Data/Sounds/LvlSfxCave-SFX.bnk').read_bytes()
        for alias,event in [('JNTWarVoice','GuardianCameoBy'),('JNTWarLock','GuardianFinalStretchHungry1'),('JNTWarHungry','GuardianFinalStretchHungry2'),('JNTWarAway','GuardianFlyAway')]:
            self.assertEqual(sounds[alias].soundName,event)
            self.assertEqual(sounds[alias].bankName,'JNTMech')
            self.assertIn(event.encode()+b'\0',bank)
        for name in ('Guardian_VisorDown','Guardian_VisorUp','Guardian_LockOn','Guardian_LeapUp','Guardian_DiveBomb','Guardian_TearPlayerCape','GuardianSearchlight'):
            self.assertEqual(sounds[name].bankName,'JNTMech')
            self.assertIn(sounds[name].soundName.encode()+b'\0',bank)
        self.assertEqual(sounds.Guardian_VisorUp.minDist,15)
        self.assertEqual(sounds.Guardian_VisorUp.maxDist,35)
        self.assertEqual(sounds.JNTFishShort.soundName,'fish-idle-move-short')
        self.assertEqual(sounds.JNTFishCall.soundName,'fish-come-here')
        for alias, event in [('JNTFishShort','fish-idle-move-short'),
                             ('JNTFishMedium','fish-idle-move-medium'),
                             ('JNTFishLong','fish-idle-move-long'),
                             ('JNTFishCall','fish-come-here'),('JNTWhale','HappyWhale')]:
            self.assertEqual(sounds[alias].soundName,event)
            self.assertEqual(sounds[alias].bankName,'JNTBio')
            self.assertGreater(sounds[alias].maxDist,sounds[alias].minDist)
            self.assertIn(event.encode()+b'\0',
                          (ROOT/'JourneyPC/Data/Sounds/LvlSfxSummit-SFX.bnk').read_bytes())
        for prefix in ('Player','Player2'):
            for call in ('Coo','Chirp','Call','Shout'):
                self.assertIsNotNone(sounds[prefix+call])
        for alias,event in [('JNTFlowSnake','FlowSnakeCall'),('JNTFlowHunter','FlowHunterCall')]:
            self.assertEqual(sounds[alias].soundName,event)
            self.assertEqual(sounds[alias].bankName,'JNTFlow')
            self.assertIn(event.encode()+b'\0',
                          (ROOT/'JourneyPC/Data/Sounds/LvlSfxRuins-SFX.bnk').read_bytes())
        self.assertLessEqual(len(list(sounds.items())),160)

    def test_native_credits_filter(self):
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        chunks=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper=''.join(ast.literal_eval(part) for part in re.findall(r'"(?:\\.|[^"\\])*"',chunks))
        lua=LuaRuntime()
        lua.execute("""
            Names={['500dd125848f5b8df2b076f6270bfbe5']={}}
            __JNETCreditsEnabled=false
            function InitializeSignBarn() end
            function TriggerInitialize(trig,desc) applied=desc end
            Credits={{SectionName='INTRO',Entries={{Text='Thank you for playing'},{Text='Created by'}}},
                     {SectionName='Companions',Entries={{Text='Your Companions...'}}}}
        """)
        lua.execute(wrapper+' __JNETApplyScenery() InitializeSignBarn()')
        self.assertEqual(lua.globals().Credits[1].Entries[1].Text,'Thank you for playing')
        self.assertEqual(lua.globals().Credits[1].Entries[2].Text,'')
        self.assertEqual(lua.globals().Credits[2].Entries[1].Text,'Your Companions...')
        lua.execute("""
            TriggerInitialize({}, {Type='DisplayText',Vars={text='Thank you for playing',duration=12,fadeTime=1.5}})
            assert(applied.Vars.text=='Thank you for playing' and applied.Vars.duration==12 and applied.Vars.fadeTime==1.5)
            TriggerInitialize({}, {Type='DisplayText',Vars={text='Created by',duration=120}})
            assert(applied.Vars.text=='')
            TriggerInitialize({}, {Type='PlaySFXTrigger',Vars={soundName='M_9mC1',vol=1}})
            assert(applied.Vars.vol==0 and applied.Vars.soundName=='')
            muted=applied
            TriggerInitialize({}, {Type='PlaySFXTrigger',Vars={soundName='Other',vol=1}})
            assert(applied.Vars.vol==1)
            __JNETCreditsEnabled=true
            TriggerInitialize({}, {Type='PlaySFXTrigger',Vars={soundName='M_9mC1',vol=1}})
            assert(applied.Vars.vol==1 and applied.Vars.soundName=='M_9mC1')
            TriggerInitialize({}, muted)
            assert(applied.Vars.vol==1 and applied.Vars.soundName=='M_9mC1')
            __JNETCreditsEnabled=false
            TriggerInstances={muted}
            __JNETApplyScenery()
            assert(muted.Vars.soundName=='' and muted.Vars.vol==0)
            TriggerInitialize({}, {ObjectName='ac2588adb7a2f6c1ad3d5dc48fbb2811',Type='HudAddElement',
                Vars={elementName='JourneyTitle',fadeIn=2,fadeOut=0,duration=10,alpha=1.25,posx=-0.85,posy=0.05}})
            assert(applied.Vars.fadeOut==1.5 and applied.Vars.fadeIn==1.5 and applied.Vars.duration==12)
            assert(applied.Vars.alpha==1 and applied.Vars.posx==-0.85 and applied.Vars.posy==0.05)
            TriggerInitialize({}, {ObjectName='Other',Type='HudAddElement',Vars={elementName='JourneyTitle',fadeOut=0}})
            assert(applied.Vars.fadeOut==0)
        """)

    @staticmethod
    def _mountain_camera_snapshot(value):
        if hasattr(value,'items'):
            return {k:ControlsTests._mountain_camera_snapshot(v) for k,v in value.items()}
        return value

    def _mountain_camera_fixture(self):
        import gzip
        from lupa.lua53 import LuaRuntime as Lua53
        folder=ROOT/'JourneyPC/Data/Scripts/Level_Mountain'
        triggers=gzip.decompress((folder/'__TriggerInstances.lua.gz').read_bytes()).decode()
        clumps=gzip.decompress((folder/'__ClumpInstances.lua.gz').read_bytes()).decode()
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        chunks=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper=''.join(ast.literal_eval(part) for part in re.findall(r'"(?:\\.|[^"\\])*"',chunks))
        lua=Lua53()
        lua.globals().Names=lua.table_from({key:key for key in re.findall(r'(?:Names\[|ObjectName = )"([a-f0-9]+)"',triggers+clumps)})
        lua.execute('''
            __JNETCreditsEnabled=true initialized={} initializeCalls=0
            current_level='Level_Mountain'
            game={GetCurrentLevelString=function() return current_level end}
            function OriginalMountainInitialize(trig,desc)
                initializeCalls=initializeCalls+1 initialized[desc.ObjectName]=desc
                trig.vars=desc.Vars
                function trig:Enable(flag) self.enabled=flag end
                return trig
            end
            TriggerInitialize=OriginalMountainInitialize
        ''')
        lua.execute(triggers);lua.execute(clumps)
        lua.execute(wrapper)
        return lua,triggers

    def _mountain_camera_records(self,lua,name='TriggerInstances'):
        return {row.ObjectName:self._mountain_camera_snapshot(row)
                for index,row in lua.globals()[name].items() if isinstance(index,int)}

    def test_mountain_camera_filter_changes_only_nine_source_records(self):
        from copy import deepcopy
        lua,_=self._mountain_camera_fixture()
        before=self._mountain_camera_records(lua)
        clumps=self._mountain_camera_records(lua,'ClumpInstances')
        regions=('a77d34545bc9b6fc95e857f67fa0067a','4f6615f5a3f9e5695f3a7fe7789a2fc8',
                 'd1a0e0b6c348ba723d4a355ddd335ee1','c37c3c3fccbc5d5275b657290f4318e2',
                 '0a72efb72de0bf4b2796c1873eadaae0','b877b24f969fd3a7dc14e5fc7b194960',
                 '2d83a6adaecf2c9643db41b6e5304c22')
        snap='91d5861a781402da21b2985d52dcf46e'
        stairs_region='eecf196522e6ad01d63b595c99a94fa3'
        expected=deepcopy(before)
        for key in regions:
            self.assertEqual(before[key]['Type'],'EventFromRegion')
            self.assertTrue(any(before[key]['Vars'][field] for field in ('onEnter','onExit','fireContinuously')))
            for field in ('onEnter','onExit','fireContinuously'):expected[key]['Vars'][field]=False
        self.assertEqual(before[snap]['Type'],'SnapToCamTrigger')
        self.assertNotEqual(before[snap]['Vars']['activationRegion'],0)
        expected[snap]['Vars']['activationRegion']=0
        expected[stairs_region]['Vars']['effects']='8790e3ade473ed442455d1987ba6ff53'
        lua.execute('__JNETApplyScenery()')
        after=self._mountain_camera_records(lua)
        self.assertEqual(after,expected)
        self.assertEqual({key for key in before if before[key]!=after[key]},set(regions)|{snap,stairs_region})
        self.assertEqual(self._mountain_camera_records(lua,'ClumpInstances'),clumps)
        for key in ('4964f0c161a4ba7181f4374c7b9740ab','8042a412c9355a7b18c0ec01c72eb3f6',
                    'afa59a1bddd639f4726af2d183571e0b','cbd065526386783310e7751a56eaf2b4',
                    'c76d5c31e978aef5949cd79278c201ef','d5c293c6794aeaa2c9fd5fb045e32d89'):
            self.assertEqual(after[key],before[key])

    def test_mountain_camera_source_paths_cut_and_stairs_preserved(self):
        lua,_=self._mountain_camera_fixture()
        before=self._mountain_camera_records(lua)
        clumps=self._mountain_camera_records(lua,'ClumpInstances')
        cameras={'6e217a87dbfed6c4317ff7b846aa74a4','953fe2549e31d9ca5b2922038c4c3122',
                 '91d5861a781402da21b2985d52dcf46e','a693bd2b935a2e1e3e26b5cd826c4aff',
                 '995f9a6b3702ffc27d77bea3393159c6','77f21ee258490a015f946645b977d077',
                 '305b0be8ba1f009ddaaf90e88fde8f00','2256355a355b51e59982c1151b0ffb9b'}
        def leaves(key,seen=frozenset()):
            self.assertNotIn(key,seen)
            if key not in clumps:return {key}
            return set().union(*(leaves(child,seen|{key}) for child in clumps[key]['Objects'].values()))
        def paths(rows):
            result=set()
            for key,row in rows.items():
                fields=row.get('Vars',{})
                if row['Type']=='EventFromRegion' and any(fields.get(flag) for flag in ('onEnter','onExit','fireContinuously')):
                    result.update((key,camera) for camera in leaves(fields.get('effects')) if camera in cameras)
                if key in cameras and fields.get('activationRegion') not in (None,0):result.add((key,key))
            return result
        original_paths=paths(before)
        self.assertEqual({camera for _,camera in original_paths},cameras)
        self.assertEqual(len(original_paths),9)
        lua.execute('__JNETApplyScenery()')
        after=self._mountain_camera_records(lua)
        self.assertEqual(paths(after),set())
        region='eecf196522e6ad01d63b595c99a94fa3';stairs='8790e3ade473ed442455d1987ba6ff53'
        self.assertIn(stairs,leaves(before[region]['Vars']['effects']))
        self.assertEqual(leaves(after[region]['Vars']['effects']),{stairs})
        self.assertTrue(after[region]['Vars']['onEnter'])
        self.assertEqual(after[stairs],before[stairs])
        self.assertTrue(after[stairs]['Vars']['enableFlag'])
        for key,row in before.items():
            if 'Cam' in row['Type'] and key!='91d5861a781402da21b2985d52dcf46e':self.assertEqual(after[key],row)

    def test_mountain_camera_reload_initialize_reenable_and_type_guards(self):
        lua,triggers=self._mountain_camera_fixture()
        lua.execute('__JNETApplyScenery()')
        expected=self._mountain_camera_records(lua)
        for replace_initializer in (False,True):
            lua.execute(triggers)
            if replace_initializer:lua.execute('TriggerInitialize=OriginalMountainInitialize __JNETApplyScenery()')
            lua.execute('''
                initialized={} initializeCalls=0
                for _,desc in ipairs(TriggerInstances) do TriggerInitialize({},desc) end
            ''')
            self.assertEqual(self._mountain_camera_records(lua),expected)
            self.assertEqual(self._mountain_camera_snapshot(lua.globals().initialized),expected)
            self.assertEqual(lua.globals().initializeCalls,len(expected))
        lua.execute('''
            local rows={} for _,desc in ipairs(TriggerInstances) do rows[desc.ObjectName]=desc end
            local sourceEnable=rows['0a304641e02c3b9d7ced9f2edb6f9864']
            assert(sourceEnable.Type=='Enable' and sourceEnable.Vars.enableFlag==true)
            assert(sourceEnable.Vars.objects=='d1a0e0b6c348ba723d4a355ddd335ee1')
            local active=TriggerInitialize({},rows[sourceEnable.Vars.objects])
            active:Enable(false) active:Enable(sourceEnable.Vars.enableFlag)
            assert(active.enabled and not active.vars.onEnter and not active.vars.onExit and not active.vars.fireContinuously)
            __JNETCreditsEnabled=false __JNETApplyScenery()
            __JNETCreditsEnabled=true __JNETApplyScenery()
        ''')
        self.assertEqual(self._mountain_camera_records(lua),expected)
        for key,kind in (('a77d34545bc9b6fc95e857f67fa0067a','SnapToCamTrigger'),
                         ('91d5861a781402da21b2985d52dcf46e','EventFromRegion'),
                         ('eecf196522e6ad01d63b595c99a94fa3','Enable')):
            desc=lua.table_from(dict(ObjectName=key,Type=kind,Vars=dict(onEnter=True,onExit=True,
                fireContinuously=True,activationRegion='source-region',effects='source-effect')),recursive=True)
            before=self._mountain_camera_snapshot(desc)
            lua.globals().TriggerInitialize(lua.table(),desc)
            self.assertEqual(self._mountain_camera_snapshot(desc),before)
        lua.globals().Names['8790e3ade473ed442455d1987ba6ff53']=None
        desc=lua.table_from(dict(ObjectName='eecf196522e6ad01d63b595c99a94fa3',Type='EventFromRegion',
            Vars=dict(onEnter=True,onExit=False,fireContinuously=False,effects='934b6411f0044ecf4626e55957acf4ee')),recursive=True)
        before=self._mountain_camera_snapshot(desc)
        lua.globals().TriggerInitialize(lua.table(),desc)
        self.assertEqual(self._mountain_camera_snapshot(desc),before)

    def test_mountain_camera_filter_is_scoped_to_current_level(self):
        import gzip
        from copy import deepcopy
        lua,mountain_source=self._mountain_camera_fixture()
        mountain_original=self._mountain_camera_records(lua)
        lua.execute('__JNETApplyScenery()')
        mountain_expected=self._mountain_camera_records(lua)
        targeted={key for key in mountain_original if mountain_original[key]!=mountain_expected[key]}
        reused_levels=set()
        def load(source):
            lua.globals().Names=lua.table_from({key:key for key in re.findall(r'(?:Names\[|ObjectName = )"([a-f0-9]+)"',source)})
            lua.execute(source)
        for path in sorted((ROOT/'JourneyPC/Data/Scripts').glob('Level_*/__TriggerInstances.lua.gz')):
            level=path.parent.name
            if level=='Level_Mountain':continue
            with self.subTest(level=level):
                source=gzip.decompress(path.read_bytes()).decode()
                lua.globals().current_level=level
                load(source)
                original=self._mountain_camera_records(lua)
                if targeted.intersection(original):reused_levels.add(level)
                expected=deepcopy(original)
                title=expected.get('ac2588adb7a2f6c1ad3d5dc48fbb2811')
                if title and title['Type']=='HudAddElement' and title['Vars'].get('elementName')=='JourneyTitle':
                    title['Vars'].update(duration=12,fadeIn=1.5,fadeOut=1.5,alpha=1)
                lua.execute('__JNETApplyScenery()')
                self.assertEqual(self._mountain_camera_records(lua),expected)
                load(source)
                lua.execute('for _,desc in ipairs(TriggerInstances) do TriggerInitialize({},desc) end')
                self.assertEqual(self._mountain_camera_records(lua),expected)
        self.assertTrue({'Level_Canyon','Level_Cave','Level_Desert','Level_Graveyard','Level_Ruins','Level_Summit'}<=reused_levels)
        for absent in ('game=nil','game={}','game={GetCurrentLevelString=function() return nil end}'):
            with self.subTest(api=absent):
                load(mountain_source);lua.execute(absent)
                lua.execute('__JNETApplyScenery()')
                self.assertEqual(self._mountain_camera_records(lua),mountain_original)
                lua.execute('for _,desc in ipairs(TriggerInstances) do TriggerInitialize({},desc) end')
                self.assertEqual(self._mountain_camera_records(lua),mountain_original)
        lua.execute('''
            current_level='Level_Desert'
            game={GetCurrentLevelString=function() return current_level end}
            TriggerInitialize=OriginalMountainInitialize __JNETApplyScenery()
            current_level='Level_Mountain'
        ''')
        load(mountain_source)
        lua.execute('for _,desc in ipairs(TriggerInstances) do TriggerInitialize({},desc) end')
        self.assertEqual(self._mountain_camera_records(lua),mountain_expected)

    def test_registered_animation_actions(self):
        lua=LuaRuntime()
        lua.execute((LAB/'work/pc_scripts/DudeAnimation.lua').read_text())
        source=(LAB/'work/pc_scripts/DudeAnimation.lua').read_text()
        for name in ('Crouch','CrouchToIdle','SlideJump1','SlideJump2','RollF','RollL','RollR'):
            self.assertRegex(source,rf'\b{name}\s*=\s*\{{\s*isAction\s*=\s*true')

    def test_spin_registration_and_clip(self):
        lua=LuaRuntime()
        lua.execute((LAB/'work/pc_scripts/DudeAnimation.lua').read_text())
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        wrapper=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        wrapper=''.join(ast.literal_eval(part) for part in re.findall(r'"(?:\\.|[^"\\])*"',wrapper))
        before=list(lua.globals().GetDudeActionAnims().values())
        self.assertNotIn('SlideJump3',before)
        native_sequences=list(lua.globals().GetDudeAnimSequences().values())
        for name in ('RollL','RollR'):
            self.assertIn(name,before)
            self.assertNotIn(name,native_sequences)
        lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()");lua.execute(wrapper+" __JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()")
        after=list(lua.globals().GetDudeActionAnims().values())
        self.assertEqual(after,before+['JNETSpin3','JNETFlipL','JNETFlipR'])
        self.assertLessEqual(len(after),196)
        lua.execute("""
            loaded={} queued={} sequences={}
            barn={AddActionAnimation=function(self)
                return {SetName=function(self,name) loaded[#loaded+1]=name end,
                        isAction=function() end,isAdditive=function() end,
                        isDucker=function() end,isDucked=function() end}
            end,AddAnimSequence=function(self,name) sequences[#sequences+1]=name end,QueueAnimation=function(self,...) queued={...} end}
            LoadDudeActionAnimations(barn)
            LoadDudeAnimSequences(barn)
            QueueAnimationSequence(barn,'JNETSpin3')
        """)
        self.assertEqual(list(lua.globals().loaded.values()),after)
        self.assertIn('JNETSpin3',list(lua.globals().sequences.values()))
        from character_resources import character_resource_script, wayfarer_resource_script, character_animation_files, character_animation_file_header
        binary=(ROOT/'JourneyPC/Journey.exe').read_bytes()
        from startup_resources import extract_persistent, prepare_candidate
        native, _ = prepare_candidate(extract_persistent(binary), active_traveler=True)
        startup=wayfarer_resource_script()+native+character_resource_script(binary)
        self.assertEqual(startup.count(b'resource "MeshAnimation" "JNETSpin3"'),1)
        self.assertLess(startup.index(b'resource "MeshAnimation" "JNETSpin3"'),
                        startup.index(b'resource "Mesh" "BlendNinja"'))
        self.assertIn(b'resource "MeshAnimation" "JNETSpin3" { source="C_ClothNinja_A_SlideJump3.PSSG", meshName="BlendNinja" }',startup)
        files=character_animation_files(binary)
        self.assertIn('JNETSpin3.D3D11x64',files)
        self.assertIn('"JNETSpin3","JNETSpin3.D3D11x64"',character_animation_file_header(files))
        self.assertIn(b'JNET/Meshes/C_ClothNinja_A_SlideJump3.dae#',files['JNETSpin3.D3D11x64'])
        self.assertEqual(lua.globals().queued[1],'JNETSpin3')
        for name,clip in (('JNETFlipL','RollL'),('JNETFlipR','RollR')):
            self.assertIn(name,list(lua.globals().sequences.values()))
            self.assertEqual(list(lua.globals().GetDudeAnimSequences().values()).count(name),1)
            declaration=('resource "MeshAnimation" "'+name+'"').encode()
            self.assertEqual(startup.count(declaration),1)
            self.assertLess(startup.index(declaration),startup.index(b'resource "Mesh" "BlendNinja"'))
            self.assertIn(('JNET/Meshes/C_ClothNinja_A_'+clip+'.dae#').encode(),files[name+'.D3D11x64'])
            lua.execute("QueueAnimationSequence(barn,'"+name+"')")
            self.assertEqual(list(lua.globals().queued.values()),[name,0.2,0.3,2,1,0])
        self.assertLessEqual(len(list(lua.globals().sequences.values())),196)
        lua.execute("QueueAnimationSequence(barn,'SlideJump1')")
        self.assertEqual(lua.globals().queued[1],'SlideJump1')
        for name in ('SlideJump1','SlideJump3'):
            self.assertTrue((ROOT/f'JourneyPC/Data/Meshes/C_ClothNinja_A_{name}.D3D11x64').is_file())

    def test_setup_dispatcher_caches_each_state_and_updates_flags(self):
        from lupa.lua53 import LuaRuntime as Lua53
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        block=source.split('static const char *const chunks[]={')[1].split('    };')[0]
        chunks=[''.join(ast.literal_eval(v) for v in re.findall(r'"(?:\\.|[^"\\])*"',part))+' end ' for part in block.split('" end ",')[:-1]]
        block=source.split('snprintf(code,sizeof(code),')[1].split('jnet_credits?')[0]
        dispatcher=''.join(ast.literal_eval(v) for v in re.findall(r'"(?:\\.|[^"\\])*"',block))
        for _ in range(2):
            lua=Lua53();prepared=[]
            def prepare():
                prepared.append(True)
                for chunk in chunks:lua.execute(chunk)
            for i in range(100):
                lua.execute(dispatcher % ('true' if i%2 else 'false','false'),prepare)
                self.assertEqual(lua.globals()['__JNETCreditsEnabled'],bool(i%2))
            self.assertEqual(len(prepared),1)
            lua.execute((LAB/'work/pc_scripts/DudeAnimation.lua').read_text())
            lua.execute(dispatcher % ('true','true'),prepare)
            self.assertEqual(list(lua.globals().GetDudeActionAnims().values()).count('JNETSpin3'),1)
            self.assertTrue(lua.globals()['__JNETParadiseCutscene'])
            self.assertEqual(len(prepared),1)

    def test_setup_cache_lua53_pool_pressure(self):
        import ctypes as c
        import lupa.lua53
        lib=c.CDLL(lupa.lua53.__file__)
        heap=c.CDLL(None)
        heap.realloc.argtypes=[c.c_void_p,c.c_size_t];heap.realloc.restype=c.c_void_p
        heap.free.argtypes=[c.c_void_p]
        allocator=c.CFUNCTYPE(c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_size_t)
        lib.lua_newstate.argtypes=[allocator,c.c_void_p];lib.lua_newstate.restype=c.c_void_p
        lib.luaL_openlibs.argtypes=[c.c_void_p]
        lib.luaL_loadbufferx.argtypes=[c.c_void_p,c.c_char_p,c.c_size_t,c.c_char_p,c.c_char_p]
        lib.lua_pcallk.argtypes=[c.c_void_p,c.c_int,c.c_int,c.c_int,c.c_ssize_t,c.c_void_p]
        lib.lua_settop.argtypes=[c.c_void_p,c.c_int];lib.lua_close.argtypes=[c.c_void_p]
        source=(LAB/'diagnostic/jnet_freeplay_live.h').read_text()
        parts=source.split('static const char *const chunks[]={')[1].split('    };')[0].split('" end ",')[:-1]
        chunks=[''.join(ast.literal_eval(v) for v in re.findall(r'"(?:\\.|[^"\\])*"',part))+' end ' for part in parts]
        old=''.join(chunk.split('() ',1)[1].rsplit(' end ',1)[0] for chunk in chunks)
        def peak(cached):
            live={};high=0
            @allocator
            def allocate(user,ptr,oldsize,size):
                nonlocal high
                if not size:
                    live.pop(ptr,None);heap.free(ptr);return None
                result=heap.realloc(ptr,size)
                if result:
                    live.pop(ptr,None);live[result]=size
                    high=max(high,sum(4096<n<=8192 for n in live.values()))
                return result
            state=lib.lua_newstate(allocate,None)
            self.assertTrue(state)
            def execute(code):
                data=code.encode()
                self.assertEqual(lib.luaL_loadbufferx(state,data,len(data),b'fixture',b't'),0)
                self.assertEqual(lib.lua_pcallk(state,0,0,0,0,None),0)
                lib.lua_settop(state,0)
            try:
                lib.luaL_openlibs(state)
                execute("collectgarbage('stop')")
                if cached:
                    for chunk in chunks:execute(chunk)
                for _ in range(100):
                    execute('__JNETApplySpin() __JNETApplySounds() __JNETApplyScenery()' if cached else old)
                return high
            finally:lib.lua_close(state)
        binary=(ROOT/'JourneyPC/Journey.exe').read_bytes();sections=pe_sections(binary)
        self.assertEqual(binary[offset_for(sections,0xf0967)],0xcc)
        table=offset_for(sections,0x5c38f0)
        self.assertIn((16,8192),[struct.unpack_from('<II',binary,table+i*8) for i in range(15)])
        previous,current=peak(False),peak(True)
        self.assertGreaterEqual(previous,16)
        self.assertLess(current,16)
        print(f'Lua 5.3 setup 8192-byte bucket peak: repeated compilation {previous}, cached {current}')

    def test_settings_apply_revert(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        functions=source[source.index('static unsigned jnet_saved_credits'):source.index('static uintptr_t jnet_menu_xml')]
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary)
            (folder/'settings_fixture.h').write_text(functions)
            exe=folder/'settings'
            subprocess.run(['clang','-std=c11','-fsanitize=address,undefined','-I'+str(folder),str(LAB/'diagnostic/test_jnet_settings.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_setting_lookup_is_bounded(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        function=source[source.index('static uintptr_t jnet_setting(uintptr_t'):source.index('static unsigned jnet_controls_playstation_buttons')]
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary)
            (folder/'setting_fixture.h').write_text(function)
            exe=folder/'setting'
            subprocess.run(['clang','-std=c11','-fsanitize=address,undefined','-I'+str(folder),'-I'+str(LAB/'diagnostic'),str(LAB/'diagnostic/test_jnet_setting.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_playstation_native_buttons(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        function=source[source.index('static unsigned jnet_controls_playstation_buttons'):source.index('struct jnet_controls_pad_state')]
        fixture=r"""
#include <stdint.h>
#include <string.h>
#include <assert.h>
static uintptr_t fault_image=0x140000000ULL,slots[4];
static unsigned char devices[4][0x470];
static int fail_read;
static unsigned reads,unplug;
static int jnet_menu_read(uintptr_t at,void *out,size_t n){
    ++reads;
    if(fail_read)return 0;
    if(at>=fault_image+0x403e400 && at+n<=fault_image+0x403e420){memcpy(out,(char *)slots+(at-fault_image-0x403e400),n);return 1;}
    for(unsigned i=0;i<4;++i)if(at>=(uintptr_t)devices[i] && at+n<=(uintptr_t)devices[i]+sizeof(devices[i])){
        memcpy(out,(void *)at,n);
        if(at==(uintptr_t)devices[i]+0x454 && unplug){if(unplug==1)slots[i]=0;else devices[i][0xc5]=0;}
        return 1;
    }
    return 0;
}
"""+function+r"""
int main(void){
    unsigned native[]={12,14,15,13,9,8,10,11,4,5,1,2,0,3,6,7};
    assert(!jnet_controls_playstation_buttons() && reads==1);
    for(unsigned slot=0;slot<4;++slot){
        slots[slot]=(uintptr_t)devices[slot];uintptr_t type=fault_image+0x682b78;
        memcpy(devices[slot],&type,8);devices[slot][0xc5]=1;
        for(unsigned bit=0;bit<16;++bit){
            devices[slot][0x454+native[bit]]=1;
            assert(jnet_controls_playstation_buttons()==(1u<<bit));
            devices[slot][0x454+native[bit]]=0;
        }
        devices[slot][0x455]=1;devices[slot][0xc5]=0;assert(!jnet_controls_playstation_buttons());
        devices[slot][0xc5]=1;devices[slot][0x455]=2;assert(!jnet_controls_playstation_buttons());
        devices[slot][0x455]=1;unplug=1;assert(!jnet_controls_playstation_buttons());slots[slot]=(uintptr_t)devices[slot];
        unplug=2;assert(!jnet_controls_playstation_buttons());unplug=0;devices[slot][0xc5]=1;
        devices[slot][0x455]=1;type=0;memcpy(devices[slot],&type,8);assert(!jnet_controls_playstation_buttons());
        slots[slot]=0;
    }
    fail_read=1;assert(!jnet_controls_playstation_buttons());
}
"""
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary);(folder/'pad.c').write_text(fixture);exe=folder/'pad'
            subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(folder/'pad.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_xinput_compatibility(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        function=source[source.index('struct jnet_controls_pad_state'):source.index('static uintptr_t jnet_crouch_controller;')]
        intro=(LAB/'diagnostic/jnet_intro_live.h').read_text()
        intro=intro[intro.index('static int jnet_intro_input_down'):intro.index('#endif')]
        fixture=r"""
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#define WINAPI
#define ERROR_SUCCESS 0
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 30
typedef unsigned DWORD;
typedef int32_t LONG;
typedef void *HMODULE;
typedef struct { DWORD dwPacketNumber; struct { unsigned short wButtons; unsigned char bLeftTrigger,bRightTrigger; int16_t sThumbLX,sThumbLY,sThumbRX,sThumbRY; } Gamepad; } XINPUT_STATE;
static XINPUT_STATE states[4];
static unsigned present,calls,native,loads,available;
static LONG JNETFrames;
static unsigned foreground=1,keys[256],key_calls,native_calls;
static uint64_t now;
static DWORD get(DWORD slot,XINPUT_STATE *state){assert(slot<4);++calls;*state=states[slot];return present&(1u<<slot)?0:1167;}
static HMODULE GetModuleHandleW(const wchar_t *name){(void)name;return 0;}
static HMODULE LoadLibraryW(const wchar_t *name){
    const wchar_t *names[]={L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"};
    assert(loads<3 && !wcscmp(name,names[loads]));++loads;
    return loads==available || (available==4 && loads<=2)?(void *)(uintptr_t)loads:0;
}
static void *GetProcAddress(HMODULE module,const char *name){assert(module && !strcmp(name,"XInputGetState"));return available==4 && module==(void *)1?NULL:(void *)get;}
static uint64_t GetTickCount64(void){return now;}
static unsigned jnet_controls_playstation_buttons(void){++native_calls;return native;}
static void *GetForegroundWindow(void){return NULL;}
static void GetWindowThreadProcessId(void *window,DWORD *process){(void)window;*process=foreground;}
static unsigned GetCurrentProcessId(void){return 1;}
static unsigned GetAsyncKeyState(unsigned key){assert(key<256);++key_calls;return keys[key]?0x8000:0;}
"""+function+intro+r"""
static unsigned next_buttons(void){++JNETFrames;return jnet_controls_buttons();}
int main(int argc,char **argv){
    assert(argc==2);available=(unsigned)atoi(argv[1]);native=8;
    assert(next_buttons()==8);assert(loads==(available==4?2:available?available:3));
    if(!available){assert(!calls);assert(jnet_intro_input_down()==1);assert(native_calls==1);return 0;}
    assert(calls==4);native=0;present=15;now=999;
    assert(!next_buttons() && calls==4);
    now=1000;assert(!next_buttons() && calls==8);
    unsigned masks[]={1,2,4,8,16,32,64,128,256,512,4096,8192,16384,32768};
    for(unsigned slot=0;slot<4;++slot){
        for(unsigned bit=0;bit<14;++bit){states[slot].Gamepad.wButtons=masks[bit];assert(next_buttons()==(1u<<bit));}
        states[slot].Gamepad.wButtons=0;
        states[slot].Gamepad.bLeftTrigger=30;states[slot].Gamepad.bRightTrigger=30;assert(!next_buttons());
        states[slot].Gamepad.bLeftTrigger=31;assert(next_buttons()==(1u<<14));
        states[slot].Gamepad.bRightTrigger=255;assert(next_buttons()==(3u<<14));
        memset(&states[slot],0,sizeof(states[slot]));
    }
    states[0].Gamepad.wButtons=4096;states[3].Gamepad.wButtons=8192;
    assert(next_buttons()==((1u<<10)|(1u<<11)));
    present=8;assert(next_buttons()==(1u<<11));memset(states,0,sizeof(states));
    present=0;assert(!next_buttons());unsigned before=calls;
    assert(!next_buttons() && calls==before);
    present=8;states[3].Gamepad.wButtons=8192;now=2000;
    assert(next_buttons()==(1u<<11));
    before=calls;assert(next_buttons()==(1u<<11) && calls==before+1);
    before=calls;unsigned sony=native_calls;assert(jnet_controls_buttons()==(1u<<11));
    assert(jnet_intro_input_down()==1 && calls==before && native_calls==sony);
    memset(states,0,sizeof(states));++JNETFrames;assert(!jnet_intro_input_down());
    for(unsigned axis=0;axis<4;++axis)for(unsigned direction=0;direction<2;++direction){
        int16_t *value=axis==0?&states[3].Gamepad.sThumbLX:axis==1?&states[3].Gamepad.sThumbLY:
            axis==2?&states[3].Gamepad.sThumbRX:&states[3].Gamepad.sThumbRY;
        *value=direction?-16000:16000;++JNETFrames;assert(!jnet_intro_input_down());
        *value=direction?-16001:16001;++JNETFrames;assert(jnet_intro_input_down()==1 && !jnet_controls_buttons());
        *value=0;
    }
    states[3].Gamepad.bLeftTrigger=31;++JNETFrames;assert(jnet_intro_input_down()==1 && jnet_controls_buttons()==(1u<<14));
    states[3].Gamepad.bLeftTrigger=0;states[3].Gamepad.bRightTrigger=31;++JNETFrames;
    assert(jnet_intro_input_down()==1 && jnet_controls_buttons()==(1u<<15));
    memset(states,0,sizeof(states));native=1u<<10;++JNETFrames;assert(jnet_intro_input_down()==1);
    native=0;++JNETFrames;assert(!jnet_intro_input_down());
    keys[255]=1;++JNETFrames;before=calls;assert(jnet_intro_input_down()==1 && calls==before);keys[255]=0;
    foreground=2;++JNETFrames;unsigned polled=key_calls;assert(jnet_intro_input_down()==-1 && calls==before && key_calls==polled);
}
"""
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary);(folder/'pad.c').write_text(fixture);exe=folder/'pad'
            subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(folder/'pad.c'),'-o',str(exe)],check=True)
            for library in range(5):
                subprocess.run([str(exe),str(library)],check=True)

    def test_action_queue_and_button_edges(self):
        source=(LAB/'diagnostic/jnet_controls_live.h').read_text()
        functions=source[source.index('static uintptr_t jnet_crouch_controller;'):source.rindex('#endif')]
        with tempfile.TemporaryDirectory(dir=LAB/'work') as temporary:
            folder=Path(temporary)
            (folder/'controls_fixture.h').write_text(functions)
            exe=folder/'controls'
            subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(folder),str(LAB/'diagnostic/test_jnet_controls.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__=='__main__':
    unittest.main()
