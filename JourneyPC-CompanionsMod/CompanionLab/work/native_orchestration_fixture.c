
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define jc_render_level original_render_level
#include "native_render_queue.h"
#include "native_read_batch.h"
#undef jc_render_level
#define JC_BODY_COUNT 2
#define JC_POSE_APPEARANCE 8
#define JC_SKEL_CAP 4
typedef int32_t LONG;
typedef uint64_t ULONGLONG;
static ULONGLONG native_playback_resume,native_follow_resume;
static unsigned native_follow_cursor;
static int native_playback_cursor=-1;
struct callback_read_budget { ULONGLONG deadline; unsigned reads,fast; int expired; struct jc_read_batch *batch; };
enum { JC_EVAL_RUN_WAIT=17 };
struct jc_eval_run_result { uint64_t mesh,job,model,control,scheduler; unsigned stage,detail,nodes,bindings,clocks,scheduler_kind,scratch_bytes,temp_bytes; unsigned output[8]; int ready; };
struct jc_eval_layout_result { unsigned stage,owner_index; };
struct jc_skel_layout { unsigned names; struct { uint64_t root,model; struct { uint64_t payload; } clone; } storage; struct { uint64_t bone; } bones[JC_SKEL_CAP]; };
struct jc_eval_layout_workspace { struct jc_skel_layout skeleton[4]; };
struct jc_anim_layout { unsigned unused; };
struct jc_anim_sample { unsigned count,total; };
struct jc_anim_bind_detail { uint64_t payload,expected,actual; uint32_t global_stamp,previous_stamp,armed_stamp,observed_stamp; unsigned field,row,changed; };
struct jc_initialized_detail { unsigned unused; };
struct jc_idle_detail { unsigned unused; };
struct life { uint64_t barn,object; int state; };
struct jc_body_context {
 unsigned index; struct life s_initialized_life; int motion_active; struct jc_anim_layout motion_layout; struct jc_anim_sample applied_animation;
 ULONGLONG native_next; uint64_t native_sequence; unsigned char native_identity[16]; float native_previous[JC_SKEL_CAP][16]; int native_seen;
 unsigned char *queue; struct jc_render_tracking tracking; LONG life_report[152]; float transform[16]; uint64_t sequence; unsigned char identity[16]; unsigned flags,appearance[4];
};
static char last_native_body[480],last_native_output[480];
static void message(const char *line) {
 assert(strlen(line)<480); assert(!strncmp(line,"[JC]",4));
 if(strstr(line,"native body="))strcpy(last_native_body,line);
 if(strstr(line,"native output body="))strcpy(last_native_output,line);
}
static struct jc_body_context body_contexts[2],*body_context;
#define initialized_life (body_context->s_initialized_life)
#define JCProbeMeshLife (body_context->life_report)
#define preview_queue (body_context->queue)
#define preview_tracking (body_context->tracking)
#define preview_applied_transform (body_context->transform)
#define preview_pose_sequence (body_context->sequence)
#define preview_pose_identity (body_context->identity)
#define native_report (JCProbeNativePlayback+body_context->index*24)
static volatile LONG JCProbeNativePlayback[48],JCProbeNativeOutputs[16],JCProbeCompanionFeatures[40],JCProbeDual[8],JCProbeResource,JCProbeFrames;
static int native_playback_mode,native_playback_room,lifetime_conflict,prepared_level=2,lifetime_lock;
static ULONGLONG native_playback_begin,now;
static struct jc_eval_layout_workspace workspace,*native_playback_work=&workspace;
static struct jc_anim_layout native_playback_source;
static _Alignas(16) unsigned char barn_data[0x16000],dude_data[2][0x1000],queue_data[2][0x10090],clone_data[2][0x20],job_data[2][0x80],execution_data[2][0x20];
static unsigned char roots[4][0x60];
static uint32_t refcounts[2]; static uint64_t models[2];
struct calls { int layout,guard,clone,capture,apply,arm,complete,consumed,mesh,submit,finish,release,skin,shader,applied; };
static struct calls calls[2];
#define C (calls[body_context->index])
static unsigned fault_owner;
#define fault(n) ((n) && body_context->index==fault_owner)
static unsigned simulated_cost;
static int decline_layout,decline_guard,bad_model,bad_submit,bad_pair,pending_job,temporary_job,bad_refs,mutate_original,mutate_sibling,lock_fail,expired_layout,expired_guard;
static int arm_failure,bind_skipped,consume_failure,bad_queue_count,bad_queue_pair,binding_refs;
static char order[512]; static unsigned order_count,visit[8],visit_count,depart_calls,close_calls;
struct jc_pose_receiver { int fresh; struct { struct jc_anim_sample animation; float transform[16]; uint64_t sequence; unsigned char identity[16]; unsigned flags,appearance[4]; } sample; };
static struct { struct { struct jc_pose_receiver peers[2]; } room; } pose_link;
static int jc_pose_fresh(const struct jc_pose_receiver *p,uint32_t level,uint64_t time) { (void)level;(void)time;return p->fresh; }
static int jc_anim_valid(const struct jc_anim_sample *s) { return s->total==2 && s->count<=2; }
static void pose_link_depart(void) { ++depart_calls; }
static void pose_link_close(int unused) { (void)unused;++close_calls; }
static void pose_link_applied(unsigned owner) { assert(owner==body_context->index);++C.applied; }
static void event(char c) { assert(order_count+1<sizeof(order));order[order_count++]=c;order[order_count]=0; }
static ULONGLONG GetTickCount64(void) { return now; }
static void *GetModuleHandleW(void *ignored) { (void)ignored;return 0; }
static unsigned GetCurrentThreadId(void) { return 1; }
static LONG InterlockedExchange(volatile LONG *p,LONG n) { LONG old=*p;*p=n;return old; }
static LONG InterlockedIncrement(volatile LONG *p) { return ++*p; }
static int TryAcquireSRWLockExclusive(int *p) { if(fault(lock_fail))return 0;assert(!*p);*p=1;return 1; }
static void ReleaseSRWLockExclusive(int *p) { assert(*p==1);*p=0; }
static void preview_fail(int code) { assert(code==27);JCProbeMeshLife[25]=-1; }
static void body_publish(void) {}
static void flight_mark(int a,int b) { (void)a;(void)b; }
static int own_read(void *ctx,uint64_t at,void *out,size_t bytes) { (void)ctx;assert(at);memcpy(out,(void *)(uintptr_t)at,bytes);return 1; }
static int callback_read(void *ctx,uint64_t at,void *out,size_t bytes) {
 struct callback_read_budget *budget=ctx;assert(!budget->fast);++budget->reads;
 if(at==(uintptr_t)clone_data[body_context->index]+0x18) { assert(bytes==8);++C.clone; }
 return own_read(0,at,out,bytes);
}
static int evaluation_writable(void *ctx,uint64_t at,size_t bytes) { (void)ctx;(void)at;(void)bytes;return 1; }
static int jc_render_level(jc_layout_reader r,void *c,uint64_t g,int *out) { (void)r;(void)c;(void)g;*out=2;return 0; }
static int jc_initialized_validate_detail(const struct life *l,jc_layout_reader r,void *ctx,uint64_t image,uint64_t game,unsigned thread,struct jc_initialized_detail *d) { (void)l;(void)r;(void)ctx;(void)image;(void)game;(void)thread;(void)d;return 0; }
static int jc_eval_layout_inspect(jc_layout_reader read,void *ctx,uint64_t image,const uint64_t *meshes,struct jc_eval_layout_result *out,struct jc_eval_layout_workspace *w) {
 (void)read;(void)image;(void)meshes;++C.layout;event('L');assert(visit_count<8);visit[visit_count++]=body_context->index;
 *out=(struct jc_eval_layout_result){.stage=1,.owner_index=4};
 for(unsigned i=0;i<4;++i) { w->skeleton[i].names=2;w->skeleton[i].storage.root=(uintptr_t)roots[i];w->skeleton[i].bones[0].bone=(uintptr_t)roots[i];w->skeleton[i].bones[1].bone=101+i*4; }
 for(unsigned i=0;i<2;++i) { w->skeleton[i+2].storage.model=(uintptr_t)&models[i]+((fault(bad_model)&&body_context->index==i)?8:0);w->skeleton[i+2].storage.clone.payload=(uintptr_t)clone_data[i]; }
 if(fault(expired_layout))((struct callback_read_budget *)ctx)->expired=1;
 return fault(decline_layout)?15:0;
}
static int jc_eval_run_prepare(jc_layout_reader read,int (*write)(void *,uint64_t,size_t),void *ctx,uint64_t image,const uint64_t *meshes,unsigned owner,struct jc_eval_run_result *out,struct jc_eval_layout_workspace *w) {
 (void)read;(void)write;(void)image;(void)w;unsigned i=body_context->index;assert(owner==2+i);
 assert(C.layout==C.guard+1 && C.mesh==C.guard+C.skin+1 && order_count && order[order_count-1]=='D');
 assert(C.clone==C.layout && C.apply==C.layout && C.arm==C.layout && C.complete==C.layout);++C.guard;event('G');
 *out=(struct jc_eval_run_result){.mesh=meshes[owner],.job=(uintptr_t)job_data[i],.model=(uintptr_t)&models[i],.control=(uintptr_t)&refcounts[i],.scheduler=0x88,.ready=1};
 if(fault(expired_guard))((struct callback_read_budget *)ctx)->expired=1;
 if(fault(decline_guard) && decline_guard==17) {out->ready=0;out->stage=2;out->detail=6;return 17;}
 return fault(decline_guard)?15:0;
}
struct jc_idle_pose { uint64_t model, control, root; uint32_t references; };
static inline int jc_idle_context_valid(const unsigned char *context,const struct jc_idle_pose *inputs) {
    uint32_t count; memcpy(&count,context+0x10080,4);
    if (count>1) return 0;
    for (unsigned i=0;i<4096;++i) {
        uint64_t pair[2]; memcpy(pair,context+0x80+i*16,sizeof(pair));
        if (i<count) {
            if (pair[0]!=inputs->model || pair[1]!=inputs->control) return 0;
        } else if (pair[0] || pair[1]) return 0;
    }
    return 1;
}

static int jc_mesh_pose_ready_detail(jc_layout_reader r,void *ctx,uint64_t image,uint64_t mesh,struct jc_idle_pose *out,struct jc_idle_detail *d,int active) { (void)r;(void)ctx;(void)image;(void)mesh;(void)d;(void)active;unsigned i=body_context->index;*out=(struct jc_idle_pose){(uintptr_t)&models[i],(uintptr_t)&refcounts[i],(uintptr_t)roots[i+2],refcounts[i]};return 0; }
static int jc_anim_capture(jc_layout_reader r,void *ctx,uint64_t i,uint64_t m,struct jc_anim_layout *l,struct jc_anim_sample *out) { (void)r;(void)ctx;(void)i;(void)m;(void)l;++C.capture;*out=(struct jc_anim_sample){1,2};return 0; }
static int jc_pose_transform_valid(const float *p) { (void)p;return 1; }
static int jc_skel_matrices(jc_layout_reader r,void *ctx,const struct jc_skel_layout *l,float out[][16]) {
 (void)r;(void)ctx;unsigned owner=(unsigned)(l-workspace.skeleton);memset(out,0,l->names*64);
 if(owner>=2)out[1][0]=(float)calls[owner-2].finish;
 if(fault(mutate_original)&&C.finish&&owner==0)out[1][0]=5;
 if(fault(mutate_sibling)&&C.finish&&owner==3-body_context->index)out[1][0]=9;
 return 0;
}
static int jc_skel_live(jc_layout_reader r,void *ctx,uint64_t image,const struct jc_skel_layout *l) { (void)r;(void)ctx;(void)image;(void)l;return 0; }
static int trial_animation_write(void *ctx,uint64_t at,const void *p,size_t n) { (void)ctx;(void)at;(void)p;(void)n;return 1; }
static int trial_stamp_write(void *ctx,uint64_t at,const void *p,size_t n) { (void)ctx;(void)at;(void)p;assert(n==4);return 1; }
static int jc_anim_apply(jc_layout_reader r,int (*w)(void *,uint64_t,const void *,size_t),void *ctx,uint64_t i,uint64_t m,const uint64_t *others,struct jc_anim_layout *l,const struct jc_anim_sample *s,int *written) { (void)r;(void)w;(void)ctx;(void)i;(void)m;(void)others;(void)l;(void)s;++C.apply;*written=1;return 0; }
static int jc_anim_bind_arm(jc_layout_reader r,int (*w)(void *,uint64_t,const void *,size_t),void *ctx,uint64_t image,const uint64_t *meshes,unsigned owner,uint64_t model,uint64_t control,struct jc_anim_bind_detail *d) {
 (void)r;(void)w;(void)ctx;(void)image;assert(owner==body_context->index+2 && meshes[owner]==initialized_life.object+0x160);
 assert(model==(uintptr_t)&models[body_context->index] && control==(uintptr_t)&refcounts[body_context->index]);++C.arm;event('A');
 d->global_stamp=d->previous_stamp=42;d->armed_stamp=41;
 if(fault(arm_failure)) {d->field=7;d->row=owner;return 7;}d->changed=1;return 0;
}
static int jc_anim_bind_complete(jc_layout_reader r,void *ctx,uint64_t image,uint64_t mesh,struct jc_anim_bind_detail *d) { (void)r;(void)ctx;(void)image;assert(mesh==initialized_life.object+0x160);++C.complete;event('C');if(fault(bind_skipped)){d->field=9;d->actual=41;return 8;}d->observed_stamp=42;return 0; }
static int jc_anim_consumed_detail(jc_layout_reader r,void *ctx,const struct jc_anim_layout *l,const struct jc_anim_sample *s,struct jc_anim_bind_detail *d) { (void)r;(void)ctx;(void)l;(void)s;++C.consumed;event('D');if(fault(consume_failure)){d->field=12;d->row=1;return 0;}return 1; }
static int trial_shader_prepare(uint64_t m,uint64_t root,int n,const float *p) { (void)m;(void)root;(void)n;(void)p;++C.shader;event('H');return 0; }
static void fake_mesh_update(uintptr_t mesh,void *queue) {
 assert(mesh==initialized_life.object+0x160 && queue==preview_queue);uint32_t count;memcpy(&count,preview_queue+0x10080,4);assert(count==0);
 uint64_t pair[2];memcpy(pair,preview_queue+0x80,16);assert(pair[0]==(uintptr_t)&models[body_context->index] && pair[1]==(uintptr_t)&refcounts[body_context->index]);
 memcpy(roots[body_context->index+2]+0x20,(void *)(mesh+0x530),64);
 count=fault(bad_queue_count)?2:1;memcpy(preview_queue+0x10080,&count,4);++C.mesh;
 if(native_report[2]==103) {event('B');if(fault(bad_queue_pair))preview_queue[0x80]^=8;if(fault(binding_refs))++refcounts[body_context->index];}
 else {++C.skin;event('K');}
}
static void fake_submit(void *context,uintptr_t mesh) { assert(mesh==initialized_life.object+0x160);assert(C.guard>=C.submit+1 && C.guard==C.layout && order_count && order[order_count-1]=='G');++C.submit;event('S');unsigned char *p=context;uint32_t n=fault(bad_submit)?2:1;uint64_t pair[2]={(uintptr_t)&models[body_context->index],(uintptr_t)&refcounts[body_context->index]};memcpy(p+0x108,pair,16);memcpy(p+0x308,&n,4);++refcounts[body_context->index]; }
static void fake_finish(void *context) { now+=simulated_cost; ++C.finish;event('F');unsigned char *p=context;memset(p+0x308,0,4);if(fault(bad_pair))p[0x108]^=8;uint64_t pending=fault(pending_job)?1:0,temp=fault(temporary_job)?1:0;memcpy(job_data[body_context->index]+0x70,&pending,8);memcpy(execution_data[body_context->index]+8,&temp,8);if(fault(bad_refs))++refcounts[body_context->index]; }
static void fake_release(void *pair) { ++C.release;event('R');--refcounts[body_context->index];memset(pair,0,16); }
#define JC_RVA_MeshUpdate ((uintptr_t)fake_mesh_update)
#define JC_RVA_AnimationSubmit ((uintptr_t)fake_submit)
#define JC_RVA_AnimationFinish ((uintptr_t)fake_finish)
#define JC_RVA_ReferenceRelease ((uintptr_t)fake_release)
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
        if (JCProbeMeshLife[25]>0) InterlockedExchange(&JCProbeMeshLife[25],3);
        body_publish();
        char line[480];
        snprintf(line,sizeof(line),"[JC] native body=%u state=%ld result=%ld stage=%ld detail=%ld accepted=%ld changed=%ld submissions=%ld elapsed=%ld reads=%ld",
            i+1,(long)native_report[0],(long)native_report[1],(long)native_report[2],(long)native_report[3],
            (long)native_report[4],(long)native_report[5],(long)native_report[16],(long)native_report[6],(long)native_report[9]);
        message(line);
        volatile LONG *o=JCProbeNativeOutputs+i*8;
        snprintf(line,sizeof(line),"[JC] native output body=%u items=%lu bindings=%lu index=%lu binding=%lu kind=%lu type_rva=%08lx bytes=%lu owners=%lu",
            i+1,(unsigned long)(uint32_t)o[0],(unsigned long)(uint32_t)o[1],(unsigned long)(uint32_t)o[2],
            (unsigned long)(uint32_t)o[3],(unsigned long)(uint32_t)o[4],(unsigned long)(uint32_t)o[5],
            (unsigned long)(uint32_t)o[6],(unsigned long)(uint32_t)o[7]);
        message(line);
    }
    body_context=selected;
    if (JCProbeCompanionFeatures[16]>0) InterlockedExchange(&JCProbeCompanionFeatures[16],3);
    if (native_playback_room) { pose_link_depart(); pose_link_close(0); }
}
static void native_playback_guard_report(const struct jc_eval_run_result *run) {
    InterlockedExchange(&native_report[2],(LONG)run->stage);
    InterlockedExchange(&native_report[3],(LONG)run->detail);
    const unsigned values[]={run->nodes,run->bindings,run->clocks,run->scheduler_kind,run->scratch_bytes,run->temp_bytes};
    for (unsigned i=0;i<6;++i) InterlockedExchange(&native_report[10+i],(LONG)values[i]);
    InterlockedExchange(&native_report[19],run->ready);
    for (unsigned i=0;i<8;++i)
        InterlockedExchange(&JCProbeNativeOutputs[body_context->index*8+i],(LONG)run->output[i]);
}
static void native_playback_body(uintptr_t game) {
    if (native_playback_mode!=1) return;
    native_playback_elapsed();
    if (JCProbeMeshLife[25]<0 || JCProbeDual[0]!=1) { native_playback_end(-1,20); return; }
    if (GetTickCount64()>=native_playback_begin+native_playback_duration()) { native_playback_end(3,0); return; }
    if (JCProbeMeshLife[25]==3) { native_playback_end(3,0); return; }
    if (GetTickCount64()<body_context->native_next || GetTickCount64()<native_playback_begin || !preview_queue) return;
    const struct jc_pose_receiver *peer=native_playback_room ? &pose_link.room.peers[body_context->index] : NULL;
    if (peer && (!jc_pose_fresh(peer,(uint32_t)prepared_level,GetTickCount64()) ||
        (peer->sample.sequence==body_context->native_sequence &&
         !memcmp(peer->sample.identity,body_context->native_identity,16)))) return;
    if (peer) {
        unsigned at=30+body_context->index*5;
        InterlockedExchange(&JCProbeCompanionFeatures[at],(peer->sample.flags&JC_POSE_APPEARANCE)!=0);
        for (unsigned j=0;j<4;++j) InterlockedExchange(&JCProbeCompanionFeatures[at+1+j],(LONG)peer->sample.appearance[j]);
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
    if (lifetime_conflict || initialized_life.state!=1 || JCProbeResource!=5 ||
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
    if (lifetime_conflict || initialized_life.state!=1 || JCProbeResource!=5 || JCProbeDual[0]!=1 ||
        JCProbeMeshLife[25]<0 ||
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
    InterlockedIncrement(&JCProbeCompanionFeatures[14+body_context->index]); code=0;
done:
    if (budget.expired) code=9;
    LONG cost=(LONG)(GetTickCount64()-started); unsigned i=body_context->index;
    InterlockedExchange(&JCProbeCompanionFeatures[22+i],cost);
    if (cost>JCProbeCompanionFeatures[24+i]) InterlockedExchange(&JCProbeCompanionFeatures[24+i],cost);
    InterlockedExchange(&JCProbeCompanionFeatures[26+i],code);
    InterlockedExchange(&JCProbeCompanionFeatures[28+i],(LONG)budget.reads);
    ReleaseSRWLockExclusive(&lifetime_lock);
    if (code) { InterlockedExchange(&native_report[2],109); InterlockedExchange(&native_report[3],code); native_playback_end(-1,code); }
}
static void native_chirp_tick(uintptr_t game) { (void)game; }
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
    unsigned first=native_playback_cursor>=0 ? (unsigned)native_playback_cursor : (unsigned)JCProbeFrames%JC_BODY_COUNT;
    native_playback_cursor=-1;
    for (unsigned n=0;n<JC_BODY_COUNT && native_playback_mode==1;++n) {
        body_context=body_contexts+(first+n)%JC_BODY_COUNT;
        ULONGLONG started=GetTickCount64();
        native_playback_body(game);
        ULONGLONG ended=GetTickCount64(),cost=ended-started;
        if (cost>=8) {
            /* Yield normal game frames; never run two costly inspections back-to-back. */
            native_playback_resume=ended+cost*3;
            InterlockedExchange(&JCProbeCompanionFeatures[13],(LONG)(cost*3));
            native_playback_cursor=(int)(1-body_context->index);
            break;
        }
    }
    body_context=body_contexts;
    native_chirp_tick(game);
}

static void reset(void) {
 memset(barn_data,0,sizeof(barn_data));
 memset((void *)JCProbeNativePlayback,0,sizeof(JCProbeNativePlayback));memset((void *)JCProbeCompanionFeatures,0,sizeof(JCProbeCompanionFeatures));memset(&workspace,0,sizeof(workspace));memset(queue_data,0,sizeof(queue_data));memset(job_data,0,sizeof(job_data));memset(execution_data,0,sizeof(execution_data));
 memset(body_contexts,0,sizeof(body_contexts));memset(dude_data,0,sizeof(dude_data));memset(clone_data,0,sizeof(clone_data));memset(calls,0,sizeof(calls));memset(&pose_link,0,sizeof(pose_link));
 native_playback_resume=0;native_playback_cursor=-1;native_follow_resume=0;native_follow_cursor=0;simulated_cost=0;
 native_playback_mode=1;native_playback_room=0;native_playback_begin=1000;now=1000;JCProbeDual[0]=1;JCProbeResource=5;JCProbeFrames=0;
 decline_layout=decline_guard=bad_model=bad_submit=bad_pair=pending_job=temporary_job=bad_refs=mutate_original=mutate_sibling=lock_fail=expired_layout=expired_guard=0;
 arm_failure=bind_skipped=consume_failure=bad_queue_count=bad_queue_pair=binding_refs=0;
 lifetime_lock=0;fault_owner=0;order_count=visit_count=depart_calls=close_calls=0;order[0]=0;
 for(unsigned i=0;i<2;++i) {
  body_context=body_contexts+i;body_context->index=i;initialized_life=(struct life){(uintptr_t)barn_data,(uintptr_t)dude_data[i],1};body_context->native_next=1000;preview_queue=queue_data[i];JCProbeMeshLife[25]=1;native_report[0]=1;
  refcounts[i]=3;uint64_t pair[2]={(uintptr_t)&models[i],(uintptr_t)&refcounts[i]};memcpy(queue_data[i]+0x80,pair,16);uint64_t execution=(uintptr_t)execution_data[i],job=(uintptr_t)job_data[i];memcpy(job_data[i]+0x68,&execution,8);memcpy(clone_data[i]+0x18,&job,8);
  preview_tracking.ledger=(struct jc_render_ledger){.queue=(uintptr_t)queue_data[i],.model=pair[0],.control=pair[1],.generation=1,.slots={1}};preview_tracking.held=1;preview_tracking.acquired=1;
  pose_link.room.peers[i].fresh=1;pose_link.room.peers[i].sample.animation=(struct jc_anim_sample){1,2};pose_link.room.peers[i].sample.sequence=10+i;pose_link.room.peers[i].sample.transform[12]=(float)(20+i);
 }
 body_context=body_contexts;
}
static void check_failed(int code) { assert(native_report[0]==-1 && native_report[1]==code && native_report[4]==0);assert(native_playback_mode==2 && !lifetime_lock); }
int main(void) {
 unsigned scenarios=0;
 for(unsigned owner=0;owner<2;++owner) {
  for(unsigned mode=0;mode<22;++mode) {
   reset();fault_owner=owner;body_context=body_contexts+owner;
   switch(mode) {
    case 0:decline_layout=1;break;case 1:decline_layout=expired_layout=1;break;
    case 2:decline_guard=1;break;case 3:decline_guard=expired_guard=1;break;
    case 4:memset(clone_data[owner]+0x18,0,8);break;case 5:bad_model=1;break;
    case 6:bad_submit=1;break;case 7:bad_pair=1;break;case 8:pending_job=1;break;
    case 9:temporary_job=1;break;case 10:bad_refs=1;break;case 11:mutate_original=1;break;
    case 12:lock_fail=1;break;case 13:arm_failure=1;break;case 14:bind_skipped=1;break;
    case 15:consume_failure=1;break;case 16:bad_queue_count=1;break;case 17:bad_queue_pair=1;break;
    case 18:binding_refs=1;break;case 19:mutate_sibling=1;break;
    case 20:break;case 21:native_playback_room=1;break;
   }
   unsigned char untouched_sibling[sizeof(dude_data[0])],untouched_queue[sizeof(queue_data[0])];
   memcpy(untouched_sibling,dude_data[1-owner],sizeof(untouched_sibling));memcpy(untouched_queue,queue_data[1-owner],sizeof(untouched_queue));
   native_playback_body(1);++scenarios;
   assert(!memcmp(untouched_sibling,dude_data[1-owner],sizeof(untouched_sibling)) && !memcmp(untouched_queue,queue_data[1-owner],sizeof(untouched_queue)));
   assert(!calls[1-owner].layout && !calls[1-owner].submit);
   if(mode<20) {
    const int expected[]={15,9,15,9,21,21,24,24,25,25,25,25,20,23,23,23,23,23,23,25};
    check_failed(expected[mode]);
    if(mode<=1 || mode==4 || mode==5 || mode==12)assert(!C.apply && !C.arm && !C.mesh && !C.submit);
    if(mode==2 || mode==3)assert(C.guard==1 && C.mesh==1 && !C.submit && refcounts[owner]==3);
    if(mode==6)assert(C.finish==1 && C.release==1 && !C.skin && refcounts[owner]==3);
    if(mode==7)assert(C.finish==1 && !C.release && !C.skin && refcounts[owner]==4);
    if(mode==8 || mode==9 || mode==10)assert(C.finish==1 && C.release==1 && !C.skin);
    if(mode==11 || mode==19)assert(C.finish==1 && C.release==1 && C.skin==1 && native_report[18]==25);
    if(mode>=13 && mode<=18) {
     const int details[]={7,9,12,101,102,104};assert(native_report[2]==103 && native_report[3]==details[mode-13]);assert(!C.guard && !C.submit);
    }
   } else {
    assert(!strcmp(order,"LABCDGSFRKH") && C.layout==1 && C.guard==1 && native_report[4]==1 && native_report[16]==1 && native_report[21]==1);
    assert(refcounts[owner]==3 && preview_tracking.held==1 && preview_tracking.reused==1 && !lifetime_lock);
    uint32_t count;memcpy(&count,preview_queue+0x10080,4);assert(!count);
    if(mode==21)assert(!C.capture && C.applied==1 && body_context->native_sequence==10+owner && preview_applied_transform[12]==(float)(20+owner));
   }
  }
 }
 for(unsigned owner=0;owner<2;++owner) {
  reset();fault_owner=owner;body_context=body_contexts+owner;decline_guard=17;
  native_playback_body(1);++scenarios;
  assert(native_playback_mode==1 && native_report[1]==17 && !C.submit && !C.finish && !C.release && refcounts[owner]==3 && !lifetime_lock);
  uint32_t count;memcpy(&count,preview_queue+0x10080,4);assert(!count);
  decline_guard=0;now+=100;native_playback_body(1);++scenarios;
  assert(native_playback_mode==1 && native_report[1]==0 && native_report[4]==1 && C.submit==1 && C.finish==1 && refcounts[owner]==3);
 }
 reset();decline_guard=17;expired_guard=1;native_playback_body(1);++scenarios;
 check_failed(9);assert(!C.submit && !C.finish && refcounts[0]==3);
 reset();native_playback_room=1;decline_guard=17;native_playback_tick(1);++scenarios;
 assert(native_playback_mode==1 && !calls[0].submit && calls[1].submit==1 && !depart_calls && !close_calls && !calls[0].applied);
 decline_guard=0;now+=100;native_playback_tick(1);++scenarios;
 assert(calls[0].submit==1 && calls[0].applied==1 && calls[1].submit==1 && !depart_calls);
 reset();JCProbeCompanionFeatures[8]=2;native_playback_tick(1);++scenarios;
 assert(calls[0].submit==1 && calls[1].submit==1);
 reset();native_playback_tick(1);++scenarios;assert(visit_count==2 && visit[0]==0 && visit[1]==1 && body_context==body_contexts);assert(calls[0].submit==1 && calls[1].submit==1);
 now+=100;JCProbeFrames=1;native_playback_tick(1);++scenarios;assert(visit_count==4 && visit[2]==1 && visit[3]==0);assert(JCProbeNativePlayback[4]==2 && JCProbeNativePlayback[28]==2 && JCProbeNativePlayback[5]==1 && JCProbeNativePlayback[29]==1);
 reset();fault_owner=1;JCProbeFrames=1;decline_layout=1;native_playback_tick(1);++scenarios;assert(calls[1].layout==1 && !calls[0].layout && JCProbeNativePlayback[24]==-1 && JCProbeNativePlayback[0]==2);
 reset();native_playback_room=1;pose_link.room.peers[0].fresh=0;native_playback_tick(1);++scenarios;assert(!calls[0].layout && calls[1].submit==1 && calls[1].applied==1);
 now+=100;native_playback_tick(1);++scenarios;assert(calls[1].submit==1);pose_link.room.peers[0].fresh=1;pose_link.room.peers[1].sample.sequence++;now+=100;native_playback_tick(1);++scenarios;assert(calls[0].submit==1 && calls[1].submit==2 && calls[1].applied==2);
 reset();native_playback_room=1;decline_guard=1;native_playback_tick(1);++scenarios;assert(depart_calls==1 && close_calls==1 && !calls[1].layout);
 reset();native_playback_room=1;native_playback_body(1);now+=100;pose_link.room.peers[0].sample.identity[0]=1;preview_pose_identity[0]=1;native_playback_body(1);++scenarios;assert(calls[0].submit==2 && calls[0].applied==2 && body_context->native_identity[0]==1);
 reset();native_playback_room=1;pose_link.room.peers[0].sample.animation.count=0;native_playback_body(1);++scenarios;assert(calls[0].submit==1 && !calls[0].capture && native_report[17]==0);
 reset();native_playback_room=1;pose_link.room.peers[0].sample.animation.total=0;native_playback_body(1);++scenarios;check_failed(22);assert(!C.apply && !C.submit);
 reset();now=60999;native_playback_body(1);now=61000;native_playback_body(1);++scenarios;assert(native_report[0]==3 && native_report[6]==60000 && native_playback_mode==2);
 reset();native_playback_room=1;now=181000;native_playback_body(1);++scenarios;assert(native_report[0]==3 && native_report[6]==180000 && depart_calls==1);
 reset();native_playback_end(2,0);native_playback_tick(1);++scenarios;assert(JCProbeNativePlayback[0]==2 && JCProbeNativePlayback[24]==2 && !calls[0].submit && !calls[1].submit);
 reset();body_context=body_contexts+1;native_report[2]=4;native_report[3]=22535;JCProbeNativeOutputs[13]=0x72c720;
 native_playback_end(-1,15);++scenarios;
 assert(strstr(last_native_body,"body=2 state=-1 result=15 stage=4 detail=22535"));
 assert(strstr(last_native_output,"body=2") && strstr(last_native_output,"type_rva=0072c720"));
 reset();simulated_cost=250;native_playback_tick(1);++scenarios;
 assert(calls[0].submit==1 && !calls[1].submit && native_playback_resume==2000 && native_playback_cursor==1);
 now=1500;native_playback_tick(1);++scenarios;assert(!calls[1].submit);
 now=2000;native_playback_tick(1);++scenarios;assert(calls[0].submit==1 && calls[1].submit==1 && native_playback_cursor==0);
 now=61000;native_playback_tick(1);++scenarios;assert(native_playback_mode==2);
 reset();native_playback_tick(1);native_playback_resume=now+1000;
 for (unsigned frame=0;frame<20;++frame) {
  now+=16;float moved=(float)(frame+10);memcpy(barn_data+0x210+0x530+48,&moved,4);
  native_playback_tick(1);
 }
 ++scenarios;assert(JCProbeCompanionFeatures[14]==10 && JCProbeCompanionFeatures[15]==10);
 assert(calls[0].submit==1 && calls[1].submit==1);

 for(unsigned owner=0;owner<2;++owner) {
  reset();body_context=body_contexts+owner;native_playback_body(1);
  float moved=40;memcpy(barn_data+0x210+0x530+48,&moved,4);
  unsigned submits=C.submit,applies=C.apply,guards=C.guard;
  native_follow_body(1);++scenarios;
  assert(native_playback_mode==1 && JCProbeCompanionFeatures[14+owner]==1);
  assert(preview_applied_transform[12]==(owner?37:43));
  assert(C.submit==submits && C.apply==applies && C.guard==guards);
  uint32_t count;memcpy(&count,preview_queue+0x10080,4);assert(!count && refcounts[owner]==3);
  native_follow_body(1);++scenarios;assert(JCProbeCompanionFeatures[14+owner]==1);
  reset();body_context=body_contexts+owner;native_follow_body(1);++scenarios;assert(!C.mesh);
  reset();body_context=body_contexts+owner;native_playback_body(1);
  memcpy(barn_data+0x210+0x530+48,&moved,4);uint64_t pending=1;memcpy(job_data[owner]+0x70,&pending,8);
  unsigned meshes=C.mesh;native_follow_body(1);++scenarios;
  assert(native_playback_mode==2 && native_report[2]==109 && C.mesh==meshes);
  reset();body_context=body_contexts+owner;native_playback_room=1;native_playback_body(1);
  pose_link.room.peers[owner].sample.transform[12]=99;pose_link.room.peers[owner].fresh=0;
  meshes=C.mesh;native_follow_body(1);++scenarios;assert(C.mesh==meshes && native_playback_mode==1);
  pose_link.room.peers[owner].fresh=1;pose_link.room.peers[owner].sample.identity[0]=1;
  native_follow_body(1);++scenarios;assert(C.mesh==meshes);
  reset();body_context=body_contexts+owner;native_playback_body(1);
  memcpy(barn_data+0x210+0x530+48,&moved,4);fault_owner=owner;bad_queue_count=1;
  native_follow_body(1);++scenarios;
  assert(native_playback_mode==2 && native_report[2]==109 && native_report[1]==23);
  assert(!JCProbeCompanionFeatures[14+owner] && C.submit==1);
  reset();body_context=body_contexts+owner;native_playback_body(1);
  memcpy(barn_data+0x210+0x530+48,&moved,4);fault_owner=owner;lock_fail=1;
  meshes=C.mesh;native_follow_body(1);++scenarios;assert(C.mesh==meshes && native_playback_mode==1);
 }
 printf("native orchestration: %u production-body scenarios passed for both owners, precise binding failures, alternating order and room sequences\n",scenarios);
}
