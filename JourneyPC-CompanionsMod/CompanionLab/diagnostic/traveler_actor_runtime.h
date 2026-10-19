#ifndef JC_TRAVELER_ACTOR_RUNTIME_H
#define JC_TRAVELER_ACTOR_RUNTIME_H
#include "native_actor_runtime.h"
#include "native_actor_material.h"
#include "native_actor_departure.h"
#include "native_peer_lifecycle.h"
#include "traveler_stream_test.h"

static void traveler_local_before_input(uintptr_t barn,uintptr_t game);
static void traveler_local_after_update(void);
static void (*traveler_character_input)(uintptr_t,uintptr_t);
static void (*traveler_character_updated)(uintptr_t,uintptr_t);
static void traveler_native_chirp_tick(void);
static void traveler_charge_before_update(uintptr_t barn,uintptr_t game);
static void traveler_charge_after_update(void);
static int traveler_peer_admit(uint64_t peer,uint64_t generation);
static int traveler_peer_vacant(void);
static int traveler_peer_fault(int code);
static uint32_t traveler_repeat_outfit(void);
static int traveler_body_prepare(const struct jc_actor_identity *id);
static int traveler_peer_before_input(void);
static void traveler_peer_after_input(void);
static void traveler_peer_after_update(void);
static unsigned traveler_peer_depart_requested,traveler_peer_restart_requested;

static uintptr_t (*original_traveler_input)(uintptr_t,uintptr_t);
static uintptr_t (*original_traveler_update)(uintptr_t,uintptr_t);
static uintptr_t (*original_traveler_finish)(uintptr_t);
static struct jc_actor_runtime *traveler_actor;
static struct jc_actor_runtime_ops traveler_actor_services;
static struct jc_actor_material_proof traveler_actor_materials;
static unsigned traveler_actor_material_hooks_ready,traveler_actor_material_scope,traveler_actor_material_failed;
static struct {
    uint64_t image,thread;
    unsigned char position[16],glow[4];
    unsigned active;
} traveler_actor_world;
static int traveler_actor_world_capture(const struct jc_actor_identity *id) {
    if(traveler_actor_world.active || !traveler_actor || !jc_actor_runtime_thread(traveler_actor))return 0;
    unsigned char position[16],glow[4];
    if(!own_read(NULL,id->image+0x3ce2320,traveler_actor_world.position,16) ||
       !own_read(NULL,id->image+0x3c941d0,traveler_actor_world.glow,4) ||
       !own_read(NULL,id->image+0x3ce2320,position,16) ||
       !own_read(NULL,id->image+0x3c941d0,glow,4) ||
       memcmp(position,traveler_actor_world.position,16) || memcmp(glow,traveler_actor_world.glow,4))return 0;
    traveler_actor_world.image=id->image;traveler_actor_world.thread=GetCurrentThreadId();
    traveler_actor_world.active=1;return 1;
}
static int traveler_actor_world_restore(void) {
    if(!traveler_actor_world.active || traveler_actor_world.thread!=GetCurrentThreadId())return 0;
    uint64_t image=traveler_actor_world.image;
    unsigned char position[16],glow[4];
    int a=jc_local_windows_write(image+0x3ce2320,traveler_actor_world.position,16);
    int b=jc_local_windows_write(image+0x3c941d0,traveler_actor_world.glow,4);
    int check=own_read(NULL,image+0x3ce2320,position,16) && own_read(NULL,image+0x3c941d0,glow,4) &&
        !memcmp(position,traveler_actor_world.position,16) && !memcmp(glow,traveler_actor_world.glow,4);
    if(a && b && check)traveler_actor_world.active=0;
    return a && b && check;
}
static int traveler_actor_material_write_allowed(uintptr_t object,unsigned payload) {
    if(traveler_glyph_init.state==3)return 1;
    if(!traveler_glyph_object_allowed(object,payload))return 0;
    if(!traveler_actor)return 1;
    int glyph=object==traveler_actor->identity.glyph+0x90;
    if(!traveler_actor_material_scope && !glyph)return 1;
    if(!jc_actor_runtime_thread(traveler_actor))return glyph?0:1;
    if(traveler_actor_material_failed)return 0;
    int handled=0;
    int code=jc_actor_material_write_check(own_read,NULL,&traveler_actor->identity,object,
        (int)payload,&traveler_actor_materials,&handled);
    if(code && code!=JC_ACTOR_MATERIAL_EMPTY_SETTER) {
        traveler_actor_material_failed=(unsigned)code;
        if(!traveler_actor_material_scope)jc_actor_runtime_quarantine(traveler_actor,code);
    }
    return !code;
}
static int traveler_actor_sampler_write_allowed(uintptr_t material,const uint64_t *model) {
    if(traveler_glyph_init.state==3)return 1;
    if(!traveler_glyph_write_allowed(material,1,model))return 0;
    uint64_t slot=0,pair[2];
    if(model && own_read(NULL,(uintptr_t)model,pair,sizeof(pair)) &&
        own_read(NULL,material+0x30,&slot,8) && !slot)return 1;
    return traveler_actor_material_write_allowed(material,0);
}
__declspec(dllexport) volatile LONG JNETActor[32];
static void traveler_actor_scope_failure(unsigned phase,unsigned step,int code,uint64_t frame) {
    if(JNETActor[13])return;
    InterlockedExchange(&JNETActor[12],(LONG)phase);
    InterlockedExchange(&JNETActor[13],(LONG)step);
    InterlockedExchange(&JNETActor[14],(LONG)code);
    InterlockedExchange(&JNETActor[15],(LONG)frame);
    char line[160];snprintf(line,sizeof(line),"[JC] actor scope failed phase=%u step=%u code=%d frame=%llu",
        phase,step,code,(unsigned long long)frame);message(line);
}
static void traveler_actor_report(int code) {
    static unsigned declines;
    InterlockedExchange(&JNETActor[1],code);
    if (!traveler_actor) return;
    if ((code==JC_ACTOR_RECEIVER || code==JC_ACTOR_WRITE) && (++declines<=8 || declines%300==0)) {
        char line[384];struct jc_actor_publication *p=&traveler_actor->publication;
        snprintf(line,sizeof(line),"[JC] actor numeric rejected count=%u stage=%u detail=%u dt=%.6f/%.6f attempted/restored=%u/%u quarantine=%u state=%u",
            declines,p->receiver_stage,p->detail,p->clocks[0],p->clocks[1],p->attempted_spans,
            p->restored_spans,p->quarantined,traveler_actor->state);
        message(line);
    }
    if(traveler_stream_enabled)for(unsigned i=0;i<2;++i) {
        JNETActor[26+i*3]=(LONG)traveler_streams[i].stream.accepted;
        JNETActor[27+i*3]=(LONG)traveler_streams[i].stream.rejected;
        JNETActor[28+i*3]=(LONG)traveler_streams[i].dropped;
    }
    InterlockedExchange(&JNETActor[0],(LONG)traveler_actor->state);
    InterlockedExchange(&JNETActor[2],(LONG)traveler_actor->input_frame);
    InterlockedExchange(&JNETActor[3],(LONG)traveler_actor->update_frame);
    InterlockedExchange(&JNETActor[4],(LONG)traveler_actor->render_frame);
    InterlockedExchange(&JNETActor[5],(LONG)traveler_actor->receiver.sequence);
    InterlockedExchange(&JNETActor[6],(LONG)traveler_actor->publication.changed_spans);
    InterlockedExchange(&JNETActor[7],(LONG)traveler_actor->publication.quarantined);
    InterlockedExchange(&JNETActor[8],(LONG)traveler_actor->finish_frame);
    InterlockedExchange(&JNETActor[9],(LONG)traveler_actor_material_failed);
    InterlockedExchange(&JNETActor[10],(LONG)traveler_actor_materials.row);
    InterlockedExchange(&JNETActor[11],(LONG)traveler_actor_materials.other);
    if(traveler_actor->state==JC_ACTOR_RUNTIME_QUARANTINED)
        traveler_peer_fault(traveler_actor->last_result?(int)traveler_actor->last_result:code);
}
static uint64_t traveler_actor_thread(void *context) {(void)context;return GetCurrentThreadId();}
static int traveler_actor_write(void *context,uint64_t address,const void *data,size_t bytes) {
    (void)context;
    if (!traveler_actor || !traveler_context() || address<traveler_object ||
        bytes>JC_ACTOR_PUBLICATION_BYTES || address-traveler_object>JC_ACTOR_PUBLICATION_BYTES-bytes) return 0;
    for (size_t i=0;i<bytes;++i)
        if (!jc_actor_numeric_byte((unsigned)(address-traveler_object+i))) return 0;
    return jc_local_windows_write(address,data,bytes);
}
static int traveler_actor_cloth_enable(const struct jc_actor_identity *id) {
    unsigned char enabled=0,check=0;
    if (!traveler_actor || id->actor!=traveler_object || !jc_actor_runtime_thread(traveler_actor) ||
        !traveler_context() || !own_read(NULL,id->actor+0x1479,&enabled,1) || enabled>1) return 0;
    if (enabled) return 1;
    enabled=1;
    return traveler_actor_write(NULL,id->actor+0x1479,&enabled,1) &&
        own_read(NULL,id->actor+0x1479,&check,1) && check==1;
}
static int traveler_actor_ready(void *context,const struct jc_actor_identity *id) {
    return JNETTraveler[0]==3 && traveler_context() && id->actor==traveler_object &&
        active_cloth_backend && jc_cloth_live_owner(active_cloth_backend) &&
        !active_cloth_backend->frames.closing && !cloth_chapter.pending && !cloth_chapter.failed &&
        traveler_actor_services.ready(context,id) &&
        !jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials);
}
static int traveler_actor_begin(void *context,const struct jc_actor_identity *id,
        unsigned phase,uint64_t frame,uint64_t render) {
    if (!traveler_actor_services.begin(context,id,phase,frame,render)) {
        traveler_actor_scope_failure(phase,1,JC_ACTOR_RUNTIME_SCOPE,frame);return 0;
    }
    if (phase==JC_ACTOR_PHASE_UPDATE && !traveler_actor_cloth_enable(id)) {
        traveler_actor_scope_failure(phase,2,JC_ACTOR_WRITE,frame);
        (void)traveler_actor_services.end(context,id,phase,frame,0);
        jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_WRITE);
        return 0;
    }
    int scope_code=phase==JC_ACTOR_PHASE_UPDATE?cloth_actor_scope_begin(id->peer,id->generation):0;
    if (scope_code) {
        traveler_actor_scope_failure(phase,3,scope_code,frame);
        if (!traveler_actor_services.end(context,id,phase,frame,0) && traveler_actor)
            jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE);
        return 0;
    }
    if(phase==JC_ACTOR_PHASE_UPDATE && !traveler_actor_world_capture(id)) {
        traveler_actor_scope_failure(phase,4,JC_ACTOR_RUNTIME_SCOPE,frame);
        (void)cloth_actor_scope_end(id->peer,id->generation,0);
        (void)traveler_actor_services.end(context,id,phase,frame,0);
        jc_actor_runtime_quarantine(traveler_actor,JC_ACTOR_RUNTIME_SCOPE);
        return 0;
    }
    traveler_actor_material_failed=0;traveler_actor_material_scope=phase;
    return 1;
}
static int traveler_actor_end(void *context,const struct jc_actor_identity *id,
        unsigned phase,uint64_t frame,int success) {
    int scope_code=phase==JC_ACTOR_PHASE_UPDATE?cloth_actor_scope_end(id->peer,id->generation,success):0;
    int scope=!scope_code;
    if(!scope)traveler_actor_scope_failure(phase,5,scope_code,frame);
    int world=phase!=JC_ACTOR_PHASE_UPDATE || traveler_actor_world_restore();
    if(!world)traveler_actor_scope_failure(phase,6,JC_ACTOR_RUNTIME_SCOPE,frame);
    int material_code=traveler_actor_material_failed?(int)traveler_actor_material_failed:
        jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials);
    int materials=!material_code;
    if(!materials)traveler_actor_scope_failure(phase,7,material_code,frame);
    traveler_actor_material_scope=0;
    int services=traveler_actor_services.end(context,id,phase,frame,success && scope && materials && world);
    if(!services)traveler_actor_scope_failure(phase,8,JC_ACTOR_RUNTIME_SCOPE,frame);
    return scope && services && materials && world;
}
/* Services are an in-process backend contract, never packet data. Retirement
   must cover torso/glyph/animation/cloth/GPU consumers. Missing services keep
   the retained traveler dormant. No host appearance or synthetic ready proof. */
__declspec(dllexport) int JNETActorActivate(uint64_t peer,uint64_t generation,
        const struct jc_actor_runtime_ops *services) {
    if (traveler_actor || !services || !services->ready || !services->prepare || !services->retired ||
        !services->begin || !services->end || !services->drain || !peer || !generation ||
        !original_traveler_input || !original_traveler_update || !original_traveler_finish ||
        !traveler_actor_material_hooks_ready ||
        JNETTraveler[0]!=3 || !traveler_context())
        return JC_ACTOR_RUNTIME_ARGUMENT;
    uint64_t glyph=0;
    if (!own_read(NULL,traveler_object+0x2240,&glyph,8))return JC_ACTOR_READ;
    struct jc_actor_identity id={(uintptr_t)GetModuleHandleW(NULL),traveler_game,traveler_barn,
        traveler_object,glyph,peer,generation};
    struct jc_actor_runtime *candidate=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*candidate));
    if (!candidate)return JC_ACTOR_RUNTIME_BUSY;
    traveler_actor_services=*services;
    struct jc_actor_runtime_ops ops=*services;
    ops.read=own_read;ops.write=traveler_actor_write;ops.thread=traveler_actor_thread;
    ops.ready=traveler_actor_ready;ops.begin=traveler_actor_begin;ops.end=traveler_actor_end;
    int code=jc_actor_runtime_bind(candidate,&id,&ops);
    if (code) {HeapFree(GetProcessHeap(),0,candidate);memset(&traveler_actor_services,0,sizeof(traveler_actor_services));}
    else traveler_actor=candidate;
    traveler_actor_report(code);return code;
}
__declspec(dllexport) int JNETActorReceive(const struct jc_actor_sender *sender,
        uint64_t peer,uint64_t generation,uint64_t sequence) {
    if (!traveler_actor)return JC_ACTOR_RUNTIME_ARGUMENT;
    int code=jc_actor_runtime_receive(traveler_actor,sender,peer,generation,sequence);
    traveler_actor_report(code);return code;
}
__declspec(dllexport) int JNETActorStop(void) {
    if (!traveler_actor)return 0;
    int code=jc_actor_runtime_stop(traveler_actor);traveler_actor_report(code);return code;
}
__declspec(dllexport) int JNETActorPrepare(uint64_t peer,uint64_t generation);
__declspec(dllexport) volatile LONG JNETActorTrial[5];
static uint64_t traveler_actor_trial_peer,traveler_actor_trial_generation,traveler_actor_trial_sequence;
static uint64_t traveler_actor_trial_started,traveler_actor_trial_serial;
static unsigned traveler_actor_trial_draining,traveler_actor_trial_join_pending;
static uint32_t traveler_actor_trial_symbol;
static uint32_t traveler_actor_trial_outfit=UINT32_MAX,traveler_actor_admission_outfit=UINT32_MAX;
static int traveler_actor_trial_sample(struct jc_actor_sender *sender,unsigned first) {
    int code=0;
    if(!traveler_stream_enabled || first)
        code=jc_actor_capture_sender(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),traveler_game,sender);
    if(!code && traveler_stream_enabled)
        code=traveler_stream_sample(0,first,traveler_actor_trial_peer,traveler_actor_trial_generation,
            traveler_actor_trial_outfit,traveler_actor_trial_symbol,sender);
    if (!code && !traveler_stream_enabled) {
        jc_receiver_store(sender->state.bytes,jc_receiver_float(sender->state.bytes)+2.0f);
        memcpy(sender->state.bytes+0x64,&traveler_actor_trial_symbol,4);
        if(traveler_actor_trial_outfit<=6)
            memcpy(sender->state.bytes+0x60,&traveler_actor_trial_outfit,4);
    }
    return code;
}
static void traveler_actor_trial_stop(int cause) {
    traveler_stream_close(0);
    traveler_actor_trial_join_pending=0;traveler_peer_restart_requested=0;
    if(traveler_actor && traveler_actor->state==JC_ACTOR_RUNTIME_RETIRED) {
        traveler_actor_trial_draining=0;traveler_peer_depart_requested=0;return;
    }
    traveler_actor_trial_draining=1;
    if(cause)JNETActorTrial[1]=cause;
    if(!traveler_actor){traveler_actor_trial_draining=0;JNETActorTrial[0]=-1;return;}
    unsigned request=cause?2:1;
    if(request>traveler_peer_depart_requested)traveler_peer_depart_requested=request;
    JNETActorTrial[0]=2;
}
__declspec(dllexport) int JNETActorTrialStop(void) {
    if(!traveler_context())return JC_ACTOR_RUNTIME_THREAD;
    if(JNETActorTrial[0]==1 || traveler_actor_trial_draining || traveler_actor_trial_join_pending)traveler_actor_trial_stop(0);
    return (int)JNETActorTrial[1];
}
__declspec(dllexport) int JNETActorTrialStart(void) {
    if((JNETActorTrial[0] && JNETActorTrial[0]!=3) ||
        (traveler_actor && !traveler_peer_vacant()) || JNETTraveler[0]!=3 || !traveler_context() ||
        traveler_actor_trial_serial==0xffffff)return JC_ACTOR_RUNTIME_ARGUMENT;
    if(traveler_actor && !traveler_peer_restart_requested) {
        traveler_peer_restart_requested=1;return 0;
    }
    traveler_peer_restart_requested=0;
    for(unsigned i=12;i<16;++i)InterlockedExchange(&JNETActor[i],0);
    if(!traveler_actor_trial_join_pending) {
        uint32_t symbols[2];
        if (!own_read(NULL,traveler_barn+0xb0+0x74,symbols,4) ||
            !own_read(NULL,traveler_barn+0x146f0+0x74,symbols+1,4) || symbols[0]>20 || symbols[1]>20)
            return JC_ACTOR_READ;
        uint32_t previous_symbol=traveler_actor_trial_symbol;
        traveler_actor_trial_symbol=(uint32_t)(traveler_actor_trial_serial%21);
        while (traveler_actor_trial_symbol==symbols[0] || traveler_actor_trial_symbol==symbols[1] ||
            (traveler_actor_trial_serial && traveler_actor_trial_symbol==previous_symbol))
            traveler_actor_trial_symbol=(traveler_actor_trial_symbol+1)%21;
        traveler_actor_trial_outfit=traveler_repeat_outfit();
        uint64_t serial=++traveler_actor_trial_serial;
        traveler_actor_trial_peer=UINT64_C(0x4c4f43414c000000)|serial;
        traveler_actor_trial_generation=serial;traveler_actor_trial_sequence=0;
        traveler_actor_trial_join_pending=1;
    }
    struct jc_actor_sender sender;
    int code=traveler_actor_trial_sample(&sender,1);
    if(code==JC_STREAM_HELD){traveler_peer_restart_requested=1;return 0;}
    traveler_actor_trial_join_pending=0;
    if(code)return code;
    if(!jc_actor_sender_valid(&sender))return JC_RECEIVER_VALUE;
    if(traveler_network_active) {
        traveler_actor_trial_peer=traveler_streams[0].stream.peer;
        traveler_actor_trial_generation=traveler_streams[0].stream.generation;
        memcpy(&traveler_actor_trial_outfit,sender.state.bytes+0x60,4);
        memcpy(&traveler_actor_trial_symbol,sender.state.bytes+0x64,4);
    }
    memcpy(&traveler_actor_admission_outfit,sender.state.bytes+0x60,4);
    code=JNETActorPrepare(traveler_actor_trial_peer,traveler_actor_trial_generation);
    if(!code && traveler_actor_trial_outfit<=6 && !traveler_body_prepare(&traveler_actor->identity))
        code=JC_ACTOR_IDENTITY;
    traveler_actor_trial_sequence=traveler_stream_enabled?traveler_streams[0].stream.sequence:1;
    if(!code)code=JNETActorReceive(&sender,traveler_actor_trial_peer,traveler_actor_trial_generation,
        traveler_actor_trial_sequence);
    if(!code)code=traveler_peer_admit(traveler_actor_trial_peer,traveler_actor_trial_generation);
    if(code) {traveler_actor_trial_stop(code);return code;}
    traveler_actor_trial_started=GetTickCount64();
    JNETActorTrial[0]=1;JNETActorTrial[1]=0;JNETActorTrial[2]=0;
    JNETActorTrial[3]=1;JNETActorTrial[4]=1;
    return 0;
}
/* Opt-in local diagnostic only. Capture belongs to the local test participant;
   real online peers continue to submit their own appearance through Receive. */
static void traveler_actor_trial_tick(uintptr_t barn,uintptr_t game) {
    if(JNETActorTrial[0]!=1 && !traveler_actor_trial_draining)return;
    if(!traveler_context() || barn!=traveler_barn || game!=traveler_game) {
        if(traveler_actor && jc_actor_runtime_thread(traveler_actor))traveler_actor_trial_stop(JC_ACTOR_IDENTITY);
        return;
    }
    if(traveler_actor_trial_draining)return;
    uint64_t now=GetTickCount64();
    if(now<traveler_actor_trial_started){traveler_actor_trial_stop(JC_ACTOR_CHANGED);return;}
    uint64_t elapsed=now-traveler_actor_trial_started;
    uint64_t report_limit=traveler_network_active?INT32_MAX:60000;
    JNETActorTrial[2]=(LONG)(elapsed>report_limit?report_limit:elapsed);
    if(!traveler_network_active && elapsed>=60000){traveler_actor_trial_stop(0);return;}
    if(!traveler_actor || traveler_actor->state!=JC_ACTOR_RUNTIME_ACTIVE) {
        traveler_actor_trial_stop(JC_ACTOR_RUNTIME_ORDER);return;
    }
    struct jc_actor_sender sender;
    int code=traveler_actor_trial_sample(&sender,0);
    if(traveler_stream_enabled && code==JC_STREAM_HELD)return;
    if(traveler_stream_enabled && code==JC_STREAM_TIMEOUT){traveler_actor_trial_stop(0);return;}
    if(!code) {
        if(traveler_actor_trial_sequence==UINT64_MAX)code=JC_ACTOR_REPLAY;
        else {
            traveler_actor_trial_sequence=traveler_stream_enabled?traveler_streams[0].stream.sequence:traveler_actor_trial_sequence+1;
            code=JNETActorReceive(&sender,traveler_actor_trial_peer,traveler_actor_trial_generation,traveler_actor_trial_sequence);
        }
    }
    if(code)traveler_actor_trial_stop(code);
    else {InterlockedExchange(&JNETActorTrial[3],(LONG)traveler_actor_trial_sequence);}
}
static void (*traveler_online_before_input)(uintptr_t);
static uintptr_t traveler_input_observer(uintptr_t barn,uintptr_t game) {
    if(traveler_online_before_input)traveler_online_before_input(game);
    traveler_local_before_input(barn,game);
    uintptr_t result=original_traveler_input(barn,game);
    if(traveler_character_input)traveler_character_input(barn,game);
    if(barn==traveler_barn && game==traveler_game) {
        if(traveler_peer_restart_requested) {
            int code=JNETActorTrialStart();
            if(code){traveler_peer_restart_requested=0;JNETActorTrial[0]=-1;JNETActorTrial[1]=code;traveler_actor_trial_stop(code);}
        }
        if(!traveler_peer_before_input())return result;
    }
    traveler_actor_trial_tick(barn,game);
    if (traveler_actor && barn==traveler_barn && game==traveler_game &&
        traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE) {
        traveler_actor_report(jc_actor_runtime_after_input(traveler_actor,(uint32_t)JNETFrames));
        if(traveler_actor->input_frame==(uint32_t)JNETFrames)traveler_peer_after_input();
    }
    return result;
}
static uintptr_t traveler_update_observer(uintptr_t barn,uintptr_t game) {
    traveler_charge_before_update(barn,game);
    uintptr_t result=original_traveler_update(barn,game);
    if(traveler_character_updated)traveler_character_updated(barn,game);
    if (traveler_actor && barn==traveler_barn && game==traveler_game &&
        traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE && traveler_actor->input_frame==(uint32_t)JNETFrames) {
        traveler_actor_report(jc_actor_runtime_after_update(traveler_actor,(uint32_t)JNETFrames));
        traveler_peer_after_update();
        if(!traveler_actor->departing)traveler_actor_chirp_tick();
    }
    traveler_charge_after_update();
    if(barn==traveler_barn && game==traveler_game)traveler_native_chirp_tick();
    traveler_local_after_update();
    return result;
}
static void traveler_actor_visibility(uintptr_t render) {
    static uint64_t samples;
    if(!jnet_diagnostic_scans)return;
    if (++samples!=1 && samples%300) return;
    uint64_t scan_started=GetTickCount64();
    ++jnet_diagnostic_scan_count;
    traveler_actor_chirp_report();
    traveler_glyph_trace_report();
    uint64_t counts[2][7][4];
    AcquireSRWLockShared(&jc_cloth_render_lock);
    memcpy(counts,jc_cloth_render_counts,sizeof(counts));
    ReleaseSRWLockShared(&jc_cloth_render_lock);
    char line[480];
    for (unsigned role=4;role<6;++role) {
        snprintf(line,sizeof(line),"[JC] visibility cloth role=%u stock registered/dispatch/built/forwarded=%llu/%llu/%llu/%llu private=%llu/%llu/%llu/%llu",
            role,(unsigned long long)counts[0][role][0],(unsigned long long)counts[0][role][1],
            (unsigned long long)counts[0][role][2],(unsigned long long)counts[0][role][3],
            (unsigned long long)counts[1][role][0],(unsigned long long)counts[1][role][1],
            (unsigned long long)counts[1][role][2],(unsigned long long)counts[1][role][3]);
        message(line);
    }
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    uint32_t mask=0,expected=0; float effects[4]={0},position[4]={0},cooldown=0,body_scale=0;
    unsigned char flags[12]={0},symbols[2]={0};
    int readable=own_read(NULL,image+0x403e3b4,&mask,4) && own_read(NULL,image+0x403e3fc,&expected,4) &&
        own_read(NULL,traveler_object+0x2000,effects,16) && own_read(NULL,traveler_object+0x10,position,16) &&
        own_read(NULL,traveler_object+0x50,flags,12) && own_read(NULL,traveler_object+0x1468,&cooldown,4) &&
        own_read(NULL,traveler_object+0x1430,&body_scale,4) && own_read(NULL,traveler_object+0xd40,symbols,1) && own_read(NULL,traveler_object+0x13e0,symbols+1,1);
    snprintf(line,sizeof(line),"[JC] visibility actor sample=%llu read=%d xyz=%.3f/%.3f/%.3f dissolve/glow/snap/level=%.5f/%.5f/%.5f/%.5f symbols=%u/%u flags50=%02x%02x%02x%02x flags54=%02x%02x%02x%02x cooldown=%.4f scale=%.5f mask=%x/%x",
        (unsigned long long)samples,readable,position[0],position[1],position[2],effects[0],effects[1],effects[2],effects[3],
        symbols[0],symbols[1],flags[0],flags[1],flags[2],flags[3],flags[4],flags[5],flags[6],flags[7],cooldown,body_scale,mask,expected);
    message(line);
    struct jc_appearance_plan appearance_plan={0};
    uint64_t definition=0;
    struct callback_read_budget plan_budget={.deadline=GetTickCount64()+10};
    int plan_code=JC_LAYOUT_READ;
    if (callback_read(&plan_budget,traveler_barn+0xb0+0x2238,&definition,8) && definition)
        plan_code=jc_appearance_plan_read(callback_read,&plan_budget,definition,&appearance_plan);
    for (unsigned n=0;n<2;++n) {
        uint64_t actor=n?traveler_object:traveler_barn+0xb0;
        struct jc_appearance_bindings bindings={0};struct jc_appearance_detail detail={0};
        struct callback_read_budget appearance_budget={.deadline=GetTickCount64()+10};
        int code=plan_code;
        if (!code) code=jc_appearance_bindings_read_detail(callback_read,&appearance_budget,actor+0x1e0,
            &appearance_plan,&bindings,&detail);
        else detail.field=-1;
        float values[JC_APPEARANCE_COUNT]={0},glow[6]={0};
        if (!code) for (unsigned i=0;i<JC_APPEARANCE_COUNT;++i)
            if (jc_appearance_kind(i)==0 && (bindings.found&(1u<<i)))
                own_read(NULL,bindings.buffer+bindings.offsets[i],values+i,4);
        int glow_read=own_read(NULL,actor+0x1448,glow,sizeof(glow));
        snprintf(line,sizeof(line),"[JC] appearance actor=%u code/field/parameter=%d/%d/%d found/zero=%x/%x atlas-handles=%llx/%llx detail/mask=%llx/%llx hardness/deco/hsv=%.4f/%.4f/%.4f/%.4f/%.4f glow-read=%d targets=%.4f/%.4f/%.4f current=%.4f/%.4f/%.4f",
            n,code,detail.field,detail.parameter,bindings.found,bindings.zero,
            (unsigned long long)bindings.textures[4].handle,(unsigned long long)bindings.textures[5].handle,
            (unsigned long long)bindings.textures[6].handle,(unsigned long long)bindings.textures[8].handle,
            values[1],values[7],values[9],values[10],values[11],glow_read,
            glow[0],glow[2],glow[4],glow[1],glow[3],glow[5]);
        message(line);
    }
    for(unsigned n=0;n<2;++n) {
        uint64_t actor=n?traveler_object:traveler_barn+0xb0,glyph=0;
        float attachment[16]={0},relative[16]={0},level=0;
        unsigned char pulse=0;struct jc_pose_quality quality={0};struct jc_pose_storage pose={0};
        struct callback_read_budget glyph_budget={.deadline=GetTickCount64()+10};
        int code=jc_pose_storage_read(callback_read,&glyph_budget,image,actor+0xe50,&pose);
        if(!code)code=jc_pose_quality_read(callback_read,&glyph_budget,pose.model,&quality);
        int read=own_read(NULL,actor+0xdd0,attachment,64) && own_read(NULL,actor+0xe10,relative,64) &&
            own_read(NULL,actor+0x2240,&glyph,8) && own_read(NULL,actor+0x64,&level,4) &&
            own_read(NULL,actor+0x1dfc,&pulse,1);
        snprintf(line,sizeof(line),"[JC] glyph actor=%u read=%d pose=%d singular=%u definition=%llx glow=%.4f pulse=%u attachment-diag=%.4f/%.4f/%.4f relative-diag=%.4f/%.4f/%.4f",
            n,read,code,quality.singular,(unsigned long long)glyph,level,pulse,
            attachment[0],attachment[5],attachment[10],relative[0],relative[5],relative[10]);message(line);
        uint32_t identity=0,parameters=0,group=0;unsigned char gates[4]={0};
        int gates_read=own_read(NULL,actor+0x74,&identity,4) &&
            own_read(NULL,glyph+0xf20,&parameters,4) && own_read(NULL,glyph+0x704,&group,4) &&
            own_read(NULL,glyph+0x70c,gates,4);
        snprintf(line,sizeof(line),"[JC] glyph gates actor=%u read=%d symbol=%u parameters=%u group=%u flags=%u/%u/%u/%u",
            n,gates_read,identity,parameters,group,gates[0],gates[1],gates[2],gates[3]);message(line);
        float world[16]={0},shape[2]={0};uint32_t draw_state=0,category=0;unsigned invalid=0;
        int world_read=own_read(NULL,glyph+0x540,world,64) && own_read(NULL,actor+0x1470,shape,4) &&
            own_read(NULL,actor+0x1e64,shape+1,4) && own_read(NULL,glyph+0x6e4,&draw_state,4) &&
            own_read(NULL,glyph+0xf30,&category,4);
        for(unsigned i=0;i<16;++i)invalid+=!isfinite(world[i]);
        struct jc_pose_quality floating_quality={0};struct jc_pose_storage floating_pose={0};
        struct callback_read_budget floating_budget={.deadline=GetTickCount64()+5};
        int floating_code=jc_pose_storage_read(callback_read,&floating_budget,image,glyph+0x10,&floating_pose);
        if(!floating_code)floating_code=jc_pose_quality_read(callback_read,&floating_budget,floating_pose.model,&floating_quality);
        snprintf(line,sizeof(line),"[JC] floating glyph actor=%u read=%d shape=%.5g/%.5g world-invalid=%u xyz=%.3f/%.3f/%.3f draw=%u/%u pose=%d matrices/invalid/singular=%u/%u/%u",
            n,world_read,shape[0],shape[1],invalid,world[12],world[13],world[14],draw_state,category,
            floating_code,floating_quality.count,floating_quality.nonfinite,floating_quality.singular);message(line);
        const uint64_t materials[3]={actor+0x830,actor+0xed0,glyph+0x90};
        for(unsigned role=0;role<3;++role) {
            struct jc_shader_inputs storage={0};
            struct callback_read_budget budget={.deadline=GetTickCount64()+5};
            int result=jc_shader_storage_read(callback_read,&budget,materials[role],&storage);
            snprintf(line,sizeof(line),"[JC] glyph material actor=%u role=%u code=%d shader=%llx bytes=%u",
                n,role,result,(unsigned long long)storage.shader,storage.bytes);message(line);
            if(role==2 && !floating_code && !result)
                traveler_glyph_trace_bind(n,materials[role],floating_pose.model,storage.shader,render);
            if(result)continue;
            const char *keys[6]={"alpha","opacity","alphaFade","scatter","softFadeoutRate","mltColor"};
            float scalars[6]={0};uint64_t texture=0;unsigned found=0;
            for(unsigned table=0;table<2;++table) {
                uint64_t entries=0;uint32_t count=0;unsigned char rows[32*24];char names[32][32];
                if(!callback_read(&budget,storage.shader+0x40+table*16,&count,4) ||
                   !callback_read(&budget,storage.shader+0x48+table*16,&entries,8))break;
                count&=0x7fffffff;
                if(count>32 || !count)continue;
                if(!callback_read(&budget,entries,rows,count*24) ||
                   jc_material_names_read(callback_read,&budget,rows,count,names))break;
                for(unsigned row=0;row<count;++row) {
                    uint16_t offset=0,width=0;unsigned char value[24]={0};
                    memcpy(&offset,rows+row*24+16,2);memcpy(&width,rows+row*24+18,2);width&=0x1fff;
                    if(offset<4 || !width || width>24 || (uint32_t)offset+width>storage.bytes)continue;
                    if(!callback_read(&budget,storage.buffer+offset,value,width))break;
                    for(unsigned k=0;k<6;++k)if(width>=4 && !memcmp(names[row],keys[k],strlen(keys[k])+1))
                        {memcpy(scalars+k,value,4);found|=1u<<k;}
                    if(width==24 && !memcmp(names[row],"tex",4)) {memcpy(&texture,value+16,8);found|=64;}

                }
            }
            snprintf(line,sizeof(line),"[JC] glyph values actor=%u role=%u found=%x alpha/opacity/fade/scatter/soft/colour=%.5g/%.5g/%.5g/%.5g/%.5g/%.5g tex=%llx budget=%d",
                n,role,found,scalars[0],scalars[1],scalars[2],scalars[3],scalars[4],scalars[5],
                (unsigned long long)texture,budget.expired);message(line);
        }
    }
    static const unsigned offsets[3]={0x160,0x7b0,0xe50};
    struct callback_read_budget budget={.deadline=GetTickCount64()+20};
    for (unsigned n=0;n<3;++n) {
        struct jc_pose_storage pose={0};struct jc_pose_quality quality={0};struct jc_render_visibility visibility={0};
        uint64_t queue=0;uint32_t queued=0,hits=0;
        int code=jc_pose_storage_read(callback_read,&budget,image,traveler_object+offsets[n],&pose);
        int visible=code ? code : jc_render_visibility_read(callback_read,&budget,pose.model,mask,expected,&visibility);
        int bones=code ? code : jc_pose_quality_read(callback_read,&budget,pose.model,&quality);
        int qr=own_read(NULL,render+(n ? 0x60 : 0x18),&queue,8) && queue && own_read(NULL,queue+0x10080,&queued,4) && queued<=4096;
        if (qr && !code) for (uint32_t i=0;i<queued;++i) {
            uint64_t model=0;
            if (!callback_read(&budget,queue+0x80+16*i,&model,8)) {qr=0;break;}
            hits+=model==pose.model;
        }
        snprintf(line,sizeof(line),"[JC] visibility mesh=%u layout=%d model=%llx queue-read/count/hits=%d/%u/%u visibility=%d parts/extent/mask/cull=%u/%u/%d/%d pose=%d matrices/nonfinite/singular=%u/%u/%u budget=%d",
            n,code,(unsigned long long)pose.model,qr,queued,hits,visible,visibility.parts,visibility.nondegenerate,
            visibility.mask_pass,visibility.culling,bones,quality.count,quality.nonfinite,quality.singular,budget.expired);
        message(line);
    }
    if (active_cloth_backend) for (unsigned role=4;role<6;++role) {
        struct jc_cloth_live_material *m=NULL;
        for (unsigned n=0;n<JC_CLOTH_FRAMES*JC_CLOTH_DRAWS;++n) {
            struct jc_cloth_live_material *candidate=active_cloth_backend->materials[n];
            if (candidate && candidate->role==role && candidate->pins) {m=candidate;break;}
        }
        if (!m) continue;
        struct jc_render_visibility visibility={0};struct jc_shader_inputs inputs={0};
        int visible=jc_render_visibility_read(callback_read,&budget,m->model,mask,expected,&visibility);
        int shader=jc_shader_inputs_read(callback_read,&budget,(uintptr_t)m->subscriber+0x88,&inputs);
        snprintf(line,sizeof(line),"[JC] visibility private role=%u model=%llx visibility=%d parts/extent/mask/cull=%u/%u/%d/%d shader-first-binding=%d found/finite/zero=%x/%x/%x dissolve-milli=%ld budget=%d",
            role,(unsigned long long)m->model,visible,visibility.parts,visibility.nondegenerate,visibility.mask_pass,
            visibility.culling,shader,inputs.found,inputs.finite,inputs.zero,(long)inputs.dissolve_milli,budget.expired);
        message(line);
    }
    jnet_diagnostic_scan_last=(LONG)(GetTickCount64()-scan_started);
    if(jnet_diagnostic_scan_last>jnet_diagnostic_scan_peak)jnet_diagnostic_scan_peak=jnet_diagnostic_scan_last;
}
static void traveler_actor_render(uintptr_t barn,uintptr_t render) {
    if (traveler_actor && barn==traveler_barn && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE &&
        traveler_actor->update_frame==(uint32_t)JNETFrames) {
        traveler_actor_report(jc_actor_runtime_after_render(traveler_actor,(uint32_t)JNETFrames,render));
        traveler_actor_visibility(render);
    }
}
static uintptr_t traveler_finish_observer(uintptr_t barn) {
    uintptr_t result=original_traveler_finish(barn);
    if(traveler_actor && barn==traveler_barn && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE &&
        traveler_actor->update_frame==(uint32_t)JNETFrames)
        traveler_actor_report(jc_actor_runtime_after_finish(traveler_actor,(uint32_t)JNETFrames));
    return result;
}
#endif
