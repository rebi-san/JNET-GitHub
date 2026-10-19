#ifndef JC_TRAVELER_TRIAL_RUNTIME_H
#define JC_TRAVELER_TRIAL_RUNTIME_H
#include "native_traveler_trial.h"
__declspec(dllexport) volatile LONG JNETTraveler[44]={
    [20]=-1,[21]=-1,[22]=-1,[23]=-1,[24]=-1,[25]=-1,[26]=-1,[27]=-1,[28]=-1,[30]=-1,[31]=-1};
static int (*traveler_level_rearm)(void);
static void (*traveler_native_initialized)(uintptr_t);
static uintptr_t (*original_full_init)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,
    uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static uint64_t traveler_object,traveler_game,traveler_barn,traveler_decorations,traveler_sound;
static uint64_t traveler_geometry_source;
static DWORD traveler_thread;
static ULONGLONG traveler_deadline;
static struct jc_traveler_identity traveler_original[2],traveler_identity;
static struct jc_callback_snapshot traveler_callbacks;
static int traveler_result(int state,int stage,int code) {
    InterlockedExchange(&JNETTraveler[1],stage);
    InterlockedExchange(&JNETTraveler[2],code);
    InterlockedExchange(&JNETTraveler[0],state);
    char line[160];
    snprintf(line,sizeof(line),"[JC] traveler state=%d stage=%d code=%d context=%ld guard=%ld",
        state,stage,code,(long)JNETTraveler[15],(long)JNETTraveler[16]);
    int logged=boundary_checkpoint(line);
    snprintf(line,sizeof(line),"[JC] traveler init=%ld cleanup=%ld delete=%ld glyph=%ld/%ld sounds=%ld",
        (long)JNETTraveler[4],(long)JNETTraveler[5],(long)JNETTraveler[6],
        (long)JNETTraveler[7],(long)JNETTraveler[8],(long)JNETTraveler[9]);
    logged=logged && boundary_checkpoint(line);
    snprintf(line,sizeof(line),"[JC] traveler geometry cleanup before/after=%ld/%ld check=%ld phase=%ld",
        (long)JNETTraveler[40],(long)JNETTraveler[41],(long)JNETTraveler[42],(long)JNETTraveler[43]);
    logged=logged && boundary_checkpoint(line);
    snprintf(line,sizeof(line),"[JC] traveler callbacks scan=%ld before=%ld/%ld after=%ld/%ld extra=%ld source-match=%ld detail=%ld",
        (long)JNETTraveler[32],(long)JNETTraveler[33],(long)JNETTraveler[34],
        (long)JNETTraveler[35],(long)JNETTraveler[36],(long)JNETTraveler[37],
        (long)JNETTraveler[38],(long)JNETTraveler[39]);
    logged=logged && boundary_checkpoint(line);
    snprintf(line,sizeof(line),"[JC] traveler prepare=%ld controller-own-mesh-before/after=%ld/%ld",
        (long)JNETTraveler[29],(long)JNETTraveler[30],(long)JNETTraveler[31]);
    logged=logged && boundary_checkpoint(line);
    snprintf(line,sizeof(line),"[JC] traveler gates=%ld/%ld/%ld blocked=%ld chapters=%ld/%ld present=%ld transport=%ld layout=%ld",
        (long)JNETTraveler[20],(long)JNETTraveler[21],(long)JNETTraveler[22],
        (long)JNETTraveler[23],(long)JNETTraveler[24],(long)JNETTraveler[25],
        (long)JNETTraveler[26],(long)JNETTraveler[27],(long)JNETTraveler[28]);
    logged=logged && boundary_checkpoint(line);
    snprintf(line,sizeof(line),"[JC] traveler clone=%ld pose=%ld animation=%ld callbacks=%ld restored=%ld",
        (long)JNETTraveler[10],(long)JNETTraveler[11],(long)JNETTraveler[12],
        (long)JNETTraveler[13],(long)JNETTraveler[14]);
    logged=logged && boundary_checkpoint(line);
    if (!logged) {
        InterlockedExchange(&JNETTraveler[2],18);
        InterlockedExchange(&JNETTraveler[0],traveler_object ? -2 : -1);
    }
    return logged;
}
static int traveler_context(void) {
    uint64_t barn;
    return GetCurrentThreadId()==traveler_thread && (uintptr_t)JNETGame==traveler_game &&
        own_read(NULL,traveler_game+0x1e0,&barn,8) && barn==traveler_barn;
}
static int traveler_callback_snapshot(uint64_t manager,struct jc_callback_snapshot *out) {
    uint64_t context=0,owner=0,expected=0,again=0;
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(own_read,NULL,manager,8,&context,8) ||
        !jc_layout_field(own_read,NULL,context,0x10,&owner,8) ||
        !jc_layout_field(own_read,NULL,traveler_game,0x200,&expected,8)) return JC_LAYOUT_READ;
    if (!owner || owner!=expected) return JC_LAYOUT_TYPE;
    struct jc_callback_workspace *work=HeapAlloc(GetProcessHeap(),0,sizeof(*work));
    const uint64_t meshes[]={traveler_barn+0x210,traveler_barn+0x14850,traveler_object ? traveler_object+0x160 : 0};
    struct callback_read_budget budget={.deadline=GetTickCount64()+1000};
    int code=jc_mesh_callbacks(callback_read,&budget,owner,meshes,out,work);
    if (work) HeapFree(GetProcessHeap(),0,work);
    if (!code && (!jc_layout_field(own_read,NULL,manager,8,&again,8) || again!=context ||
        !jc_layout_field(own_read,NULL,context,0x10,&again,8) || again!=owner ||
        !jc_layout_field(own_read,NULL,traveler_game,0x200,&again,8) || again!=owner)) code=JC_LAYOUT_CHANGED;
    return budget.expired ? 9 : code;
}
static int traveler_callback_source(uint64_t mesh,uint64_t *source) {
    uint64_t node=0,again=0;
    return jc_layout_field(own_read,NULL,mesh,8,&node,8) &&
        jc_layout_field(own_read,NULL,node,0x10,source,8) && *source &&
        jc_layout_field(own_read,NULL,mesh,8,&again,8) && again==node;
}
static int traveler_originals_match(uint64_t image) {
    for (unsigned i=0;i<2;++i) {
        struct jc_traveler_identity now;
        if (jc_traveler_identity_read(own_read,NULL,image,traveler_barn+(i ? 0x146f0 : 0xb0),&now) ||
            memcmp(&now,&traveler_original[i],sizeof(now))) return 0;
    }
    return 1;
}
static struct jc_traveler_identity traveler_admission_identity[2];
static uint64_t traveler_admission_barn;
static int traveler_admission_ready;
static void traveler_admission_initialized(uintptr_t dude) {
    uint64_t barn=0;uintptr_t game=(uintptr_t)JNETGame;
    if(!game || !own_read(NULL,game+0x1e0,&barn,8) || !barn)return;
    if(dude==barn+0xb0){traveler_admission_ready=0;return;}
    if(dude!=barn+0x146f0)return;
    traveler_admission_ready=0;
    struct jc_traveler_identity identities[2];
    for(unsigned i=0;i<2;++i)
        if(jc_traveler_identity_read(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),
            barn+(i?0x146f0:0xb0),&identities[i]))return;
    memcpy(traveler_admission_identity,identities,sizeof(identities));
    traveler_admission_barn=barn;traveler_admission_ready=1;
}
static int traveler_admission_storage_matches(uint64_t image) {
    if(!traveler_admission_ready || traveler_admission_barn!=traveler_barn)return 0;
    for(unsigned i=0;i<2;++i){
        struct jc_traveler_identity now;
        if(jc_traveler_identity_read(own_read,NULL,image,traveler_barn+(i?0x146f0:0xb0),&now) ||
           !jc_traveler_storage_same(&now,&traveler_admission_identity[i]))return 0;
    }
    return 1;
}
static int traveler_presence(uintptr_t image,uintptr_t game) {
    struct jc_layout_result layout;
    int code=jc_native_layout(own_read,NULL,image,game,&layout);
    for (unsigned i=20;i<28;++i) JNETTraveler[i]=-1;
    JNETTraveler[28]=code;
    if (code) { JNETTraveler[15]=30; return 0; }
    for (unsigned i=0;i<3;++i) JNETTraveler[20+i]=layout.gates[i];
    JNETTraveler[23]=layout.blocked;
    JNETTraveler[24]=(LONG)layout.local_chapter;
    JNETTraveler[25]=(LONG)layout.remote_chapter;
    JNETTraveler[26]=layout.remote_gate;
    if (layout.remote_gate) { JNETTraveler[15]=23; return 0; }
    return 1;
}
static void traveler_arm(uintptr_t game) {
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    if (JNETTraveler[0] || allocation_attempted || bound_state || JNETResource || pose_link.started) return;
    boundary_checkpoint_begin();
    if (JNETStartup[1]!=1 || JNETStartup[2]!=1 || JNETStartup[3] || JNETStartup[4]!=1 ||
        !traveler_presence(image,game) ||
        !own_read(NULL,game+0x1e0,&traveler_barn,8)) { traveler_result(-1,1,8); return; }
    traveler_game=game; traveler_thread=GetCurrentThreadId();
    traveler_deadline=GetTickCount64()+600000; allocation_attempted=1;
    resource_game=game; resource_barn=traveler_barn; resource_thread=traveler_thread;
    traveler_result(1,1,0);
}
static void traveler_mesh_report(const char *stage,unsigned index,uintptr_t mesh) {
    uint64_t source=0,payload=0,slot=0,clone=0,root=0,entries=0,count=0,slots=0,context=0;
    uint32_t capacity=0,mask=0;
    if(own_read(NULL,mesh+8,&source,8))mask|=1;
    if(source && own_read(NULL,source+0x10,&payload,8))mask|=2;
    if(own_read(NULL,mesh+0x30,&slot,8))mask|=4;
    if(slot && own_read(NULL,slot+0x10,&clone,8))mask|=8;
    if(clone && own_read(NULL,clone+0x18,&root,8))mask|=16;
    if(own_read(NULL,mesh+0x578,&entries,8) && own_read(NULL,mesh+0x580,&count,8))mask|=32;
    if(payload && own_read(NULL,payload+0xb0,&slots,8) && own_read(NULL,payload+0xd8,&capacity,4) &&
       own_read(NULL,payload+0x50,&context,8))mask|=64;
    char line[480];int n=snprintf(line,sizeof(line),
        "[JNET-mesh] %s index=%u read=%x source=%llx payload=%llx slot=%llx clone=%llx root=%llx anim=%llx/%llu pool=%llu/%u context=%llx callbacks=%ld/%ld error=%ld\r\n",
        stage,index,mask,(unsigned long long)source,(unsigned long long)payload,(unsigned long long)slot,
        (unsigned long long)clone,(unsigned long long)root,(unsigned long long)entries,(unsigned long long)count,
        (unsigned long long)slots,capacity,(unsigned long long)context,(long)JNETMesh[1],(long)JNETMesh[2],(long)mesh_fault);
    if(n>0 && (size_t)n<sizeof(line))jnet_log_append(&session_logs->fault,line,(size_t)n);
}
static uintptr_t full_init_observer(uintptr_t dude,uintptr_t manager,uintptr_t effects,uintptr_t unused,
        uintptr_t lua,uintptr_t random,uintptr_t name,uintptr_t sound,uintptr_t decorations,uintptr_t remote) {
    if((JNETTraveler[0]==4 || JNETTraveler[0]==-1) && (unsigned char)remote==1 && traveler_level_rearm)traveler_level_rearm();
    uintptr_t result=original_full_init(dude,manager,effects,unused,lua,random,name,sound,decorations,remote);
    traveler_admission_initialized(dude);
    if((unsigned char)remote==1 && traveler_native_initialized)traveler_native_initialized(dude);
    if (JNETTraveler[19] && !JNETTraveler[0]) {
        uintptr_t game=(uintptr_t)JNETGame; uint64_t barn=0; int current=-1;
        if (game && own_read(NULL,game+0x1e0,&barn,8) && dude==barn+0x146f0 &&
            !jc_render_level(own_read,NULL,game,&current) && current>=0 && current<12) {
            InterlockedExchange(&JNETTraveler[19],0);
            if ((DWORD)JNETFlight[3]!=GetCurrentThreadId()) traveler_result(-1,2,8);
            else traveler_arm(game);
        }
    }
    if (JNETTraveler[0]!=1 || dude!=traveler_barn+0x146f0) return result;
    int level=-1,code=jc_render_level(own_read,NULL,traveler_game,&level);
    if (code || level<0 || level>=12 || !traveler_context() || GetTickCount64()>=traveler_deadline ||
        !TryAcquireSRWLockExclusive(&lifetime_lock)) { traveler_result(-1,2,8); return result; }
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    uint64_t value=0,match=0; unsigned char connected=1,global=1;
    uint32_t before=1024,after=0; struct jc_geometry_pool pool;
    const unsigned offsets[]={0x28,0xb0,0x20,0x1a0,0x150,0x138};
    const uint64_t arguments[]={manager,effects,lua,random,sound,decorations};
    int stage=2;
    for (unsigned i=0;i<6;++i) if (!arguments[i] ||
        !own_read(NULL,traveler_game+offsets[i],&value,8) || value!=arguments[i]) {
        JNETTraveler[15]=(LONG)(100+i); code=8; goto done;
    }
#define JC_TRAVELER_REQUIRE(test,detail) do { if (!(test)) { JNETTraveler[15]=detail; code=8; goto done; } } while (0)
    JC_TRAVELER_REQUIRE((unsigned char)remote==1,20);
    JC_TRAVELER_REQUIRE(name==image+0x5dffa0,21);
    JC_TRAVELER_REQUIRE(own_read(NULL,image+0x37c7198,&global,1) && !global,22);
    /* RVA 117740: raw activity flags alone do not identify a resident peer. */
    if (!traveler_presence(image,traveler_game)) { code=8; goto done; }
    JC_TRAVELER_REQUIRE(own_read(NULL,traveler_game+0x180,&match,8) &&
        jc_layout_field(own_read,NULL,match,0x11db4,&connected,1),31);
    JNETTraveler[27]=connected;
    JC_TRAVELER_REQUIRE(!connected,24);
    JC_TRAVELER_REQUIRE(jc_layout_field(own_read,NULL,decorations,0x434010,&before,4),25);
    JNETTraveler[7]=(LONG)before;
    JC_TRAVELER_REQUIRE(before<1024,26);
    JC_TRAVELER_REQUIRE(!jc_geometry_pool_read(own_read,NULL,dude+0x1e0,&pool),27);
    JNETTraveler[17]=(LONG)pool.slots; JNETTraveler[3]=(LONG)pool.occupied;
    JNETTraveler[18]=(LONG)pool.free_slots;
    JC_TRAVELER_REQUIRE(pool.slots==4 && pool.occupied==2 && pool.free_slots==2,28);
    JC_TRAVELER_REQUIRE(own_read(NULL,manager+8,&mesh_context,8) && mesh_context,29);
#undef JC_TRAVELER_REQUIRE
    traveler_geometry_source=pool.source;
    for (unsigned i=0;i<2;++i) {
        code=jc_traveler_identity_read(own_read,NULL,image,traveler_barn+(i ? 0x146f0 : 0xb0),&traveler_original[i]);
        if (code) goto done;
    }
    code=traveler_callback_snapshot(manager,&traveler_callbacks);
    JNETTraveler[32]=code;
    if (code) { JNETTraveler[39]=1; code=15; goto done; }
    JNETTraveler[33]=(LONG)traveler_callbacks.owners[0].total;
    JNETTraveler[34]=(LONG)traveler_callbacks.owners[1].total;
    stage=3;
    if (!traveler_result(2,stage,0)) { code=18; goto done; }
    const struct jc_allocation_ops ops={own_read,trial_create,trial_destroy,(void *)image};
    if (jc_allocation_create_checked(&ops,image,traveler_barn,&traveler_object)!=JC_ALLOC_OK) { code=10; goto done; }
    traveler_decorations=decorations; traveler_sound=sound;
    /* Native chapter setup calls a2ec0 before a32c0, including controller binding. */
    stage=9;
    if (!own_read(NULL,traveler_object+0xa620,&value,8)) { code=19; goto done; }
    JNETTraveler[30]=(value==traveler_object+0x160);
    if (value && value!=traveler_object+0x160) { code=19; goto done; }
    if (!traveler_result(2,stage,0)) { code=18; goto done; }
    flight_mark(126,1); ++JNETTraveler[29];
    trial_prepare(image,traveler_object,manager);
    flight_mark(127,1);
    if (!own_read(NULL,traveler_object+0xa620,&value,8)) { code=19; goto done; }
    JNETTraveler[31]=(value==traveler_object+0x160);
    if (!JNETTraveler[31] || !traveler_context() || !traveler_originals_match(image) ||
        jc_allocation_validate(&ops,image,traveler_barn,traveler_object)!=JC_ALLOC_OK) { code=19; goto done; }
    struct jc_callback_snapshot prepared_callbacks;
    if (traveler_callback_snapshot(manager,&prepared_callbacks) || prepared_callbacks.owners[2].total ||
        !jc_callback_equal(&prepared_callbacks.owners[0],&traveler_callbacks.owners[0]) ||
        !jc_callback_equal(&prepared_callbacks.owners[1],&traveler_callbacks.owners[1])) { code=19; goto done; }
    stage=4; if (!traveler_result(2,stage,0)) { code=18; goto done; }
    if(!traveler_glyph_init_begin(image,traveler_object,traveler_original[1].glyph+0x90,
        decorations,before)) {JNETTraveler[16]=traveler_glyph_init.failed;code=5;goto done;}
    body_context=&body_contexts[0];
    InterlockedExchange(&JNETMesh[1],0); InterlockedExchange(&JNETMesh[2],0);
    InterlockedExchangePointer(&guarded_mesh,(void *)(uintptr_t)(traveler_object+0x160));
    traveler_mesh_report("before-extra",0,traveler_barn+0x210);
    traveler_mesh_report("before-extra",1,traveler_barn+0x14850);
    flight_mark(120,1);
    ++JNETTraveler[4];
    original_full_init(traveler_object,manager,effects,unused,lua,random,name,sound,decorations,remote);
    traveler_mesh_report("after-extra",0,traveler_barn+0x210);
    traveler_mesh_report("after-extra",1,traveler_barn+0x14850);
    traveler_mesh_report("after-extra",2,traveler_object+0x160);
    int glyph_initialized=traveler_glyph_init_end();
    InterlockedExchangePointer(&guarded_mesh,NULL);
    flight_mark(121,1);
    if(!glyph_initialized){JNETTraveler[16]=traveler_glyph_init.failed;code=5;goto done;}
    if (mesh_fault || !jc_mesh_guard_complete(JNETMesh[1],JNETMesh[2])) {
        JNETTraveler[16]=body_context->mesh_guard_failure; code=11; goto done;
    }
    stage=5;
    code=jc_traveler_identity_read(own_read,NULL,image,traveler_object,&traveler_identity); if (code) goto done;
    unsigned char remote_flag=0;
    if (!own_read(NULL,traveler_object+0x81,&remote_flag,1) || remote_flag!=1) { code=2; goto done; }
    if (!traveler_context() || !traveler_originals_match(image) ||
        !jc_layout_field(own_read,NULL,decorations,0x434010,&after,4)) { code=3; goto done; }
    JNETTraveler[8]=(LONG)after;
    unsigned live_sounds=0;
    code=jc_traveler_added(traveler_original,&traveler_identity,decorations,before,after,&live_sounds);
    JNETTraveler[9]=(LONG)live_sounds; if (code) goto done;
    if (jc_geometry_pool_read(own_read,NULL,dude+0x1e0,&pool) ||
        pool.source!=traveler_geometry_source || pool.slots!=4 || pool.occupied!=3 || pool.free_slots!=1) {
        code=17; goto done;
    }
    const uint64_t meshes[]={traveler_barn+0x210,traveler_barn+0x14850,traveler_object+0x160};
    int distinct=0,models=0,roots=0;
    code=jc_three_clone_check(own_read,NULL,image,meshes,&distinct);
    JNETTraveler[16]=code; JNETTraveler[10]=!code && distinct; if (code || !distinct) { code=12; goto done; }
    code=jc_three_pose_storage(own_read,NULL,image,meshes,&models,&roots);
    JNETTraveler[16]=code; JNETTraveler[11]=!code && models && roots; if (code || !models || !roots) { code=13; goto done; }
    struct jc_animation_workspace *aw=HeapAlloc(GetProcessHeap(),0,sizeof(*aw));
    struct jc_animation_result ar;
    code=jc_three_animation_storage(own_read,NULL,image,meshes,&ar,aw);
    if (aw) HeapFree(GetProcessHeap(),0,aw);
    JNETTraveler[16]=code; JNETTraveler[12]=!code && ar.disjoint; if (code || !ar.disjoint) { code=14; goto done; }
    struct jc_callback_snapshot now;
    code=traveler_callback_snapshot(manager,&now);
    JNETTraveler[32]=code;
    JNETTraveler[35]=(LONG)now.owners[0].total;
    JNETTraveler[36]=(LONG)now.owners[1].total;
    JNETTraveler[37]=(LONG)now.owners[2].total;
    if (code) { JNETTraveler[39]=1; code=15; }
    else if (!jc_callback_equal(&now.owners[0],&traveler_callbacks.owners[0]) ||
        !jc_callback_equal(&now.owners[1],&traveler_callbacks.owners[1])) { JNETTraveler[39]=2; code=15; }
    else {
        uint64_t native_source=0,extra_source=0;
        JNETTraveler[38]=traveler_callback_source(meshes[1],&native_source) &&
            traveler_callback_source(meshes[2],&extra_source) && native_source==extra_source;
        if (!JNETTraveler[38]) { JNETTraveler[39]=3; code=15; }
        /* 262b00 registers matching resource subscribers; zero is valid for both. */
        else if (!jc_callback_equal(&now.owners[1],&now.owners[2])) { JNETTraveler[39]=4; code=15; }
    }
    JNETTraveler[13]=!code;
 done:
    traveler_result(code ? (traveler_object ? -2 : -1) : 3,stage,code);
    ReleaseSRWLockExclusive(&lifetime_lock);
    return result;
}
static void traveler_cleanup(uintptr_t dude,uintptr_t sound) {
    if(dude==traveler_admission_barn+0xb0 || dude==traveler_admission_barn+0x146f0)traveler_admission_ready=0;
    if (JNETTraveler[0]!=3 || dude!=traveler_barn+0xb0) return;
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL); uint64_t manager=0,value=0;
    struct jc_traveler_identity now;
    if (!traveler_context() || sound!=traveler_sound || !traveler_originals_match(image) ||
        jc_traveler_identity_read(own_read,NULL,image,traveler_object,&now) ||
        memcmp(&now,&traveler_identity,sizeof(now)) ||
        !own_read(NULL,traveler_game+0x138,&value,8) || value!=traveler_decorations ||
        !own_read(NULL,traveler_game+0x28,&manager,8) || !manager) { traveler_result(-2,6,3); return; }
    struct jc_geometry_pool cleanup_pool;
    struct jc_geometry_slots cleanup_slots;
    uint64_t material=traveler_object+0x1e0,links[3];
    int geometry_code=jc_geometry_pool_read_slots(own_read,NULL,material,&cleanup_pool,&cleanup_slots);
    if (!geometry_code && !own_read(NULL,material+0x30,links,sizeof(links))) geometry_code=JC_LAYOUT_READ;
    if (!geometry_code && (links[1] || links[2])) geometry_code=JC_LAYOUT_TYPE;
    JNETTraveler[43]=1; JNETTraveler[42]=geometry_code;
    if (geometry_code) {
        traveler_result(-2,6,17); return;
    }
    JNETTraveler[40]=(LONG)cleanup_pool.occupied;
    if (!traveler_result(2,6,0)) return;
    flight_mark(122,1); ++JNETTraveler[5];
    original_cleanup(traveler_object,sound);
    flight_mark(123,1);
    /* Native cleanup leaves the glyph pointer for the imminent chapter pool reset. */
    const uint64_t empty[]={0x168,0x190,0x6d8,0x6e0,0x9b40,0x9bb0};
    for (unsigned i=0;i<sizeof(empty)/sizeof(empty[0]);++i)
        if (!own_read(NULL,traveler_object+empty[i],&value,8) || value) { traveler_result(-2,7,16); return; }
    for (unsigned i=0;i<14;++i) {
        uint64_t links[3];
        if (!own_read(NULL,traveler_object+0x9bb0+i*0x90,links,sizeof(links)) || links[0] || links[1] || links[2]) {
            traveler_result(-2,7,16); return;
        }
    }
    const uint64_t type_offsets[]={0,0x160,0x7b0,0xe50,0xa390};
    const uint64_t type_rvas[]={0x5d0380,0x67f3f8,0x67f3f8,0x67f3f8,0x5d0360};
    for (unsigned i=0;i<5;++i)
        if (!own_read(NULL,traveler_object+type_offsets[i],&value,8) || value!=image+type_rvas[i]) {
            traveler_result(-2,7,2); return;
        }
    struct jc_callback_snapshot callbacks;
    if (traveler_callback_snapshot(manager,&callbacks) || callbacks.owners[2].total ||
        !jc_callback_equal(&callbacks.owners[0],&traveler_callbacks.owners[0]) ||
        !jc_callback_equal(&callbacks.owners[1],&traveler_callbacks.owners[1]) ||
        !traveler_originals_match(image)) { traveler_result(-2,7,15); return; }
    uint32_t occupied=0;
    geometry_code=jc_geometry_released(own_read,NULL,&cleanup_pool,&cleanup_slots,material,&occupied);
    JNETTraveler[43]=2; JNETTraveler[42]=geometry_code; JNETTraveler[41]=(LONG)occupied;
    if (geometry_code) {
        traveler_result(-2,7,17); return;
    }
    if(!traveler_body_release(traveler_object)) {traveler_result(-2,7,16);return;}
    JNETTraveler[14]=1;
    flight_mark(124,1); ++JNETTraveler[6]; trial_destroy((void *)image,traveler_object);
    if(!traveler_glyph_init_release(traveler_object)) {traveler_result(-2,8,5);return;}
    traveler_object=0; flight_mark(125,0); traveler_result(4,8,0);
}
#endif
