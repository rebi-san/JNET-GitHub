from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root / 'diagnostic/probe_dll.c').read_text()
start = source.index('static ULONGLONG native_playback_duration(void)')
end = source.index('static int native_playback_start(', start)
production = source[start:end]
chirp_start = production.index('static void native_chirp_tick(')
tick_start = production.index('static void native_playback_tick(', chirp_start)
production = production[:chirp_start] + 'static void native_chirp_tick(uintptr_t game) { (void)game; }\n' + production[tick_start:]
idle = (root / 'diagnostic/native_idle_pose.h').read_text()
idle_start = idle.index('struct jc_idle_pose')
idle_end = idle.index('static inline int jc_idle_floats', idle_start)
prefix = r'''
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
#define JNETMeshLife (body_context->life_report)
#define preview_queue (body_context->queue)
#define preview_tracking (body_context->tracking)
#define preview_applied_transform (body_context->transform)
#define preview_pose_sequence (body_context->sequence)
#define preview_pose_identity (body_context->identity)
#define native_report (JNETNativePlayback+body_context->index*24)
static volatile LONG JNETNativePlayback[48],JNETNativeOutputs[16],JNETCompanionFeatures[40],JNETDual[8],JNETResource,JNETFrames;
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
static void preview_fail(int code) { assert(code==27);JNETMeshLife[25]=-1; }
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
'''
middle = r'''
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
'''
tests = r'''
static void reset(void) {
 memset(barn_data,0,sizeof(barn_data));
 memset((void *)JNETNativePlayback,0,sizeof(JNETNativePlayback));memset((void *)JNETCompanionFeatures,0,sizeof(JNETCompanionFeatures));memset(&workspace,0,sizeof(workspace));memset(queue_data,0,sizeof(queue_data));memset(job_data,0,sizeof(job_data));memset(execution_data,0,sizeof(execution_data));
 memset(body_contexts,0,sizeof(body_contexts));memset(dude_data,0,sizeof(dude_data));memset(clone_data,0,sizeof(clone_data));memset(calls,0,sizeof(calls));memset(&pose_link,0,sizeof(pose_link));
 native_playback_resume=0;native_playback_cursor=-1;native_follow_resume=0;native_follow_cursor=0;simulated_cost=0;
 native_playback_mode=1;native_playback_room=0;native_playback_begin=1000;now=1000;JNETDual[0]=1;JNETResource=5;JNETFrames=0;
 decline_layout=decline_guard=bad_model=bad_submit=bad_pair=pending_job=temporary_job=bad_refs=mutate_original=mutate_sibling=lock_fail=expired_layout=expired_guard=0;
 arm_failure=bind_skipped=consume_failure=bad_queue_count=bad_queue_pair=binding_refs=0;
 lifetime_lock=0;fault_owner=0;order_count=visit_count=depart_calls=close_calls=0;order[0]=0;
 for(unsigned i=0;i<2;++i) {
  body_context=body_contexts+i;body_context->index=i;initialized_life=(struct life){(uintptr_t)barn_data,(uintptr_t)dude_data[i],1};body_context->native_next=1000;preview_queue=queue_data[i];JNETMeshLife[25]=1;native_report[0]=1;
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
 reset();JNETCompanionFeatures[8]=2;native_playback_tick(1);++scenarios;
 assert(calls[0].submit==1 && calls[1].submit==1);
 reset();native_playback_tick(1);++scenarios;assert(visit_count==2 && visit[0]==0 && visit[1]==1 && body_context==body_contexts);assert(calls[0].submit==1 && calls[1].submit==1);
 now+=100;JNETFrames=1;native_playback_tick(1);++scenarios;assert(visit_count==4 && visit[2]==1 && visit[3]==0);assert(JNETNativePlayback[4]==2 && JNETNativePlayback[28]==2 && JNETNativePlayback[5]==1 && JNETNativePlayback[29]==1);
 reset();fault_owner=1;JNETFrames=1;decline_layout=1;native_playback_tick(1);++scenarios;assert(calls[1].layout==1 && !calls[0].layout && JNETNativePlayback[24]==-1 && JNETNativePlayback[0]==2);
 reset();native_playback_room=1;pose_link.room.peers[0].fresh=0;native_playback_tick(1);++scenarios;assert(!calls[0].layout && calls[1].submit==1 && calls[1].applied==1);
 now+=100;native_playback_tick(1);++scenarios;assert(calls[1].submit==1);pose_link.room.peers[0].fresh=1;pose_link.room.peers[1].sample.sequence++;now+=100;native_playback_tick(1);++scenarios;assert(calls[0].submit==1 && calls[1].submit==2 && calls[1].applied==2);
 reset();native_playback_room=1;decline_guard=1;native_playback_tick(1);++scenarios;assert(depart_calls==1 && close_calls==1 && !calls[1].layout);
 reset();native_playback_room=1;native_playback_body(1);now+=100;pose_link.room.peers[0].sample.identity[0]=1;preview_pose_identity[0]=1;native_playback_body(1);++scenarios;assert(calls[0].submit==2 && calls[0].applied==2 && body_context->native_identity[0]==1);
 reset();native_playback_room=1;pose_link.room.peers[0].sample.animation.count=0;native_playback_body(1);++scenarios;assert(calls[0].submit==1 && !calls[0].capture && native_report[17]==0);
 reset();native_playback_room=1;pose_link.room.peers[0].sample.animation.total=0;native_playback_body(1);++scenarios;check_failed(22);assert(!C.apply && !C.submit);
 reset();now=60999;native_playback_body(1);now=61000;native_playback_body(1);++scenarios;assert(native_report[0]==3 && native_report[6]==60000 && native_playback_mode==2);
 reset();native_playback_room=1;now=181000;native_playback_body(1);++scenarios;assert(native_report[0]==3 && native_report[6]==180000 && depart_calls==1);
 reset();native_playback_end(2,0);native_playback_tick(1);++scenarios;assert(JNETNativePlayback[0]==2 && JNETNativePlayback[24]==2 && !calls[0].submit && !calls[1].submit);
 reset();body_context=body_contexts+1;native_report[2]=4;native_report[3]=22535;JNETNativeOutputs[13]=0x72c720;
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
 ++scenarios;assert(JNETCompanionFeatures[14]==10 && JNETCompanionFeatures[15]==10);
 assert(calls[0].submit==1 && calls[1].submit==1);

 for(unsigned owner=0;owner<2;++owner) {
  reset();body_context=body_contexts+owner;native_playback_body(1);
  float moved=40;memcpy(barn_data+0x210+0x530+48,&moved,4);
  unsigned submits=C.submit,applies=C.apply,guards=C.guard;
  native_follow_body(1);++scenarios;
  assert(native_playback_mode==1 && JNETCompanionFeatures[14+owner]==1);
  assert(preview_applied_transform[12]==(owner?37:43));
  assert(C.submit==submits && C.apply==applies && C.guard==guards);
  uint32_t count;memcpy(&count,preview_queue+0x10080,4);assert(!count && refcounts[owner]==3);
  native_follow_body(1);++scenarios;assert(JNETCompanionFeatures[14+owner]==1);
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
  assert(!JNETCompanionFeatures[14+owner] && C.submit==1);
  reset();body_context=body_contexts+owner;native_playback_body(1);
  memcpy(barn_data+0x210+0x530+48,&moved,4);fault_owner=owner;lock_fail=1;
  meshes=C.mesh;native_follow_body(1);++scenarios;assert(C.mesh==meshes && native_playback_mode==1);
 }
 printf("native orchestration: %u production-body scenarios passed for both owners, precise binding failures, alternating order and room sequences\n",scenarios);
}
'''
fixture = root / 'work/native_orchestration_fixture.c'
fixture.write_text(prefix + idle[idle_start:idle_end] + middle + production + tests)
binary = root / 'work/native_orchestration_fixture'
subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-fno-omit-frame-pointer', '-I', str(root / 'diagnostic'),
                str(fixture), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)

helper = (root / 'diagnostic/probe_helper.c').read_text()
report_start = helper.index('static int traveler_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {')
report_end = helper.index('static int skeleton_report(', report_start)
report_fixture = root / 'work/native_report_fixture.c'
report_fixture.write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef int32_t LONG;
typedef void *HANDLE;
#define JC_HELPER_VERSION L"development"
#define JC_ACTOR_TRIAL_RVA 950
#define JC_ACTOR_RVA 900
#define JC_TRAVELER_RVA 800
#define JC_NATIVE_PLAYBACK_RVA 100
#define JC_COMMAND_RVA 200
#define JC_BODIES_RVA 300
#define JC_COMPANION_FEATURES_RVA 400
#define JC_ROOM_RVA 500
#define JC_NATIVE_OUTPUTS_RVA 600
static LONG playback[48],body[48],features[40],room[20],outputs[16],traveler[44],actor[12],actor_trial[5],pending;
static unsigned watch_poll;
static int remote_read(HANDLE p,uintptr_t at,void *out,size_t size) {
 (void)p;
 if(watch_poll>=2)return 0;
 if(at==950) {assert(size==sizeof(actor_trial));memcpy(out,actor_trial,size);return 1;}
 if(at==900) {assert(size==sizeof(actor));memcpy(out,actor,size);return 1;}
 if(at==800) {assert(size==sizeof(traveler));memcpy(out,traveler,size);return 1;}
 if(at==700) {memset(out,0,size);return 1;}
 if(at==100) {assert(size==sizeof(playback));memcpy(out,playback,size);}
 else if(at==200) {assert(size==sizeof(pending));memcpy(out,&pending,size);}
 else if(at==300) {assert(size==sizeof(body));memcpy(out,body,size);}
 else if(at==400) {assert(size==sizeof(features));memcpy(out,features,size);}
 else if(at==500) {assert(size==sizeof(room));memcpy(out,room,size);}
 else if(at==600) {assert(size==sizeof(outputs));memcpy(out,outputs,size);}
 else assert(0);
 return 1;
}
''' + helper[report_start:report_end] + helper[helper.index('static void format_exit_report('):helper.index('static void append_exit_text(')] + r'''
#define WINAPI
#define WAIT_TIMEOUT 258
#define WAIT_OBJECT_0 0
#define JC_PATH_CAP 1024
typedef struct { unsigned low,high; } FILETIME;
#define JC_FLIGHT_FIELDS 18
#define JC_FLIGHT_RVA 700
#define CP_UTF8 65001
typedef uint32_t DWORD;
static char captured_log[50000];static size_t log_used;static unsigned flushes,closes;
static DWORD WaitForSingleObject(HANDLE h,DWORD ms) {(void)h;(void)ms;return ++watch_poll<4 ? WAIT_TIMEOUT : WAIT_OBJECT_0;}
static int GetExitCodeProcess(HANDLE h,DWORD *code) {(void)h;*code=0;return 1;}
static DWORD GetLastError(void) {return 0;}
static int WriteFile(HANDLE h,const void *data,DWORD n,DWORD *written,void *unused) {
 (void)h;(void)unused;assert(n<sizeof(captured_log)-log_used);
 memcpy(captured_log+log_used,data,n);log_used+=n;captured_log[log_used]=0;*written=n;return 1;
}
static int FlushFileBuffers(HANDLE h) {(void)h;++flushes;return 1;}
static int CloseHandle(HANDLE h) {(void)h;++closes;return 1;}
static int WideCharToMultiByte(unsigned cp,unsigned flags,const wchar_t *wide,int size,char *out,int cap,void *a,void *b) {
 (void)flags;(void)a;(void)b;assert(cp==CP_UTF8 && size==-1);size_t n=wcslen(wide)+1;
 if(n>(size_t)cap)return 0;for(size_t i=0;i<n;++i){assert(wide[i]<128);out[i]=(char)wide[i];}return (int)n;
}
''' + helper[helper.index('struct exit_watch {'):helper.index('static int watch_exit_start(')] + r'''
int main(void) {
 wchar_t report[10000];
 assert(native_playback_report(0,0,report,10000)==0);
 assert(traveler_report(0,0,report,10000)==0);
 actor[0]=2; actor[1]=44;
 features[8]=1;features[9]=0;features[30]=27;features[32]=8;features[33]=3;features[34]=2;features[35]=12;features[36]=11;features[37]=9;features[38]=17;features[39]=28;
 assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"ACTIVE TRAVELER") && wcsstr(report,L"2 / 44"));
 assert(wcsstr(report,L"frame IDs") && wcsstr(report,L"12 / 11") && wcsstr(report,L"8 / 3 / 2") && wcsstr(report,L"9 / 17 ms"));
 memset(features,0,sizeof(features));
 for(size_t cap=1;cap<500;++cap){report[cap]=L'Z';assert(traveler_report(0,0,report,cap)==1 && report[cap-1]==0 && report[cap]==L'Z');}
 memset(actor,0,sizeof(actor));
 traveler[19]=1;assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"normal Desert initialization"));
 traveler[19]=0;traveler[0]=3;
 assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"Full object retained"));
 for(size_t cap=1;cap<500;++cap) {
   for(size_t j=0;j<10000;++j)report[j]=L'Z';
   assert(traveler_report(0,0,report,cap)==1 && report[cap-1]==0 && report[cap]==L'Z');
 }
 traveler[0]=4;assert(traveler_report(0,0,report,10000)==1 && wcsstr(report,L"cleanup and deletion completed"));
 traveler[0]=0;
 pending=20;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Queued"));
 pending=0;playback[0]=-1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 1: declined before native submission"));
 playback[16]=1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 1: stopped after native submission"));
 playback[0]=0;playback[24]=-1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 2: declined before native submission"));
 playback[40]=1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Body 2: stopped after native submission"));
 playback[23]=1;assert(native_playback_report(0,0,report,10000)==1);assert(wcsstr(report,L"Room input"));
 for(unsigned check=20;check<29;++check) {
  playback[2]=4;playback[3]=(LONG)(check*1024+7);outputs[5]=0x72c720;
  assert(native_playback_report(0,0,report,10000)==1);
  assert(wcsstr(report,L"Output guard:") && wcsstr(report,L"row 7") && wcsstr(report,L"0x0072C720"));
 }
 const wchar_t *checks[]={L"counter pointer",L"four-byte alignment",L"counter read",L"live reference count",L"counter storage"};
 const wchar_t *owners[]={L"Local",L"vanilla Remote",L"body 1",L"body 2"};
 for(unsigned check=1;check<=5;++check)for(unsigned owner=0;owner<4;++owner) {
  playback[2]=9;playback[3]=(LONG)(check*4+owner);
  assert(native_playback_report(0,0,report,10000)==1);
  wchar_t expected[128];swprintf(expected,128,L"Reference guard: %ls; owner %ls.",checks[check-1],owners[owner]);
  assert(wcsstr(report,expected) && !wcsstr(report,L"Output guard:"));
 }
 for(unsigned check=1;check<=7;++check) {
  playback[2]=2;playback[3]=(LONG)check;
  assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"Scheduler guard:"));
 }
 playback[0]=1;playback[1]=17;playback[2]=2;playback[3]=6;
 assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"allocator retry pending") && wcsstr(report,L"ring reservation"));
 playback[2]=0;assert(native_playback_report(0,0,report,10000)==1);assert(!wcsstr(report,L"Output guard:"));
 playback[23]=1;features[30]=1;features[31]=12;features[32]=24;features[33]=6;features[34]=20;
 assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"Sender appearance: captured; outfit 6") && wcsstr(report,L"scarf 12/24; symbol 20"));
 features[30]=0;assert(native_playback_report(0,0,report,10000)==1 && wcsstr(report,L"Sender appearance: unavailable"));
 for(unsigned i=0;i<48;++i)playback[i]=body[i]=INT32_MAX;for(unsigned i=0;i<40;++i)features[i]=INT32_MAX;for(unsigned i=0;i<20;++i)room[i]=INT32_MAX;
 playback[0]=playback[24]=1;
 assert(native_playback_report(0,0,report,10000)==1);size_t maximum=wcslen(report);assert(maximum<6500);assert(wcsstr(report,L"8 fresh recheck") && wcsstr(report,L"Bind details") && wcsstr(report,L"Body 2:"));
 struct {wchar_t before;wchar_t short_text[16];wchar_t after;} small={.before=0x1234,.after=0x5678};
 assert(native_playback_report(0,0,small.short_text,16)==-1);assert(small.before==0x1234 && small.after==0x5678);
 wchar_t exit_report[10000];
 format_exit_report(exit_report,10000,1,0,report);
 assert(wcsstr(exit_report,L"0x00000000") && wcsstr(exit_report,L"NATIVE COMPANIONS") && wcsstr(exit_report,L"Last readable live report"));
 format_exit_report(exit_report,10000,0,0,L"");assert(wcsstr(exit_report,L"No live report was retained"));
 for(size_t cap=1;cap<500;++cap) {
  wchar_t bounded[502];for(unsigned i=0;i<502;++i)bounded[i]=0x1234;
  format_exit_report(bounded,cap,1,0,report);assert(bounded[cap]==0x1234);assert(wmemchr(bounded,0,cap));
 }
 memset(playback,0,sizeof(playback));playback[0]=-1;playback[1]=15;playback[2]=4;playback[3]=22*1024+7;
 outputs[5]=0x72c720;
 watch_exit(&exit_watch);
 assert(strstr(captured_log,"code=00000000") && strstr(captured_log,"last readable native report") &&
  strstr(captured_log,"unsupported output type") && strstr(captured_log,"row 7") && strstr(captured_log,"0x0072C720"));
 assert(flushes==1 && closes==2);
 watch_poll=0;log_used=0;captured_log[0]=0;memset(playback,0,sizeof(playback));
 watch_exit(&exit_watch);assert(strstr(captured_log,"code=00000000") && !strstr(captured_log,"NATIVE COMPANIONS"));
 printf("native report: 49 production-body scenarios passed; maximal numeric report %zu / 10000 characters; exit report retention and 499 bounded capacities and two exit-recorder cases passed\n",maximum);
}
''')
report_binary = root / 'work/native_report_fixture'
subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-fno-omit-frame-pointer', str(report_fixture), '-o', str(report_binary)], check=True)
subprocess.run([str(report_binary)], check=True)

def extract_function(signature):
    start = source.rindex(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


cloth_prefix = r'''
#define main cloth_header_main
#include "../diagnostic/test_native_companion_cloth.c"
#undef main
#define native_cloth_lease lease
struct callback_read_budget { uint64_t deadline; unsigned reads,fast; int expired; struct jc_read_batch *batch; };
static int JNETCompanionFeatures[40];
static uintptr_t JNETGame=GAME;
static unsigned resource_thread=7,current_thread=7;
typedef uint32_t DWORD;
static DWORD cloth_production_thread;
static unsigned cloth_production_pass;
static int native_cloth_disabled,published,finish_count,bad_finish_return,before_count,forward_count;
static unsigned GetCurrentThreadId(void) {return current_thread;}
static uint64_t GetTickCount64(void) {return 1000;}
static void *GetModuleHandleW(void *unused) {(void)unused;return 0;}
static void flight_mark(int a,int b) {(void)a;(void)b;}
static int own_read(void *c,uint64_t at,void *out,size_t bytes) {return read_memory(c,at,out,bytes);}
#define callback_read read_memory
#define companion_cloth_write write_memory
#define companion_cloth_call native_call
static void companion_cloth_publish(int code) {published=code;}
static uintptr_t fake_finish(uintptr_t at,void *out,unsigned pass,uintptr_t spare) {
    assert(at==BARN && pass==1 && spare==0);++finish_count;
    if(bad_finish_return)return 0;
    unsigned char *result=out;memset(result,0,16);result[0]=finish_count>=2;
    if(result[0]){barn[0x6bcf84]=0;job[0x58]=0;}
    return (uintptr_t)out;
}
#define JC_RVA_ClothFinish ((uintptr_t)fake_finish)
static void companion_cloth_before(uintptr_t at) {assert(at==BARN);++before_count;}
static uintptr_t fake_update(uintptr_t at,float dt,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,
    uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11,uintptr_t a12,uintptr_t a13,uintptr_t a14) {
    assert(at==BARN && dt==1.25f && before_count==1);const uintptr_t args[]={a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14};
    for(unsigned i=0;i<12;++i)assert(args[i]==0x100+i);++forward_count;return 0x123456789;
}
static uintptr_t (*original_cloth_update)(uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t,
    uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t)=fake_update;
'''
cloth_tests = r'''
int main(void) {
    cloth_header_main();
    reset();finish_count=0;assert(!companion_cloth_finish(BARN) && !finish_count);
    barn[0x6bcf84]=1;job[0x58]=1;assert(!companion_cloth_finish(BARN) && finish_count==2 && !barn[0x6bcf84]);
    reset();finish_count=0;barn[0x6bcf84]=2;assert(companion_cloth_finish(BARN)==13 && !finish_count);
    reset();barn[0x6bcf84]=1;bad_finish_return=1;assert(companion_cloth_finish(BARN)==13);bad_finish_return=0;
    reset();assert(!step(1,1));lease.generation=1;barn[0x6bcf84]=1;job[0x58]=1;finish_count=0;
    native_cloth_disabled=0;published=-99;companion_cloth_cleanup_locked(PLAYERS+0xb0);
    assert(finish_count==2 && calls[JC_CLOTH_RELEASE]==1 && lease.state==JC_CLOTH_RETIRED && native_cloth_disabled && !published);
    reset();assert(!step(1,1));lease.generation=1;current_thread=8;published=0;
    companion_cloth_cleanup_locked(PLAYERS+0xb0);assert(published==11 && !calls[JC_CLOTH_RELEASE] && lease.state==JC_CLOTH_LIVE);current_thread=7;
    reset();assert(!step(1,1));lease.generation=1;companion_cloth_cleanup_locked(PLAYERS+0x146f0);assert(!calls[JC_CLOTH_RELEASE]);
    before_count=forward_count=0;assert(companion_cloth_update(BARN,1.25f,0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108,0x109,0x10a,0x10b)==0x123456789);
    assert(before_count==1 && forward_count==1);
    puts("native cloth orchestration: 8 extracted-production finish, cleanup and 14-argument forwarding scenarios passed");
}
'''
cloth_production = ''.join(extract_function(signature) for signature in (
    'static int companion_cloth_finish(', 'static void companion_cloth_cleanup_locked(',
    'static uintptr_t companion_cloth_update('))

appearance_prefix = r'''
#include <assert.h>
#include <stdio.h>
#include "native_companion_cloth.h"
#define jc_shader_inputs_read actual_shader_inputs_read
#define jc_shader_storage_read actual_shader_storage_read
#include "native_shader_inputs.h"
#undef jc_shader_inputs_read
#undef jc_shader_storage_read
typedef int32_t LONG;
struct callback_read_budget { uint64_t deadline; unsigned reads,fast; int expired; struct jc_read_batch *batch; };
struct jc_inspection { struct callback_read_budget *budget; };
static LONG JNETCompanionFeatures[40];
static LONG InterlockedExchange(volatile LONG *p,LONG n) {LONG old=*p;*p=n;return old;}
static int inspection_read(void *ctx,uint64_t at,void *out,size_t bytes) {(void)ctx;(void)at;(void)out;(void)bytes;return 1;}
static struct jc_cloth_lease native_cloth_lease;
static struct {struct {uint64_t object;} s_initialized_life;} body_contexts[2];
static struct jc_shader_inputs shader_inputs[8];
static unsigned char shader_data[8][256];
static uintptr_t material_addresses[8];
static uint64_t environment[2]={0x110000,0x120000};
static const unsigned widths[]={4,16,64};
static int appearance_calls,texture_calls,writes,read_failure,layout_change,environment_change,write_failure,texture_mutation,expire_after_call;
static struct callback_read_budget budget;
static uint64_t GetTickCount64(void) {return 1000;}
static void *GetModuleHandleW(void *unused) {(void)unused;return 0;}
static void flight_mark(int a,int b) {(void)a;(void)b;}
static int callback_read(void *c,uint64_t at,void *out,size_t bytes) {
    (void)c;assert(at==0x300000+0x646d30 && bytes==16);
    memcpy(out,environment,16);return 1;
}
static int jc_shader_inputs_read(jc_layout_reader read,void *c,uint64_t at,struct jc_shader_inputs *out) {
    (void)read;(void)c;
    if(read_failure)return 1;
    for(unsigned i=0;i<8;++i)if(at==material_addresses[i]){*out=shader_inputs[i];return 0;}
    assert(0);return 1;
}
static int jc_shader_storage_read(jc_layout_reader read,void *c,uint64_t at,struct jc_shader_inputs *out) {
 return jc_shader_inputs_read(read,c,at,out);
}
static int evaluation_writable(void *c,uint64_t at,size_t bytes) {(void)c;(void)at;return bytes>0;}
static int companion_cloth_write(void *c,uint64_t at,const void *data,size_t bytes) {
    struct callback_read_budget *b=c;assert(!b->expired && b->deadline>GetTickCount64());
    if(write_failure)return 0;
    assert(writes<3 && bytes==widths[writes] && at==shader_inputs[0].buffer+shader_inputs[0].offsets[writes]);
    memcpy(shader_inputs[0].values[writes],data,bytes);++writes;return 1;
}
static void fake_appearance(uintptr_t dude,uintptr_t barn) {
    assert(dude==0x200000 && barn==0x300000);++appearance_calls;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<widths[i]/4;++j)shader_inputs[0].values[i][j]+=7.25f;
    if(layout_change)++shader_inputs[0].offsets[2];
    if(environment_change)++environment[0];
    if(expire_after_call)budget.expired=1;
}
static void fake_texture(uintptr_t material,const char *name,uintptr_t texture) {
    assert(texture_calls<3 && material==material_addresses[texture_calls] && (uintptr_t)name==0x5cf7f0);
    assert(texture==0x110000);++texture_calls;
    if(texture_mutation && texture_calls==3)shader_inputs[0].values[1][0]+=1;
}
static int companion_cloth_atlas(struct callback_read_budget *b,uintptr_t image,uint64_t dude,uint64_t barn) {(void)b;(void)image;(void)dude;(void)barn;assert(0);return 0;}
static void fake_unused(void) {assert(0);}
#define JC_RVA_ClothRelease ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothInit ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothAttach ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothCollide ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothAppearance ((uintptr_t)fake_appearance)
#define JC_TEST_MATERIAL_TEXTURE_SET ((uintptr_t)fake_texture)
static void reset_appearance(void) {
    appearance_calls=texture_calls=writes=read_failure=layout_change=environment_change=write_failure=texture_mutation=expire_after_call=0;
    budget=(struct callback_read_budget){.deadline=1500};environment[0]=0x110000;environment[1]=0x120000;
    native_cloth_lease.players=0x400000;body_contexts[1].s_initialized_life.object=0x500000;
    const uintptr_t addresses[]={0x2001e0,0x300000+0x666ee8+4*0x630,0x300000+0x666ee8+5*0x630,
        0x400290,0x4148d0,0x300000+0x666ee8+2*0x630,0x300000+0x666ee8+3*0x630,0x5001e0};
    memcpy(material_addresses,addresses,sizeof(addresses));memset(shader_inputs,0,sizeof(shader_inputs));
    for(unsigned i=0;i<8;++i){
        struct jc_shader_inputs *s=shader_inputs+i;s->buffer=(uintptr_t)shader_data[i];s->bytes=256;
        s->payload=0x600000+i*0x100;s->instance=0x700000+i*0x100;s->shader=0x800000+i*0x100;s->found=s->finite=7;
        s->offsets[0]=4;s->offsets[1]=16;s->offsets[2]=32;
        for(unsigned j=0;j<3;++j){s->sizes[j]=widths[j];for(unsigned k=0;k<widths[j]/4;++k)s->values[j][k]=(float)(i*100+j*20+k);}
    }
}
'''
appearance_tests = r'''
static int invoke(void) {return companion_cloth_call(&budget,JC_CLOTH_APPEARANCE,0x200000,0x300000,0);}
int main(void) {
    unsigned scenarios=0;
    reset_appearance();struct jc_shader_inputs saved=shader_inputs[0];assert(invoke());++scenarios;
    assert(appearance_calls==1 && writes==3 && texture_calls==3 && !memcmp(saved.values,shader_inputs[0].values,sizeof(saved.values)));
    for(unsigned owner=0;owner<3;++owner)for(unsigned other=0;other<8;++other)if(owner!=other){
        reset_appearance();shader_inputs[owner].buffer=shader_inputs[other].buffer+128;
        assert(!invoke() && !appearance_calls && !writes && !texture_calls);++scenarios;
    }
    reset_appearance();read_failure=1;assert(!invoke() && !appearance_calls);++scenarios;
    reset_appearance();layout_change=1;assert(!invoke() && appearance_calls==1 && !writes && !texture_calls);++scenarios;
    reset_appearance();environment_change=1;assert(!invoke() && appearance_calls==1 && writes==3 && texture_calls==3);++scenarios;
    reset_appearance();write_failure=1;assert(!invoke() && appearance_calls==1 && !writes && !texture_calls);++scenarios;
    reset_appearance();texture_mutation=1;assert(!invoke() && appearance_calls==1 && writes==3 && texture_calls==3);++scenarios;
    reset_appearance();expire_after_call=1;assert(invoke() && budget.expired && writes==3 && texture_calls==3);++scenarios;
    reset_appearance();budget.expired=1;assert(!invoke() && !appearance_calls);++scenarios;
    printf("native cloth appearance: %u extracted-production buffer-disjointness, exact restoration and texture-binding scenarios passed\n",scenarios);
}
'''
appearance_production = extract_function('static int companion_cloth_call(')
assert appearance_production.count('image+0x273640') == 1
appearance_production = appearance_production.replace('image+0x273640', 'image+JC_TEST_MATERIAL_TEXTURE_SET')
atlas_prefix = r'''
#define main cloth_header_main
#include "../diagnostic/test_native_companion_cloth.c"
#undef main
#define native_cloth_lease lease
struct callback_read_budget { int unused; };
#define ATLAS UINT64_C(0x2000000)
static unsigned char atlas_memory[6][0x200];
static unsigned resolved,selected,flag_reads[6];
static int atlas_failure;
static int JNETCompanionFeatures[40];
typedef int LONG;
static int InterlockedExchange(int *p,int n) { int old=*p;*p=n;return old; }
static int callback_read(void *c,uint64_t at,void *out,size_t bytes) {
    if (at==IMAGE+0x403f050 && bytes==8) { uint64_t type=IMAGE+0x403f060;memcpy(out,&type,8);return 1; }
    if (at==IMAGE+0x403f060 && bytes==8) { uint64_t type=IMAGE+0x5cd4c8;memcpy(out,&type,8);return 1; }
    if (at>=ATLAS && at-ATLAS<sizeof(atlas_memory) && bytes<=sizeof(atlas_memory)-(at-ATLAS)) {
        memcpy(out,(unsigned char *)atlas_memory+(at-ATLAS),bytes);
        if (atlas_failure==9 && (at-ATLAS)%0x200==0x1a4 && bytes==4 && ++flag_reads[(at-ATLAS)/0x200]==2) {
            uint32_t changed=1;memcpy(out,&changed,4);
        }
        return 1;
    }
    return read_memory(c,at,out,bytes);
}
static int evaluation_writable(void *c,uint64_t at,size_t bytes) {
    (void)c;(void)at;(void)bytes;return atlas_failure!=1;
}
static uintptr_t fake_lookup(uintptr_t manager,uintptr_t type,const char *name) {
    assert(manager==MATERIALS && type==IMAGE+0x403f060);
    const char *names[2][3];uint32_t outfit;memcpy(&outfit,dude+0x70,4);assert(jc_cloth_atlas_names(outfit,names));
    if (atlas_failure==8) return 0;
    for (unsigned i=0;i<6;++i) if (!strcmp(name,names[i/3][i%3])) return ATLAS+i*0x200;
    assert(0);return 0;
}
static void fake_resolve(uintptr_t manager,const char *name,uintptr_t holder) {
    assert(manager==MATERIALS && resolved<6);
    unsigned i=resolved++;uint64_t at=fake_lookup(manager,IMAGE+0x403f060,name);unsigned char *p=atlas_memory[i];
    assert(at==ATLAS+i*0x200 && holder==BARN+0x666d68+(4+i/3)*0x630+(i%3)*0x28);
    put64(resolve(holder,16),0,at);put64(resolve(holder,16),8,at+0x28);
    put64(p,0x38,at+0x40);put64(p,0x108,at+0x180);
    /* No fabricated name string in the payload: native binding proves identity. */
    if (atlas_failure==2) put64(resolve(holder,16),8,at+0x30);
    if (atlas_failure==3) put64(p,0x108,0);
    if (atlas_failure==4 || (atlas_failure==7 && i<3)) put32(p,0x1a4,0x80);
}
static void fake_select(uintptr_t barn_ptr,unsigned role,uintptr_t manager,const char *a,const char *b,const char *c) {
    assert(barn_ptr==BARN && manager==MATERIALS && role==4+selected && a && b && c);++selected;
    if (atlas_failure==5 && selected==1) barn[0x666d68+2*0x630]^=1;
    if (atlas_failure==6) put64(game,0x28,MATERIALS+8);
}
static void reset_atlas(void) {
    reset();assert(!step(1,1));put64(game,0x28,MATERIALS);
    resolved=selected=0;atlas_failure=0;memset(flag_reads,0,sizeof(flag_reads));memset(atlas_memory,0,sizeof(atlas_memory));
}
'''
atlas_production = extract_function('static int companion_cloth_atlas(')
atlas_production = atlas_production.replace('(image+0x8ee60)', '(uintptr_t)fake_resolve').replace('(image+0x8ab10)', '(uintptr_t)fake_select').replace('(image+0x2ce120)', '(uintptr_t)fake_lookup')
atlas_tests = r'''
int main(void) {
    struct callback_read_budget budget={0};unsigned scenarios=0;
    for (unsigned outfit=0;outfit<7;++outfit) {
        reset_atlas();put32(dude,0x70,outfit);
        assert(companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && resolved==6 && selected==2);++scenarios;
    }
    for (int bad=1;bad<=6;++bad) {
        reset_atlas();atlas_failure=bad;
        assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN));++scenarios;
        if (bad<=4) assert(!selected);
    }
    reset_atlas();put32(dude,0x70,7);assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved);++scenarios;
    reset_atlas();lease.state=JC_CLOTH_RETIRED;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved);++scenarios;
    reset_atlas();failed_read=GAME+0x28;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved);++scenarios;
    reset_atlas();atlas_failure=7;assert(companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && selected==2);++scenarios;
    reset_atlas();atlas_failure=8;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved && JNETCompanionFeatures[11]==40);++scenarios;
    reset_atlas();atlas_failure=9;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !selected && JNETCompanionFeatures[11]==106);++scenarios;
    printf("Native atlas: %u production scenarios; all seven outfits, exact resource names, protected textures, missing resources, original holders and manager mutation checked. Native loaders simulated.\n",scenarios);
}
'''
numeric_appearance_prefix = r"""
#define main cloth_header_main
#include "../diagnostic/test_native_companion_cloth.c"
#undef main
#include "native_cloth_frame.h"
static struct jc_cloth_lease cloth_actor_scarf;
static uint64_t cloth_actor_scarf_peer;
static uint64_t traveler_object=DUDE,traveler_game=GAME,traveler_barn=PLAYERS;
static long JNETFrames=1;
static unsigned hsv_calls,status_messages;
static int context_ready=1;
static void *GetModuleHandleW(void *p) {(void)p;return (void *)(uintptr_t)IMAGE;}
static int traveler_context(void) {return context_ready;}
static void message(const char *s) {assert(s && *s);++status_messages;}
static int own_read(void *ctx,uint64_t at,void *out,size_t bytes) {
    if (at==failed_read) return 0;
    if (at==IMAGE+0x217a70 && bytes==16) {
        const unsigned char entry[16]={64,83,72,131,236,96,15,41,116,36,80,15,40,194,15,41};
        memcpy(out,entry,16);if(bad_fingerprint)((unsigned char *)out)[0]^=1;return 1;
    }
    if (bytes==4 && (at==IMAGE+0x3c943dc || at==IMAGE+0x3763b38 || at==IMAGE+0x3763b3c)) {
        float value=at==IMAGE+0x3c943dc?0.f:1.f;memcpy(out,&value,4);return 1;
    }
    return read_memory(ctx,at,out,bytes);
}
static int jc_local_windows_write(uint64_t at,const void *data,size_t bytes) {return write_memory(NULL,at,data,bytes);}
static void fake_actor_hsv(uintptr_t unused,uintptr_t actor,float hue,float saturation,float value) {
    assert(!unused && actor==DUDE && hue==0 && saturation==1 && value==1);++hsv_calls;
}
"""
numeric_appearance_production = '\n'.join(extract_function(signature) for signature in (
    'static int cloth_actor_appearance_status(', 'static int cloth_actor_descriptor_write(',
    'static int cloth_actor_numeric_appearance('))
numeric_appearance_production = numeric_appearance_production.replace('(image+0x217a70)', '(uintptr_t)fake_actor_hsv')
numeric_appearance_tests = r"""
static void numeric_reset(void) {
    reset();memset(&cloth_actor_scarf,0,sizeof(cloth_actor_scarf));cloth_actor_scarf_peer=0;
    hsv_calls=0;context_ready=1;
}
static void numeric_ready(void) {
    numeric_reset();put32(players,0xb0+0x70,6);put32(players,0xb0+0x68,2);
    assert(!step(1,1));writes=0;
}
int main(void) {
    numeric_reset();
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    unsigned messages=status_messages;
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && status_messages==messages);
    put64(dude,0x9b40,COLLECTION);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    put32(collection,0x91b0,1);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    numeric_ready();put32(barn,0x270118+0xe0,2);put32(barn,0x80+0xd00+0xc94,0);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    put32(barn,0x270118+0xe0,1);put32(barn,0x80+0xd00+0xc94,4);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && writes && hsv_calls==1);
    uint32_t mode;memcpy(&mode,barn+0x80+0xcb8,4);assert(mode==3);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && hsv_calls==2);
    numeric_ready();bad_fingerprint=1;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();failed_read=DUDE+0x9b40;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put32(collection,0x91b0,3);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put64(collection,0x18,BARN+0x10);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put32(barn,0x270118,3);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put32(barn,0x80+0xc94,4);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();failed_write=BARN+0x80+0xca4;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_MATERIAL && !hsv_calls);
    numeric_ready();context_ready=0;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_MATERIAL && !writes && !hsv_calls);
    puts("Actor appearance integration: absent, empty, partial and pending cloth wait without writes; activation, colour, ownership and write failures checked. Native HSV intercepted.");
}
"""
for name, contents in (
    ('native_actor_appearance_fixture', numeric_appearance_prefix + numeric_appearance_production + numeric_appearance_tests),
    ('native_cloth_orchestration_fixture', cloth_prefix + cloth_production + cloth_tests),
    ('native_cloth_appearance_fixture', appearance_prefix + appearance_production + appearance_tests),
    ('native_cloth_atlas_fixture', atlas_prefix + atlas_production + atlas_tests),
):
    fixture = root / f'work/{name}.c'
    binary = root / f'work/{name}'
    fixture.write_text(contents)
    subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-fno-omit-frame-pointer', '-I', str(root / 'diagnostic'),
                    str(fixture), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

# Exercise the actual callback adapter, including its compatibility fallback.
reader_start = source.index('struct callback_read_budget {')
reader_end = source.index('struct jc_inspection {', reader_start)
reader_fixture = root / 'work/native_callback_reader_fixture.c'
reader_fixture.write_text(r'''
#include "native_local_read.h"
#include "native_read_batch.h"
#include <assert.h>
#include <stdio.h>
typedef uint64_t ULONGLONG;
static unsigned char memory[8192];
static unsigned queries, copies, fallback, denied;
static int JNETCompanionFeatures[40];
static uint64_t now;
static uint64_t GetTickCount64(void) {return now;}
static int jc_local_windows_query(void *ctx,uint64_t at,struct jc_local_region *r) {
 (void)ctx;++queries;if(at<0x100000 || at>=0x102000 || denied)return 0;
 *r=(struct jc_local_region){0x100000,0x102000};return 1;
}
static int jc_local_windows_copy(void *ctx,uint64_t at,void *out,size_t n) {
 (void)ctx;++copies;if(denied || at<0x100000 || n>sizeof(memory) || at-0x100000>sizeof(memory)-n)return 0;
 memcpy(out,memory+(at-0x100000),n);return 1;
}
static int own_read(void *ctx,uint64_t at,void *out,size_t n) {
 ++fallback;return jc_local_windows_copy(ctx,at,out,n);
}
''' + source[reader_start:reader_end] + r'''
int main(void) {
 struct callback_read_budget b={.deadline=100,.fast=1};unsigned char out;
 for(unsigned i=0;i<1000;++i) {memory[0]=(unsigned char)i;assert(callback_read(&b,0x100000,&out,1) && out==(unsigned char)i);}
 assert(queries==1 && copies==1000 && !fallback && b.reads==1000);
 denied=1;assert(!callback_read(&b,0x100000,&out,1) && b.read_failed && !b.local.used);
 denied=0;b=(struct callback_read_budget){.deadline=100,.fast=1};
 assert(callback_read(&b,0x100000,&out,1) && queries==2);
 b=(struct callback_read_budget){.deadline=100};
 assert(callback_read(&b,0x100000,&out,1) && fallback==1 && queries==2);
 JNETCompanionFeatures[8]=1;b=(struct callback_read_budget){.deadline=100};
 assert(callback_read(&b,0x100000,&out,1) && fallback==1 && queries==3);
 JNETCompanionFeatures[8]=2;
 now=100;unsigned previous=copies;
 assert(!callback_read(&b,0x100000,&out,1) && b.expired && copies==previous);
 b=(struct callback_read_budget){.deadline=200};denied=1;
 assert(!callback_read(&b,0x100000,&out,1) && b.read_failed && fallback==2);
 denied=0;now=0;struct jc_read_batch batch={0};b=(struct callback_read_budget){.deadline=200,.batch=&batch};
 unsigned start=fallback;unsigned char block[8];
 for(unsigned i=0;i<100;++i)assert(callback_read(&b,0x100000+i*8,block,8));
 assert(fallback==start+1 && b.reads==1);
 memory[0]=77;assert(callback_read(&b,0x100000,block,8) && block[0]==77 && b.reads==2);
 jc_read_batch_reset(&batch);memory[8]=88;
 assert(callback_read(&b,0x100008,block,8) && block[0]==88 && b.reads==3);
 puts("Production callback reader: 1000 fresh reads, one permission query; fault rejection, fresh inspection, fallback and deadline checks passed.");
}
''')
reader_binary = root / 'work/native_callback_reader_fixture'
subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-fno-omit-frame-pointer', '-I', str(root / 'diagnostic'),
                str(reader_fixture), '-o', str(reader_binary)], check=True)
subprocess.run([str(reader_binary)], check=True)

# The watchdog runs in JNET, never in Journey or its initialization thread.
reader_parent = helper[helper.index('static LONG reader_check_isolated(void)'):helper.index('static int run_action(')]
watchdog = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <stddef.h>
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uintptr_t HANDLE;
#define JC_PATH_CAP 1024
#define JC_READER_PASS_EXIT 0x4a430001u
#define FALSE 0
#define CREATE_NO_WINDOW 1
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258
#define WAIT_FAILED 0xffffffffu
#define STILL_ACTIVE 259
struct startup { unsigned cb; }; typedef struct startup STARTUPINFOW;
struct process { HANDLE hProcess,hThread; }; typedef struct process PROCESS_INFORMATION;
static unsigned scenario,waits,closed,terminated;
static DWORD GetModuleFileNameW(void *m,wchar_t *out,DWORD size) {
 (void)m;assert(size==JC_PATH_CAP);wcscpy(out,L"C:\\Game Folder\\JC-Diagnostics\\JNET.exe");return (DWORD)wcslen(out);
}
static int CreateProcessW(const wchar_t *exe,wchar_t *command,void *a,void *b,int inherit,DWORD flags,void *env,void *cwd,STARTUPINFOW *s,PROCESS_INFORMATION *p) {
 assert(!a&&!b&&!inherit&&!env&&!cwd&&flags==CREATE_NO_WINDOW&&s->cb==sizeof(*s));
 assert(wcsstr(command,exe) && command[0]=='"' && wcsstr(command,L" --reader-check-child"));
 if(scenario==3)return 0;*p=(PROCESS_INFORMATION){10,11};return 1;
}
static int CloseHandle(HANDLE h) {assert(h==10||h==11);++closed;return 1;}
static DWORD WaitForSingleObject(HANDLE h,DWORD timeout) {
 assert(h==10);++waits;assert(timeout==(waits==1?5000:1000));
 return scenario==2 ? WAIT_TIMEOUT : scenario==4 ? WAIT_FAILED : WAIT_OBJECT_0;
}
static int GetExitCodeProcess(HANDLE h,DWORD *out) {
 assert(h==10);if(scenario==5)return 0;
 *out=scenario==0?JC_READER_PASS_EXIT:scenario==1?3:scenario==6?0:0xc0000005u;return 1;
}
static int TerminateProcess(HANDLE h,unsigned code) {assert(h==10&&code==104);++terminated;return 1;}
static void trace(const char *stage,DWORD exit_code,DWORD result) {(void)stage;(void)exit_code;(void)result;}
''' + reader_parent + r'''
int main(void) {
 const LONG expected[]={0,3,101,100,102,102,103,103};
 for(scenario=0;scenario<8;++scenario) {
  waits=closed=terminated=0;assert(reader_check_isolated()==expected[scenario]);
  assert(closed==(scenario==3?0:2));
  assert(terminated==((scenario==2||scenario==4||scenario==5)?1u:0u));
 }
 puts("Reader watchdog: pass, fault rejection, timeout, spawn/wait/exit-query failure, premature success and crash exit checked; only the owned child is terminated.");
}
'''
fixture = root / 'work/native_reader_watchdog_fixture.c'
binary = root / 'work/native_reader_watchdog_fixture'
fixture.write_text(watchdog)
subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', str(fixture), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)


# Exercise the exact production dependency, worker-fallback and finish observers.
def cloth_function(name):
    at = source.index(name + '(')
    while source[source.rfind('\n', 0, at) + 1:at].strip() not in ('static int', 'static void', 'static uint32_t', 'static uintptr_t') or ';' in source[at:source.index('{', at)]:
        at = source.index(name + '(', at + len(name))
    start = source.rfind('\n', 0, at) + 1
    brace = source.index('{', at)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

cloth_fixture = (root / 'diagnostic/test_native_cloth_producer.c').read_text().split('int main(void) {')[0]
cloth_fixture += r"""
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef size_t SIZE_T;
struct jc_cloth_live { struct jc_cloth_frames frames; unsigned failed; };
struct jc_cloth_producer_live {
 struct jc_cloth_live *backend;void *context;
 int (*adopt)(void *,unsigned,uint64_t,unsigned);
 int (*pass_done)(void *,uint64_t,unsigned);
};
static struct fixture *current;
static struct jc_cloth_live backend,*active_cloth_backend;
static struct jc_cloth_producer cloth_producer;
static struct jc_cloth_producer_participant cloth_participant;
static unsigned cloth_participant_ready,cloth_production_pass,cloth_map_lock;
static uint64_t cloth_production_serial,cloth_production_job;
static DWORD cloth_production_thread;
static struct { unsigned roles,index,launched_roles,seen[2],finish_failed;volatile LONG worker_reported;uint64_t barn,generation; } cloth_maps;
static volatile LONG JNETCompanionFeatures[40];
static unsigned dependencies,stock_workers,routed_workers,original_finishes,seals,commits,finalizes,unmaps;
static unsigned owner_ok=1,flip_busy,write_fail,launch_second_pass;
static void *traveler_actor;
static LONG JNETActorTrial[5];
enum { JC_ACTOR_RUNTIME_SCOPE=77 };
static int jc_actor_runtime_quarantine(void *p,int code) { (void)p;return code; }
static void traveler_actor_report(int code) { (void)code; }
static unsigned char job_object[0x60];
static int cloth_produce_before_dependency(uintptr_t dependency);
static void *GetModuleHandleW(void *v) { (void)v;return (void *)(uintptr_t)UINT64_C(0x140000000); }
static DWORD GetCurrentThreadId(void) { return 7; }
static void *GetCurrentProcess(void) { return NULL; }
static void *GetProcessHeap(void) { return NULL; }
static void *HeapAlloc(void *h,unsigned f,size_t n) { (void)h;(void)f;return malloc(n); }
static void HeapFree(void *h,unsigned f,void *p) { (void)h;(void)f;free(p); }
static LONG InterlockedIncrement(volatile LONG *v) { return ++*v; }
static LONG InterlockedExchange(volatile LONG *v,LONG n) { LONG old=*v;*v=n;return old; }
static LONG InterlockedCompareExchange(volatile LONG *v,LONG n,LONG expected) { LONG old=*v;if(old==expected)*v=n;return old; }
static void message(const char *s) { assert(!strncmp(s,"[JC]",4)); }
static void AcquireSRWLockExclusive(unsigned *p) { assert(!*p);*p=1; }
static void ReleaseSRWLockExclusive(unsigned *p) { assert(*p==1);*p=0; }
static void AcquireSRWLockShared(unsigned *p) { AcquireSRWLockExclusive(p); }
static void ReleaseSRWLockShared(unsigned *p) { ReleaseSRWLockExclusive(p); }
static int own_read(void *ctx,uint64_t at,void *out,size_t n) {
 (void)ctx;
 if(at==UINT64_C(0x140731f70)&&n==8) { memcpy(out,current->passes+4,n);return 1; }
 if(at==(uintptr_t)job_object+0x58&&n==1) {
  memcpy(out,job_object+0x58,1);if(flip_busy)job_object[0x58]=0;return 1;
 }
 return read_memory(current,at,out,n);
}
static int jc_local_windows_write(uint64_t to,const void *from,size_t n) {
 if(write_fail)return 0;memcpy((void *)(uintptr_t)to,from,n);return 1;
}
static int jc_cloth_live_owner(struct jc_cloth_live *s) { return s==&backend&&owner_ok; }
static int cloth_producer_adopt(void *ctx,unsigned index,uint64_t gen,unsigned pass) {
 (void)ctx;(void)index;(void)gen;(void)pass;assert(0);return 1;
}
static int cloth_producer_pass_done(void *ctx,uint64_t gen,unsigned pass) { return pass_done(ctx,gen,pass); }
static int adopt_live(void *ctx,unsigned index,uint64_t generation,unsigned pass) {
 int code=adopt(ctx,index,generation,pass);
 if(!code) {
  backend.frames=current->frames;
  cloth_maps.roles=(1u<<4)|(1u<<5);cloth_maps.barn=(uintptr_t)current->barn;
  cloth_maps.generation=generation;cloth_maps.index=index;
  cloth_maps.seen[0]=cloth_maps.seen[1]=cloth_maps.roles;
 }
 return code;
}
static struct jc_cloth_producer_ops jc_cloth_producer_live_ops(struct jc_cloth_producer_live *p) {
 assert(p->backend==&backend);
 return (struct jc_cloth_producer_ops){current,prepare,abandon,begin,configure,adopt_live,cancel};
}
static uint32_t original_cloth_dependency(uintptr_t dep) {
 assert(dep==(uintptr_t)current->barn+0x6bce00);++dependencies;return 123;
}
static void original_worker(void *w,const void *entry,void *args,void *streams,void *descriptor,void *state) {
 (void)w;(void)entry;(void)args;(void)streams;(void)descriptor;(void)state;++stock_workers;
}
static void (*original_cloth_worker[2])(void *,const void *,void *,void *,void *,void *)={original_worker,original_worker};
static void cloth_worker_call(void *ctx,void *w,const void *e,void *a,void *s,void *d,void *v) {
 (void)ctx;(void)w;(void)e;(void)a;(void)s;(void)d;(void)v;
}
static int fixture_dispatch(struct jc_cloth_frame *f,uint64_t gen,unsigned roles,void *streams,
 void *ctx,jc_cloth_worker_call call,void *w,const void *e,void *args,void *actual,void *desc,void *state) {
 (void)f;(void)gen;(void)roles;(void)streams;(void)ctx;(void)call;(void)w;(void)e;(void)args;(void)actual;(void)desc;(void)state;
 ++routed_workers;return 1;
}
#define jc_cloth_frame_dispatch fixture_dispatch
static uintptr_t original_cloth_finish(uintptr_t barn,void *result,unsigned pass,uintptr_t fourth) {
 assert(barn==(uintptr_t)current->barn&&pass<=1&&fourth==99);++original_finishes;
 if(launch_second_pass) {
  assert(pass==0 && cloth_production_pass==1 && cloth_production_thread==7);
  assert(cloth_produce_before_dependency(barn+0x6bce00)==1);
  launch_second_pass=0;
 }
 unsigned char *bytes=result;memset(bytes,0,16);bytes[0]=!job_object[0x58];
 uint64_t job=(uintptr_t)job_object;if(job_object[0x58])memcpy(bytes+8,&job,8);return (uintptr_t)result;
}
struct cloth_finish_call { uintptr_t barn,result,fourth,returned; unsigned roles,detail,called; };
static int cloth_finish_native(void *ctx,uint64_t generation,unsigned pass) {
 struct cloth_finish_call *c=ctx;assert(generation==cloth_maps.generation);
 c->called=1;c->returned=original_cloth_finish(c->barn,(void *)c->result,pass,c->fourth);return 1;
}
static int fixture_seal(struct jc_cloth_frames *s,unsigned index,unsigned pass) {
 assert(s==&backend.frames&&index==cloth_maps.index&&pass==0&&!job_object[0x58]);++seals;return 0;
}
static int fixture_commit(struct jc_cloth_frame *f,uint64_t gen,unsigned pass,void *args,size_t n) {
 assert(f->generation==gen&&pass==0&&args==f->source_args&&n==JC_CLOTH_WORKER_ARGS);++commits;return 0;
}
static int fixture_finalize(struct jc_cloth_frames *s,unsigned index,unsigned pass,
 int (*call)(void *,uint64_t,unsigned),void *ctx) {
 ++finalizes;assert(seals==1&&commits==1);assert(call(ctx,s->frames[index].generation,pass)==1);
 s->frames[index].phase=JC_FRAME_READY;return 0;
}
static int jc_cloth_live_unmap(struct jc_cloth_live *s,unsigned index) {
 assert(s==&backend&&index==cloth_maps.index);++unmaps;return 0;
}
#define jc_cloth_frame_seal_pass fixture_seal
#define jc_cloth_frame_commit_pass fixture_commit
#define jc_cloth_frame_finalize_pass fixture_finalize
"""
cloth_fixture += ''.join(cloth_function(n) for n in ('cloth_produce_before_dependency', 'cloth_dependency_observer', 'cloth_worker_observer', 'cloth_finish_observer'))
cloth_fixture += r"""
typedef unsigned UINT;
enum { WM_CLOSE=16, INVALID_HANDLE_VALUE=-1 };
static int logfile=1,terminated,flushed,terminate_ok=1;
static int FlushFileBuffers(int file) { assert(file==1);++flushed;return 1; }
static int TerminateProcess(void *process,unsigned code) {
 assert(process==GetCurrentProcess() && code==0xe04a4301u);++terminated;return terminate_ok;
}
"""
cloth_fixture += cloth_function('cloth_quarantined_close')

cloth_fixture += r"""
static void start_case(unsigned fail) {
 setup(current,0);current->fail=fail;
 memset(&backend,0,sizeof(backend));memset(&cloth_maps,0,sizeof(cloth_maps));memset(&cloth_producer,0,sizeof(cloth_producer));
 memset((void *)JNETCompanionFeatures,0,sizeof(JNETCompanionFeatures));memset(job_object,0,sizeof(job_object));
 cloth_participant=current->participant;cloth_participant_ready=1;cloth_production_thread=7;cloth_production_pass=0;cloth_production_serial=0;
 active_cloth_backend=&backend;owner_ok=1;dependencies=stock_workers=routed_workers=original_finishes=seals=commits=finalizes=unmaps=0;flip_busy=write_fail=0;
}
static void run_worker(unsigned slot) {
 unsigned char *entry=current->barn+0x669d10+768*16+(slot-1)*16;
 cloth_worker_observer(slot&1,NULL,entry,current->barn+0x669ac0,current->barn+0x669960,NULL,current->barn+0x270080+slot*0xe0);
}
int main(void) {
 current=calloc(1,sizeof(*current));assert(current);unsigned scenarios=0;
 start_case(0);cleanup(current);setup(current,1);cloth_participant=current->participant;
 cloth_maps.roles=0;cloth_participant_ready=1;cloth_production_thread=0;cloth_production_pass=0;
 launch_second_pass=1;unsigned char second_result[16];
 assert(cloth_finish_observer((uintptr_t)current->barn,second_result,0,99)==(uintptr_t)second_result);
 assert(cloth_producer.adopted && cloth_producer.pass==1 && !cloth_participant_ready);
 assert(!cloth_production_thread && !cloth_production_pass);cleanup(current);++scenarios;
 for(unsigned fail=4;fail<=8;++fail) {
  if(fail==5)continue;start_case(fail);
  assert(cloth_dependency_observer((uintptr_t)current->barn+0x6bce00)==123);
  assert(dependencies==1&&!backend.failed&&JNETCompanionFeatures[30]==1&&!cloth_participant_ready);
  run_worker(1);run_worker(2);run_worker(3);run_worker(4);
  assert(stock_workers==2&&!routed_workers&&JNETCompanionFeatures[31]==2);
  assert(cloth_producer.lease.state==JC_CLOTH_LIVE);
  current->dude[0x81]=0;run_worker(1);
  assert(stock_workers==3&&JNETCompanionFeatures[31]==2);
  cleanup(current);++scenarios;
 }
 start_case(9);assert(cloth_dependency_observer((uintptr_t)current->barn+0x6bce00)==0);
 assert(!dependencies&&backend.failed&&cloth_producer.quarantined&&!JNETCompanionFeatures[30]);cleanup(current);++scenarios;
 start_case(0);assert(cloth_dependency_observer((uintptr_t)current->barn+0x6bce00)==123);
 assert(dependencies==1&&cloth_maps.launched_roles==cloth_maps.roles&&!backend.failed);
 run_worker(1);run_worker(2);assert(routed_workers==2&&!stock_workers&&!JNETCompanionFeatures[31]);cleanup(current);++scenarios;
 start_case(4);active_cloth_backend=NULL;run_worker(1);assert(stock_workers==1&&!JNETCompanionFeatures[31]);cleanup(current);++scenarios;
 start_case(4);cloth_participant.dude=0;run_worker(1);assert(stock_workers==1&&!JNETCompanionFeatures[31]);cleanup(current);++scenarios;
 for(unsigned variant=0;variant<3;++variant) {
  start_case(0);cloth_participant_ready=0;cloth_maps.roles=0x30;cloth_maps.barn=(uintptr_t)current->barn;cloth_maps.generation=1;
  struct jc_cloth_frame *frame=backend.frames.frames;frame->generation=1;frame->phase=JC_FRAME_WRITING;
  frame->pass_required=1;frame->pass_roles[0]=0x30;frame->source_args=current->barn+0x669ac0;
  current->barn[0x6bcf84]=1;put64(current->barn+0x6bcef0,(uintptr_t)job_object);job_object[0x58]=1;
  flip_busy=variant!=0;write_fail=variant==2;
  unsigned char result[16];memset(result,0xcd,sizeof(result));
  assert(cloth_finish_observer((uintptr_t)current->barn,result,0,99)==(uintptr_t)result);
  assert(original_finishes==(variant==2)&&!seals&&!commits);
  if(variant<2) {
   uint64_t pending_job;memcpy(&pending_job,result+8,8);
   assert(!result[0]&&!backend.failed&&pending_job==(uintptr_t)job_object);
   job_object[0x58]=0;flip_busy=0;
   assert(cloth_finish_observer((uintptr_t)current->barn,result,0,99)==(uintptr_t)result);
   assert(original_finishes==1&&seals==1&&commits==1&&finalizes==1&&unmaps==1&&result[0]==1);
  } else assert(backend.failed&&frame->phase==JC_FRAME_UNKNOWN);
  cleanup(current);++scenarios;
 }
 active_cloth_backend=NULL;assert(!cloth_quarantined_close(WM_CLOSE) && !terminated);
 active_cloth_backend=&backend;backend.failed=0;assert(!cloth_quarantined_close(WM_CLOSE) && !terminated);
 backend.failed=1;assert(!cloth_quarantined_close(0) && !terminated);
 assert(cloth_quarantined_close(WM_CLOSE) && terminated==1 && flushed==1);
 terminate_ok=0;assert(cloth_quarantined_close(WM_CLOSE) && terminated==2 && flushed==2);
 puts("Quarantined close: only an explicit close on the failed current process requests termination; normal close/messages remain native; OS calls simulated");
 free(current);printf("Production cloth integration: %u ordinary decline, stock preservation, quarantine, owned routing and native busy-result race scenarios passed.\n",scenarios);
}
"""
fixture = root / 'work/native_cloth_fallback_fixture.c'
binary = root / 'work/native_cloth_fallback_fixture'
fixture.write_text(cloth_fixture)
subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-I', str(root / 'diagnostic'), str(fixture), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)

backend_source = (root / 'diagnostic/traveler_live_backend.h').read_text()
a = backend_source.index('static int traveler_live_detach_body_probe(')
b = backend_source.index('static int traveler_live_ready(', a)
detach_fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { JC_FRAME_OK,JC_FRAME_OWNERSHIP,JC_FRAME_BUSY,JC_FRAME_MATERIAL,
       JC_ACTOR_RUNTIME_RETIRED=9 };
struct jc_environment_probe { uint64_t pair[2]; unsigned pins; };
struct jc_cloth_live { unsigned closed; struct jc_environment_probe probes[2]; };
struct jc_actor_identity { unsigned unused; };
struct fixture_actor { unsigned state; struct jc_actor_identity identity; };
struct fixture_live { struct jc_cloth_live backend; };
struct jc_cloth_graphics_token { unsigned locked; };
struct jc_environment_texture { uint64_t texture; };
static struct fixture_live live,*traveler_live=&live;
static struct fixture_actor actor,*traveler_actor=&actor;
static unsigned traveler_actor_materials;
static uint64_t traveler_object,traveler_game,barn_addr,stock=0x1000,private_texture=0x2000,binding;
static uint32_t refs=1,private_refs=1;
static unsigned retired,independent,set_calls,bad_set,pair_reads,change_pair,lock_calls,unlock_calls,probe_bad;
static unsigned char object[0x400],game[0x200],*barn;
static int own_read(void *ctx,uint64_t address,void *out,size_t n) {
 (void)ctx;
 if(address==barn_addr+0x646d40) {
  assert(n==16);uint64_t pair[2]={stock,(uintptr_t)&refs};
  if(change_pair && ++pair_reads>1)pair[0]=0x3000;
  memcpy(out,pair,16);return 1;
 }
 if(address==(uintptr_t)&refs && n==4) {memcpy(out,&refs,4);return 1;}
 if(address==traveler_game+0x108 && n==8) {memcpy(out,&barn_addr,8);return 1;}
 if(address==traveler_object+0x81 && n==1) {*(unsigned char *)out=1;return 1;}
 assert(!"unexpected production read");return 0;
}
static int traveler_live_retired(void *ctx,const struct jc_actor_identity *id) {
 (void)ctx;assert(id==&actor.identity);return retired;
}
static int jc_actor_material_independent(void *read,void *ctx,const struct jc_actor_identity *id,void *proof) {
 (void)read;(void)ctx;assert(id==&actor.identity && proof==&traveler_actor_materials);return !independent;
}
static int jc_cloth_live_graphics_enter(struct jc_cloth_live *s,struct jc_cloth_graphics_token *t) {
 assert(s==&live.backend);++lock_calls;t->locked=1;return 1;
}
static int jc_cloth_live_graphics_leave(struct jc_cloth_live *s,struct jc_cloth_graphics_token *t) {
 assert(s==&live.backend && t->locked);++unlock_calls;t->locked=0;return 1;
}
static int jc_environment_texture_read(void *read,void *ctx,uint64_t texture,struct jc_environment_texture *out) {
 (void)read;(void)ctx;out->texture=texture;return 0;
}
static int jc_environment_texture_ready(const struct jc_environment_texture *t) {return t->texture==stock;}
static int jc_environment_texture_same(const struct jc_environment_texture *a,const struct jc_environment_texture *b) {
 return a->texture==b->texture;
}
static int jc_cloth_texture_check(void *read,void *ctx,uint64_t material,const char *name,uint64_t texture,unsigned bound) {
 (void)read;(void)ctx;(void)texture;assert(material==traveler_object+0x1e0 && !strcmp(name,"envOccCube") && !bound);return 0;
}
static int jc_cloth_texture_bound(void *read,void *ctx,uint64_t material,const char *name,uint64_t texture) {
 (void)read;(void)ctx;assert(material==traveler_object+0x1e0 && !strcmp(name,"envOccCube"));return binding!=texture;
}
static int jc_environment_probe_check(void *read,void *ctx,struct jc_environment_probe *p) {
 (void)read;(void)ctx;assert(p==live.backend.probes);return probe_bad;
}
static uintptr_t setter(uintptr_t material,uintptr_t name,uintptr_t texture) {
 assert(lock_calls>unlock_calls && material==traveler_object+0x1e0 && !strcmp((char *)name,"envOccCube") && texture==stock);
 ++set_calls;if(!bad_set)binding=texture;return 0;
}
static uintptr_t (*original_cloth_setter[3])(uintptr_t,uintptr_t,uintptr_t)={NULL,NULL,setter};
'''
detach_fixture += backend_source[a:b]
detach_fixture += r'''
static void setup(void) {
 memset(&live,0,sizeof(live));actor.state=JC_ACTOR_RUNTIME_RETIRED;
 traveler_live=&live;traveler_actor=&actor;traveler_object=(uintptr_t)object;traveler_game=(uintptr_t)game;
 live.backend.probes[0].pair[0]=private_texture;live.backend.probes[0].pair[1]=(uintptr_t)&private_refs;
 binding=private_texture;refs=1;retired=independent=1;
 set_calls=bad_set=pair_reads=change_pair=lock_calls=unlock_calls=probe_bad=0;
}
int main(void) {
 barn=calloc(1,0x647000);assert(barn);barn_addr=(uintptr_t)barn;unsigned cases=0;
 setup();retired=0;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_BUSY && !set_calls && !lock_calls);++cases;
 setup();change_pair=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();binding=0x4000;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();bad_set=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && set_calls==1 && binding==private_texture && lock_calls==unlock_calls);++cases;
 setup();assert(!traveler_live_detach_body_probe(&live.backend) && binding==stock && set_calls==1);
 assert(!traveler_live_detach_body_probe(&live.backend) && binding==stock && set_calls==2 && lock_calls==unlock_calls);
 live.backend.closed=1;assert(!traveler_live_detach_body_probe(&live.backend) && set_calls==2);++cases;
 setup();independent=0;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && !lock_calls);++cases;
 setup();live.backend.probes[0].pins=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();probe_bad=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();memset(live.backend.probes,0,sizeof(live.backend.probes));traveler_actor=NULL;
 assert(!traveler_live_detach_body_probe(&live.backend) && !set_calls && !lock_calls);++cases;
 free(barn);printf("Production body probe detachment: %u retirement, identity, foreign binding, failed setter, repeated close and lease cases passed.\n",cases);
}
'''
fixture = root / 'work/native_body_probe_detach_fixture.c'
binary = root / 'work/native_body_probe_detach_fixture'
fixture.write_text(detach_fixture)
subprocess.run(['clang', '-std=c11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                str(fixture), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)

a = backend_source.index('static struct jc_body_appearance_lease traveler_body_lease;')
b = backend_source.index('static int traveler_live_begin(', a)
body_fixture = r'''
#define main appearance_component_main
#include "test_native_appearance.c"
#undef main
#define JC_RVA_MaterialScalarLookup 0x273a00
#define JC_FRAME_OWNERSHIP 90
struct jc_actor_identity {uint64_t image,barn,actor;};
struct jc_cloth_graphics_token {unsigned held;};
struct body_live {unsigned backend;};
static struct body_live body_live,*traveler_live=&body_live;
static uint64_t traveler_object;
static unsigned traveler_actor_materials,body_thread=7,body_destroyed,body_locked,body_independent=1;
static struct jc_actor_identity body_id;
static unsigned GetCurrentThreadId(void){return body_thread;}
static void message(const char *line){assert(line && *line);puts(line);}
static int own_read(void *ctx,uint64_t at,void *out,size_t n) {
    (void)ctx;
    if(at==body_id.barn+0xb0+0x2238 && n==8){uint64_t definition=base+0x3000;memcpy(out,&definition,8);return 1;}
    if((at==body_id.barn+0xb0+0x70 || at==body_id.actor+0x70) && n==4){uint32_t outfit=4;memcpy(out,&outfit,4);return 1;}
    if(at==body_id.actor+0x168 && n==8){uint64_t model=body_destroyed?0:1;memcpy(out,&model,8);return 1;}
    if(at>=body_id.barn+0x148d0 && at<body_id.barn+0x148d0+0x80)at=materials[1]+(at-(body_id.barn+0x148d0));
    return read_memory(NULL,at,out,n);
}
static int traveler_live_identity(const struct jc_actor_identity *id){return id==&body_id;}
static int jc_actor_material_independent(jc_layout_reader read,void *ctx,const struct jc_actor_identity *id,void *proof) {
    (void)read;(void)ctx;assert(id==&body_id && proof==&traveler_actor_materials);return !body_independent;
}
static int jc_cloth_live_graphics_enter(void *s,struct jc_cloth_graphics_token *token){
    assert(s==&body_live.backend && !body_locked);token->held=body_locked=1;return 1;
}
static int jc_cloth_live_graphics_leave(void *s,struct jc_cloth_graphics_token *token){
    assert(s==&body_live.backend && body_locked && token->held);token->held=body_locked=0;return 1;
}
struct jc_cloth_atlas_native {uint64_t image;jc_layout_reader read;void *ctx,*device;};
static int jc_cloth_atlas_native_hold(void *ctx,const uint64_t pair[2]){(void)ctx;assert(body_locked);return body_hold(NULL,pair);}
static void jc_cloth_atlas_native_drop(void *ctx,const uint64_t pair[2]){(void)ctx;assert(body_destroyed);body_drop(NULL,pair);}
static float *body_lookup(uintptr_t payload,const char *name){
    assert(body_locked && payload==base+0x20300);
    for(unsigned i=0;i<13;++i)if(!strcmp(name,jc_appearance_names[i]))return (float *)(memory+0x21004+i*32);
    return NULL;
}
static int jc_local_windows_write(uint64_t at,const void *value,size_t size){
    assert(body_locked && at>=(uintptr_t)memory && at+size<=(uintptr_t)(memory+sizeof(memory)));
    memcpy((void *)(uintptr_t)at,value,size);return 1;
}
static uintptr_t body_vector_set(uintptr_t m,uintptr_t name,uintptr_t value){assert(body_locked);return set_vector(NULL,m,(const char *)name,(const float *)value);}
static uintptr_t body_matrix_set(uintptr_t m,uintptr_t name,uintptr_t value){(void)m;(void)name;(void)value;assert(!"matrix setter called for vector");return 0;}
static uintptr_t body_texture_set(uintptr_t m,uintptr_t name,uintptr_t value){assert(body_locked);return set_texture(NULL,m,(const char *)name,value);}
static uintptr_t (*original_cloth_setter[3])(uintptr_t,uintptr_t,uintptr_t)={body_vector_set,body_matrix_set,body_texture_set};
''' + backend_source[a:b] + r'''
int main(void){
    body_fixture();traveler_object=materials[2]-0x1e0;
    body_id=(struct jc_actor_identity){(uintptr_t)body_lookup-JC_RVA_MaterialScalarLookup,materials[0]-0x290,traveler_object};
    assert(traveler_body_prepare(&body_id) && body_holds==4 && traveler_body_lease.applied && !body_locked);
    unsigned calls=setter_calls;
    assert(traveler_body_prepare(&body_id) && setter_calls==calls && body_holds==4);
    assert(!traveler_body_release(traveler_object) && !body_drops);
    body_destroyed=1;put(0x20030,0);body_thread=8;
    assert(!traveler_body_release(traveler_object) && !body_drops);
    body_thread=7;assert(traveler_body_release(traveler_object) && body_drops==4);
    assert(traveler_body_release(traveler_object) && body_drops==4);
    body_fixture();body_independent=0;
    assert(!traveler_body_prepare(&body_id) && !body_holds && !setter_calls);
    puts("Production body appearance: actual vector/texture setters, graphics scope, one-time preparation, owner thread and native material destruction gate passed; native refcounts simulated.");
}
'''
fixture=root/'work/native_body_appearance_fixture.c'
binary=root/'work/native_body_appearance_fixture'
fixture.write_text(body_fixture)
subprocess.run(['clang','-std=c11','-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                '-I',str(root/'diagnostic'),str(fixture),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True)
