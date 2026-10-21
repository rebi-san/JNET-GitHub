#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "native_initialized_lifetime.h"
#include "native_mesh_callbacks.h"
#include "native_render_queue.h"
#include "native_appearance.h"
typedef int32_t LONG; typedef uint32_t DWORD; typedef uint64_t ULONGLONG;
#define JC_BUILD_ID {0}
static inline LONG InterlockedExchange(volatile LONG *p,LONG x) {LONG old=*p;*p=x;return old;}
 volatile LONG JCProbeStartup[5] = {0};
 volatile LONG JCProbeStatus = 0;
 volatile LONG JCProbeCommand = 0;
 volatile LONG JCProbeObservation = 0;
 volatile LONG JCProbePhase = 0;
 volatile LONG JCProbeFrames = 0;
 volatile LONG JCProbeTrial = 0;
 volatile LONG JCProbeLifetime = 0;
 volatile LONG JCProbeResource = 0;
 volatile LONG JCProbePool[5] = {-1,0,0,0,0};
 volatile LONG JCProbeMesh[6] = {0};
 volatile LONG JCProbeMaterial = 0;
 volatile LONG JCProbeCallbacks[11] = {-1,0,0,-1,0,-1,-1,-1,0,0,0};
 volatile LONG JCProbeMeshLife[152] = {[32]=-1,[40]=-1,[49]=-1,[55]=-1,[57]=-1,[58]=-1,[69]=-1,[81]=-1,[84]=-1,[92]=-1,[93]=-1,[94]=-1,[96]=-1,[97]=-1,[98]=-1,[108]=-1,[110]=-1,[112]=-1,[118]=-1,[127]=-1,[137]=-1,[138]=-1,[144]=-1,[150]=-1};
 volatile LONG JCProbeRender[23] = {0};
 volatile LONG JCProbeAppearance[40] = {-1,0,0,-1,0,0,0,0,0,-1,0,0,-1,-1,-1,[22]=-1,[23]=-1,[24]=-1,[34]=-1,[37]=-1,[38]=-1};
 volatile LONG JCProbeIdle[5] = {-1,0,-1,-1,-1};
 volatile LONG JCProbeAnimation[17] = {-1,-1};
 volatile LONG JCProbePoseWrites[8] = {-1,-1,0,0,0,0,0,0};
 volatile LONG JCProbePose[3] = {-1,-1,-1};
 volatile LONG JCProbeCommit[3] = {-1,0,0};
 volatile LONG JCProbeBuilder[8] = {0};
 volatile LONG JCProbeGeometry[9] = {-1,0,0,0,0,-1,-1,-1,0};
 void *volatile JCProbeGame = NULL;
 const unsigned char JCProbeBuildId[32] = JC_BUILD_ID;

 volatile LONG JCProbePerformance[30]={[1]=-1,[6]=-1,[8]=-1,[9]=-1,[18]=-1,[19]=-1,[26]=-1,[29]=-1};
#define JC_BODY_COUNT 2u
#define JC_BODY_CAPACITY 4u
 volatile LONG JCProbeBodies[48]={0};
 volatile LONG JCProbeDual[8]={0};
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
    memcpy(body_contexts[0].data_Mesh,(const void *)JCProbeMesh,6*sizeof(LONG));
    body_contexts[0].out_Mesh=JCProbeMesh;
    memcpy(body_contexts[0].data_Material,(const void *)&JCProbeMaterial,1*sizeof(LONG));
    body_contexts[0].out_Material=&JCProbeMaterial;
    memcpy(body_contexts[0].data_Callbacks,(const void *)JCProbeCallbacks,11*sizeof(LONG));
    body_contexts[0].out_Callbacks=JCProbeCallbacks;
    memcpy(body_contexts[0].data_MeshLife,(const void *)JCProbeMeshLife,152*sizeof(LONG));
    body_contexts[0].out_MeshLife=JCProbeMeshLife;
    memcpy(body_contexts[0].data_Appearance,(const void *)JCProbeAppearance,40*sizeof(LONG));
    body_contexts[0].out_Appearance=JCProbeAppearance;
    memcpy(body_contexts[0].data_Idle,(const void *)JCProbeIdle,5*sizeof(LONG));
    body_contexts[0].out_Idle=JCProbeIdle;
    memcpy(body_contexts[0].data_Animation,(const void *)JCProbeAnimation,17*sizeof(LONG));
    body_contexts[0].out_Animation=JCProbeAnimation;
    memcpy(body_contexts[0].data_PoseWrites,(const void *)JCProbePoseWrites,8*sizeof(LONG));
    body_contexts[0].out_PoseWrites=JCProbePoseWrites;
    memcpy(body_contexts[0].data_Pose,(const void *)JCProbePose,3*sizeof(LONG));
    body_contexts[0].out_Pose=JCProbePose;
    memcpy(body_contexts[0].data_Commit,(const void *)JCProbeCommit,3*sizeof(LONG));
    body_contexts[0].out_Commit=JCProbeCommit;
    memcpy(body_contexts[0].data_Geometry,(const void *)JCProbeGeometry,9*sizeof(LONG));
    body_contexts[0].out_Geometry=JCProbeGeometry;
    memcpy(body_contexts[0].data_Performance,(const void *)JCProbePerformance,30*sizeof(LONG));
    body_contexts[0].out_Performance=JCProbePerformance;
    body_contexts[1].index=1;
    memcpy(body_contexts[1].data_Mesh,(const void *)JCProbeMesh,6*sizeof(LONG));
    body_contexts[1].out_Mesh=body_contexts[1].data_Mesh;
    memcpy(body_contexts[1].data_Material,(const void *)&JCProbeMaterial,1*sizeof(LONG));
    body_contexts[1].out_Material=body_contexts[1].data_Material;
    memcpy(body_contexts[1].data_Callbacks,(const void *)JCProbeCallbacks,11*sizeof(LONG));
    body_contexts[1].out_Callbacks=body_contexts[1].data_Callbacks;
    memcpy(body_contexts[1].data_MeshLife,(const void *)JCProbeMeshLife,152*sizeof(LONG));
    body_contexts[1].out_MeshLife=body_contexts[1].data_MeshLife;
    memcpy(body_contexts[1].data_Appearance,(const void *)JCProbeAppearance,40*sizeof(LONG));
    body_contexts[1].out_Appearance=body_contexts[1].data_Appearance;
    memcpy(body_contexts[1].data_Idle,(const void *)JCProbeIdle,5*sizeof(LONG));
    body_contexts[1].out_Idle=body_contexts[1].data_Idle;
    memcpy(body_contexts[1].data_Animation,(const void *)JCProbeAnimation,17*sizeof(LONG));
    body_contexts[1].out_Animation=body_contexts[1].data_Animation;
    memcpy(body_contexts[1].data_PoseWrites,(const void *)JCProbePoseWrites,8*sizeof(LONG));
    body_contexts[1].out_PoseWrites=body_contexts[1].data_PoseWrites;
    memcpy(body_contexts[1].data_Pose,(const void *)JCProbePose,3*sizeof(LONG));
    body_contexts[1].out_Pose=body_contexts[1].data_Pose;
    memcpy(body_contexts[1].data_Commit,(const void *)JCProbeCommit,3*sizeof(LONG));
    body_contexts[1].out_Commit=body_contexts[1].data_Commit;
    memcpy(body_contexts[1].data_Geometry,(const void *)JCProbeGeometry,9*sizeof(LONG));
    body_contexts[1].out_Geometry=body_contexts[1].data_Geometry;
    memcpy(body_contexts[1].data_Performance,(const void *)JCProbePerformance,30*sizeof(LONG));
    body_contexts[1].out_Performance=body_contexts[1].data_Performance;
}
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
#define JCProbeMesh (body_context->out_Mesh)
#define JCProbeMaterial (*body_context->out_Material)
#define JCProbeCallbacks (body_context->out_Callbacks)
#define JCProbeMeshLife (body_context->out_MeshLife)
#define JCProbeAppearance (body_context->out_Appearance)
#define JCProbeIdle (body_context->out_Idle)
#define JCProbeAnimation (body_context->out_Animation)
#define JCProbePoseWrites (body_context->out_PoseWrites)
#define JCProbePose (body_context->out_Pose)
#define JCProbeCommit (body_context->out_Commit)
#define JCProbeGeometry (body_context->out_Geometry)
#define JCProbePerformance (body_context->out_Performance)
static void body_publish(void) {
    unsigned row=24*body_context->index;
    const LONG values[24]={JCProbeMeshLife[0],JCProbeMeshLife[25],JCProbeMeshLife[26],JCProbeMeshLife[27],
        JCProbePerformance[18],JCProbePerformance[19],JCProbeMeshLife[6],JCProbeMeshLife[7],JCProbeMeshLife[8],
        JCProbeAppearance[0],JCProbeAppearance[34],JCProbeMeshLife[116],body_context->mesh_guard_failure,
        JCProbePerformance[4],JCProbePerformance[5],JCProbePerformance[2],JCProbePerformance[3],
        JCProbePerformance[13],JCProbePerformance[14],JCProbePerformance[15],JCProbeGeometry[6],JCProbeMeshLife[2],
        JCProbeIdle[0],JCProbeMeshLife[28]};
    for (unsigned i=0;i<24;++i) InterlockedExchange(&JCProbeBodies[row+i],values[i]);
}
int main(void) {
 body_context_init();
 static unsigned char queues[2][0x10090];
 body_context=&body_contexts[0];
 initialized_life.object=0x200000;initialized_life.state=1;
 preview_queue=queues[0];preview_tracking.ledger.model=0x500000;
 preview_applied_transform[12]=10;preview_pose_sequence=7;preview_pose_identity[0]=1;
 preview_retry.consecutive=2;JCProbeMeshLife[25]=2;JCProbeMeshLife[26]=100;
 JCProbePerformance[4]=55;JCProbeAppearance[0]=0;body_publish();
 body_context=&body_contexts[1];
 assert(!initialized_life.object && !preview_queue && !preview_pose_sequence);
 assert(JCProbeMeshLife[25]==0 && JCProbePerformance[4]==0 && JCProbeAppearance[0]==-1);
 initialized_life.object=0x240000;initialized_life.state=1;
 preview_queue=queues[1];preview_tracking.ledger.model=0x600000;
 preview_applied_transform[12]=-30;preview_pose_sequence=7;preview_pose_identity[0]=2;
 preview_retry.consecutive=1;JCProbeMeshLife[25]=-1;JCProbeMeshLife[27]=21;JCProbePerformance[4]=88;body_context->mesh_guard_failure=4;body_publish();
 body_context=&body_contexts[0];
 assert(initialized_life.object==0x200000 && preview_queue==queues[0]);
 assert(preview_tracking.ledger.model==0x500000 && preview_applied_transform[12]==10);
 assert(preview_pose_identity[0]==1 && preview_pose_sequence==7 && preview_retry.consecutive==2);
 assert(JCProbeMeshLife[25]==2 && JCProbeMeshLife[26]==100 && JCProbeMeshLife[27]==0 && JCProbePerformance[4]==55);
 assert(JCProbeBodies[1]==2 && JCProbeBodies[25]==-1 && JCProbeBodies[27]==21);
 assert(JCProbeBodies[12]==0 && JCProbeBodies[36]==4);
 assert(body_contexts[0].out_MeshLife!=body_contexts[1].out_MeshLife);
 puts("Production body contexts: independent lifetime, queues, pose identity/sequence, transforms, material status, retries, failures and exported reports passed.");

 struct { int started,mode; } pose_link={1,0};
 for(unsigned i=0;i<2;++i){body_context=&body_contexts[i];preview_linked=1;JCProbeMeshLife[25]=2;}
    if (pose_link.started && !pose_link.mode) {
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i];
            if (preview_linked && (JCProbeMeshLife[25]==1 || JCProbeMeshLife[25]==2))
                InterlockedExchange(&JCProbeMeshLife[25],3);
            body_publish();
        }
        body_context=&body_contexts[0];
    }

 assert(JCProbeBodies[1]==3 && JCProbeBodies[25]==3);
 body_context=&body_contexts[1];JCProbeMeshLife[25]=-1;
    if (pose_link.started && !pose_link.mode) {
        for (unsigned i=0;i<JC_BODY_COUNT;++i) {
            body_context=&body_contexts[i];
            if (preview_linked && (JCProbeMeshLife[25]==1 || JCProbeMeshLife[25]==2))
                InterlockedExchange(&JCProbeMeshLife[25],3);
            body_publish();
        }
        body_context=&body_contexts[0];
    }

 assert(JCProbeBodies[25]==-1);
 return 0;
}
