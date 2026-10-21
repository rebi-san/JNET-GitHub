#ifndef JNET_FLOWER_LIVE_H
#define JNET_FLOWER_LIVE_H
/* Game-thread half of Flower Trail: plants flowers while the action is held (called from the creature input step), keeps the camera and
   lens current for the render thread, and hides flowers whose line of sight crosses terrain. See jnet_flower_render.h for drawing. */
#include "jnet_flower_trail.h"
#include "jnet_flower_view.h"

static lua_State *jnet_flower_state;
static float jnet_flower_lens[4],jnet_flower_player[3];
static unsigned jnet_flower_lens_valid,jnet_flower_player_valid,jnet_flower_reported,jnet_flower_lens_failed;
static unsigned jnet_flower_planted_level=~0u;
static uint64_t jnet_flower_camera_log_next;
static uint64_t jnet_flower_lens_next,jnet_flower_probe_next;
/* Lua's GetWorldMatrix, sampled a few times, tells where the camera position really is: its row 4 and column 4 are the two possible
   translation vectors. The node/camera memory is then searched for those values. 0 = not yet, 1 = located, 2 = not found. */
static float jnet_flower_probe[6];
static unsigned jnet_flower_probe_ready,jnet_flower_probe_tries,jnet_flower_calibrated;
static unsigned jnet_flower_position_base;
static size_t jnet_flower_position_offset;

static int jnet_flower_lens_callback(lua_State *state) {
    float values[4];
    for(unsigned i=0;i<4;++i){
        const char *text=tolstring(state,(int)i+1,NULL);
        if(!text)return 0;
        if(!jnet_flower_number(text,&values[i]))return 0;
    }
    memcpy(jnet_flower_lens,values,sizeof(values));jnet_flower_lens_valid=1;
    return 0;
}
static int jnet_flower_probe_callback(lua_State *state) {
    float values[6];
    for(unsigned i=0;i<6;++i){
        const char *text=tolstring(state,(int)i+1,NULL);
        if(!text || !jnet_flower_number(text,&values[i]))return 0;
    }
    memcpy(jnet_flower_probe,values,sizeof(values));jnet_flower_probe_ready=1;
    return 0;
}
/* Adds __JNETFlowerLens() to a Lua state: it hands the camera's lens (degrees, aspect, near, far) to native code as four numbers, so it
   allocates nothing that the game's fixed Lua pools would have to hold. */
static void jnet_flower_install(lua_State *state) {
    static const char source[]=
        "local lens,probe=...\n"
        "function __JNETFlowerLens()\n"
        "local node=game:cameraSystem():GetCameraNode()\n"
        "lens(node:GetFov(),node:GetAspect(),node:GetNearPlane(),node:GetFarPlane())\n"
        "end\n"
        "function __JNETFlowerProbe()\n"
        "local m=game:cameraSystem():GetCameraNode():GetWorldMatrix()\n"
        "probe(m[4][1],m[4][2],m[4][3],m[1][4],m[2][4],m[3][4])\n"
        "end";
    if(jnet_flower_state==state)return;
    int top=gettop(state);
    int ok=original_loadbuffer(state,source,sizeof(source)-1,"JNET-flower-lens","t")==0;
    if(ok){pushclosure(state,jnet_flower_lens_callback,0);pushclosure(state,jnet_flower_probe_callback,0);ok=pcallfn(state,2,0,0,0,NULL)==0;}
    settop(state,top);
    if(ok){jnet_flower_state=state;jnet_flower_lens_valid=0;jnet_flower_lens_failed=0;jnet_flower_calibrated=jnet_flower_probe_ready=jnet_flower_probe_tries=0;}
    else message("[JNET] flower trail camera script could not be installed");
}
/* Terrain height under a position, from the native sampler the creature movement also uses. `context` is the game object. */
static int jnet_flower_ground(void *context,const float position[3],float *height) {
    uint64_t terrain=0;
    if(!jnet_character_terrain_binding(own_read,NULL,(uintptr_t)context,&terrain))return 0;
    float (*sample)(float,float,uintptr_t,float,uintptr_t,unsigned char)=(void *)(fault_image+0x82340);
    *height=sample(position[0],position[2],terrain,0,0,0);
    return isfinite(*height);
}
static int evaluation_writable(void *opaque,uint64_t address,size_t bytes);
static uintptr_t jnet_flower_game;
static uint32_t jnet_flower_sound_next;
static unsigned jnet_flower_sound_sequence,jnet_flower_sound_reported;
/* Journey's own response for the Desert flower is a "Flower" shout (sound FlowerRespond in the LvlSfx bank). Emits one at `position`
   through the same native shout enqueue the companion chirps use; any step that declines is skipped, and the first failure is logged. */
static void jnet_flower_respond(const float position[3]) {
    uint32_t now=(uint32_t)GetTickCount64();
    if(!jnet_flower_game || jnet_flower_elapsed(now,jnet_flower_sound_next)<0)return;
    jnet_flower_sound_next=now+350;
    struct jc_chirp_event event={.participant=1,.epoch=1,.sequence=++jnet_flower_sound_sequence,.sound=23,.radius=1.f,.duration=.4f};
    memcpy(event.position,position,sizeof(event.position));
    struct jc_chirp_bridge bridge;
    int code=jc_chirp_bridge_prepare_event(own_read,evaluation_writable,NULL,fault_image,jnet_flower_game,(uint32_t)JNETFrames,&event,&bridge);
    if(!code){
        void (*shout)(uintptr_t,const void *,uintptr_t,unsigned char)=(void *)(fault_image+JC_RVA_NativeShout);
        shout((uintptr_t)bridge.barn,bridge.input,0,0);
        code=jc_chirp_bridge_post(own_read,NULL,(uint32_t)JNETFrames,&bridge);
    }
    if((code || !jnet_flower_sound_reported) && jnet_flower_sound_reported<3){
        ++jnet_flower_sound_reported;char line[110];
        snprintf(line,sizeof(line),"[JNET] flower trail response sound %s (code %d)",code?"declined":"played",code);message(line);
    }
}
static void jnet_flower_after_motion(unsigned kind,const float position[3],float dt,jnet_flower_floor_function floor,void *context) {
    if(!jnet_flower_finite(position))return;
    memcpy(jnet_flower_player,position,sizeof(jnet_flower_player));jnet_flower_player_valid=1;
    uint32_t now=(uint32_t)GetTickCount64();
    AcquireSRWLockExclusive(&jnet_flower_lock);
    unsigned planted=jnet_flower_trail_step(&jnet_flower_trail_state,jnet_current_level,kind,jnet_flower_trail_down,position,dt,floor,context,now);
    float woken[3]={0};
    unsigned touched=jnet_flower_trail_touch(&jnet_flower_trail_state,position,now,woken);
    for(unsigned n=1;n<=planted;++n)jnet_flower_hidden[(jnet_flower_trail_state.next+JNET_FLOWER_CAPACITY-n)%JNET_FLOWER_CAPACITY]=0;
    if(!jnet_flower_trail_state.count)memset(jnet_flower_hidden,0,sizeof(jnet_flower_hidden));
    unsigned level=jnet_flower_trail_state.level;
    ReleaseSRWLockExclusive(&jnet_flower_lock);
    if(touched)jnet_flower_respond(woken);
    if(planted && jnet_flower_planted_level!=level){
        jnet_flower_planted_level=level;
        char line[96];snprintf(line,sizeof(line),"[JNET] flower trail planting on level %u",level);message(line);
    }
}
static void jnet_flower_lens_poll(uintptr_t game,uint64_t now) {
    lua_State *state=NULL;
    if(jnet_flower_lens_failed || now<jnet_flower_lens_next || !own_read(NULL,game+0x20,&state,8) || !state || state!=jnet_flower_state)return;
    jnet_flower_lens_next=now+250;
    static const char call[]="__JNETFlowerLens()";
    int top=gettop(state);
    int ok=original_loadbuffer(state,call,sizeof(call)-1,"JNET-flower-lens-call","t")==0 && pcallfn(state,0,0,0,0,NULL)==0;
    if(!ok){
        const char *error=tolstring(state,-1,NULL);char line[240];
        snprintf(line,sizeof(line),"[JNET] flower trail camera lens unavailable: %.160s",error?error:"Lua error");message(line);
        jnet_flower_lens_failed=1;
    }
    settop(state,top);
    if(jnet_flower_calibrated || jnet_flower_probe_ready || now<jnet_flower_probe_next)return;
    jnet_flower_probe_next=now+1000;
    if(++jnet_flower_probe_tries>4){jnet_flower_calibrated=2;message("[JNET] flower trail camera calibration: no world matrix received");return;}
    static const char probe[]="__JNETFlowerProbe()";
    top=gettop(state);
    ok=original_loadbuffer(state,probe,sizeof(probe)-1,"JNET-flower-probe-call","t")==0 && pcallfn(state,0,0,0,0,NULL)==0;
    if(!ok){
        const char *error=tolstring(state,-1,NULL);char line[240];
        snprintf(line,sizeof(line),"[JNET] flower trail camera calibration unavailable: %.150s",error?error:"Lua error");message(line);
        jnet_flower_calibrated=2;
    }
    settop(state,top);
}
/* Looks for the position Lua reported inside the camera node and then the camera object (0x600 bytes each), as row 4 or as column 4
   of the world matrix, and uses the first hit as the camera position from then on. */
static void jnet_flower_calibrate(uintptr_t camera_object,uintptr_t node) {
    static unsigned char memory[0x600];
    const uintptr_t bases[2]={node,camera_object};
    char line[300];
    for(unsigned base=0;base<2;++base){
        size_t size=sizeof(memory);
        while(size>=0x100 && !own_read(NULL,bases[base],memory,size))size-=0x100;
        if(size<0x100)continue;
        for(unsigned candidate=0;candidate<2;++candidate){
            size_t offset=0;
            if(!jnet_flower_find_vector(memory,size,jnet_flower_probe+3*candidate,.05f,&offset))continue;
            jnet_flower_position_base=base;jnet_flower_position_offset=offset;jnet_flower_calibrated=1;
            snprintf(line,sizeof(line),"[JNET] flower trail camera calibration: Lua %s %.1f %.1f %.1f found in %s at +0x%x",
                candidate?"column 4":"row 4",(double)jnet_flower_probe[3*candidate],(double)jnet_flower_probe[3*candidate+1],
                (double)jnet_flower_probe[3*candidate+2],base?"camera object":"camera node",(unsigned)offset);
            message(line);return;
        }
    }
    jnet_flower_calibrated=2;
    snprintf(line,sizeof(line),"[JNET] flower trail camera calibration: Lua row 4 %.1f %.1f %.1f / column 4 %.1f %.1f %.1f not found in node or camera object",
        (double)jnet_flower_probe[0],(double)jnet_flower_probe[1],(double)jnet_flower_probe[2],
        (double)jnet_flower_probe[3],(double)jnet_flower_probe[4],(double)jnet_flower_probe[5]);
    message(line);
}
/* The local player's chirps (any creature or the Wayfarer) open the flowers in range, as a chirp near the Desert's flower fires its event.
   Read from the native shout slots with a private cursor, so it never disturbs the chirp relay's own. */
static struct jc_chirp_capture jnet_flower_chirps;
static unsigned jnet_flower_chirp_logged;
static void jnet_flower_chirp_poll(uintptr_t game) {
    struct jc_chirp_batch batch;
    if(jc_chirp_capture_local(own_read,NULL,game,(uint32_t)JNETFrames,1,1,&jnet_flower_chirps,&batch) || !batch.count)return;
    uint32_t now=(uint32_t)GetTickCount64();unsigned reached=0,woke=0;float woken[3]={0},first[3]={0};
    AcquireSRWLockExclusive(&jnet_flower_lock);
    for(unsigned i=0;i<batch.count;++i){
        unsigned n=jnet_flower_trail_chirp(&jnet_flower_trail_state,batch.source[i].event.position,batch.source[i].event.radius,now,first);
        if(n && !woke)memcpy(woken,first,sizeof(woken));
        reached+=n;woke+=n;
    }
    ReleaseSRWLockExclusive(&jnet_flower_lock);
    if(woke)jnet_flower_respond(woken);
    if(jnet_flower_chirp_logged<4){
        ++jnet_flower_chirp_logged;char line[120];
        snprintf(line,sizeof(line),"[JNET] flower trail chirp: %u flowers in range of %u chirps",reached,batch.count);message(line);
    }
}
/* Per game tick: publishes the camera to the render thread and refreshes terrain hiding for a slice of the flowers. */
static void jnet_flower_tick(uintptr_t game) {
    AcquireSRWLockShared(&jnet_flower_lock);unsigned count=jnet_flower_trail_state.count;ReleaseSRWLockShared(&jnet_flower_lock);
    if(!count)return;
    uint64_t now=GetTickCount64();
    jnet_flower_game=game;
    jnet_flower_chirp_poll(game);
    jnet_flower_lens_poll(game,now);
    uintptr_t camera_object=0,node=0;float rows[16];struct jnet_flower_camera camera;
    if(!jnet_flower_lens_valid || !jnet_flower_player_valid || !own_read(NULL,game+0x60,&camera_object,8) || !camera_object ||
       !own_read(NULL,camera_object+0x30,&node,8) || !node || !own_read(NULL,node+0x170,rows,sizeof(rows)))return;
    if(jnet_flower_probe_ready && !jnet_flower_calibrated)jnet_flower_calibrate(camera_object,node);
    if(jnet_flower_calibrated==1){
        float position[3];
        if(own_read(NULL,(jnet_flower_position_base?camera_object:node)+jnet_flower_position_offset,position,sizeof(position)))memcpy(rows+12,position,sizeof(position));
    }
    if(!jnet_flower_camera_rows(&camera,rows,jnet_flower_player) ||
       !jnet_flower_camera_lens(&camera,jnet_flower_lens[0],jnet_flower_lens[1],jnet_flower_lens[2],jnet_flower_lens[3])){
        if(!jnet_flower_reported){
            /* The node layout was inferred, not proven. Record what was read so the offsets can be corrected from a session report. */
            jnet_flower_reported=1;float dump[24];
            char line[512];
            snprintf(line,sizeof(line),"[JNET] flower trail camera rejected: lens %.4g %.4g %.4g %.4g rows %.3g %.3g %.3g %.3g | %.3g %.3g %.3g %.3g | %.3g %.3g %.3g %.3g | %.3g %.3g %.3g %.3g",
                (double)jnet_flower_lens[0],(double)jnet_flower_lens[1],(double)jnet_flower_lens[2],(double)jnet_flower_lens[3],
                (double)rows[0],(double)rows[1],(double)rows[2],(double)rows[3],(double)rows[4],(double)rows[5],(double)rows[6],(double)rows[7],
                (double)rows[8],(double)rows[9],(double)rows[10],(double)rows[11],(double)rows[12],(double)rows[13],(double)rows[14],(double)rows[15]);
            message(line);
            if(own_read(NULL,node+0x140,dump,sizeof(dump))){
                snprintf(line,sizeof(line),"[JNET] flower trail camera node +0x140: %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g %.3g",
                    (double)dump[0],(double)dump[1],(double)dump[2],(double)dump[3],(double)dump[4],(double)dump[5],(double)dump[6],(double)dump[7],
                    (double)dump[8],(double)dump[9],(double)dump[10],(double)dump[11],(double)dump[12],(double)dump[13],(double)dump[14],(double)dump[15],
                    (double)dump[16],(double)dump[17],(double)dump[18],(double)dump[19],(double)dump[20],(double)dump[21],(double)dump[22],(double)dump[23]);
                message(line);
            }
        }
        return;
    }
    if(now>=jnet_flower_camera_log_next){
        jnet_flower_camera_log_next=now+30000;char line[260];
        snprintf(line,sizeof(line),"[JNET] flower trail camera ok (%s): position %.1f %.1f %.1f player %.1f %.1f %.1f fov %.1f aspect %.3f near %.3g far %.4g flowers %u",
            jnet_flower_calibrated==1?"calibrated":"unverified +0x1a0",
            (double)camera.position[0],(double)camera.position[1],(double)camera.position[2],
            (double)jnet_flower_player[0],(double)jnet_flower_player[1],(double)jnet_flower_player[2],(double)camera.fov,
            (double)camera.aspect,(double)camera.near_plane,(double)camera.far_plane,count);
        message(line);
    }
    static unsigned cursor;unsigned char hidden[32];unsigned slice=count<32?count:32,first=cursor%count;
    for(unsigned k=0;k<slice;++k)
        hidden[k]=!jnet_flower_line_clear(&camera,&jnet_flower_trail_state.items[(first+k)%count],jnet_flower_ground,(void *)game);
    cursor=(first+slice)%count;
    AcquireSRWLockExclusive(&jnet_flower_lock);
    jnet_flower_camera_state=camera;jnet_flower_camera_time=now;
    for(unsigned k=0;k<slice && count==jnet_flower_trail_state.count;++k)jnet_flower_hidden[(first+k)%count]=hidden[k];
    ReleaseSRWLockExclusive(&jnet_flower_lock);
}
#endif
