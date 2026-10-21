#include <winsock2.h>
#define JC_STREAM_UDP 1
#define JC_NETWORK_UDP 1
#include "platform.h"
#include "MinHook.h"
#include "target_build.h"
#include "embedded_probe.h"
#include "startup_candidate.h"
#if !JC_NATIVE_CHARACTERS_ENABLED
_Static_assert(JC_STARTUP_CANDIDATE_SIZE==JC_STARTUP_SIZE,"Recovery startup must retain the native resource buffer size");
_Static_assert(sizeof(jc_character_startup)==1,"Recovery startup cannot append creature resources");
#endif
#include "native_resources.h"
#include "native_lifetime.h"
#include "native_clone_pool.h"
#include "native_geometry_pool.h"
#include "native_material_ready.h"
#include "native_shader_inputs.h"
#include "native_appearance.h"
#include "native_trial_shader.h"
#include "native_mesh_callbacks.h"
#include "native_mesh_guard.h"
#include "native_mesh_rehearsal.h"
#include "native_pose_storage.h"
#include "native_pose_writes.h"
#include "native_animation_storage.h"
#include "native_idle_pose.h"
#include "native_read_batch.h"
#include "native_local_read.h"
#include "native_render_queue.h"
#include "native_initialized_lifetime.h"
#include "diagnostic_checkpoint.h"
#include "native_dual_body.h"
#include "native_skeleton_mirror.h"
#include "native_evaluation_layout.h"
#include "native_evaluation_run.h"
#include "native_companion_chirp.h"
#include "native_companion_cloth.h"
#include "native_scarf_growth.h"
#include "diagnostic_fault.h"
#include "native_lifecycle_hooks.h"
#include "native_cloth_material_live.h"
#include "native_render_retirement.h"
#include "native_cloth_live.h"
#include "native_cloth_prelaunch.h"
#include "native_cloth_producer.h"
#include "native_actor_frame.h"
#include "native_chapter_gate.h"

static struct jc_live_retirement cloth_retirement;
static struct jc_actor_frame actor_frame;
static struct jc_cloth_live *active_cloth_backend;
static uint32_t (*original_vertex_map)(uintptr_t,uintptr_t);
static uint32_t (*original_index_map)(uintptr_t,uintptr_t);
typedef void (*jc_native_cloth_worker)(void *,const void *,void *,void *,void *,void *);
static jc_native_cloth_worker original_cloth_worker[2];
static uint32_t (*original_cloth_dependency)(uintptr_t);
static uintptr_t (*original_cloth_finish)(uintptr_t,void *,unsigned,uintptr_t);
static void (*original_cloth_render)(uintptr_t,unsigned,uintptr_t);
static DWORD cloth_production_thread;
static unsigned cloth_production_pass;
static struct jc_cloth_producer cloth_producer;
static struct jc_cloth_producer_participant cloth_participant;
static uint64_t cloth_production_serial,cloth_production_job;
static unsigned cloth_participant_ready;
static int cloth_produce_before_dependency(uintptr_t dependency);
static void cloth_publish_at_render(uintptr_t barn,unsigned pass,uintptr_t context);
static void traveler_actor_render(uintptr_t barn,uintptr_t context);
static void traveler_actor_chirp_tick(void);
static int traveler_body_release(uint64_t actor);
static void traveler_actor_chirp_report(void);
static SRWLOCK cloth_map_lock=SRWLOCK_INIT;
static struct {
    uintptr_t barn,game,objects[7][2];
    uint64_t generation;
    unsigned index,roles,seen[2],launched_roles,finish_failed;
    volatile LONG worker_reported;
    struct jc_cloth_ring_checkpoint rings[7][2];
} cloth_maps;
static SRWLOCK cloth_appearance_lock=SRWLOCK_INIT;
static struct jc_cloth_appearance_route cloth_appearance_scope;
static DWORD cloth_appearance_thread;
static volatile LONG cloth_appearance_enabled;
static uintptr_t (*original_cloth_setter[3])(uintptr_t,uintptr_t,uintptr_t);
static uintptr_t (*original_cloth_lookup)(uintptr_t,uintptr_t);
static uintptr_t (*original_actor_material_commit)(uintptr_t);
static void traveler_glyph_trace_bind(unsigned,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static void traveler_glyph_trace_commit(uintptr_t,int);
static void traveler_glyph_trace_queue(uintptr_t);
static void traveler_glyph_trace_dispatch(uintptr_t,uint32_t);
static int traveler_glyph_trace_build(uintptr_t);
static void traveler_glyph_trace_built(int,uint32_t,uint64_t,uint64_t,uint32_t);
static void traveler_glyph_trace_report(void);
static unsigned char (*original_actor_sampler_init)(uintptr_t,const uint64_t *);
static int traveler_actor_material_write_allowed(uintptr_t object,unsigned payload);
static int traveler_actor_sampler_write_allowed(uintptr_t material,const uint64_t *model_pair);
static uintptr_t cloth_appearance_destination(uintptr_t object,unsigned lookup) {
    if (!InterlockedCompareExchange(&cloth_appearance_enabled,0,0)) return object;
    AcquireSRWLockShared(&cloth_appearance_lock);
    uintptr_t target=cloth_appearance_thread==GetCurrentThreadId() ?
        jc_cloth_appearance_target(&cloth_appearance_scope,object,lookup) : object;
    ReleaseSRWLockShared(&cloth_appearance_lock);
    return target;
}
static uintptr_t cloth_vector_setter(uintptr_t material,uintptr_t name,uintptr_t value) {
    uintptr_t target=cloth_appearance_destination(material,0);
    return traveler_actor_material_write_allowed(target,0) ? original_cloth_setter[0](target,name,value) : 0;
}
static uintptr_t cloth_matrix_setter(uintptr_t material,uintptr_t name,uintptr_t value) {
    uintptr_t target=cloth_appearance_destination(material,0);
    return traveler_actor_material_write_allowed(target,0) ? original_cloth_setter[1](target,name,value) : 0;
}
static uintptr_t cloth_texture_setter(uintptr_t material,uintptr_t name,uintptr_t value) {
    uintptr_t target=cloth_appearance_destination(material,0);
    return traveler_actor_material_write_allowed(target,0) ? original_cloth_setter[2](target,name,value) : 0;
}
static uintptr_t cloth_scalar_lookup(uintptr_t payload,uintptr_t name) {
    uintptr_t target=cloth_appearance_destination(payload,1);
    return traveler_actor_material_write_allowed(target,1) ? original_cloth_lookup(target,name) : 0;
}
static uintptr_t actor_material_commit_observer(uintptr_t material) {
    int allowed=traveler_actor_material_write_allowed(material,2);
    uintptr_t result=allowed ? original_actor_material_commit(material) : 0;
    traveler_glyph_trace_commit(material,allowed);return result;
}
static unsigned char actor_sampler_init_observer(uintptr_t material,const uint64_t *model_pair) {
    return traveler_actor_sampler_write_allowed(material,model_pair) ?
        original_actor_sampler_init(material,model_pair) : 0;
}
static struct jc_chapter_gate cloth_chapter;
static void (*original_chapter_switch)(uintptr_t,const char *);
#define JC_CHAPTER_SWITCH_RVA 0xda5d0u
#define JC_D3D_CONTEXT_SLOT_RVA 0x40408b8u
static _Alignas(16) unsigned char cloth_progress_command[128];
static volatile LONG cloth_progress_pending;
static void cloth_progress_request(void);
static WNDPROC original_game_window;
static HWND cloth_exit_window;
static WPARAM cloth_exit_wparam;
static LPARAM cloth_exit_lparam;
static UINT_PTR cloth_exit_timer_id;
static int cloth_exit_reported;
static UINT_PTR (WINAPI *cloth_set_timer)(HWND,UINT_PTR,UINT,TIMERPROC);
static BOOL (WINAPI *cloth_kill_timer)(HWND,UINT_PTR);

__declspec(dllexport) volatile LONG JNETStartup[5] = {0};
__declspec(dllexport) volatile LONG JNETStatus = 0;
__declspec(dllexport) volatile LONG JNETCommand = 0;
__declspec(dllexport) volatile LONG JNETObservation = 0;
__declspec(dllexport) volatile LONG JNETPhase = 0;
__declspec(dllexport) volatile LONG JNETFrames = 0;
__declspec(dllexport) volatile LONG JNETTrial = 0;
__declspec(dllexport) volatile LONG JNETLifetime = 0;
__declspec(dllexport) volatile LONG JNETResource = 0;
__declspec(dllexport) volatile LONG JNETPool[5] = {-1,0,0,0,0};
__declspec(dllexport) volatile LONG JNETMesh[6] = {0};
__declspec(dllexport) volatile LONG JNETMaterial = 0;
__declspec(dllexport) volatile LONG JNETCallbacks[11] = {-1,0,0,-1,0,-1,-1,-1,0,0,0};
__declspec(dllexport) volatile LONG JNETMeshLife[152] = {[32]=-1,[40]=-1,[49]=-1,[55]=-1,[57]=-1,[58]=-1,[69]=-1,[81]=-1,[84]=-1,[92]=-1,[93]=-1,[94]=-1,[96]=-1,[97]=-1,[98]=-1,[108]=-1,[110]=-1,[112]=-1,[118]=-1,[127]=-1,[137]=-1,[138]=-1,[144]=-1,[150]=-1};
__declspec(dllexport) volatile LONG JNETRender[23] = {0};
__declspec(dllexport) volatile LONG JNETAppearance[40] = {-1,0,0,-1,0,0,0,0,0,-1,0,0,-1,-1,-1,[22]=-1,[23]=-1,[24]=-1,[34]=-1,[37]=-1,[38]=-1};
__declspec(dllexport) volatile LONG JNETIdle[5] = {-1,0,-1,-1,-1};
__declspec(dllexport) volatile LONG JNETAnimation[17] = {-1,-1};
__declspec(dllexport) volatile LONG JNETPoseWrites[8] = {-1,-1,0,0,0,0,0,0};
__declspec(dllexport) volatile LONG JNETPose[3] = {-1,-1,-1};
__declspec(dllexport) volatile LONG JNETCommit[3] = {-1,0,0};
__declspec(dllexport) volatile LONG JNETBuilder[8] = {0};
__declspec(dllexport) volatile LONG JNETGeometry[9] = {-1,0,0,0,0,-1,-1,-1,0};
__declspec(dllexport) void *volatile JNETGame = NULL;
__declspec(dllexport) const unsigned char JNETBuildId[32] = JC_BUILD_ID;

__declspec(dllexport) volatile LONG JNETFlight[JC_FLIGHT_FIELDS]={0};
#include "jnet_session_log.h"
static HANDLE session_mapping;
static struct jnet_session_logs session_fallback,*session_logs=&session_fallback;
static uintptr_t fault_image,fault_self;
static uint32_t fault_self_size;
static volatile LONG fault_recorded;
static void flight_mark(LONG phase,LONG body) {
    InterlockedExchange(&JNETFlight[1],body);
    InterlockedExchange(&JNETFlight[2],JNETFrames);
    InterlockedExchange(&JNETFlight[3],(LONG)GetCurrentThreadId());
    InterlockedIncrement(&JNETFlight[4]);
    InterlockedExchange(&JNETFlight[0],phase);
}
static LONG CALLBACK fault_observer(EXCEPTION_POINTERS *exception) {
    if (!exception || !exception->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    EXCEPTION_RECORD *record=exception->ExceptionRecord;
    if (!jc_fault_interesting(record->ExceptionCode) || InterlockedIncrement(&fault_recorded)>8) return EXCEPTION_CONTINUE_SEARCH;
    uint32_t fields[JC_FAULT_FIELDS]={0};
    uintptr_t address=(uintptr_t)record->ExceptionAddress;
    fields[0]=record->ExceptionCode; fields[1]=GetCurrentThreadId();
    fields[3]=jc_fault_rva(address,fault_image,JC_IMAGE_SIZE);
    if (fields[3]!=UINT32_MAX) fields[2]=1;
    else {
        fields[3]=jc_fault_rva(address,fault_self,fault_self_size);
        if (fields[3]!=UINT32_MAX) fields[2]=2;
    }
    fields[4]=record->NumberParameters && (fields[0]==0xc0000005u || fields[0]==0xc0000006u) ?
        (uint32_t)record->ExceptionInformation[0] : UINT32_MAX;
    fields[5]=(uint32_t)JNETFrames;
    for (unsigned i=0;i<JC_FLIGHT_FIELDS;++i) fields[6+i]=(uint32_t)JNETFlight[i];
    if (exception->ContextRecord) {
        const CONTEXT *cpu=exception->ContextRecord;
        uint64_t values[26]={record->NumberParameters>=2 ? record->ExceptionInformation[1] : 0,
            cpu->Rip,cpu->Rsp,cpu->Rax,cpu->Rbx,cpu->Rcx,cpu->Rdx,cpu->Rsi,cpu->Rdi,
            cpu->Rbp,cpu->R8,cpu->R9,cpu->R10,cpu->R11,cpu->R12,cpu->R13,cpu->R14,cpu->R15};
        SIZE_T got=0;
        if (!ReadProcessMemory(GetCurrentProcess(),(void *)(uintptr_t)cpu->Rsp,values+18,64,&got) || got!=64)
            memset(values+18,0,64);
        for (unsigned i=0;i<26;++i) {
            fields[18+2*i]=(uint32_t)values[i];fields[19+2*i]=(uint32_t)(values[i]>>32);
        }
        unsigned char instruction[16]={0};got=0;
        if (ReadProcessMemory(GetCurrentProcess(),(void *)(uintptr_t)cpu->Rip,instruction,16,&got) && got==16)
            memcpy(fields+70,instruction,16);
        MEMORY_BASIC_INFORMATION mapping={0};
        if (record->NumberParameters>=2 && VirtualQuery((void *)(uintptr_t)values[0],&mapping,sizeof(mapping))==sizeof(mapping)) {
            fields[74]=mapping.State;fields[75]=mapping.Protect;fields[76]=mapping.Type;
            fields[77]=(uint32_t)(uintptr_t)mapping.BaseAddress;
            fields[78]=(uint32_t)((uint64_t)(uintptr_t)mapping.BaseAddress>>32);
        }
        fields[79]=(uint32_t)fault_recorded;fields[80]=cpu->EFlags;fields[81]=(uint32_t)got;
        fields[82]=1;
        for(unsigned block=0;block<JC_FAULT_STACK_BYTES/64;++block){
            uintptr_t at=(uintptr_t)cpu->Rsp+block*64;
            uint32_t *out=fields+JC_FAULT_STACK_FIELD+block*16;
            SIZE_T bytes=0;
            if(at>=(uintptr_t)cpu->Rsp && ReadProcessMemory(GetCurrentProcess(),(void *)at,out,64,&bytes) && bytes==64)
                fields[83]|=1u<<block;
            else memset(out,0,64);
        }
    }
    char line[JC_FAULT_BYTES];
    size_t length=jc_fault_record(line,sizeof(line),fields);
    int ok=jnet_log_append(&session_logs->fault,line,length);
    InterlockedExchange(&JNETFlight[5],(LONG)fields[0]);
    InterlockedExchange(&JNETFlight[6],(LONG)fields[2]);
    InterlockedExchange(&JNETFlight[7],(LONG)fields[3]);
    InterlockedExchange(&JNETFlight[10],ok ? 1 : -1);
    return EXCEPTION_CONTINUE_SEARCH;
}

typedef struct lua_State lua_State;
typedef int (*lua_callback)(lua_State *);
typedef int (*lua_continuation)(lua_State *, int, intptr_t);
static int (*gettop)(lua_State *);
static void (*settop)(lua_State *, int);
static int (*loadbuffer)(lua_State *, const char *, size_t, const char *, const char *);
static int (*pcallfn)(lua_State *, int, int, int, intptr_t, lua_continuation);
static const char *(*tolstring)(lua_State *, int, size_t *);
static void (*pushclosure)(lua_State *, lua_callback, int);
static int64_t (*original_update)(uintptr_t, float);
static HMODULE self;

static SRWLOCK loglock = SRWLOCK_INIT;
static unsigned jnet_diagnostic_scans=0,jnet_diagnostic_logs=0;
static volatile LONG jnet_diagnostic_suppressed;
static LONG jnet_diagnostic_scan_last,jnet_diagnostic_scan_peak,jnet_diagnostic_scan_count;
static int jnet_observation_line(const char *line,size_t length) {
    static const char *prefixes[]={"[JC] visibility ","[JC] appearance actor=","[JC] glyph ","[JC] floating glyph "};
    for(unsigned i=0;i<sizeof(prefixes)/sizeof(prefixes[0]);++i)
        if(length>=strlen(prefixes[i]) && !memcmp(line,prefixes[i],strlen(prefixes[i])))return 1;
    return 0;
}
static lua_State *bound_state;
static volatile LONG in_callback;
static volatile LONG initialization_started;

static void log_line(const char *line, size_t length) {
    if(!jnet_diagnostic_logs && jnet_observation_line(line,length)){InterlockedIncrement(&jnet_diagnostic_suppressed);return;}
    if (length > 480 || length < 4 ||
        (memcmp(line, "[JC]", 4) && (length < 6 || memcmp(line, "[JNET]", 6)))) return;
    for (size_t i = 0; i < length; ++i) {
        if ((unsigned char)line[i] < 32 || (unsigned char)line[i] > 126) return;
    }
    AcquireSRWLockExclusive(&loglock);
    char record[482];memcpy(record,line,length);memcpy(record+length,"\r\n",2);
    jnet_log_append(&session_logs->runtime,record,length+2);
    ReleaseSRWLockExclusive(&loglock);
}

static void message(const char *line) { log_line(line, strlen(line)); }
#include "jnet_steam_probe.h"

static int lua_log(lua_State *state) {
    size_t length = 0;
    const char *line = tolstring(state, 1, &length);
    if (line) {
        static const char prefix[] = "[JC] observation_state=";
        if (length == sizeof(prefix) && !memcmp(line, prefix, sizeof(prefix) - 1) &&
            line[length - 1] >= '0' && line[length - 1] <= '8')
            InterlockedExchange(&JNETObservation, line[length - 1] - '0');
        static const char stage_prefix[] = "[JC] sample_stage=";
        if (length == sizeof(stage_prefix) + 1 && !memcmp(line, stage_prefix, sizeof(stage_prefix) - 1) &&
            line[length - 2] == '1' && line[length - 1] >= '0' && line[length - 1] <= '8')
            InterlockedExchange(&JNETPhase, 10 + line[length - 1] - '0');
        log_line(line, length);
    }
    return 0;
}

static int execute(lua_State *state, const char *code, size_t size, int with_sink) {
    int top = gettop(state);
    int ok = loadbuffer(state, code, size, "JC-diagnostic", "t") == 0;
    if (ok) {
        if (with_sink) pushclosure(state, lua_log, 0);
        ok = pcallfn(state, with_sink ? 1 : 0, 0, 0, 0, NULL) == 0;
    }
    settop(state, top);
    if (!ok) {
        InterlockedExchange(&JNETStatus, -5);
        message("[JC] Lua diagnostic failed; observation disabled; error text omitted");
    }
    return ok;
}

#include "jnet_lobby_bridge.h"
static int startup_hash_bytes(const unsigned char *buffer,size_t bytes, unsigned char result[32]) {
    BCRYPT_ALG_HANDLE algorithm=NULL;
    BCRYPT_HASH_HANDLE hash=NULL;
    int ok=0;
    if (BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0) goto done;
    if (BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0) goto done;
    if (BCryptHashData(hash,(PUCHAR)buffer,(ULONG)bytes,0)<0) goto done;
    ok=BCryptFinishHash(hash,result,32,0)>=0;
done:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    return ok;
}
#include "jnet_sound_banks.h"
#include "native_chirp_audio.h"
static int (*original_loadbuffer)(lua_State *, const char *, size_t, const char *, const char *);
static int startup_load_observer(lua_State *state, const char *buffer, size_t size,
                                 const char *name, const char *mode) {
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    int matched=JNETStartup[0] && (uintptr_t)buffer==image+JC_STARTUP_SOURCE_RVA && size==JC_STARTUP_SIZE;
    LONG attempt=matched ? InterlockedIncrement(&JNETStartup[1]) : 0;
    unsigned char *copy=NULL, hash[32];
    if (attempt==1) {
        SIZE_T got=0;
        copy=HeapAlloc(GetProcessHeap(),0,JC_STARTUP_CANDIDATE_SIZE);
        int valid=copy && ReadProcessMemory(GetCurrentProcess(),buffer,copy,size,&got) && got==size &&
            startup_hash_bytes(copy,JC_STARTUP_SIZE,hash) && !memcmp(hash,jc_startup_source_hash,32);
        for (unsigned i=0;valid && i<sizeof(jc_startup_offsets)/sizeof(jc_startup_offsets[0]);++i) {
            unsigned offset=jc_startup_offsets[i];
            if (offset>=size || copy[offset]!=jc_startup_before[i]) valid=0;
            else copy[offset]=jc_startup_after[i];
        }
        size_t prefix=sizeof(jc_wayfarer_startup)-1;
        if(valid && prefix+sizeof(jc_character_startup)-1==JC_STARTUP_CANDIDATE_SIZE-JC_STARTUP_SIZE) {
            memmove(copy+prefix,copy,JC_STARTUP_SIZE);
            memcpy(copy,jc_wayfarer_startup,prefix);
            memcpy(copy+prefix+JC_STARTUP_SIZE,jc_character_startup,sizeof(jc_character_startup)-1);
        } else valid=0;
        valid=valid && startup_hash_bytes(copy,JC_STARTUP_CANDIDATE_SIZE,hash) && !memcmp(hash,jc_startup_candidate_hash,32);
        if(valid && !jnet_sound_banks_prepare(image)){
            valid=0;message("[JNET] sound-bank expansion declined; private resource preload withheld");
        }
        if(valid && !jc_chirp_audio_prepare(image)){
            valid=0;message("[JNET] remote chirp audio preparation declined; startup resource candidate withheld");
        }
        if (!valid) {
            if (copy) HeapFree(GetProcessHeap(),0,copy);
            copy=NULL;
            InterlockedExchange(&JNETStartup[4],-1);
            message("[JC] startup resource candidate declined; original buffer forwarded");
        } else {
            InterlockedExchange(&JNETStartup[4],1);
            message("[JC] startup resource candidate validated; private characters with animation stage checkpoints");
        }
    } else if (attempt>1) {
        InterlockedExchange(&JNETStartup[4],-2);
    }
    int result=original_loadbuffer(state,copy ? (const char *)copy : buffer,
                                  copy ? JC_STARTUP_CANDIDATE_SIZE : size,name,mode);
    if (copy) HeapFree(GetProcessHeap(),0,copy);
    if (attempt==1) {
        InterlockedExchange(&JNETStartup[3],result);
        InterlockedExchange(&JNETStartup[2],1);
        message("[JC] startup compiler returned; original executable and assets unchanged");
    }
    return result;
}

static int allocation_attempted;
static struct jc_lifetime lifetime;
static SRWLOCK lifetime_lock=SRWLOCK_INIT;
static volatile LONG lifetime_conflict;
static uintptr_t resource_game, resource_barn;
static DWORD resource_thread;
static uintptr_t (*original_resource_init)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static uintptr_t (*original_cleanup)(uintptr_t, uintptr_t);
__declspec(dllexport) volatile LONG JNETCompanionFeatures[40]={0};
__declspec(dllexport) DWORD WINAPI JNETControl(void *parameter) {
    uint64_t packet=(uintptr_t)parameter;
    unsigned operation=(unsigned)packet;
    LONG value=(LONG)(packet>>32);
    if (operation==1) {
        InterlockedExchange(&JNETCompanionFeatures[9],value);
        return 0;
    }
    if (operation==2 && value>=1 && value<=27)
        return InterlockedCompareExchange(&JNETCommand,value,0)==0 ? 0 : 2;
    if (operation==3 && value==1 && JNETStatus==0) {
        InterlockedExchange(&JNETStartup[0],1);
        return 0;
    }
    return 1;
}

static __declspec(thread) struct jc_local_read_cache own_read_cache;
static int own_read(void *context, uint64_t address, void *out, size_t size) {
    (void)context;
    if (JNETCompanionFeatures[8]==1 && JNETCompanionFeatures[9]==0)
        return jc_local_read(&own_read_cache,(uint32_t)JNETFrames+UINT64_C(1),
            jc_local_windows_query,jc_local_windows_copy,NULL,address,out,size);
    SIZE_T got=0;
    return ReadProcessMemory(GetCurrentProcess(),(const void *)(uintptr_t)address,out,size,&got) && got==size;
}
#include "pose_link.h"
#define JNET_LOBBY_ENABLED 1
static int jnet_lobby_allows(uint64_t peer);
#include "jnet_menu_live.h"
#include "jnet_controls_live.h"
static uint64_t trial_create(void *context) {
    InterlockedExchange(&JNETPhase,20);
    uintptr_t (*factory)(void)=(void *)((unsigned char *)context+JC_RVA_DudeFactory);
    return factory();
}
static void trial_prepare(uintptr_t image,uint64_t dude,uint64_t manager) {
    ((void (*)(uintptr_t,uintptr_t))(image+JC_RVA_DudePrepare))((uintptr_t)dude,(uintptr_t)manager);
}
static void trial_destroy(void *context, uint64_t address) {
    InterlockedExchange(&JNETTrial,3);
    InterlockedCompareExchange(&JNETLifetime,3,1);
    InterlockedExchange(&JNETPhase,21);
    void (*destroy)(void *,unsigned int)=(void *)((unsigned char *)context+JC_RVA_DudeDelete);
    destroy((void *)(uintptr_t)address,1);
}
__declspec(dllexport) volatile LONG JNETPerformance[30]={[1]=-1,[6]=-1,[8]=-1,[9]=-1,[18]=-1,[19]=-1,[26]=-1,[29]=-1};

#define JC_BODY_COUNT 2u
#define JC_BODY_CAPACITY 4u
__declspec(dllexport) volatile LONG JNETBodies[48]={0};
__declspec(dllexport) volatile LONG JNETDual[8]={0};
__declspec(dllexport) volatile LONG JNETSkeleton[32]={0};
__declspec(dllexport) volatile LONG JNETEvaluation[40]={[1]=-1,[3]=-1,[6]=4,[15]=-1,[23]=-1,[31]=-1,[39]=-1};
__declspec(dllexport) volatile LONG JNETNativePlayback[48]={0};
__declspec(dllexport) volatile LONG JNETNativeOutputs[16]={0};
static int native_playback_mode;
static ULONGLONG native_playback_begin;
static int native_playback_room;
static struct jc_cloth_lease native_cloth_lease;
static int native_cloth_disabled;
static void companion_cloth_cleanup_locked(uintptr_t dude);
static struct jc_chirp_capture native_chirp_capture;
static struct jc_chirp_delivery native_chirp_delivery[JC_BODY_COUNT];
static struct jc_eval_layout_workspace *native_playback_work;
static struct jc_anim_layout native_playback_source;
static int mirror_mode;
static ULONGLONG mirror_not_before;
static struct jc_skel_mirror *mirror_work;
struct jc_body_context {
    unsigned index;
    struct jc_callback_snapshot prepared_callbacks;
    struct jc_initialized_lifetime s_initialized_life;
    LONG s_initialized_frame;
    unsigned s_boundary_checkpoints;
    uintptr_t s_mesh_context, s_mesh_source, s_material_manager, s_material_name;
    uint64_t s_geometry_source_before, s_geometry_slot_before;
    uint64_t s_callback_manager, s_callback_meshes[3];
    struct jc_callback_snapshot s_callback_baseline;
    float s_preview_origin[3],s_preview_shader_world[16],s_preview_applied_transform[16];
    uint64_t s_preview_pose_sequence;
    unsigned char s_preview_pose_identity[16];
    int s_preview_linked;
    struct jc_anim_layout motion_layout;
    struct jc_anim_sample applied_animation;
    int motion_active;
    ULONGLONG mirror_next,native_next;
    uint64_t native_sequence;
    unsigned char native_identity[16];
    float native_previous[JC_SKEL_CAP][16];
    int native_seen;
    float motion_spine[16]; int motion_spine_known;
    uintptr_t s_execution_owner,s_execution_begin,s_execution_end;
    unsigned char s_execution_first[128];
    ULONGLONG s_execution_deadline;
    DWORD s_execution_submit_thread;
    volatile LONG s_preview_observer_thread;
    volatile LONG s_local_consumer_thread,s_local_builder_thread;
    uint64_t s_local_draw_item;
    unsigned char *s_preview_queue;
    struct jc_render_tracking s_preview_tracking;
    struct jc_render_workspace s_preview_work;
    uintptr_t s_preview_source_queue;
    int s_preview_started;
    struct jc_render_budget_retry s_preview_retry;
    ULONGLONG s_preview_deadline,s_preview_not_before;
    struct jc_appearance_borrow s_appearance_borrow;
    volatile LONG s_mesh_fault;
    int mesh_guard_failure;
    volatile LONG *out_Mesh; LONG data_Mesh[6];
    volatile LONG *out_Material; LONG data_Material[1];
    volatile LONG *out_Callbacks; LONG data_Callbacks[11];
    volatile LONG *out_MeshLife; LONG data_MeshLife[152];
    volatile LONG *out_Appearance; LONG data_Appearance[40];
    volatile LONG *out_Idle; LONG data_Idle[5];
    volatile LONG *out_Animation; LONG data_Animation[17];
    volatile LONG *out_PoseWrites; LONG data_PoseWrites[8];
    volatile LONG *out_Pose; LONG data_Pose[3];
    volatile LONG *out_Commit; LONG data_Commit[3];
    volatile LONG *out_Geometry; LONG data_Geometry[9];
    volatile LONG *out_Performance; LONG data_Performance[30];
};
static struct jc_body_context body_contexts[JC_BODY_COUNT];
static struct jc_body_context *body_context=&body_contexts[0];
static void body_context_init(void) {
    body_contexts[0].index=0;
    memcpy(body_contexts[0].data_Mesh,(const void *)JNETMesh,6*sizeof(LONG));
    body_contexts[0].out_Mesh=JNETMesh;
    memcpy(body_contexts[0].data_Material,(const void *)&JNETMaterial,1*sizeof(LONG));
    body_contexts[0].out_Material=&JNETMaterial;
    memcpy(body_contexts[0].data_Callbacks,(const void *)JNETCallbacks,11*sizeof(LONG));
    body_contexts[0].out_Callbacks=JNETCallbacks;
    memcpy(body_contexts[0].data_MeshLife,(const void *)JNETMeshLife,152*sizeof(LONG));
    body_contexts[0].out_MeshLife=JNETMeshLife;
    memcpy(body_contexts[0].data_Appearance,(const void *)JNETAppearance,40*sizeof(LONG));
    body_contexts[0].out_Appearance=JNETAppearance;
    memcpy(body_contexts[0].data_Idle,(const void *)JNETIdle,5*sizeof(LONG));
    body_contexts[0].out_Idle=JNETIdle;
    memcpy(body_contexts[0].data_Animation,(const void *)JNETAnimation,17*sizeof(LONG));
    body_contexts[0].out_Animation=JNETAnimation;
    memcpy(body_contexts[0].data_PoseWrites,(const void *)JNETPoseWrites,8*sizeof(LONG));
    body_contexts[0].out_PoseWrites=JNETPoseWrites;
    memcpy(body_contexts[0].data_Pose,(const void *)JNETPose,3*sizeof(LONG));
    body_contexts[0].out_Pose=JNETPose;
    memcpy(body_contexts[0].data_Commit,(const void *)JNETCommit,3*sizeof(LONG));
    body_contexts[0].out_Commit=JNETCommit;
    memcpy(body_contexts[0].data_Geometry,(const void *)JNETGeometry,9*sizeof(LONG));
    body_contexts[0].out_Geometry=JNETGeometry;
    memcpy(body_contexts[0].data_Performance,(const void *)JNETPerformance,30*sizeof(LONG));
    body_contexts[0].out_Performance=JNETPerformance;
    body_contexts[1].index=1;
    memcpy(body_contexts[1].data_Mesh,(const void *)JNETMesh,6*sizeof(LONG));
    body_contexts[1].out_Mesh=body_contexts[1].data_Mesh;
    memcpy(body_contexts[1].data_Material,(const void *)&JNETMaterial,1*sizeof(LONG));
    body_contexts[1].out_Material=body_contexts[1].data_Material;
    memcpy(body_contexts[1].data_Callbacks,(const void *)JNETCallbacks,11*sizeof(LONG));
    body_contexts[1].out_Callbacks=body_contexts[1].data_Callbacks;
    memcpy(body_contexts[1].data_MeshLife,(const void *)JNETMeshLife,152*sizeof(LONG));
    body_contexts[1].out_MeshLife=body_contexts[1].data_MeshLife;
    memcpy(body_contexts[1].data_Appearance,(const void *)JNETAppearance,40*sizeof(LONG));
    body_contexts[1].out_Appearance=body_contexts[1].data_Appearance;
    memcpy(body_contexts[1].data_Idle,(const void *)JNETIdle,5*sizeof(LONG));
    body_contexts[1].out_Idle=body_contexts[1].data_Idle;
    memcpy(body_contexts[1].data_Animation,(const void *)JNETAnimation,17*sizeof(LONG));
    body_contexts[1].out_Animation=body_contexts[1].data_Animation;
    memcpy(body_contexts[1].data_PoseWrites,(const void *)JNETPoseWrites,8*sizeof(LONG));
    body_contexts[1].out_PoseWrites=body_contexts[1].data_PoseWrites;
    memcpy(body_contexts[1].data_Pose,(const void *)JNETPose,3*sizeof(LONG));
    body_contexts[1].out_Pose=body_contexts[1].data_Pose;
    memcpy(body_contexts[1].data_Commit,(const void *)JNETCommit,3*sizeof(LONG));
    body_contexts[1].out_Commit=body_contexts[1].data_Commit;
    memcpy(body_contexts[1].data_Geometry,(const void *)JNETGeometry,9*sizeof(LONG));
    body_contexts[1].out_Geometry=body_contexts[1].data_Geometry;
    memcpy(body_contexts[1].data_Performance,(const void *)JNETPerformance,30*sizeof(LONG));
    body_contexts[1].out_Performance=body_contexts[1].data_Performance;
}
#define native_report (JNETNativePlayback+body_context->index*24)
#define initialized_life (body_context->s_initialized_life)
#define initialized_frame (body_context->s_initialized_frame)
#define boundary_checkpoints (body_context->s_boundary_checkpoints)
#define mesh_context (body_context->s_mesh_context)
#define mesh_source (body_context->s_mesh_source)
#define material_manager (body_context->s_material_manager)
#define material_name (body_context->s_material_name)
#define geometry_source_before (body_context->s_geometry_source_before)
#define geometry_slot_before (body_context->s_geometry_slot_before)
#define callback_manager (body_context->s_callback_manager)
#define callback_meshes (body_context->s_callback_meshes)
#define callback_baseline (body_context->s_callback_baseline)
#define preview_origin (body_context->s_preview_origin)
#define preview_shader_world (body_context->s_preview_shader_world)
#define preview_applied_transform (body_context->s_preview_applied_transform)
#define preview_pose_sequence (body_context->s_preview_pose_sequence)
#define preview_pose_identity (body_context->s_preview_pose_identity)
#define preview_linked (body_context->s_preview_linked)
#define execution_owner (body_context->s_execution_owner)
#define execution_begin (body_context->s_execution_begin)
#define execution_end (body_context->s_execution_end)
#define execution_first (body_context->s_execution_first)
#define execution_deadline (body_context->s_execution_deadline)
#define execution_submit_thread (body_context->s_execution_submit_thread)
#define preview_observer_thread (body_context->s_preview_observer_thread)
#define local_consumer_thread (body_context->s_local_consumer_thread)
#define local_builder_thread (body_context->s_local_builder_thread)
#define local_draw_item (body_context->s_local_draw_item)
#define preview_queue (body_context->s_preview_queue)
#define preview_tracking (body_context->s_preview_tracking)
#define preview_work (body_context->s_preview_work)
#define preview_source_queue (body_context->s_preview_source_queue)
#define preview_started (body_context->s_preview_started)
#define preview_retry (body_context->s_preview_retry)
#define preview_deadline (body_context->s_preview_deadline)
#define preview_not_before (body_context->s_preview_not_before)
#define appearance_borrow (body_context->s_appearance_borrow)
#define mesh_fault (body_context->s_mesh_fault)
#define JNETMesh (body_context->out_Mesh)
#define JNETMaterial (*body_context->out_Material)
#define JNETCallbacks (body_context->out_Callbacks)
#define JNETMeshLife (body_context->out_MeshLife)
#define JNETAppearance (body_context->out_Appearance)
#define JNETIdle (body_context->out_Idle)
#define JNETAnimation (body_context->out_Animation)
#define JNETPoseWrites (body_context->out_PoseWrites)
#define JNETPose (body_context->out_Pose)
#define JNETCommit (body_context->out_Commit)
#define JNETGeometry (body_context->out_Geometry)
#define JNETPerformance (body_context->out_Performance)
static void body_publish(void) {
    unsigned row=24*body_context->index;
    const LONG values[24]={JNETMeshLife[0],JNETMeshLife[25],JNETMeshLife[26],JNETMeshLife[27],
        JNETPerformance[18],JNETPerformance[19],JNETMeshLife[6],JNETMeshLife[7],JNETMeshLife[8],
        JNETAppearance[0],JNETAppearance[34],JNETMeshLife[116],body_context->mesh_guard_failure,
        JNETPerformance[4],JNETPerformance[5],JNETPerformance[2],JNETPerformance[3],
        JNETPerformance[13],JNETPerformance[14],JNETPerformance[15],JNETGeometry[6],JNETMeshLife[2],
        JNETIdle[0],JNETMeshLife[28]};
    for (unsigned i=0;i<24;++i) InterlockedExchange(&JNETBodies[row+i],values[i]);
}
static int checkpoint_write(void *context,const char *line,size_t length) {
    return jnet_log_append(context,line,length);
}
static int checkpoint_flush(void *context) { return context!=NULL; }
static int checkpoint_failure(int code,DWORD error) {
    if (InterlockedCompareExchange(&JNETMeshLife[116],code,0)==0)
        InterlockedExchange(&JNETMeshLife[117],(LONG)error);
    return 0;
}
static void boundary_checkpoint_begin(void) {
    AcquireSRWLockExclusive(&loglock);
    boundary_checkpoints=0;
    InterlockedExchange(&JNETMeshLife[115],0);
    ReleaseSRWLockExclusive(&loglock);
}
static int boundary_checkpoint(const char *line) {
    if (!TryAcquireSRWLockExclusive(&loglock)) return checkpoint_failure(JC_CHECKPOINT_BUSY,0);
    size_t length=strlen(line);
    int code=jc_checkpoint_record(&boundary_checkpoints,line,length,&session_logs->runtime,checkpoint_write,checkpoint_flush);
    DWORD error=code>=JC_CHECKPOINT_LINE ? GetLastError() : 0;
    InterlockedExchange(&JNETMeshLife[115],(LONG)boundary_checkpoints);
    ReleaseSRWLockExclusive(&loglock);
    return code ? checkpoint_failure(code,error) : 1;
}
static void boundary_record(const char *line) {
    if (!boundary_checkpoint(line)) InterlockedExchange(&JNETResource,-2);
}
static void *volatile builder_target;
static uintptr_t (*original_geometry_build)(uintptr_t);
static uintptr_t geometry_build_observer(uintptr_t object) {
    uintptr_t target=(uintptr_t)InterlockedCompareExchangePointer(&builder_target,NULL,NULL);
    if (!target || object!=target) return original_geometry_build(object);
    LONG attempt=InterlockedIncrement(&JNETBuilder[0]);
    if (attempt>4) { InterlockedExchange(&JNETBuilder[6],5); return original_geometry_build(object); }
    struct jc_geometry_build_state before,after;
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    int code=jc_geometry_build_read(own_read,NULL,image,object,&before);
    char line[160];
    if (!code) snprintf(line,sizeof(line),"[JC] geometry build enter limit=%u slots=%u occupied=%u; observation only",
        (unsigned)before.limit,(unsigned)before.slots,(unsigned)before.occupied);
    else snprintf(line,sizeof(line),"[JC] geometry build enter snapshot declined code=%d; original forwarded",code);
    boundary_record(line);
    uintptr_t result=original_geometry_build(object);
    int after_code=jc_geometry_build_read(own_read,NULL,image,object,&after);
    InterlockedExchange(&JNETBuilder[6],code ? code : after_code);
    if (!code) {
        InterlockedExchange(&JNETBuilder[1],before.limit);
        InterlockedExchange(&JNETBuilder[2],before.slots);
        InterlockedExchange(&JNETBuilder[3],before.occupied);
    }
    if (!after_code) {
        InterlockedExchange(&JNETBuilder[4],after.slots);
        InterlockedExchange(&JNETBuilder[5],after.occupied);
    }
    boundary_record("[JC] geometry build original returned; observation only");
    return result;
}
static void *volatile guarded_mesh;
struct callback_read_budget { ULONGLONG deadline; int expired,read_failed; unsigned reads,fast; struct jc_local_read_cache local; struct jc_read_batch *batch; };
static int callback_raw(void *opaque,uint64_t address,void *out,size_t size) {
    struct callback_read_budget *budget=opaque;
    if (GetTickCount64()>=budget->deadline) { budget->expired=1; return 0; }
    ++budget->reads;
    int ok=(budget->fast || JNETCompanionFeatures[8]==1) ? jc_local_read(&budget->local,1,jc_local_windows_query,jc_local_windows_copy,NULL,address,out,size) :
        own_read(NULL,address,out,size);

    return ok;
}
static int callback_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct callback_read_budget *budget=opaque;
    if (GetTickCount64()>=budget->deadline) { budget->expired=1; return 0; }
    int ok=budget->batch && !budget->fast && JNETCompanionFeatures[8]!=1 ? jc_read_batch_get(budget->batch,callback_raw,budget,address,out,size) :
        callback_raw(budget,address,out,size);
    if (!ok && !budget->expired) budget->read_failed=1;
    return ok;
}
struct jc_inspection { struct callback_read_budget *budget; struct jc_read_batch batch; };
static int inspection_raw(void *opaque,uint64_t address,void *out,size_t size) {
    return callback_raw(opaque,address,out,size);
}
static int inspection_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct jc_inspection *inspection=opaque;
    if (inspection->budget->fast || JNETCompanionFeatures[8]==1)
        return callback_read(inspection->budget,address,out,size);
    if (GetTickCount64()>=inspection->budget->deadline) { inspection->budget->expired=1; return 0; }
    int ok=jc_read_batch_get(&inspection->batch,inspection_raw,inspection->budget,address,out,size);
    if (!ok && !inspection->budget->expired) inspection->budget->read_failed=1;
    return ok;
}
static void (*original_submit)(uintptr_t,uintptr_t);
static int64_t (*original_consume)(uintptr_t,uintptr_t,uint32_t);
static uint32_t (*original_geometry_dispatch)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t,uint64_t);
static uint32_t (*original_command_build)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uint32_t);
static uint32_t (*original_render_flush)(uintptr_t);
static uint32_t (*original_render_handoff)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static uint32_t (*original_base_flush)(uintptr_t);
static uint32_t (*original_base_handoff)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static uint32_t (*original_command_execute)(uintptr_t,uintptr_t,uintptr_t);
#include "native_cloth_render_runtime.h"
static ULONGLONG preparation_deadline;
static int prepared_level=-1;
static void capture_execution_range(uintptr_t owner,uintptr_t begin,uintptr_t end) {
    if (InterlockedCompareExchange(&JNETMeshLife[89],0,0) || begin==end) return;
    uint32_t records=0;
    if (!jc_render_command_range(begin,end,&records)) {
        InterlockedExchange(&JNETMeshLife[89],-2); return;
    }
    if (!own_read(NULL,begin,execution_first,sizeof(execution_first))) {
        InterlockedExchange(&JNETMeshLife[89],-1); return;
    }
    execution_owner=owner; execution_begin=begin; execution_end=end;
    execution_deadline=GetTickCount64()+60000; execution_submit_thread=GetCurrentThreadId();
    InterlockedExchange(&JNETMeshLife[90],(LONG)records);
    InterlockedExchange(&JNETMeshLife[89],1);
}
static void draw_input_snapshot(unsigned offset,const unsigned char command[128]) {
    struct jc_render_draw_input value;
    struct callback_read_budget budget={.deadline=GetTickCount64()+25};
    int code=jc_render_draw_input(callback_read,&budget,(uintptr_t)GetModuleHandleW(NULL),command,&value);
    if (budget.expired) code=9;
    if (!code) {
        InterlockedExchange(&JNETMeshLife[offset+1],(LONG)value.kind);
        InterlockedExchange(&JNETMeshLife[offset+2],(LONG)value.stored);
        InterlockedExchange(&JNETMeshLife[offset+3],(LONG)value.indexed);
        InterlockedExchange(&JNETMeshLife[offset+4],(LONG)value.index_count);
        InterlockedExchange(&JNETMeshLife[offset+5],(LONG)value.instances);
        InterlockedExchange(&JNETMeshLife[offset+6],(LONG)value.gate);
        InterlockedExchange(&JNETMeshLife[offset+7],(LONG)value.context);
        InterlockedExchange(&JNETMeshLife[offset+8],(LONG)value.topology);
    }
    InterlockedExchange(&JNETMeshLife[offset],code);
}
static int preview_count_entry(unsigned index) {
    LONG thread=InterlockedCompareExchange(&preview_observer_thread,0,0);
    if (!thread || (DWORD)thread!=GetCurrentThreadId()) return 0;
    if (JNETMeshLife[index]>=65536) {
        InterlockedExchange(&JNETMeshLife[48],1); return 0;
    }
    InterlockedIncrement(&JNETMeshLife[index]);
    return 1;
}
static uint32_t (*original_async_geometry_dispatch)(uintptr_t,uintptr_t,uintptr_t);
static uint32_t async_geometry_dispatch_observer(uintptr_t renderer,uintptr_t item,uintptr_t constants) {
    uint64_t bindings=0;
    own_read(NULL,item+0x20,&bindings,8);
    struct jc_cloth_render_scope scope;
    int owned=jc_cloth_render_dispatch_begin(renderer,item,(uintptr_t)bindings,&scope);
    uint32_t result=owned<0 ? 5 : original_async_geometry_dispatch(renderer,item,constants);
    jc_cloth_render_dispatch_end(&scope,result);
    traveler_glyph_trace_dispatch(item,result);
    return result;
}
static uint32_t geometry_dispatch_observer(uintptr_t renderer,uintptr_t item,uintptr_t bindings,
                                          uintptr_t optional,uint32_t generation,uint64_t excluded) {
    int counted=preview_count_entry(44);
    int local=JNETMeshLife[118]==-1 && !local_builder_thread &&
        (DWORD)InterlockedCompareExchange(&local_consumer_thread,0,0)==GetCurrentThreadId() && item==local_draw_item;
    if (counted && JNETMeshLife[44]==1) {
        uint64_t source=0; uint32_t batches=0,binding_count=0;
        int valid=own_read(NULL,item,&source,8) && source &&
            own_read(NULL,source,&batches,4) && own_read(NULL,bindings,&binding_count,4);
        if (valid) {
            InterlockedExchange(&JNETMeshLife[50],(LONG)(batches&0x7fffffff));
            InterlockedExchange(&JNETMeshLife[51],(LONG)(binding_count&0x7fffffff));
            InterlockedExchange(&JNETMeshLife[52],excluded!=0);
        }
        InterlockedExchange(&JNETMeshLife[49],valid ? 0 : 1);
    }
    if (local) InterlockedExchange(&local_builder_thread,(LONG)GetCurrentThreadId());
    struct jc_cloth_render_scope cloth_scope;
    int cloth_owned=jc_cloth_render_dispatch_begin(renderer,item,bindings,&cloth_scope);
    uint32_t result=cloth_owned<0 ? 5 :
        original_geometry_dispatch(renderer,item,bindings,optional,generation,excluded);
    jc_cloth_render_dispatch_end(&cloth_scope,result);
    traveler_glyph_trace_dispatch(item,result);
    if (local) InterlockedExchange(&local_builder_thread,0);
    if (counted) {
        InterlockedIncrement(&JNETMeshLife[45]);
        if (!result) InterlockedIncrement(&JNETMeshLife[46]);
        else InterlockedCompareExchange(&JNETMeshLife[47],(LONG)result,0);
    }
    return result;
}
static uint32_t command_build_observer(uintptr_t renderer,uintptr_t material,uintptr_t passes,
                                      uintptr_t optional,uintptr_t batch,uint32_t index,
                                      uintptr_t state,uintptr_t storage,uint32_t flags) {
    int glyph_trace=traveler_glyph_trace_build(material);
    uint64_t glyph_before=0,glyph_after=0;uint32_t glyph_passes=0;
    if(glyph_trace>=0) {own_read(NULL,renderer+0x98,&glyph_before,8);own_read(NULL,passes+8,&glyph_passes,4);}
    int counted=preview_count_entry(53);
    int trial=counted && JNETMeshLife[53]==1;
    int local=JNETMeshLife[118]==-1 &&
        (DWORD)InterlockedCompareExchange(&local_builder_thread,0,0)==GetCurrentThreadId();
    uint64_t local_before=0,local_pass=0; uint32_t local_passes=0;
    int local_valid=0;
    if (local) {
        InterlockedExchange(&JNETMeshLife[118],-2);
        local_valid=own_read(NULL,renderer+0x98,&local_before,8) &&
            own_read(NULL,passes+8,&local_passes,4) && own_read(NULL,passes+0x10,&local_pass,8);
    }
    int sample=trial, valid=0;
    uint32_t pass_count=0; uint64_t before=0,after=0;
    if (sample) {
        uint64_t table=0,target=0; uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
        if (own_read(NULL,renderer,&table,8) && own_read(NULL,table+0x10,&target,8))
            if (trial) InterlockedExchange(&JNETMeshLife[84],target==image+JC_RVA_RenderFlush ? 1 : target==image+JC_RVA_BaseFlush ? 2 : 0);
        valid=own_read(NULL,passes+8,&pass_count,4) && own_read(NULL,renderer+0x98,&before,8);
        if (valid && trial) InterlockedExchange(&JNETMeshLife[56],(LONG)(pass_count&0x7fffffff));
    }
    struct jc_cloth_command_source cloth_source={material,passes,optional,batch,state,storage,index,flags};
    const struct jc_cloth_command_source *cloth_redirected=&cloth_source;
    int cloth_owned=jc_cloth_render_build_before(renderer,&cloth_source,&cloth_redirected);
    uint32_t result=cloth_owned<0 ? 5 : original_command_build(renderer,cloth_redirected->material,
        cloth_redirected->passes,cloth_redirected->optional,cloth_redirected->batch,cloth_redirected->index,
        cloth_redirected->state,cloth_redirected->storage,cloth_redirected->instances);
    if (cloth_owned>0) jc_cloth_render_build_after(result);
    if(glyph_trace>=0) {
        own_read(NULL,renderer+0x98,&glyph_after,8);
        traveler_glyph_trace_built(glyph_trace,result,glyph_before,glyph_after,glyph_passes&0x7fffffff);
    }
    if (local) {
        uint64_t end=0; unsigned char command[128]; struct jc_render_command_input input;
        int code=JC_LAYOUT_READ;
        if (local_valid && own_read(NULL,renderer+0x98,&end,8)) {
            if ((local_passes&0x7fffffff)!=1 || end==local_before || end<128) code=JC_LAYOUT_RANGE;
            else if (own_read(NULL,end-128,command,sizeof(command))) {
                code=jc_render_command_input(command,material,local_pass,batch,&input);
                if (!code) draw_input_snapshot(118,command);
            }
        }
        if (code) InterlockedExchange(&JNETMeshLife[118],code);
    }
    if (counted) InterlockedIncrement(&JNETMeshLife[54]);
    if (trial) {
        valid=valid && own_read(NULL,renderer+0x98,&after,8);
        if (valid) InterlockedExchange(&JNETMeshLife[57],before!=after);
        InterlockedExchange(&JNETMeshLife[55],valid ? 0 : 1);
    }
    return result;
}
static uint32_t render_flush_observer(uintptr_t renderer) {
    int counted=preview_count_entry(65), sample=counted && JNETMeshLife[65]==1;
    uint64_t context=0,begin=0,end=0; int valid=0;
    if (sample) {
        valid=own_read(NULL,renderer+0x140,&context,8) &&
            own_read(NULL,renderer+0x90,&begin,8) && own_read(NULL,renderer+0x98,&end,8);
        if (valid) {
            InterlockedExchange(&JNETMeshLife[70],context!=0);
            InterlockedExchange(&JNETMeshLife[71],begin!=end);
        }
    }
    uint32_t result=original_render_flush(renderer);
    if (counted) {
        InterlockedIncrement(&JNETMeshLife[66]);
        if (!result) InterlockedIncrement(&JNETMeshLife[67]);
        else InterlockedCompareExchange(&JNETMeshLife[68],(LONG)result,0);
    }
    if (sample) {
        valid=valid && own_read(NULL,renderer+0x90,&begin,8) && own_read(NULL,renderer+0x98,&end,8);
        if (valid) InterlockedExchange(&JNETMeshLife[72],!begin && !end);
        InterlockedExchange(&JNETMeshLife[69],valid ? 0 : 1);
    }
    return result;
}
static uint32_t render_handoff_observer(uintptr_t executor,uintptr_t context,uintptr_t fence,
                                       uintptr_t begin,uintptr_t end) {
    int counted=preview_count_entry(73);
    if (counted) capture_execution_range(executor,begin,end);
    int handled=0;
    uint32_t result=jc_cloth_render_handoff(executor,context,fence,begin,end,original_render_handoff,&handled);
    if (!handled) result=jc_cloth_live_stock_handoff(active_cloth_backend,executor,context,fence,begin,end,&handled);
    if (!handled) result=original_render_handoff(executor,context,fence,begin,end);
    if (counted) {
        InterlockedIncrement(&JNETMeshLife[74]);
        if (!result) InterlockedIncrement(&JNETMeshLife[75]);
        else InterlockedCompareExchange(&JNETMeshLife[76],(LONG)result,0);
    }
    return result;
}
static uint32_t base_flush_observer(uintptr_t renderer) {
    int counted=preview_count_entry(77),sample=counted && JNETMeshLife[77]==1;
    uint64_t begin=0,end=0; int valid=0;
    if (sample) {
        valid=own_read(NULL,renderer+0x90,&begin,8) && own_read(NULL,renderer+0x98,&end,8);
        if (valid) InterlockedExchange(&JNETMeshLife[82],begin!=end);
    }
    uint32_t result=original_base_flush(renderer);
    if (counted) {
        InterlockedIncrement(&JNETMeshLife[78]);
        if (!result) InterlockedIncrement(&JNETMeshLife[79]);
        else InterlockedCompareExchange(&JNETMeshLife[80],(LONG)result,0);
    }
    if (sample) {
        valid=valid && own_read(NULL,renderer+0x90,&begin,8) && own_read(NULL,renderer+0x98,&end,8);
        if (valid) InterlockedExchange(&JNETMeshLife[83],!begin && !end);
        InterlockedExchange(&JNETMeshLife[81],valid ? 0 : 1);
    }
    return result;
}
static uint32_t base_handoff_observer(uintptr_t executor,uintptr_t begin,uintptr_t end,uintptr_t fence) {
    if (active_cloth_backend) for (unsigned i=0;i<JC_CLOTH_FRAMES;++i) {
        const struct jc_cloth_stock_batch *b=active_cloth_backend->stock_batches+i;
        if (b->begin && begin<b->end && b->begin<end) {
            active_cloth_backend->failed=1;
            active_cloth_backend->frames.frames[i].phase=JC_FRAME_UNKNOWN;
            message("[JC] cloth batch reached an unsupported base handoff; retained");
            return 5;
        }
    }
    int counted=preview_count_entry(85);
    if (counted) capture_execution_range(executor,begin,end);
    int handled=0;
    uint32_t result=jc_cloth_render_base_handoff(executor,begin,end,fence,original_base_handoff,&handled);
    if (!handled) result=original_base_handoff(executor,begin,end,fence);
    if (counted) {
        InterlockedIncrement(&JNETMeshLife[86]);
        if (!result) InterlockedIncrement(&JNETMeshLife[87]);
        else InterlockedCompareExchange(&JNETMeshLife[88],(LONG)result,0);
    }
    return result;
}
static uint32_t command_execute_observer(uintptr_t executor,uintptr_t begin,uintptr_t end) {
    unsigned cloth_mask=jc_live_retirement_enter(&cloth_retirement,executor,begin,end);
    unsigned actor_mask=jc_live_retirement_enter(&actor_frame.retirement,executor,begin,end);
    uintptr_t cloth_device_context=0;
    if (cloth_mask || actor_mask) {
        if (!own_read(NULL,(uintptr_t)GetModuleHandleW(NULL)+JC_D3D_CONTEXT_SLOT_RVA,
                &cloth_device_context,sizeof(cloth_device_context)))
            cloth_device_context=0;
    }
    int matched=0;
    if (InterlockedCompareExchange(&JNETMeshLife[89],0,0)==1) {
        if (GetTickCount64()>=execution_deadline) InterlockedCompareExchange(&JNETMeshLife[89],-3,1);
        else if (executor==execution_owner && begin==execution_begin && end==execution_end) {
            unsigned char first[128];
            int valid=own_read(NULL,begin,first,sizeof(first));
            if (!valid) InterlockedCompareExchange(&JNETMeshLife[89],-1,1);
            else if (memcmp(first,execution_first,sizeof(first))) InterlockedCompareExchange(&JNETMeshLife[89],-4,1);
            else matched=InterlockedCompareExchange(&JNETMeshLife[89],2,1)==1;
        }
    }
    if (matched) draw_input_snapshot(127,execution_first);
    uint32_t result=original_command_execute(executor,begin,end);
    if (active_cloth_backend) {
        uint64_t image=(uintptr_t)GetModuleHandleW(NULL);
        struct jc_cloth_graphics_ops graphics=jc_cloth_graphics_native_ops(image);
        struct jc_cloth_graphics_token token={0};
        int locked=!jc_cloth_graphics_lock(own_read,NULL,image,&graphics,
            (cloth_mask || actor_mask) ? cloth_device_context : 0,&token);
        if (cloth_mask) jc_live_retirement_leave(&cloth_retirement,cloth_mask,result,
            locked ? (ID3D11DeviceContext *)(uintptr_t)token.device_context : NULL);
        else if (locked) jc_live_retirement_poll(&cloth_retirement);
        if (actor_mask) jc_live_retirement_leave(&actor_frame.retirement,actor_mask,result,
            locked ? (ID3D11DeviceContext *)(uintptr_t)token.device_context : NULL);
        else if (locked) jc_live_retirement_poll(&actor_frame.retirement);
        if (!locked || jc_cloth_graphics_unlock(own_read,NULL,image,&graphics,&token))
            active_cloth_backend->failed=1;
    }
    if (begin==(uintptr_t)cloth_progress_command && end==begin+sizeof(cloth_progress_command))
        InterlockedExchange(&cloth_progress_pending,result ? -1 : 0);
    if (matched) {
        InterlockedExchange(&JNETMeshLife[91],(LONG)result);
        InterlockedExchange(&JNETMeshLife[92],GetCurrentThreadId()==execution_submit_thread);
        InterlockedExchange(&JNETMeshLife[89],3);
    }
    return result;
}
static void cloth_progress_callback(const uintptr_t *payload) {
    (void)payload;
    uint64_t image=(uintptr_t)GetModuleHandleW(NULL);
    struct jc_cloth_graphics_ops graphics=jc_cloth_graphics_native_ops(image);
    struct jc_cloth_graphics_token token={0};
    if (jc_cloth_graphics_lock(own_read,NULL,image,&graphics,0,&token)) {
        if (active_cloth_backend) active_cloth_backend->failed=1;
        return;
    }
    jc_live_retirement_progress(&cloth_retirement);
    jc_live_retirement_progress(&actor_frame.retirement);
    if (jc_cloth_graphics_unlock(own_read,NULL,image,&graphics,&token) && active_cloth_backend)
        active_cloth_backend->failed=1;
}
static void cloth_progress_request(void) {
    if (!active_cloth_backend || !jc_cloth_live_owner(active_cloth_backend) ||
        InterlockedCompareExchange(&cloth_progress_pending,0,0)) return;
    uintptr_t executor=(uintptr_t)jc_live_retirement_pending_executor(&cloth_retirement);
    uintptr_t actor_executor=(uintptr_t)jc_live_retirement_pending_executor(&actor_frame.retirement);
    if (executor && actor_executor && executor!=actor_executor) {active_cloth_backend->failed=1;return;}
    if (!executor) executor=actor_executor;
    if (!executor) return;
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL),reservation=0;
    uintptr_t (*get_executor)(void)=(void *)(image+0x378930);
    uint32_t (*reserve)(uintptr_t,uintptr_t *)=(void *)(image+0x378790);
    if (executor!=get_executor()) { active_cloth_backend->failed=1; return; }
    memset(cloth_progress_command,0,sizeof(cloth_progress_command));
    uint32_t kind=0x2c;
    uintptr_t callback=(uintptr_t)cloth_progress_callback;
    memcpy(cloth_progress_command,&kind,4);
    memcpy(cloth_progress_command+8,&callback,8);
    InterlockedExchange(&cloth_progress_pending,1);
    if (reserve(executor,&reservation) || !reservation) {
        InterlockedExchange(&cloth_progress_pending,-1); active_cloth_backend->failed=1; return;
    }
    if (original_render_handoff(executor,(uintptr_t)&reservation,0,
            (uintptr_t)cloth_progress_command,(uintptr_t)cloth_progress_command+128)) {
        InterlockedExchange(&cloth_progress_pending,-1); active_cloth_backend->failed=1;
    }
}
static uintptr_t render_game,render_barn,render_context,render_queue;
static uint64_t render_model,render_control;
static DWORD render_thread;
static LONG render_frame;
static ULONGLONG render_deadline;
static struct jc_render_workspace render_work;
struct appearance_context {
    struct callback_read_budget budget;
    uint64_t material;
    struct jc_appearance_bindings destination;
};
static int appearance_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct appearance_context *ctx=opaque;
    return callback_read(&ctx->budget,address,out,size);
}
static int appearance_destination(struct appearance_context *ctx,uint64_t material,const char *name,
                                  unsigned kind,unsigned *index) {
    uint64_t payload=0;
    if (material!=ctx->material || GetCurrentThreadId()!=initialized_life.thread || lifetime_conflict ||
        jc_material_ready(appearance_read,ctx,material,&payload) || payload!=ctx->destination.payload) return 0;
    for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i) if (name==jc_appearance_names[i] &&
        jc_appearance_kind(i)==kind && (ctx->destination.found&(1u<<i))) { *index=i; return 1; }
    return 0;
}
static int appearance_scalar(void *opaque,uint64_t material,const char *name,float value) {
    struct appearance_context *ctx=opaque; unsigned index;
    if (!isfinite(value) || !appearance_destination(ctx,material,name,0,&index)) return 0;
    float *(*lookup)(uintptr_t,const char *)=(void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_MaterialScalarLookup);
    float *destination=lookup(ctx->destination.payload,name);
    if ((uintptr_t)destination!=ctx->destination.buffer+ctx->destination.offsets[index]) return 0;
    memcpy(destination,&value,sizeof(value)); return 1;
}
static int appearance_vector(void *opaque,uint64_t material,const char *name,const float value[4]) {
    struct appearance_context *ctx=opaque; unsigned index;
    if (!appearance_destination(ctx,material,name,1,&index)) return 0;
    for (unsigned i=0;i<4;++i) if (!isfinite(value[i])) return 0;
    float *(*lookup)(uintptr_t,const char *)=(void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_MaterialScalarLookup);
    if ((uintptr_t)lookup(ctx->destination.payload,name)!=ctx->destination.buffer+ctx->destination.offsets[index]) return 0;
    void (*set)(uintptr_t,const char *,const float *)=(void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_MaterialVectorSet);
    set(material,name,value); return 1;
}
static int appearance_texture(void *opaque,uint64_t material,const char *name,uint64_t texture) {
    struct appearance_context *ctx=opaque; unsigned index;
    if (!texture || !appearance_destination(ctx,material,name,2,&index)) return 0;
    void (*set)(uintptr_t,const char *,uintptr_t)=(void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_MaterialTextureSet);
    set(material,name,texture); return 1;
}
static int appearance_prepare(void) {
    uint64_t definition=0;
    const uint64_t materials[]={initialized_life.barn+0x290,initialized_life.barn+0x148d0,
                               initialized_life.object+0x1e0};
    struct appearance_context ctx={{.deadline=GetTickCount64()+500},materials[2],{0}};
    struct jc_appearance_preparation result={.originals_unchanged=-1,.detail={.parameter=-1,.table=-1,.row=-1}};
    struct jc_appearance_plan plan;
    const struct jc_appearance_setters setters={appearance_scalar,appearance_vector,appearance_texture};
    ULONGLONG start=GetTickCount64(); int code=JC_LAYOUT_READ;
    if (appearance_read(&ctx,initialized_life.barn+0xb0+0x2238,&definition,8) && definition) {
        code=jc_appearance_plan_read(appearance_read,&ctx,definition,&plan);
        result.check=1;
        if (!code) {
            result.check=4;
            code=jc_appearance_bindings_read_detail(appearance_read,&ctx,materials[2],&plan,&ctx.destination,&result.detail);
        }
        if (!code && JNETDual[0]==1) {
            uint64_t other=body_contexts[1-body_context->index].s_initialized_life.object;
            struct jc_appearance_bindings sibling;
            code=other ? jc_appearance_bindings_read(appearance_read,&ctx,other+0x1e0,&plan,&sibling) : 3;
            if (!code && !jc_appearance_storage_disjoint(&ctx.destination,&sibling)) code=3;
        }
        if (!code) code=jc_appearance_prepare(appearance_read,&ctx,&setters,definition,materials,&result);
        if (!code) code=jc_appearance_borrow_begin(appearance_read,&ctx,definition,materials[0],materials[2],&appearance_borrow);
    }
    if (ctx.budget.expired || GetTickCount64()>=ctx.budget.deadline) code=9;
    InterlockedExchange(&JNETAppearance[0],code);
    InterlockedExchange(&JNETAppearance[1],result.stage);
    InterlockedExchange(&JNETAppearance[2],result.calls);
    InterlockedExchange(&JNETAppearance[3],result.originals_unchanged);
    InterlockedExchange(&JNETAppearance[4],result.source_found);
    InterlockedExchange(&JNETAppearance[5],result.trial_before);
    InterlockedExchange(&JNETAppearance[6],result.trial_after);
    InterlockedExchange(&JNETAppearance[7],result.check);
    InterlockedExchange(&JNETAppearance[8],(LONG)(GetTickCount64()-start));
    InterlockedExchange(&JNETAppearance[11],result.detail.field);
    InterlockedExchange(&JNETAppearance[12],result.detail.parameter);
    InterlockedExchange(&JNETAppearance[13],result.detail.table);
    InterlockedExchange(&JNETAppearance[14],result.detail.row);
    InterlockedExchange(&JNETAppearance[15],result.detail.variant);
    InterlockedExchange(&JNETAppearance[16],result.detail.instances);
    InterlockedExchange(&JNETAppearance[17],result.detail.bytes);
    InterlockedExchange(&JNETAppearance[18],result.detail.offset);
    InterlockedExchange(&JNETAppearance[19],result.detail.declared);
    InterlockedExchange(&JNETAppearance[20],result.detail.required);
    InterlockedExchange(&JNETAppearance[21],result.detail.unique_instances);
    InterlockedExchange(&JNETAppearance[22],result.check>=5 ? result.source_reason : -1);
    InterlockedExchange(&JNETAppearance[23],result.check>=5 ? result.source_parameter : -1);
    InterlockedExchange(&JNETAppearance[24],result.check>=5 ? result.source_code : -1);
    InterlockedExchange(&JNETAppearance[25],result.trial_found);
    InterlockedExchange(&JNETAppearance[26],result.source_matching);
    InterlockedExchange(&JNETAppearance[27],result.missing_source);
    InterlockedExchange(&JNETAppearance[28],result.missing_trial);
    InterlockedExchange(&JNETAppearance[29],result.definition_difference);
    InterlockedExchange(&JNETAppearance[30],result.refreshed_indices);
    InterlockedExchange(&JNETAppearance[31],result.cube_stored_index);
    InterlockedExchange(&JNETAppearance[32],result.cube_current_index);
    InterlockedExchange(&JNETAppearance[33],result.cube_slots);
    char line[JC_CHECKPOINT_LENGTH+1];
    snprintf(line,sizeof(line),JC_CP_APPEARANCE_RESULT,
        code,result.stage,result.calls,result.originals_unchanged,result.check,result.source_found,result.trial_before,result.trial_after);
    if (!boundary_checkpoint(line)) return 15;
    snprintf(line,sizeof(line),JC_CP_APPEARANCE_SOURCE,
        result.source_reason,result.source_parameter,result.source_code,result.source_found,result.trial_found,
        result.source_matching);
    if (!boundary_checkpoint(line)) return 15;
    snprintf(line,sizeof(line),JC_CP_APPEARANCE_TEXTURES,
        result.missing_source,result.missing_trial,result.definition_difference,result.refreshed_indices);
    if (!boundary_checkpoint(line)) return 15;
    snprintf(line,sizeof(line),JC_CP_APPEARANCE_LAYOUT,
        result.detail.field,result.detail.parameter,result.detail.variant,result.detail.instances,result.detail.bytes);
    if (!boundary_checkpoint(line)) return 15;
    snprintf(line,sizeof(line),JC_CP_APPEARANCE_WIDTH,
        result.detail.table,result.detail.row,result.detail.offset,result.detail.declared,result.detail.required);
    if (!boundary_checkpoint(line)) return 15;
    return code ? 20 : 0;
}
static void performance_time(unsigned last,unsigned peak,ULONGLONG started) {
    ULONGLONG elapsed=GetTickCount64()-started;
    LONG value=elapsed>INT32_MAX ? INT32_MAX : (LONG)elapsed;
    InterlockedExchange(&JNETPerformance[last],value);
    if (value>JNETPerformance[peak]) InterlockedExchange(&JNETPerformance[peak],value);
}
static int appearance_check(struct callback_read_budget *budget) {
    ULONGLONG started=GetTickCount64();
    struct jc_inspection inspection={.budget=budget};
    uint64_t definition=0;
    struct jc_appearance_borrow_detail detail={8,-1,-1};
    int code=JC_LAYOUT_READ;
    if (inspection_read(&inspection,initialized_life.barn+0xb0+0x2238,&definition,8))
        code=definition!=appearance_borrow.plan.definition ? JC_LAYOUT_CHANGED :
            jc_appearance_borrow_check_detail(inspection_read,&inspection,&appearance_borrow,&detail);
    code=jc_render_budget_result(code,budget->expired,budget->read_failed,GetTickCount64()>=budget->deadline);
    InterlockedExchange(&JNETAppearance[9],code);
    InterlockedExchange(&JNETPerformance[6],code);
    InterlockedExchange(&JNETPerformance[7],detail.stage);
    InterlockedExchange(&JNETPerformance[8],detail.parameter);
    InterlockedExchange(&JNETPerformance[9],detail.role);
    performance_time(10,11,started);
    if (!code) InterlockedIncrement(&JNETAppearance[10]);
    else if (code!=9) {
        char line[128];
        snprintf(line,sizeof(line),"[JC] borrow stopped rc=%d stage=%d param=%d role=%d ms=%ld",code,
            detail.stage,detail.parameter,detail.role,(long)JNETPerformance[10]);
        boundary_checkpoint(line);
    }
    return code;
}
static int trial_shader_write(void *opaque,uint64_t address,const void *value,size_t bytes) {
    struct jc_inspection *inspection=opaque;
    struct callback_read_budget *budget=inspection->budget;
    jc_read_batch_reset(&inspection->batch);
    if (budget->expired || GetTickCount64()>=budget->deadline || (bytes!=16 && bytes!=64)) return 0;
    return jc_local_windows_write(address,value,bytes);
}
static int trial_animation_write(void *opaque,uint64_t address,const void *value,size_t bytes) {
    struct callback_read_budget *budget=opaque;
    if (budget->expired || GetTickCount64()>=budget->deadline || bytes!=20) return 0;
    return jc_local_windows_write(address,value,bytes);
}
static int trial_stamp_write(void *opaque,uint64_t address,const void *value,size_t bytes) {
    struct callback_read_budget *budget=opaque;
    if (budget->expired || GetTickCount64()>=budget->deadline || bytes!=4) return 0;
    return jc_local_windows_write(address,value,bytes);
}
static int trial_shader_prepare(uint64_t model,uint64_t root,int updating,const float transform[16]) {
    ULONGLONG started=GetTickCount64(); struct callback_read_budget budget={.deadline=started+500};
    struct jc_inspection inspection={.budget=&budget};
    float world[16],again[16];
    const uint64_t materials[]={initialized_life.barn+0x290,initialized_life.barn+0x148d0,initialized_life.object+0x1e0};
    struct jc_trial_shader_result result={0,-1,-1};
    int step=1,code=updating ? jc_trial_spine_lookup(inspection_read,&inspection,model,root,world) :
        jc_trial_spine(inspection_read,&inspection,model,root,world);
    if (!code) {
        step=2;
        code=jc_trial_shader_update(inspection_read,trial_shader_write,&inspection,appearance_borrow.plan.definition,materials,world,updating ? preview_shader_world : NULL,&result);
    }
    if (!code) {
        step=3; code=updating ? jc_trial_spine_lookup(inspection_read,&inspection,model,root,again) :
            jc_trial_spine(inspection_read,&inspection,model,root,again);
        if (!code && memcmp(world,again,sizeof(world))) code=JC_LAYOUT_CHANGED;
    }
    if (budget.expired || GetTickCount64()>=budget.deadline) code=9;
    InterlockedExchange(&JNETAppearance[34],code);
    InterlockedExchange(&JNETAppearance[35],step);
    InterlockedExchange(&JNETAppearance[36],result.stage);
    InterlockedExchange(&JNETAppearance[37],result.originals_unchanged);
    InterlockedExchange(&JNETAppearance[38],result.trial_only);
    InterlockedExchange(&JNETAppearance[39],(LONG)(GetTickCount64()-started));
    char line[160];
    snprintf(line,sizeof(line),"[JC] trial shader result=%d step=%d stage=%d originals=%d trial_only=%d",code,step,result.stage,result.originals_unchanged,result.trial_only);
    if ((!updating || code) && !boundary_checkpoint(line)) return 15;
    if (!code) {
        memcpy(preview_shader_world,world,sizeof(world));
        float inverse[16],relative[16];
        if (!jc_trial_inverse(transform,inverse)) {
            jc_trial_multiply(inverse,world,relative);
            int changed=0;
            for (unsigned i=0;i<16;++i) {
                float tolerance=i>=12 && i<=14 ? .002f+fabsf(transform[i])*1e-6f : .002f;
                if (fabsf(relative[i]-body_context->motion_spine[i])>tolerance) changed=1;
            }
            if (changed && body_context->motion_spine_known) InterlockedIncrement(&JNETMotion[10+body_context->index*7]);
            memcpy(body_context->motion_spine,relative,sizeof(relative)); body_context->motion_spine_known=1;
        }
    }
    return code ? 22 : 0;
}
static void preview_fail(int code) {
    InterlockedCompareExchange(&JNETMeshLife[27],code,0);
    char line[JC_CHECKPOINT_LENGTH+1];
    snprintf(line,sizeof(line),"[JC] preview stop=%d first=%ld stage=%ld draws=%ld cp=%ld os=%lu",code,
             (long)JNETMeshLife[27],(long)JNETMeshLife[28],(long)JNETMeshLife[26],
             (long)JNETMeshLife[116],(unsigned long)JNETMeshLife[117]);
    boundary_checkpoint(line);
    InterlockedExchange(&JNETMeshLife[25],-1);
}
static int preview_owner_check(struct callback_read_budget *budget,uint64_t game) {
    ULONGLONG started=GetTickCount64(); unsigned reads=budget->reads;
    struct jc_initialized_detail detail;
    int code=jc_initialized_validate_detail(&initialized_life,callback_read,budget,
        (uintptr_t)GetModuleHandleW(NULL),game,GetCurrentThreadId(),&detail);
    code=jc_render_budget_result(code,budget->expired,budget->read_failed,GetTickCount64()>=budget->deadline);
    InterlockedExchange(&JNETPerformance[18],code);
    InterlockedExchange(&JNETPerformance[19],detail.field);
    performance_time(20,21,started);
    InterlockedExchange(&JNETPerformance[22],(LONG)(budget->reads-reads));
    InterlockedExchange(&JNETPerformance[23],detail.pool_valid ? (LONG)detail.slots : -1);
    InterlockedExchange(&JNETPerformance[24],detail.pool_valid ? (LONG)detail.occupied : -1);
    InterlockedExchange(&JNETPerformance[25],detail.pool_valid ? (LONG)detail.free_slots : -1);
    InterlockedExchange(&JNETPerformance[26],detail.pool_valid);
    InterlockedExchange(&JNETPerformance[27],detail.comparison_valid ? detail.same_pool : -1);
    InterlockedExchange(&JNETPerformance[28],detail.comparison_valid ? detail.changed_mask : -1);
    InterlockedExchange(&JNETPerformance[29],detail.comparison_valid);
    return code;
}
static void preview_check_failed(int failure,int code,struct callback_read_budget *budget,int pose_written) {
    code=jc_render_budget_result(code,budget->expired,budget->read_failed,GetTickCount64()>=budget->deadline);
    InterlockedExchange(&JNETPerformance[16],failure);
    InterlockedExchange(&JNETPerformance[17],code);
    uint32_t queued=1;
    if (preview_queue) memcpy(&queued,preview_queue+0x10080,4);
    int defer=jc_render_budget_defer(&preview_retry,code,(unsigned)JNETMeshLife[26],pose_written,queued);
    InterlockedExchange(&JNETPerformance[13],(LONG)preview_retry.total);
    InterlockedExchange(&JNETPerformance[14],(LONG)preview_retry.consecutive);
    if (defer) { InterlockedExchange(&JNETMeshLife[25],1); return; }
    char line[JC_CHECKPOINT_LENGTH+1];
    snprintf(line,sizeof(line),"[JC] check=%d rc=%d owner=%ld field=%ld ms=%ld reads=%ld",failure,code,
        (long)JNETPerformance[18],(long)JNETPerformance[19],
        (long)JNETPerformance[20],(long)JNETPerformance[22]);
    boundary_checkpoint(line);
    preview_fail(failure);
}
static void shader_inputs_snapshot(void) {
    struct jc_shader_inputs values[2]={{0},{0}};
    const uint64_t materials[]={initialized_life.barn+0x210+0x80,initialized_life.object+0x160+0x80};
    int codes[2]; ULONGLONG start=GetTickCount64();
    struct callback_read_budget budget={.deadline=start+250};
    for (unsigned i=0;i<2;++i) {
        unsigned offset=138+i*6;
        int code=jc_shader_inputs_read(callback_read,&budget,materials[i],&values[i]);
        if (budget.expired || GetTickCount64()>=budget.deadline) code=9;
        codes[i]=code;
        if (!code) {
            InterlockedExchange(&JNETMeshLife[offset+1],(LONG)values[i].found);
            InterlockedExchange(&JNETMeshLife[offset+2],(LONG)values[i].finite);
            InterlockedExchange(&JNETMeshLife[offset+3],(LONG)values[i].zero);
            InterlockedExchange(&JNETMeshLife[offset+4],values[i].dissolve_milli);
            InterlockedExchange(&JNETMeshLife[offset+5],(LONG)values[i].bytes);
        }
        InterlockedExchange(&JNETMeshLife[offset],code);
    }
    if (!codes[0] && !codes[1])
        InterlockedExchange(&JNETMeshLife[150],values[0].buffer+values[0].bytes<=values[1].buffer ||
            values[1].buffer+values[1].bytes<=values[0].buffer ? 1 : 0);
    InterlockedExchange(&JNETMeshLife[151],(LONG)(GetTickCount64()-start));
}
static void mirror_elapsed(void) {
    ULONGLONG now=GetTickCount64();
    ULONGLONG elapsed=mirror_not_before && now>mirror_not_before ? now-mirror_not_before : 0;
    InterlockedExchange(&JNETSkeleton[7],elapsed>INT32_MAX ? INT32_MAX : (LONG)elapsed);
}
static int mirror_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct jc_inspection *inspection=opaque;
    return callback_read(inspection->budget,address,out,size);
}
static int mirror_apply(struct callback_read_budget *budget,unsigned *written) {
    unsigned row=8+body_context->index*8,changed=0;
    ULONGLONG started=GetTickCount64(); unsigned before=budget->reads;
    struct jc_inspection inspection={.budget=budget};
    int code=jc_skel_apply(mirror_read,trial_shader_write,&inspection,(uintptr_t)GetModuleHandleW(NULL),
        mirror_work,body_context->index,written,&changed);
    if (budget->expired && !*written) code=9;
    LONG elapsed=(LONG)(GetTickCount64()-started);
    InterlockedExchange(&JNETSkeleton[row],code);
    InterlockedExchange(&JNETSkeleton[row+1],(LONG)mirror_work->stage);
    if (!code) {
        InterlockedIncrement(&JNETSkeleton[row+2]);
        if (changed) InterlockedIncrement(&JNETSkeleton[row+3]);
    }
    InterlockedExchange(&JNETSkeleton[row+4],elapsed);
    if (elapsed>JNETSkeleton[row+5]) InterlockedExchange(&JNETSkeleton[row+5],elapsed);
    InterlockedExchange(&JNETSkeleton[row+6],(LONG)(budget->reads-before));
    InterlockedExchange(&JNETSkeleton[row+7],(LONG)*written);
    return code;
}
static int mirror_post_render(void) {
    unsigned row=24+body_context->index*4;
    ULONGLONG started=GetTickCount64();
    struct callback_read_budget budget={.deadline=started+250};
    int code=jc_skel_post_render(callback_read,&budget,mirror_work,body_context->index);
    LONG elapsed=(LONG)(GetTickCount64()-started);
    InterlockedExchange(&JNETSkeleton[row],elapsed);
    if (elapsed>JNETSkeleton[row+1]) InterlockedExchange(&JNETSkeleton[row+1],elapsed);
    InterlockedExchange(&JNETSkeleton[row+2],(LONG)budget.reads);
    return code;
}
static void pose_link_distance(const float local[3],const float remote[3]) {
    double square=0;
    for (unsigned i=0;i<3;++i) { double d=(double)local[i]-remote[i]; square+=d*d; }
    double milli=sqrt(square)*1000;
    InterlockedExchange(&JNETLink[11],!isfinite(milli) ? -1 : milli>INT32_MAX ? INT32_MAX : (LONG)milli);
}
static void preview_draw(uintptr_t queue,uintptr_t device,uint32_t flags,int allow_pose) {
    struct jc_pose_receiver mirrored={0};
    const struct jc_pose_receiver *receiver=pose_link.mode>=4 ? &pose_link.room.peers[body_context->index] : &pose_link.receiver;
    int streamed=preview_linked || mirror_mode==1;
    if (mirror_mode==1) {
        if (!own_read(NULL,initialized_life.barn+0x740,mirrored.sample.transform,64)) { preview_fail(26); return; }
        mirrored.sample.transform[12]+=body_context->index ? -3.0f : 3.0f;
        if (!jc_pose_transform_valid(mirrored.sample.transform)) { preview_fail(26); return; }
        mirrored.sample.sequence=GetTickCount64()/50+1;
        receiver=&mirrored;
        if (GetTickCount64()<body_context->mirror_next) allow_pose=0;
    }
    if (!preview_queue && !allow_pose) return;
    if (JNETMeshLife[25]!=1 && JNETMeshLife[25]!=2) return;
    ULONGLONG now=GetTickCount64();
    if (now<preview_not_before) {
        InterlockedExchange(&JNETMeshLife[109],(LONG)((preview_not_before-now+999)/1000));
        return;
    }
    InterlockedExchange(&JNETMeshLife[109],0);
    if (now>=preview_deadline || (preview_linked && pose_link.mode!=1 && pose_link.mode!=3 && pose_link.mode!=4 && pose_link.mode!=5)) { InterlockedExchange(&JNETMeshLife[25],3); return; }
    if (preview_linked && (GetCurrentThreadId()!=pose_link.thread ||
        !jc_pose_fresh(receiver,(uint32_t)prepared_level,now))) return;
    if (!TryAcquireSRWLockExclusive(&lifetime_lock)) { preview_fail(1); return; }
    uint64_t game=(uintptr_t)JNETGame,context=0,primary=0,view=0,type=0;
    uint32_t original_count=0,after_count=0; unsigned char pass=0;
    int pose_written=0;
    struct jc_dual_pose_snapshot sibling_before,sibling_after;
    uint64_t sibling_object=body_contexts[1-body_context->index].s_initialized_life.object;
    struct callback_read_budget budget={.deadline=GetTickCount64()+100};
    if (initialized_life.state!=1 || game!=initialized_life.game ||
        GetCurrentThreadId()!=initialized_life.thread || lifetime_conflict) { preview_fail(2); goto done; }
    if (!callback_read(&budget,game+0x98,&context,8) ||
        !callback_read(&budget,context+0x18,&primary,8)) {
        preview_check_failed(3,JC_LAYOUT_READ,&budget,pose_written); goto done;
    }
    if (queue!=primary) goto done;
    InterlockedExchange(&JNETPerformance[6],-1);
    InterlockedExchange(&JNETPerformance[18],-1);
    int level=-1;
    int level_code=jc_render_level(callback_read,&budget,game,&level);
    InterlockedExchange(&JNETMeshLife[96],level);
    InterlockedExchange(&JNETMeshLife[97],level_code);
    if (level_code) { preview_check_failed(19,level_code,&budget,pose_written); goto done; }
    if (!jc_render_trial_same_level(prepared_level,level)) { preview_fail(19); goto done; }
    if (!preview_source_queue) {
        if (budget.expired || GetTickCount64()>=budget.deadline) { preview_fail(10); goto done; }
        if (!boundary_checkpoint("[JC] preview matching consumer returned; beginning trial validation")) { preview_fail(15); goto done; }
        /* First preflight includes the same full pose scan as preparation (250 ms).
           Start after the durable checkpoint so disk latency does not spend its read budget. */
        budget.deadline=GetTickCount64()+250;
    }
    if (preview_source_queue && primary!=preview_source_queue) { preview_fail(2); goto done; }
    int owner_code=preview_owner_check(&budget,game);
    if (owner_code) { preview_check_failed(4,owner_code,&budget,pose_written); goto done; }
    unsigned char view_data[0xc0];
    if (!callback_read(&budget,queue,&type,8)) {
        preview_check_failed(5,JC_LAYOUT_READ,&budget,pose_written); goto done;
    }
    if (type!=(uintptr_t)GetModuleHandleW(NULL)+0x6896e8) { preview_fail(5); goto done; }
    if (!callback_read(&budget,queue+0x10080,&original_count,4)) {
        preview_check_failed(5,JC_LAYOUT_READ,&budget,pose_written); goto done;
    }
    if (original_count) { preview_fail(5); goto done; }
    if (!callback_read(&budget,queue+0x70,&view,8) ||
        !callback_read(&budget,view,view_data,sizeof(view_data)) ||
        !callback_read(&budget,queue+0x78,&pass,1)) {
        preview_check_failed(5,JC_LAYOUT_READ,&budget,pose_written); goto done;
    }
    if (!preview_queue) {
        if (!boundary_checkpoint("[JC] preview appearance preparation enter; trial material only")) { preview_fail(15); goto done; }
        preview_started=1; /* Borrowed material resources require restart even after a partial setter failure. */
        int appearance_code=appearance_prepare();
        if (appearance_code) { preview_fail(appearance_code); goto done; }
        budget.deadline=GetTickCount64()+250;
        struct jc_idle_pose inputs; float transform[16],origin[3];
        uintptr_t image=(uintptr_t)GetModuleHandleW(NULL),mesh=initialized_life.object+0x160;
        struct jc_idle_detail detail;
        ULONGLONG input_started=GetTickCount64();
        int step=1,code=jc_idle_pose_ready_detail(callback_read,&budget,image,mesh,&inputs,&detail);
        if (!code) {
            step=2;
            if (!callback_read(&budget,initialized_life.barn+0xb0+0x160+0x530,transform,sizeof(transform))) code=1;
        }
        if (!code) {
            step=3;
            if (streamed) {
                memcpy(origin,transform+12,12);
                memcpy(transform,receiver->sample.transform,64);
                if (!jc_pose_transform_valid(transform)) code=4;
            } else if (!jc_render_preview_transform(transform,origin)) code=4;
            else if (native_playback_mode==1 && body_context->index) transform[12]-=6.0f;
        }
        if (budget.expired || GetTickCount64()>=budget.deadline) code=9;
        InterlockedExchange(&JNETMeshLife[110],code);
        InterlockedExchange(&JNETMeshLife[111],detail.field);
        InterlockedExchange(&JNETMeshLife[112],detail.index);
        InterlockedExchange(&JNETMeshLife[113],step);
        ULONGLONG input_elapsed=GetTickCount64()-input_started;
        InterlockedExchange(&JNETMeshLife[114],input_elapsed>0x7fffffff ? 0x7fffffff : (LONG)input_elapsed);
        if (code) {
            char why[160];
            snprintf(why,sizeof(why),"[JC] preview input rejected code=%d field=%d index=%d step=%d elapsed_ms=%ld",
                code,detail.field,detail.index,step,(long)JNETMeshLife[114]);
            boundary_checkpoint(why);
            preview_fail(6); goto done;
        }
        preview_queue=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,0x10090);
        if (!preview_queue) { preview_fail(7); goto done; }
        memcpy(preview_queue,&type,8);
        if (jc_render_track_begin(&preview_tracking,(uintptr_t)preview_queue,1,inputs.model,inputs.control,&preview_work)) {
            preview_fail(8); goto done;
        }
        preview_source_queue=queue;
        if (preview_linked && preview_deadline>GetTickCount64()+180000) preview_deadline=GetTickCount64()+180000;
        if (!boundary_checkpoint("[JC] preview inputs accepted; transform write and private CPU update next")) { preview_fail(15); goto done; }
        preview_started=1;
        memcpy(preview_origin,origin,sizeof(preview_origin));
        if (!sibling_object || jc_dual_pose_snapshot_read(own_read,NULL,image,sibling_object,&sibling_before)) { preview_fail(24); goto done; }
        pose_written=1;
        memcpy((void *)(mesh+0x530),transform,sizeof(transform));
        InterlockedExchange(&JNETMeshLife[28],1);
        void (*mesh_update)(uintptr_t,void *)=(void *)(image+JC_RVA_MeshUpdate);
        if (!boundary_checkpoint("[JC] preview private CPU update enter")) { preview_fail(15); goto done; }
        flight_mark(10,(LONG)body_context->index+1);
        mesh_update(mesh,preview_queue);
        flight_mark(11,(LONG)body_context->index+1);
        if (!boundary_checkpoint("[JC] preview private CPU update returned")) { preview_fail(15); goto done; }
        InterlockedExchange(&JNETMeshLife[28],2);
        uint32_t count=0,refs=0; memcpy(&count,preview_queue+0x10080,4);
        if (count!=1 || !jc_idle_context_valid(preview_queue,&inputs) ||
            !own_read(NULL,inputs.control,&refs,4) || refs!=inputs.references+1 ||
            jc_render_track_assignment(&preview_tracking,(uintptr_t)preview_queue,1,0,0,0,inputs.model,inputs.control)) {
            preview_fail(9); goto done;
        }
        InterlockedExchange(&JNETMeshLife[29],(LONG)preview_tracking.held);
        InterlockedExchange(&JNETMeshLife[30],(LONG)(refs-inputs.references));
        float root_transform[16];
        int root_matches=own_read(NULL,inputs.root+0x20,root_transform,sizeof(root_transform)) &&
            !memcmp(root_transform,transform,sizeof(transform));
        InterlockedExchange(&JNETMeshLife[40],root_matches);
        if (!root_matches) { preview_fail(16); goto done; }
        if (!boundary_checkpoint("[JC] trial spine shader preparation enter")) { preview_fail(15); goto done; }
        flight_mark(12,(LONG)body_context->index+1);
        int shader_code=trial_shader_prepare(inputs.model,inputs.root,0,transform);
        flight_mark(13,(LONG)body_context->index+1);
        if (shader_code) { preview_fail(shader_code); goto done; }
        if (jc_dual_pose_snapshot_read(own_read,NULL,image,sibling_object,&sibling_after) ||
            !jc_dual_pose_same(&sibling_before,&sibling_after)) { preview_fail(24); goto done; }
        memcpy(preview_applied_transform,transform,sizeof(transform));
        if (streamed) { if (preview_linked) pose_link_distance(origin,transform+12); preview_pose_sequence=receiver->sample.sequence; memcpy(preview_pose_identity,receiver->sample.identity,16); if (preview_linked) pose_link_applied(body_context->index); }
        budget.deadline=GetTickCount64()+250;
    }
    if (preview_linked && (preview_pose_sequence!=receiver->sample.sequence || memcmp(preview_pose_identity,receiver->sample.identity,16)) &&
        !memcmp(preview_applied_transform,receiver->sample.transform,64) &&
        !memcmp(&body_context->applied_animation,&receiver->sample.animation,sizeof(receiver->sample.animation))) {
        preview_pose_sequence=receiver->sample.sequence; memcpy(preview_pose_identity,receiver->sample.identity,16);
        InterlockedIncrement(&JNETPerformance[12]);
    }
    if (native_playback_mode!=1 && allow_pose && streamed && (preview_pose_sequence!=receiver->sample.sequence || memcmp(preview_pose_identity,receiver->sample.identity,16))) {
        ULONGLONG pose_started=GetTickCount64();
        struct jc_idle_pose inputs; struct jc_idle_detail detail;
        uintptr_t image=(uintptr_t)GetModuleHandleW(NULL),mesh=initialized_life.object+0x160;
        budget.deadline=GetTickCount64()+250;
        uint32_t count=0,refs=0; float root_transform[16],originals[2][16],check[16];
        const uint64_t original_meshes[]={initialized_life.barn+0x210,initialized_life.barn+0x14850};
        memcpy(&count,preview_queue+0x10080,4);
        if (count || !jc_render_preview_queue(preview_queue,preview_tracking.ledger.model,preview_tracking.ledger.control)) {
            preview_fail(23); goto done;
        }
        struct jc_inspection pose_inspection={.budget=&budget};
        int pose_code=jc_mesh_pose_ready_detail(mirror_mode==1 ? inspection_read : callback_read,
            mirror_mode==1 ? (void *)&pose_inspection : (void *)&budget,image,mesh,&inputs,&detail,body_context->motion_active);
        if (pose_code) { preview_check_failed(23,pose_code,&budget,0); goto done; }
        if (inputs.model!=preview_tracking.ledger.model || inputs.control!=preview_tracking.ledger.control ||
            !own_read(NULL,original_meshes[0]+0x530,originals[0],64) ||
            !own_read(NULL,original_meshes[1]+0x530,originals[1],64)) { preview_fail(23); goto done; }
        if (!sibling_object || jc_dual_pose_snapshot_read(own_read,NULL,image,sibling_object,&sibling_before)) { preview_fail(24); goto done; }
        unsigned motion_row=4+body_context->index*7;
        int animation_written=0,animation_code=0;
        ULONGLONG animation_started=GetTickCount64();
        struct jc_anim_sample animation=receiver->sample.animation;
        if (!animation.total && body_context->motion_active)
            animation=(struct jc_anim_sample){.schema=body_context->motion_layout.schema,.total=body_context->motion_layout.count};
        if (animation.total) {
            const uint64_t others[]={original_meshes[0],original_meshes[1],sibling_object+0x160};
            unsigned animation_reads=budget.reads;
            animation_code=jc_anim_apply(callback_read,trial_animation_write,&budget,image,mesh,others,
                &body_context->motion_layout,&animation,&animation_written);
            LONG reads=(LONG)(budget.reads-animation_reads); unsigned cost_row=22+body_context->index*3;
            InterlockedExchange(&JNETMotion[cost_row],reads);
            if (reads>JNETMotion[cost_row+1]) InterlockedExchange(&JNETMotion[cost_row+1],reads);
            InterlockedExchange(&JNETMotion[cost_row+2],animation_written);
            animation_code=jc_render_budget_result(animation_code,budget.expired,budget.read_failed,
                GetTickCount64()>=budget.deadline);
            InterlockedExchange(&JNETMotion[motion_row],animation_code);
            InterlockedExchange(&JNETMotion[motion_row+1],(LONG)animation.count);
            InterlockedExchange(&JNETMotion[motion_row+3],(LONG)(GetTickCount64()-animation_started));
            if (animation_code) {
                InterlockedIncrement(&JNETMotion[motion_row+4]);
                if (animation_code==9 && !animation_written) { preview_check_failed(25,9,&budget,0); goto done; }
                if (animation_written || body_context->motion_active) { preview_fail(25); goto done; }
                /* An untouched inactive body can continue showing position if the clip schema is incompatible. */
            } else {
                body_context->motion_active=animation.count!=0;
                InterlockedIncrement(&JNETMotion[motion_row+2]);
            }
        } else InterlockedExchange(&JNETMotion[motion_row],-1);
        if (mirror_mode==1) {
            unsigned written=0;
            budget.deadline=GetTickCount64()+250;
            flight_mark(32,(LONG)body_context->index+1);
            int code=mirror_apply(&budget,&written);
            flight_mark(33,(LONG)body_context->index+1);
            if (code) {
                if (written) { preview_fail(26); goto done; }
                preview_check_failed(26,code,&budget,0); goto done;
            }
            body_context->mirror_next=pose_started+50;
        }
        pose_written=1;
        memcpy((void *)(mesh+0x530),receiver->sample.transform,64);
        void (*mesh_update)(uintptr_t,void *)=(void *)(image+JC_RVA_MeshUpdate);
        flight_mark(10,(LONG)body_context->index+1);
        mesh_update(mesh,preview_queue);
        flight_mark(11,(LONG)body_context->index+1);
        memcpy(&count,preview_queue+0x10080,4);
        if (animation.total && !animation_code) {
            if (!jc_anim_consumed(own_read,NULL,&body_context->motion_layout,&animation)) { preview_fail(25); goto done; }
            InterlockedIncrement(&JNETMotion[motion_row+5]);
        }
        if (count!=1 || !jc_idle_context_valid(preview_queue,&inputs) ||
            !own_read(NULL,inputs.control,&refs,4) || refs!=inputs.references ||
            jc_render_track_assignment(&preview_tracking,(uintptr_t)preview_queue,1,0,
                inputs.model,inputs.control,inputs.model,inputs.control) ||
            !own_read(NULL,inputs.root+0x20,root_transform,64) ||
            memcmp(root_transform,receiver->sample.transform,64)) { preview_fail(23); goto done; }
        for (unsigned i=0;i<2;++i) if (!own_read(NULL,original_meshes[i]+0x530,check,64) ||
            memcmp(check,originals[i],64)) { preview_fail(23); goto done; }
        flight_mark(12,(LONG)body_context->index+1);
        int shader_code=trial_shader_prepare(inputs.model,inputs.root,1,receiver->sample.transform);
        flight_mark(13,(LONG)body_context->index+1);
        if (shader_code) { preview_fail(shader_code); goto done; }
        if (mirror_mode==1 && mirror_post_render()) {
            InterlockedExchange(&JNETSkeleton[8+body_context->index*8],8);
            InterlockedExchange(&JNETSkeleton[9+body_context->index*8],8);
            preview_fail(26); goto done;
        }
        if (jc_dual_pose_snapshot_read(own_read,NULL,image,sibling_object,&sibling_after) ||
            !jc_dual_pose_same(&sibling_before,&sibling_after)) { preview_fail(24); goto done; }
        memcpy(preview_applied_transform,receiver->sample.transform,64);
        body_context->applied_animation=receiver->sample.animation;
        performance_time(4,5,pose_started);
        pose_link_distance(originals[0]+12,receiver->sample.transform+12);
        preview_pose_sequence=receiver->sample.sequence; memcpy(preview_pose_identity,receiver->sample.identity,16); if (preview_linked) pose_link_applied(body_context->index);
        budget.deadline=GetTickCount64()+250;
    }
    if (budget.expired || GetTickCount64()>=budget.deadline) {
        preview_check_failed(10,0,&budget,pose_written); goto done;
    }
    int borrow_code=appearance_check(&budget);
    if (borrow_code) {
        preview_check_failed(21,borrow_code,&budget,pose_written);
        goto done;
    }
    if (!JNETMeshLife[26]) {
        struct jc_render_visibility local={0},trial={0}; struct jc_pose_storage pose;
        uint32_t mask=0,expected_mask=0; uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
        int code=jc_pose_storage_read(callback_read,&budget,image,initialized_life.barn+0x210,&pose);
        if (!code && (!callback_read(&budget,image+0x403e3b4,&mask,4) ||
                      !callback_read(&budget,image+0x403e3fc,&expected_mask,4))) code=JC_LAYOUT_READ;
        if (!code) code=jc_render_visibility_read(callback_read,&budget,pose.model,mask,expected_mask,&local);
        if (!code) code=jc_render_visibility_read(callback_read,&budget,preview_tracking.ledger.model,mask,expected_mask,&trial);
        if (budget.expired || GetTickCount64()>=budget.deadline) code=9;
        InterlockedExchange(&JNETMeshLife[32],code);
        InterlockedExchange(&JNETMeshLife[33],(LONG)local.parts);
        InterlockedExchange(&JNETMeshLife[34],(LONG)trial.parts);
        InterlockedExchange(&JNETMeshLife[35],local.mask_pass);
        InterlockedExchange(&JNETMeshLife[36],trial.mask_pass);
        InterlockedExchange(&JNETMeshLife[37],(LONG)trial.nondegenerate);
        InterlockedExchange(&JNETMeshLife[38],local.culling);
        InterlockedExchange(&JNETMeshLife[39],trial.culling);
        if (code) { preview_fail(17); goto done; }
        struct jc_pose_quality local_quality={0},trial_quality={0};
        int quality=jc_pose_quality_read(callback_read,&budget,pose.model,&local_quality);
        if (!quality) quality=jc_pose_quality_read(callback_read,&budget,preview_tracking.ledger.model,&trial_quality);
        if (budget.expired || GetTickCount64()>=budget.deadline) quality=9;
        InterlockedExchange(&JNETMeshLife[59],local_quality.count);
        InterlockedExchange(&JNETMeshLife[60],local_quality.nonfinite);
        InterlockedExchange(&JNETMeshLife[61],local_quality.singular);
        InterlockedExchange(&JNETMeshLife[62],trial_quality.count);
        InterlockedExchange(&JNETMeshLife[63],trial_quality.nonfinite);
        InterlockedExchange(&JNETMeshLife[64],trial_quality.singular);
        InterlockedExchange(&JNETMeshLife[58],quality);
        if (quality) { preview_fail(18); goto done; }
    }
    struct jc_idle_pose expected={.model=preview_tracking.ledger.model,.control=preview_tracking.ledger.control};
    uint32_t refs_before=0,refs_after=0,count=1;
    if (preview_tracking.failed || preview_tracking.held!=1 || !jc_render_preview_queue(preview_queue,expected.model,expected.control) ||
        !own_read(NULL,expected.control,&refs_before,4) || !refs_before) { preview_fail(11); goto done; }
    memcpy(preview_queue+0x70,&view,8); memcpy(preview_queue+0x78,&pass,1);
    memcpy(preview_queue+0x10080,&count,4);
    InterlockedExchange(&JNETMeshLife[25],2);
    InterlockedExchange(&JNETMeshLife[28],3);
    if (!JNETMeshLife[26] && !boundary_checkpoint("[JC] preview private draw enter; reference acquisition validated")) { preview_fail(15); goto done; }
    InterlockedExchange(&preview_observer_thread,(LONG)GetCurrentThreadId());
    flight_mark(20,(LONG)body_context->index+1);
    original_consume((uintptr_t)preview_queue,device,flags);
    flight_mark(21,(LONG)body_context->index+1);
    InterlockedExchange(&preview_observer_thread,0);
    InterlockedExchange(&JNETMeshLife[28],4);
    if (!JNETMeshLife[26] && !boundary_checkpoint("[JC] preview private draw returned")) { preview_fail(15); goto done; }
    memcpy(&count,preview_queue+0x10080,4);
    InterlockedExchange(&JNETMeshLife[31],(LONG)count);
    if (count || !jc_render_preview_queue(preview_queue,expected.model,expected.control) ||
        !own_read(NULL,expected.control,&refs_after,4) || refs_after!=refs_before ||
        !own_read(NULL,queue+0x10080,&after_count,4) || after_count!=original_count) {
        preview_fail(12); goto done;
    }
    if (!JNETMeshLife[26] && !boundary_checkpoint("[JC] preview first draw postchecks passed")) { preview_fail(15); goto done; }
    if (!JNETMeshLife[26]) shader_inputs_snapshot();
    jc_render_budget_drawn(&preview_retry);
    InterlockedExchange(&JNETPerformance[14],0);
    InterlockedExchange(&JNETPerformance[15],(LONG)preview_retry.recovered);
    LONG draws=InterlockedIncrement(&JNETMeshLife[26]);
    if (mirror_mode!=1 && native_playback_mode!=1 && draws>=(preview_linked ? 10800 : 600)) InterlockedExchange(&JNETMeshLife[25],3);
done:
    performance_time(2,3,now);
    ReleaseSRWLockExclusive(&lifetime_lock);
}
static void render_fail(int code) {
    InterlockedExchange(&JNETRender[1],code);
    InterlockedExchange(&JNETRender[0],-1);
}
static int render_snapshot(int index) {
    struct jc_render_snapshot snapshot;
    struct callback_read_budget budget={.deadline=GetTickCount64()+100};
    uint64_t barn,context,queue;
    if (!callback_read(&budget,render_game+0x1e0,&barn,8) || barn!=render_barn ||
        !callback_read(&budget,render_game+0x98,&context,8) || context!=render_context ||
        !callback_read(&budget,context+0x18,&queue,8) || queue!=render_queue) {
        render_fail(3); return 0;
    }
    int code=jc_render_queue(callback_read,&budget,(uintptr_t)GetModuleHandleW(NULL),
                              render_queue,render_model,render_control,&render_work,&snapshot);
    if (budget.expired || GetTickCount64()>=budget.deadline) code=9;
    if (code) { render_fail(code); return 0; }
    InterlockedExchange(&JNETRender[index],snapshot.count);
    InterlockedExchange(&JNETRender[index+1],snapshot.active);
    InterlockedExchange(&JNETRender[index+2],snapshot.stored);
    return 1;
}
static void submit_observer(uintptr_t barn,uintptr_t context) {
    int observe=JNETRender[0]==1 && GetCurrentThreadId()==render_thread && barn==render_barn;
    if (observe) InterlockedExchange(&JNETRender[0],2);
    int after_cleanup=JNETRender[0]==8 && GetCurrentThreadId()==render_thread && barn==render_barn;
    original_submit(barn,context);
    traveler_actor_render(barn,context);
    if (after_cleanup && JNETRender[0]==8 && render_snapshot(20)) InterlockedExchange(&JNETRender[0],9);
    if (!observe || JNETRender[0]!=2) return;
    struct jc_pose_storage pose;
    struct callback_read_budget budget={.deadline=GetTickCount64()+100};
    int code=jc_pose_storage_read(callback_read,&budget,(uintptr_t)GetModuleHandleW(NULL),barn+0x210,&pose);
    if (code || !callback_read(&budget,pose.clone.payload+8,&render_control,8) ||
        !callback_read(&budget,context+0x18,&render_queue,8)) { render_fail(budget.expired ? 9 : code ? code : 1); return; }
    render_model=pose.model; render_context=context;
    if (render_snapshot(2)) InterlockedExchange(&JNETRender[0],3);
}
static int64_t consume_observer(uintptr_t queue,uintptr_t device,uint32_t flags) {
    traveler_glyph_trace_queue(queue);
    int local_scope=0;
    if ((JNETMeshLife[25]==1 || JNETMeshLife[25]==2) && JNETMeshLife[118]==-1 &&
        JNETMeshLife[136]<8 && !local_consumer_thread && GetCurrentThreadId()==initialized_life.thread &&
        TryAcquireSRWLockExclusive(&lifetime_lock)) {
        InterlockedIncrement(&JNETMeshLife[136]);
        uint64_t context=0,primary=0; struct jc_pose_storage pose;
        struct callback_read_budget budget={.deadline=GetTickCount64()+25};
        int code=JC_LAYOUT_CHANGED;
        if (initialized_life.state==1 && (uintptr_t)JNETGame==initialized_life.game &&
            jc_layout_field(callback_read,&budget,initialized_life.game,0x98,&context,8) &&
            jc_layout_field(callback_read,&budget,context,0x18,&primary,8) && primary==queue) {
            code=jc_pose_storage_read(callback_read,&budget,(uintptr_t)GetModuleHandleW(NULL),initialized_life.barn+0x210,&pose);
            if (!code) code=jc_render_first_item(callback_read,&budget,pose.model,&local_draw_item);
            if (budget.expired) code=9;
            if (!code) { local_scope=1; InterlockedExchange(&local_consumer_thread,(LONG)GetCurrentThreadId()); }
        }
        InterlockedExchange(&JNETMeshLife[137],code);
        ReleaseSRWLockExclusive(&lifetime_lock);
    }
    int observe=JNETRender[0]==3 && GetCurrentThreadId()==render_thread && queue==render_queue;
    if (observe) {
        InterlockedExchange(&JNETRender[0],4);
        observe=render_snapshot(5);
    }
    int64_t result=original_consume(queue,device,flags);
    if (local_scope) { InterlockedExchange(&local_consumer_thread,0); local_draw_item=0; }
    if (observe && JNETRender[0]==4 && render_snapshot(8)) InterlockedExchange(&JNETRender[0],5);
    if (GetCurrentThreadId()==resource_thread && JNETDual[0]==1) {
        unsigned first=(unsigned)JNETFrames%JC_BODY_COUNT;
        for (unsigned n=0;n<JC_BODY_COUNT;++n) {
            unsigned index=(first+n)%JC_BODY_COUNT;
            body_context=&body_contexts[index];
            if (index==0 || pose_link.mode>=4 || preview_linked || mirror_mode==1 || native_playback_mode==1) preview_draw(queue,device,flags,n==0);
            body_publish();
        }
        body_context=&body_contexts[0];
        if (mirror_mode==1) {
            mirror_elapsed();
            int active=0,failed=0;
            for (unsigned i=0;i<JC_BODY_COUNT;++i) {
                LONG state=body_contexts[i].out_MeshLife[25];
                active|=state==1 || state==2;
                failed|=state<0;
            }
            if (!active) {
                mirror_mode=2;
                InterlockedExchange(&JNETSkeleton[0],failed ? -2 : 3);
            }
        }
    }
    return result;
}
static void render_arm(uintptr_t game) {
    if (JNETRender[0]) return;
    struct jc_layout_result layout;
    struct callback_read_budget budget={.deadline=GetTickCount64()+100};
    int code=jc_native_layout(callback_read,&budget,(uintptr_t)GetModuleHandleW(NULL),game,&layout);
    if (code || !callback_read(&budget,game+0x1e0,&render_barn,8)) { render_fail(budget.expired ? 9 : code ? code : 1); return; }
    render_game=game; render_thread=GetCurrentThreadId(); render_frame=JNETFrames; render_deadline=GetTickCount64()+600000;
    InterlockedExchange(&JNETRender[0],1);
}
static int callback_snapshot_read(struct jc_callback_snapshot *out) {
    struct jc_callback_workspace *work=HeapAlloc(GetProcessHeap(),0,sizeof(*work));
    struct callback_read_budget budget={.deadline=GetTickCount64()+250};
    int code=jc_mesh_callbacks(callback_read,&budget,callback_manager,callback_meshes,out,work);
    if (work) HeapFree(GetProcessHeap(),0,work);
    return budget.expired ? JC_CALLBACK_TIME : code;
}

static uintptr_t (*original_mesh_register)(uintptr_t,uintptr_t);
static uintptr_t mesh_register_observer(uintptr_t mesh, uintptr_t context) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    uintptr_t owned=(uintptr_t)InterlockedCompareExchangePointer(&guarded_mesh,NULL,NULL);
    if (!owned || mesh!=owned) return original_mesh_register(mesh,context);
    uint64_t entries=0,count=0;
    int matching=GetCurrentThreadId()==resource_thread && context==mesh_context && !mesh_fault &&
        own_read(NULL,mesh+0x578,&entries,8) && own_read(NULL,mesh+0x580,&count,8);
    int failure=0;
    int decision=jc_mesh_guard_detail(owned,mesh,matching,caller-(uintptr_t)GetModuleHandleW(NULL),
                              entries,count,JNETMesh[1],JNETMesh[2],&failure);
    if (decision==1) {
        InterlockedExchange(&JNETMesh[1],1);
        if (!boundary_checkpoint("[JC] mesh early growth callback deferred")) InterlockedExchange(&mesh_fault,1);
        return 0;
    }
    if (decision==2) {
        InterlockedExchange(&JNETMesh[2],1); /* Consume before calling: rejects reentry. */
        if (!boundary_checkpoint("[JC] mesh initialized callback enter")) { InterlockedExchange(&mesh_fault,1); return 0; }
        uintptr_t result=original_mesh_register(mesh,context);
        if (!boundary_checkpoint("[JC] mesh initialized callback returned")) InterlockedExchange(&mesh_fault,1);
        return result;
    }
    InterlockedExchange(&mesh_fault,1);
    if (!body_context->mesh_guard_failure) body_context->mesh_guard_failure=failure;
    char line[160];
    snprintf(line,sizeof(line),"[JC] body=%u callback rejected reason=%d early=%ld ready=%ld; cleanup required",
        body_context->index+1,failure,(long)JNETMesh[1],(long)JNETMesh[2]);
    boundary_checkpoint(line);
    return 0;
}
static uint64_t mesh_create(void *image) {
    if (!boundary_checkpoint("[JC] mesh factory enter")) { InterlockedExchange(&mesh_fault,1); return 0; }
    uint64_t object=trial_create(image);
    if (!boundary_checkpoint("[JC] mesh factory returned")) InterlockedExchange(&mesh_fault,1);
    return object;
}
static int material_mark(LONG phase,const char *text) {
    InterlockedExchange(&JNETPhase,phase);
    if (boundary_checkpoint(text)) return 1;
    InterlockedExchange(&mesh_fault,1);
    return 0;
}
static void idle_pose_trial(void *image,uintptr_t mesh) {
    struct jc_idle_pose inputs;
    struct callback_read_budget budget={.deadline=GetTickCount64()+250};
    int code=jc_idle_pose_ready(callback_read,&budget,(uintptr_t)image,mesh,&inputs);
    if (budget.expired) code=9;
    InterlockedExchange(&JNETIdle[0],code);
    if (code) { InterlockedExchange(&mesh_fault,1); return; }
    unsigned char *context=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,0x10090);
    if (!context) { InterlockedExchange(&JNETIdle[0],10); InterlockedExchange(&mesh_fault,1); return; }
    /* Native caller's temporary reference context; direct update only reads its queue. */
    uint64_t type=(uintptr_t)image+0x6896e8;
    memcpy(context,&type,8);
    if (!material_mark(38,"[JC] inactive-weight mesh update enter; one call; no explicit draw")) {
        HeapFree(GetProcessHeap(),0,context); return;
    }
    InterlockedExchange(&JNETIdle[1],1);
    void (*mesh_update)(uintptr_t,void *)=(void *)((unsigned char *)image+JC_RVA_MeshUpdate);
    mesh_update(mesh,context);
    InterlockedExchange(&JNETIdle[1],2);
    boundary_record("[JC] inactive-weight mesh update returned; checking temporary references");
    uint32_t queued; memcpy(&queued,context+0x10080,4);
    InterlockedExchange(&JNETIdle[2],(LONG)queued);
    int valid=jc_idle_context_valid(context,&inputs);
    if (!valid) {
        /* Quarantine unknown references: never invoke a destructor through them. */
        InterlockedExchange(&JNETIdle[0],11); InterlockedExchange(&mesh_fault,1);
        boundary_record("[JC] unexpected temporary reference context quarantined; restart required"); return;
    }
    void (*release)(void *)=(void *)((unsigned char *)image+JC_RVA_ReferenceRelease);
    memset(context+0x10080,0,4);
    for (unsigned i=0;i<queued;++i) release(context+0x80+i*16);
    HeapFree(GetProcessHeap(),0,context);
    InterlockedExchange(&JNETIdle[1],3);
    uint32_t references=0; unsigned char expected[64],actual[64];
    int restored=own_read(NULL,inputs.control,&references,4) && references==inputs.references;
    int matched=own_read(NULL,mesh+0x530,expected,64) && own_read(NULL,inputs.root+0x20,actual,64) && !memcmp(expected,actual,64);
    InterlockedExchange(&JNETIdle[3],restored ? 1 : 0);
    InterlockedExchange(&JNETIdle[4],matched ? 1 : 0);
    if (!restored || !matched) { InterlockedExchange(&JNETIdle[0],12); InterlockedExchange(&mesh_fault,1); }
    boundary_record("[JC] idle pose reference and root checks recorded; native mesh cleanup follows");
}

static int body_pair_storage(void *image,uint64_t mesh) {
    if (!body_context->index) return 0;
    struct jc_body_context *first=&body_contexts[0];
    uint64_t object=first->s_initialized_life.object;
    if (first->s_initialized_life.state!=1 || !object ||
        !jc_clone_prefix_disjoint(object,mesh-0x160,0x14640)) return 3;
    const uint64_t meshes[3]={resource_barn+0x210,object+0x160,mesh};
    int distinct=0,models=0,roots=0;
    struct callback_read_budget budget={.deadline=GetTickCount64()+250};
    int code=jc_three_clone_check(callback_read,&budget,(uintptr_t)image,meshes,&distinct);
    if (!code && !distinct) code=3;
    if (!code) code=jc_three_pose_storage(callback_read,&budget,(uintptr_t)image,meshes,&models,&roots);
    if (!code && (!models || !roots)) code=3;
    if (budget.expired) code=9;
    if (code) return code;
    struct jc_pose_write_workspace *writes=HeapAlloc(GetProcessHeap(),0,sizeof(*writes));
    struct jc_pose_write_result result;
    budget=(struct callback_read_budget){.deadline=GetTickCount64()+250};
    code=jc_three_pose_writes(callback_read,&budget,(uintptr_t)image,meshes,&result,writes);
    if (writes) HeapFree(GetProcessHeap(),0,writes);
    if (!code && !result.disjoint) code=3;
    if (budget.expired) code=9;
    if (code) return code;
    struct jc_animation_workspace *animation=HeapAlloc(GetProcessHeap(),0,sizeof(*animation));
    struct jc_animation_result animated;
    budget=(struct callback_read_budget){.deadline=GetTickCount64()+250};
    code=jc_three_animation_storage(callback_read,&budget,(uintptr_t)image,meshes,&animated,animation);
    if (animation) HeapFree(GetProcessHeap(),0,animation);
    if (!code && !animated.disjoint) code=3;
    if (budget.expired) code=9;
    if (code) return code;
    struct jc_callback_workspace *work=HeapAlloc(GetProcessHeap(),0,sizeof(*work));
    struct jc_callback_snapshot callbacks;
    budget=(struct callback_read_budget){.deadline=GetTickCount64()+250};
    code=jc_mesh_callbacks(callback_read,&budget,first->s_callback_manager,first->s_callback_meshes,&callbacks,work);
    if (work) HeapFree(GetProcessHeap(),0,work);
    if (!code) for (unsigned i=0;i<3;++i)
        if (!jc_callback_equal(&callbacks.owners[i],&first->prepared_callbacks.owners[i])) code=3;
    return budget.expired ? 9 : code;
}
static void material_prepare(void *image,uintptr_t mesh) {
    void (*clear)(uintptr_t)=(void *)((unsigned char *)image+JC_RVA_PropertyClear);
    uintptr_t (*get)(uintptr_t,void *)=(void *)((unsigned char *)image+JC_RVA_CloneReference);
    void (*assign)(uintptr_t,void *)=(void *)((unsigned char *)image+JC_RVA_PropertyAssign);
    void (*release)(void *)=(void *)((unsigned char *)image+JC_RVA_ReferenceRelease);
    void (*bind)(uintptr_t,uintptr_t,uintptr_t)=(void *)((unsigned char *)image+JC_RVA_GeometryBind);
    uint64_t reference[2]={0,0};
    InterlockedExchange(&JNETMaterial,-1);
    struct jc_geometry_pool geometry;
    int geometry_code=jc_geometry_pool_read(own_read,NULL,resource_barn+0x146f0+0x160+0x80,&geometry);
    InterlockedExchange(&JNETGeometry[0],geometry_code);
    if (geometry_code) {
        InterlockedExchange(&mesh_fault,1);
        boundary_checkpoint("[JC] original remote geometry-pool snapshot declined; preparation not attempted");
        return;
    }
    if (geometry.slots!=JC_BODY_CAPACITY || geometry.occupied!=2+body_context->index || geometry.free_slots!=JC_BODY_COUNT-body_context->index || geometry.source!=geometry_source_before) {
        InterlockedExchange(&mesh_fault,1);
        boundary_checkpoint("[JC] free geometry slot changed; binding declined");
        return;
    }
    InterlockedExchange(&JNETGeometry[1],(LONG)geometry.slots);
    InterlockedExchange(&JNETGeometry[2],(LONG)geometry.occupied);
    InterlockedExchange(&JNETGeometry[3],(LONG)geometry.free_slots);
    char geometry_line[160];
    snprintf(geometry_line,sizeof(geometry_line),"[JC] geometry pool before binding slots=%u occupied=%u free=%u",
        (unsigned)geometry.slots,(unsigned)geometry.occupied,(unsigned)geometry.free_slots);
    if (!material_mark(36,geometry_line)) return;
    if (!material_mark(30,"[JC] material property clear enter")) return;
    clear(mesh+0x80);
    if (!material_mark(30,"[JC] material property clear returned")) return;
    if (!material_mark(31,"[JC] material clone reference enter")) return;
    uintptr_t returned=get(mesh,reference);
    if (returned!=(uintptr_t)reference) {
        InterlockedExchange(&mesh_fault,1);
        boundary_checkpoint("[JC] material reference result unexpected; temporary reference quarantined");
        return;
    }
    int proceed=material_mark(31,"[JC] material clone reference returned");
    if (proceed) proceed=material_mark(32,"[JC] material property assign enter");
    if (proceed) {
        assign(mesh+0x80,reference);
        proceed=material_mark(32,"[JC] material property assign returned");
    }
    /* The getter's known output owns one temporary reference. Release even if logging failed. */
    if (!material_mark(33,"[JC] material temporary release enter")) proceed=0;
    release(reference);
    if (!material_mark(33,"[JC] material temporary release returned")) proceed=0;
    if (!proceed) return;
    if (!material_mark(34,"[JC] material geometry binding enter")) return;
    bind(material_manager,material_name,mesh+0x88);
    if (!material_mark(34,"[JC] material geometry binding returned")) return;
    uint64_t bound_source, bound_slot;
    int source_state=3,slot_state=-1;
    if (jc_layout_field(own_read,NULL,mesh+0x80,0x10,&bound_source,8))
        source_state=!bound_source ? 0 : bound_source==geometry.source ? 1 : 2;
    if (jc_layout_field(own_read,NULL,mesh+0x80,0x30,&bound_slot,8)) slot_state=bound_slot ? 1 : 0;
    InterlockedExchange(&JNETGeometry[4],source_state);
    InterlockedExchange(&JNETGeometry[5],slot_state);
    snprintf(geometry_line,sizeof(geometry_line),"[JC] trial geometry source comparison=%d slot_present=%d; no render commit",source_state,slot_state);
    if (!material_mark(36,geometry_line)) return;
    struct jc_geometry_pool trial_pool;
    uint64_t remote_slot=0,local_slot=0;
    int owned=source_state==1 && slot_state==1 &&
        !jc_geometry_pool_read(own_read,NULL,mesh+0x80,&trial_pool) &&
        trial_pool.source==geometry_source_before && trial_pool.slots==JC_BODY_CAPACITY && trial_pool.occupied==3+body_context->index && trial_pool.free_slots==JC_BODY_COUNT-1-body_context->index &&
        jc_layout_field(own_read,NULL,resource_barn+0x146f0+0x160+0x80,0x30,&remote_slot,8) &&
        jc_layout_field(own_read,NULL,resource_barn+0xb0+0x160+0x80,0x30,&local_slot,8) &&
        remote_slot==geometry_slot_before && bound_slot!=remote_slot && bound_slot!=local_slot;
    InterlockedExchange(&JNETGeometry[6],owned ? 1 : 0);
    if (!owned) InterlockedExchange(&mesh_fault,1);
    boundary_record(owned ? "[JC] trial owns a distinct geometry slot in four-slot pool" : "[JC] trial geometry ownership not established; cleanup required");
    uint64_t slot, resource, payload, end;
    int present=jc_layout_field(own_read,NULL,mesh+0x80,0x30,&slot,8) &&
        jc_layout_field(own_read,NULL,slot,0x10,&resource,8) &&
        jc_layout_field(own_read,NULL,resource,0x18,&payload,8) &&
        jc_layout_address(payload,0x98,&end);
    InterlockedExchange(&JNETMaterial,present ? 1 : 2);
    if (!owned || !present || mesh_fault) return;
    uint64_t ready_payload=0,cached=0;
    int ready=jc_material_ready(own_read,NULL,mesh+0x80,&ready_payload);
    InterlockedExchange(&JNETCommit[0],ready);
    if (ready) { InterlockedExchange(&mesh_fault,1); boundary_record("[JC] material commit input declined; cleanup required"); return; }
    if (!material_mark(37,"[JC] material commit enter; one call before cleanup")) return;
    InterlockedExchange(&JNETCommit[1],1);
    void (*commit)(uintptr_t)=(void *)((unsigned char *)image+JC_RVA_MaterialCommit);
    commit(mesh+0x80);
    InterlockedExchange(&JNETCommit[1],2);
    if (!material_mark(37,"[JC] material commit returned")) return;
    if (jc_layout_field(own_read,NULL,mesh+0x80,0x468,&cached,8) && cached==ready_payload)
        InterlockedExchange(&JNETCommit[1],3);
    else InterlockedExchange(&mesh_fault,1);
    callback_meshes[2]=mesh;
    int pose_models=-1,pose_roots=-1;
    int pose_code=jc_three_pose_storage(own_read,NULL,(uintptr_t)image,callback_meshes,&pose_models,&pose_roots);
    InterlockedExchange(&JNETPose[1],pose_models);
    InterlockedExchange(&JNETPose[2],pose_roots);
    InterlockedExchange(&JNETPose[0],pose_code);
    boundary_record("[JC] pose storage observation complete; no animation update or draw invoked");
    struct jc_pose_write_workspace *pose_work=HeapAlloc(GetProcessHeap(),0,sizeof(*pose_work));
    struct callback_read_budget pose_budget={.deadline=GetTickCount64()+250};
    struct jc_pose_write_result pose_writes;
    int write_code=jc_three_pose_writes(callback_read,&pose_budget,(uintptr_t)image,callback_meshes,&pose_writes,pose_work);
    if (pose_work) HeapFree(GetProcessHeap(),0,pose_work);
    if (pose_budget.expired) write_code=9;
    if (!write_code) {
        for (unsigned i=0;i<3;++i) for (unsigned j=0;j<2;++j)
            InterlockedExchange(&JNETPoseWrites[2+i*2+j],(LONG)pose_writes.counts[i][j]);
        InterlockedExchange(&JNETPoseWrites[1],pose_writes.disjoint);
    }
    InterlockedExchange(&JNETPoseWrites[0],write_code);
    boundary_record("[JC] nested matrix destination inspection returned; cleanup follows; no animation calls");
    struct jc_animation_workspace *animation_work=HeapAlloc(GetProcessHeap(),0,sizeof(*animation_work));
    struct callback_read_budget animation_budget={.deadline=GetTickCount64()+250};
    struct jc_animation_result animation;
    int animation_code=jc_three_animation_storage(callback_read,&animation_budget,(uintptr_t)image,callback_meshes,&animation,animation_work);
    if (animation_work) HeapFree(GetProcessHeap(),0,animation_work);
    if (animation_budget.expired) animation_code=9;
    if (!animation_code) {
        for (unsigned i=0;i<3;++i) for (unsigned j=0;j<5;++j)
            InterlockedExchange(&JNETAnimation[2+i*5+j],(LONG)animation.counts[i][j]);
        InterlockedExchange(&JNETAnimation[1],animation.disjoint);
    }
    InterlockedExchange(&JNETAnimation[0],animation_code);
    boundary_record("[JC] animation storage inspection returned; no animation functions invoked");
    struct jc_callback_snapshot callbacks;
    int code=callback_snapshot_read(&callbacks);
    InterlockedExchange(&JNETCallbacks[3],code);
    InterlockedExchange(&JNETCallbacks[9],(LONG)callbacks.visited);
    if (!code) {
        InterlockedExchange(&JNETCallbacks[4],(LONG)callbacks.owners[2].total);
        int matched=callbacks.owners[2].total && jc_callback_equal(&callbacks.owners[1],&callbacks.owners[2]);
        InterlockedExchange(&JNETCallbacks[5],matched ? 1 : 0);
        if (!jc_callback_equal(&callbacks.owners[0],&callback_baseline.owners[0]) ||
            !jc_callback_equal(&callbacks.owners[1],&callback_baseline.owners[1])) InterlockedExchange(&mesh_fault,1);
    } else InterlockedExchange(&mesh_fault,1);
    if (!code) body_context->prepared_callbacks=callbacks;
    boundary_record("[JC] bounded callback registration snapshot complete; no callbacks invoked by probe");
    if (!mesh_fault) {
        int pair_code=body_pair_storage(image,mesh);
        InterlockedExchange(&JNETDual[2],pair_code);
        if (pair_code) InterlockedExchange(&mesh_fault,1);
    }
    if (!mesh_fault && JNETPose[0]==0 && JNETPose[1]==1 && JNETPose[2]==1 &&
        JNETPoseWrites[0]==0 && JNETPoseWrites[1]==1 && JNETAnimation[0]==0 && JNETAnimation[1]==1 &&
        JNETCommit[1]==3 && JNETGeometry[6]==1 && code==0) idle_pose_trial(image,mesh);
    else { InterlockedExchange(&JNETIdle[0],13); InterlockedExchange(&mesh_fault,1); }
}
static void mesh_initialize(void *image,uint64_t object) {
    callback_meshes[2]=object+0x160;
    if (mesh_fault) return;
    void (*attach)(uintptr_t,uintptr_t,uintptr_t)=(void *)((unsigned char *)image+JC_RVA_ResourceAttach);
    if (!boundary_checkpoint("[JC] mesh attachment enter")) { InterlockedExchange(&mesh_fault,1); return; }
    InterlockedExchangePointer(&guarded_mesh,(void *)(uintptr_t)(object+0x160));
    InterlockedExchange(&JNETPhase,22);
    attach(mesh_source,mesh_context,(uintptr_t)object+0x160);
    InterlockedExchangePointer(&guarded_mesh,NULL);
    if (!boundary_checkpoint("[JC] mesh attachment returned")) InterlockedExchange(&mesh_fault,1);
    if (!jc_mesh_guard_complete(JNETMesh[1],JNETMesh[2])) InterlockedExchange(&mesh_fault,1);
    if (mesh_fault) return;
    material_prepare(image,(uintptr_t)object+0x160);
}
static void mesh_cleanup(void *image,uint64_t object) {
    void (*cleanup)(uintptr_t)=(void *)((unsigned char *)image+JC_RVA_MeshCleanup);
    if (!boundary_checkpoint("[JC] mesh cleanup enter")) InterlockedExchange(&mesh_fault,1);
    InterlockedExchange(&JNETPhase,24);
    if (initialized_life.state==3) InterlockedExchange(&JNETMeshLife[5],4);
    cleanup((uintptr_t)object+0x160);
    if (initialized_life.state==3) InterlockedExchange(&JNETMeshLife[5],5);
    uint64_t cached=1;
    int cleared=jc_layout_field(own_read,NULL,object+0x160+0x80,0x468,&cached,8) && !cached;
    InterlockedExchange(&JNETCommit[2],cleared ? 1 : -1);
    if (!cleared) InterlockedExchange(&mesh_fault,1);
    if (!boundary_checkpoint("[JC] mesh cleanup returned")) InterlockedExchange(&mesh_fault,1);
}
static void mesh_destroy(void *image,uint64_t object) {
    if (!boundary_checkpoint("[JC] mesh deleting destructor enter")) InterlockedExchange(&mesh_fault,1);
    if (initialized_life.state==3) InterlockedExchange(&JNETMeshLife[5],6);
    trial_destroy(image,object);
    if (initialized_life.state==3) InterlockedExchange(&JNETMeshLife[5],7);
    if (!boundary_checkpoint("[JC] mesh deleting destructor returned")) InterlockedExchange(&mesh_fault,1);
}
static void initialized_publish(const struct jc_initialized_detail *detail) {
    InterlockedExchange(&JNETMeshLife[4],detail->field);
    InterlockedExchange(&JNETMeshLife[6],(LONG)detail->slots);
    InterlockedExchange(&JNETMeshLife[7],(LONG)detail->occupied);
    InterlockedExchange(&JNETMeshLife[8],(LONG)detail->free_slots);
    InterlockedExchange(&JNETMeshLife[9],detail->pool_valid);
    InterlockedExchange(&JNETMeshLife[19],detail->remaining_valid);
    InterlockedExchange(&JNETMeshLife[20],detail->remaining_code);
    InterlockedExchange(&JNETMeshLife[21],detail->remaining_field);
    InterlockedExchange(&JNETMeshLife[10],detail->comparison_valid);
    InterlockedExchange(&JNETMeshLife[11],detail->same_pool);
    InterlockedExchange(&JNETMeshLife[12],detail->changed_mask);
    for (unsigned i=0;i<3;++i) {
        InterlockedExchange(&JNETMeshLife[13+i],detail->before[i]);
        InterlockedExchange(&JNETMeshLife[16+i],detail->after[i]);
    }
}
static void initialized_fault_field(int field,int phase) {
    const struct jc_initialized_detail detail={.field=field};
    initialized_publish(&detail);
    InterlockedExchange(&JNETMeshLife[5],phase);
}
static void initialized_quarantine(int code) {
    if (mirror_mode==1) {
        mirror_elapsed(); mirror_mode=2;
        InterlockedExchange(&JNETSkeleton[0],-2);
    }
    InterlockedExchange(&JNETDual[0],-1);
    initialized_life.object=0; initialized_life.state=-1;
    InterlockedExchange(&JNETMeshLife[2],code);
    InterlockedExchange(&JNETMeshLife[0],-1);
    InterlockedExchange(&JNETResource,-1);
}
static int mesh_retain(void *image,uint64_t object) {
    if (mesh_fault || lifetime_conflict || initialized_life.state ||
        JNETIdle[0]!=0 || JNETIdle[1]!=3 || JNETCommit[1]!=3 ||
        JNETGeometry[6]!=1 || JNETCallbacks[3]!=0) return 0;
    struct jc_initialized_lifetime candidate={.object=object,.game=resource_game,
        .barn=resource_barn,.thread=resource_thread,.state=1};
    struct callback_read_budget budget={.deadline=GetTickCount64()+100};
    InterlockedExchange(&JNETMeshLife[5],1);
    int code=jc_initialized_owner_read_group(callback_read,&budget,(uintptr_t)image,object,&candidate.owner,&candidate.detail,JC_BODY_CAPACITY,3+body_context->index);
    if (!code) code=jc_initialized_validate_detail(&candidate,callback_read,&budget,(uintptr_t)image,
                                           (uintptr_t)JNETGame,GetCurrentThreadId(),&candidate.detail);
    if (!code) for (unsigned i=0;i<JC_BODY_CAPACITY;++i) {
        candidate.head_kinds[i]=jc_initialized_head_kind(callback_read,&budget,(uintptr_t)image,&candidate.owner,i);
        if (i<3) InterlockedExchange(&JNETMeshLife[22+i],candidate.head_kinds[i]);
    }
    if (budget.expired) code=jc_initialized_error(&candidate.detail,JC_IF_BUDGET,9);
    initialized_publish(&candidate.detail);
    if (code || !boundary_checkpoint("[JC] initialized undrawn mesh retained until next local cleanup")) {
        InterlockedExchange(&mesh_fault,1);
        if (!code) initialized_fault_field(JC_IF_CHECKPOINT,1);
        InterlockedExchange(&JNETMeshLife[2],code ? code : 10); return 0;
    }
    initialized_life=candidate; initialized_frame=JNETFrames;
    InterlockedExchange(&JNETMeshLife[0],1);
    return 1;
}
static void mesh_restoration(uintptr_t dude,int mesh_result) {
    struct jc_geometry_pool released_pool;
    uint64_t released_slot=0;
    int restored=!jc_geometry_pool_read(own_read,NULL,dude+0x160+0x80,&released_pool) &&
        released_pool.source==geometry_source_before && released_pool.slots==JC_BODY_CAPACITY && released_pool.occupied==2+body_context->index && released_pool.free_slots==JC_BODY_COUNT-body_context->index &&
        jc_layout_field(own_read,NULL,dude+0x160+0x80,0x30,&released_slot,8) && released_slot==geometry_slot_before;
    if (initialized_life.state==2) {
        restored=initialized_life.pool_released && jc_initialized_pool_released(&initialized_life,own_read,NULL);
        if (restored) released_pool.free_slots=initialized_life.released_free;
    }
    InterlockedExchange(&JNETGeometry[7],restored ? 1 : 0);
    if (restored) InterlockedExchange(&JNETGeometry[8],(LONG)released_pool.free_slots);
    boundary_record(restored ? "[JC] geometry slot returned; original remote ownership preserved" : "[JC] geometry pool restoration not established; restart required");
    struct jc_callback_snapshot callback_after;
    int callback_after_code=callback_snapshot_read(&callback_after);
    InterlockedExchange(&JNETCallbacks[6],callback_after_code);
    InterlockedExchange(&JNETCallbacks[10],(LONG)callback_after.visited);
    int callbacks_restored=!callback_after_code && !callback_after.owners[2].total &&
        jc_callback_equal(&callback_after.owners[0],&callback_baseline.owners[0]) &&
        jc_callback_equal(&callback_after.owners[1],&callback_baseline.owners[1]);
    InterlockedExchange(&JNETCallbacks[7],callbacks_restored ? 1 : 0);
    boundary_record(callbacks_restored ? "[JC] trial callback registrations absent after cleanup; originals match" : "[JC] callback registration restoration not established");
    int good=JNETIdle[0]==0 && JNETIdle[1]==3 && callbacks_restored && JNETCallbacks[3]==0 && JNETCommit[1]==3 && JNETCommit[2]==1 && restored && JNETGeometry[6]==1 && !mesh_fault && !lifetime_conflict && JNETMaterial==1 && mesh_result==2 && jc_mesh_guard_complete(JNETMesh[1],JNETMesh[2]);
    InterlockedExchange(&JNETResource,good ? 4 : -1);
    boundary_record(good ? "[JC] material commit returned; trial resources inspected and released" :
        "[JC] guarded mesh rehearsal incomplete or declined; restart required");
}
static uintptr_t resource_observer(uintptr_t dude, uintptr_t manager, uintptr_t effects,
                                   uintptr_t unused, uintptr_t lua, uintptr_t name) {
    int observing=JNETResource==1 || JNETResource==2;
    if (observing) boundary_record("[JC] boundary resource original enter");
    uintptr_t result=original_resource_init(dude,manager,effects,unused,lua,name);
    if (observing && JNETResource>=0) boundary_record("[JC] boundary resource original returned");
    if (JNETResource!=2) return result;
    if (!TryAcquireSRWLockExclusive(&lifetime_lock)) {
        InterlockedExchange(&lifetime_conflict,1);
        InterlockedExchange(&JNETResource,-1);
        return result;
    }
    if (JNETResource!=2 || dude!=resource_barn+0x146f0) goto finished;
    uint64_t barn, value, head, local_head, payload, match;
    uintptr_t expected_image=(uintptr_t)GetModuleHandleW(NULL);
    unsigned char connected;
    char mesh_name[sizeof("MeshDudeRemote")];
    if (lifetime_conflict || GetCurrentThreadId()!=resource_thread ||
        (uintptr_t)JNETGame!=resource_game ||
        !own_read(NULL,resource_game+0x1e0,&barn,8) || barn!=resource_barn ||
        !own_read(NULL,barn,&value,8) || value!=expected_image+0x5da340 ||
        !own_read(NULL,barn+0xb0,&value,8) || value!=expected_image+0x5d0380 ||
        !own_read(NULL,dude,&value,8) || value!=expected_image+0x5d0380 ||
        !own_read(NULL,resource_game+0x28,&value,8) || value!=manager ||
        !own_read(NULL,resource_game+0xb0,&value,8) || value!=effects ||
        !own_read(NULL,resource_game+0x20,&value,8) || value!=lua ||
        !own_read(NULL,resource_game+0x180,&match,8) ||
        !jc_layout_field(own_read,NULL,match,0x11db4,&connected,1) || connected ||
        !own_read(NULL,name,mesh_name,sizeof(mesh_name)) || memcmp(mesh_name,"MeshDudeRemote",sizeof(mesh_name)) ||
        !own_read(NULL,dude+0x168,&head,8) || head<0x28 ||
        !own_read(NULL,barn+0xb0+0x168,&local_head,8) || local_head!=head ||
        !jc_layout_field(own_read,NULL,head,0x10,&payload,8) || !payload ||
        !jc_layout_field(own_read,NULL,manager,8,&value,8) || !value) {
        InterlockedExchange(&JNETResource,-1);
        boundary_record("[JC] boundary remote context declined; no trial object allocated");
        goto finished;
    }
    if (JNETStartup[0]) {
        int level=-1; int code=jc_render_level(own_read,NULL,resource_game,&level);
        InterlockedExchange(&JNETMeshLife[94],level);
        InterlockedExchange(&JNETMeshLife[97],code);
        int reason=code ? 1 : GetTickCount64()>=preparation_deadline ? 2 :
            (level!=0 && !jc_render_trial_level(level)) ? 3 : 0;
        if (reason) {
            InterlockedExchange(&JNETMeshLife[107],reason);
            InterlockedExchange(&JNETResource,-1);
            boundary_checkpoint("[JC] preparation declined: unknown/non-target level or timeout; no trial allocated");
            goto finished;
        }
        if (level==0) {
            LONG skipped=InterlockedIncrement(&JNETMeshLife[95]);
            if (skipped>2) InterlockedExchange(&JNETMeshLife[107],4);
            InterlockedExchange(&JNETResource,skipped<=2 ? 1 : -1);
            boundary_checkpoint("[JC] chapter-select initialization skipped; enter an eligible ordinary chapter");
            goto finished;
        }
        prepared_level=level;
        InterlockedExchange(&JNETMeshLife[108],level);
        if (!boundary_checkpoint("[JC] ordinary chapter selected; validating native trial resources")) {
            InterlockedExchange(&JNETResource,-1); goto finished;
        }
    }
    uintptr_t build_target=(uintptr_t)InterlockedCompareExchangePointer(&builder_target,NULL,NULL);
    if (build_target) {
        struct jc_geometry_pool geometry;
        uint64_t current;
        int code=jc_geometry_pool_read(own_read,NULL,dude+0x160+0x80,&geometry);
        if (!code && !jc_layout_field(own_read,NULL,geometry.source,0x10,&current,8)) code=JC_LAYOUT_READ;
        if (!code) InterlockedExchange(&JNETBuilder[7],current==build_target ? 1 : 2);
        else InterlockedExchange(&JNETBuilder[6],code);
        InterlockedExchangePointer(&builder_target,NULL);
        InterlockedExchange(&JNETResource,code || JNETBuilder[6] ? -1 : 4);
        boundary_record("[JC] geometry build observation ended at remote initialization; no trial allocated");
        goto finished;
    }
    const uint64_t meshes[2]={barn+0xb0+0x160,dude+0x160};
    struct jc_pool_result pool;
    int pool_status=jc_clone_pool(own_read,NULL,payload,meshes,&pool);
    uint64_t after;
    if (!pool_status && (!jc_layout_field(own_read,NULL,head,0x10,&after,8) || after!=payload ||
        !own_read(NULL,dude+0x168,&after,8) || after!=head ||
        !own_read(NULL,barn+0xb0+0x168,&after,8) || after!=head)) pool_status=JC_LAYOUT_CHANGED;
    if (!pool_status) {
        InterlockedExchange(&JNETPool[1],(LONG)pool.slots);
        InterlockedExchange(&JNETPool[2],(LONG)pool.capacity);
        InterlockedExchange(&JNETPool[3],(LONG)pool.occupied);
        InterlockedExchange(&JNETPool[4],(LONG)pool.free_slots);
    }
    InterlockedExchange(&JNETPool[0],pool_status);
    char pool_line[160];
    if (!pool_status) snprintf(pool_line,sizeof(pool_line),
        "[JC] pool slots=%u capacity=%u occupied=%u free=%u; no trial object allocated",
        (unsigned)pool.slots,(unsigned)pool.capacity,(unsigned)pool.occupied,(unsigned)pool.free_slots);
    else snprintf(pool_line,sizeof(pool_line),"[JC] pool snapshot declined code=%d; no trial object allocated",pool_status);
    boundary_record(pool_line);
    if (JNETResource<0) goto finished;
    if (pool_status || pool.slots!=2 || pool.capacity!=2 || pool.occupied!=2 || pool.free_slots) {
        InterlockedExchange(&JNETResource,-1);
        boundary_checkpoint("[JC] guarded mesh declined: requires matched exhausted two-slot pool");
        goto finished;
    }
    struct jc_geometry_pool before_geometry;
    struct jc_geometry_build_state before_build;
    uint64_t geometry_object=0;
    if (JNETStartup[1]!=1 || JNETStartup[2]!=1 || JNETStartup[3]!=0 || JNETStartup[4]!=1 ||
        jc_geometry_pool_read(own_read,NULL,dude+0x160+0x80,&before_geometry) ||
        before_geometry.slots!=JC_BODY_CAPACITY || before_geometry.occupied!=2 || before_geometry.free_slots!=JC_BODY_COUNT ||
        !jc_layout_field(own_read,NULL,before_geometry.source,0x10,&geometry_object,8) ||
        jc_geometry_build_read(own_read,NULL,expected_image,geometry_object,&before_build) || before_build.limit!=JC_BODY_CAPACITY ||
        !jc_layout_field(own_read,NULL,dude+0x160+0x80,0x30,&geometry_slot_before,8)) {
        InterlockedExchange(&JNETResource,-1);
        boundary_checkpoint("[JC] slot trial declined: validated startup and two free four-slot geometry slots required");
        goto finished;
    }
    uint64_t saved_slot=geometry_slot_before;
    InterlockedExchange(&JNETDual[0],0);
    for (unsigned index=0;index<JC_BODY_COUNT;++index) {
        body_context=&body_contexts[index];
        InterlockedExchange(&JNETDual[1],(LONG)index+1);
        callback_meshes[0]=barn+0x210; callback_meshes[1]=dude+0x160; callback_meshes[2]=0;
        uint64_t local_manager=0;
        int callback_code=JC_LAYOUT_READ;
        if (own_read(NULL,callback_meshes[1]+0x528,&callback_manager,8) &&
            own_read(NULL,callback_meshes[0]+0x528,&local_manager,8) && callback_manager==local_manager)
            callback_code=callback_snapshot_read(&callback_baseline);
        InterlockedExchange(&JNETCallbacks[0],callback_code);
        InterlockedExchange(&JNETCallbacks[8],(LONG)callback_baseline.visited);
        InterlockedExchange(&JNETDual[5],callback_code);
        if (callback_code) goto pair_failed;
        InterlockedExchange(&JNETCallbacks[1],(LONG)callback_baseline.owners[0].total);
        InterlockedExchange(&JNETCallbacks[2],(LONG)callback_baseline.owners[1].total);
        geometry_source_before=before_geometry.source; geometry_slot_before=saved_slot;
        mesh_context=value; mesh_source=head-0x28; material_manager=manager; material_name=name;
        InterlockedExchange(&JNETResource,3);
        InterlockedExchange(&JNETTrial,1);
        const struct jc_mesh_ops ops={{own_read,mesh_create,mesh_destroy,(void *)expected_image},mesh_initialize,mesh_cleanup};
        struct jc_mesh_result snapshot;
        int mesh_result=jc_mesh_rehearse_or_retain(&ops,expected_image,barn,&snapshot,mesh_retain);
        for (unsigned i=0;i<3;++i) InterlockedExchange(&JNETMesh[3+i],(LONG)snapshot.counts[i]);
        InterlockedExchange(&JNETMesh[0],mesh_result);
        body_publish();
        if (mesh_result!=5) {
            InterlockedExchange(&JNETDual[3],mesh_result);
            mesh_restoration(dude,mesh_result);
            goto pair_failed;
        }
    }
    {
        struct jc_initialized_lifetime pair[2]={body_contexts[0].s_initialized_life,body_contexts[1].s_initialized_life};
        struct jc_initialized_owner owners[2];
        struct jc_initialized_detail details[2];
        int code=0;
        for (unsigned i=0;i<JC_BODY_COUNT && !code;++i) {
            struct callback_read_budget budget={.deadline=GetTickCount64()+100};
            code=jc_initialized_owner_read_group(callback_read,&budget,expected_image,pair[i].object,&owners[i],&details[i],4,4);
            if (budget.expired) code=9;
        }
        if (!code) code=jc_dual_owner_pair(pair,owners);
        InterlockedExchange(&JNETDual[4],code);
        if (code) goto pair_failed;
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i];
            initialized_life.owner=owners[i]; initialized_life.detail=details[i];
            initialized_publish(&details[i]); body_publish();
        }
        InterlockedExchange(&JNETDual[0],1);
        InterlockedExchange(&JNETDual[1],3);
        InterlockedExchange(&JNETTrial,4);
        InterlockedExchange(&JNETResource,5);
    }
    goto finished;
pair_failed:
    InterlockedExchange(&JNETDual[0],-1);
    InterlockedExchange(&JNETResource,-1);
    body_publish();

finished:
    body_context=&body_contexts[0];
    ReleaseSRWLockExclusive(&lifetime_lock);
    return result;
}
#include "traveler_glyph_init.h"
#include "traveler_trial_runtime.h"
static int cloth_stage_unavailable(void *ctx,uint64_t generation,const struct jc_cloth_frame_draw *draws,
        unsigned count,struct jc_cloth_live_commands *commands) {
    (void)ctx; (void)generation; (void)draws; (void)count; (void)commands; return 0;
}
static void cloth_stage_discard_empty(void *ctx,struct jc_cloth_live_commands *commands) {
    (void)ctx; (void)jc_cloth_render_discard(commands);
}
static int cloth_native_retired(void *context,uint64_t ticket) {
    int handled=0;
    int result=jc_cloth_render_retired(context,ticket,&handled);
    return handled ? result : jc_cloth_live_retired(context,ticket);
}
__declspec(dllexport) int JNETClothInitialize(struct jc_cloth_live *backend,
        ID3D11DeviceContext *context,const struct jc_cloth_live_ops *services) {
    if (!backend || !services || active_cloth_backend || cloth_chapter.pending || cloth_chapter.failed ||
        !original_render_handoff || !original_chapter_switch || !original_game_window ||
        cloth_exit_window || InterlockedCompareExchange(&cloth_progress_pending,0,0) || JNETTraveler[0]!=3 ||
        GetCurrentThreadId()!=traveler_thread || (uintptr_t)JNETGame!=traveler_game)
        return JC_FRAME_ARGUMENT;
    struct jc_cloth_live_ops ops=*services;
    ops.prepare=cloth_stage_unavailable; ops.discard=cloth_stage_discard_empty;
    ops.handoff=original_render_handoff;
    uint64_t image=(uintptr_t)GetModuleHandleW(NULL);
    struct jc_cloth_graphics_ops graphics=jc_cloth_graphics_native_ops(image);
    struct jc_cloth_graphics_token token={0};
    if (jc_cloth_graphics_lock(own_read,NULL,image,&graphics,(uintptr_t)context,&token)) return JC_FRAME_OWNERSHIP;
    int code=jc_cloth_live_init(backend,context,&cloth_retirement,own_read,NULL,ops);
    if (!code) {
        backend->graphics_image=image; backend->graphics=graphics;
        backend->frames.backend.retired=cloth_native_retired; active_cloth_backend=backend;
    }
    if (jc_cloth_graphics_unlock(own_read,NULL,image,&graphics,&token)) {
        if (!code) backend->failed=1;
        code=JC_FRAME_QUARANTINED;
    }
    return code;
}
static struct jc_cloth_lease cloth_actor_scarf;
static uint64_t cloth_actor_scarf_peer;
static struct jc_scarf_growth cloth_actor_growth;
static int traveler_growth_active(void);
static int cloth_actor_growth_write(void *ctx,uint64_t address,const void *value,size_t bytes) {
    (void)ctx;return jc_local_windows_write(address,value,bytes);
}
static int cloth_actor_growth_stop(void) {
    return jc_scarf_growth_stop(own_read,cloth_actor_growth_write,NULL,&cloth_actor_growth);
}
static int cloth_actor_appearance_status(int code,unsigned stage,unsigned detail) {
    static int previous_code=-1;
    static unsigned previous_stage,previous_detail;
    if (code!=previous_code || stage!=previous_stage || detail!=previous_detail) {
        char line[192];
        snprintf(line,sizeof(line),"[JC] actor appearance code/stage/detail=%d/%u/%u frame=%ld",
            code,stage,detail,(long)JNETFrames);
        message(line);previous_code=code;previous_stage=stage;previous_detail=detail;
    }
    return code;
}
static int cloth_actor_descriptor_write(void *context,uint64_t address,const void *data,size_t bytes) {
    struct jc_cloth_lease *lease=context;
    unsigned char check[16];
    if (!bytes || bytes>sizeof(check) || !traveler_context()) return 0;
    for (unsigned i=0;i<2;++i) {
        uint64_t base=lease->descriptor[i];
        if (address<base+0xc62 || address>base+0xcc4 || bytes>base+0xcc4-address) continue;
        return jc_local_windows_write(address,data,bytes) && own_read(NULL,address,check,bytes) &&
            !memcmp(check,data,bytes);
    }
    return 0;
}
static int cloth_actor_numeric_appearance(uint64_t peer,uint64_t generation,uint64_t barn) {
    static const unsigned char entry[16]={64,83,72,131,236,96,15,41,116,36,80,15,40,194,15,41};
    uint64_t image=(uintptr_t)GetModuleHandleW(NULL),timer=0,collection=0;
    unsigned char actual[sizeof(entry)];
    uint32_t count=0,scarf[2],maximum;
    float hsv[3];
    if (!own_read(NULL,image+0x217a70,actual,sizeof(actual)) || memcmp(actual,entry,sizeof(entry)))
        return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,1,0);
    if (!own_read(NULL,traveler_object+0x9b40,&collection,8))
        return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,2,0);
    if (!collection) return cloth_actor_appearance_status(0,2,0);
    if (!own_read(NULL,collection+0x91b0,&count,4))
        return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,3,0);
    if (count<2) return cloth_actor_appearance_status(0,3,count);
    if (count!=2) return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,3,count);
    struct jc_cloth_lease ready={.image=image,.game=traveler_game,.barn=barn,.dude=traveler_object};
    int capture=jc_cloth_capture(own_read,NULL,&ready,0);
    if (capture) return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,4,ready.capture_detail);
    unsigned pending=0;
    for (unsigned i=0;i<2;++i) {
        uint32_t state=0,role=0;
        if (!own_read(NULL,barn+0x270118+(uint64_t)ready.slot[i]*0xe0,&state,4) ||
            !own_read(NULL,ready.descriptor[i]+0xc94,&role,4))
            return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,5,i);
        if (state==2) pending|=1u<<i;
        else if (state!=1 || role!=(i?4u:5u))
            return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,5,16+i);
    }
    if (pending) return cloth_actor_appearance_status(0,5,pending);
    if (cloth_actor_scarf.dude!=traveler_object || cloth_actor_scarf.generation!=generation ||
        cloth_actor_scarf_peer!=peer) {
        if (!own_read(NULL,traveler_object+0x68,scarf,sizeof(scarf)) ||
            !own_read(NULL,image+0x731fb0,&maximum,4) || !maximum || maximum>4096 ||
            scarf[0]>scarf[1] || scarf[1]>4096) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,6,0);
        cloth_actor_scarf=(struct jc_cloth_lease){.image=image,.game=traveler_game,.barn=barn,
            .players=traveler_barn,.dude=traveler_object,.generation=generation,
            .scarf_charge=0,.scarf_length=0};
        cloth_actor_scarf_peer=peer;
    }
    cloth_actor_scarf.collection=0;
    if (jc_cloth_capture(own_read,NULL,&cloth_actor_scarf,1) ||
        !own_read(NULL,traveler_game+0x68,&timer,8) ||
        !own_read(NULL,image+0x3c943dc,hsv,4) ||
        !own_read(NULL,image+0x3763b38,hsv+1,4) ||
        !own_read(NULL,image+0x3763b3c,hsv+2,4)) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,6,cloth_actor_scarf.capture_detail);
    for (unsigned i=0;i<3;++i) if (!isfinite(hsv[i]) || fabsf(hsv[i])>1000)
        return cloth_actor_appearance_status(JC_FRAME_MATERIAL,7,i);
    if(!own_read(NULL,traveler_object+0x68,scarf,sizeof(scarf)) ||
        !own_read(NULL,image+0x731fb0,&maximum,4) || !maximum || maximum>4096 ||
        scarf[0]>scarf[1] || scarf[1]>4096)return cloth_actor_appearance_status(JC_FRAME_MATERIAL,8,0);
    float growth=fminf(1.f,(float)scarf[1]/maximum)-cloth_actor_scarf.scarf_length;
    int step=jc_cloth_scarf_step(own_read,cloth_actor_descriptor_write,&cloth_actor_scarf,
            &cloth_actor_scarf,timer);
    if (step) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,8,(unsigned)step);
    if(traveler_growth_active()) {
        if(!jc_scarf_growth_prepare(own_read,NULL,image,traveler_game,traveler_object,&cloth_actor_growth))
            return cloth_actor_appearance_status(JC_FRAME_MATERIAL,10,2);
        if(!jc_scarf_growth_step(own_read,cloth_actor_growth_write,NULL,&cloth_actor_growth,growth)) {
            uint64_t handle=0;float vectors[8]={0};
            memcpy(&handle,cloth_actor_growth.native+0x18,8);
            int position=own_read(NULL,traveler_object+0x2290,vectors,16);
            int direction=own_read(NULL,traveler_object+0x14270,vectors+4,16);
            char line[512];
            snprintf(line,sizeof(line),"[JC] scarf growth failed delta=%g owner=%d handle=%llu read=%d/%d vectors=%g,%g,%g,%g/%g,%g,%g,%g",
                growth,jc_scarf_growth_check(own_read,NULL,&cloth_actor_growth),
                (unsigned long long)handle,position,direction,
                vectors[0],vectors[1],vectors[2],vectors[3],vectors[4],vectors[5],vectors[6],vectors[7]);
            message(line);
            return cloth_actor_appearance_status(JC_FRAME_MATERIAL,10,3);
        }
    } else if(!cloth_actor_growth_stop())return cloth_actor_appearance_status(JC_FRAME_MATERIAL,10,1);
    ((void (*)(uintptr_t,uintptr_t,float,float,float))(image+0x217a70))
        (0,traveler_object,hsv[0],hsv[1],hsv[2]);
    return cloth_actor_appearance_status(0,9,0);
}
static int cloth_actor_scope_begin(uint64_t peer,uint64_t generation) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!jc_cloth_live_owner(s) || s->frames.closing || !peer || !generation ||
        JNETTraveler[0]!=3 || !traveler_context()) return JC_FRAME_ARGUMENT;
    uint64_t barn=0,source[2],target[2]={0};
    struct jc_cloth_storage stock[2],owned[2]; struct jc_cloth_appearance_route route;
    if (!own_read(NULL,traveler_game+0x108,&barn,8) || !barn) return JC_FRAME_OWNERSHIP;
    for (unsigned i=0;i<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++i) {
        struct jc_cloth_live_material *m=s->materials[i];
        if (!m || m->peer!=peer || m->generation!=generation || m->role<4 || m->role>5 ||
            m->pins || !m->ready || m->quarantined || target[m->role-4]) continue;
        target[m->role-4]=(uintptr_t)m->subscriber+0x88;
    }
    for (unsigned i=0;i<2;++i) {
        source[i]=barn+0x666ee8+(4+i)*0x630;
        if (!target[i] || jc_cloth_storage_read(own_read,NULL,source[i],stock+i) ||
            jc_cloth_storage_read(own_read,NULL,target[i],owned+i)) return JC_FRAME_MATERIAL;
    }
    if (jc_cloth_appearance_route_make(source,target,stock,owned,&route)) return JC_FRAME_MATERIAL;
    AcquireSRWLockExclusive(&cloth_appearance_lock);
    if (cloth_appearance_thread) { ReleaseSRWLockExclusive(&cloth_appearance_lock); return JC_FRAME_BUSY; }
    cloth_appearance_scope=route;cloth_appearance_thread=GetCurrentThreadId();
    InterlockedExchange(&cloth_appearance_enabled,1);
    ReleaseSRWLockExclusive(&cloth_appearance_lock);return JC_FRAME_OK;
}
static int traveler_live_material_rebuild(struct jc_cloth_live_material *m);
__declspec(dllexport) int JNETClothApplyAnimatedAppearance(uint64_t peer,uint64_t generation,
        const uint32_t sender_appearance[4]) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!peer || !generation || !jc_cloth_live_owner(s) || s->frames.closing ||
        JNETTraveler[0]!=3 || !traveler_context()) return cloth_actor_appearance_status(JC_FRAME_ARGUMENT,21,0);
    unsigned char remote=0;uint32_t current[4];
    if (!sender_appearance || sender_appearance[0]>sender_appearance[1] ||
        sender_appearance[1]>4096 || sender_appearance[2]>6 || sender_appearance[3]>20 ||
        !own_read(NULL,traveler_object+0x81,&remote,1) || !remote ||
        !own_read(NULL,traveler_object+0x68,current,sizeof(current)) ||
        memcmp(current,sender_appearance,sizeof(current))) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,22,0);
    struct jc_cloth_live_material *selected[2]={0};
    for (unsigned i=0;i<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++i) {
        struct jc_cloth_live_material *m=s->materials[i];
        if (!m || m->peer!=peer || m->generation!=generation) continue;
        if (m->role<4 || m->role>5 || !m->ready || m->quarantined ||
            m->thread!=GetCurrentThreadId()) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,23,0);
        if (!m->pins && !selected[m->role-4]) selected[m->role-4]=m;
    }
    if (!selected[0] || !selected[1]) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,24,0);
    uint64_t barn=0,manager=0,source[2],target[2];
    const char *atlas_names[2][3];
    struct jc_cloth_storage stock[2],owned[2];
    struct jc_cloth_appearance_route route;
    if (!own_read(NULL,traveler_game+0x108,&barn,8) || !barn ||
        !own_read(NULL,traveler_game+0x28,&manager,8) || !manager ||
        !jc_cloth_atlas_names(sender_appearance[2],atlas_names)) return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,25,0);
    for (unsigned i=0;i<2;++i) {
        if(selected[i]->atlas_holders && selected[i]->atlas_outfit!=sender_appearance[2] &&
            traveler_live_material_rebuild(selected[i]))return cloth_actor_appearance_status(JC_FRAME_MATERIAL,26,1+i);
        source[i]=barn+0x666ee8+(4+i)*0x630;
        target[i]=(uintptr_t)selected[i]->subscriber+0x88;
        if (jc_cloth_storage_read(own_read,NULL,source[i],stock+i) ||
            jc_cloth_storage_read(own_read,NULL,target[i],owned+i) ||
            memcmp(owned+i,&selected[i]->storage,sizeof(*owned))) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,26,0);
    }
    if (jc_cloth_appearance_route_make(source,target,stock,owned,&route)) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,27,0);
    for (unsigned i=0;i<2;++i) {
        int atlas_code=jc_cloth_live_atlas_select(own_read,NULL,selected[i],manager,barn,
                sender_appearance[2],atlas_names[i]);
        if(atlas_code)return cloth_actor_appearance_status(JC_FRAME_MATERIAL,28,(i+4)*100+atlas_code);
    }
    if (cloth_actor_numeric_appearance(peer,generation,barn)) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,29,0);
    _Alignas(16) float center[4];
    if (!own_read(NULL,traveler_object+0x2260,center,sizeof(center))) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,30,0);
    struct jc_cloth_graphics_token graphics={0};
    if (!jc_cloth_live_graphics_enter(s,&graphics)) return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,31,0);
    struct jc_environment_probe *probe=jc_cloth_live_probe_prepare(s,(uintptr_t)GetModuleHandleW(NULL),
        barn,peer,generation);
    int probe_code=probe ? jc_environment_probe_update(own_read,NULL,traveler_game,center,probe) : 1;
    int graphics_released=jc_cloth_live_graphics_leave(s,&graphics);
    if (probe_code || !graphics_released) {
        if (probe && probe->quarantined) s->failed=1;
        return cloth_actor_appearance_status(JC_FRAME_MATERIAL,32,0);
    }
    AcquireSRWLockExclusive(&cloth_appearance_lock);
    if (cloth_appearance_thread) {
        ReleaseSRWLockExclusive(&cloth_appearance_lock);return cloth_actor_appearance_status(JC_FRAME_BUSY,33,0);
    }
    cloth_appearance_scope=route;cloth_appearance_thread=GetCurrentThreadId();
    InterlockedExchange(&cloth_appearance_enabled,1);
    ReleaseSRWLockExclusive(&cloth_appearance_lock);
    ((void (*)(uintptr_t,uintptr_t))((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_DudeClothAppearance))
        ((uintptr_t)traveler_object,(uintptr_t)barn);
    AcquireSRWLockExclusive(&cloth_appearance_lock);
    InterlockedExchange(&cloth_appearance_enabled,0);
    memset(&cloth_appearance_scope,0,sizeof(cloth_appearance_scope));cloth_appearance_thread=0;
    ReleaseSRWLockExclusive(&cloth_appearance_lock);
    /* Same texture names and role selection as 8a7a0, using private holders. */
    static const char *const texture_names[6]={"textureAtlasA","textureAtlasB","textureMask",
        "textureDetail","textureFilledA","textureFilledB"};
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<(i ? 6u : 4u);++j) {
        uint64_t texture;
        if (jc_cloth_live_atlas_check(own_read,NULL,selected[i],j,&texture) ||
            texture!=selected[i]->atlas_textures[j] ||
            jc_cloth_texture_check(own_read,NULL,target[i],texture_names[j],texture,0)) {
            selected[i]->quarantined=1;s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,34,0);
        }
        original_cloth_setter[2]((uintptr_t)target[i],(uintptr_t)texture_names[j],(uintptr_t)texture);
        if (jc_cloth_texture_bound(own_read,NULL,target[i],texture_names[j],texture)) {
            selected[i]->quarantined=1;s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,35,0);
        }
    }
    for (unsigned i=0;i<2;++i) {
        struct jc_cloth_storage after;
        if (jc_cloth_texture_check(own_read,NULL,target[i],"envOccCube",probe->pair[0],0)) {
            selected[i]->quarantined=1;s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,36,0);
        }
        original_cloth_setter[2]((uintptr_t)target[i],(uintptr_t)"envOccCube",(uintptr_t)probe->pair[0]);
        if (jc_cloth_texture_bound(own_read,NULL,target[i],"envOccCube",probe->pair[0])) {
            selected[i]->quarantined=1;s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,37,0);
        }
        if (jc_cloth_storage_read(own_read,NULL,target[i],&after) ||
            memcmp(&after,owned+i,sizeof(after))) {
            selected[i]->quarantined=1;s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,38,0);
        }
    }
    /* a9470 also binds the stock Remote probe to Dude+1e0 (a9836).
       Replace it before 117790 commits the body; the whole-frame fence owns
       this probe even when the cloth queues are culled. */
    uint64_t body_material=traveler_object+0x1e0;
    if (!traveler_actor_material_write_allowed(body_material,0) ||
        jc_cloth_texture_check(own_read,NULL,body_material,"envOccCube",probe->pair[0],0)) {
        s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,39,0);
    }
    original_cloth_setter[2]((uintptr_t)body_material,(uintptr_t)"envOccCube",(uintptr_t)probe->pair[0]);
    if (jc_cloth_texture_bound(own_read,NULL,body_material,"envOccCube",probe->pair[0])) {
        s->failed=1;return cloth_actor_appearance_status(JC_FRAME_MATERIAL,40,0);
    }
    struct jc_cloth_producer_participant participant={.game=traveler_game,.barn=barn,
        .dude=traveler_object,.peer=peer,.generation=generation};
    for (unsigned i=0;i<2;++i) {
        struct jc_cloth_live_material *m=selected[i];
        if (!m->storage.count) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,41,0);
        struct jc_cloth_material *d=participant.materials+i;
        *d=(struct jc_cloth_material){.peer=peer,.generation=generation,.material=target[i],
            .uniforms=m->storage.spans[0].begin,.uniform_bytes=m->storage.spans[0].bytes,
            .environment=probe->pair[0],.participant=1,.role=4+i,
            .scarf=current[0],.scarf_capacity=current[1],.outfit=current[2],.symbol=current[3]};
        memcpy(d->atlas,m->atlas_textures,sizeof(d->atlas));
    }
    cloth_participant=participant; cloth_participant_ready=1;
    return JC_FRAME_OK;
}
static int cloth_actor_scope_end(uint64_t peer,uint64_t generation,int success) {
    AcquireSRWLockExclusive(&cloth_appearance_lock);
    int owned=cloth_appearance_thread==GetCurrentThreadId();
    if (owned) {
        InterlockedExchange(&cloth_appearance_enabled,0);
        memset(&cloth_appearance_scope,0,sizeof(cloth_appearance_scope));cloth_appearance_thread=0;
    }
    ReleaseSRWLockExclusive(&cloth_appearance_lock);
    if (!owned || !success) return JC_FRAME_QUARANTINED;
    uint32_t appearance[4];
    if (!own_read(NULL,traveler_object+0x68,appearance,sizeof(appearance))) return JC_FRAME_OWNERSHIP;
    return JNETClothApplyAnimatedAppearance(peer,generation,appearance);
}
#include "traveler_glyph_trace.h"
#include "traveler_actor_runtime.h"
static int traveler_growth_active(void) {
    return traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE && !traveler_actor->departing;
}
#define JC_SAND_ABI
#include "traveler_sand_guard.h"
#define JC_GLYPH_ABI
#include "traveler_chirp_glyph.h"
__declspec(dllexport) int JNETClothPublish(unsigned index,
        const struct jc_cloth_command_source stock[7],const struct jc_cloth_command_source *extras,
        uintptr_t executor,uintptr_t context,uintptr_t fence) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !jc_cloth_live_owner(s) || index>=JC_CLOTH_FRAMES || cloth_chapter.failed)
        return JC_FRAME_ARGUMENT;
    int code=s->native_stages[index] ? JC_FRAME_OK :
        jc_cloth_live_stage_native(s,index,original_command_build,stock,extras,executor,context,fence);
    return code ? code : jc_cloth_frame_submit(&s->frames,index);
}
static int cloth_map_objects(uintptr_t barn,unsigned role,uintptr_t objects[2]);
static int cloth_stage_stock_batch(unsigned index,
        const struct jc_cloth_command_source stock[7],const struct jc_cloth_command_source *extras,
        uintptr_t begin,uintptr_t end,const unsigned first[7],const unsigned counts[7]) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !jc_cloth_live_owner(s) || index>=JC_CLOTH_FRAMES || cloth_chapter.failed)
        return JC_FRAME_ARGUMENT;
    if (cloth_maps.roles && (cloth_maps.index!=index ||
        cloth_maps.generation!=s->frames.frames[index].generation ||
        cloth_maps.seen[0]!=cloth_maps.roles || cloth_maps.seen[1]!=cloth_maps.roles))
        return JC_FRAME_BUSY;
    for (unsigned r=0;r<7;++r) if (cloth_maps.roles&(1u<<r)) {
        uintptr_t objects[2]={0},vertices=0,indices=0;
        if (!cloth_map_objects(cloth_maps.barn,r,objects) ||
            memcmp(objects,cloth_maps.objects[r],sizeof(objects)) ||
            jc_cloth_ring_finished(own_read,NULL,&cloth_maps.rings[r][0]) ||
            jc_cloth_ring_finished(own_read,NULL,&cloth_maps.rings[r][1]) ||
            !own_read(NULL,cloth_maps.barn+0x669970+r*8,&vertices,8) || vertices ||
            !own_read(NULL,cloth_maps.barn+0x6699a8+r*8,&indices,8) || indices) {
            s->failed=1; s->frames.frames[index].phase=JC_FRAME_UNKNOWN;
            message("[JC] native cloth finalizer did not retire its mapped rings; frame retained");
            return JC_FRAME_QUARANTINED;
        }
    }
    int code=s->native_stages[index] ? JC_FRAME_OK :
        jc_cloth_live_stage_native(s,index,original_command_build,stock,extras,0,0,0);
    if (!code) code=jc_cloth_live_bind_stock_batch(s,index,begin,end,stock,first,counts);
    if (!code) memset(&cloth_maps,0,sizeof(cloth_maps));
    return code;
}
__declspec(dllexport) int JNETClothStageStockBatch(unsigned index,
        const struct jc_cloth_command_source stock[7],const struct jc_cloth_command_source *extras,
        uintptr_t begin,uintptr_t end,const unsigned first[7],const unsigned counts[7]) {
    AcquireSRWLockExclusive(&cloth_map_lock);
    int code=cloth_stage_stock_batch(index,stock,extras,begin,end,first,counts);
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return code;
}
static int cloth_map_objects(uintptr_t barn,unsigned role,uintptr_t objects[2]) {
    uintptr_t slot=0,geometry=0;
    return own_read(NULL,barn+0x666e40+role*0x630,&slot,8) && slot &&
        own_read(NULL,slot+8,&geometry,8) && geometry &&
        own_read(NULL,geometry+0x28,objects,8) && objects[0] &&
        (objects[1]=geometry+0x30)!=0;
}
static int cloth_bind_native_maps(unsigned index,uint64_t generation,unsigned roles) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !jc_cloth_live_owner(s) || s->frames.closing || cloth_maps.roles ||
        index>=JC_CLOTH_FRAMES || !roles || (roles&~0x7fu) || !traveler_context())
        return JC_FRAME_ARGUMENT;
    struct jc_cloth_frame *f=s->frames.frames+index;
    if (f->phase!=JC_FRAME_WRITING || f->generation!=generation ||
        !f->pass_mode || f->pass_sealed || f->pass_committed) return JC_FRAME_BUSY;
    unsigned required=0;
    for (unsigned r=0;r<7;++r)
        if (f->streams[r].vertices || f->streams[r].indices) required|=1u<<r;
    if (roles!=required) return JC_FRAME_ARGUMENT;
    uintptr_t barn=0,again=0;
    if (!own_read(NULL,traveler_game+0x108,&barn,8) || !barn) return JC_FRAME_OWNERSHIP;
    uintptr_t objects[7][2]={{0}};
    for (unsigned r=0;r<7;++r) if (roles&(1u<<r)) {
        if (!cloth_map_objects(barn,r,objects[r])) return JC_FRAME_OWNERSHIP;
        const struct jc_cloth_stream *v=f->streams+r;
        struct jc_cloth_live_buffer *vb=jc_cloth_live_buffer_find(s,v->vertex);
        struct jc_cloth_live_buffer *ib=jc_cloth_live_buffer_find(s,v->index);
        if (!vb || !ib || vb->pins!=1 || ib->pins!=1 || !vb->mapped || !ib->mapped ||
            (uintptr_t)vb->mapped+v->vertex_offset!=(uintptr_t)v->mapped_vertices ||
            (uintptr_t)ib->mapped+v->index_offset!=(uintptr_t)v->mapped_indices)
            return JC_FRAME_OWNERSHIP;
    }
    if (!own_read(NULL,traveler_game+0x108,&again,8) || again!=barn) return JC_FRAME_OWNERSHIP;
    cloth_maps.barn=barn; cloth_maps.game=traveler_game;
    cloth_maps.index=index; cloth_maps.generation=generation;
    memcpy(cloth_maps.objects,objects,sizeof(objects)); cloth_maps.roles=roles;
    return JC_FRAME_OK;
}
__declspec(dllexport) int JNETClothBindNativeMaps(unsigned index,uint64_t generation,unsigned roles) {
    AcquireSRWLockExclusive(&cloth_map_lock);
    int code=cloth_bind_native_maps(index,generation,roles);
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return code;
}
static int cloth_prelaunch_write(void *context,uint64_t address,const void *value,size_t bytes) {
    (void)context;
    return jc_local_windows_write(address,value,bytes);
}
/* Called at the owned dependency boundary after capturing THIS frame's jobs.
   Unlike BindNativeMaps, native mapping has already completed for this pass. */
__declspec(dllexport) int JNETClothAdoptMappedPass(unsigned index,uint64_t generation,unsigned pass) {
    AcquireSRWLockExclusive(&cloth_map_lock);
    struct jc_cloth_live *s=active_cloth_backend;
    int code=JC_FRAME_ARGUMENT;
    if (!jc_cloth_live_owner(s) || index>=JC_CLOTH_FRAMES || pass>1 || !traveler_context()) goto done;
    struct jc_cloth_frame *f=s->frames.frames+index;
    if (f->phase!=JC_FRAME_WRITING || f->generation!=generation || !f->pass_mode ||
        f->pass_required!=(1u<<pass) || f->pass_sealed || f->pass_committed) goto done;
    code=cloth_bind_native_maps(index,generation,f->pass_roles[pass]);
    if (code) goto done;
    struct jc_cloth_prelaunch transaction;
    int capture=jc_cloth_prelaunch_capture(own_read,NULL,cloth_maps.barn,cloth_maps.roles,f->streams,&transaction);
    if (capture) { memset(&cloth_maps,0,sizeof(cloth_maps));code=JC_FRAME_OWNERSHIP;goto done; }
    int applied=jc_cloth_prelaunch_apply(own_read,cloth_prelaunch_write,NULL,&transaction);
    if (applied) {
        if (applied==JC_PRELAUNCH_QUARANTINED) {
            s->failed=1;f->phase=JC_FRAME_UNKNOWN;code=JC_FRAME_QUARANTINED;
            message("[JC] native prelaunch mapping restoration failed; retained private buffers");
        } else { memset(&cloth_maps,0,sizeof(cloth_maps));code=JC_FRAME_OWNERSHIP; }
        goto done;
    }
    memcpy(cloth_maps.rings,transaction.rings,sizeof(cloth_maps.rings));
    cloth_maps.seen[0]=cloth_maps.seen[1]=cloth_maps.roles;
    code=JC_FRAME_OK;
done:
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return code;
}
static uint32_t cloth_map_observer(uintptr_t object,uintptr_t output,unsigned kind) {
    uint32_t (*native)(uintptr_t,uintptr_t)=kind ? original_index_map : original_vertex_map;
    unsigned role=7;
    for (unsigned r=0;r<7;++r) if ((cloth_maps.roles&(1u<<r)) &&
        output==cloth_maps.barn+0x666e00+r*0x630+kind*0x10) { role=r; break; }
    if (role==7) return native(object,output);
    struct jc_cloth_live *s=active_cloth_backend;
    struct jc_cloth_frame *f=s ? s->frames.frames+cloth_maps.index : NULL;
    uintptr_t objects[2],barn=0;
    if (!jc_cloth_live_owner(s) || !f || f->phase!=JC_FRAME_WRITING ||
        f->generation!=cloth_maps.generation || (cloth_maps.seen[kind]&(1u<<role)) ||
        !(atomic_load(&f->route.active_roles)&(1u<<role)) ||
        !own_read(NULL,cloth_maps.game+0x108,&barn,8) || barn!=cloth_maps.barn ||
        !cloth_map_objects(barn,role,objects) ||
        memcmp(objects,cloth_maps.objects[role],sizeof(objects)) || object!=objects[kind]) goto failed;
    const struct jc_cloth_stream *v=f->streams+role;
    struct jc_cloth_live_buffer *b=jc_cloth_live_buffer_find(s,kind ? v->index : v->vertex);
    uintptr_t address=(uintptr_t)(kind ? v->mapped_indices : v->mapped_vertices);
    uint32_t offset=kind ? v->index_offset : v->vertex_offset;
    uint32_t bytes=kind ? v->indices*4u : v->vertices*60u;
    if (!b || b->pins!=1 || !b->mapped || offset>b->bytes || bytes>b->bytes-offset ||
        (uintptr_t)b->mapped+offset!=address) goto failed;
    /* Preserve the stock map/unmap pair; redirect only its returned storage. */
    if (native(object,output)) goto failed;
    if (jc_cloth_ring_capture(own_read,NULL,object,kind,&cloth_maps.rings[role][kind])) goto failed;
    unsigned char mapped[12],original_output[12]; uintptr_t stock_address=0; uint32_t stock_bytes=0;
    if (!own_read(NULL,output,mapped,sizeof(mapped))) goto failed;
    memcpy(original_output,mapped,sizeof(mapped));
    memcpy(&stock_address,mapped,8); memcpy(&stock_bytes,mapped+8,4);
    if (!stock_address || !stock_bytes || stock_bytes>bytes ||
        jc_cloth_overlap(stock_address,stock_bytes,address,bytes)) goto failed;
    memcpy(mapped,&address,8); memcpy(mapped+8,&bytes,4);
    unsigned char check[12];
    if (!jc_local_windows_write(output,mapped,sizeof(mapped)) || !own_read(NULL,output,check,sizeof(check)) ||
        memcmp(check,mapped,sizeof(check))) {
        if (!jc_local_windows_write(output,original_output,sizeof(original_output)) || !own_read(NULL,output,check,sizeof(check)) ||
            memcmp(check,original_output,sizeof(check)))
            message("[JC] cloth map output restoration failed; native caller does not check map status");
        goto failed;
    }
    cloth_maps.seen[kind]|=1u<<role;
    return 0;
failed:
    if (s) s->failed=1;
    if (f) f->phase=JC_FRAME_UNKNOWN;
    message("[JC] cloth mapping ownership changed; retained frame quarantined");
    return 5;
}
static uint32_t cloth_dependency_observer(uintptr_t dependency) {
    if (cloth_produce_before_dependency(dependency)<0) return 0;
    AcquireSRWLockExclusive(&cloth_map_lock);
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_maps.roles || dependency!=cloth_maps.barn+0x6bce00) {
        ReleaseSRWLockExclusive(&cloth_map_lock);
        return original_cloth_dependency(dependency);
    }
    struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
    unsigned roles=atomic_load_explicit(&f->route.active_roles,memory_order_acquire);
    /* A completed Remote-only frame does not own the later environment pass. */
    if (!roles) {
        ReleaseSRWLockExclusive(&cloth_map_lock);
        return original_cloth_dependency(dependency);
    }
    unsigned pass=roles==f->pass_roles[0] ? 0 : 1;
    struct jc_cloth_native_jobs *capture=NULL;
    int valid=jc_cloth_live_owner(s) && f->phase==JC_FRAME_WRITING &&
        f->generation==cloth_maps.generation && roles==f->pass_roles[pass] &&
        !(roles&cloth_maps.launched_roles) &&
        (cloth_maps.seen[0]&roles)==roles && (cloth_maps.seen[1]&roles)==roles;
    if (valid) capture=HeapAlloc(GetProcessHeap(),0,sizeof(*capture));
    unsigned char owners[JC_CLOTH_PARTITION_SLOTS]={0};
    unsigned expected=0;
    for (unsigned slot=0;slot<JC_CLOTH_PARTITION_SLOTS;++slot) if (f->route.present[slot]) {
        owners[slot]=(unsigned char)f->route.slots[slot].participant;
        expected+=(roles&(1u<<f->route.slots[slot].role))!=0;
    }
    if (!capture || jc_cloth_native_jobs_read(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),
            cloth_maps.barn,pass,roles,owners,capture) || capture->count!=expected ||
        capture->arguments!=(uintptr_t)f->source_args ||
        jc_cloth_worker_prefix_difference(capture->prefix,f->route.args[0].bytes)!=0x210) valid=0;
    if (valid) for (unsigned i=0;i<capture->count;++i) {
        const struct jc_cloth_partition_job *job=capture->jobs+i;
        if (!f->route.present[job->slot] ||
            memcmp(job,f->route.slots+job->slot,sizeof(*job))) { valid=0; break; }
    }
    if (capture) HeapFree(GetProcessHeap(),0,capture);
    if (valid) cloth_maps.launched_roles|=roles;
    else {
        atomic_store_explicit(&f->route.failed,JC_PARTITION_ARGUMENT,memory_order_release);
        s->failed=1; f->phase=JC_FRAME_UNKNOWN;
        message("[JC] native cloth launch refused: current jobs differ from owned frame; resources retained");
    }
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return valid ? original_cloth_dependency(dependency) : 0;
}
static int cloth_producer_adopt(void *context,unsigned index,uint64_t generation,unsigned pass) {
    (void)context;
    if (!own_read(NULL,cloth_participant.barn+0x6bcef0,&cloth_production_job,8) || !cloth_production_job)
        return JC_FRAME_OWNERSHIP;
    return JNETClothAdoptMappedPass(index,generation,pass);
}
static int cloth_producer_pass_done(void *context,uint64_t generation,unsigned pass) {
    (void)context;
    uint64_t job=0; unsigned char busy=1;
    if (generation!=cloth_producer.generation || pass!=cloth_producer.pass ||
        !cloth_producer.adopted || !traveler_context() ||
        !own_read(NULL,cloth_participant.barn+0x6bcef0,&job,8) || job!=cloth_production_job ||
        !job || !own_read(NULL,job+0x58,&busy,1)) return -1;
    return !busy;
}
static int cloth_produce_before_dependency(uintptr_t dependency) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_participant_ready || cloth_production_thread!=GetCurrentThreadId() ||
        dependency!=cloth_participant.barn+0x6bce00) return 0;
    uint32_t pass[2]; int code=JC_FRAME_OWNERSHIP;unsigned stage=0,quarantined=0;
    if (!own_read(NULL,(uintptr_t)GetModuleHandleW(NULL)+0x731f70,pass,sizeof(pass))) goto failed;
    if (pass[0]!=cloth_production_pass || pass[1]!=cloth_production_pass) return 0;
    if (cloth_maps.roles) { quarantined=1;goto failed; }
    if (!jc_cloth_live_owner(s) || s->frames.closing || cloth_production_serial==UINT64_MAX) goto failed;
    struct jc_cloth_producer_live adapter={s,NULL,cloth_producer_adopt,cloth_producer_pass_done};
    struct jc_cloth_producer_ops ops=jc_cloth_producer_live_ops(&adapter);
    code=jc_cloth_producer_prepare(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),
        &cloth_participant,++cloth_production_serial,cloth_production_pass,&ops,&cloth_producer);
    stage=cloth_producer.stage;quarantined=cloth_producer.quarantined;
    if (!code) {
        InterlockedExchange(&JNETCompanionFeatures[32],0);
        InterlockedExchange(&JNETCompanionFeatures[33],0);
        InterlockedExchange(&JNETCompanionFeatures[34],0);
        InterlockedIncrement(&JNETCompanionFeatures[35]);
        cloth_participant_ready=0;return 1;
    }
failed:
    InterlockedExchange(&JNETCompanionFeatures[32],code);
    InterlockedExchange(&JNETCompanionFeatures[33],(LONG)stage);
    InterlockedExchange(&JNETCompanionFeatures[34],(LONG)cloth_producer.detail);
    cloth_participant_ready=0;
    if (!quarantined) {
        LONG declines=InterlockedIncrement(&JNETCompanionFeatures[30]);
        if (declines==1 || (declines&63)==0) {
            char line[192];
            snprintf(line,sizeof(line),"[JC] private cloth declined code=%d stage=%u detail=%u jobs=%u roles=%u declines=%ld",
                code,stage,cloth_producer.detail,cloth_producer.jobs.count,cloth_producer.roles,(long)declines);
            message(line);
        }
        return 0;
    }
    s->failed=1;
    message("[JC] automatic cloth preparation failed before worker release; retained for inspection");
    return -1;
}
static void cloth_render_model_report(unsigned role,unsigned participant,uintptr_t model) {
    uint64_t span[2]={0},item=0,geometry=0,collection=0,binding=0,shader=0,table=0;
    uint32_t batches=0,count=0,selector=0; uint64_t members=0;
    int readable=model && own_read(NULL,model+0x60,span,sizeof(span)) && span[0] &&
        own_read(NULL,span[0],&item,8) && item && own_read(NULL,item,&geometry,8) && geometry &&
        own_read(NULL,geometry,&batches,4) && own_read(NULL,geometry+8,&table,8) &&
        own_read(NULL,item+0x20,&collection,8) && collection && own_read(NULL,collection,&count,4) &&
        own_read(NULL,table,&selector,4) && (count&0x7fffffff)>0 && (count&0x7fffffff)<=16 && selector<(count&0x7fffffff);
    if (readable) {
        readable=own_read(NULL,collection+8,&members,8) && members;
        if (readable && (count&0x7fffffff)==1) binding=members;
        else if (readable) readable=members<=UINT64_MAX-8*selector && own_read(NULL,members+8*selector,&binding,8);
        readable=readable && binding && own_read(NULL,binding,&shader,8);
    }
    char line[384];
    snprintf(line,sizeof(line),"[JC] cloth render model role=%u owner=%u readable=%d model=%llx span=%llx/%llx item=%llx geometry=%llx batches=%u table=%llx collection=%llx count=%u selector=%u binding=%llx shader=%llx",
        role,participant,readable,(unsigned long long)model,(unsigned long long)span[0],(unsigned long long)span[1],
        (unsigned long long)item,(unsigned long long)geometry,batches,(unsigned long long)table,
        (unsigned long long)collection,count,selector,(unsigned long long)binding,(unsigned long long)shader);
    message(line);
}
static void cloth_publish_at_render(uintptr_t barn,unsigned pass,uintptr_t context) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_producer.adopted || barn!=cloth_participant.barn ||
        pass!=cloth_producer.pass || !jc_cloth_live_owner(s)) return;
    unsigned index=cloth_producer.index;
    if (index>=JC_CLOTH_FRAMES || s->frames.frames[index].phase!=JC_FRAME_READY ||
        s->automatic_pending[index] || s->commands[index].owner) return;
    unsigned detail=0;
    int code=jc_cloth_render_register(s,index,(uintptr_t)GetModuleHandleW(NULL),cloth_participant.barn,context,(uint32_t)JNETFrames,&detail);
    if (code) {
        InterlockedExchange(&JNETCompanionFeatures[32],code);
        InterlockedExchange(&JNETCompanionFeatures[33],105);
        InterlockedExchange(&JNETCompanionFeatures[34],(LONG)detail);
        char line[256];
        snprintf(line,sizeof(line),"[JC] cloth render registration failed code=%d detail=%u frame=%u phase=%u committed=%u draws=%u refs=%u thread=%lu/%lu context=%llx",
            code,detail,index,s->frames.frames[index].phase,s->frames.frames[index].stock_committed,
            s->frames.frames[index].draw_count,s->frames.frames[index].ref_count,
            (unsigned long)GetCurrentThreadId(),(unsigned long)s->thread,(unsigned long long)context);
        message(line);
        struct jc_cloth_frame *f=s->frames.frames+index;
        for (unsigned n=0;n<f->ref_count;++n) if (f->refs[n].kind==JC_FRAME_VERTEX || f->refs[n].kind==JC_FRAME_INDEX) {
            struct jc_cloth_live_buffer *buffer=jc_cloth_live_buffer_find(s,f->refs[n].object);
            snprintf(line,sizeof(line),"[JC] cloth render buffer ref=%u kind=%u object=%llx found=%u mapped=%u pins=%u",
                n,f->refs[n].kind,(unsigned long long)f->refs[n].object,buffer!=NULL,
                buffer && buffer->mapped,buffer ? buffer->pins : 0);
            message(line);
        }
        for (unsigned r=0;r<7;++r) if (f->streams[r].indices) {
            uintptr_t model=0;own_read(NULL,barn+0x666ed8+r*0x630,&model,8);
            cloth_render_model_report(r,0,model);
        }
        for (unsigned n=0;n<f->draw_count;++n) {
            const struct jc_cloth_material *d=&f->draws[n].material;
            struct jc_cloth_live_material *m=jc_cloth_live_material_find(s,d->material);
            cloth_render_model_report(d->role,d->participant,m ? (uintptr_t)m->model : 0);
            snprintf(line,sizeof(line),"[JC] cloth render material draw=%u role=%u owner=%u found=%u pins=%u ready=%u quarantined=%u bindings=%d",
                n,d->role,d->participant,m!=NULL,m ? m->pins : 0,m ? m->ready : 0,m ? m->quarantined : 0,
                m ? jc_cloth_live_bindings_match(s,d) : 0);
            message(line);
        }
        s->failed=1;s->frames.frames[index].phase=JC_FRAME_UNKNOWN;cloth_participant_ready=0;
        if (traveler_actor) traveler_actor_report(jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE));
        JNETActorTrial[0]=-1;JNETActorTrial[1]=JC_ACTOR_RUNTIME_SCOPE;
        return;
    }
    InterlockedIncrement(&JNETCompanionFeatures[36]);
    memset(&cloth_maps,0,sizeof(cloth_maps));cloth_producer.adopted=0;
}
static void cloth_render_observer(uintptr_t barn,unsigned pass,uintptr_t context) {
    original_cloth_render(barn,pass,context);
    cloth_publish_at_render(barn,pass,context);
}
struct cloth_finish_call { uintptr_t barn,result,fourth,returned; unsigned roles,detail,called; };
static int cloth_finish_native(void *context,uint64_t generation,unsigned pass) {
    struct cloth_finish_call *call=context;
    if (generation!=cloth_maps.generation) { call->detail=1;return -1; }
    call->called=1;
    DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
    cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=1;
    call->returned=original_cloth_finish(call->barn,(void *)call->result,pass,call->fourth);
    cloth_production_thread=old_thread; cloth_production_pass=old_pass;
    unsigned char completed=0;
    if (call->returned!=call->result || !own_read(NULL,call->result,&completed,1) || completed!=1) {
        call->detail=2;return -1;
    }
    for (unsigned r=0;r<7;++r) if (call->roles&(1u<<r)) {
        uintptr_t objects[2],vertices=0,indices=0;
        unsigned detail=0;
        if (!cloth_map_objects(call->barn,r,objects) ||
            memcmp(objects,cloth_maps.objects[r],sizeof(objects))) detail=1;
        else if (jc_cloth_ring_finished(own_read,NULL,&cloth_maps.rings[r][0])) detail=2;
        else if (jc_cloth_ring_finished(own_read,NULL,&cloth_maps.rings[r][1])) detail=3;
        else if (!own_read(NULL,call->barn+0x669970+r*8,&vertices,8) || vertices) detail=4;
        else if (!own_read(NULL,call->barn+0x6699a8+r*8,&indices,8) || indices) detail=5;
        if (detail) { call->detail=16+r*8+detail;return -1; }
    }
    return 1;
}
static uintptr_t cloth_finish_observer(uintptr_t barn,void *result,unsigned pass,uintptr_t fourth) {
    struct jc_cloth_live *s=active_cloth_backend;
    if (!s || !cloth_maps.roles || barn!=cloth_maps.barn || pass>1) {
        DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
        if (s && cloth_participant_ready && barn==cloth_participant.barn && pass==0) {
            cloth_production_thread=GetCurrentThreadId();cloth_production_pass=1;
        }
        uintptr_t returned=original_cloth_finish(barn,result,pass,fourth);
        cloth_production_thread=old_thread;cloth_production_pass=old_pass;
        return returned;
    }
    struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
    if (cloth_maps.finish_failed ||
        !(f->pass_required&(1u<<pass)) || (f->pass_finalized&(1u<<pass))) {
        DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
        cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=1;
        uintptr_t returned=original_cloth_finish(barn,result,pass,fourth);
        cloth_production_thread=old_thread; cloth_production_pass=old_pass;
        return returned;
    }
    unsigned char pending=0,done=0,busy=0; uintptr_t job=0;
    unsigned guard=0;
    if (!jc_cloth_live_owner(s)) guard=1;
    else if (f->phase!=JC_FRAME_WRITING) guard=2;
    else if (f->generation!=cloth_maps.generation) guard=3;
    else if (!own_read(NULL,barn+0x6bcf84,&pending,1)) guard=4;
    else if (pending!=1) guard=5;
    else if (!own_read(NULL,barn+0x6bcad0+pass,&done,1)) guard=6;
    else if (done) guard=7;
    else if (!own_read(NULL,barn+0x6bcef0,&job,8) || !job) guard=8;
    else if (!own_read(NULL,job+0x58,&busy,1)) guard=9;
    int valid=!guard;
    if (valid && busy) {
        unsigned char waiting[16]={0};
        memcpy(waiting+8,&job,8);
        if (jc_local_windows_write((uintptr_t)result,waiting,sizeof(waiting))) return (uintptr_t)result;
        valid=0;guard=10;
    }
    unsigned stage=100,phase=f->phase;
    int code=valid ? JC_FRAME_OK : JC_FRAME_OWNERSHIP;
    if (!code) { stage=101;code=jc_cloth_frame_seal_pass(&s->frames,cloth_maps.index,pass); }
    if (!code) { stage=102;code=jc_cloth_frame_commit_pass(f,f->generation,pass,(void *)f->source_args,JC_CLOTH_WORKER_ARGS); }
    struct cloth_finish_call call={.barn=barn,.result=(uintptr_t)result,.fourth=fourth,.roles=f->pass_roles[pass]};
    if (!code) { stage=103;code=jc_cloth_frame_finalize_pass(&s->frames,cloth_maps.index,pass,cloth_finish_native,&call); }
    if (!code && f->phase==JC_FRAME_READY) { stage=104;code=jc_cloth_live_unmap(s,cloth_maps.index); }
    if (code) {
        unsigned route=atomic_load_explicit(&f->route.failed,memory_order_acquire);
        unsigned rejection=atomic_load_explicit(&f->route.rejection,memory_order_acquire);
        unsigned detail=stage==100 ? guard : call.detail ? call.detail : rejection ? rejection : route;
        InterlockedExchange(&JNETCompanionFeatures[32],code);
        InterlockedExchange(&JNETCompanionFeatures[33],(LONG)stage);
        InterlockedExchange(&JNETCompanionFeatures[34],(LONG)detail);
        char line[384];
        snprintf(line,sizeof(line),"[JC] cloth finish failure code=%d stage=%u detail=%u pass=%u phase=%u pending=%u done=%u busy=%u route=%u completed=%u jobs=%u sealed=%u committed=%u finalized=%u native-called=%u",
            code,stage,detail,pass,phase,pending,done,busy,route,
            atomic_load_explicit(&f->route.completed,memory_order_acquire),(unsigned)f->route.jobs,
            f->pass_sealed,f->pass_committed,f->pass_finalized,call.called);
        message(line);
        for (unsigned owner=0;owner<JC_CLOTH_PARTICIPANTS;++owner)
            for (unsigned role=0;role<7;++role) if (f->pass_roles[pass]&(1u<<role)) {
                const struct jc_cloth_partition_range *range=&f->route.plan.ranges[owner][role];
                if (!range->jobs) continue;
                snprintf(line,sizeof(line),"[JC] cloth finish counters owner=%u role=%u vertices=%u/%u indices=%u/%u jobs=%u",
                    owner,role,jc_cloth_u32(f->route.args[owner].bytes+0x210+4*role),
                    range->vertex_begin+range->vertices,jc_cloth_u32(f->route.args[owner].bytes+0x22c+4*role),
                    range->index_begin+range->indices,range->jobs);
                message(line);
            }
        cloth_maps.finish_failed=1;
        s->failed=1; f->phase=JC_FRAME_UNKNOWN;cloth_participant_ready=0;
        atomic_store_explicit(&f->route.active_roles,0,memory_order_release);
        if (traveler_actor)
            traveler_actor_report(jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE));
        JNETActorTrial[0]=-1;JNETActorTrial[1]=JC_ACTOR_RUNTIME_SCOPE;
        /* Native completion and private publication are separate contracts.
           Keep all private storage pinned; never replace the native result with
           an incomplete result that its caller will poll forever. */
        return call.called ? call.returned : original_cloth_finish(barn,result,pass,fourth);
    }
    return call.returned;
}
static void cloth_worker_call(void *context,void *worker,const void *entry,void *args,
                              void *streams,void *descriptor,void *state) {
    jc_native_cloth_worker *native=context;
    (*native)(worker,entry,args,streams,descriptor,state);
}
static void cloth_worker_observer(unsigned kind,void *worker,const void *entry,void *args,
                                  void *streams,void *descriptor,void *state) {
    AcquireSRWLockShared(&cloth_map_lock);
    struct jc_cloth_live *s=active_cloth_backend;
    int handled=0;
    if (s && cloth_maps.roles && (uintptr_t)streams==cloth_maps.barn+0x669960) {
        struct jc_cloth_frame *f=s->frames.frames+cloth_maps.index;
        unsigned char captured[16];
        if (!own_read(NULL,(uintptr_t)entry,captured,sizeof(captured))) {
            if (!InterlockedCompareExchange(&cloth_maps.worker_reported,1,0)) {
                char line[128];
                snprintf(line,sizeof(line),"[JC] cloth worker entry unreadable kind=%u entry=%llx",
                    kind,(unsigned long long)(uintptr_t)entry);
                message(line);
            }
            atomic_store_explicit(&f->route.failed,JC_PARTITION_ARGUMENT,memory_order_release);
            handled=-1;
        } else {
            unsigned slot=jc_cloth_u32(captured),role=jc_cloth_u32(captured+4);
            if (role<7 && (cloth_maps.roles&(1u<<role)) &&
                (!(cloth_maps.launched_roles&(1u<<role)) || slot>=JC_CLOTH_PARTITION_SLOTS ||
                 (uintptr_t)state!=cloth_maps.barn+0x270080+(uintptr_t)slot*0xe0)) {
                atomic_store_explicit(&f->route.failed,JC_PARTITION_SLOT,memory_order_release);
                handled=-1;
            } else handled=jc_cloth_frame_dispatch(f,cloth_maps.generation,cloth_maps.roles,
                (void *)(cloth_maps.barn+0x669960),&original_cloth_worker[kind],cloth_worker_call,
                worker,captured,args,streams,descriptor,state);
            if (handled<0 && !InterlockedCompareExchange(&cloth_maps.worker_reported,1,0)) {
                uintptr_t vertex=0,index=0;
                if (role<7) {
                    own_read(NULL,(uintptr_t)streams+0x10+role*8,&vertex,8);
                    own_read(NULL,(uintptr_t)streams+0x48+role*8,&index,8);
                }
                char line[512];
                snprintf(line,sizeof(line),"[JC] cloth worker rejected kind=%u slot=%u role=%u route=%u rejection=%u phase=%u args=%llx/%llx vertex=%llx/%llx index=%llx/%llx visible=%u subdivided=%u",
                    kind,slot,role,atomic_load(&f->route.failed),atomic_load(&f->route.rejection),f->phase,
                    (unsigned long long)(uintptr_t)args,(unsigned long long)(uintptr_t)f->source_args,
                    (unsigned long long)vertex,(unsigned long long)(uintptr_t)(role<7 ? f->streams[role].mapped_vertices : NULL),
                    (unsigned long long)index,(unsigned long long)(uintptr_t)(role<7 ? f->streams[role].mapped_indices : NULL),
                    captured[9],captured[8]);
                message(line);
            }
        }
    }
    if (!handled && s && cloth_participant.dude) {
        unsigned char captured[16];
        if (own_read(NULL,(uintptr_t)entry,captured,sizeof(captured)) &&
            jc_cloth_producer_owns_worker(own_read,NULL,&cloth_participant,&cloth_producer.lease,captured,
                (uintptr_t)args,(uintptr_t)streams,(uintptr_t)state)) {
            InterlockedIncrement(&JNETCompanionFeatures[31]);handled=1;
        }
    }
    if (!handled) original_cloth_worker[kind](worker,entry,args,streams,descriptor,state);
    ReleaseSRWLockShared(&cloth_map_lock);
}
static void cloth_worker_1e2d10(void *worker,const void *entry,void *args,void *streams,
                                 void *descriptor,void *state) {
    cloth_worker_observer(0,worker,entry,args,streams,descriptor,state);
}
static void cloth_worker_1e4970(void *worker,const void *entry,void *args,void *streams,
                                    void *descriptor,void *state) {
    cloth_worker_observer(1,worker,entry,args,streams,descriptor,state);
}
static uint32_t cloth_vertex_map_observer(uintptr_t object,uintptr_t output) {
    AcquireSRWLockExclusive(&cloth_map_lock);
    uint32_t result=cloth_map_observer(object,output,0);
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return result;
}
static uint32_t cloth_index_map_observer(uintptr_t object,uintptr_t output) {
    AcquireSRWLockExclusive(&cloth_map_lock);
    uint32_t result=cloth_map_observer(object,output,1);
    ReleaseSRWLockExclusive(&cloth_map_lock);
    return result;
}
static int traveler_frame_retired(void) {
    int ready=jc_actor_frame_poll(&actor_frame);
    if (ready==1 && actor_frame.generation && active_cloth_backend)
        return jc_cloth_render_frame_complete(active_cloth_backend,actor_frame.generation);
    return ready;
}
static int traveler_peer_replace(uint64_t peer,uint64_t generation);
#include "traveler_live_backend.h"
#include "traveler_peer_lifecycle.h"
#include "traveler_local_peer.h"
#include "jnet_lobby_runtime.h"
#include "jnet_steam_lobby.h"
#include "jnet_steam_transport.h"
#include "jnet_steam_game_live.h"
#include "jnet_steam_maintenance.h"
#include "jnet_steam_realtime.h"
#include "jnet_travel_live.h"
#include "jnet_chat_live.h"
#include "jnet_options_live.h"
#include "jnet_appearance_live.h"
#include "jnet_character_live.h"
#include "jnet_rocket_live.h"
#include "jnet_intro_live.h"
static void (*jnet_character_skeleton_build_original)(uintptr_t,uintptr_t);
static void jnet_character_skeleton_build(uintptr_t output,uintptr_t model) {
    char name[64]={0},line[200];
    int traced=output>=0xc8 && own_read(NULL,output-0xc8,name,sizeof(name)) &&
        memchr(name,0,sizeof(name)) && !strncmp(name,"JNETCharacter_",14);
    if(traced){
        snprintf(line,sizeof(line),"[JNET] skeleton build begin thread=%lu key=%s",
            (unsigned long)GetCurrentThreadId(),name);message(line);
    }
    jnet_character_skeleton_build_original(output,model);
    if(traced){
        uint32_t count=0;
        int readable=own_read(NULL,output+0x20,&count,sizeof(count));
        snprintf(line,sizeof(line),"[JNET] skeleton build end thread=%lu key=%s nodes=%lu readable=%d",
            (unsigned long)GetCurrentThreadId(),name,(unsigned long)count,readable);message(line);
    }
}
static void (*jnet_character_animation_load_original)(uintptr_t);
static void (*jnet_character_animation_finish_original)(uintptr_t);
static SRWLOCK jnet_character_animation_lock=SRWLOCK_INIT;
static void jnet_character_animation_available(const char *name,int ready) {
    AcquireSRWLockExclusive(&jnet_character_animation_lock);
    unsigned blocked=jnet_character_animation_blocked(jnet_character_animation_dependencies,
        sizeof(jnet_character_animation_dependencies)/sizeof(jnet_character_animation_dependencies[0]),name,ready);
    InterlockedExchange((volatile LONG *)&jnet_menu_supported_characters,
        ((1u<<JNET_CHARACTER_COUNT)-2u)&~blocked);
    ReleaseSRWLockExclusive(&jnet_character_animation_lock);
}
static void jnet_character_animation_stage(uintptr_t resource,int finish) {
    char name[96]={0},line[240];uint64_t length=0,capacity=0,address=resource+8,payload=0;
    int private_resource=own_read(NULL,resource+0x18,&length,8) && length<sizeof(name) &&
        own_read(NULL,resource+0x20,&capacity,8) && capacity>=length;
    if(private_resource && capacity>=16)private_resource=own_read(NULL,resource+8,&address,8);
    private_resource=private_resource && own_read(NULL,address,name,(size_t)length+1) &&
        !name[length];
    int spin_resource=private_resource && !strcmp(name,"JNETSpin3");
    private_resource=private_resource && !strncmp(name,"JC_",3);
    if(spin_resource){
        snprintf(line,sizeof(line),"[JNET-mesh] Spin3 %s begin\r\n",finish?"finalize":"initialize");
        jnet_log_append(&session_logs->fault,line,strlen(line));
    }
    if(private_resource){
        snprintf(line,sizeof(line),"[JNET] animation %s begin thread=%lu key=%s",
            finish?"finalize":"initialize",(unsigned long)GetCurrentThreadId(),name);message(line);
        if(finish && (!own_read(NULL,resource+0xe8,&payload,8) || !payload)){
            jnet_character_animation_available(name,0);
            message("[JNET] private animation has no payload; finalization declined and dependent characters disabled");return;
        }
    }
    if(finish)jnet_character_animation_finish_original(resource);
    else jnet_character_animation_load_original(resource);
    if(spin_resource){
        int readable=own_read(NULL,resource+0xe8,&payload,8);
        snprintf(line,sizeof(line),"[JNET-mesh] Spin3 %s end read=%d payload=%llx\r\n",
            finish?"finalize":"initialize",readable,(unsigned long long)payload);
        jnet_log_append(&session_logs->fault,line,strlen(line));
    }
    if(private_resource){
        int readable=own_read(NULL,resource+0xe8,&payload,8);
        snprintf(line,sizeof(line),"[JNET] animation %s end thread=%lu key=%s payload=%d",
            finish?"finalize":"initialize",(unsigned long)GetCurrentThreadId(),name,readable && payload!=0);message(line);
        jnet_character_animation_available(name,readable && payload!=0);
    }
}
static void jnet_character_animation_load(uintptr_t resource) {jnet_character_animation_stage(resource,0);}
static void jnet_character_animation_finish(uintptr_t resource) {jnet_character_animation_stage(resource,1);}
static int (*jnet_character_asset_load_original)(const char *,void *,unsigned char,const char *);
static int jnet_character_asset_load(const char *path,void *result,unsigned char packed,const char *key) {
    char line[480], bundle_path[MAX_PATH];
    const char *resolved=path;
    if(!packed && key){
        for(size_t i=0;i<sizeof(jnet_character_animation_files)/sizeof(jnet_character_animation_files[0]);++i){
            if(strcmp(key,jnet_character_animation_files[i].key))continue;
            DWORD length=GetModuleFileNameA(self,bundle_path,sizeof(bundle_path));
            if(!length || length>=sizeof(bundle_path))return 10;
            char *slash=strrchr(bundle_path,'\\');
            if(!slash)return 10;
            size_t prefix=(size_t)(slash+1-bundle_path);
            const char *file=jnet_character_animation_files[i].file;
            if(prefix+strlen(file)>=sizeof(bundle_path))return 10;
            strcpy(bundle_path+prefix,file);
            resolved=bundle_path;
            break;
        }
    }
    int trace=(key && (strstr(key,"JNETCharacter_") || !strncmp(key,"JC_",3))) || (path && strstr(path,"JNETCharacter_"));
    if(trace){
        DWORD attributes=GetFileAttributesA(resolved);
        DWORD error=attributes==INVALID_FILE_ATTRIBUTES?GetLastError():0;
        snprintf(line,sizeof(line),"[JNET] character asset begin thread=%lu packed=%u key=%.80s path=%.220s file_error=%lu",
            (unsigned long)GetCurrentThreadId(),(unsigned)packed,key?key:"(none)",resolved?resolved:"(null)",(unsigned long)error);
        message(line);
    }
    int code=jnet_character_asset_load_original(resolved,result,packed,key);
    if(trace){
        snprintf(line,sizeof(line),"[JNET] character asset end thread=%lu key=%.80s result=%d",
            (unsigned long)GetCurrentThreadId(),key?key:"(none)",code);
        message(line);
    }
    return code;
}
#include "jnet_daynight_live.h"
#include "jnet_freeplay_live.h"
static int jnet_online_enabled=1;
static LONG jnet_online_input_frame=-1;
static void jnet_online_before_input(uintptr_t game) {
    if(!jnet_online_enabled || game!=(uintptr_t)JNETGame || jnet_online_input_frame==JNETFrames)return;
    jnet_online_input_frame=JNETFrames;jnet_sg_game_phase(game,1);
}
static void jnet_online_game_tick(uintptr_t game) {
    traveler_pending_initialize();
    jnet_menu_escape();
    jnet_options_apply();
    jnet_travel_game(game);
    if(JC_NATIVE_CHARACTERS_ENABLED)jnet_character_tick_native(game);
    jnet_appearance_game(game);
    jnet_daynight_game(game);
    jnet_rocket_game(game);
    if(JC_NATIVE_CHARACTERS_ENABLED)jnet_character_remote_tick(game);
    JNETSteamGame[76]=jnet_diagnostic_scan_last;JNETSteamGame[77]=jnet_diagnostic_scan_peak;
    JNETSteamGame[78]=jnet_diagnostic_scan_count;JNETSteamGame[79]=jnet_diagnostic_suppressed;
    if(jnet_online_input_frame!=JNETFrames){if(JNETSteamGame[0] || traveler_network_external)++JNETSteamGame[56];jnet_online_before_input(game);}
    static uint64_t next;uint64_t now=GetTickCount64();if(now<next){jnet_sg_game_phase(game,0);return;}next=now+250;
    unsigned char pending=1;int state=0,resume=0,level=-1;
    JNETSteamLobby[9]=!jnet_travel_busy && !jc_render_level(own_read,NULL,game,&level) && level>=0 && level<12 &&
        own_read(NULL,game+0x1c0330,&pending,1) && own_read(NULL,game+0x278,&state,4) &&
        own_read(NULL,game+0x27c,&resume,4) && jnet_travel_native_loaded((unsigned)level,(unsigned)level,pending,state,resume);
    uintptr_t match=0;unsigned char connected=0;
    JNETSteamLobby[10]=!game || !own_read(NULL,game+0x180,&match,sizeof(match)) || !match || !own_read(NULL,match+0x11db4,&connected,1)?-1:connected?1:0;
    jnet_sg_game_phase(game,0);
}
#include "traveler_chirp_charge.h"
static int64_t traveler_last_update;
static int traveler_frame_wait(void) {
    if (!traveler_actor || !active_cloth_backend) return 1;
    ULONGLONG deadline=GetTickCount64()+500;
    for (;;) {
        int ready=traveler_frame_retired();
        if (ready<0 || active_cloth_backend->failed) return 0;
        int all_free=1;
        if (jc_cloth_frames_poll(&active_cloth_backend->frames)) return 0;
        for (unsigned i=0;i<JC_CLOTH_FRAMES;++i)
            if (active_cloth_backend->frames.frames[i].phase!=JC_FRAME_FREE) all_free=0;
        if (ready && all_free && !InterlockedCompareExchange(&cloth_progress_pending,0,0)) return 1;
        cloth_progress_request();
        if (GetTickCount64()>=deadline) {
            message("[JC] prior actor frame still pending; preserving leases before another game update");
            return 0;
        }
        Sleep(1);
    }
}
static void traveler_frame_complete(uintptr_t game) {
    if (!active_cloth_backend ||
        ((JNETTraveler[0]!=3 || !traveler_context()) &&
         !jc_cloth_render_frame_pending(active_cloth_backend,(uint32_t)JNETFrames))) return;
    /* Stopping actor updates does not unregister its floating-glyph effects.
       Fence every retained frame until the native chapter cleanup removes them. */
    if (jc_actor_frame_after(&actor_frame,own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),game,
            (uint32_t)JNETFrames,original_base_flush,original_render_handoff)) {
        active_cloth_backend->failed=1;
        if (traveler_actor)
            traveler_actor_report(jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE));
        message("[JC] actor frame closure could not be proven; resources retained");
    }
}
static void chapter_switch_observer(uintptr_t game,const char *target) {
    if(traveler_actor_trial_join_pending || traveler_stream_waiting[1]) {
        traveler_local_interrupt();traveler_repeat_cancel();
        traveler_actor_trial_join_pending=0;traveler_peer_restart_requested=0;
    }
    if (!active_cloth_backend) { original_chapter_switch(game,target); return; }
    if (cloth_exit_window) return;
    traveler_local_interrupt();
    traveler_repeat_cancel();
    traveler_peer_depart_requested=traveler_peer_restart_requested=0;
    active_cloth_backend->frames.closing=1;
    char name[64];
    int readable=0;
    memset(name,0,sizeof(name));
    for (unsigned i=0;i<sizeof(name);++i) {
        if (!own_read(NULL,(uintptr_t)target+i,name+i,1)) break;
        if (!name[i]) { readable=1; break; }
    }
    if (!readable || game!=traveler_game || !jc_chapter_defer(&cloth_chapter,game,GetCurrentThreadId(),name)) {
        cloth_chapter.failed=1;
        message("[JC] cloth chapter request quarantined; native teardown withheld"); return;
    }
    /* GameUpdate clears its pending name after this hook returns. The private
       copy is replayed at the next update boundary after the drain completes. */
}
static void cloth_chapter_tick(uintptr_t game) {
    if (!active_cloth_backend || cloth_exit_window || !cloth_chapter.pending || cloth_chapter.failed) return;
    if (game!=cloth_chapter.game || GetCurrentThreadId()!=cloth_chapter.thread) {
        cloth_chapter.failed=1; message("[JC] cloth chapter owner changed; native teardown withheld"); return;
    }
    cloth_progress_request();
    traveler_repeat_cancel();
    int actor_code=JNETActorStop();
    if (actor_code) {
        if (actor_code!=JC_ACTOR_RUNTIME_BUSY) cloth_chapter.failed=1;
        return;
    }
    int code=traveler_live_detach_body_probe(active_cloth_backend);
    if (!code) code=jc_cloth_live_close(active_cloth_backend);
    if (!code && InterlockedCompareExchange(&cloth_progress_pending,0,0)) return;
    char target[64];
    int drained=code==JC_FRAME_OK ? 1 : code==JC_FRAME_BUSY ? 0 : -1;
    if (jc_chapter_take(&cloth_chapter,game,GetCurrentThreadId(),drained,target)) {
        AcquireSRWLockExclusive(&cloth_map_lock);
        memset(&cloth_maps,0,sizeof(cloth_maps));
        ReleaseSRWLockExclusive(&cloth_map_lock);
        active_cloth_backend=NULL;
        original_chapter_switch(game,target);
    } else if (cloth_chapter.failed) message("[JC] cloth chapter drain quarantined; native teardown withheld");
}
static void CALLBACK cloth_exit_timer(HWND window,UINT message_id,UINT_PTR timer,DWORD elapsed) {
    (void)window; (void)message_id; (void)elapsed;
    if (!cloth_exit_window || timer!=cloth_exit_timer_id || !active_cloth_backend) return;
    if (active_cloth_backend->thread!=GetCurrentThreadId()) return;
    cloth_progress_request();
    traveler_repeat_cancel();
    if (JNETActorStop()) return;
    int code=traveler_live_detach_body_probe(active_cloth_backend);
    if (!code) code=jc_cloth_live_close(active_cloth_backend);
    LONG progress=InterlockedCompareExchange(&cloth_progress_pending,0,0);
    if ((code && code!=JC_FRAME_BUSY) || progress<0) {
        if (!cloth_exit_reported) {
            cloth_exit_reported=1;
            message("[JC] cloth exit drain quarantined; native destruction withheld");
        }
        return;
    }
    if (code || progress) return;
    HWND target=cloth_exit_window;
    WPARAM wp=cloth_exit_wparam; LPARAM lp=cloth_exit_lparam;
    if (!cloth_kill_timer(NULL,cloth_exit_timer_id)) return;
    AcquireSRWLockExclusive(&cloth_map_lock);
    memset(&cloth_maps,0,sizeof(cloth_maps));
    ReleaseSRWLockExclusive(&cloth_map_lock);
    cloth_exit_timer_id=0; cloth_exit_window=NULL; active_cloth_backend=NULL;
    memset(&cloth_chapter,0,sizeof(cloth_chapter));
    original_game_window(target,WM_CLOSE,wp,lp);
}
static int cloth_quarantined_close(UINT msg) {
    if (msg!=WM_CLOSE || !active_cloth_backend || !active_cloth_backend->failed) return 0;
    message("[JC] user requested close after cloth quarantine; terminating this Journey process, exit e04a4301");
    if (!TerminateProcess(GetCurrentProcess(),0xe04a4301u))
        message("[JC] quarantined Journey process termination failed");
    return 1;
}
static LRESULT CALLBACK cloth_window_observer(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
    if (cloth_quarantined_close(msg)) return 0;
    if(jnet_chat_window(msg,wp,lp))return 0;
    if(jnet_chat_focus && msg==WM_INPUT){
        RAWINPUT input;UINT size=sizeof(input);
        if(GetRawInputData((HRAWINPUT)lp,RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))!=(UINT)-1 &&
           input.header.dwType==RIM_TYPEKEYBOARD && !(input.data.keyboard.Flags&RI_KEY_BREAK))
            return DefWindowProcW(window,msg,wp,lp);
    }
    if(jnet_menu_password_focus && msg==WM_INPUT){
        RAWINPUT input;UINT size=sizeof(input);
        if(GetRawInputData((HRAWINPUT)lp,RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))!=(UINT)-1 &&
           input.header.dwType==RIM_TYPEKEYBOARD && input.data.keyboard.VKey==VK_BACK)
            return DefWindowProcW(window,msg,wp,lp);
    }
    if(jnet_menu_password_focus && ((wp==VK_BACK &&
       (msg==WM_KEYDOWN || msg==WM_KEYUP || msg==WM_SYSKEYDOWN || msg==WM_SYSKEYUP)) ||
       (msg==WM_CHAR && wp==8)))return 0;
    if (msg!=WM_CLOSE || !active_cloth_backend) return original_game_window(window,msg,wp,lp);
    if (active_cloth_backend->thread!=GetCurrentThreadId()) {
        message("[JC] cloth close deferred: window/producer thread mismatch"); return 0;
    }
    if (cloth_exit_window) return 0;
    HMODULE user=GetModuleHandleW(L"user32.dll");
    cloth_set_timer=(void *)GetProcAddress(user,"SetTimer");
    cloth_kill_timer=(void *)GetProcAddress(user,"KillTimer");
    if (!cloth_set_timer || !cloth_kill_timer) {
        message("[JC] cloth close deferred: timer services unavailable"); return 0;
    }
    cloth_exit_window=window; cloth_exit_wparam=wp; cloth_exit_lparam=lp;
    cloth_exit_reported=0;
    active_cloth_backend->frames.closing=1;
    cloth_exit_timer_id=cloth_set_timer(NULL,0,16,cloth_exit_timer);
    if (!cloth_exit_timer_id) {
        cloth_exit_window=NULL;
        message("[JC] cloth close deferred: timer allocation failed");
    }
    return 0;
}
static uintptr_t cleanup_observer(uintptr_t dude, uintptr_t handles) {
    if(dude==traveler_barn+0xb0 || dude==traveler_object) {
        if(!cloth_actor_growth_stop())message("[JC] scarf growth cleanup ownership changed");
        memset(&cloth_actor_growth,0,sizeof(cloth_actor_growth));
    }
    traveler_charge_cleanup(dude);
    traveler_local_cleanup(dude);
    traveler_cleanup(dude,handles);
    int render_cleanup=JNETRender[0]==6 && GetCurrentThreadId()==render_thread && dude==render_barn+0xb0;
    if (render_cleanup) {
        InterlockedExchange(&JNETRender[0],7);
        render_cleanup=render_snapshot(14);
    }
    int observing=JNETResource==1 || JNETResource==2;
    if (observing) boundary_record("[JC] boundary cleanup hook enter");
    if (!TryAcquireSRWLockExclusive(&lifetime_lock)) {
        InterlockedExchange(&lifetime_conflict,1);
        InterlockedExchange(&JNETLifetime,-1);
        if (initialized_life.state==1) { initialized_fault_field(JC_IF_LOCK,3); initialized_quarantine(3); }
        uintptr_t result=original_cleanup(dude,handles);
        if (render_cleanup && JNETRender[0]==7 && render_snapshot(17)) InterlockedExchange(&JNETRender[0],8);
        return result;
    }
    companion_cloth_cleanup_locked(dude);
    if (JNETDual[1] && dude==resource_barn+0xb0) {
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i];
            if (initialized_life.state==1) { preview_fail(13); initialized_quarantine(3); }
            body_publish();
        }
        body_context=&body_contexts[0];
        InterlockedExchange(&JNETDual[0],-1);
    }
    if (JNETResource==1 && dude==resource_barn+0xb0) {
        uint64_t barn;
        int valid=!lifetime_conflict && GetCurrentThreadId()==resource_thread &&
            (uintptr_t)JNETGame==resource_game && own_read(NULL,resource_game+0x1e0,&barn,8) && barn==resource_barn;
        InterlockedExchange(&JNETResource,valid ? 2 : -1);
        boundary_record(valid ? "[JC] boundary local cleanup matched" : "[JC] boundary local cleanup context declined");
    }
    if (lifetime_conflict && lifetime.state==1) jc_lifetime_quarantine(&lifetime);
    if (lifetime.state==1) {
        unsigned char *image=(unsigned char *)GetModuleHandleW(NULL);
        const struct jc_allocation_ops ops={own_read,trial_create,trial_destroy,image};
        int previous=lifetime.state;
        int state=jc_lifetime_cleanup(&lifetime,&ops,(uint64_t)(uintptr_t)image,
                                     (uint64_t)(uintptr_t)JNETGame,GetCurrentThreadId(),dude);
        if (state==2) {
            InterlockedExchange(&JNETTrial,2);
            InterlockedCompareExchange(&JNETLifetime,2,3);
        }
        else if (state<0) InterlockedExchange(&JNETLifetime,-1);
        if (state!=previous) message(state==2 ?
            "[JC] retained unregistered Dude released at native local traveler cleanup; original cleanup continues" :
            "[JC] lifetime validation failed; allocation quarantined until process exit; restart required");
    }
    ReleaseSRWLockExclusive(&lifetime_lock);
    if (observing && JNETResource>=0) boundary_record("[JC] boundary cleanup original enter");
    uintptr_t result=original_cleanup(dude,handles);
    if (render_cleanup && JNETRender[0]==7 && render_snapshot(17)) InterlockedExchange(&JNETRender[0],8);
    if (observing && JNETResource>=0) boundary_record("[JC] boundary cleanup original returned");
    return result;
}
static void allocation_trial(uintptr_t game, int retain) {
    if (allocation_attempted) return;
    unsigned char *image=(unsigned char *)GetModuleHandleW(NULL);
    struct jc_layout_result layout;
    struct jc_resource_result resources;
    uint64_t barn, match, after;
    unsigned char connected;
    if (bound_state || lifetime_conflict || jc_native_layout(own_read,NULL,(uint64_t)(uintptr_t)image,game,&layout) ||
        layout.remote_gate || !layout.local_position_finite ||
        jc_native_resources(own_read,NULL,(uint64_t)(uintptr_t)image,game,&resources) ||
        !resources.tables_available || resources.tables_overlap ||
        !resources.controller_owns_mesh[0] || !resources.controller_owns_mesh[1] ||
        !jc_layout_field(own_read,NULL,game,0x1e0,&barn,8) ||
        !jc_layout_field(own_read,NULL,game,0x180,&match,8) ||
        !jc_layout_field(own_read,NULL,match,0x11db4,&connected,1) || connected) {
        InterlockedExchange(&JNETTrial,-1);
        message("[JC] allocation rehearsal declined: requires fresh native-only solo gameplay and valid resources");
        return;
    }
    for (size_t i=0;i<sizeof(jc_targets)/sizeof(jc_targets[0]);++i) {
        const struct jc_target *t=jc_targets+i;
        if (t->rva==JC_RVA_EnvironmentCopy || t->rva==JC_RVA_EnvironmentUpdate || t->rva==JC_RVA_TextureNullGuard || t->rva==JC_RVA_IntroUpdate || t->rva==JC_RVA_CharacterBank || t->rva==JC_RVA_CharacterMeshUpdate || t->rva==JC_RVA_CharacterShoutInit || t->rva==JC_RVA_CharacterDecorationUpdate || t->rva==JC_RVA_CharacterDecorationDraw || t->rva==JC_RVA_CharacterDecorationCleanup || t->rva==JC_RVA_CharacterInteract || t->rva==JC_RVA_CharacterBodyInput || t->rva==JC_RVA_ChestSymbol || t->rva==JC_RVA_DudeClothAppearance || t->rva==JC_RVA_NativeShout || t->rva==JC_RVA_CharacterEye || t->rva==JC_RVA_CharacterAttack || t->rva==JC_RVA_CharacterAttackHandler || t->rva==JC_RVA_CharacterJump || t->rva==JC_RVA_CharacterCamera || t->rva==JC_RVA_CreatureReset || t->rva==JC_RVA_SavePoll || t->rva==JC_RVA_SaveLoad || t->rva==JC_RVA_MenuInput || t->rva==JC_RVA_MenuPop || t->rva==JC_RVA_MenuFrame || t->rva==JC_RVA_ScriptExecute || t->rva==JC_RVA_MenuPresent || t->rva==JC_RVA_MenuNodes || t->rva==JC_RVA_MenuBuild || t->rva==JC_RVA_SandInline || t->rva==JC_RVA_SandAppend || t->rva==JC_RVA_SandConsume || t->rva==JC_RVA_DudeFullInit || t->rva==JC_RVA_luaL_loadbufferx || t->rva==JC_RVA_GameUpdate || t->rva==JC_RVA_DudeCleanup || t->rva==JC_RVA_DudeResourceInit || t->rva==JC_RVA_MeshRegister || t->rva==JC_RVA_GeometryBuild || t->rva==JC_RVA_TravelerSubmit || t->rva==JC_RVA_RenderConsume || t->rva==JC_RVA_GeometryDispatch || t->rva==JC_RVA_CommandBuild || t->rva==JC_RVA_RenderFlush || t->rva==JC_RVA_RenderHandoff || t->rva==JC_RVA_BaseFlush || t->rva==JC_RVA_BaseHandoff || t->rva==JC_RVA_CommandExecute || t->rva==JC_RVA_ClothUpdate || t->rva==JC_RVA_ClothFinish || t->rva==JC_RVA_PlayerUpdate || t->rva==JC_RVA_ActorInput || t->rva==JC_RVA_ActorFinish || t->rva==JC_RVA_ClothRender || t->rva==JC_RVA_AsyncGeometryDispatch || t->rva==JC_RVA_ClothDependency || t->rva==JC_RVA_MaterialCommit || t->rva==JC_RVA_MaterialSamplerInit) continue; /* Our own detour is expected. */
        unsigned char bytes[16];
        if (!own_read(NULL,(uint64_t)(uintptr_t)(image+t->rva),bytes,16) || memcmp(bytes,t->prologue,16)) {
            InterlockedExchange(&JNETTrial,-1);
            message("[JC] allocation rehearsal declined: loaded entry points changed");
            return;
        }
    }
    allocation_attempted=1;
    if (retain>=2) {
        if (retain==3) {
            struct jc_geometry_pool geometry;
            struct jc_geometry_build_state snapshot;
            uint64_t object;
            int code=jc_geometry_pool_read(own_read,NULL,barn+0x146f0+0x160+0x80,&geometry);
            if (!code && !jc_layout_field(own_read,NULL,geometry.source,0x10,&object,8)) code=JC_LAYOUT_READ;
            if (!code) code=jc_geometry_build_read(own_read,NULL,(uintptr_t)image,object,&snapshot);
            if (code) { InterlockedExchange(&JNETBuilder[6],code); InterlockedExchange(&JNETResource,-1); return; }
            InterlockedExchange(&JNETBuilder[1],snapshot.limit);
            InterlockedExchange(&JNETBuilder[2],snapshot.slots);
            InterlockedExchange(&JNETBuilder[3],snapshot.occupied);
            InterlockedExchangePointer(&builder_target,(void *)(uintptr_t)object);
        }
        if (!TryAcquireSRWLockExclusive(&lifetime_lock)) {
            InterlockedExchange(&JNETResource,-1); return;
        }
        if (retain==2 && JNETStartup[0]) {
            int level=-1; int code=jc_render_level(own_read,NULL,game,&level);
            InterlockedExchange(&JNETMeshLife[93],level);
            InterlockedExchange(&JNETMeshLife[97],code);
            if (code) { InterlockedExchange(&JNETResource,-1); ReleaseSRWLockExclusive(&lifetime_lock); return; }
            preparation_deadline=GetTickCount64()+600000;
        }
        resource_game=game; resource_barn=barn; resource_thread=GetCurrentThreadId();
        InterlockedExchange(&JNETResource,lifetime_conflict ? -1 : 1);
        boundary_record(retain==3 ? "[JC] geometry build observation armed; no allocations or pool edits" : "[JC] guarded mesh armed; one attempt at next matched resource boundary");
        ReleaseSRWLockExclusive(&lifetime_lock);
        return;
    }
    InterlockedExchange(&JNETTrial,1);
    message("[JC] one-shot native allocation rehearsal starting; no registration or gameplay update");
    const struct jc_allocation_ops ops={own_read,trial_create,trial_destroy,image};
    uint64_t object=0;
    int result=retain ? jc_allocation_create_checked(&ops,(uint64_t)(uintptr_t)image,barn,&object) :
                        jc_allocation_rehearse(&ops,(uint64_t)(uintptr_t)image,barn);
    if (retain && result==JC_ALLOC_OK) {
        if (!TryAcquireSRWLockExclusive(&lifetime_lock)) {
            InterlockedExchange(&lifetime_conflict,1);
            InterlockedExchange(&JNETLifetime,-1);
            InterlockedExchange(&JNETTrial,-3);
            return;
        }
        lifetime=(struct jc_lifetime){object,game,barn,GetCurrentThreadId(),1};
        if (lifetime_conflict || !own_read(NULL,game+0x1e0,&after,8) || after!=barn ||
            InterlockedCompareExchange(&JNETLifetime,1,0)!=0) jc_lifetime_quarantine(&lifetime);
        int state=lifetime.state;
        if (state<0) InterlockedExchange(&JNETLifetime,-1);
        InterlockedExchange(&JNETTrial,state==1 ? 4 : -3);
        ReleaseSRWLockExclusive(&lifetime_lock);
        message(state==1 ? "[JC] unregistered Dude retained; waiting for native local traveler cleanup; no graphics or networking" :
                "[JC] lifetime ownership could not be established; restart required");
        return;
    }
    if (retain) InterlockedExchange(&JNETLifetime,-1);
    if (result==JC_ALLOC_OK && (!jc_layout_field(own_read,NULL,game,0x1e0,&after,8) || after!=barn)) result=-4;
    InterlockedExchange(&JNETTrial,result);
    message(result==JC_ALLOC_OK ? "[JC] native Dude constructor and deleting destructor returned; no visible traveler or networking added" :
            "[JC] native allocation rehearsal did not complete validation; restart Journey before further diagnostics");
}

static void evaluation_inspect(uintptr_t game) {
    ULONGLONG started=GetTickCount64();
    struct callback_read_budget budget={.deadline=started+1500};
    struct jc_eval_layout_result observed={.disjoint=-1,.owner_index=4};
    struct jc_eval_layout_workspace *work=NULL;
    for (unsigned i=0;i<4;++i) observed.owner[i].pending=-1;
    InterlockedExchange(&JNETEvaluation[0],2);
    int code=8,locked=0,level=-1;
    if (native_playback_mode || mirror_mode || pose_link.started || lifetime_conflict || JNETDual[0]!=1 || JNETResource!=5 ||
        jc_render_level(callback_read,&budget,game,&level) || !jc_render_trial_same_level(prepared_level,level)) goto done;
    locked=TryAcquireSRWLockExclusive(&lifetime_lock);
    if (!locked) goto done;
    uint64_t meshes[4],barn=body_contexts[0].s_initialized_life.barn;
    meshes[0]=barn+0x210; meshes[1]=barn+0x14850;
    for (unsigned i=0;i<JC_BODY_COUNT;++i) {
        struct jc_body_context *body=body_contexts+i;
        struct jc_initialized_detail detail;
        if (body->out_MeshLife[25] || body->s_initialized_life.barn!=barn) { code=8; goto done; }
        code=jc_initialized_validate_detail(&body->s_initialized_life,callback_read,&budget,
            (uintptr_t)GetModuleHandleW(NULL),game,GetCurrentThreadId(),&detail);
        if (code) { code=8; goto done; }
        meshes[i+2]=body->s_initialized_life.object+0x160;
    }
    work=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*work));
    if (!work) { code=5; goto done; }
    flight_mark(34,0);
    code=jc_eval_layout_inspect(callback_read,&budget,(uintptr_t)GetModuleHandleW(NULL),meshes,&observed,work);
    flight_mark(35,0);
done:
    if (budget.expired || GetTickCount64()>=budget.deadline) { code=9; observed.disjoint=-1; }
    if (work) HeapFree(GetProcessHeap(),0,work);
    if (locked) ReleaseSRWLockExclusive(&lifetime_lock);
    InterlockedExchange(&JNETEvaluation[1],code);
    InterlockedExchange(&JNETEvaluation[2],(LONG)observed.stage);
    InterlockedExchange(&JNETEvaluation[3],observed.disjoint);
    InterlockedExchange(&JNETEvaluation[4],(LONG)(GetTickCount64()-started));
    InterlockedExchange(&JNETEvaluation[5],(LONG)budget.reads);
    InterlockedExchange(&JNETEvaluation[6],(LONG)observed.owner_index);
    InterlockedExchange(&JNETEvaluation[7],(LONG)observed.states);
    for (unsigned i=0;i<4;++i) {
        const struct jc_eval_owner_result *r=observed.owner+i;
        const LONG row[]={(LONG)r->records,(LONG)r->bindings,(LONG)r->slots,(LONG)r->scratch_bytes,
            (LONG)r->execution_bytes,(LONG)r->output_pairs,(LONG)r->shape_mutations,(LONG)r->pending};
        for (unsigned j=0;j<8;++j) InterlockedExchange(&JNETEvaluation[8+i*8+j],row[j]);
    }
    InterlockedExchange(&JNETEvaluation[0],code ? -1 : 1);
    message(code ? "[JC] evaluator storage observation declined; no scheduler or game-memory writes" :
        "[JC] selected evaluator storage observed; scheduler disabled, graph/phase ownership still unverified");
}
static int evaluation_writable(void *opaque,uint64_t address,size_t bytes) {
    struct callback_read_budget *budget=opaque;
    uint64_t end;
    if (!jc_layout_address(address,bytes,&end)) return 0;
    while (address<end) {
        MEMORY_BASIC_INFORMATION info;
        if (GetTickCount64()>=budget->deadline) { budget->expired=1; return 0; }
        if (VirtualQuery((void *)(uintptr_t)address,&info,sizeof(info))!=sizeof(info) ||
            info.State!=MEM_COMMIT || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
            !(info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return 0;
        uint64_t next=(uintptr_t)info.BaseAddress+info.RegionSize;
        if (next<=address) return 0;
        address=next;
    }
    return 1;
}
static int traveler_chirp_read(void *context,uint64_t address,void *out,size_t bytes) {
    if(address==fault_image+0x13fbb0 && bytes==sizeof(jc_chirp_enqueue_code))
        return jc_chirp_hook_entry_read(callback_read,context,fault_image,(uintptr_t)jnet_character_shout_original,out);
    return callback_read(context,address,out,bytes);
}
static void traveler_actor_chirp_emit(void *context,const struct jc_chirp_bridge *bridge) {
    (void)context;
    ((void (*)(uintptr_t,const void *,uintptr_t,unsigned char))
        ((uintptr_t)GetModuleHandleW(NULL)+0x13fbb0))(bridge->barn,bridge->input,0,0);
    traveler_chirp_glyph_mark(bridge);
    traveler_chirp_audio_mark(bridge);
}
static struct jc_chirp_relay traveler_chirp_relay;
static int traveler_chirp_code;
static struct jc_chirp_relay traveler_native_chirp_relay;
static void traveler_native_chirp_emit(void *context,const struct jc_chirp_bridge *bridge) {
    (void)context;
    ((void (*)(uintptr_t,const void *,uintptr_t,unsigned char))
        ((uintptr_t)GetModuleHandleW(NULL)+0x13fbb0))(bridge->barn,bridge->input,0,0);
    traveler_chirp_audio_mark(bridge);
}
static struct jc_scarf_arrival traveler_native_scarf_arrival;
static void traveler_native_arrival(uint64_t actor,uint64_t generation) {
    traveler_native_scarf_arrival=(struct jc_scarf_arrival){.actor=actor,.generation=generation};
}
typedef void (*jc_native_scarf_update)(uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static jc_native_scarf_update original_native_scarf_update;
static void native_scarf_update(uintptr_t actor,float dt,uintptr_t state,uintptr_t fourth,uintptr_t fifth,uintptr_t sounds) {
    struct jc_scarf_arrival *a=&traveler_native_scarf_arrival;
    if(a->actor==actor && a->generation!=a->applied && traveler_context()) {
        int ok=jc_scarf_arrival_apply(own_read,cloth_actor_growth_write,NULL,
            (uintptr_t)GetModuleHandleW(NULL),traveler_game,actor,state,a);
        message(ok?"[JC] companion scarf arrival reset": "[JC] companion scarf arrival reset declined");
    }
    original_native_scarf_update(actor,dt,state,fourth,fifth,sounds);
}
static int traveler_stream_chirp(unsigned slot,const float position[3],struct jc_chirp_relay *relay,
        jc_chirp_emit emit,struct callback_read_budget *budget) {
    struct jc_script_peer *s=&traveler_streams[slot];
    uint64_t now=GetTickCount64(),sequence=(now-s->started)/2500;
    if(s->stream.departing || !traveler_udp[slot].ready)return 0;
    if(!traveler_network_active && sequence && sequence>s->chirp && now-s->started<5000+slot*1000) {
        struct jc_chirp_event outgoing={.participant=s->stream.peer,.epoch=s->stream.generation,
            .sequence=sequence,.sound=(unsigned)(sequence%4)+1,.radius=3,.duration=.5f};
        memcpy(outgoing.position,position,sizeof(outgoing.position));
        int code=traveler_udp_chirp_send(slot,s->stream.sample.chapter,&outgoing);
        if(code){relay->stopped=1;return code;}
        s->chirp=sequence;
    }
    struct jc_chirp_event event;
    if(!jc_chirp_inbox_take(&traveler_udp[slot].chirps,&s->stream,now,&event))return 0;
    relay->game=traveler_game;relay->participant=event.participant;relay->epoch=event.epoch;
    int code=jc_chirp_deliver_event(traveler_chirp_read,evaluation_writable,emit,budget,
        (uintptr_t)GetModuleHandleW(NULL),traveler_game,(uint32_t)JNETFrames,&event,&relay->delivery,slot);
    if(!code){
        ++relay->emitted;char line[192];
        snprintf(line,sizeof(line),"[JNET] authenticated %s chirp slot=%u generation=%llu event=%llu accepted/rejected=%u/%u",
            traveler_network_external?"Steam":"UDP",slot,(unsigned long long)event.epoch,(unsigned long long)event.sequence,
            traveler_udp[slot].chirps.accepted,traveler_udp[slot].chirps.rejected);
        message(line);
    }
    else if(code==JC_CHIRP_FULL || code==JC_CHIRP_CHANGED){++relay->dropped;}
    else relay->stopped=1;
    return code;
}
static void traveler_native_chirp_tick(void) {
    if(traveler_local_state!=2 || !traveler_local_offline())return;
    struct jc_chirp_relay *relay=&traveler_native_chirp_relay;
    float position[3];
    if(!own_read(NULL,traveler_barn+0x146f0+0x2250,position,sizeof(position)))return;
    struct callback_read_budget budget={.deadline=GetTickCount64()+20};
    unsigned emitted=relay->emitted,dropped=relay->dropped;
    uint64_t generation=(uint64_t)traveler_local_completed+1;
    if(relay->epoch!=generation)memset(relay,0,sizeof(*relay));
    if(relay->stopped)return;
    int code=traveler_stream_enabled?traveler_stream_chirp(1,position,relay,traveler_native_chirp_emit,&budget):
        jc_chirp_relay_tick_role(traveler_chirp_read,evaluation_writable,traveler_native_chirp_emit,
        &budget,(uintptr_t)GetModuleHandleW(NULL),traveler_game,(uint32_t)JNETFrames,
        UINT64_C(0x4c4f43414c4e4154),generation,position,relay,1);
    if(code || relay->emitted!=emitted || relay->dropped!=dropped) {
        char line[160];snprintf(line,sizeof(line),"[JC] native companion chirp code=%d emitted=%u dropped=%u stopped=%u symbol=%u",
            code,relay->emitted,relay->dropped,relay->stopped,traveler_local_symbol);message(line);
    }
}
static void traveler_actor_chirp_report(void) {
    char line[192];
    snprintf(line,sizeof(line),"[JC] actor chirp code=%d emitted=%u dropped=%u stopped=%u frame=%ld",
        traveler_chirp_code,traveler_chirp_relay.emitted,traveler_chirp_relay.dropped,
        traveler_chirp_relay.stopped,(long)JNETFrames);
    message(line);
}
static void traveler_actor_chirp_tick(void) {
    struct jc_chirp_relay *relay=&traveler_chirp_relay;
    if (!traveler_actor || traveler_actor->state!=JC_ACTOR_RUNTIME_ACTIVE ||
        !jc_actor_runtime_thread(traveler_actor) || !traveler_context() ||
        traveler_actor->update_frame!=(uint32_t)JNETFrames) return;
    const struct jc_actor_identity *id=&traveler_actor->identity;
    if (relay->game!=id->game || relay->participant!=id->peer || relay->epoch!=id->generation)
        {memset(relay,0,sizeof(*relay));traveler_chirp_code=0;}
    if (relay->stopped) return;
    float position[3];
    if (!own_read(NULL,id->actor+0x2250,position,sizeof(position))) return;
    struct callback_read_budget budget={.deadline=GetTickCount64()+20};
    unsigned emitted=relay->emitted,dropped=relay->dropped;
    int code=traveler_stream_enabled?traveler_stream_chirp(0,position,relay,traveler_actor_chirp_emit,&budget):
        jc_chirp_relay_tick(callback_read,evaluation_writable,traveler_actor_chirp_emit,
        &budget,id->image,id->game,(uint32_t)JNETFrames,id->peer,id->generation,position,relay);
    traveler_chirp_code=code;
    if (code || relay->emitted!=emitted || relay->dropped!=dropped) traveler_actor_chirp_report();
}
typedef uintptr_t (*jc_cloth_update_fn)(uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t,
    uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static jc_cloth_update_fn original_cloth_update;
static int companion_cloth_write(void *opaque,uint64_t address,const void *value,size_t size) {
    struct callback_read_budget *budget=opaque;
    if (budget->expired || GetTickCount64()>=budget->deadline || !size || size>64 ||
        !evaluation_writable(opaque,address,size)) return 0;
    return jc_local_windows_write(address,value,size);
}
static int companion_cloth_atlas(struct callback_read_budget *budget,uintptr_t image,uint64_t dude,uint64_t barn) {
    const char *names[2][3]; uint32_t outfit; uint64_t manager,again,type,vtable,entries[2][3];
    unsigned char local_before[2][3][0x28],local_after[0x28];
    InterlockedExchange(&JNETCompanionFeatures[11],30);
    if (dude!=native_cloth_lease.dude || barn!=native_cloth_lease.barn ||
        native_cloth_lease.state!=JC_CLOTH_LIVE ||
        !callback_read(budget,dude+0x70,&outfit,4) || !jc_cloth_atlas_names(outfit,names) ||
        !callback_read(budget,native_cloth_lease.game+0x28,&manager,8) || !manager ||
        !callback_read(budget,image+0x403f050,&type,8) || !type ||
        !callback_read(budget,type,&vtable,8) || vtable!=image+0x5cd4c8) return 0;
    uintptr_t (*lookup)(uintptr_t,uintptr_t,const char *)=(void *)(image+0x2ce120);
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<3;++j) {
        InterlockedExchange(&JNETCompanionFeatures[11],40+(LONG)(i*3+j));
        if (!evaluation_writable(budget,barn+0x666d68+(4+i)*0x630+j*0x28,0x28) ||
            !callback_read(budget,barn+0x666d68+(2+i)*0x630+j*0x28,local_before[i][j],0x28)) return 0;
        entries[i][j]=lookup((uintptr_t)manager,(uintptr_t)type,names[i][j]);
        if (!entries[i][j]) return 0;
    }
    void (*resolve)(uintptr_t,const char *,uintptr_t)=(void *)(image+0x8ee60);
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<3;++j) {
        uintptr_t holder=(uintptr_t)barn+0x666d68+(4+i)*0x630+j*0x28;
        resolve((uintptr_t)manager,names[i][j],holder);
        uint64_t texture;
        int code=jc_cloth_atlas_loaded(callback_read,budget,holder,entries[i][j],i==1,&texture);
        InterlockedExchange(&JNETCompanionFeatures[11],100+(LONG)(i*3+j)*10+code);
        if (code || (i==1 && !evaluation_writable(budget,texture+0xa,2))) return 0;
    }
    void (*select)(uintptr_t,unsigned,uintptr_t,const char *,const char *,const char *)=(void *)(image+0x8ab10);
    for (unsigned i=0;i<2;++i) select((uintptr_t)barn,4+i,(uintptr_t)manager,names[i][0],names[i][1],names[i][2]);
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<3;++j) {
        int code=jc_cloth_atlas_loaded(callback_read,budget,barn+0x666d68+(4+i)*0x630+j*0x28,entries[i][j],i==1,NULL);
        InterlockedExchange(&JNETCompanionFeatures[11],200+(LONG)(i*3+j)*10+code);
        if (code || lookup((uintptr_t)manager,(uintptr_t)type,names[i][j])!=entries[i][j] ||
            !callback_read(budget,barn+0x666d68+(2+i)*0x630+j*0x28,local_after,0x28) ||
            memcmp(local_before[i][j],local_after,0x28)) return 0;
    }
    InterlockedExchange(&JNETCompanionFeatures[11],31);
    return callback_read(budget,native_cloth_lease.game+0x28,&again,8) && again==manager;
}
static int companion_cloth_call(void *opaque,unsigned op,uint64_t a,uint64_t b,uint64_t c) {
    struct callback_read_budget *budget=opaque;
    if (op!=JC_CLOTH_RELEASE) InterlockedExchange(&JNETCompanionFeatures[11],1);
    if (budget->expired || GetTickCount64()>=budget->deadline) return 0;
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    flight_mark(45,(LONG)op);
    if (op==JC_CLOTH_RELEASE) ((void (*)(uintptr_t))(image+JC_RVA_ClothRelease))((uintptr_t)a);
    else if (op==JC_CLOTH_INIT) ((void (*)(uintptr_t,uintptr_t))(image+JC_RVA_DudeClothInit))((uintptr_t)a,(uintptr_t)b);
    else if (op==JC_CLOTH_ATLAS) {
        if (!companion_cloth_atlas(budget,image,a,b)) return 0;
    }
    else if (op==JC_CLOTH_APPEARANCE) {
        struct jc_inspection inspection={.budget=budget};
        InterlockedExchange(&JNETCompanionFeatures[11],2);
        uint64_t environment[2],again[2];
        if (!callback_read(budget,b+0x646d30,environment,sizeof(environment)) || !environment[0] || !environment[1]) return 0;
        const uintptr_t materials[]={(uintptr_t)a+0x1e0,(uintptr_t)b+0x666ee8+4*0x630,(uintptr_t)b+0x666ee8+5*0x630,
            (uintptr_t)native_cloth_lease.players+0x290,(uintptr_t)native_cloth_lease.players+0x148d0,
            (uintptr_t)b+0x666ee8+2*0x630,(uintptr_t)b+0x666ee8+3*0x630,
            (uintptr_t)body_contexts[1].s_initialized_life.object+0x1e0};
        struct jc_shader_inputs inputs[8],held,after;
        for (unsigned i=0;i<8;++i) {
            InterlockedExchange(&JNETCompanionFeatures[11],10+(LONG)i);
            int storage_code=i ? jc_shader_storage_read(inspection_read,&inspection,materials[i],&inputs[i]) :
                jc_shader_inputs_read(inspection_read,&inspection,materials[i],&inputs[i]);
            InterlockedExchange(&JNETCompanionFeatures[12],storage_code);
            if (storage_code) return 0;
            if (i<3 && !evaluation_writable(budget,inputs[i].buffer,inputs[i].bytes)) {
                InterlockedExchange(&JNETCompanionFeatures[12],16); return 0;
            }
        }
        InterlockedExchange(&JNETCompanionFeatures[11],20);
        held=inputs[0];
        if ((held.found&7)!=7 || (held.finite&7)!=7) return 0;
        InterlockedExchange(&JNETCompanionFeatures[11],21);
        for (unsigned i=0;i<3;++i) for (unsigned j=0;j<8;++j)
            if (i!=j && inputs[i].buffer<inputs[j].buffer+inputs[j].bytes &&
                inputs[j].buffer<inputs[i].buffer+inputs[i].bytes) return 0;
        ((void (*)(uintptr_t,uintptr_t))(image+JC_RVA_DudeClothAppearance))((uintptr_t)a,(uintptr_t)b);
        InterlockedExchange(&JNETCompanionFeatures[11],22);
        struct callback_read_budget restore_budget={.deadline=GetTickCount64()+500};
        if (jc_shader_inputs_read(callback_read,&restore_budget,materials[0],&after) ||
            after.buffer!=held.buffer || after.payload!=held.payload || after.instance!=held.instance ||
            after.shader!=held.shader || after.bytes!=held.bytes ||
            memcmp(after.offsets,held.offsets,3*sizeof(*held.offsets)) ||
            memcmp(after.sizes,held.sizes,3*sizeof(*held.sizes))) return 0;
        InterlockedExchange(&JNETCompanionFeatures[11],23);
        const unsigned sizes[]={4,16,64};
        for (unsigned i=0;i<3;++i)
            if (!companion_cloth_write(&restore_budget,held.buffer+held.offsets[i],held.values[i],sizes[i])) return 0;
        void (*set_texture)(uintptr_t,const char *,uintptr_t)=(void *)(image+0x273640);
        for (unsigned i=0;i<3;++i) set_texture(materials[i],(const char *)(image+0x5cf7f0),(uintptr_t)environment[0]);
        InterlockedExchange(&JNETCompanionFeatures[11],24);
        if (!callback_read(&restore_budget,b+0x646d30,again,sizeof(again)) || memcmp(environment,again,sizeof(environment)) ||
            jc_shader_inputs_read(callback_read,&restore_budget,materials[0],&after) ||
            after.buffer!=held.buffer || after.payload!=held.payload || after.instance!=held.instance ||
            after.shader!=held.shader || after.bytes!=held.bytes ||
            memcmp(after.offsets,held.offsets,3*sizeof(*held.offsets)) ||
            memcmp(after.sizes,held.sizes,3*sizeof(*held.sizes))) return 0;
        for (unsigned i=0;i<3;++i) if (memcmp(after.values[i],held.values[i],sizes[i])) return 0;
    }
    else if (op==JC_CLOTH_ATTACH || op==JC_CLOTH_COLLIDE)
        ((void (*)(uintptr_t,uintptr_t,uintptr_t))(image+(op==JC_CLOTH_ATTACH ? JC_RVA_DudeClothAttach : JC_RVA_DudeClothCollide)))((uintptr_t)a,(uintptr_t)b,(uintptr_t)c);
    else return 0;
    if (op!=JC_CLOTH_RELEASE) InterlockedExchange(&JNETCompanionFeatures[11],0);
    flight_mark(46,(LONG)op); return 1;
}
static void companion_cloth_publish(int code) {
    int state=native_cloth_lease.state==JC_CLOTH_QUARANTINED ? -2 :
        native_cloth_lease.state==JC_CLOTH_RETIRED ? 3 : code ? -1 :
        native_cloth_lease.state==JC_CLOTH_LIVE ? 2 : 1;
    InterlockedExchange(&JNETCompanionFeatures[0],state);
    InterlockedExchange(&JNETCompanionFeatures[1],code);
    InterlockedExchange(&JNETCompanionFeatures[2],(LONG)native_cloth_lease.stage);
    InterlockedExchange(&JNETCompanionFeatures[3],(LONG)native_cloth_lease.updates);
    InterlockedExchange(&JNETCompanionFeatures[4],native_cloth_lease.state==JC_CLOTH_RETIRED);
    InterlockedExchange(&JNETCompanionFeatures[5],(LONG)native_cloth_lease.native_calls);
    InterlockedExchange(&JNETCompanionFeatures[6],(LONG)native_cloth_lease.free_slots);
    InterlockedExchange(&JNETCompanionFeatures[7],(LONG)native_cloth_lease.capture_detail);
    InterlockedExchange(&JNETCompanionFeatures[10],(LONG)native_cloth_lease.failure_stage);
}
static int companion_cloth_finish(uintptr_t barn) {
    unsigned char pending=0;
    if (!own_read(NULL,barn+0x6bcf84,&pending,1) || pending>1) return 13;
    if (pending) {
        _Alignas(16) unsigned char completed[16]={0};
        uintptr_t (*finish)(uintptr_t,void *,unsigned,uintptr_t)=
            (void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_ClothFinish);
        do {
            flight_mark(47,1);
            if (finish(barn,completed,1,0)!=(uintptr_t)completed) return 13;
            flight_mark(48,1);
        } while (!completed[0]);
    }
    return 0;
}
static void companion_cloth_cleanup_locked(uintptr_t dude) {
    if (native_cloth_lease.state!=JC_CLOTH_LIVE || dude!=native_cloth_lease.players+0xb0) return;
    uint64_t game=(uintptr_t)JNETGame,barn=0,players=0;
    if (GetCurrentThreadId()!=resource_thread || game!=native_cloth_lease.game ||
        !own_read(NULL,game+0x108,&barn,8) || barn!=native_cloth_lease.barn ||
        !own_read(NULL,game+0x1e0,&players,8) || players!=native_cloth_lease.players) {
        companion_cloth_publish(11); return;
    }
    int code=companion_cloth_finish((uintptr_t)barn);
    if (!code) {
        struct callback_read_budget budget={.deadline=GetTickCount64()+500,.fast=JNETCompanionFeatures[8]==1};
        code=jc_companion_cloth_retire(callback_read,companion_cloth_write,companion_cloth_call,&budget,
            game,1,1,&native_cloth_lease);
    }
    native_cloth_disabled=1; companion_cloth_publish(code);
}
static void companion_cloth_before(uintptr_t barn) {
    int live=native_cloth_lease.state==JC_CLOTH_LIVE;
    int want=native_playback_mode==1 && !native_playback_room && !native_cloth_disabled &&
        JNETNativePlayback[4]>0;
    if (!live && !want) return;
    uintptr_t game=(uintptr_t)JNETGame,image=(uintptr_t)GetModuleHandleW(NULL);
    uint64_t actual=0;
    if (GetCurrentThreadId()!=resource_thread || game!=resource_game ||
        !own_read(NULL,game+0x108,&actual,8) || actual!=barn ||
        !TryAcquireSRWLockExclusive(&lifetime_lock)) { companion_cloth_publish(11); return; }
    struct jc_body_context *saved=body_context; body_context=body_contexts;
    struct callback_read_budget budget={.deadline=GetTickCount64()+500,.fast=JNETCompanionFeatures[8]==1};
    struct jc_initialized_detail detail;
    int code=0,level=-1;
    uint64_t definition=0,mesh_inner=0;
    /* Match the original ClothBarn preamble: finish its existing pass-1 job before
       touching descriptors, including when a native remote has just arrived. */
    code=companion_cloth_finish(barn);
    if (code) goto done;
    budget.deadline=GetTickCount64()+500;
    if (lifetime_conflict || initialized_life.state!=1 || JNETDual[0]!=1 ||
        jc_render_level(callback_read,&budget,game,&level) || !jc_render_trial_same_level(prepared_level,level)) want=0;
    if (want) {
        code=jc_initialized_validate_detail(&initialized_life,callback_read,&budget,image,game,GetCurrentThreadId(),&detail);
        if (code) { code=14; want=0; }
        if (want && (!callback_read(&budget,initialized_life.object+0x168,&mesh_inner,8) || !mesh_inner ||
            !callback_read(&budget,mesh_inner+0x10,&mesh_inner,8) || !mesh_inner)) { code=12; want=0; }
        if (want) {
            uintptr_t (*lookup)(uintptr_t,const char *)=(void *)(image+JC_RVA_MeshClothDefinition);
            definition=lookup((uintptr_t)mesh_inner,(const char *)(image+0x5ceba0));
            if (!definition) { code=12; want=0; }
        }
    }
    if (want || live) {
        int step=jc_companion_cloth_step(callback_read,companion_cloth_write,companion_cloth_call,&budget,
            image,game,initialized_life.object,definition,1,1,want,&native_cloth_lease);
        if (step) code=step;
    }
done:
    if (code && code!=JC_CLOTH_BUSY) native_cloth_disabled=1;
    companion_cloth_publish(code);
    body_context=saved; ReleaseSRWLockExclusive(&lifetime_lock);
}
static uintptr_t companion_cloth_update(uintptr_t barn,float dt,uintptr_t a3,uintptr_t a4,
        uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,
        uintptr_t a11,uintptr_t a12,uintptr_t a13,uintptr_t a14) {
    companion_cloth_before(barn);
    DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
    cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=0;
    uintptr_t result=original_cloth_update(barn,dt,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14);
    cloth_production_thread=old_thread; cloth_production_pass=old_pass;
    return result;
}
static ULONGLONG native_playback_resume,native_follow_resume;
static unsigned native_follow_cursor;
static int native_playback_cursor=-1;
static ULONGLONG native_playback_duration(void) { return native_playback_room ? 180000 : 60000; }
static void native_playback_elapsed(void) {
    ULONGLONG now=GetTickCount64();
    ULONGLONG elapsed=native_playback_begin && now>native_playback_begin ? now-native_playback_begin : 0;
    InterlockedExchange(&native_report[6],elapsed>native_playback_duration() ? (LONG)native_playback_duration() : (LONG)elapsed);
}
static void native_playback_end(int state,int code) {
    struct jc_body_context *selected=body_context;
    native_playback_mode=2;
    for (unsigned i=0;i<JC_BODY_COUNT;++i) {
        body_context=body_contexts+i; native_playback_elapsed();
        if (body_context==selected) {
            InterlockedExchange(&native_report[1],code);
            InterlockedExchange(&native_report[0],state);
            if (code) preview_fail(27);
        } else if (native_report[0]>=0) InterlockedExchange(&native_report[0],code ? 2 : state);
        if (JNETMeshLife[25]>0) InterlockedExchange(&JNETMeshLife[25],3);
        body_publish();
        char line[480];
        snprintf(line,sizeof(line),"[JC] native body=%u state=%ld result=%ld stage=%ld detail=%ld accepted=%ld changed=%ld submissions=%ld elapsed=%ld reads=%ld",
            i+1,(long)native_report[0],(long)native_report[1],(long)native_report[2],(long)native_report[3],
            (long)native_report[4],(long)native_report[5],(long)native_report[16],(long)native_report[6],(long)native_report[9]);
        message(line);
        volatile LONG *o=JNETNativeOutputs+i*8;
        snprintf(line,sizeof(line),"[JC] native output body=%u items=%lu bindings=%lu index=%lu binding=%lu kind=%lu type_rva=%08lx bytes=%lu owners=%lu",
            i+1,(unsigned long)(uint32_t)o[0],(unsigned long)(uint32_t)o[1],(unsigned long)(uint32_t)o[2],
            (unsigned long)(uint32_t)o[3],(unsigned long)(uint32_t)o[4],(unsigned long)(uint32_t)o[5],
            (unsigned long)(uint32_t)o[6],(unsigned long)(uint32_t)o[7]);
        message(line);
    }
    body_context=selected;
    if (JNETCompanionFeatures[16]>0) InterlockedExchange(&JNETCompanionFeatures[16],3);
    if (native_playback_room) { pose_link_depart(); pose_link_close(0); }
}
static void native_playback_guard_report(const struct jc_eval_run_result *run) {
    InterlockedExchange(&native_report[2],(LONG)run->stage);
    InterlockedExchange(&native_report[3],(LONG)run->detail);
    const unsigned values[]={run->nodes,run->bindings,run->clocks,run->scheduler_kind,run->scratch_bytes,run->temp_bytes};
    for (unsigned i=0;i<6;++i) InterlockedExchange(&native_report[10+i],(LONG)values[i]);
    InterlockedExchange(&native_report[19],run->ready);
    for (unsigned i=0;i<8;++i)
        InterlockedExchange(&JNETNativeOutputs[body_context->index*8+i],(LONG)run->output[i]);
}
static void native_playback_body(uintptr_t game) {
    if (native_playback_mode!=1) return;
    native_playback_elapsed();
    if (JNETMeshLife[25]<0 || JNETDual[0]!=1) { native_playback_end(-1,20); return; }
    if (GetTickCount64()>=native_playback_begin+native_playback_duration()) { native_playback_end(3,0); return; }
    if (JNETMeshLife[25]==3) { native_playback_end(3,0); return; }
    if (GetTickCount64()<body_context->native_next || GetTickCount64()<native_playback_begin || !preview_queue) return;
    const struct jc_pose_receiver *peer=native_playback_room ? &pose_link.room.peers[body_context->index] : NULL;
    if (peer && (!jc_pose_fresh(peer,(uint32_t)prepared_level,GetTickCount64()) ||
        (peer->sample.sequence==body_context->native_sequence &&
         !memcmp(peer->sample.identity,body_context->native_identity,16)))) return;
    if (peer) {
        unsigned at=30+body_context->index*5;
        InterlockedExchange(&JNETCompanionFeatures[at],(peer->sample.flags&JC_POSE_APPEARANCE)!=0);
        for (unsigned j=0;j<4;++j) InterlockedExchange(&JNETCompanionFeatures[at+1+j],(LONG)peer->sample.appearance[j]);
    }
    if (!TryAcquireSRWLockExclusive(&lifetime_lock)) { native_playback_end(-1,20); return; }
    ULONGLONG started=GetTickCount64(); body_context->native_next=started+100;
    struct jc_read_batch read_batch;
    jc_read_batch_reset(&read_batch);
    struct callback_read_budget budget={.deadline=started+1500};
    struct jc_eval_run_result run={0};
    struct jc_eval_layout_result layout={0};
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    uint64_t barn=initialized_life.barn,mesh=initialized_life.object+0x160;
    unsigned target_owner=2+body_context->index,sibling_owner=3-body_context->index;
    const uint64_t meshes[]={barn+0x210,barn+0x14850,
        body_contexts[0].s_initialized_life.object+0x160,body_contexts[1].s_initialized_life.object+0x160};
    const uint64_t others[]={meshes[0],meshes[1],meshes[sibling_owner]};
    struct jc_initialized_detail life_detail;
    struct jc_idle_pose inputs; struct jc_idle_detail input_detail;
    struct jc_anim_sample sample;
    struct jc_anim_bind_detail binding={0};
    float transform[16],original_world[3][16],before[3][JC_SKEL_CAP][16],after[JC_SKEL_CAP][16];
    uint32_t count=0,refs=0; int level=-1,code=20,written=0,deferred=0;
    InterlockedExchange(&native_report[2],100); InterlockedExchange(&native_report[3],0);
    if (lifetime_conflict || initialized_life.state!=1 || JNETResource!=5 ||
        jc_render_level(callback_read,&budget,game,&level) || !jc_render_trial_same_level(prepared_level,level) ||
        jc_initialized_validate_detail(&initialized_life,callback_read,&budget,image,game,GetCurrentThreadId(),&life_detail) ||
        jc_initialized_validate_detail(&body_contexts[1-body_context->index].s_initialized_life,callback_read,&budget,image,game,GetCurrentThreadId(),&life_detail)) goto done;
    memcpy(&count,preview_queue+0x10080,4);
    code=23;
    if (count || !jc_render_preview_queue(preview_queue,preview_tracking.ledger.model,preview_tracking.ledger.control)) goto done;
    budget.batch=&read_batch;
    code=jc_eval_layout_inspect(callback_read,&budget,image,meshes,&layout,native_playback_work);
    budget.batch=NULL;
    run.stage=layout.stage; run.detail=layout.owner_index;
    native_playback_guard_report(&run);
    if (code) goto done;
    code=jc_mesh_pose_ready_detail(callback_read,&budget,image,mesh,&inputs,&input_detail,body_context->motion_active);
    if (code) { code=21; goto done; }
    run.mesh=mesh; run.model=inputs.model; run.control=inputs.control;
    if (!callback_read(&budget,native_playback_work->skeleton[target_owner].storage.clone.payload+0x18,&run.job,8) ||
        !run.job || inputs.model!=native_playback_work->skeleton[target_owner].storage.model || inputs.model!=preview_tracking.ledger.model ||
        inputs.control!=preview_tracking.ledger.control) { code=21; goto done; }
    InterlockedExchange(&native_report[2],101); InterlockedExchange(&native_report[3],0);
    if (peer) {
        sample=peer->sample.animation; memcpy(transform,peer->sample.transform,64);
        if (!sample.total || !jc_anim_valid(&sample)) { code=22; goto done; }
    } else {
        code=jc_anim_capture(callback_read,&budget,image,meshes[0],&native_playback_source,&sample);
        if (code || !sample.count) { code=22; goto done; }
        if (!callback_read(&budget,meshes[0]+0x530,transform,64)) { code=21; goto done; }
        transform[12]+=body_context->index ? -3.0f : 3.0f;
    }
    InterlockedExchange(&native_report[17],(LONG)sample.count);
    if (!jc_pose_transform_valid(transform)) { code=21; goto done; }
    for (unsigned i=0;i<3;++i) {
        unsigned owner=i==2 ? sibling_owner : i;
        if (!callback_read(&budget,others[i]+0x530,original_world[i],64) ||
            jc_skel_matrices(callback_read,&budget,native_playback_work->skeleton+owner,before[i])) { code=21; goto done; }
    }
    InterlockedExchange(&native_report[2],102); InterlockedExchange(&native_report[3],0);
    code=jc_anim_apply(callback_read,trial_animation_write,&budget,image,mesh,others,&body_context->motion_layout,&sample,&written);
    if (code) { code=22; goto done; }
    body_context->motion_active=1;
    if (budget.expired || GetTickCount64()>=budget.deadline) { code=9; goto done; }
    memcpy((void *)(mesh+0x530),transform,64);
    void (*mesh_update)(uintptr_t,void *)=(void *)(image+JC_RVA_MeshUpdate);
    InterlockedExchange(&native_report[2],103); InterlockedExchange(&native_report[3],0); flight_mark(36,(LONG)body_context->index+1);
    int binding_code=jc_anim_bind_arm(callback_read,trial_stamp_write,&budget,image,meshes,target_owner,
        inputs.model,inputs.control,&binding);
    if (binding.changed) InterlockedIncrement(&native_report[21]);
    if (binding_code) {
        InterlockedExchange(&native_report[3],(LONG)binding.field);
        InterlockedExchange(&native_report[20],(LONG)binding.row);
        InterlockedExchange(&native_report[22],binding_code); code=23; goto done;
    }
    mesh_update(mesh,preview_queue);
    flight_mark(37,(LONG)body_context->index+1);
    memcpy(&count,preview_queue+0x10080,4);
    if (count!=1) { InterlockedExchange(&native_report[3],101); code=23; goto done; }
    if (!jc_idle_context_valid(preview_queue,&inputs)) { InterlockedExchange(&native_report[3],102); code=23; goto done; }
    binding_code=jc_anim_bind_complete(own_read,NULL,image,mesh,&binding);
    if (binding_code || !jc_anim_consumed_detail(own_read,NULL,&body_context->motion_layout,&sample,&binding)) {
        InterlockedExchange(&native_report[3],(LONG)binding.field);
        InterlockedExchange(&native_report[20],(LONG)binding.row);
        InterlockedExchange(&native_report[22],binding_code); code=23; goto done;
    }
    if (!own_read(NULL,inputs.control,&refs,4)) { InterlockedExchange(&native_report[3],103); code=23; goto done; }
    if (refs!=inputs.references) { InterlockedExchange(&native_report[3],104); code=23; goto done; }
    InterlockedExchange(&native_report[20],0); InterlockedExchange(&native_report[22],0);
    struct jc_eval_run_result rebound={0};
    jc_read_batch_reset(&read_batch); budget.batch=&read_batch;
    code=jc_eval_run_prepare(callback_read,evaluation_writable,&budget,image,meshes,target_owner,&rebound,native_playback_work);
    budget.batch=NULL;
    native_playback_guard_report(&rebound);
    if (code==JC_EVAL_RUN_WAIT && rebound.stage==2 && rebound.detail==6) {
        /* Binding left one validated borrowed pair; no evaluator was submitted.
           Keep the body available and retry from fresh inputs next update. */
        memset(preview_queue+0x10080,0,4);
        InterlockedExchange(&native_report[1],code);
        deferred=1; goto done;
    }
    if (code || !rebound.ready) { if (!code) code=15; goto done; }
    if (run.mesh!=rebound.mesh || run.job!=rebound.job || run.model!=rebound.model || run.control!=rebound.control ||
        budget.expired || GetTickCount64()>=budget.deadline) { code=3; goto done; }
    _Alignas(16) unsigned char evaluation[0x310]={0};
    void (*submit)(void *,uintptr_t)=(void *)(image+JC_RVA_AnimationSubmit);
    void (*finish)(void *)=(void *)(image+JC_RVA_AnimationFinish);
    void (*release)(void *)=(void *)(image+JC_RVA_ReferenceRelease);
    InterlockedExchange(&native_report[2],104); InterlockedExchange(&native_report[3],0); flight_mark(38,(LONG)body_context->index+1);
    InterlockedIncrement(&native_report[16]);
    submit(evaluation,mesh);
    uint32_t submitted=0; memcpy(&submitted,evaluation+0x308,4);
    /* Complete the native wait before any report check can return or stop this mode. */
    InterlockedExchange(&native_report[2],105); InterlockedExchange(&native_report[3],0); flight_mark(39,(LONG)body_context->index+1);
    finish(evaluation);
    flight_mark(40,(LONG)body_context->index+1);
    uint64_t pair[2]; memcpy(pair,evaluation+0x108,sizeof(pair));
    memcpy(&count,evaluation+0x308,4);
    int context_ok=submitted==1 && count==0 && pair[0]==run.model && pair[1]==run.control;
    for (unsigned i=1;i<32;++i) {
        uint64_t other[2]; memcpy(other,evaluation+0x108+i*16,sizeof(other));
        if (other[0] || other[1]) context_ok=0;
    }
    /* Unknown pairs are quarantined; never release through a changed native pointer. */
    if (pair[0]==run.model && pair[1]==run.control) release(evaluation+0x108);
    if (!context_ok) { code=24; goto done; }
    uint64_t pending=1,execution=0,temporary=1;
    if (!own_read(NULL,run.job+0x70,&pending,8) || pending ||
        !own_read(NULL,run.job+0x68,&execution,8) || !execution ||
        !own_read(NULL,execution+8,&temporary,8) || temporary ||
        !own_read(NULL,inputs.control,&refs,4) || refs!=inputs.references) { code=25; goto done; }
    /* Only our queue's count resets; its one validated held reference remains. */
    memset(preview_queue+0x10080,0,4);
    InterlockedExchange(&native_report[2],106); InterlockedExchange(&native_report[3],0); flight_mark(41,(LONG)body_context->index+1);
    mesh_update(mesh,preview_queue);
    flight_mark(42,(LONG)body_context->index+1);
    memcpy(&count,preview_queue+0x10080,4);
    if (count!=1 || !jc_idle_context_valid(preview_queue,&inputs) ||
        !own_read(NULL,inputs.control,&refs,4) || refs!=inputs.references ||
        jc_render_track_assignment(&preview_tracking,(uintptr_t)preview_queue,1,0,
            inputs.model,inputs.control,inputs.model,inputs.control)) { code=23; goto done; }
    InterlockedExchange(&native_report[2],107); InterlockedExchange(&native_report[3],0);
    code=trial_shader_prepare(inputs.model,inputs.root,1,transform);
    if (code) { code=25; goto done; }
    InterlockedExchange(&native_report[2],108); InterlockedExchange(&native_report[3],0);
    budget.deadline=GetTickCount64()+500;
    for (unsigned i=0;i<3;++i) {
        float world[16]; unsigned owner=i==2 ? sibling_owner : i;
        if (!callback_read(&budget,others[i]+0x530,world,64) || memcmp(world,original_world[i],64) ||
            jc_skel_live(callback_read,&budget,image,native_playback_work->skeleton+owner) ||
            jc_skel_matrices(callback_read,&budget,native_playback_work->skeleton+owner,after) ||
            memcmp(after,before[i],native_playback_work->skeleton[owner].names*64)) { code=25; goto done; }
    }
    if (jc_skel_live(callback_read,&budget,image,native_playback_work->skeleton+target_owner) ||
        jc_skel_matrices(callback_read,&budget,native_playback_work->skeleton+target_owner,after)) { code=25; goto done; }
    int changed=0;
    const struct jc_skel_layout *target=native_playback_work->skeleton+target_owner;
    for (unsigned i=0;i<target->names;++i) if (body_context->native_seen && target->bones[i].bone!=target->storage.root &&
        memcmp(body_context->native_previous[i],after[i],64)) changed=1;
    memcpy(body_context->native_previous,after,target->names*64); body_context->native_seen=1;
    if (changed) InterlockedIncrement(&native_report[5]);
    memcpy(preview_applied_transform,transform,64); body_context->applied_animation=sample;
    memset(preview_queue+0x10080,0,4);
    InterlockedIncrement(&native_report[4]);
    if (peer) {
        body_context->native_sequence=peer->sample.sequence;
        memcpy(body_context->native_identity,peer->sample.identity,16);
        preview_pose_sequence=peer->sample.sequence; memcpy(preview_pose_identity,peer->sample.identity,16);
        pose_link_applied(body_context->index);
    }
    InterlockedExchange(&native_report[2],0); InterlockedExchange(&native_report[3],0); code=0;
done:
    if (budget.expired) code=9;
    LONG elapsed=(LONG)(GetTickCount64()-started);
    InterlockedExchange(&native_report[7],elapsed);
    if (elapsed>native_report[8]) InterlockedExchange(&native_report[8],elapsed);
    InterlockedExchange(&native_report[9],(LONG)budget.reads);
    InterlockedExchange(&native_report[18],code==25 ? code : 0);
    if (code && !(deferred && code==JC_EVAL_RUN_WAIT)) native_playback_end(-1,code);
    if (!code) InterlockedExchange(&native_report[1],0);
    body_publish();
    ReleaseSRWLockExclusive(&lifetime_lock);
}
static void native_follow_body(uintptr_t game) {
    if (!body_context->native_seen || !preview_queue || native_playback_mode!=1) return;
    if (!TryAcquireSRWLockExclusive(&lifetime_lock)) return;
    ULONGLONG started=GetTickCount64();
    struct callback_read_budget budget={.deadline=started+250};
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    uint64_t barn=initialized_life.barn,mesh=initialized_life.object+0x160;
    const uint64_t meshes[]={barn+0x210,barn+0x14850,
        body_contexts[0].s_initialized_life.object+0x160,body_contexts[1].s_initialized_life.object+0x160};
    unsigned owner=2+body_context->index;
    struct jc_initialized_detail detail; struct jc_idle_pose inputs; struct jc_idle_detail idle;
    float transform[16],world[4][16],before[4][JC_SKEL_CAP][16],after[JC_SKEL_CAP][16],root[16];
    uint32_t count=0,refs=0; uint64_t job=0,pending=1;
    int level=-1,code=20;
    if (lifetime_conflict || initialized_life.state!=1 || JNETResource!=5 || JNETDual[0]!=1 ||
        JNETMeshLife[25]<0 ||
        jc_render_level(callback_read,&budget,game,&level) || !jc_render_trial_same_level(prepared_level,level) ||
        jc_initialized_validate_detail(&initialized_life,callback_read,&budget,image,game,GetCurrentThreadId(),&detail) ||
        jc_initialized_validate_detail(&body_contexts[1-body_context->index].s_initialized_life,
            callback_read,&budget,image,game,GetCurrentThreadId(),&detail)) goto done;
    code=21;
    if (native_playback_room) {
        const struct jc_pose_receiver *peer=&pose_link.room.peers[body_context->index];
        if (!jc_pose_fresh(peer,(uint32_t)prepared_level,GetTickCount64()) ||
            memcmp(peer->sample.identity,body_context->native_identity,16)) { code=0; goto done; }
        memcpy(transform,peer->sample.transform,64);
    } else {
        if (!callback_read(&budget,meshes[0]+0x530,transform,64)) goto done;
        transform[12]+=body_context->index ? -3.0f : 3.0f;
    }
    if (!jc_pose_transform_valid(transform)) goto done;
    if (!memcmp(transform,preview_applied_transform,64)) { code=0; goto done; }
    /* Revalidate the last accepted layout, not the evaluator graph. No clip times,
       request flags, frame stamps or evaluated local-player bones are copied. */
    for (unsigned i=0;i<4;++i) {
        if (jc_skel_live(callback_read,&budget,image,native_playback_work->skeleton+i) ||
            !callback_read(&budget,meshes[i]+0x530,world[i],64) ||
            (i!=owner && jc_skel_matrices(callback_read,&budget,native_playback_work->skeleton+i,before[i]))) goto done;
    }
    if (jc_mesh_pose_ready_detail(callback_read,&budget,image,mesh,&inputs,&idle,body_context->motion_active) ||
        inputs.model!=preview_tracking.ledger.model || inputs.control!=preview_tracking.ledger.control ||
        inputs.model!=native_playback_work->skeleton[owner].storage.model ||
        inputs.root!=native_playback_work->skeleton[owner].storage.root ||
        !callback_read(&budget,native_playback_work->skeleton[owner].storage.clone.payload+0x18,&job,8) || !job ||
        !callback_read(&budget,job+0x70,&pending,8) || pending) goto done;
    code=23;
    memcpy(&count,preview_queue+0x10080,4);
    if (count || !jc_render_preview_queue(preview_queue,inputs.model,inputs.control)) goto done;
    if (budget.expired || GetTickCount64()>=budget.deadline) { code=9; goto done; }
    memcpy((void *)(mesh+0x530),transform,64);
    void (*mesh_update)(uintptr_t,void *)=(void *)(image+JC_RVA_MeshUpdate);
    flight_mark(41,(LONG)body_context->index+1); mesh_update(mesh,preview_queue); flight_mark(42,(LONG)body_context->index+1);
    memcpy(&count,preview_queue+0x10080,4);
    if (count!=1 || !jc_idle_context_valid(preview_queue,&inputs) ||
        !own_read(NULL,inputs.control,&refs,4) || refs!=inputs.references ||
        jc_render_track_assignment(&preview_tracking,(uintptr_t)preview_queue,1,0,
            inputs.model,inputs.control,inputs.model,inputs.control)) goto done;
    code=25;
    if (!own_read(NULL,inputs.root+0x20,root,64) || memcmp(root,transform,64) ||
        trial_shader_prepare(inputs.model,inputs.root,1,transform)) goto done;
    budget.deadline=GetTickCount64()+250;
    for (unsigned i=0;i<4;++i) if (i!=owner) {
        float check[16];
        if (!callback_read(&budget,meshes[i]+0x530,check,64) || memcmp(check,world[i],64) ||
            jc_skel_live(callback_read,&budget,image,native_playback_work->skeleton+i) ||
            jc_skel_matrices(callback_read,&budget,native_playback_work->skeleton+i,after) ||
            memcmp(before[i],after,native_playback_work->skeleton[i].names*64)) goto done;
    }
    if (jc_skel_live(callback_read,&budget,image,native_playback_work->skeleton+owner)) goto done;
    memset(preview_queue+0x10080,0,4);
    memcpy(preview_applied_transform,transform,64);
    InterlockedIncrement(&JNETCompanionFeatures[14+body_context->index]); code=0;
done:
    if (budget.expired) code=9;
    LONG cost=(LONG)(GetTickCount64()-started); unsigned i=body_context->index;
    InterlockedExchange(&JNETCompanionFeatures[22+i],cost);
    if (cost>JNETCompanionFeatures[24+i]) InterlockedExchange(&JNETCompanionFeatures[24+i],cost);
    InterlockedExchange(&JNETCompanionFeatures[26+i],code);
    InterlockedExchange(&JNETCompanionFeatures[28+i],(LONG)budget.reads);
    ReleaseSRWLockExclusive(&lifetime_lock);
    if (code) { InterlockedExchange(&native_report[2],109); InterlockedExchange(&native_report[3],code); native_playback_end(-1,code); }
}
static void native_chirp_tick(uintptr_t game) {
    if (native_playback_room || native_playback_mode!=1 || JNETCompanionFeatures[16]<0) return;
    struct callback_read_budget budget={.deadline=GetTickCount64()+150,.fast=JNETCompanionFeatures[8]==1};
    struct jc_chirp_batch batch;
    uint64_t players=0; unsigned char gates[3];
    if (!callback_read(&budget,game+0x1e0,&players,8) ||
        !callback_read(&budget,players+0xa2,gates,sizeof(gates))) {
        InterlockedExchange(&JNETCompanionFeatures[17],JC_CHIRP_READ);
        InterlockedExchange(&JNETCompanionFeatures[16],-1); return;
    }
    if (gates[0] || gates[2]) {
        InterlockedExchange(&JNETCompanionFeatures[17],9);
        InterlockedExchange(&JNETCompanionFeatures[16],-1); return;
    }
    int code=jc_chirp_capture_local(callback_read,&budget,game,(uint32_t)JNETFrames,1,1,&native_chirp_capture,&batch);
    InterlockedExchange(&JNETCompanionFeatures[17],code);
    if (code) { InterlockedExchange(&JNETCompanionFeatures[16],-1); return; }
    InterlockedExchange(&JNETCompanionFeatures[16],1);
    InterlockedExchangeAdd(&JNETCompanionFeatures[18],(LONG)batch.count);
    InterlockedExchangeAdd(&JNETCompanionFeatures[21],(LONG)batch.dropped);
    if (!batch.count) return;
    if (!TryAcquireSRWLockExclusive(&lifetime_lock)) {
        InterlockedExchangeAdd(&JNETCompanionFeatures[21],(LONG)(batch.count*JC_BODY_COUNT)); return;
    }
    for (unsigned i=0;i<JC_BODY_COUNT && JNETCompanionFeatures[16]>=0;++i) {
        const struct jc_body_context *body=body_contexts+i;
        if (lifetime_conflict || body->s_initialized_life.state!=1 ||
            body->s_initialized_life.game!=game || body->s_initialized_life.thread!=GetCurrentThreadId() ||
            body->out_MeshLife[25]!=2 || !JNETNativePlayback[i*24+4]) {
            InterlockedExchangeAdd(&JNETCompanionFeatures[21],(LONG)batch.count); continue;
        }
        for (unsigned j=0;j<batch.count;++j) {
            struct jc_chirp_event event=batch.source[j].event;
            event.participant=i+2; event.position[0]+=i ? -3.0f : 3.0f;
            if (!jc_chirp_delivery_fresh(native_chirp_delivery+i,&event)) continue;
            struct jc_chirp_bridge bridge;
            code=jc_chirp_bridge_prepare(callback_read,evaluation_writable,&budget,
                (uintptr_t)GetModuleHandleW(NULL),game,(uint32_t)JNETFrames,&batch,j,&event,&bridge);
            if (code) {
                InterlockedExchange(&JNETCompanionFeatures[17],code);
                InterlockedExchangeAdd(&JNETCompanionFeatures[21],(LONG)(batch.count-j));
                if (code!=JC_CHIRP_FULL) InterlockedExchange(&JNETCompanionFeatures[16],-1);
                break;
            }
            void (*shout)(uintptr_t,const void *,uintptr_t,unsigned char)=
                (void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_NativeShout);
            flight_mark(43,(LONG)i+1); shout((uintptr_t)bridge.barn,bridge.input,0,0); flight_mark(44,(LONG)i+1);
            /* Enqueue is irreversible: consume this event even if its postcheck declines. */
            jc_chirp_delivery_commit(native_chirp_delivery+i,&event);
            code=jc_chirp_bridge_post(own_read,NULL,(uint32_t)JNETFrames,&bridge);
            InterlockedExchange(&JNETCompanionFeatures[17],code);
            if (code) { InterlockedExchange(&JNETCompanionFeatures[16],-1); break; }
            InterlockedIncrement(&JNETCompanionFeatures[19+i]);
            InterlockedExchange(&JNETCompanionFeatures[16],2);
        }
    }
    ReleaseSRWLockExclusive(&lifetime_lock);
}
static void native_playback_tick(uintptr_t game) {
    if (native_playback_mode!=1) return;
    ULONGLONG now=GetTickCount64();
    if (now<native_playback_resume && now<native_playback_begin+native_playback_duration()) {
        if (now>=native_follow_resume) {
            body_context=body_contexts+(native_follow_cursor++%JC_BODY_COUNT);
            ULONGLONG started=GetTickCount64(); native_follow_body(game);
            ULONGLONG ended=GetTickCount64(),cost=ended-started;
            native_follow_resume=ended+(cost>=8 ? cost*3 : 0);
            body_context=body_contexts;
        }
        native_chirp_tick(game); return;
    }
    unsigned first=native_playback_cursor>=0 ? (unsigned)native_playback_cursor : (unsigned)JNETFrames%JC_BODY_COUNT;
    native_playback_cursor=-1;
    for (unsigned n=0;n<JC_BODY_COUNT && native_playback_mode==1;++n) {
        body_context=body_contexts+(first+n)%JC_BODY_COUNT;
        ULONGLONG started=GetTickCount64();
        native_playback_body(game);
        ULONGLONG ended=GetTickCount64(),cost=ended-started;
        if (cost>=8) {
            /* Yield normal game frames; never run two costly inspections back-to-back. */
            native_playback_resume=ended+cost*3;
            InterlockedExchange(&JNETCompanionFeatures[13],(LONG)(cost*3));
            native_playback_cursor=(int)(1-body_context->index);
            break;
        }
    }
    body_context=body_contexts;
    native_chirp_tick(game);
}
static int native_playback_start(int room) {
    native_playback_work=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*native_playback_work));
    if (!native_playback_work) {
        native_playback_mode=2; InterlockedExchange(&native_report[0],-1);
        InterlockedExchange(&native_report[1],26); return 0;
    }
    native_playback_room=room; native_playback_mode=1;
    native_playback_resume=0; native_playback_cursor=-1; native_follow_resume=0; native_follow_cursor=0;
    InterlockedExchange(&JNETCompanionFeatures[8],JNETCompanionFeatures[9]==0 ? 1 : 2);
    InterlockedExchange(&JNETCompanionFeatures[0],room ? 0 : 1);
    InterlockedExchange(&JNETCompanionFeatures[16],room ? 0 : 1);
    native_playback_begin=GetTickCount64()+(room ? 0 : 3000);
    for (unsigned i=0;i<JC_BODY_COUNT;++i) {
        body_context=body_contexts+i; body_context->native_next=native_playback_begin;
        preview_not_before=native_playback_begin; preview_deadline=preview_not_before+native_playback_duration();
        InterlockedExchange(&native_report[0],1); InterlockedExchange(&native_report[23],room);
        InterlockedExchange(&JNETMeshLife[25],1); body_publish();
    }
    body_context=body_contexts; return 1;
}
static void jnet_update_cost(unsigned index,ULONGLONG started) {
    if(JNETActorTrial[0]!=1)return;
    LONG cost=(LONG)(GetTickCount64()-started);JNETSteamGame[index]=cost;
    if(cost>JNETSteamGame[index+1])JNETSteamGame[index+1]=cost;
}
static int64_t update(uintptr_t game, float dt) {
    ULONGLONG update_started=GetTickCount64();
    if ((JNETTraveler[0] || JNETTraveler[19]) && JNETCompanionFeatures[9]==0)
        InterlockedExchange(&JNETCompanionFeatures[8],1);
    if (!traveler_frame_wait()) return traveler_last_update;
    jnet_update_cost(41,update_started);
    cloth_chapter_tick(game);
    InterlockedExchangePointer(&JNETGame, (void *)game);
    InterlockedIncrement(&JNETFrames);
    InterlockedExchange(&JNETPhase, 1);
    if (JNETObservation==4 || JNETObservation==5) {
        if (JNETObservation==5) lifecycle_boundary(game,(unsigned)JNETFrames,0);
        int64_t result=original_update(game,dt);
        if (JNETObservation==5) lifecycle_boundary(game,(unsigned)JNETFrames,1);
        InterlockedExchange(&JNETPhase,2);
        InterlockedExchange(&JNETStatus,1);
        return result;
    }
    if (JNETRender[0]>0 && JNETRender[0]<9 &&
        (game!=render_game || GetCurrentThreadId()!=render_thread)) render_fail(3);
    if (JNETCommand==10 && !JNETMeshLife[106]) {
        if (boundary_checkpoint("[JC] Show Trial pending before original game update"))
            InterlockedExchange(&JNETMeshLife[106],1);
        else { InterlockedCompareExchange(&JNETCommand,0,10); preview_fail(15); }
    }
    flight_mark(1,0);
    ULONGLONG section_started=GetTickCount64();
    jnet_character_frame_dt=dt;
    int64_t result = original_update(game, dt);
    jnet_update_cost(43,section_started);section_started=GetTickCount64();
    traveler_last_update=result;
    traveler_frame_complete(game);
    jnet_update_cost(45,section_started);section_started=GetTickCount64();
    if(jnet_online_enabled)jnet_online_game_tick(game);else jnet_lobby_tick(game);
    jnet_update_cost(47,section_started);
    if (JNETActorTrial[0]==1) {
        LONG cost=(LONG)(GetTickCount64()-update_started);
        InterlockedExchange(&JNETCompanionFeatures[37],cost);
        if (cost>JNETCompanionFeatures[38]) InterlockedExchange(&JNETCompanionFeatures[38],cost);
        InterlockedIncrement(&JNETCompanionFeatures[39]);
    }
    flight_mark(2,0);
    flight_mark(30,0);
    pose_link_tick(game);
    flight_mark(31,0);
    if (pose_link.started && !pose_link.mode) {
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i];
            if (preview_linked && (JNETMeshLife[25]==1 || JNETMeshLife[25]==2))
                InterlockedExchange(&JNETMeshLife[25],3);
            body_publish();
        }
        body_context=&body_contexts[0];
    }
    if (JNETMeshLife[106]==1) {
        if (boundary_checkpoint("[JC] original game update returned with Show Trial pending"))
            InterlockedExchange(&JNETMeshLife[106],2);
        else { InterlockedCompareExchange(&JNETCommand,0,10); preview_fail(15); }
    }
    if (JNETStartup[0] && (JNETResource==1 || JNETResource==2) &&
        preparation_deadline && GetTickCount64()>=preparation_deadline) {
        InterlockedExchange(&JNETResource,-1);
        InterlockedExchange(&JNETMeshLife[107],2);
        boundary_checkpoint("[JC] preparation expired; no trial allocated");
    }
    if (JNETRender[0]==5 && render_snapshot(11)) InterlockedExchange(&JNETRender[0],6);
    if (JNETRender[0]>0 && JNETRender[0]<6 && (uint32_t)(JNETFrames-render_frame)>=120) render_fail(10);
    if (JNETRender[0]>=6 && JNETRender[0]<9 && GetTickCount64()>=render_deadline) render_fail(10);
    InterlockedExchange(&JNETPhase, 2);
    if (TryAcquireSRWLockExclusive(&lifetime_lock)) {
      for (unsigned i=0;i<JC_BODY_COUNT;++i) {
        body_context=&body_contexts[i];
      if (initialized_life.state==1) {
        uint64_t barn=0;
        int field=0,fast_code=3;
        if (lifetime_conflict) field=JC_IF_LOCK;
        else if (game!=initialized_life.game) field=JC_IF_GAME;
        else if (GetCurrentThreadId()!=initialized_life.thread) field=JC_IF_THREAD;
        else if (!own_read(NULL,game+0x1e0,&barn,8)) { field=JC_IF_BARN; fast_code=1; }
        else if (barn!=initialized_life.barn) field=JC_IF_BARN;
        if (field) { initialized_fault_field(field,2); initialized_quarantine(fast_code); }
        else {
            uint32_t elapsed=(uint32_t)JNETFrames-(uint32_t)initialized_frame;
            InterlockedExchange(&JNETMeshLife[3],(LONG)elapsed);
            if (elapsed>=60u*(uint32_t)(JNETMeshLife[1]+1) && JNETMeshLife[1]<10) {
                struct callback_read_budget budget={.deadline=GetTickCount64()+100};
                struct jc_initialized_detail detail;
                InterlockedExchange(&JNETMeshLife[5],2);
                int code=jc_initialized_validate_detail(&initialized_life,callback_read,&budget,
                    (uintptr_t)GetModuleHandleW(NULL),game,GetCurrentThreadId(),&detail);
                if (budget.expired) code=jc_initialized_error(&detail,JC_IF_BUDGET,9);
                initialized_publish(&detail);
                if (code) initialized_quarantine(code);
                else InterlockedIncrement(&JNETMeshLife[1]);
            }
        }
      }
        body_publish();
      }
      body_context=&body_contexts[0];
      if (lifetime.state==1) {
        uint64_t barn;
        if (lifetime_conflict || game!=lifetime.game || GetCurrentThreadId()!=lifetime.thread ||
            !own_read(NULL,game+0x1e0,&barn,8) || barn!=lifetime.barn) {
            jc_lifetime_quarantine(&lifetime);
            InterlockedExchange(&JNETLifetime,-1);
            message("[JC] retained object's game/barn/thread changed without matched cleanup; restart required");
        }
      }
      ReleaseSRWLockExclusive(&lifetime_lock);
    } else { InterlockedExchange(&lifetime_conflict,1); InterlockedExchange(&JNETLifetime,-1); }
    if (JNETCommand!=13) native_playback_tick(game);
    if (JNETStatus < 0 || InterlockedCompareExchange(&in_callback, 1, 0)) return result;
    if (JNETStatus == 2) {
        InterlockedExchange(&JNETStatus, 1);
        message("[JC] native capture ready; Lua observation remains disabled until requested");
    }
    LONG command = InterlockedExchange(&JNETCommand, 0);
    if (JNETTraveler[0]==1 && GetTickCount64()>=traveler_deadline) traveler_result(-1,2,9);
    if (command==21 && !JNETTraveler[19]) { traveler_arm(game); goto done; }
    if(command==27 && !jnet_room.room) {traveler_actor_report(JC_ACTOR_RUNTIME_BUSY);goto done;}
    if(command==27) {
        int code=traveler_local_network_connect();
        JNETNetwork[2]=code;if(code)JNETNetwork[0]=-1;
        traveler_actor_report(code);goto done;
    }
    if(command==13 && traveler_network_external){struct jnet_lobby_action leave={.kind=3};jnet_lobby_submit(&leave);goto done;}
    if(command==13 && traveler_network_active) {
        traveler_network.paused=1;JNETNetwork[0]=2;
        traveler_local_stop();
        if(traveler_actor)traveler_actor_trial_stop(0);
        else {traveler_actor_trial_join_pending=0;traveler_peer_restart_requested=0;}
        goto done;
    }
    if(traveler_network_active && (command==22 || command==24 || command==25 || command==26)) {
        traveler_actor_report(JC_ACTOR_RUNTIME_BUSY);goto done;
    }
    if(command==26){traveler_stream_enabled=1;traveler_actor_report(traveler_local_start());goto done;}
    if (command==24 || command==25) {
        int code=traveler_repeat_start(command==25);
        if(code && !JNETActorTrial[0]){JNETActorTrial[0]=-1;JNETActorTrial[1]=code;}
        traveler_actor_report(code);goto done;
    }
    if (command==22) {
        int code=traveler_repeat.state==1?JC_ACTOR_RUNTIME_BUSY:JNETActorTrialStart();
        if (code && !JNETActorTrial[0]) {
            JNETActorTrial[0]=-1;JNETActorTrial[1]=code;JNETActorTrial[4]=1;
        }
        traveler_actor_report(code); goto done;
    }
    if(command==13 && traveler_local_state) {traveler_local_stop();traveler_repeat_cancel();
        if(JNETActorTrial[0] || traveler_actor_trial_join_pending)JNETActorTrialStop();goto done;}
    if (command==13 && JNETActorTrial[0]) { traveler_local_stop();traveler_repeat_cancel();JNETActorTrialStop();goto done; }
    if (command==23 && JNETActorTrial[0]>0 && JNETActorTrial[0]<3) {
        traveler_repeat_cancel();traveler_actor_trial_stop(0);traveler_peer_depart_requested=2;goto done;
    }
    if (JNETTraveler[0] || JNETTraveler[19]) {
        if (command) message("[JC] full traveler lifecycle selected; restart before another mode");
        goto done;
    }
    if (!command) goto done;
    if (command==13) {
        if (native_playback_mode) {
            if (native_playback_mode==1) native_playback_end(2,0);
            goto done;
        }
        if (mirror_mode) {
            if (mirror_mode!=1) goto done;
            mirror_elapsed();
            mirror_mode=2; InterlockedExchange(&JNETSkeleton[0],2);
            for (unsigned i=0;i<JC_BODY_COUNT;++i) {
                body_context=&body_contexts[i];
                if (JNETMeshLife[25]>0) InterlockedExchange(&JNETMeshLife[25],3);
                body_publish();
            }
            body_context=&body_contexts[0]; goto done;
        }
        pose_link_depart();
        pose_link_close(0);
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i];
            if (preview_linked && JNETMeshLife[25]>0) InterlockedExchange(&JNETMeshLife[25],3);
            body_publish();
        }
        body_context=&body_contexts[0];
        goto done;
    }
    if (command==19) { evaluation_inspect(game); goto done; }
    if (native_playback_mode) { message("[JC] native playback selected; restart before another mode"); goto done; }
    if (mirror_mode) { message("[JC] local skeleton test selected; restart before another mode"); goto done; }
    if (command==20) {
        int level=-1;
        native_playback_mode=2;
        if (pose_link.started || lifetime_conflict || JNETDual[0]!=1 || JNETResource!=5 ||
            JNETMeshLife[25] || body_contexts[1].out_MeshLife[25] ||
            jc_render_level(own_read,NULL,game,&level) || !jc_render_trial_same_level(prepared_level,level)) {
            InterlockedExchange(&JNETNativePlayback[0],-1); InterlockedExchange(&JNETNativePlayback[1],20); goto done;
        }
        if (!native_playback_start(0)) goto done;
        message("[JC] two-body native playback selected; guarded clip evaluation after original GameUpdate");
        goto done;
    }
    if (command==18) {
        int level=-1;
        if (pose_link.started || JNETDual[0]!=1 || JNETResource!=5 || JNETMeshLife[25] ||
            jc_render_level(own_read,NULL,game,&level) || !jc_render_trial_same_level(prepared_level,level)) {
            InterlockedExchange(&JNETSkeleton[0],-1); InterlockedExchange(&JNETSkeleton[1],6); goto done;
        }
        mirror_mode=1; InterlockedExchange(&JNETSkeleton[0],1);
        mirror_work=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*mirror_work));
        int code=5;
        if (mirror_work) {
            uint64_t barn=initialized_life.barn;
            const uint64_t meshes[]={barn+0x210,barn+0x14850,
                body_contexts[0].s_initialized_life.object+0x160,body_contexts[1].s_initialized_life.object+0x160};
            struct callback_read_budget budget={.deadline=GetTickCount64()+1500};
            struct jc_inspection inspection={.budget=&budget};
            code=jc_skel_prepare(mirror_read,&inspection,(uintptr_t)GetModuleHandleW(NULL),meshes,mirror_work);
            if (budget.expired) code=9;
            InterlockedExchange(&JNETSkeleton[2],(LONG)mirror_work->stage);
            for (unsigned i=0;i<4;++i) InterlockedExchange(&JNETSkeleton[3+i],(LONG)mirror_work->layout[i].names);
        }
        InterlockedExchange(&JNETSkeleton[1],code);
        if (code) { mirror_mode=2; InterlockedExchange(&JNETSkeleton[0],-1); goto done; }
        mirror_not_before=GetTickCount64()+3000;
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i]; preview_not_before=mirror_not_before;
            preview_deadline=preview_not_before+60000;
            InterlockedExchange(&JNETMeshLife[25],1); body_publish();
        }
        body_context=&body_contexts[0]; goto done;
    }
    if (command==17) {
        if (pose_link.mode>=4 && jc_room_next(&pose_link.room)) {
            jc_room_selected(&pose_link.room,&pose_link.receiver);
            InterlockedExchange(&JNETRoom[1],(LONG)pose_link.room.selected+1);
        }
        goto done;
    }
    if (command==11 || command==12 || command==14 || command==15 || command==16) {
        int level=-1;
        if (pose_link.started || jc_render_level(own_read,NULL,game,&level) || !jc_render_trial_level(level) ||
            (command!=12 && (JNETDual[0]!=1 || initialized_life.state!=1 || !JNETStartup[0] || JNETResource!=5 ||
                JNETMeshLife[25] || !jc_render_trial_same_level(prepared_level,level))) ||
            (command==12 && (JNETStartup[0] || allocation_attempted || bound_state || JNETResource || JNETRender[0]))) {
            pose_link_close(1); goto done;
        }
        if (!pose_link_start(command==15 ? 4 : command==16 ? 5 : command==11 ? 1 : command==14 ? 3 : 2)) goto done;
        if (command==15 || command==16) {
            if (!native_playback_start(1)) { pose_link_depart(); pose_link_close(0); goto done; }
        }
        if (command!=12) {
            for (unsigned i=0;i<(pose_link.mode>=4 ? JC_BODY_COUNT : 1);++i) {
                body_context=&body_contexts[i];
                preview_linked=1; preview_not_before=GetTickCount64(); preview_deadline=pose_link.deadline;
                InterlockedExchange(&JNETMeshLife[25],1); body_publish();
            }
            body_context=&body_contexts[0];
        }
        goto done;
    }
    if (pose_link.started) { message("[JC] pose link selected; other experiments refused until restart"); goto done; }
    if (command==10) {
        int level=-1; int code=jc_render_level(own_read,NULL,game,&level);
        InterlockedExchange(&JNETMeshLife[96],level);
        InterlockedExchange(&JNETMeshLife[97],code);
        if (code || !jc_render_trial_same_level(prepared_level,level)) { preview_fail(19); goto done; }
        if (!JNETMeshLife[25] && initialized_life.state==1 && JNETStartup[0] && JNETResource==5) {
            if (!boundary_checkpoint("[JC] Show Trial command accepted; waiting for matching consumer")) { preview_fail(15); goto done; }
            preview_not_before=GetTickCount64()+5000;
            preview_deadline=preview_not_before+60000;
            InterlockedExchange(&JNETMeshLife[109],5);
            InterlockedExchange(&JNETMeshLife[25],1);
        } else if (!JNETMeshLife[25]) preview_fail(14);
        goto done;
    }
    if (command==9) {
        if (JNETStartup[0] || allocation_attempted || bound_state || JNETResource) render_fail(11);
        else render_arm(game);
        goto done;
    }
    if (JNETRender[0]) { message("[JC] render observation selected; restart before other experiments"); goto done; }
    if (JNETStartup[0] && command!=7) { message("[JC] slot trial: other experimental commands refused; restart first"); goto done; }
    if (command==7 && (!JNETStartup[0] || JNETStartup[1]!=1 || JNETStartup[2]!=1 || JNETStartup[3]!=0 || JNETStartup[4]!=1)) { InterlockedExchange(&JNETResource,-1); message("[JC] slot trial refused: successful capacity startup required"); goto done; }
    if (allocation_attempted && (JNETTrial < 0 || JNETLifetime < 0 || JNETResource < 0)) {
        message("[JC] allocation failure: restart required; further commands refused");
        goto done;
    }
    if (command>=5 && command<=8) { allocation_trial(game,(int)command-5); goto done; }
    if (JNETResource) { message("[JC] resource test selected; Lua observation refused; restart for other diagnostics"); goto done; }
    if (JNETLifetime==1) { message("[JC] lifetime test active; Lua observation refused; change chapters to test cleanup"); goto done; }
    if (command < 1 || command > 4) {
        message("[JC] unknown control value ignored");
        goto done;
    }
    if (command == 3 && !bound_state) {
        InterlockedExchange(&JNETObservation, 3);
        message("[JC] no Lua observer active; observation remains paused");
        goto done;
    }
    InterlockedExchange(&JNETPhase, 3);
    if (!jc_readable((void *)(game + 32), sizeof(void *))) goto invalid_state;
    lua_State *state = *(lua_State **)(game + 32);
    if (!jc_readable(state, 64)) goto invalid_state;
    if (bound_state && state != bound_state) {
        message("[JC] Lua state changed; restart Journey before another diagnostic session");
        goto invalid_state;
    }
    if (!bound_state) {
        bound_state = state;
        if (!execute(state, jc_embedded_probe, sizeof(jc_embedded_probe) - 1, 1)) goto done;
        InterlockedExchange(&JNETStatus, 1);
        message("[JC] ready; observation is manual; native matchmaking unchanged");
    }
    const char *action = NULL;
    if (command == 1) action = "JourneyCompanionProbe.request()";
    else if (command == 2) action = "JourneyCompanionProbe.start()";
    else if (command == 3) action = "JourneyCompanionProbe.stop()";
    else if (command == 4) action = "JourneyCompanionProbe.bindings()";
    else if (command != 0) message("[JC] unknown control value ignored");
    if (action) {
        InterlockedExchange(&JNETPhase, 4);
        execute(state, action, strlen(action), 0);
    }
    goto done;
invalid_state:
    InterlockedExchange(&JNETStatus, -6);
    message("[JC] unexpected Lua context; observation disabled");
done:
    InterlockedExchange(&JNETPhase, 0);
    InterlockedExchange(&in_callback, 0);
    return result;
}

static int check_code(unsigned char *image) {
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)image;
    if (!jc_readable(image, sizeof(*dos)) || dos->e_magic != IMAGE_DOS_SIGNATURE ||
        dos->e_lfanew < 0 || dos->e_lfanew > 1048576) return 0;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(image + dos->e_lfanew);
    if (!jc_readable(nt, sizeof(*nt)) || nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        (nt->OptionalHeader.SizeOfImage != JC_IMAGE_SIZE && nt->OptionalHeader.SizeOfImage != JC_ORIGINAL_IMAGE_SIZE)) return 0;
    for (size_t i = 0; i < sizeof(jc_targets) / sizeof(jc_targets[0]); ++i) {
        const struct jc_target *target = jc_targets + i;
        if (target->rva > JC_IMAGE_SIZE - sizeof(target->prologue) ||
            !jc_readable(image + target->rva, sizeof(target->prologue)) ||
            memcmp(image + target->rva, target->prologue, sizeof(target->prologue))) return 0;
    }
    return 1;
}

static DWORD WINAPI initialize(void *unused) {
    int lifecycle_only=(uintptr_t)unused==2;
    int connection_only=(uintptr_t)unused==1 || lifecycle_only;
    if (unused && !connection_only && (uintptr_t)unused!=3) { JNETStatus=-8; return 0; }
    if ((uintptr_t)unused==3) InterlockedExchange(&JNETTraveler[19],1);
    body_context_init();
    wchar_t directory[JC_PATH_CAP], game[JC_PATH_CAP], actual[JC_PATH_CAP];
    unsigned char hash[32];
    if (!jc_paths(self, directory, game)) { JNETStatus = -7; return 0; }
    struct jnet_session_logs *shared=jnet_session_open(GetCurrentProcess(),GetCurrentProcessId(),&session_mapping);
    if(shared)session_logs=shared;
    atomic_store(&session_logs->ready,1);
    message("[JNET] native session diagnostics initialized; memory-only recording");
    message("[JC] validating executable identity");
    DWORD length = GetModuleFileNameW(NULL, actual, JC_PATH_CAP);
    if (!length || length >= JC_PATH_CAP || !jc_same_file(actual, game) ||
        !jc_hash(actual, hash) || !jc_executable_size(hash)) {
        message("[JC] executable path or hash mismatch; no hook installed");
        JNETStatus = -1;
        return 0;
    }
    message("[JC] executable identity passed; validating loaded entry points");
    unsigned char *image = (unsigned char *)GetModuleHandleW(NULL);
    if (!check_code(image)) {
        message("[JC] loaded code differs from expected entry points; no hook installed");
        JNETStatus = -2;
        return 0;
    }
    /* Fault-injection self-tests must not gate initialization inside Journey. */
    InterlockedExchange(&JNETCompanionFeatures[8],0);
    InterlockedExchange(&JNETCompanionFeatures[9],-1);
    message("[JC] native playback reader: ReadProcessMemory; in-game fault self-test disabled");
    fault_image=(uintptr_t)image; fault_self=(uintptr_t)self;
    IMAGE_DOS_HEADER *self_dos=(IMAGE_DOS_HEADER *)self;
    IMAGE_NT_HEADERS64 *self_nt=(IMAGE_NT_HEADERS64 *)(fault_self+self_dos->e_lfanew);
    fault_self_size=self_nt->OptionalHeader.SizeOfImage;
    if (AddVectoredExceptionHandler(1,fault_observer))
        InterlockedExchange(&JNETFlight[8],1);
    else {
        InterlockedExchange(&JNETFlight[8],-1);
        InterlockedExchange(&JNETFlight[9],(LONG)GetLastError());
    }
    message("[JC] passive fault observer configured; exceptions remain unhandled by this diagnostic");
    gettop = (void *)(image + JC_RVA_lua_gettop);
    settop = (void *)(image + JC_RVA_lua_settop);
    loadbuffer = (void *)(image + JC_RVA_luaL_loadbufferx);
    pcallfn = (void *)(image + JC_RVA_lua_pcallk);
    tolstring = (void *)(image + JC_RVA_lua_tolstring);
    pushclosure = (void *)(image + JC_RVA_lua_pushcclosure);
    InterlockedExchange(&JNETPerformance[0],0);
    InterlockedExchange(&JNETPerformance[1],-1);
    message("[JC] compatibility reader: ReadProcessMemory; fault self-test disabled");
    if (MH_Initialize() != MH_OK) goto hook_failed;
    if (MH_CreateHook(image + JC_RVA_GameUpdate, (void *)update, (void **)&original_update) != MH_OK) {
        MH_Uninitialize();
        goto hook_failed;
    }
    if (connection_only) {
        if (lifecycle_only && !lifecycle_install(image)) {
            MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
        }
        InterlockedExchange(&JNETObservation,lifecycle_only ? 5 : 4);
        InterlockedExchange(&JNETStatus,2);
        if (MH_EnableHook(lifecycle_only ? MH_ALL_HOOKS : image+JC_RVA_GameUpdate)!=MH_OK) {
            MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
        }
        message(lifecycle_only ? "[JC] lifecycle trace: twenty native entry observers; no trial objects or gameplay mutations" : "[JC] connection scan: only GameUpdate capture installed; no startup, render, resource or matchmaking hooks");
        return 0;
    }
    if (MH_CreateHook(image + JC_RVA_DudeFullInit,(void *)full_init_observer,(void **)&original_full_init)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_DudeCleanup, (void *)cleanup_observer, (void **)&original_cleanup) != MH_OK) {
        MH_RemoveHook(image + JC_RVA_GameUpdate); MH_Uninitialize(); goto hook_failed;
    }
    {
        static const unsigned char expected[16]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,
            0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48};
        if (!jc_readable(image+JC_CHAPTER_SWITCH_RVA,sizeof(expected)) ||
            memcmp(image+JC_CHAPTER_SWITCH_RVA,expected,sizeof(expected)) ||
            MH_CreateHook(image+JC_CHAPTER_SWITCH_RVA,(void *)chapter_switch_observer,
                (void **)&original_chapter_switch)!=MH_OK) {
            MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
        }
    }
    if(MH_CreateHook(image+JC_RVA_EnvironmentCopy,(void *)jnet_environment_copy,(void **)&jnet_environment_copy_original)!=MH_OK ||
       MH_CreateHook(image+JC_RVA_EnvironmentUpdate,(void *)jnet_environment_update,(void **)&jnet_environment_update_original)!=MH_OK){
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    jnet_controls_initialize();
    static const unsigned char controls_xml_entry[]={0x48,0x89,0x5c,0x24,0x18,0x56,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0x48,0x8b,0xd9};
    static const unsigned char controls_setting_entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57};
    static const unsigned char controls_action_entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x8b};
    static const unsigned char controls_dispatch_entry[]={0x48,0x89,0x5c,0x24,0x18,0x55,0x57,0x41,0x57,0x48,0x8d,0x6c,0x24,0xb9};
    static const unsigned char controls_push_entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57};
    if(!jc_readable(image+0x107c70,16) || memcmp(image+0x107c70,controls_push_entry,16) ||
       !jc_readable(image+0x101e30,14) || memcmp(image+0x101e30,controls_dispatch_entry,14) ||
       !jc_readable(image+0x2ecf60,16) || memcmp(image+0x2ecf60,controls_xml_entry,16) ||
       !jc_readable(image+0x1084f0,16) || memcmp(image+0x1084f0,controls_setting_entry,16) ||
       !jc_readable(image+0xaea60,16) || memcmp(image+0xaea60,controls_action_entry,16) ||
       MH_CreateHook(image+0xaea60,(void *)jnet_controls_queue,(void **)&jnet_controls_queue_original)!=MH_OK ||
       MH_CreateHook(image+0x2ecf60,(void *)jnet_menu_xml,(void **)&jnet_menu_xml_original)!=MH_OK ||
       MH_CreateHook(image+0x1084f0,(void *)jnet_setting,(void **)&jnet_setting_original)!=MH_OK ||
       MH_CreateHook(image+0x101e30,(void *)jnet_menu_action,(void **)&jnet_original_menu_action)!=MH_OK){
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    {
        static const unsigned char window_bytes[16]={0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x6c,
            0x24,0x20,0x56,0x57,0x41,0x57,0x48,0x81};
        static const unsigned char reserve_bytes[16]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,
            0xec,0x20,0x80,0x3d,0x2f,0xb5,0x88,0x03};
        if (!jc_readable(image+0x488e50,16) || memcmp(image+0x488e50,window_bytes,16) ||
            !jc_readable(image+0x378790,16) || memcmp(image+0x378790,reserve_bytes,16) ||
            MH_CreateHook(image+0x488e50,(void *)cloth_window_observer,(void **)&original_game_window)!=MH_OK) {
            MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
        }
    }
    {
        static const unsigned char map_bytes[16]={0x48,0x89,0x5c,0x24,0x18,0x55,0x48,0x83,
            0xec,0x20,0x48,0x8b,0xea,0x48,0x8b,0xd9};
        if (!jc_readable(image+0x267d10,16) || memcmp(image+0x267d10,map_bytes,16) ||
            !jc_readable(image+0xc0c60,16) || memcmp(image+0xc0c60,map_bytes,16) ||
            MH_CreateHook(image+0x267d10,(void *)cloth_vertex_map_observer,(void **)&original_vertex_map)!=MH_OK ||
            MH_CreateHook(image+0xc0c60,(void *)cloth_index_map_observer,(void **)&original_index_map)!=MH_OK) {
            MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
        }
    }
    {
        static const unsigned char entry[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,
            0x24,0x10,0x48,0x89,0x7c,0x24,0x20};
        if (!jc_readable(image+0x1e2d10,sizeof(entry)) || memcmp(image+0x1e2d10,entry,sizeof(entry)) ||
            !jc_readable(image+0x1e4970,sizeof(entry)) || memcmp(image+0x1e4970,entry,sizeof(entry)) ||
            MH_CreateHook(image+0x8e210,(void *)cloth_dependency_observer,(void **)&original_cloth_dependency)!=MH_OK ||
            MH_CreateHook(image+JC_RVA_ClothFinish,(void *)cloth_finish_observer,(void **)&original_cloth_finish)!=MH_OK ||
            MH_CreateHook(image+0x8a7a0,(void *)cloth_render_observer,(void **)&original_cloth_render)!=MH_OK ||
            MH_CreateHook(image+0x118030,(void *)traveler_input_observer,(void **)&original_traveler_input)!=MH_OK ||
            MH_CreateHook(image+JC_RVA_PlayerUpdate,(void *)traveler_update_observer,(void **)&original_traveler_update)!=MH_OK ||
            MH_CreateHook(image+0x118df0,(void *)traveler_finish_observer,(void **)&original_traveler_finish)!=MH_OK ||
            MH_CreateHook(image+0x352060,(void *)async_geometry_dispatch_observer,(void **)&original_async_geometry_dispatch)!=MH_OK ||
            MH_CreateHook(image+JC_RVA_MaterialCommit,(void *)actor_material_commit_observer,(void **)&original_actor_material_commit)!=MH_OK ||
            MH_CreateHook(image+JC_RVA_MaterialSamplerInit,(void *)actor_sampler_init_observer,(void **)&original_actor_sampler_init)!=MH_OK ||
            MH_CreateHook(image+0x1e2d10,(void *)cloth_worker_1e2d10,(void **)&original_cloth_worker[0])!=MH_OK ||
            MH_CreateHook(image+0x1e4970,(void *)cloth_worker_1e4970,(void **)&original_cloth_worker[1])!=MH_OK) {
            MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
        }
    }
    if (MH_CreateHook(image+JC_RVA_ShoutUpdate,(void *)traveler_glyph_update,(void **)&original_glyph_update)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    sand_inline_resume=image+JC_RVA_SandInlineResume;
    if (MH_CreateHook(image+JC_RVA_SandInline,(void *)traveler_sand_inline,&original_sand_inline)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image+JC_RVA_SandAppend,(void *)traveler_sand_append,(void **)&original_sand_append)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image+JC_RVA_SandConsume,(void *)traveler_sand_consume,(void **)&original_sand_consume)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    static const unsigned char scarf_entry[]={0x4c,0x8b,0xdc,0x56,0x57,0x41,0x54,0x48,0x81,0xec,0xb0,0,0,0};
    if(memcmp(image+0x24c010,scarf_entry,sizeof(scarf_entry))) {JNETStatus=-2;return 0;}
    if (MH_CreateHook(image+0x24c010,(void *)native_scarf_update,(void **)&original_native_scarf_update)!=MH_OK) {
        message("[JC] scarf arrival hook failed");JNETStatus=-4;return 0;
    }
    if (MH_CreateHook(image+0x273200,(void *)cloth_vector_setter,(void **)&original_cloth_setter[0])!=MH_OK ||
        MH_CreateHook(image+0x273400,(void *)cloth_matrix_setter,(void **)&original_cloth_setter[1])!=MH_OK ||
        MH_CreateHook(image+0x273640,(void *)cloth_texture_setter,(void **)&original_cloth_setter[2])!=MH_OK ||
        MH_CreateHook(image+0x273a00,(void *)cloth_scalar_lookup,(void **)&original_cloth_lookup)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_DudeResourceInit, (void *)resource_observer, (void **)&original_resource_init) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_MeshRegister, (void *)mesh_register_observer, (void **)&original_mesh_register) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_GeometryBuild, (void *)geometry_build_observer, (void **)&original_geometry_build) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_ScriptExecute, (void *)jnet_script_execute, (void **)&jnet_script_execute_original) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED && MH_CreateHook(image+JC_RVA_CharacterSkeletonBuild,
        (void *)jnet_character_skeleton_build,(void **)&jnet_character_skeleton_build_original)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED &&
        (MH_CreateHook(image+JC_RVA_CharacterAnimationLoad,(void *)jnet_character_animation_load,(void **)&jnet_character_animation_load_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterAnimationFinish,(void *)jnet_character_animation_finish,(void **)&jnet_character_animation_finish_original)!=MH_OK)) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED && MH_CreateHook(image+JC_RVA_CharacterAssetLoad,(void *)jnet_character_asset_load,(void **)&jnet_character_asset_load_original)!=MH_OK) {
        message("[JNET] character asset mapping hook failed");
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    jnet_texture_missing=image+0x9e3de;
    if (JC_NATIVE_CHARACTERS_ENABLED &&
        (MH_CreateHook(image+JC_RVA_ResourceView,(void *)jnet_resource_view_guard,&jnet_resource_view_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_TextureNullGuard,(void *)jnet_texture_guard,&jnet_texture_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterMeshUpdate,(void *)jnet_character_mesh_update,(void **)&jnet_character_mesh_update_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterShoutInit,(void *)jnet_character_shout_init,(void **)&jnet_character_shout_init_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterDecorationUpdate,(void *)jnet_character_decoration_update,(void **)&jnet_character_decoration_update_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterDecorationDraw,(void *)jnet_character_decoration_draw,(void **)&jnet_character_decoration_draw_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterDecorationCleanup,(void *)jnet_character_decoration_cleanup,(void **)&jnet_character_decoration_cleanup_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterInteract,(void *)jnet_character_interact,(void **)&jnet_character_interact_original)!=MH_OK ||
         MH_CreateHook(*(void **)(image+0x553c78),(void *)jnet_decoration_read,(void **)&jnet_decoration_read_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_IntroUpdate,(void *)jnet_intro_update,(void **)&jnet_intro_update_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterBank,(void *)jnet_character_bank,(void **)&jnet_character_bank_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterBodyInput,(void *)jnet_character_body_input,(void **)&jnet_character_body_input_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_DudeClothAppearance,(void *)jnet_character_appearance,(void **)&jnet_character_appearance_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterClothRide,(void *)jnet_character_ride,(void **)&jnet_character_cloth_ride_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterClothTouch,(void *)jnet_character_touch,(void **)&jnet_character_touch_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterWhaleRide,(void *)jnet_character_ride,(void **)&jnet_character_ride_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterLinkEmitter,(void *)jnet_character_link,(void **)&jnet_character_link_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_ChestSymbol,(void *)jnet_character_chest,(void **)&jnet_character_chest_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_NativeShout,(void *)jnet_character_shout,(void **)&jnet_character_shout_original)!=MH_OK)) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED && MH_CreateHook(image+JC_RVA_CharacterEye,
        (void *)jnet_character_eye,(void **)&jnet_character_eye_original)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED &&
        (MH_CreateHook(image+JC_RVA_CharacterAttack,(void *)jnet_character_attack,(void **)&jnet_character_attack_original)!=MH_OK ||
         MH_CreateHook(image+JC_RVA_CharacterAttackHandler,(void *)jnet_character_attack_handler,(void **)&jnet_character_attack_handler_original)!=MH_OK)) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED && MH_CreateHook(image+JC_RVA_CharacterJump,
        (void *)jnet_character_jump,(void **)&jnet_character_jump_original)!=MH_OK) {
        message("[JNET] native creature jump hook unavailable");
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED && MH_CreateHook(image+JC_RVA_CharacterCamera,
        (void *)jnet_character_camera,(void **)&jnet_character_camera_original)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (JC_NATIVE_CHARACTERS_ENABLED && MH_CreateHook(image+JC_RVA_CreatureReset,(void *)jnet_character_reset_native,(void **)&jnet_character_reset_original)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_luaL_loadbufferx, (void *)startup_load_observer, (void **)&original_loadbuffer) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_ClothUpdate, (void *)companion_cloth_update, (void **)&original_cloth_update) != MH_OK ||
        MH_CreateHook(image + JC_RVA_TravelerSubmit, (void *)submit_observer, (void **)&original_submit) != MH_OK ||
        MH_CreateHook(image + JC_RVA_RenderConsume, (void *)consume_observer, (void **)&original_consume) != MH_OK ||
        MH_CreateHook(image + JC_RVA_GeometryDispatch, (void *)geometry_dispatch_observer, (void **)&original_geometry_dispatch) != MH_OK ||
        MH_CreateHook(image + JC_RVA_CommandBuild, (void *)command_build_observer, (void **)&original_command_build) != MH_OK ||
        MH_CreateHook(image + JC_RVA_RenderFlush, (void *)render_flush_observer, (void **)&original_render_flush) != MH_OK ||
        MH_CreateHook(image + JC_RVA_RenderHandoff, (void *)render_handoff_observer, (void **)&original_render_handoff) != MH_OK ||
        MH_CreateHook(image + JC_RVA_BaseFlush, (void *)base_flush_observer, (void **)&original_base_flush) != MH_OK ||
        MH_CreateHook(image + JC_RVA_BaseHandoff, (void *)base_handoff_observer, (void **)&original_base_handoff) != MH_OK ||
        MH_CreateHook(image + JC_RVA_CommandExecute, (void *)command_execute_observer, (void **)&original_command_execute) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
#if JNET_MENU_IMAGE_PREVIEW
    if (MH_CreateHook(image + JC_RVA_MenuPresent, (void *)jnet_present, (void **)&jnet_original_present) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
#else
    InterlockedExchange(&JNETMenu[8],-1);
    message("[JNET] recovery build: image presentation hook and mouse polling disabled");
#endif
    static const unsigned char menu_text_entry[]={0x48,0x8b,0xc4,0x48,0x89,0x48,0x08};
    if(memcmp(image+0x103fd0,menu_text_entry,sizeof(menu_text_entry)) ||
       MH_CreateHook(image+0x103fd0,(void *)jnet_menu_text,(void **)&jnet_original_menu_text)!=MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (MH_CreateHook(image + JC_RVA_SavePoll, (void *)jnet_save_poll, (void **)&jnet_save_poll_original) != MH_OK ||
        MH_CreateHook(image + JC_RVA_SaveLoad, (void *)jnet_save_load, (void **)&jnet_save_load_original) != MH_OK ||
        MH_CreateHook(image + JC_RVA_MenuInput, (void *)jnet_menu_input, (void **)&jnet_original_menu_input) != MH_OK ||
        MH_CreateHook(image + JC_RVA_MenuPop, (void *)jnet_menu_pop, (void **)&jnet_menu_pop_original) != MH_OK ||
        MH_CreateHook(image + JC_RVA_MenuFrame, (void *)jnet_menu_frame, (void **)&jnet_menu_frame_original) != MH_OK ||
        MH_CreateHook(image + JC_RVA_MenuBuild, (void *)jnet_menu_build, (void **)&jnet_original_menu_build) != MH_OK ||
        MH_CreateHook(image + JC_RVA_MenuNodes, (void *)jnet_menu_nodes, (void **)&jnet_original_menu_nodes) != MH_OK) {
        MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); goto hook_failed;
    }
    static const unsigned char queue_entry[]={0x40,0x57,0x48,0x83,0xec,0x40,0x4c,0x8b,0xd1};
    if(memcmp(image+0xda6c0,queue_entry,sizeof(queue_entry))){
        message("[JNET] QueueLevel startup signature mismatch at RVA 000da6c0");
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if(MH_CreateHook(image+0xda6c0,(void *)jnet_queue_level,(void **)&jnet_queue_level_original)!=MH_OK){
        message("[JNET] QueueLevel hook creation failed at RVA 000da6c0");
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    traveler_level_rearm=jnet_traveler_rearm;
    traveler_native_initialized=traveler_local_level_initialized;
    jnet_travel_request=jnet_travel_begin;jnet_travel_receive=jnet_travel_message;jnet_travel_poll=jnet_travel_poll_live;
    {
        const unsigned char signature[]={0x4c,0x8b,0xdc,0x49,0x89,0x5b,0x10};
        unsigned char actual[sizeof(signature)];
        if(!own_read(NULL,(uintptr_t)image+0x225350,actual,sizeof(actual)) || memcmp(actual,signature,sizeof(actual)) ||
           MH_CreateHook(image+0x225350,(void *)jnet_rocket_update,(void **)&jnet_rocket_update_original)!=MH_OK){
            message("[JNET] native rocket hook unavailable");
            MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
        }
    }
    jnet_rocket_receive=jnet_rocket_message;jnet_rocket_poll=jnet_rocket_poll_live;
    jnet_chat_receive=jnet_chat_message;jnet_chat_poll=jnet_chat_lobby;
    jnet_member_view=jnet_travel_members_view;jnet_travel_session_reset=jnet_travel_reset;
    jnet_quit_to_title=jnet_travel_quit_to_title;
    jnet_sl_roster=jnet_sn_roster;jnet_sl_transport_poll=jnet_sn_tick;jnet_sl_transport_reset=jnet_sn_reset;
    traveler_online_before_input=jnet_online_before_input;
    if(JC_NATIVE_CHARACTERS_ENABLED){
        jnet_character_capture_rotation=jnet_character_rotation;
        traveler_character_input=jnet_character_input_native;
        traveler_character_updated=jnet_character_after_player;
        jnet_character_apply_native=jnet_character_apply;
        jnet_menu_supported_characters=(1u<<JNET_CHARACTER_LONGTAIL)|(1u<<JNET_CHARACTER_WHALE)|(1u<<JNET_CHARACTER_WAR_MACHINE)|(1u<<JNET_CHARACTER_FLOW_1)|(1u<<JNET_CHARACTER_FLOW_2)|(1u<<JNET_CHARACTER_ROCKET);
    }
    jnet_sl_background=jnet_maintenance_poll;
    jnet_steam_session_poll=jnet_realtime_poll;
    jnet_steam_owner_enter=jnet_realtime_enter;jnet_steam_owner_leave=jnet_realtime_leave;
    {
        HMODULE self=NULL;wchar_t config[MAX_PATH];
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)(uintptr_t)jnet_motion_apply,&self)) {
            DWORD n=GetModuleFileNameW(self,config,MAX_PATH);
            if(n && n<MAX_PATH) {
                wchar_t *slash=wcsrchr(config,L'\\');
                if(slash && slash-config+19<MAX_PATH) {
                    wcscpy(slash+1,L"JNET-network.ini");
                    jnet_realtime_enabled=GetPrivateProfileIntW(L"Transport",L"Worker",0,config)!=0;
                    jnet_diagnostic_scans=GetPrivateProfileIntW(L"Diagnostics",L"Scans",0,config)!=0;
                    jnet_diagnostic_logs=GetPrivateProfileIntW(L"Diagnostics",L"ObservationLogs",0,config)!=0;
                    jnet_motion_ms=GetPrivateProfileIntW(L"Motion",L"SmoothMs",0,config)==80?80:0;
                }
            }
        }
        JNETSteamGame[74]=(LONG)jnet_diagnostic_scans;JNETSteamGame[75]=(LONG)jnet_diagnostic_logs;
        JNETSteamGame[72]=(LONG)jnet_motion_ms;JNETSteamGame[73]=(LONG)jnet_realtime_enabled;
    }
    jnet_steam_install_probe();
    InterlockedExchange(&JNETStatus, 2);
    unsigned char chirp_entry[sizeof(jc_chirp_enqueue_code)];
    if (JC_NATIVE_CHARACTERS_ENABLED &&
        (!own_read(NULL,(uintptr_t)image+JC_RVA_NativeShout,chirp_entry,sizeof(chirp_entry)) ||
         memcmp(chirp_entry,jc_chirp_enqueue_code,sizeof(chirp_entry)))) {
        MH_RemoveHook(MH_ALL_HOOKS);MH_Uninitialize();goto hook_failed;
    }
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        if (MH_DisableHook(MH_ALL_HOOKS)==MH_OK) { MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize(); }
        goto hook_failed;
    }
    traveler_actor_material_hooks_ready=1;
    traveler_glyph_sampler_hook_ready=1;
    InterlockedExchange(&JNETMenu[6],1);
    message("[JNET] title/pause menu hook enabled; waiting for native menu draw");
    message("[JC] game update hook installed; waiting for a game frame");
    return 0;
hook_failed:
    JNETStatus = -4;
    message("[JC] hook installation failed; do not retry without restarting Journey");
    return 0;
}

__declspec(dllexport) DWORD WINAPI JNETInitialize(void *unused) {
    if (InterlockedCompareExchange(&initialization_started, 1, 0)) return 0;
    initialize(unused);
    return JNETStatus < 0 ? 1 : 0;
}

/* Called only by JNET's disposable reader-check child, never initialization. */
__declspec(dllexport) DWORD WINAPI JNETReaderCheck(void) {
    DWORD code=(DWORD)jc_local_windows_test();
    if (code) return code;
    LONG mode=JNETCompanionFeatures[8],gate=JNETCompanionFeatures[9];
    JNETCompanionFeatures[8]=1;JNETCompanionFeatures[9]=0;
    uint64_t value=0,readback=0;
    for (unsigned i=0;i<1000;++i) {
        value=i;
        if (!own_read(NULL,(uintptr_t)&value,&readback,sizeof(readback)) || readback!=value) {code=40;break;}
    }
    if (!code && own_read(NULL,0,&readback,sizeof(readback))) code=41;
    JNETCompanionFeatures[8]=mode;JNETCompanionFeatures[9]=gate;
    return code;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        self = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
