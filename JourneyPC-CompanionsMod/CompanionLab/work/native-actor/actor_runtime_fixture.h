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
        if(p->receiver_stage==1 && p->detail==JC_RECEIVER_VALUE) {
            unsigned bad=0;float value=0;
            for(unsigned offset=0x10;offset<=0x64;offset+=4) {
                if(offset>=0x50 && offset<0x5c)continue;
                float candidate=jc_receiver_float(p->before+offset);
                if(!isfinite(candidate) || fabsf(candidate)>1000000.f){bad=offset;value=candidate;break;}
            }
            uint32_t appearance[4];memcpy(appearance,p->before+0x68,sizeof(appearance));
            snprintf(line,sizeof(line),"[JC] actor numeric input actor=%llx peer=%llu bad_float=+%x value=%.9g appearance=%u/%u/%u/%u age=%.9g delay=%.9g cooldown=%.9g inactive=%u",
                (unsigned long long)traveler_actor->identity.actor,(unsigned long long)traveler_actor->identity.peer,
                bad,value,appearance[0],appearance[1],appearance[2],appearance[3],
                jc_receiver_float(traveler_actor->receiver.image+0x28dc4),
                jc_receiver_float(traveler_actor->receiver.image+0x28da0),
                jc_receiver_float(p->before+0x1468),traveler_actor->input.inactive);
            message(line);
        }
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
        JNETTraveler[0]!=3 || !traveler_context()) {
        char line[240];snprintf(line,sizeof(line),"[JNET] extra traveler admission declined stage=activate actor=%u peer=%u generation=%u material_hooks=%u lifecycle=%ld/%ld/%ld context=%u",
            traveler_actor!=NULL,peer!=0,generation!=0,traveler_actor_material_hooks_ready!=0,
            (long)JNETTraveler[0],(long)JNETTraveler[1],(long)JNETTraveler[2],traveler_context()!=0);message(line);
        return JC_ACTOR_RUNTIME_ARGUMENT;
    }
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
    unsigned declined=((JNETActorTrial[0] && JNETActorTrial[0]!=3)?1u:0u) |
        ((traveler_actor && !traveler_peer_vacant())?2u:0u) | (JNETTraveler[0]!=3?4u:0u) |
        (!traveler_context()?8u:0u) | (traveler_actor_trial_serial==0xffffff?16u:0u);
    if(declined){
        char line[320];snprintf(line,sizeof(line),"[JNET] extra traveler admission declined stage=start reasons=%u trial=%ld lifecycle=%ld/%ld/%ld object=%u actor=%u backend=%u thread=%lu/%lu; reasons: 1 trial, 2 occupied, 4 lifecycle, 8 context, 16 serial",
            declined,(long)JNETActorTrial[0],(long)JNETTraveler[0],(long)JNETTraveler[1],(long)JNETTraveler[2],
            traveler_object!=0,traveler_actor!=NULL,active_cloth_backend!=NULL,(unsigned long)GetCurrentThreadId(),(unsigned long)traveler_thread);
        message(line);return JC_ACTOR_RUNTIME_ARGUMENT;
    }
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
static void traveler_actor_visibility(uintptr_t render) {(void)render;}
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
