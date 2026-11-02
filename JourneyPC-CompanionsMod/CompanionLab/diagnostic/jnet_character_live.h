#ifndef JNET_CHARACTER_LIVE_H
#define JNET_CHARACTER_LIVE_H
#include "jnet_character_native.h"
#include "jnet_remote_pose.h"
#include "jnet_character_audio.h"
#include "jnet_level_fixes.h"
#include "jnet_character_payloads.h"
struct jnet_character_state {
    struct jnet_character_binding binding;
    struct jnet_character_pools pools;
    uint64_t game,barn;
    unsigned kind,initialized,failed,owner,skin;
    struct jnet_character_control control;
    const char *failure_stage;
    int failure_detail;
    uint64_t visual_started,activation_started;
    unsigned activation_reported;
    unsigned visual_samples,input_frames,follow_frames;
    uint64_t teleport_event,eye_event,ride_event,grab_event,ride_peer,ride_epoch,ride_tick,ride_active;
    uint64_t attack_event,attack_handler,attack_report_tick;
    unsigned attack_busy,grab_blocked;
    uint64_t attack_target,attack_target_lease,attack_actor;
    struct jnet_war_hit war_hits[2];float remote_anger;
    struct jnet_remote_pose remote_pose;
    uint32_t teleport_identity;
    unsigned eye_ready,eye_warned,chirp_variant,variant,variant_applied;
    float saved_dissolve[2],motion[3],follow_position[3],sprint;
    unsigned follow_position_valid;
    uint32_t motion_frame;
    unsigned motion_frame_valid;
    unsigned char saved_visibility[2];
};
static struct jnet_character_state jnet_character_instances[3];
static unsigned jnet_character_owner;
#define jnet_character_instance jnet_character_instances[jnet_character_owner]
static uint64_t jnet_character_generation=1,jnet_character_lua_generation;
static struct jnet_character_state jnet_character_caches[3][JNET_CHARACTER_COUNT*3];
#define jnet_character_cached jnet_character_caches[jnet_character_owner]
static unsigned jnet_character_failed_masks[3];
#define jnet_character_failed_mask jnet_character_failed_masks[jnet_character_owner]
static uint64_t jnet_character_retry_at;
static uintptr_t jnet_character_decorations,jnet_character_level_decorations,jnet_character_pool_game;
static unsigned jnet_character_pool_waiting;
static int jnet_character_pool_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    struct jnet_character_state *instance=ctx;
    if(instance && address==instance->game+0x138 && bytes==8 && jnet_character_decorations){
        memcpy(out,&jnet_character_decorations,8);return 1;
    }
    return own_read(NULL,address,out,bytes);
}
static int jnet_character_private_pool(uintptr_t game) {
    uintptr_t level=0,type=0;
    if(!own_read(NULL,game+0x138,&level,8) || !level ||
       !own_read(NULL,level,&type,8) || type!=fault_image+0x5cef40)return 0;
    if(jnet_character_decorations && jnet_character_pool_game!=game)return 0;
    if(!jnet_character_decorations){
        uintptr_t (*create)(void)=(void *)(fault_image+0x9fc20);
        jnet_character_decorations=create();
        if(!jnet_character_decorations)return 0;
    }
    jnet_character_level_decorations=level;jnet_character_pool_game=game;
    return own_read(NULL,jnet_character_decorations,&type,8) && type==fault_image+0x5cef40;
}
static unsigned char (*jnet_character_bank_original)(uintptr_t,const char *);
static unsigned char jnet_character_bank(uintptr_t bank,const char *path) {
    char mapped[1024];
    const char *selected=path;
    if(jnet_character_bank_path(path,mapped,sizeof(mapped)))selected=mapped;
    unsigned char result=jnet_character_bank_original(bank,selected);
    if(selected!=path){
        char line[1200];snprintf(line,sizeof(line),"[JNET] creature sound bank %s loaded=%u",selected,result);
        message(line);
    }
    return result;
}
static void (*jnet_character_mesh_update_original)(uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static void jnet_character_mesh_update(uintptr_t mesh,float dt,uintptr_t camera,uintptr_t world,uintptr_t unused,uintptr_t lua) {
    jnet_bryan_observe(mesh);
    if(jnet_character_decorations && mesh>=jnet_character_decorations+0x10 &&
       mesh<jnet_character_decorations+0x434010 &&
       (mesh-jnet_character_decorations-0x10)%0x10d0==0 &&
       !jnet_character_textures_ready(own_read,NULL,mesh)){
        uint32_t hidden=0;jc_local_windows_write(mesh+0x6e4,&hidden,4);
        if(!jnet_character_pool_waiting)message("[JNET] character mesh waiting for native texture payload");
        jnet_character_pool_waiting=1;return;
    }
    jnet_character_mesh_update_original(mesh,dt,camera,world,unused,lua);
}
static void (*jnet_character_shout_init_original)(uintptr_t,uintptr_t,uintptr_t);
static void jnet_character_shout_init(uintptr_t shouts,uintptr_t resources,uintptr_t pool) {
    uintptr_t game=(uintptr_t)JNETGame,expected=0,manager=0;
    uint32_t count=1024;
    if(game && own_read(NULL,game+0x1b8,&expected,8) && expected==shouts &&
       own_read(NULL,game+0x28,&manager,8) && manager==resources &&
       jnet_character_private_pool(game) && pool==jnet_character_level_decorations &&
       own_read(NULL,jnet_character_decorations+0x434010,&count,4) && count<=1008){
        jnet_character_shout_init_original(shouts,resources,jnet_character_decorations);
        message("[JNET] native chirp visuals allocated in private decoration pool");return;
    }
    jnet_character_shout_init_original(shouts,resources,pool);
}
static void (*jnet_character_decoration_update_original)(uintptr_t,unsigned,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static void jnet_character_before_cloth(float dt,uintptr_t cloth);
static void jnet_character_decoration_update(uintptr_t pool,unsigned group,float dt,uintptr_t resources,
        uintptr_t lua,uintptr_t camera,uintptr_t world,uintptr_t sounds) {
    jnet_character_decoration_update_original(pool,group,dt,resources,lua,camera,world,sounds);
    if(pool==jnet_character_level_decorations && jnet_character_decorations){
        if(group==0)jnet_character_before_cloth(dt,world);
        jnet_character_decoration_update_original(jnet_character_decorations,group,dt,resources,lua,camera,world,sounds);
    }
}
static void (*jnet_character_decoration_draw_original)(uintptr_t,uintptr_t);
static void jnet_character_decoration_draw(uintptr_t pool,uintptr_t renderer) {
    jnet_character_decoration_draw_original(pool,renderer);
    if(pool==jnet_character_level_decorations && jnet_character_decorations)
        jnet_character_decoration_draw_original(jnet_character_decorations,renderer);
}
static void (*jnet_character_decoration_cleanup_original)(uintptr_t);
static void jnet_character_decoration_cleanup(uintptr_t mesh) {
    if(mesh==jnet_bryan_mesh)jnet_bryan_mesh=0;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    if(caller==fault_image+0xd9e41)jnet_crouch_active=0;
    if(caller==fault_image+0xd9e41 && jnet_character_decorations && mesh==jnet_character_level_decorations+0x10){
        uint32_t count=0,zero=0;
        if(own_read(NULL,jnet_character_decorations+0x434010,&count,4) && count<=1024){
            memset(jnet_character_instances,0,sizeof(jnet_character_instances));
            memset(jnet_character_caches,0,sizeof(jnet_character_caches));
            void (*remove_render)(uintptr_t)=(void *)(fault_image+0x263090);
            void (*release_cloth)(uintptr_t)=(void *)(fault_image+0x87880);
            for(unsigned i=0;i<count;++i){
                uintptr_t item=jnet_character_decorations+0x10+(uintptr_t)i*0x10d0;
                remove_render(item+0x10);
                jnet_character_decoration_cleanup_original(item);
                release_cloth(item+0xf38);
            }
            jc_local_windows_write(jnet_character_decorations+0x434010,&zero,4);
            jnet_character_pool_waiting=0;
            if(count)message("[JNET] private character decorations released with native level cleanup");
        }
    }
    jnet_character_decoration_cleanup_original(mesh);
}

static int jnet_character_write_memory(void *ctx,uint64_t address,const void *data,size_t bytes) {
    (void)ctx;return jc_local_windows_write((uintptr_t)address,data,bytes);
}
static int jnet_character_resources_ready(void *ctx) {
    struct jnet_character_state *instance=ctx;
    const struct jnet_character_spec *spec=&jnet_character_specs[instance->kind];
    uintptr_t manager=0;
    if(!own_read(NULL,instance->game+0x28,&manager,8) || !manager)return 0;
    uintptr_t (*mesh_type)(void)=(void *)(fault_image+0x7a8f0);
    uintptr_t (*shader_type)(void)=(void *)(fault_image+0x267490);
    uintptr_t (*lookup)(uintptr_t,uintptr_t,const char *)=(void *)(fault_image+0x2ce120);
    uintptr_t types[2]={mesh_type(),shader_type()};
    if(!types[0] || !types[1])return 0;
    for(unsigned i=0;i<spec->mesh_count;++i){
        const char *names[2]={spec->meshes[i].mesh,spec->meshes[i].shader};
        for(unsigned k=0;k<2;++k){
            uintptr_t entry=lookup(manager,types[k],names[k]),payload=0;
            if(!entry || !own_read(NULL,entry+0x38,&payload,8) || !payload)return 0;
            if(k==0 && spec->meshes[i].visible && !jnet_character_mesh_ready(own_read,NULL,payload)){
                char line[200];
                snprintf(line,sizeof(line),"[JNET] character apply declined: %s has no initialized mesh instance",names[k]);
                message(line);return 0;
            }
        }
    }
    if(instance->kind==JNET_CHARACTER_WAR_MACHINE){
        uintptr_t entry=lookup(manager,types[1],"JNETCharacter_MeshSnowTopSide"),payload=0;
        if(!entry || !own_read(NULL,entry+0x38,&payload,8) || !payload)return 0;
    }
    return 1;
}
static uint64_t jnet_character_allocate_native(void *ctx,uint64_t barn) {
    (void)ctx;uintptr_t (*allocate)(uintptr_t)=(void *)(fault_image+0x9bea0);
    return allocate((uintptr_t)barn);
}
static int jnet_character_lua_arguments(lua_State *state) {
    uintptr_t objects[4]={jnet_character_instance.binding.creature,jnet_character_instance.pools.decorations,0,0};
    if(!own_read(NULL,jnet_character_instance.game+0x168,&objects[2],8) || !objects[2] ||
       !own_read(NULL,jnet_character_instance.game+0x258,&objects[3],8) || !objects[3])return 0;
    void (*push)(lua_State *,uintptr_t,uintptr_t)=(void *)(fault_image+0x2cb100);
    for(unsigned i=0;i<4;++i){
        uintptr_t table=0,entry=0;
        if(!own_read(NULL,objects[i],&table,8) || !table ||
           !own_read(NULL,table+8,&entry,8) || !entry)return 0;
        uintptr_t (*metadata)(uintptr_t)=(void *)entry;
        uintptr_t type=metadata(objects[i]);if(!type)return 0;
        push(state,objects[i],type);
    }
    return 4;
}
static int jnet_character_lua_chunk(lua_State *state,const char *source,int results) {
    return original_loadbuffer(state,source,strlen(source),"JNET-character","t")==0 &&
        pcallfn(state,0,results,0,0,NULL)==0;
}
static int jnet_character_lua_factory(lua_State *state,const struct jnet_character_spec *spec,const float position[3]) {
    int top=gettop(state),ok=0;char spawn[160];
    snprintf(spawn,sizeof(spawn),"return {%.9g,%.9g,%.9g}",(double)position[0],(double)position[1],(double)position[2]);
    if(jnet_character_lua_generation!=jnet_character_generation){
        if(!jnet_character_lua_chunk(state,"__JNETCharacterNative=nil; __JNETCharacterInstances={}",0))goto done;
        jnet_character_lua_generation=jnet_character_generation;
    } else if(!jnet_character_lua_chunk(state,"__JNETCharacterNative=nil",0))goto done;
    char owner[96];snprintf(owner,sizeof(owner),"__JNETCharacterOwner=%u; __JNETCharacterSkin=%u",jnet_character_owner,jnet_character_instance.skin);
    if(!jnet_character_lua_chunk(state,owner,0))goto done;
    if(original_loadbuffer(state,jnet_character_factory_source,sizeof(jnet_character_factory_source)-1,"JNET-character-factory","t"))goto done;
    pushclosure(state,jnet_character_lua_arguments,0);
    if(pcallfn(state,0,4,0,0,NULL) || !jnet_character_lua_chunk(state,spec->source,1) ||
       !jnet_character_lua_chunk(state,spawn,1))goto done;
    ok=pcallfn(state,6,0,0,0,NULL)==0;
 done:
    if(!ok){
        const char *error=tolstring(state,-1,NULL);char line[320];
        snprintf(line,sizeof(line),"[JNET] character factory failed: %.240s",error?error:"native argument unavailable");message(line);
    }
    settop(state,top);return ok;
}
static int jnet_character_hide_created(struct jnet_character_state *instance) {
    uint32_t count=0;unsigned char zero=0;int ok=1;
    if(!own_read(NULL,instance->pools.decorations+0x434010,&count,4) || count>1024 ||
       count<instance->pools.decoration_count)return 0;
    for(unsigned i=instance->pools.decoration_count;i<count;++i)
        if(!jc_local_windows_write(instance->pools.decorations+0x10+(uint64_t)i*0x10d0+0x70c,&zero,1))ok=0;
    if(!own_read(NULL,instance->pools.events+0x1008,&count,4) || count>512 || count<instance->pools.event_count)return 0;
    for(unsigned i=instance->pools.event_count;i<count;++i){
        uint64_t event=0;
        if(!own_read(NULL,instance->pools.events+8+(uint64_t)i*8,&event,8) || !event ||
           !jc_local_windows_write(event+9,&zero,1))ok=0;
    }
    return ok;
}
static inline int jnet_character_prepare_native(uintptr_t game,uintptr_t barn,unsigned kind,unsigned skin) {
    if(!jnet_character_is_creature(kind) || !(jnet_menu_supported_characters&(1u<<kind)) || jnet_character_instance.binding.creature ||
       jnet_character_instance.failed || (jnet_character_failed_mask&(1u<<kind)))return 0;
    const struct jnet_character_spec *spec=&jnet_character_specs[kind];
    if(kind==JNET_CHARACTER_WAR_MACHINE){
        uint64_t rails=0,count=0,level_events=0;uint32_t event_count=0;
        if(!own_read(NULL,game+0x258,&level_events,8) || !level_events ||
           !own_read(NULL,level_events+0x4008,&event_count,4) || event_count>=2048 ||
           !own_read(NULL,game+0x168,&rails,8) || !rails ||
           !own_read(NULL,rails+0x42880,&count,8) || count>156)return 0;
    }
    if(!jnet_character_private_pool(game))return 0;
    float position[3];lua_State *state=NULL;
    if(!own_read(NULL,barn+0xc0,position,sizeof(position)) ||
       !own_read(NULL,game+0x20,&state,8) || !state)return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i]))return 0;
    jnet_character_instance=(struct jnet_character_state){.kind=kind,.skin=skin,.owner=jnet_character_owner,.game=game,.barn=barn,.control={.kind=kind}};
    uint64_t creature=0;
    int code=jnet_character_native_reserve(jnet_character_pool_read,jnet_character_write_memory,&jnet_character_instance,
        fault_image,game,&spec->budget,jnet_character_resources_ready,jnet_character_allocate_native,
        &jnet_character_instance.pools,&creature);
    if(code){
        struct jnet_character_pools snapshot={0};
        int readable=jnet_character_pool_snapshot(jnet_character_pool_read,&jnet_character_instance,fault_image,game,&snapshot);
        char line[300];snprintf(line,sizeof(line),"[JNET] character %s reservation declined: %d snapshot=%d creatures=%u decorations=%u markers=%llu clumps=%llu events=%u required=%u/%u/%u/%u",jnet_character_name(kind),code,readable,snapshot.creature_count,snapshot.decoration_count,(unsigned long long)snapshot.marker_count,(unsigned long long)snapshot.clump_count,snapshot.event_count,spec->budget.decorations,spec->budget.markers,spec->budget.clumps,spec->budget.events);message(line);
        jnet_character_instance.failed=code<=-4;
        if(code<=-4)jnet_character_failed_mask|=1u<<kind;
        return 0;
    }
    jnet_character_instance.binding=(struct jnet_character_binding){fault_image,creature,0,jnet_character_generation};
    if(!jnet_character_lua_factory(state,spec,position))goto failed;
    uint32_t count=0;
    if(!own_read(NULL,jnet_character_instance.pools.decorations+0x434010,&count,4) ||
       count!=jnet_character_instance.pools.decoration_count+spec->mesh_count ||
       !own_read(NULL,creature+0x10,&jnet_character_instance.binding.target,8) ||
       !jnet_character_binding_valid(own_read,NULL,&jnet_character_instance.binding,jnet_character_generation))goto failed;
    for(unsigned i=0;i<spec->mesh_count;++i){
        uint64_t mesh=jnet_character_instance.pools.decorations+0x10+
            (uint64_t)(jnet_character_instance.pools.decoration_count+i)*0x10d0;
        unsigned blend=spec->meshes[i].visible?0:spec->meshes[i].flags;
        if(!jc_local_windows_write(mesh+0xf30,&blend,4))goto failed;
    }
    uint64_t rails=0,rail_count=0,level_events=0,segments=0,segment_count=0,clump_count=0;
    uint32_t event_count=0;
    if(!own_read(NULL,game+0x168,&rails,8) || !rails ||
       !own_read(NULL,rails+0x42880,&rail_count,8) || rail_count>=160 ||
       !own_read(NULL,game+0x258,&level_events,8) || !level_events ||
       !own_read(NULL,level_events+0x4008,&event_count,4) || event_count>2048 ||
       !own_read(NULL,creature+0x18,&segments,8) || !segments ||
       !own_read(NULL,segments+0x10,&segment_count,8) || !segment_count || segment_count>16 ||
       segment_count>2048-event_count ||
       !own_read(NULL,jnet_character_instance.pools.clumps+0x2c080,&clump_count,8) ||
       clump_count>2048 || segment_count+1>2048-clump_count)goto failed;
    if(kind==JNET_CHARACTER_WAR_MACHINE){
        uint64_t event=0,type=0;
        if(!event_count || !own_read(NULL,level_events+8+(uint64_t)(event_count-1)*8,&event,8) ||
           !own_read(NULL,event,&type,8) || type!=fault_image+0x66ce70)goto failed;
        jnet_character_instance.attack_event=event;
    }
    void (*initialize)(uintptr_t,uintptr_t,uintptr_t,uintptr_t)=(void *)(fault_image+0x236e80);
    initialize(creature,jnet_character_instance.pools.clumps,level_events,rails);
    unsigned char ready[2]={1,1};
    if(!jc_local_windows_write(creature+8,ready,2))goto failed;
    if(!own_read(NULL,jnet_character_instance.pools.events+0x1008,&count,4) ||
       count!=jnet_character_instance.pools.event_count+spec->budget.events)goto failed;
    unsigned glow_prepared=0;
    for(unsigned i=jnet_character_instance.pools.event_count;i<count;++i){
        uint64_t event=0,type=0,source=0,objects=0,n=0,scale=0,rotation=0;
        if(!own_read(NULL,jnet_character_instance.pools.events+8+(uint64_t)i*8,&event,8) || !event ||
           !own_read(NULL,event,&type,8))goto failed;
        if(type==fault_image+0x66ce70){jnet_character_instance.attack_event=event;continue;}
        if(type==fault_image+0x66cfe8){jnet_character_instance.attack_handler=event;continue;}
        if(type==fault_image+0x672158){jnet_character_instance.eye_event=event;continue;}
        if(type==fault_image+JNET_CHARACTER_GLOW_VTABLE_RVA){
            static const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20};
            unsigned char actual[sizeof(entry)];
            uint64_t first=jnet_character_instance.pools.decorations+0x10+
                (uint64_t)jnet_character_instance.pools.decoration_count*0x10d0;
            if(!own_read(NULL,fault_image+0x179300,actual,sizeof(actual)) || memcmp(actual,entry,sizeof(entry)) ||
               !jnet_character_glow_ready(own_read,NULL,fault_image,event,first,spec->mesh_count,0))goto failed;
            void (*prepare_glow)(uintptr_t)=(void *)(fault_image+0x179300);
            prepare_glow(event);
            if(!jnet_character_glow_ready(own_read,NULL,fault_image,event,first,spec->mesh_count,1))goto failed;
            ++glow_prepared;
            message("[JNET] native cloth touch glow prepared for private creature");
            continue;
        }
        if(type==fault_image+0x679ed8){
            uint64_t first=jnet_character_instance.pools.decorations+0x10+
                (uint64_t)jnet_character_instance.pools.decoration_count*0x10d0;
            if(!jnet_character_cloth_ride_ready(own_read,NULL,fault_image,event,first,spec->mesh_count))goto failed;
            if(!jnet_character_instance.ride_event)jnet_character_instance.ride_event=event;
            else if(kind==JNET_CHARACTER_LONGTAIL && !jnet_character_instance.grab_event)
                jnet_character_instance.grab_event=event;
            else goto failed;
            continue;
        }
        if(type==fault_image+0x671358){
            if(kind!=JNET_CHARACTER_LONGTAIL || jnet_character_instance.grab_event)goto failed;
            jnet_character_instance.grab_event=event;
            continue;
        }
        if(type==fault_image+0x6718b0){
            static const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48};
            unsigned char actual[sizeof(entry)];
            if(!own_read(NULL,fault_image+0x1c2b70,actual,sizeof(actual)) || memcmp(actual,entry,sizeof(entry)) ||
               !jnet_character_ride_ready(own_read,NULL,fault_image,event,creature,0))goto failed;
            void (*prepare_ride)(uintptr_t)=(void *)(fault_image+0x1c2b70);
            prepare_ride(event);
            if(!jnet_character_ride_ready(own_read,NULL,fault_image,event,creature,1))goto failed;
            jnet_character_instance.ride_event=event;
            message("[JNET] native WhaleRide prepared for private creature");
            continue;
        }
        if(type==fault_image+0x670678){
            static const unsigned char entry[]={0x4c,0x8b,0xdc,0x41,0x55,0x48,0x83,0xec,0x50,0x48,0x8b,0x41,0x50,0x4c,0x8b,0xe9};
            unsigned char actual[sizeof(entry)];
            uint64_t first=jnet_character_instance.pools.decorations+0x10+
                (uint64_t)jnet_character_instance.pools.decoration_count*0x10d0;
            if(!own_read(NULL,fault_image+0x1ba890,actual,sizeof(actual)) || memcmp(actual,entry,sizeof(entry)) ||
               !jnet_character_animation_ready(own_read,NULL,fault_image,event,creature,first,spec->budget.decorations,0))goto failed;
            void (*prepare_animation)(uintptr_t)=(void *)(fault_image+0x1ba890);
            prepare_animation(event);
            if(!jnet_character_animation_ready(own_read,NULL,fault_image,event,creature,first,spec->budget.decorations,1))goto failed;
            message("[JNET] private creature animation prepared and ownership verified");
            continue;
        }
        if(type!=fault_image+0x65ab80)continue;
        if(!own_read(NULL,event+0x48,&source,8) || !source ||
           !own_read(NULL,event+0x50,&objects,8) || !objects ||
           !own_read(NULL,objects+0x10,&n,8) || !n || n>1024)goto failed;
        void (*prepare_attachment)(uintptr_t,uintptr_t)=(void *)(fault_image+0x158970);
        prepare_attachment(event,level_events+0x4010);
        if(!own_read(NULL,event+0x78,&scale,8) || !scale ||
           !own_read(NULL,event+0x80,&rotation,8) || !rotation)goto failed;
    }
    if(glow_prepared!=1)goto failed;
    int top=gettop(state);
    int started=jnet_character_lua_chunk(state,"assert(__JNETCharacterNative):startEvents()",0);
    settop(state,top);
    if(!started)goto failed;
    return 1;
 failed:
    jnet_character_hide_created(&jnet_character_instance);
    jnet_character_failed_mask|=1u<<kind;
    jnet_character_instance.failed=1;return 0;
}

static float jnet_character_frame_dt;
static int jnet_character_visibility(struct jnet_character_state *instance,int visible) {
    if(!jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation))return 0;
    const struct jnet_character_spec *spec=&jnet_character_specs[instance->kind];
    int ok=1;
    if(!visible && instance->kind==JNET_CHARACTER_WAR_MACHINE)instance->control.spotlight=0;
    for(unsigned i=0;i<spec->mesh_count;++i){
        unsigned char show=visible && spec->meshes[i].visible;
        uint64_t mesh=instance->pools.decorations+0x10+(uint64_t)(instance->pools.decoration_count+i)*0x10d0;
        if(!jc_local_windows_write(mesh+0x70c,&show,1))ok=0;
    }
    return ok;
}
static int jnet_character_restore_wayfarer(struct jnet_character_state *instance) {
    uint64_t type=0;
    return own_read(NULL,instance->barn+0xb0,&type,8) && type==fault_image+0x5d0380 &&
        jc_local_windows_write(instance->barn+0xb0+0x1e00,&instance->saved_dissolve[0],4) &&
        jc_local_windows_write(instance->barn+0xb0+0x2000,&instance->saved_dissolve[1],4) &&
        jc_local_windows_write(instance->barn+0xb0+0x9c34,&instance->saved_visibility[0],1) &&
        jc_local_windows_write(instance->barn+0xb0+0x9cc4,&instance->saved_visibility[1],1);
}
static void jnet_character_fail(struct jnet_character_state *instance) {
    if(!instance->failed){
        char line[240];
        snprintf(line,sizeof(line),"[JNET] native character %s stopped: %s (detail %d)",
            jnet_character_name(instance->kind),instance->failure_stage?instance->failure_stage:"switch/restore",instance->failure_detail);
        message(line);
    }
    instance->failed=1;
    if(instance->owner<3)jnet_character_failed_masks[instance->owner]|=1u<<instance->kind;
    if(jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation)){
        unsigned char stopped=1;
        jc_local_windows_write(instance->binding.creature+9,&stopped,1);
        jnet_character_visibility(instance,0);
    }
    jnet_character_restore_wayfarer(instance);
    if(!instance->owner && jnet_menu_edit.applied.character==instance->kind){
        jnet_menu_edit.applied.character=JNET_CHARACTER_WAYFARER;
        jnet_menu_edit.applied.variant=0;
        jnet_appearance_applied_at=0;
        strcpy(jnet_appearance_status,"Character unavailable");
    }
}
static void jnet_character_restore_previous(const struct jnet_character_state *previous) {
    if(!previous->binding.creature || previous->failed ||
       !jnet_character_binding_valid(own_read,NULL,&previous->binding,jnet_character_generation))return;
    jnet_character_instance=*previous;
    lua_State *state=NULL;
    if(!own_read(NULL,previous->game+0x20,&state,8) || !state){
        jnet_character_fail(&jnet_character_instance);return;
    }
    char command[160];
    snprintf(command,sizeof(command),"__JNETCharacterNative=assert(__JNETCharacterInstances[\"%u|%s|%u\"])",jnet_character_owner,jnet_character_name(previous->kind),previous->skin);
    int top=gettop(state),ok=jnet_character_lua_chunk(state,command,0);settop(state,top);
    unsigned char stopped=0;
    if(!ok || !jc_local_windows_write(previous->binding.creature+9,&stopped,1) ||
       !jnet_character_visibility(&jnet_character_instance,previous->initialized))
        jnet_character_fail(&jnet_character_instance);
}
static void jnet_character_reactivate_state(struct jnet_character_state *instance) {
    instance->activation_started=GetTickCount64();
    instance->activation_reported=0;
    instance->initialized=0;
    instance->visual_samples=0;
    instance->input_frames=instance->follow_frames=0;
    instance->follow_position_valid=0;
    instance->motion_frame_valid=0;
    instance->sprint=0;
    instance->grab_blocked=0;
    instance->ride_peer=instance->ride_epoch=instance->ride_tick=instance->ride_active=0;
    instance->attack_busy=0;
    instance->attack_target=instance->attack_target_lease=instance->attack_actor=0;
    memset(instance->motion,0,sizeof(instance->motion));
    memset(&instance->remote_pose,0,sizeof(instance->remote_pose));
}
static int jnet_character_reposition_cached(uintptr_t game,unsigned kind) {
    struct jnet_character_pools pools;lua_State *state=NULL;char command[256];
    if(!jnet_character_pool_snapshot(own_read,NULL,fault_image,game,&pools) || pools.event_count>=512 ||
       !own_read(NULL,game+0x20,&state,8) || !state)return 0;
    float position[3];
    if(!own_read(NULL,jnet_character_instance.barn+0xc0,position,sizeof(position)))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i]))return 0;
    snprintf(command,sizeof(command),"__JNETCharacterNative=assert(__JNETCharacterInstances[\"%u|%s|%u\"]); __JNETCharacterNative:teleport({%.9g,%.9g,%.9g})",
        jnet_character_owner,jnet_character_name(kind),jnet_character_instance.skin,(double)position[0],(double)position[1],(double)position[2]);
    int top=gettop(state),ok=jnet_character_lua_chunk(state,command,0);settop(state,top);
    uint32_t count=0;
    if(!ok || !own_read(NULL,pools.events+0x1008,&count,4) || count!=pools.event_count+1 ||
       !own_read(NULL,pools.events+8+(uint64_t)pools.event_count*8,&jnet_character_instance.teleport_event,8) ||
       !jnet_character_teleport_identity(own_read,jnet_character_write_memory,NULL,fault_image,
            pools.events,jnet_character_instance.teleport_event,&jnet_character_instance.teleport_identity))return 0;
    top=gettop(state);
    ok=jnet_character_lua_chunk(state,"local n=assert(__JNETCharacterNative); assert(n.teleportEvent):Start(); n.teleportEvent=nil",0);
    settop(state,top);
    if(!ok)return 0;
    unsigned char start[2]={1,0};
    if(!jc_local_windows_write(jnet_character_instance.binding.creature+8,start,2))return 0;
    jnet_character_reactivate_state(&jnet_character_instance);
    return 1;
}
static int jnet_character_switch_native(uintptr_t game,uintptr_t barn,unsigned kind,unsigned variant) {
    if(variant>jnet_character_variant_limit(kind))return 0;
    unsigned skin=kind==JNET_CHARACTER_WAR_MACHINE?variant:0;
    unsigned cache=kind+JNET_CHARACTER_COUNT*skin;
    if((kind!=JNET_CHARACTER_WAYFARER && !jnet_character_is_creature(kind)) ||
       (kind && (!(jnet_menu_supported_characters&(1u<<kind)) || (jnet_character_failed_mask&(1u<<kind)))))return 0;
    if(jnet_character_instance.binding.creature &&
       (jnet_character_instance.game!=game || jnet_character_instance.binding.generation!=jnet_character_generation))return 0;
    if(kind && jnet_character_cached[cache].binding.creature &&
       (jnet_character_cached[cache].game!=game || jnet_character_cached[cache].binding.generation!=jnet_character_generation ||
        jnet_character_cached[cache].failed))return 0;
    if(!jnet_character_instance.failed && (jnet_character_instance.kind!=kind || jnet_character_instance.skin!=skin) &&
       jnet_character_attack_locked(own_read,NULL,fault_image,jnet_character_instance.attack_event,jnet_character_instance.binding.creature)!=0)return 0;
    struct jnet_character_state previous=jnet_character_instance;
    if(previous.binding.creature && previous.kind==kind && previous.skin==skin && previous.barn==barn && !previous.failed)return 1;
    float dissolve[2];unsigned char visibility[2];
    if(!own_read(NULL,barn+0xb0+0x1e00,dissolve,4) ||
       !own_read(NULL,barn+0xb0+0x2000,dissolve+1,4) ||
       !isfinite(dissolve[0]) || !isfinite(dissolve[1]) ||
       !own_read(NULL,barn+0xb0+0x9c34,visibility,1) ||
       !own_read(NULL,barn+0xb0+0x9cc4,visibility+1,1))return 0;
    if(previous.binding.creature){
        unsigned char stopped=1;
        int valid=jnet_character_binding_valid(own_read,NULL,&previous.binding,jnet_character_generation);
        if((valid && (!jnet_character_visibility(&previous,0) ||
           !jc_local_windows_write(previous.binding.creature+9,&stopped,1))) ||
           !jnet_character_restore_wayfarer(&previous)){
            jnet_character_restore_previous(&previous);return 0;
        }
        if(previous.barn==barn){
            memcpy(dissolve,previous.saved_dissolve,sizeof(dissolve));
            memcpy(visibility,previous.saved_visibility,sizeof(visibility));
        }
        if(valid && !previous.failed)jnet_character_cached[previous.kind+JNET_CHARACTER_COUNT*previous.skin]=previous;
    }
    memset(&jnet_character_instance,0,sizeof(jnet_character_instance));
    if(!kind){
        if(previous.binding.creature){
            float restored[2]={0};
            int read=own_read(NULL,barn+0xb0+0x1e00,restored,4) &&
                own_read(NULL,barn+0xb0+0x2000,restored+1,4);
            char line[224];
            snprintf(line,sizeof(line),"[JNET] Wayfarer restored owner=%u actor=%llx previous=%u dissolve saved=%.6g/%.6g read=%d current=%.6g/%.6g",
                jnet_character_owner,(unsigned long long)(barn+0xb0),previous.kind,
                previous.saved_dissolve[0],previous.saved_dissolve[1],read,restored[0],restored[1]);
            message(line);
        }
        return 1;
    }
    if(jnet_character_cached[cache].binding.creature){
        jnet_character_instance=jnet_character_cached[cache];
        jnet_character_instance.game=game;jnet_character_instance.barn=barn;
        memcpy(jnet_character_instance.saved_dissolve,dissolve,sizeof(dissolve));
        memcpy(jnet_character_instance.saved_visibility,visibility,sizeof(visibility));
        if(jnet_character_binding_valid(own_read,NULL,&jnet_character_instance.binding,jnet_character_generation) &&
           jnet_character_reposition_cached(game,kind))return 1;
        jnet_character_fail(&jnet_character_instance);
        jnet_character_restore_previous(&previous);return 0;
    }
    if(!jnet_character_prepare_native(game,barn,kind,skin)){
        jnet_character_restore_previous(&previous);
        return 0;
    }
    memcpy(jnet_character_instance.saved_dissolve,dissolve,sizeof(dissolve));
    memcpy(jnet_character_instance.saved_visibility,visibility,sizeof(visibility));
    unsigned char start[2]={1,0};
    if(!jc_local_windows_write(jnet_character_instance.binding.creature+8,start,2)){
        jnet_character_fail(&jnet_character_instance);
        jnet_character_restore_previous(&previous);return 0;
    }
    return 1;
}
static int jnet_character_ground(void *ctx,const float position[3],float *height) {
    struct jnet_character_state *instance=ctx;
    uint64_t terrain=0;
    instance->failure_stage="terrain binding";
    if(!jnet_character_terrain_binding(own_read,NULL,instance->game,&terrain))return 0;
    instance->failure_stage="terrain sample";
    float (*sample)(float,float,uintptr_t,float,uintptr_t,unsigned char)=(void *)(fault_image+0x82340);
    *height=sample(position[0],position[2],terrain,0,0,0);
    if(!isfinite(*height))return 0;
    instance->failure_stage="target write/readback";
    return 1;
}
static void (*jnet_character_eye_original)(uintptr_t,uintptr_t);
static void jnet_character_eye(uintptr_t event,uintptr_t game) {
    struct jnet_character_state *instance=NULL;unsigned active=0;
    for(unsigned owner=0;owner<3;++owner){
        if(jnet_character_instances[owner].eye_event==event){instance=&jnet_character_instances[owner];active=1;break;}
        for(unsigned skin=0;skin<3;++skin)
            if(jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*skin].eye_event==event)
                instance=&jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*skin];
    }
    if(!instance){jnet_character_eye_original(event,game);return;}
    if(instance->game!=game || !jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation))return;
    if(!active || !instance->initialized || instance->failed) {
        uint64_t sound=0,handle=0;
        if(own_read(NULL,game+0x150,&sound,8) && sound && own_read(NULL,event+0xa4,&handle,8)) {
            void (*volume)(uintptr_t,uint64_t,float,float)=(void *)(fault_image+0x1468e0);
            volume(sound,handle,0,0);
        }
        return;
    }
    const struct jnet_character_spec *spec=&jnet_character_specs[instance->kind];
    unsigned checked=0;
    for(unsigned i=0;i<spec->mesh_count;++i){
        const char *name=spec->meshes[i].mesh;
        if(strcmp(name,"JNETCharacter_P_MechHead") && strcmp(name,"JNETCharacter_P_MechHeadGlowMask"))continue;
        uint64_t mesh=instance->pools.decorations+0x10+(uint64_t)(instance->pools.decoration_count+i)*0x10d0;
        uint64_t states=0;uint32_t count=0;unsigned char first[80];
        if(!own_read(NULL,mesh+0x588,&states,8) || !states ||
           !own_read(NULL,mesh+0x590,&count,4) || !count || count>64 ||
           !own_read(NULL,states,first,sizeof(first))){
            if(!instance->eye_warned){message("[JNET] spotlight waiting for native eyelid animation storage");instance->eye_warned=1;}
            instance->eye_ready=0;return;
        }
        ++checked;
    }
    if(checked!=2)return;
    unsigned requested=active && instance->initialized && !instance->failed && instance->control.spotlight==2;
    if(!instance->attack_busy && !jc_local_windows_write(event+0x7c,&requested,4))return;
    instance->eye_ready=1;
    unsigned dim=instance->control.spotlight==1 && !instance->attack_busy;
    const unsigned offsets[]={0xcc,0xdc,0x10c,0x11c};
    float saved[4],zero=0;
    if(dim){
        for(unsigned i=0;i<4;++i)if(!own_read(NULL,event+offsets[i],&saved[i],4))return;
        unsigned written=0;
        for(;written<4;++written)if(!jc_local_windows_write(event+offsets[written],&zero,4))break;
        if(written!=4){
            for(unsigned i=0;i<written;++i)jc_local_windows_write(event+offsets[i],&saved[i],4);
            return;
        }
    }
    if(instance->owner && instance->kind==JNET_CHARACTER_WAR_MACHINE){
        unsigned char angry=instance->remote_anger>0;
        jc_local_windows_write(event+0x88,&instance->remote_anger,4);
        jc_local_windows_write(event+0xa0,&angry,1);
    }
    jnet_character_eye_original(event,game);
    for(unsigned i=0;i<spec->mesh_count;++i){
        if(strcmp(spec->meshes[i].mesh,"JNETCharacter_P_MechHeadGlowMask"))continue;
        uint64_t mesh=instance->pools.decorations+0x10+(uint64_t)(instance->pools.decoration_count+i)*0x10d0;
        unsigned char visible=instance->control.spotlight!=0 || instance->attack_busy;
        if(!jc_local_windows_write(mesh+0x70c,&visible,1))jnet_character_fail(instance);
    }
    if(dim){
        for(unsigned i=0;i<4;++i)if(!jc_local_windows_write(event+offsets[i],&saved[i],4))jnet_character_fail(instance);
        uint64_t sound=0,handle=0;
        if(own_read(NULL,game+0x150,&sound,8) && sound && own_read(NULL,event+0xa4,&handle,8)){
            void (*volume)(uintptr_t,uint64_t,float,float)=(void *)(fault_image+0x1468e0);
            volume(sound,handle,0,0);
        }
    }
}
static unsigned char (*jnet_character_jump_original)(uintptr_t,unsigned char);
static unsigned char jnet_character_jump(uintptr_t pad,unsigned char cheats) {
    unsigned char pressed=jnet_character_jump_original(pad,cheats);
    uintptr_t rider_input=0,rider_pad=0;
    if(pressed && traveler_game && jnet_menu_edit.applied.character==JNET_CHARACTER_WAYFARER &&
       own_read(NULL,traveler_game+0x40,&rider_input,8) && rider_input &&
       own_read(NULL,rider_input+8,&rider_pad,8) && rider_pad==pad){
        for(unsigned owner=1;owner<3;++owner){
            struct jnet_character_state *holder=&jnet_character_instances[owner];uint64_t attached=0;
            if(holder->initialized && !holder->failed && holder->game==traveler_game && holder->grab_event &&
               own_read(NULL,holder->grab_event+0x90,&attached,8) && attached==holder->binding.creature)
                holder->grab_blocked=1;
        }
    }
    struct jnet_character_state *instance=&jnet_character_instance;
    uintptr_t input=0,local_pad=0;
    if(instance->initialized && !instance->failed && instance->kind!=JNET_CHARACTER_WAYFARER &&
       jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation) &&
       own_read(NULL,instance->game+0x40,&input,8) && input &&
       own_read(NULL,input+8,&local_pad,8) && local_pad==pad)return 0;
    return pressed;
}
static void (*jnet_character_interact_original)(uintptr_t,uintptr_t);
static void jnet_character_interact(uintptr_t module,uintptr_t game) {
    struct jnet_character_state *instance=&jnet_character_instance;
    if(instance->initialized && !instance->failed && instance->game==game &&
       instance->kind==JNET_CHARACTER_WAR_MACHINE &&
       jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation))return;
    jnet_character_interact_original(module,game);
}
static struct jnet_character_state *jnet_character_attack_instance(uintptr_t event,int handler,unsigned *active) {
    *active=0;
    for(unsigned owner=0;owner<3;++owner){
        struct jnet_character_state *instance=&jnet_character_instances[owner];
        if((handler?instance->attack_handler:instance->attack_event)==event){*active=1;return instance;}
        for(unsigned skin=0;skin<3;++skin){
            instance=&jnet_character_caches[owner][JNET_CHARACTER_WAR_MACHINE+JNET_CHARACTER_COUNT*skin];
            if((handler?instance->attack_handler:instance->attack_event)==event)return instance;
        }
    }
    return NULL;
}
static int jnet_character_attack_sync(struct jnet_character_state *instance) {
    int locked=jnet_character_attack_locked(own_read,NULL,fault_image,instance->attack_event,instance->binding.creature);
    if(locked<0)return 0;
    if(instance->attack_busy && !locked){
        float position[3],zero[4]={0};
        if(!jnet_character_head_position(own_read,NULL,&instance->binding,jnet_character_generation,position) ||
           !jc_local_windows_write(instance->binding.target+0x10,position,sizeof(position)) ||
           !jc_local_windows_write(instance->binding.creature+0x1200,zero,sizeof(zero)))return 0;
        instance->follow_position_valid=0;
        message("[JNET] native war machine attack complete; movement released");
    }else if(!instance->attack_busy && locked){
        memset(instance->motion,0,sizeof(instance->motion));instance->sprint=0;
        message("[JNET] native war machine attack started; controls locked");
    }
    instance->attack_busy=(unsigned)locked;
    return 1;
}
static __thread uintptr_t jnet_character_attack_scope;
static unsigned char (*jnet_character_authority_original)(uintptr_t,uintptr_t);
static unsigned char jnet_character_authority(uintptr_t creature,uintptr_t barn) {
    if(creature && creature==jnet_character_attack_scope)return 0;
    return jnet_character_authority_original(creature,barn);
}
static void (*jnet_character_attack_original)(uintptr_t,uintptr_t);
static void (*jnet_character_attack_handler_original)(uintptr_t,uintptr_t);
static int jnet_character_attack_active(struct jnet_character_state *instance,uintptr_t game,unsigned active) {
    return active && !instance->owner && instance->initialized && !instance->failed && instance->game==game &&
        jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation);
}
static void (*jnet_character_hit_original)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static void (*jnet_character_collision_original)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static uint64_t jnet_character_hit_sequence;
static void jnet_character_hit(uintptr_t event,uintptr_t dude,uintptr_t a,uintptr_t b,uintptr_t events) {
    unsigned active=0;struct jnet_character_state *instance=jnet_character_attack_instance(event,0,&active);
    if(!instance){jnet_character_hit_original(event,dude,a,b,events);return;}
    if(!active || instance->owner || !instance->attack_target || dude!=instance->attack_actor)return;
    unsigned slot=0;
    if(instance->war_hits[0].victim && instance->war_hits[0].victim!=instance->attack_target)slot=1;
    struct jnet_war_hit *hit=&instance->war_hits[slot];
    uint64_t sequence=++jnet_character_hit_sequence;
    *hit=(struct jnet_war_hit){instance->attack_target,instance->attack_target_lease,sequence,GetTickCount64()};
}
static void jnet_character_collision(uintptr_t event,uintptr_t local,uintptr_t companion,
        uintptr_t camera,uintptr_t events,uintptr_t sounds) {
    unsigned active=0;struct jnet_character_state *instance=jnet_character_attack_instance(event,0,&active);
    if(!instance){jnet_character_collision_original(event,local,companion,camera,events,sounds);return;}
    if(!jnet_character_attack_active(instance,traveler_game,active) ||
       jnet_travel_busy || traveler_network.paused || !instance->attack_actor || !instance->attack_target)return;
    unsigned admitted=0;
    for(unsigned slot=0;slot<2;++slot){
        const struct jc_network_peer *peer=&traveler_network.peers[slot];
        unsigned ready=slot?traveler_local_state==2:traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE;
        uintptr_t actor=slot?traveler_barn+0x146f0:traveler_object;
        if(ready && actor && actor==instance->attack_actor && peer->connection.admitted && peer->connection.peer==instance->attack_target &&
           peer->connection.remote_epoch==instance->attack_target_lease)admitted=1;
    }
    if(admitted)jnet_character_collision_original(event,instance->attack_actor,0,camera,events,sounds);
}
static void jnet_character_attack(uintptr_t event,uintptr_t game) {
    unsigned active=0;
    struct jnet_character_state *instance=jnet_character_attack_instance(event,0,&active);
    if(!instance){jnet_character_attack_original(event,game);return;}
    if(!jnet_character_attack_active(instance,game,active))return;
    instance->failure_stage="native attack update";
    if(!jnet_character_attack_sync(instance)){jnet_character_fail(instance);return;}
    uintptr_t previous_scope=jnet_character_attack_scope;
    jnet_character_attack_scope=instance->binding.creature;
    jnet_character_attack_original(event,game);
    jnet_character_attack_scope=previous_scope;
    if(!jnet_character_attack_sync(instance))jnet_character_fail(instance);
}
static void jnet_character_attack_handler(uintptr_t event,uintptr_t game) {
    unsigned active=0;
    struct jnet_character_state *instance=jnet_character_attack_instance(event,1,&active);
    if(!instance){jnet_character_attack_handler_original(event,game);return;}
    if(!jnet_character_attack_active(instance,game,active))return;
    instance->failure_stage="native attack targets";
    if(!jnet_character_attack_ready(own_read,NULL,fault_image,instance->attack_event,instance->attack_handler,
            instance->binding.creature,instance->eye_event)){jnet_character_fail(instance);return;}
    if(!jnet_character_attack_sync(instance)){jnet_character_fail(instance);return;}
    uint64_t target_ids[3]={0},target_leases[3]={0},target_actors[3]={0};
    unsigned was_busy=instance->attack_busy;
    if(!instance->attack_busy){
        uint64_t actors[3]={0,0,0};
        struct jnet_game_record records[JNET_STEAM_CAPACITY];uint64_t room=0;
        AcquireSRWLockExclusive(&jnet_sg_lock);
        room=jnet_sg_mail.room;memcpy(records,jnet_sg_mail.records,sizeof(records));
        ReleaseSRWLockExclusive(&jnet_sg_lock);
        uint64_t now=GetTickCount64();
        for(unsigned slot=0;slot<2;++slot){
            struct jc_network_peer *peer=&traveler_network.peers[slot];
            unsigned ready=slot?traveler_local_state==2:traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE;
            if(!room || !ready || traveler_network.paused || !peer->connection.admitted)continue;
            for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)
                if(records[i].id==peer->connection.peer && records[i].lease==peer->connection.remote_epoch &&
                   records[i].sample.chapter==jnet_current_level && records[i].sequence &&
                   records[i].character_known && records[i].character<JNET_CHARACTER_COUNT &&
                   now>=records[i].seen && now-records[i].seen<2000)
                    {actors[slot+1]=slot?traveler_barn+0x146f0:traveler_object;
                     target_ids[slot+1]=records[i].id;target_leases[slot+1]=records[i].lease;
                     target_actors[slot+1]=actors[slot+1];}
        }
        if(!jnet_character_attack_targets(own_read,jnet_character_write_memory,NULL,instance->attack_event,
                actors,jnet_attack_down && instance->control.spotlight==2 && instance->eye_ready)){jnet_character_fail(instance);return;}
        uint64_t merged=0,rails=0,count=0;
        if(!own_read(NULL,instance->attack_event+0x128,&merged,8) ||
           (!merged && (!own_read(NULL,game+0x168,&rails,8) || !rails ||
            !own_read(NULL,rails+0x42880,&count,8) || count>=160))){
            instance->failure_stage="native attack rail capacity";jnet_character_fail(instance);return;
        }
    }
    uintptr_t previous_scope=jnet_character_attack_scope;
    jnet_character_attack_scope=instance->binding.creature;
    jnet_character_attack_handler_original(event,game);
    jnet_character_attack_scope=previous_scope;
    if(!jnet_character_attack_sync(instance))jnet_character_fail(instance);
    if(!was_busy && instance->attack_busy){
        uint32_t selected=3;own_read(NULL,instance->attack_event+0xe4,&selected,4);
        if(selected<3)jc_local_windows_write(instance->attack_event+0xe0,&selected,4);
        instance->attack_target=selected<3?target_ids[selected]:0;
        instance->attack_target_lease=selected<3?target_leases[selected]:0;
        instance->attack_actor=selected<3?target_actors[selected]:0;
        unsigned char clear[2]={0};jc_local_windows_write(instance->attack_event+0xf8,clear,2);
    }
    if(jnet_attack_down && GetTickCount64()>=instance->attack_report_tick){
        uint32_t eye_state=0,target_index=0;float acquired=0;
        own_read(NULL,instance->eye_event+0x78,&eye_state,4);
        own_read(NULL,instance->attack_event+0xe4,&target_index,4);
        own_read(NULL,event+0x6c,&acquired,4);
        char line[192];snprintf(line,sizeof(line),"[JNET] held Attack: spotlight=%u eye=%u target=%u acquisition=%.3f active=%u",
            instance->control.spotlight,eye_state,target_index,acquired,instance->attack_busy);message(line);
        instance->attack_report_tick=GetTickCount64()+2000;
    }
}
static void jnet_character_input_native(uintptr_t barn,uintptr_t game) {
    jnet_controls_tick(barn,game,jnet_menu_edit.applied.character);
    if(jnet_menu_edit.applied.character==JNET_CHARACTER_WAYFARER && !jnet_travel_busy && !traveler_network.paused){
        uintptr_t input=0,pad=0;
        if(own_read(NULL,game+0x40,&input,8) && input && own_read(NULL,input+8,&pad,8) && pad)
            jnet_character_jump(pad,1);
    }
    struct jnet_character_state *instance=&jnet_character_instance;
    if(!instance->initialized || instance->failed || instance->game!=game || instance->barn!=barn)return;
    int attacking=jnet_character_attack_locked(own_read,NULL,fault_image,instance->attack_event,instance->binding.creature);
    if(attacking<0){instance->failure_stage="attack state";jnet_character_fail(instance);return;}
    if(attacking){memset(instance->motion,0,sizeof(instance->motion));instance->sprint=0;return;}
    ++instance->input_frames;
    unsigned char menu=0;
    if(jnet_menu_last && !own_read(NULL,jnet_menu_last+0xd68,&menu,1))return;
    struct jnet_character_input input;struct jnet_character_intent intent;
    instance->failure_stage="input sample";
    instance->failure_detail=0;
    uintptr_t input_system=0,pad=0;unsigned char cheats=0;
    if(!own_read(NULL,game+0x40,&input_system,8) || !input_system ||
       !own_read(NULL,input_system+8,&pad,8) || !pad ||
       !own_read(NULL,fault_image+0x3c9420d,&cheats,1))return;
    unsigned jump=jnet_character_jump_original(pad,cheats);
    if(!jnet_character_input_sample(own_read,NULL,fault_image,game,barn,!menu,jump,&input)){
        jnet_character_fail(instance);return;
    }
    uintptr_t channel=0;unsigned char sit[4]={0};
    if(!own_read(NULL,pad+0x260,&channel,8) || !channel ||
       !own_read(NULL,channel+0x4c,sit,sizeof(sit)))return;
    input.interact=jnet_spotlight_down;
    input.fly=jnet_fly_up_down;
    input.fly_down=jnet_fly_down_down;
    input.level=jnet_level_down;
    instance->failure_stage="input direction";
    if(!jnet_character_control_step(&instance->control,&input,&intent)){
        jnet_character_fail(instance);return;
    }
    jnet_character_spotlight=intent.spotlight;
    if(intent.spotlight_changed){
        char line[96];snprintf(line,sizeof(line),"[JNET] war machine spotlight requested=%u native_ready=%u",intent.spotlight,instance->eye_ready);message(line);
    }
    if(!jnet_character_smooth(instance->motion,&intent,jnet_character_frame_dt,!menu)){
        jnet_character_fail(instance);return;
    }
    float ramp=1-expf(-fminf(jnet_character_frame_dt,.05f)/.28f);
    instance->sprint+=ramp*((float)jnet_sprint_down-instance->sprint);
    instance->failure_stage="target binding/pose";
    int result=jnet_character_target_step(own_read,jnet_character_write_memory,instance,&instance->binding,
        jnet_character_generation,&intent,jnet_character_speed(instance->kind)*(1+instance->sprint),jnet_character_frame_dt,jnet_character_ground);
    if(result){instance->failure_detail=result;jnet_character_fail(instance);return;}
    instance->motion_frame=(uint32_t)JNETFrames;instance->motion_frame_valid=1;
}
static void jnet_character_motion_report(struct jnet_character_state *instance,float dt,int early) {
    static struct {
        uint64_t creature,generation,at,last;
        float target[3],head[3],gap_peak,root_error_peak,vertical_error_peak,step_peak,dt_peak;
        unsigned samples,moving,vertical,held,early;
    } trace;
    float target[3],head[3];uint64_t now=GetTickCount64();
    if(!own_read(NULL,instance->binding.target+0x10,target,sizeof(target)) ||
       !own_read(NULL,instance->binding.creature+0x110,head,sizeof(head)))return;
    for(unsigned i=0;i<3;++i)if(!isfinite(target[i]) || !isfinite(head[i]))return;
    if(trace.creature!=instance->binding.creature || trace.generation!=jnet_character_generation ||
       now<trace.last){
        memset(&trace,0,sizeof(trace));trace.creature=instance->binding.creature;
        trace.generation=jnet_character_generation;trace.at=trace.last=now;
        memcpy(trace.target,target,sizeof(target));memcpy(trace.head,head,sizeof(head));return;
    }
    float target_step=0,head_step=0,error=0,vertical_error=0;
    for(unsigned i=0;i<3;++i){
        float t=target[i]-trace.target[i],h=head[i]-trace.head[i];
        target_step+=t*t;head_step+=h*h;error+=(h-t)*(h-t);
        if(i==1){vertical_error=fabsf(h-t);trace.vertical+=fabsf(t)>.001f;}
    }
    trace.gap_peak=fmaxf(trace.gap_peak,(float)(now-trace.last));
    trace.dt_peak=fmaxf(trace.dt_peak,dt);
    if(target_step<4 && head_step<4){
        ++trace.samples;trace.moving+=target_step>.000001f;trace.early+=early!=0;
        trace.held+=target_step>.0001f && head_step<target_step*.01f;
        trace.root_error_peak=fmaxf(trace.root_error_peak,sqrtf(error));
        trace.vertical_error_peak=fmaxf(trace.vertical_error_peak,vertical_error);
        trace.step_peak=fmaxf(trace.step_peak,sqrtf(head_step));
    }
    memcpy(trace.target,target,sizeof(target));memcpy(trace.head,head,sizeof(head));trace.last=now;
    if(now-trace.at>=2000){
        if(trace.moving){
            char line[320];snprintf(line,sizeof(line),
                "[JNET] creature motion %s samples/moving/vertical/early=%u/%u/%u/%u head_held=%u gap_peak_ms=%.0f dt_peak_ms=%.1f head_step_peak=%.4f root_step_error_peak=%.4f vertical_step_error_peak=%.4f (native poses, not cloth/render)",
                jnet_character_name(instance->kind),trace.samples,trace.moving,trace.vertical,trace.early,trace.held,
                (double)trace.gap_peak,(double)trace.dt_peak*1000,(double)trace.step_peak,
                (double)trace.root_error_peak,(double)trace.vertical_error_peak);message(line);
        }
        trace.at=now;trace.samples=trace.moving=trace.vertical=trace.held=trace.early=0;
        trace.gap_peak=trace.root_error_peak=trace.vertical_error_peak=trace.step_peak=trace.dt_peak=0;
    }
}
static __declspec(thread) uintptr_t jnet_character_pose_creature;
static void (*jnet_character_segment_original)(uintptr_t,uintptr_t,float,float);
static void jnet_character_segment(uintptr_t segment,uintptr_t target,float dt,float blend) {
    uintptr_t creature=jnet_character_pose_creature;
    float previous[3],basis[12];uint32_t count=0;
    int controlled=creature && segment>=creature+0x110 &&
        (segment-creature-0x110)%0xa0==0 &&
        own_read(NULL,creature+0xb10,&count,4) && count<=16 &&
        (segment-creature-0x110)/0xa0<count &&
        own_read(NULL,segment+0x60,previous,sizeof(previous));
    jnet_character_segment_original(segment,target,dt,blend);
    if(controlled && own_read(NULL,segment+0x40,basis,sizeof(basis)) &&
       jnet_character_vertical_basis(previous,basis))
        jc_local_windows_write(segment+0x40,basis,sizeof(basis));
}
static void (*jnet_character_simulate_original)(float,uintptr_t,uintptr_t,uintptr_t);
static void jnet_character_simulate_owned(float dt,uintptr_t creature,uintptr_t unused,uintptr_t cloth) {
    struct jnet_character_state *instance=&jnet_character_instances[0];
    uintptr_t previous=jnet_character_pose_creature;
    jnet_character_pose_creature=instance->initialized && !instance->failed &&
        jnet_character_is_creature(instance->kind) && instance->binding.creature==creature &&
        jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation) &&
        jnet_character_attack_locked(own_read,NULL,fault_image,instance->attack_event,creature)==0?creature:0;
    jnet_character_simulate_original(dt,creature,unused,cloth);
    jnet_character_pose_creature=previous;
}
static struct { uintptr_t creature;uint64_t generation;uint32_t frame;unsigned valid; } jnet_character_early_pose;
static void jnet_character_simulate(float dt,uintptr_t creature,uintptr_t unused,uintptr_t cloth) {
    if(jnet_character_early_pose.valid && jnet_character_early_pose.creature==creature &&
       jnet_character_early_pose.generation==jnet_character_generation &&
       jnet_character_early_pose.frame==(uint32_t)JNETFrames)return;
    jnet_character_simulate_owned(dt,creature,unused,cloth);
    struct jnet_character_state *instance=&jnet_character_instances[0];
    if(instance->initialized && !instance->failed && instance->binding.creature==creature &&
       jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation))
        jnet_character_motion_report(instance,dt,0);
}
static void jnet_character_before_cloth(float dt,uintptr_t cloth) {
    struct jnet_character_state *instance=&jnet_character_instances[0];
    uintptr_t expected=0;unsigned char state[2]={0};
    if(!instance->initialized || instance->failed || instance->kind!=JNET_CHARACTER_LONGTAIL ||
       instance->game!=(uintptr_t)JNETGame || !instance->motion_frame_valid ||
       instance->motion_frame!=(uint32_t)JNETFrames || !isfinite(dt) || dt<=0 ||
       !jnet_character_simulate_original ||
       !jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation) ||
       !own_read(NULL,instance->game+0x108,&expected,8) || !expected || cloth!=expected ||
       !own_read(NULL,instance->binding.creature+8,state,sizeof(state)) || state[0]!=1 || state[1] ||
       jnet_character_attack_locked(own_read,NULL,fault_image,instance->attack_event,instance->binding.creature)!=0)return;
    if(jnet_character_early_pose.valid && jnet_character_early_pose.creature==instance->binding.creature &&
       jnet_character_early_pose.generation==jnet_character_generation &&
       jnet_character_early_pose.frame==(uint32_t)JNETFrames)return;
    jnet_character_simulate_owned(dt,instance->binding.creature,0,cloth);
    jnet_character_early_pose.creature=instance->binding.creature;
    jnet_character_early_pose.generation=jnet_character_generation;
    jnet_character_early_pose.frame=(uint32_t)JNETFrames;jnet_character_early_pose.valid=1;
    jnet_character_motion_report(instance,dt,1);
}
static void jnet_character_after_player(uintptr_t barn,uintptr_t game) {
    struct jnet_character_state *instance=&jnet_character_instance;
    if(!instance->initialized || instance->failed || instance->game!=game || instance->barn!=barn)return;
    ++instance->follow_frames;
    float native_position[3];
    if(!own_read(NULL,barn+0xc0,native_position,sizeof(native_position)))return;
    int attacking=jnet_character_attack_locked(own_read,NULL,fault_image,instance->attack_event,instance->binding.creature);
    if(attacking<0){instance->failure_stage="attack follow state";jnet_character_fail(instance);return;}
    if(instance->follow_position_valid && !attacking){
        float distance=0;
        for(unsigned i=0;i<3;++i){
            if(!isfinite(native_position[i]))return;
            float delta=native_position[i]-instance->follow_position[i];distance+=delta*delta;
        }
        if(distance>4.f){
            instance->failure_stage="native spawn relocation";
            if(!jnet_character_visibility(instance,0) ||
               !jnet_character_reposition_cached(game,instance->kind))jnet_character_fail(instance);
            else message("[JNET] creature following native player relocation");
            return;
        }
    }
    float position[3],hidden=1;unsigned char no_feet=0;
    instance->failure_stage="player follow target";
    instance->failure_detail=0;
    if(!jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation) ||
       !(attacking?jnet_character_head_position(own_read,NULL,&instance->binding,jnet_character_generation,position):
         own_read(NULL,instance->binding.target+0x10,position,sizeof(position)))){jnet_character_fail(instance);return;}
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i])){jnet_character_fail(instance);return;}
    instance->failure_stage="player position/dissolve write";
    if(!jc_local_windows_write(barn+0xb0+0x9c34,&no_feet,1) ||
       !jc_local_windows_write(barn+0xb0+0x9cc4,&no_feet,1) ||
       !jc_local_windows_write(barn+0xc0,position,sizeof(position)) ||
       !jc_local_windows_write(barn+0xb0+0x1e00,&hidden,4) ||
       !jc_local_windows_write(barn+0xb0+0x2000,&hidden,4)){
        jnet_character_fail(instance);
    }else{
        memcpy(instance->follow_position,position,sizeof(position));instance->follow_position_valid=1;
    }
}
static void jnet_character_visual_report(struct jnet_character_state *instance) {
    uint64_t now=GetTickCount64();
    if(instance->visual_samples>=2 || now-instance->visual_started<(instance->visual_samples?5000u:1000u))return;
    ++instance->visual_samples;
    if(!jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation))return;
    const struct jnet_character_spec *spec=&jnet_character_specs[instance->kind];
    float target[3]={0},player[3]={0};unsigned char menu=0;char line[640];
    int target_ok=own_read(NULL,instance->binding.target+0x10,target,sizeof(target));
    int player_ok=own_read(NULL,instance->barn+0xc0,player,sizeof(player));
    int menu_ok=jnet_menu_last && own_read(NULL,jnet_menu_last+0xd68,&menu,1);
    snprintf(line,sizeof(line),"[JNET] character visibility %s sample=%u input/follow=%u/%u menu=%d target_read=%d pos=%.3g,%.3g,%.3g player_read=%d pos=%.3g,%.3g,%.3g (state snapshots, not draw proof)",
        jnet_character_name(instance->kind),instance->visual_samples,instance->input_frames,instance->follow_frames,
        menu_ok?(int)menu:-1,target_ok,(double)target[0],(double)target[1],(double)target[2],
        player_ok,(double)player[0],(double)player[1],(double)player[2]);message(line);
    uint32_t count=0;
    if(!own_read(NULL,instance->pools.decorations+0x434010,&count,4) || count>1024 ||
       instance->pools.decoration_count>count || spec->mesh_count>count-instance->pools.decoration_count){
        message("[JNET] character visibility: decoration range unavailable");return;
    }
    for(unsigned i=0;i<spec->mesh_count;++i){
        if(!spec->meshes[i].visible)continue;
        unsigned duplicate=0;
        for(unsigned j=0;j<i;++j)
            if(spec->meshes[j].visible && !strcmp(spec->meshes[i].mesh,spec->meshes[j].mesh))duplicate=1;
        if(duplicate)continue;
        struct jnet_character_visual_snapshot s={0};
        uint64_t mesh=instance->pools.decorations+0x10+(uint64_t)(instance->pools.decoration_count+i)*0x10d0;
        int code=jnet_character_visual_read(own_read,NULL,mesh,&s);
        snprintf(line,sizeof(line),"[JNET] character mesh %s row=%u read=%d binding/resource/slots/instance=%u/%u/%u/%u changed/pending=%u/%u enabled/current/previous=%u/%u/%u state=%u group=%u blend=%u render_visits=%u pos=%.3g,%.3g,%.3g fade=%.3g/%.3g cloth_def/collection=%u/%u cloth_count/expected=%u/%u cloth_fade=%.3g basis=%.3g/%.3g/%.3g",
            spec->meshes[i].mesh,i,code,!!s.mesh_binding,!!s.mesh_resource,!!s.mesh_slots,!!s.mesh_instance,s.mesh_changed,s.cloth_pending,s.enabled[0],s.enabled[1],s.enabled[2],s.render_state,s.update_group,s.blend,s.render_visits,
            (double)s.matrix[12],(double)s.matrix[13],(double)s.matrix[14],(double)s.fade[0],(double)s.fade[1],
            !!s.cloth_definition,!!s.cloth_collection,s.cloth_count,s.cloth_expected,(double)s.cloth_fade,
            sqrt((double)s.matrix[0]*s.matrix[0]+(double)s.matrix[1]*s.matrix[1]+(double)s.matrix[2]*s.matrix[2]),
            sqrt((double)s.matrix[4]*s.matrix[4]+(double)s.matrix[5]*s.matrix[5]+(double)s.matrix[6]*s.matrix[6]),
            sqrt((double)s.matrix[8]*s.matrix[8]+(double)s.matrix[9]*s.matrix[9]+(double)s.matrix[10]*s.matrix[10]));message(line);
    }
}
static void jnet_character_activation_wait(struct jnet_character_state *instance,const char *stage) {
    if(!instance->activation_started || instance->activation_reported ||
       GetTickCount64()-instance->activation_started<2000)return;
    instance->activation_reported=1;
    unsigned char creature[2]={0},event[2]={0};uint32_t pending=0;
    int cr=own_read(NULL,instance->binding.creature+8,creature,2);
    int er=instance->teleport_event && own_read(NULL,instance->teleport_event+8,event,2);
    int pr=own_read(NULL,instance->binding.creature+0x1250,&pending,4);
    char line[256];
    snprintf(line,sizeof(line),"[JNET] %s activation waiting: %s; creature read/enable/stop=%d/%u/%u teleport read/start/active=%d/%u/%u request read/id=%d/%u",
        jnet_character_name(instance->kind),stage,cr,creature[0],creature[1],er,event[0],event[1],pr,pending);
    message(line);
}
static void jnet_character_tick_native(uintptr_t game) {
    struct jnet_character_state *instance=&jnet_character_instance;
    if(!jnet_character_owner && !instance->binding.creature && !instance->failed && jnet_character_is_creature(jnet_menu_edit.applied.character)){
        uintptr_t barn=0,save=0;
        if(GetTickCount64()>=jnet_character_retry_at && jnet_appearance_context(game,&barn,&save)){
            jnet_character_retry_at=GetTickCount64()+1000;
            jnet_character_switch_native(game,barn,jnet_menu_edit.applied.character,jnet_menu_edit.applied.variant);
        }
    }
    if(!instance->binding.creature || instance->failed || instance->game!=game)return;
    if(instance->initialized){
        unsigned variant=jnet_character_owner?instance->variant:jnet_menu_edit.applied.variant;
        if(jnet_character_variant_limit(instance->kind) && instance->variant_applied!=variant+1){
            unsigned cloth=instance->kind==JNET_CHARACTER_LONGTAIL || instance->kind==JNET_CHARACTER_WHALE;
            uint32_t before=0;
            if(cloth && (!own_read(NULL,instance->pools.events+0x1008,&before,4) || before>=512))return;
            lua_State *state=NULL;
            if(!own_read(NULL,game+0x20,&state,8) || !state)return;
            int top=gettop(state);char source[192];
            snprintf(source,sizeof(source),"assert(__JNETCharacterInstances[\"%u|%s|%u\"]):setAppearance(%u)",jnet_character_owner,jnet_character_name(instance->kind),instance->skin,variant);
            int ok=jnet_character_lua_chunk(state,source,0);settop(state,top);
            if(ok && cloth){
                uint32_t count=0;uint64_t event=0;
                uint64_t first=instance->pools.decorations+0x10+(uint64_t)instance->pools.decoration_count*0x10d0;
                unsigned meshes=jnet_character_specs[instance->kind].mesh_count;
                static const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20};
                unsigned char actual[sizeof(entry)];
                ok=own_read(NULL,instance->pools.events+0x1008,&count,4) && count==before+1 &&
                    own_read(NULL,instance->pools.events+8+(uint64_t)before*8,&event,8) &&
                    own_read(NULL,fault_image+0x15d270,actual,sizeof(actual)) && !memcmp(actual,entry,sizeof(entry)) &&
                    jnet_character_color_ready(own_read,NULL,fault_image,event,first,meshes,0);
                if(ok){
                    void (*prepare)(uintptr_t)=(void *)(fault_image+0x15d270);prepare(event);
                    ok=jnet_character_color_ready(own_read,NULL,fault_image,event,first,meshes,1);
                }
                if(ok){
                    snprintf(source,sizeof(source),"assert(__JNETCharacterInstances[\"%u|%s|%u\"]):finishAppearance()",jnet_character_owner,jnet_character_name(instance->kind),instance->skin);
                    ok=jnet_character_lua_chunk(state,source,0);settop(state,top);
                }
            }
            if(!ok){instance->failure_stage="creature appearance";jnet_character_fail(instance);return;}
            instance->variant_applied=variant+1;
        }
        if(!instance->owner && !jnet_menu_edit.awaiting && jnet_menu_edit.applied.character==instance->kind && !jnet_appearance_applied_at){
            jnet_appearance_applied_at=GetTickCount64();jnet_appearance_status[0]=0;
        }
        jnet_character_visual_report(instance);return;
    }
    unsigned char initialized=0;lua_State *state=NULL;
    if(!jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation) ||
       !own_read(NULL,instance->binding.creature+8,&initialized,1) || !initialized ||
       !own_read(NULL,game+0x20,&state,8) || !state){jnet_character_activation_wait(instance,"binding/enable/Lua");return;}
    if(instance->teleport_event){
        int complete=jnet_character_teleport_complete(own_read,NULL,fault_image,
            instance->pools.events,instance->teleport_event,instance->teleport_identity,instance->binding.creature);
        if(complete<0){instance->failure_stage="teleport completion read";jnet_character_fail(instance);return;}
        if(!complete){jnet_character_activation_wait(instance,"teleport");return;}
        uint32_t settling=0;
        if(!own_read(NULL,instance->binding.creature+0x125c,&settling,4)){
            instance->failure_stage="teleport settling read";jnet_character_fail(instance);return;
        }
        if(settling){jnet_character_activation_wait(instance,"teleport settling");return;}
        char line[160];
        snprintf(line,sizeof(line),"[JNET] %s teleport %u consumed and settled",
            jnet_character_name(instance->kind),instance->teleport_identity);
        message(line);
        instance->teleport_event=0;
    }
    int top=gettop(state);
    instance->failure_stage="activation events/visibility";
    instance->failure_detail=0;
    char activate[240];
    snprintf(activate,sizeof(activate),"__JNETCharacterNative=assert(__JNETCharacterInstances[\"%u|%s|%u\"]); local n=__JNETCharacterNative; if not n.started then n:startEvents() end",
        jnet_character_owner,jnet_character_name(instance->kind),instance->skin);
    int ok=jnet_character_lua_chunk(state,activate,0);
    settop(state,top);
    if(ok)ok=jnet_character_visibility(instance,1);
    if(ok){
        unsigned char stopped=0;
        ok=jc_local_windows_write(instance->binding.creature+9,&stopped,1);
    }
    if(ok){
        ok=own_read(NULL,instance->binding.target+0x10,instance->follow_position,sizeof(instance->follow_position));
        instance->follow_position_valid=ok;
    }
    if(ok){
        instance->initialized=1;
        instance->visual_started=GetTickCount64();
        char line[128];
        snprintf(line,sizeof(line),"[JNET] native character %s initialized in generation %llu",jnet_character_name(instance->kind),(unsigned long long)jnet_character_generation);
        message(line);
    }
    else jnet_character_fail(instance);
}
static void jnet_character_war_capture(struct jnet_game_record *out) {
    struct jnet_character_state *instance=&jnet_character_instances[0];
    if(out->character!=JNET_CHARACTER_WAR_MACHINE || !instance->initialized || instance->failed)return;
    out->war_active=instance->attack_busy;
    if(out->war_active){
        float position[3];uint64_t clump=0,items=0,head=0;
        if(jnet_character_head_position(own_read,NULL,&instance->binding,jnet_character_generation,position))
            memcpy(out->sample.state.bytes,position,sizeof(position));
        if(own_read(NULL,instance->binding.creature+0x18,&clump,8) && clump &&
           own_read(NULL,clump+8,&items,8) && items && own_read(NULL,items,&head,8) && head){
            float rotation[4],norm=0;
            if(own_read(NULL,head+0x20,rotation,sizeof(rotation))){
                for(unsigned k=0;k<4;++k)norm+=rotation[k]*rotation[k];
                if(isfinite(norm) && norm>=.99f && norm<=1.01f){
                    memcpy(out->rotation,rotation,sizeof(rotation));out->rotation_known=1;
                }
            }
        }
    }
    float anger=0;
    if(own_read(NULL,instance->eye_event+0x88,&anger,4) && isfinite(anger))out->war_anger=fmaxf(0,fminf(1,anger));
    memcpy(out->war_hits,instance->war_hits,sizeof(out->war_hits));
}
static int jnet_character_rotation(float *rotation) {
    struct jnet_character_state *instance=&jnet_character_instances[0];
    if(!instance->initialized || instance->failed || instance->kind!=jnet_menu_edit.applied.character ||
       !own_read(NULL,instance->binding.target+0x20,rotation,16))return 0;
    float norm=0;for(unsigned i=0;i<4;++i){if(!isfinite(rotation[i]))return 0;norm+=rotation[i]*rotation[i];}
    return isfinite(norm) && norm>=.99f && norm<=1.01f;
}
static int jnet_character_ride_pose(struct jnet_character_state *instance,float position[3],float rotation[4]) {
    uint64_t clump=0,items=0,head=0;
    if(!instance->initialized || instance->failed || instance->kind!=JNET_CHARACTER_LONGTAIL ||
       !jnet_character_head_position(own_read,NULL,&instance->binding,jnet_character_generation,position) ||
       !own_read(NULL,instance->binding.creature+0x18,&clump,8) || !clump ||
       !own_read(NULL,clump+8,&items,8) || !items || !own_read(NULL,items,&head,8) || !head ||
       !own_read(NULL,head+0x20,rotation,16))return 0;
    float norm=0;
    for(unsigned k=0;k<4;++k){if(!isfinite(rotation[k]))return 0;norm+=rotation[k]*rotation[k];}
    return norm>=.99f && norm<=1.01f;
}
static void jnet_character_ride_capture(struct jnet_game_record *out) {
    out->carrier=out->carrier_lease=0;
    memset(out->ride_offset,0,sizeof(out->ride_offset));
    if(out->character!=JNET_CHARACTER_WAYFARER || traveler_network.paused || jnet_travel_busy)return;
    for(unsigned owner=1;owner<3;++owner){
        struct jnet_character_state *holder=&jnet_character_instances[owner];
        struct jc_network_peer *peer=&traveler_network.peers[owner-1];
        uint64_t attached=0;float position[3],rotation[4],rider[3],offset[3];
        if(!peer->connection.admitted || holder->grab_blocked || holder->game!=traveler_game ||
           !holder->grab_event || !own_read(NULL,holder->grab_event+0x90,&attached,8) ||
           attached!=holder->binding.creature || !jnet_character_ride_pose(holder,position,rotation))continue;
        memcpy(rider,out->sample.state.bytes,sizeof(rider));
        for(unsigned k=0;k<3;++k)offset[k]=rider[k]-position[k];
        rotation[0]=-rotation[0];rotation[1]=-rotation[1];rotation[2]=-rotation[2];
        jnet_character_ride_rotate(rotation,offset,out->ride_offset);
        float distance=0;for(unsigned k=0;k<3;++k)distance+=out->ride_offset[k]*out->ride_offset[k];
        if(!isfinite(distance) || distance>400){memset(out->ride_offset,0,sizeof(out->ride_offset));return;}
        out->carrier=peer->connection.peer;out->carrier_lease=peer->connection.remote_epoch;return;
    }
}
static int jnet_character_ride_resolve(const struct jnet_game_record *r,float position[3]) {
    uint64_t now=GetTickCount64(),identity=0;int fresh=0;
    if(!r->carrier || r->character!=JNET_CHARACTER_WAYFARER || traveler_network.paused || jnet_travel_busy ||
       now<r->seen || now-r->seen>=500)return 0;
    AcquireSRWLockExclusive(&jnet_sg_lock);
    identity=jnet_sg_mail.identity;
    if(jnet_sg_mail.room)for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i){
        const struct jnet_game_record *carrier=&jnet_sg_mail.records[i];
        if(carrier->id==r->carrier && carrier->lease==r->carrier_lease && carrier->sequence &&
           carrier->character_known && carrier->character==JNET_CHARACTER_LONGTAIL && carrier->grab &&
           carrier->sample.chapter==r->sample.chapter && now>=carrier->seen && now-carrier->seen<500)fresh=1;
    }
    ReleaseSRWLockExclusive(&jnet_sg_lock);
    if(!fresh)return 0;
    unsigned owner=3;
    if(r->carrier==identity){if(!jnet_grab_down)return 0;owner=0;}
    else for(unsigned slot=0;slot<2;++slot){
        struct jc_network_peer *peer=&traveler_network.peers[slot];
        if(peer->connection.admitted && peer->connection.peer==r->carrier &&
           peer->connection.remote_epoch==r->carrier_lease)owner=slot+1;
    }
    if(owner==3)return 0;
    float rotation[4],offset[3];
    if(!jnet_character_ride_pose(&jnet_character_instances[owner],position,rotation))return 0;
    jnet_character_ride_rotate(rotation,r->ride_offset,offset);
    for(unsigned k=0;k<3;++k)position[k]+=offset[k];
    return 1;
}
static void jnet_character_remote_tick(uintptr_t game) {
    static uint64_t identities[2],leases[2],retry[2],hit_received[2];
    struct jnet_game_record records[JNET_STEAM_CAPACITY];uint64_t room=0;
    AcquireSRWLockExclusive(&jnet_sg_lock);
    room=jnet_sg_mail.room;memcpy(records,jnet_sg_mail.records,sizeof(records));
    for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)for(unsigned e=0;e<JNET_STEAM_CAPACITY;++e){
        const struct jnet_game_record *event=&jnet_sg_mail.war_events[e];
        if(records[i].id && event->id==records[i].id && event->lease==records[i].lease &&
           event->sample.chapter==records[i].sample.chapter && records[i].character==JNET_CHARACTER_WAR_MACHINE){
            for(unsigned h=0;h<2;++h)if(event->war_hits[h].sequence>records[i].war_hits[h].sequence)
                records[i].war_hits[h]=event->war_hits[h];
        }
    }
    ReleaseSRWLockExclusive(&jnet_sg_lock);
    uint64_t now=GetTickCount64();
    for(unsigned slot=0;slot<2;++slot){
        struct jc_network_peer *peer=&traveler_network.peers[slot];
        uintptr_t dude=slot?traveler_barn+0x146f0:traveler_object;
        unsigned ready=slot?traveler_local_state==2:
            traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE;
        const struct jnet_game_record *record=NULL;
        if(room && ready && dude && !traveler_network.paused && peer->connection.admitted)
            for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)
                if(records[i].id==peer->connection.peer && records[i].lease==peer->connection.remote_epoch &&
                   records[i].sample.chapter==jnet_current_level && records[i].sequence &&
                   now>=records[i].seen && now-records[i].seen<2000 && records[i].character_known)
                    record=records+i;
        jnet_character_owner=slot+1;
        struct jnet_character_state *instance=&jnet_character_instance;
        unsigned kind=record && jnet_character_is_creature(record->character)?record->character:0;
        uint64_t identity=peer->connection.admitted?peer->connection.peer:0;
        uint64_t lease=peer->connection.admitted?peer->connection.remote_epoch:0;
        if(identities[slot]!=identity || leases[slot]!=lease){
            instance->grab_blocked=0;hit_received[slot]=0;
            memset(&instance->remote_pose,0,sizeof(instance->remote_pose));
            for(unsigned cached=0;cached<JNET_CHARACTER_COUNT*3;++cached){
                jnet_character_cached[cached].grab_blocked=0;
                memset(&jnet_character_cached[cached].remote_pose,0,sizeof(jnet_character_cached[cached].remote_pose));
            }
            if(instance->binding.creature){
                unsigned char stopped=1;
                if(instance->game==game && jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation)){
                    jnet_character_visibility(instance,0);
                    jc_local_windows_write(instance->binding.creature+9,&stopped,1);
                    if(dude==instance->barn+0xb0)jnet_character_restore_wayfarer(instance);
                    if(!instance->failed)jnet_character_cached[instance->kind+JNET_CHARACTER_COUNT*instance->skin]=*instance;
                }
                memset(instance,0,sizeof(*instance));
            }
            identities[slot]=identity;leases[slot]=lease;retry[slot]=0;
        }
        if(!kind){
            if(instance->binding.creature)jnet_character_switch_native(game,instance->barn,0,0);
            continue;
        }
        if((!instance->binding.creature || instance->failed || instance->kind!=kind ||
            instance->skin!=(kind==JNET_CHARACTER_WAR_MACHINE?record->variant:0) ||
            instance->game!=game || instance->binding.generation!=jnet_character_generation ||
            instance->barn!=dude-0xb0) && now<retry[slot])continue;
        if(!jnet_character_switch_native(game,dude-0xb0,kind,record->variant)){
            retry[slot]=now+1000;
            if(instance->binding.creature && !instance->failed)
                jnet_character_switch_native(game,instance->barn,JNET_CHARACTER_WAYFARER,0);
            continue;
        }
        instance->variant=record->variant;
        instance->control.spotlight=record->spotlight;
        instance->remote_anger=record->war_anger;
        instance->attack_busy=record->war_active;
        if(!jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation)){
            instance->failure_stage="remote creature binding";jnet_character_fail(instance);continue;
        }
        jnet_character_tick_native(game);
        if(instance->initialized && !instance->failed && kind==JNET_CHARACTER_WAR_MACHINE){
            uint64_t local_id=0,local_lease=0,events=0;
            AcquireSRWLockExclusive(&jnet_sg_lock);local_id=jnet_sg_mail.identity;ReleaseSRWLockExclusive(&jnet_sg_lock);
            for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)if(records[i].id==local_id)local_lease=records[i].lease;
            for(unsigned i=0;i<2;++i){const struct jnet_war_hit *hit=&record->war_hits[i];
                if(hit->victim!=local_id || hit->lease!=local_lease || !local_lease ||
                   hit->sequence<=hit_received[slot] || now<hit->seen || now-hit->seen>=2000)continue;
                hit_received[slot]=hit->sequence;
                if(jnet_menu_edit.applied.character!=JNET_CHARACTER_WAYFARER ||
                   (jnet_character_instances[0].binding.creature && !jnet_character_instances[0].failed))continue;
                if(!own_read(NULL,game+0x250,&events,8) || !events)continue;
                jnet_character_hit_original(instance->attack_event,traveler_barn+0xb0,0,0,events);
                float velocity[4],impulse[4];
                if(own_read(NULL,traveler_barn+0xb0+0x1d80,velocity,sizeof(velocity)) &&
                   own_read(NULL,fault_image+0x68bab0,impulse,sizeof(impulse))){
                    for(unsigned k=0;k<4;++k)velocity[k]+=impulse[k];
                    jc_local_windows_write(traveler_barn+0xb0+0x1d80,velocity,sizeof(velocity));
                }
            }
        }
        if(!instance->initialized || instance->failed)continue;
        float position[3],yaw=0,rotation[4];
        if(!own_read(NULL,dude+0x10,position,sizeof(position)) || !own_read(NULL,dude+0x30,&yaw,4) || !isfinite(yaw))continue;
        int finite=1;for(unsigned i=0;i<3;++i)finite&=isfinite(position[i]);if(!finite)continue;
        rotation[0]=rotation[2]=0;rotation[1]=sinf(yaw*.5f);rotation[3]=cosf(yaw*.5f);
        if(record->rotation_known)memcpy(rotation,record->rotation,sizeof(rotation));
        if(record->war_active)memcpy(position,record->sample.state.bytes,sizeof(position));
        if(kind==JNET_CHARACTER_WAR_MACHINE){
            memcpy(position,record->sample.state.bytes,sizeof(position));
            if(!jnet_remote_pose_update(&instance->remote_pose,record->sequence,record->seen,now,position,rotation))continue;
        }
        float hidden=1;unsigned char no_feet=0;
        if(!jc_local_windows_write(instance->binding.target+0x10,position,sizeof(position)) ||
           !jc_local_windows_write(instance->binding.target+0x20,rotation,sizeof(rotation)) ||
           !jc_local_windows_write(dude+0x1e00,&hidden,4) ||
           !jc_local_windows_write(dude+0x2000,&hidden,4) ||
           !jc_local_windows_write(dude+0x9c34,&no_feet,1) ||
           !jc_local_windows_write(dude+0x9cc4,&no_feet,1))jnet_character_fail(instance);
    }
    for(unsigned slot=0;slot<2;++slot){
        struct jc_network_peer *peer=&traveler_network.peers[slot];
        if(!peer->connection.admitted)continue;
        uintptr_t rider=slot?traveler_barn+0x146f0:traveler_object;
        for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i){float attached[3];
            if(records[i].id==peer->connection.peer && records[i].lease==peer->connection.remote_epoch &&
               jnet_character_ride_resolve(records+i,attached))jc_local_windows_write(rider+0x10,attached,sizeof(attached));
        }
    }
    jnet_character_owner=0;
}
static struct jnet_character_state *jnet_character_creature_for_dude(uintptr_t dude) {
    for(unsigned owner=0;owner<3;++owner){
        struct jnet_character_state *instance=&jnet_character_instances[owner];
        if(instance->initialized && !instance->failed && instance->binding.creature &&
           dude==instance->barn+0xb0)return instance;
    }
    return NULL;
}
static void (*jnet_character_body_input_original)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,unsigned char);
static void jnet_character_body_input(uintptr_t dude,uintptr_t game,uintptr_t axes,uintptr_t direction,uintptr_t peer,unsigned char enabled) {
    struct jnet_character_state *instance=jnet_character_creature_for_dude(dude);
    if(instance && instance->initialized && !instance->failed && instance->game==game &&
       dude==instance->barn+0xb0 &&
       jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation)){
        float zero[4]={0},dt=0;uintptr_t clock=0;unsigned char clear=0;
        if(!own_read(NULL,game+0x68,&clock,8) || !clock || !own_read(NULL,clock+0x10,&dt,4) ||
           !jnet_character_vortex_decay(own_read,jnet_character_write_memory,NULL,dude,dt) ||
           !jc_local_windows_write(dude+0x1bd4,&clear,1) ||
           !jc_local_windows_write(dude+0x20,zero,16) ||
           !jc_local_windows_write(dude+0x34,zero,4) ||
           !jc_local_windows_write(dude+0x3c,zero,4) ||
           !jc_local_windows_write(dude+0x1c00,zero,16))jnet_character_fail(instance);
        return;
    }
    jnet_character_body_input_original(dude,game,axes,direction,peer,enabled);
}
static unsigned char (*jnet_character_touch_original)(uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static unsigned char jnet_character_touch(uintptr_t event,float dt,uintptr_t dude,uintptr_t sounds,
        uintptr_t fifth,uintptr_t sixth,uintptr_t cloth) {
    if(jnet_character_creature_for_dude(dude))return 0;
    return jnet_character_touch_original(event,dt,dude,sounds,fifth,sixth,cloth);
}
static void (*jnet_character_grab_original)(uintptr_t,uintptr_t);
static void jnet_character_grab(uintptr_t event,uintptr_t game) {
    struct jnet_character_state *instance=NULL;unsigned active=0;
    for(unsigned owner=0;owner<3;++owner){
        if(jnet_character_instances[owner].grab_event==event){instance=&jnet_character_instances[owner];active=owner;break;}
        for(unsigned kind=1;kind<JNET_CHARACTER_COUNT*3;++kind)
            if(jnet_character_caches[owner][kind].grab_event==event)instance=&jnet_character_caches[owner][kind];
    }
    if(!instance){jnet_character_grab_original(event,game);return;}
    if(instance->game!=game || !instance->initialized || instance->failed ||
       !jnet_character_binding_valid(own_read,NULL,&instance->binding,jnet_character_generation) ||
       !jnet_character_grab_ready(own_read,NULL,fault_image,event,instance->binding.creature))return;
    int held=0;
    uint64_t now=GetTickCount64();
    if(active && game==traveler_game && traveler_barn && !traveler_network.paused &&
       !jnet_travel_busy && jnet_menu_edit.applied.character==JNET_CHARACTER_WAYFARER){
        unsigned slot=active-1;
        struct jc_network_peer *peer=&traveler_network.peers[slot];
        unsigned ready=slot?traveler_local_state==2:
            traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE;
        AcquireSRWLockExclusive(&jnet_sg_lock);
        if(ready && peer->connection.admitted && jnet_sg_mail.room)
            for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i){
                const struct jnet_game_record *r=&jnet_sg_mail.records[i];
                if(r->id==peer->connection.peer && r->lease==peer->connection.remote_epoch &&
                   r->character_known && r->character==JNET_CHARACTER_LONGTAIL && r->sequence &&
                   r->sample.chapter==jnet_current_level && now>=r->seen && now-r->seen<500){
                    if(!r->grab)instance->grab_blocked=0;
                    held=r->grab && !instance->grab_blocked;
                }
            }
        ReleaseSRWLockExclusive(&jnet_sg_lock);
        for(unsigned owner=1;owner<3 && held;++owner){
            struct jnet_character_state *other=&jnet_character_instances[owner];uint64_t attached=0;
            if(other!=instance && other->grab_event && other->initialized && !other->failed &&
               other->game==game && own_read(NULL,other->grab_event+0x90,&attached,8) && attached)held=0;
        }
    }
    if(held){jnet_character_grab_original(event,game);return;}
    uint64_t attached=0,clump=0,count=0;unsigned char enforce=0;
    if(!own_read(NULL,event+0x90,&attached,8) || !attached)return;
    if(!own_read(NULL,event+0x48,&clump,8) || !clump ||
       !own_read(NULL,clump+0x10,&count,8) || count!=1 || !own_read(NULL,event+0x84,&enforce,1))return;
    uint64_t empty=0;unsigned char released=0;
    if(!jc_local_windows_write(event+0x84,&released,1))return;
    if(jc_local_windows_write(clump+0x10,&empty,8)){
        jnet_character_grab_original(event,game);
        if(!jc_local_windows_write(clump+0x10,&count,8))jnet_character_fail(instance);
    }
    if(!jc_local_windows_write(event+0x84,&enforce,1))jnet_character_fail(instance);
}
static void (*jnet_character_ride_original)(uintptr_t,uintptr_t);
static void (*jnet_character_cloth_ride_original)(uintptr_t,uintptr_t);
static int jnet_character_ride_owner(uintptr_t event,uintptr_t game) {
    for(unsigned owner=0;owner<3;++owner){
        const struct jnet_character_state *instance=&jnet_character_instances[owner];
        if(instance->ride_event==event || instance->grab_event==event)
            return owner && instance->initialized && !instance->failed && instance->game==game;
    }
    for(unsigned owner=0;owner<3;++owner)
        for(unsigned kind=1;kind<JNET_CHARACTER_COUNT*3;++kind)
            if(jnet_character_caches[owner][kind].ride_event==event || jnet_character_caches[owner][kind].grab_event==event)return 0;
    return -1;
}
static void jnet_character_ride(uintptr_t event,uintptr_t game) {
    if(jnet_menu_edit.applied.character!=JNET_CHARACTER_WAYFARER)return;
    uint64_t event_type=0;
    if(!own_read(NULL,event,&event_type,8))return;
    void (*original)(uintptr_t,uintptr_t)=event_type==fault_image+0x679ed8?
        jnet_character_cloth_ride_original:jnet_character_ride_original;
    int owner=jnet_character_ride_owner(event,game);
    if(owner<0){original(event,game);return;}
    if(!owner)return;
    for(unsigned owner=1;owner<3;++owner){
        struct jnet_character_state *holder=&jnet_character_instances[owner];uint64_t attached=0;
        if(holder->initialized && !holder->failed && holder->game==game && holder->grab_event &&
           own_read(NULL,holder->grab_event+0x90,&attached,8) && attached)return;
    }
    if(game!=traveler_game || !traveler_barn || traveler_network.paused)return;
    struct jnet_game_record records[JNET_STEAM_CAPACITY];uint64_t room,now=GetTickCount64();
    AcquireSRWLockExclusive(&jnet_sg_lock);
    room=jnet_sg_mail.room;memcpy(records,jnet_sg_mail.records,sizeof(records));
    ReleaseSRWLockExclusive(&jnet_sg_lock);
    if(!room)return;
    float local[3],nearest=INFINITY;uintptr_t selected=0;
    if(!own_read(NULL,traveler_barn+0xc0,local,sizeof(local)))return;
    for(unsigned k=0;k<3;++k)if(!isfinite(local[k]))return;
    for(unsigned slot=0;slot<2;++slot){
        struct jnet_character_state *instance=&jnet_character_instances[slot+1];
        struct jc_network_peer *peer=&traveler_network.peers[slot];
        unsigned ready=slot?traveler_local_state==2:
            traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE;
        if(!ready || !peer->connection.admitted || !instance->initialized || instance->failed ||
           instance->game!=game || !instance->ride_event || !jnet_character_is_creature(instance->kind))continue;
        int fresh=0,grab=0;
        for(unsigned i=0;i<JNET_STEAM_CAPACITY;++i)
            if(records[i].id==peer->connection.peer && records[i].lease==peer->connection.remote_epoch &&
               records[i].character_known && records[i].character==instance->kind && records[i].sequence &&
               records[i].sample.chapter==jnet_current_level && now>=records[i].seen && now-records[i].seen<500){fresh=1;grab=records[i].grab;}
        uintptr_t ride=instance->kind==JNET_CHARACTER_LONGTAIL && grab?instance->grab_event:instance->ride_event;
        float head[3],distance=0;
        if(!fresh || !jnet_character_head_position(own_read,NULL,&instance->binding,jnet_character_generation,head) ||
           (instance->kind!=JNET_CHARACTER_LONGTAIL &&
            !jnet_character_ride_ready(own_read,NULL,fault_image,ride,instance->binding.creature,1)))continue;
        for(unsigned k=0;k<3;++k){float d=head[k]-local[k];distance+=d*d;}
        if(distance<nearest){nearest=distance;selected=ride;}
    }
    if(selected==event){
        for(unsigned slot=0;slot<2;++slot){
            struct jnet_character_state *instance=&jnet_character_instances[slot+1];
            struct jc_network_peer *peer=&traveler_network.peers[slot];
            if(instance->ride_event!=event && instance->grab_event!=event)continue;
            if(event_type==fault_image+0x6718b0 &&
               (instance->ride_active!=event || instance->ride_peer!=peer->connection.peer || instance->ride_epoch!=peer->connection.remote_epoch ||
                now<instance->ride_tick || now-instance->ride_tick>=500) &&
               !jnet_character_ride_reset(own_read,jnet_character_write_memory,NULL,fault_image,event))return;
            instance->ride_peer=peer->connection.peer;instance->ride_epoch=peer->connection.remote_epoch;
            instance->ride_tick=now;instance->ride_active=event;
            float before[3];uint64_t rider=0;
            int glow=instance->kind==JNET_CHARACTER_WAR_MACHINE &&
                own_read(NULL,game+0x1e0,&rider,8) && rider==traveler_barn &&
                own_read(NULL,rider+0xc0,before,sizeof(before));
            original(event,game);
            if(glow)jnet_character_ride_glow(own_read,jnet_character_write_memory,NULL,rider+0xb0,before);
            break;
        }
    }
}
static void (*jnet_character_link_original)(uintptr_t,uintptr_t,unsigned char);
static void jnet_character_link(uintptr_t emitter,uintptr_t dude,unsigned char enabled) {
    if(jnet_character_creature_for_dude(dude))enabled=0;
    jnet_character_link_original(emitter,dude,enabled);
}
static void (*jnet_character_chest_original)(uintptr_t,float,uintptr_t);
static void jnet_character_chest(uintptr_t dude,float dt,uintptr_t resources) {
    struct jnet_character_state *instance=jnet_character_creature_for_dude(dude);
    if(instance){
        unsigned char hidden=0;
        if(jc_local_windows_write(dude+0xd40,&hidden,1) &&
           jc_local_windows_write(dude+0x13e0,&hidden,1))return;
        jnet_character_fail(instance);
    }
    jnet_character_chest_original(dude,dt,resources);
}
static void (*jnet_character_appearance_original)(uintptr_t,uintptr_t);
static void jnet_character_appearance(uintptr_t dude,uintptr_t barn) {
    jnet_character_appearance_original(dude,barn);
    struct jnet_character_state *instance=jnet_character_creature_for_dude(dude);
    if(instance){
        unsigned char remote=0;float hidden=0;
        if(!own_read(NULL,dude+0x81,&remote,1) ||
           !jc_local_windows_write(fault_image+(remote ? 0x3c941d0 : 0x3c941d4),&hidden,4))
            jnet_character_fail(instance);
    }
}
static void (*jnet_character_shout_original)(uintptr_t,const void *,uintptr_t,unsigned char);
static void jnet_character_shout(uintptr_t barn,const void *input,uintptr_t network,unsigned char send) {
    struct jnet_character_state *instance=&jnet_character_instance;
    _Alignas(16) unsigned char copy[0x40]={0};
    uint64_t expected=0,sounds=0;uint32_t count=0;
    if(instance->initialized && !instance->failed && instance->binding.creature &&
       own_read(NULL,instance->game+0x1b8,&expected,8) && barn==expected &&
       own_read(NULL,(uintptr_t)input,copy,0x38) && copy[0x2c]==1 && !copy[0x2d] &&
       own_read(NULL,instance->game+0x150,&sounds,8) && sounds &&
       own_read(NULL,sounds+0x7c7b4,&count,4) && count && count<=JC_CHIRP_SOUND_CAP){
        uint32_t source=jc_chirp_u32(copy+0x28);
        unsigned char source_name[32]={0};
        if(source<count && own_read(NULL,sounds+0x70234+(uint64_t)source*0x13c,source_name,32) &&
           jc_chirp_sound_kind(source_name)>=1 && jc_chirp_sound_kind(source_name)<=4){
            if(jnet_character_is_creature(instance->kind)){
                float head[3];
                if(jnet_character_head_position(own_read,NULL,&instance->binding,jnet_character_generation,head))
                    memcpy(copy,head,sizeof(head));
            }
            if(jnet_character_large_chirp(instance->kind,jc_chirp_sound_kind(source_name))){
                float radius=15.f,duration=1.5f;
                memcpy(copy+0x20,&radius,4);memcpy(copy+0x24,&duration,4);
            }
            unsigned variant=jnet_character_chirp_variant(instance->kind,instance->chirp_variant++,jc_chirp_sound_kind(source_name));
            const char *name=jc_chirp_sound_name(variant,0);
            if(name)for(uint32_t i=0;i<count;++i){
                unsigned char candidate[32];
                if(own_read(NULL,sounds+0x70234+(uint64_t)i*0x13c,candidate,32) &&
                   memchr(candidate,0,32) && !strcmp((char *)candidate,name)){
                    memcpy(copy+0x28,&i,4);break;
                }
            }
            jnet_character_shout_original(barn,copy,network,send);return;
        }
    }
    jnet_character_shout_original(barn,input,network,send);
}
static void (*jnet_character_camera_original)(uintptr_t,uintptr_t);
static void jnet_character_camera(uintptr_t camera,uintptr_t context) {
    struct jnet_character_state *instance=&jnet_character_instance;
    uint64_t owner=0,dude=0;float original=0,widened=0;
    int changed=instance->initialized && !instance->failed && instance->binding.creature &&
        own_read(NULL,context+0x50,&owner,8) && owner &&
        own_read(NULL,owner+0x18,&dude,8) && dude==instance->barn+0xb0 &&
        own_read(NULL,fault_image+0x3763dac,&original,4) &&
        jnet_character_camera_distance(instance->kind,original,&widened) &&
        jc_local_windows_write(fault_image+0x3763dac,&widened,4);
    jnet_character_camera_original(camera,context);
    if(changed && !jc_local_windows_write(fault_image+0x3763dac,&original,4))
        jnet_character_fail(instance);
}
static void (*jnet_character_reset_original)(uintptr_t);
static void jnet_character_reset_native(uintptr_t barn) {
    ++jnet_character_generation;
    memset(jnet_character_instances,0,sizeof(jnet_character_instances));
    memset(jnet_character_caches,0,sizeof(jnet_character_caches));
    memset(jnet_character_failed_masks,0,sizeof(jnet_character_failed_masks));jnet_character_retry_at=0;
    jnet_character_reset_original(barn);
}
#endif
