import unittest
from pathlib import Path
from functools import cache
from lupa import LuaRuntime
from character_resources import lua_literal, decorations
from daynight_resources import profiles, records, MAPPINGS, runtime_source, profile_parts

profiles=cache(profiles)
profile_parts=cache(profile_parts)
runtime_source=cache(runtime_source)
decorations=cache(decorations)
records=cache(records)
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
        lua.execute(runtime_source(),lua.globals().visit)
        self.load(lua,level)
        lua.globals().allocations=0
        lua.globals().updating=True
        return lua

    def load(self,lua,level):
        lua.globals().updating=False
        name='Level_'+MAPPINGS[level][0]
        lua.execute('EnvNodeInstances='+lua_literal(list(records(name,'EnvNode').values()))+';ParticleEmitterInstances='+lua_literal(list(records(name,'ParticleEmitter').values())))
        lua.execute('Names={} LoadEnvNodes({}) LoadEnvFXParticles(fx,particles)')
        for key in decorations(name): lua.globals().Names[key]=lua.globals().object()
        for part in profile_parts()[level]: lua.execute(part)
        lua.execute('__JNETDayNight.ready=true')
        lua.globals().updating=True

    def test_native_day_keeps_ending_stars(self):
        level=next(i for i,mapping in enumerate(MAPPINGS) if mapping[0]=='Graveyard')
        lua=self.scene(level)
        update=lua.globals()['__JNETDayNightUpdate']
        effects=lua.globals()['__JNETDayNight'].effects
        for emitter in lua.globals().emitters.values():
            if effects[emitter.name]:
                emitter.intensity(emitter,.75)
                effect=lua.globals().Names[effects[emitter.name].name]
                effect.SetEnabled(effect,True)
        for time in (0,1,10):
            update(time,level,0)
        for emitter in lua.globals().emitters.values():
            if effects[emitter.name]:
                self.assertEqual(emitter.intensityValue[1],.75)
                self.assertTrue(lua.globals().Names[effects[emitter.name].name].SetEnabledValue[1])
        update(0,level,1)
        update(0,level,0)
        for emitter in lua.globals().emitters.values():
            row=effects[emitter.name]
            if row:
                self.assertEqual(emitter.intensityValue[1],lua.globals().emitterData[emitter.name].intensity)
                effect=lua.globals().Names[row.name]
                self.assertEqual(effect.SetEnabledValue[1],row.enabled)
                effect.SetEnabled(effect,True)
        update(1,level,0)
        for row in effects.values():
            self.assertTrue(lua.globals().Names[row.name].SetEnabledValue[1])

    def test_update_before_particle_initialization(self):
        lua=self.scene(0)
        lua.execute('__JNETDayNight.effects=nil')
        lua.globals()['__JNETDayNightUpdate'](0,0,1)
        self.assertEqual(lua.globals().allocations,0)

    def test_full_cycle_and_restore(self):
        for level in range(8):
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            key=next(iter(profiles()[level]['nodes']))
            day=records('Level_'+MAPPINGS[level][0],'EnvNode')[key]
            node=lua.globals().Names[key]
            update(0,level,0)
            self.assertEqual(lua.globals().allocations,0)
            for time,amount in [(0,0),(450,.5),(900,1),(1350,.5),(1800,0),(2700,1),(3600,0)]:
                update(time,level,2)
                night=profiles()[level]['nodes'][key]['SkyMaxColor'][0]
                actual=node.SetSkyValue[1][1]
                self.assertAlmostEqual(actual,(day['SkyMaxColor'][0]*(1-amount)+night*amount)/255)
            count=lua.globals().allocations
            update(4000,level,0)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day['SkyMaxColor'][0]/255)
            update(5000,level,2)
            self.assertEqual(lua.globals().allocations,count)
            self.assertIsNone(lua.eval('rawget')(node,'SetEnabledValue'))

    def test_fixed_night_and_cycle_switch(self):
        for level in range(8):
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            key=next(iter(profiles()[level]['nodes']))
            node=lua.globals().Names[key]
            night=profiles()[level]['nodes'][key]['SkyMaxColor'][0]/255
            day=records('Level_'+MAPPINGS[level][0],'EnvNode')[key]['SkyMaxColor'][0]/255
            for time in (0,1800,3600,7200):
                update(time,level,1)
                self.assertAlmostEqual(node.SetSkyValue[1][1],night)
            update(0,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day)
            update(900,level,2)
            self.assertAlmostEqual(node.SetSkyValue[1][1],night)
            update(9200,level,0)
            self.assertAlmostEqual(node.SetSkyValue[1][1],day)

    def test_existing_sun_moon_texture_and_restore(self):
        for level in (0,1):
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            lua.globals().updating=True
            profile=profiles()[level]
            for time,mode,texture in ((0,1,'Moon'),(1,0,'Sun'),(0,2,'Sun'),(900,2,'Moon'),(1800,2,'Sun')):
                update(time,level,mode)
                for name,values in profile['suns'].items():
                    mesh=lua.globals().Names[name]
                    self.assertEqual(mesh.SetShaderTextureParamValue[3],texture)
                    expected=values['night' if texture=='Moon' else 'day']['alpha']
                    self.assertAlmostEqual(mesh.SetShaderParamfValue[2],expected)
            self.assertEqual(lua.globals().allocations,0)

    def test_stars_follow_moon_and_clear_before_sun_returns(self):
        for level in range(8):
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
                    if stars>0 and not values.get('separate'):
                        self.assertEqual(mesh.SetShaderTextureParamValue[3],'Moon')
                        self.assertAlmostEqual(mesh.shaderValues.alpha,stars*values['night']['alpha'])

    def test_credits_effect_colors_and_day_restoration(self):
        data=profiles()
        for level in range(8):
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            for time,mode,amount in ((0,1,1),(1,0,0),(0,2,0),(450,2,.5),(900,2,1),(1800,2,0)):
                update(time,level,mode)
                for name,values in data[level]['clouds'].items():
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
                    self.assertEqual(data[level]['clouds'][name]['mltColor']['night'],list(expected))
                if row['Mesh'] in ('P_Level1MountainClouds','P_Level1MountainClouds2'):
                    self.assertEqual(data[level]['clouds'][name]['mltColor']['night'],list(params(chris['b612c4987ff83c47a0cc7b1efb845c0a'])['mltColor']))
        for name,row in decorations('Level_Desert').items():
            if row['Mesh']=='P_CloudStripDesert_A':
                values=data[2]['alternate']['clouds'][name]
                self.assertEqual(values['mltColor']['night'],list(params(row)['mltColor']))

    def test_night_dust_fades_out_and_day_restores_opacity(self):
        from daynight_resources import params
        data=profiles()
        for level,meshes,count in ((0,('P_FlatMist','P_DomeMist','Level3_M_GraveyardDust','P_MistCardSphere'),33),
                                  (1,('Level3_M_GraveyardDust','P_MistCardSphere'),63)):
            rows={k:r for k,r in decorations('Level_'+MAPPINGS[level][0]).items() if r['Mesh'] in meshes}
            self.assertEqual(len(rows),count)
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            for time,mode,opacity in ((0,1,0),(1,0,1),(0,2,1),(450,2,.5),(900,2,0),(1350,2,.5),(1800,2,1)):
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
            self.assertEqual(value['day'],list(params(row)['mltColor']))
            self.assertLess(value['night'][0],value['night'][1])
            self.assertLess(value['night'][1],value['night'][2])
            self.assertLessEqual(max(value['night'][:3]),max(value['day'][:3]))

    def test_scene_reload_discards_old_handles(self):
        lua=self.scene(0)
        lua.globals()['__JNETDayNightUpdate'](0,0,2)
        lua.globals()['__JNETDayNightUpdate'](900,0,2)
        lua.execute('generation=generation+1; for _,e in ipairs(emitters) do e.generation=generation end')
        self.load(lua,1)
        lua.globals()['__JNETDayNightUpdate'](900,1,2)
        self.assertEqual(lua.globals().visits,2)
        key=next(iter(profiles()[1]['nodes']))
        self.assertAlmostEqual(lua.globals().Names[key].SetSkyValue[1][1],profiles()[1]['nodes'][key]['SkyMaxColor'][0]/255)

    def test_all_level_reload_pairs(self):
        for source in range(8):
            for destination in range(8):
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

    def test_partial_data_is_not_applied(self):
        lua=self.scene(2)
        self.load(lua,2)
        lua.execute('__JNETDayNight.ready=false')
        lua.globals()['__JNETDayNightUpdate'](900,2,2)
        key=next(iter(profiles()[2]['nodes']))
        self.assertIsNone(lua.eval('rawget')(lua.globals().Names[key],'SetSkyValue'))
        lua.execute('__JNETDayNight.ready=true')
        lua.globals()['__JNETDayNightUpdate'](900,2,2)
        self.assertAlmostEqual(lua.globals().Names[key].SetSkyValue[1][1],profiles()[2]['nodes'][key]['SkyMaxColor'][0]/255)

    def test_dusk_dawn_are_separate_from_cycle(self):
        for level in (2,3):
            lua=self.scene(level)
            update=lua.globals()['__JNETDayNightUpdate']
            key=next(k for k in profiles()[level]['nodes'] if profiles()[level]['nodes'][k]['SkyMaxColor']!=profiles()[level]['alternate']['nodes'][k]['SkyMaxColor'])
            node=lua.globals().Names[key]
            for mode,variant in ((3,'alternate'),(1,None),(3,'alternate'),(2,None)):
                update(900,level,mode)
                data=profiles()[level]
                if variant: data=data[variant]
                self.assertAlmostEqual(node.SetSkyValue[1][1],data['nodes'][key]['SkyMaxColor'][0]/255)

    def test_paradise_independent_sun_moon(self):
        lua=self.scene(7)
        update=lua.globals()['__JNETDayNightUpdate']
        for time in (0,225,450,675,900,1125,1350,1575,1800):
            update(time,7,2)
            phase=(time%1800)/900
            amount=phase if phase<=1 else 2-phase
            amount=amount*amount*(3-2*amount)
            for key,values in profiles()[7]['suns'].items():
                mesh=lua.globals().Names[key]
                factor=max(0,2*amount-1) if values['separate']=='Moon' else max(0,1-2*amount)
                self.assertAlmostEqual(mesh.shaderValues.alpha,values['day']['alpha']*factor)
                self.assertIsNone(lua.eval('rawget')(mesh,'SetShaderTextureParamValue'))
        self.assertEqual(lua.globals().allocations,0)

    def test_summit_selected_shadow_planes(self):
        selected = {'c8a3854cb941f5b7bd82df63e0142f5a',
                    'f281fa80e26c5507b8be57bb6c447200',
                    '5fd6b7b0c8eef88daaeda898ce399893',
                    '570c221cd2f9048548590784dd5b92ff',
                    'd0bcda556118028a6ff1dd52de6ea5b1',
                    '34b9219071a7c1eaa85c2bae204f2e98',
                    '9467edcaf435a69e22e2d37d2a189076'}
        profile = profiles()[7]
        shadows = {key for key, row in decorations('Level_Summit').items()
                   if row['Mesh'] == 'P_ShadowPlane'}
        self.assertEqual(shadows.intersection(profile['suns']), selected)
        lua = self.scene(7)
        for mode, factor in ((1, 0), (0, 1)):
            lua.globals()['__JNETDayNightUpdate'](0, 7, mode)
            for key in selected:
                self.assertAlmostEqual(lua.globals().Names[key].shaderValues.alpha,
                                       profile['suns'][key]['day']['alpha'] * factor)

    def test_unsupported_level_does_not_allocate(self):
        lua=self.scene(0)
        for level in range(8,12):lua.globals()['__JNETDayNightUpdate'](1800,level,2)
        self.assertEqual(lua.globals().allocations,0)

    def test_star_loading_is_scoped_to_supported_scenes(self):
        lua=self.scene(0)
        for level in ('Graveyard','Barrens','Desert','Canyon','Cave','Ruins','Mountain','Summit','Credits','Bryan','Matt','Chris'):
            nodes=list(records('Level_'+level,'EnvNode').values())
            emitters=list(records('Level_'+level,'ParticleEmitter').values())
            lua.globals().updating=False
            lua.execute('Names={} EnvNodeInstances='+lua_literal(nodes)+';ParticleEmitterInstances='+lua_literal(emitters))
            lua.execute('allocations=0 LoadEnvNodes({}) LoadEnvFXParticles(fx,particles)')
            self.assertEqual(lua.globals().allocations,len(emitters)+(sum(e['EmitterType'] not in {r['EmitterType'] for r in emitters} for e in profiles()[[m[0] for m in MAPPINGS].index(level)]['emitters']) if level in [m[0] for m in MAPPINGS] else 0),level)

    def test_miniature_horizontal_landmark_mapping(self):
        landmarks=[('P_GraveyardGate','P_GraveyardGateGrate_A_OIC'),
                   ('P_RockBackWallTop','P_AquaductCrateAdd1'),
                   ('P_DesertFinalTower','P_DesertFinalTowerGrate'),
                   ('P_CaveEntranceFacade','P_RuinFence')]
        for (target,source,offset,_),names in zip(MAPPINGS,landmarks):
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
            self.assertEqual(len(profiles()[i]['emitters']),4)
            if i<2:self.assertTrue(profiles()[i]['moon'])

if __name__=='__main__':unittest.main()
