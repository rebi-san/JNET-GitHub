import functools
import unittest
from pathlib import Path
from lupa import LuaRuntime
from character_resources import lua_literal, decorations
from daynight_resources import profiles as generate_profiles, records, MAPPINGS, MINIATURES, ALTERNATES, luminance, params, runtime_payload, PART_LIMIT
profiles = functools.lru_cache(maxsize=1)(generate_profiles)
payload = functools.lru_cache(maxsize=1)(lambda: runtime_payload(profiles()))
LEVELS = range(len(MAPPINGS))
ALTERNATE_LEVELS = tuple(i for i, m in enumerate(MAPPINGS) if m[0] in ALTERNATES)


def active(level, mode):
    """The profile the runtime applies: Desert and Canyon use their Matt Dusk/Dawn look for modes 1 and 2."""
    profile = profiles()[level]
    return profile['alternate'] if 'alternate' in profile and mode in (1, 2) else profile


def night_mode(level):
    return 3 if level in ALTERNATE_LEVELS else 1

LAB=Path(__file__).resolve().parent

class DayNightTests(unittest.TestCase):
    def scene(self, level):
        lua=LuaRuntime()
        lua.execute('''
        generation=0 allocations=0 starts=0 visits=0
        DecorationBlend_Hidden=8 DecorationBlend_Additive=5
        function object()
            local o={generation=generation}
            return setmetatable(o,{__index=function(self,key)
                local methods={SetShadow=true,SetFog=true,SetLight=true,SetSky=true,SetBloom=true,SetVelvet=true,
                    SetEnabled=true,SetBlend=true,SetShaderTextureParam=true,SetShaderParam4f=true,SetShaderParamf=true,SetAt=true,
                    GetName=true,GetBlueprint=true,intensity=true,SetPos=true,SetTargetPos=true,SetDir=true,
                    SetFollowCamera=true,SetOffsetPos=true,SetForceCache=true,windHullBlendVel=true,windHullBlendDir=true,
                    camLeadMult=true,followLocalPlayer=true,followRemotePlayer=true,targetLocalPlayer=true,
                    targetRemotePlayer=true,SetTargetJointName=true,killParticlesOnDisable=true}
                if not methods[key] then return nil end
                return function(self,...)
                    assert(self.generation==generation,'stale native object')
                    self[key..'Value']={...}
                    if key=='SetShaderParam4f' or key=='SetShaderParamf' then
                        local name,value=...
                        self.shaderValues=self.shaderValues or {}
                        self.shaderValues[name]=value
                    end
                    if key=='GetName' then return self.name end
                    if key=='GetBlueprint' then return self end
                    if key=='Start' then starts=starts+1 end
                end
            end})
        end
        Names={} emitterData={Stars={intensity=.1},StarsBright={intensity=5},StarsDim={intensity=.03},ShootingStars={intensity=1}}
        emitters={} for n in pairs(emitterData) do local o=object() o.name=n table.insert(emitters,o) end
        particles={GetNumEmitters=function() return #emitters end,GetEmitter=function(self,i) return emitters[i+1] end}
        fx={AddEffect=function(self,p,n) assert(not updating,'runtime particle allocation') allocations=allocations+1 return object() end}
        game={particleBarn=function() return particles end,resources=function() return {} end,
            clumpBarn=function() error('unexpected runtime clump') end,
            eventBarn=function() error('unexpected runtime event') end}
        function ParticleEmitterInitialize(o,d) o:SetEnabled(d.Enabled) end
        function LoadEnvNodes(env)
            for _,d in ipairs(EnvNodeInstances) do Names[d.ObjectName]=object() end
        end
        function LoadEnvFXParticles(f,p)
            for _,d in ipairs(ParticleEmitterInstances) do Names[d.ObjectName]=object() end
            ParticleEmitterInstances={}
        end
        function vec3_scalar_mult(v,n) return {v[1]*n,v[2]*n,v[3]*n} end
        function vec4_scalar_mult(v,n) return {v[1]*n,v[2]*n,v[3]*n,v[4]*n} end
        function GammaToLinearVec3(v)return v end function GammaToLinearVec4(v)return v end
        function visit() visits=visits+1 end

        ''')
        lua.execute((LAB/'work/pc_scripts/EnvFX.lua').read_text())
        lua.execute(payload()['source'],lua.globals().visit)
        self.load(lua,level)
        lua.globals().allocations=0
        lua.globals().updating=True
        return lua

    def load(self,lua,level,parts=True):
        lua.globals().updating=False
        name='Level_'+MAPPINGS[level][0]
        lua.execute('EnvNodeInstances='+lua_literal(list(records(name,'EnvNode').values()))+';ParticleEmitterInstances='+lua_literal(list(records(name,'ParticleEmitter').values())))
        lua.execute('Names={} LoadEnvNodes({}) LoadEnvFXParticles(fx,particles)')
        for key in decorations(name): lua.globals().Names[key]=lua.globals().object()
        if parts:self.load_parts(lua,level)
        lua.globals().updating=True

    def load_parts(self,lua,level):
        # what jnet_daynight_load_level() does in C: compile and run each part separately
        for part in payload()['parts'][level]:lua.execute(part)

    def test_update_before_particle_initialization(self):
        lua=self.scene(0)
        lua.execute('__JNETDayNight.effects=nil')
        lua.globals()['__JNETDayNightUpdate'](0,0,1)
        self.assertEqual(lua.globals().allocations,0)

    def test_full_cycle_and_restore(self):
        for level in LEVELS:
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            key=next(iter(profiles()[level]['nodes']))
            day=records('Level_'+MAPPINGS[level][0],'EnvNode')[key]
            node=lua.globals().Names[key]
            update(0,level,0)
            self.assertEqual(lua.globals().allocations,0)
            for time,amount in [(0,0),(450,.5),(900,1),(1350,.5),(1800,0),(2700,1),(3600,0)]:
                update(time,level,2)
                night=active(level,2)['nodes'][key]['SkyMaxColor'][0]
                actual=node.SetSkyValue[1][1]
                self.assertAlmostEqual(actual,(day['SkyMaxColor'][0]*(1-amount)+night*amount)/255)
            count=lua.globals().allocations
            update(4000,level,0)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day['SkyMaxColor'][0]/255)
            update(5000,level,2)
            self.assertEqual(lua.globals().allocations,count)
            self.assertIsNone(lua.eval('rawget')(node,'SetEnabledValue'))

    def test_fixed_night_and_cycle_switch(self):
        for level in LEVELS:
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            key=next(iter(profiles()[level]['nodes']))
            node=lua.globals().Names[key]
            night=active(level,1)['nodes'][key]['SkyMaxColor'][0]/255
            day=records('Level_'+MAPPINGS[level][0],'EnvNode')[key]['SkyMaxColor'][0]/255
            for time in (0,1800,3600,7200):
                update(time,level,1)
                self.assertAlmostEqual(node.SetSkyValue[1][1],night)
            update(7300,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day)
            update(8200,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],night)
            update(9200,level,0)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day)

    def test_existing_sun_moon_texture_and_restore(self):
        for level in LEVELS:
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            lua.globals().updating=True
            profile=profiles()[level]
            sequence=((0,night_mode(level),'Moon'),(1,0,'Sun'))
            if level not in ALTERNATE_LEVELS:sequence+=((2,2,'Sun'),(902,2,'Moon'),(1802,2,'Sun'))
            for time,mode,texture in sequence:
                update(time,level,mode)
                for name,values in profile['suns'].items():
                    mesh=lua.globals().Names[name]
                    self.assertEqual(mesh.SetShaderTextureParamValue[3],texture)
                    expected=values['night' if texture=='Moon' else 'day']['alpha']
                    self.assertAlmostEqual(mesh.SetShaderParamfValue[2],expected)
            self.assertEqual(lua.globals().allocations,0)

    def test_stars_follow_moon_and_clear_before_sun_returns(self):
        for level in LEVELS:
            if level in ALTERNATE_LEVELS:continue
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            for time in (0,225,450,675,900,1125,1350,1575,1800):
                update(time,level,2)
                phase=(time%1800)/900
                amount=phase if phase<=1 else 2-phase
                amount=amount*amount*(3-2*amount)
                stars=max(0,2*amount-1)
                effects=lua.globals()['__JNETDayNight'].effects
                for emitter in lua.globals().emitters.values():
                    if effects[emitter.name] is not None:
                        self.assertAlmostEqual(emitter.intensityValue[1],lua.globals().emitterData[emitter.name].intensity*stars)
                        effect=lua.globals().Names[effects[emitter.name].name]
                        self.assertEqual(effect.SetEnabledValue[1],stars>0)
                        self.assertTrue(effect.killParticlesOnDisableValue[1])
                for name,values in profiles()[level]['suns'].items():
                    mesh=lua.globals().Names[name]
                    if stars>0:
                        self.assertEqual(mesh.SetShaderTextureParamValue[3],'Moon')
                        self.assertAlmostEqual(mesh.shaderValues.alpha,stars*values['night']['alpha'])

    def test_credits_effect_colors_and_day_restoration(self):
        data=profiles()
        for level in LEVELS:
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            for time,mode,amount in ((0,1,1),(1,0,0),(2,2,0),(452,2,.5),(902,2,1),(1802,2,0)):
                update(time,level,mode)
                applied=[active(level,mode)]
                if mode==0 and 'alternate' in data[level]:applied.append(data[level]['alternate'])
                for profile in applied:
                  for name,values in profile['clouds'].items():
                    actual=lua.globals().Names[name].shaderValues
                    for param,value in values.items():
                        a,b=value['day'],value['night']
                        if isinstance(a,(tuple,list)):
                            for axis in range(len(a)):
                                self.assertAlmostEqual(actual[param][axis+1],a[axis]+(b[axis]-a[axis])*amount)
                        else:self.assertAlmostEqual(actual[param],a+(b-a)*amount)
            self.assertEqual(lua.globals().allocations,0)

    def test_dust_and_cloud_source_selection(self):
        data=profiles()
        chris=decorations('Level_Chris')
        from daynight_resources import params
        for level,source_id in ((0,'6e5a5e5a490b01694b0052861aee7963'),(1,'05bd9660cbbdc3ee2c15a3c968fcaa23')):
            expected=params(chris[source_id])['mltColor']
            for name,row in decorations('Level_'+MAPPINGS[level][0]).items():
                if row['Mesh']=='Level3_M_GraveyardDust':
                    self.assertEqual(data[level]['clouds'][name]['mltColor']['night'],expected)
                if row['Mesh'] in ('P_Level1MountainClouds','P_Level1MountainClouds2'):
                    self.assertEqual(data[level]['clouds'][name]['mltColor']['night'],params(chris['b612c4987ff83c47a0cc7b1efb845c0a'])['mltColor'])
        strip=[r for r in chris.values() if r['Mesh']=='P_CloudStripDesert_A']
        self.assertEqual(len(strip),1)
        for name,row in decorations('Level_Desert').items():
            if row['Mesh']=='P_CloudStripDesert_A':
                values=data[2]['clouds'][name]
                self.assertEqual(values['mltColor']['night'],params(strip[0])['mltColor'])

    def test_night_dust_fades_out_and_day_restores_opacity(self):
        from daynight_resources import params
        data=profiles()
        for level,meshes,count in ((0,('P_FlatMist','P_DomeMist','Level3_M_GraveyardDust','P_MistCardSphere'),33),
                                  (1,('Level3_M_GraveyardDust','P_MistCardSphere'),63)):
            rows={k:r for k,r in decorations('Level_'+MAPPINGS[level][0]).items() if r['Mesh'] in meshes}
            self.assertEqual(len(rows),count)
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            for time,mode,opacity in ((0,1,0),(1,0,1),(2,2,1),(452,2,.5),(902,2,0),(1352,2,.5),(1802,2,1)):
                update(time,level,mode)
                for name,row in rows.items():
                    actual=lua.globals().Names[name].shaderValues
                    self.assertAlmostEqual(actual['alpha'],params(row)['alpha']*opacity)
                    if opacity==1 and 'mltColor' in data[level]['clouds'][name]:
                        for axis,value in enumerate(params(row)['mltColor'],1):
                            self.assertAlmostEqual(actual['mltColor'][axis],value)

    def test_graveyard_rays_are_cool_at_night(self):
        from daynight_resources import params
        data=profiles()[0]['clouds']
        rows={k:r for k,r in decorations('Level_Graveyard').items()
              if r['Mesh'] in ('P_ShadowPlane','P_GraveyardGateRay','P_GraveyardGateSphinxRay')}
        self.assertEqual(len(rows),9)
        for name,row in rows.items():
            value=data[name]['mltColor']
            self.assertEqual(value['day'],params(row)['mltColor'])
            self.assertLess(value['night'][0],value['night'][1])
            self.assertLess(value['night'][1],value['night'][2])
            self.assertLessEqual(max(value['night'][:3]),max(value['day'][:3]))

    def test_scene_reload_discards_old_handles(self):
        lua=self.scene(0)
        lua.globals()['__JNETDayNightUpdate'](0,0,2)
        lua.globals()['__JNETDayNightUpdate'](900,0,2)
        lua.execute('generation=generation+1; for _,e in ipairs(emitters) do e.generation=generation end')
        self.load(lua,1)
        lua.globals()['__JNETDayNightUpdate'](2000,1,2)
        self.assertEqual(lua.globals().visits,2)
        self.assertEqual(lua.globals()['__JNETDayNight'].started,2000)

    def test_all_level_reload_pairs(self):
        for source in LEVELS:
            for destination in LEVELS:
                lua=self.scene(source)
                update=lua.globals()['__JNETDayNightUpdate']
                update(0,source,1)
                lua.execute('generation=generation+1; for _,e in ipairs(emitters) do e.generation=generation end')
                self.load(lua,destination)
                update(3600,destination,2)
                update(4500,destination,2)
                update(5400,destination,2)
                update(7300,destination,0)
                for e in lua.globals().emitters.values():
                    value=lua.eval('rawget')(e,'intensityValue')
                    if value is not None:
                        self.assertAlmostEqual(value[1],lua.globals().emitterData[e.name].intensity)

    def test_unsupported_level_does_not_allocate(self):
        lua=self.scene(0)
        for level in range(len(MAPPINGS),12):lua.globals()['__JNETDayNightUpdate'](1800,level,2)
        self.assertEqual(lua.globals().allocations,0)

    def test_star_loading_is_scoped_to_supported_scenes(self):
        lua=self.scene(0)
        for level in ('Graveyard','Barrens','Desert','Canyon','Cave','Ruins','Mountain','Summit','Credits','Bryan','Matt','Chris'):
            nodes=list(records('Level_'+level,'EnvNode').values())
            emitters=list(records('Level_'+level,'ParticleEmitter').values())
            lua.globals().updating=False
            lua.execute('Names={} EnvNodeInstances='+lua_literal(nodes)+';ParticleEmitterInstances='+lua_literal(emitters))
            lua.execute('allocations=0 LoadEnvNodes({}) LoadEnvFXParticles(fx,particles)')
            added=0 if level in ('Graveyard','Credits','Bryan','Matt','Chris') else 4
            self.assertEqual(lua.globals().allocations,len(emitters)+added,level)

    def test_miniature_horizontal_landmark_mapping(self):
        landmarks=[('P_GraveyardGate','P_GraveyardGateGrate_A_OIC'),
                   ('P_RockBackWallTop','P_AquaductCrateAdd1')]
        for (target,source,offset,_),names in zip(MAPPINGS,landmarks):
            self.assertIn(target,MINIATURES)
            a=list(decorations('Level_'+target).values())
            b=list(decorations('Level_'+source).values())
            for name in names:
                left=[r for r in a if r['Mesh']==name]
                right=[r for r in b if r['Mesh']==name]
                self.assertEqual((len(left),len(right)),(1,1))
                for axis in (0,2):
                    self.assertAlmostEqual(left[0]['Transformation'][12+axis]*.5+offset[axis],
                                           right[0]['Transformation'][12+axis],delta=.002)

    def test_sources_and_resident_moon_texture(self):
        image=(LAB.parent.parent/'JourneyPC/Journey.exe').read_bytes()
        self.assertIn(b'"Texture" "Moon"',image)
        script=(LAB/'diagnostic/jnet_daynight.lua').read_text()
        for forbidden in ('AddDecoration','MetaAddEvent','CreateClump','SetMatrix'):
            self.assertNotIn(forbidden,script)
        for i,(_,source,_,anchors) in enumerate(MAPPINGS):
            for anchor in anchors:self.assertIn(anchor,records('Level_'+source,'EnvNode'))
            self.assertEqual(source,'Chris')
            self.assertEqual(len(profiles()[i]['emitters']),4)
            self.assertTrue(profiles()[i]['moon'])

    def test_every_world_select_level_uses_the_chris_night(self):
        self.assertEqual([m[0] for m in MAPPINGS],['Graveyard','Barrens','Desert','Canyon','Cave','Ruins','Mountain','Summit'])
        self.assertEqual({m[1] for m in MAPPINGS},{'Chris'})
        travel=(LAB/'diagnostic/jnet_travel_live.h').read_text()
        names=travel.split('jnet_travel_levels[]={')[1].split('};')[0].replace('"','').replace(' ','').replace(chr(10),'').replace(chr(13),'').split(',')
        self.assertEqual(['Level_'+m[0] for m in MAPPINGS],names[:len(MAPPINGS)])

    def test_palette_levels_copy_chris_night_and_never_exceed_day(self):
        chris=records('Level_Chris','EnvNode')
        anchor=chris['b4ad6f584115d4b81fb5b91c022aa353']
        data=profiles()
        for level,(target,_,_,_) in enumerate(MAPPINGS):
            if target in MINIATURES:continue
            day=records('Level_'+target,'EnvNode')
            self.assertEqual(set(data[level]['nodes']),set(day),target)
            for key,night in data[level]['nodes'].items():
                for field in ('Env_LightDir','FogFarAlpha','BloomIntensity','VelvetInt','Env_SpecularPower','Env_AmbientColor'):
                    self.assertEqual(night[field],anchor[field],(target,field))
                for field in ('FogCloseColor','FogFarColor','SkyMinColor','SkyLowColor','SkyMidColor','SkyHighColor','SkyMaxColor'):
                    self.assertLessEqual(luminance(night[field]),luminance(day[key][field])+1e-9,(target,key,field))
                    self.assertLessEqual(luminance(night[field]),luminance(anchor[field])+1e-9,(target,key,field))
                for color,intensity in (('Env_AmbientColor','Env_AmbientInt'),('Env_LightColor','Env_LightInt'),('Env_SpecularColor','Env_SpecularInt')):
                    self.assertLessEqual(luminance(night[color])*night[intensity],luminance(day[key][color])*day[key][intensity]+1e-9,(target,key,color))
                    self.assertLessEqual(night[intensity],anchor[intensity]+1e-9)

    def test_hidden_summit_sun_stays_hidden(self):
        suns=profiles()[7]['suns']
        self.assertIn('c3786080cece66f6a8b5eb53c8de6e84',suns)
        self.assertNotIn('b8a81a650dfb58459c0c41a2d39754b3',suns)
        for level,data in enumerate(profiles()):
            for values in data['suns'].values():self.assertGreater(values['day']['alpha'],0,level)

    def test_unmatched_clouds_get_a_dim_cool_tint_and_lightning_is_untouched(self):
        for level,(target,_,_,_) in enumerate(MAPPINGS):
            if target in MINIATURES:continue
            clouds=profiles()[level]['clouds']
            tinted=0
            for name,row in decorations('Level_'+target).items():
                if 'lightning' in row['Mesh'].lower():
                    self.assertNotIn(name,clouds,row['Mesh'])
                if name not in clouds or set(clouds[name])!={'mltColor'}:continue
                value=clouds[name]['mltColor']
                self.assertEqual(value['day'],params(row)['mltColor'])
                night=value['night']
                self.assertEqual(night[3],value['day'][3])
                self.assertLessEqual(luminance(night),luminance(value['day'])+1e-9)
                self.assertLessEqual(luminance(night),0.4667)
                self.assertLessEqual(night[0],night[1]);self.assertLessEqual(night[1],night[2])
                tinted+=1
            self.assertGreater(tinted,0,target)

    def test_dusk_and_dawn_are_preserved_beside_night(self):
        self.assertEqual(set(ALTERNATES),{'Desert','Canyon'})
        for level in ALTERNATE_LEVELS:
            target=MAPPINGS[level][0]
            main,alternate=profiles()[level],profiles()[level]['alternate']
            self.assertEqual(ALTERNATES[target][0],'Matt')
            self.assertEqual(set(main['nodes']),set(alternate['nodes']))
            self.assertEqual(main['nodeCount'],alternate['nodeCount'])
            self.assertTrue(main['moon'])
            self.assertFalse(alternate['moon'])
            self.assertEqual(alternate['emitters'],[])
            for values in alternate['suns'].values():self.assertEqual(values['night'],values['day'])
            self.assertNotEqual(main['nodes'],alternate['nodes'])
            matt=records('Level_Matt','EnvNode')
            for anchor in ALTERNATES[target][2]:self.assertIn(anchor,matt)

    def test_night_is_a_fourth_mode_that_switches_cleanly_with_dusk_and_dawn(self):
        for level in ALTERNATE_LEVELS:
            main,alternate=profiles()[level],profiles()[level]['alternate']
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            key=next(iter(main['nodes']))
            node=lua.globals().Names[key]
            day=records('Level_'+MAPPINGS[level][0],'EnvNode')[key]['SkyMaxColor'][0]/255
            sky=lambda profile:profile['nodes'][key]['SkyMaxColor'][0]/255
            self.assertNotAlmostEqual(sky(main),sky(alternate))
            effects=lua.globals()['__JNETDayNight']
            for time,mode,expected,stars in ((0,1,sky(alternate),False),(1,3,sky(main),True),(2,1,sky(alternate),False),
                                             (3,3,sky(main),True),(4,0,day,False)):
                update(time,level,mode)
                self.assertAlmostEqual(node.SetSkyValue[1][1],expected,msg=(level,mode))
                for row in effects.effects.values():
                    effect=lua.globals().Names[row.name]
                    self.assertEqual(effect.SetEnabledValue[1],stars,(level,mode))
                if mode==3:
                    for name,values in main['suns'].items():
                        self.assertEqual(lua.globals().Names[name].SetShaderTextureParamValue[3],'Moon')
                if mode==1 and time>0:
                    for name,values in main['suns'].items():
                        self.assertEqual(lua.globals().Names[name].SetShaderTextureParamValue[3],'Sun')
                    for name,values in main['clouds'].items():
                        if name in alternate['clouds']:continue
                        shader=lua.globals().Names[name].shaderValues
                        for param,value in values.items():
                            a=value['day']
                            if isinstance(a,(tuple,list)):
                                for axis in range(len(a)):self.assertAlmostEqual(shader[param][axis+1],a[axis])
                            else:self.assertAlmostEqual(shader[param],a)
            self.assertEqual(lua.globals().allocations,0)
            update(5,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day)
            update(455,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],(day+sky(alternate))/2)
            update(905,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],sky(alternate))

    def test_update_does_nothing_until_the_level_data_has_been_loaded(self):
        lua=self.scene(4)
        self.load(lua,4,parts=False)
        lua.globals().allocations=0
        update=lua.globals()['__JNETDayNightUpdate']
        key=next(iter(profiles()[4]['nodes']))
        node=lua.globals().Names[key]
        for mode in (0,1,2,3):update(100,4,mode)
        self.assertIsNone(lua.eval('rawget')(node,'SetSkyValue'))
        self.assertFalse(lua.globals()['__JNETDayNight'].active)
        self.load_parts(lua,4)
        update(200,4,1)
        self.assertAlmostEqual(node.SetSkyValue[1][1],profiles()[4]['nodes'][key]['SkyMaxColor'][0]/255)
        self.assertEqual(lua.globals().allocations,0)

    def test_level_parts_rebuild_each_profile_exactly_and_stay_small(self):
        built=payload()
        self.assertEqual(len(built['parts']),len(MAPPINGS))
        self.assertLess(len(built['source']),24000)
        for level,parts in enumerate(built['parts']):
            self.assertTrue(parts)
            for part in parts:self.assertLessEqual(len(part),PART_LIMIT+1600,(level,len(part)))
            lua=self.scene(level)
            data=lua.globals()['__JNETDayNight'].data
            others=[i for i in range(len(MAPPINGS)) if i!=level and data[i+1] is not None]
            self.assertEqual(others,[],'only the loaded level may have data')
            for name,source in (('main',profiles()[level]),('alternate',profiles()[level].get('alternate'))):
                loaded=data[level+1] if name=='main' else data[level+1].alternate
                if source is None:
                    self.assertIsNone(loaded);continue
                self.assertEqual(set(loaded.nodes.keys()),set(source['nodes']))
                self.assertEqual(set(loaded.suns.keys()),set(source['suns']))
                clouds={}
                for part in loaded.clouds.values():clouds.update(dict(part.items()))
                self.assertEqual(set(clouds),set(source['clouds']))
                self.assertEqual(bool(loaded.moon),source['moon'])
                self.assertEqual(len(loaded.emitters),len(source['emitters']))
                key=next(iter(source['nodes']))
                self.assertAlmostEqual(loaded.nodes[key].SkyMaxColor[1],source['nodes'][key]['SkyMaxColor'][0])
                for name_,values in list(source['clouds'].items())[:5]:
                    for param,value in values.items():
                        night=clouds[name_][param].night
                        if isinstance(value['night'],(tuple,list)):self.assertAlmostEqual(night[1],value['night'][0])
                        else:self.assertAlmostEqual(night,value['night'])

if __name__=='__main__':unittest.main()
