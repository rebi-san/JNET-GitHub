#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "diagnostic_fault.h"
typedef int32_t LONG;
typedef void *HANDLE;
#define JC_LINK_RVA 1
#define JC_COMMAND_RVA 2
#define JC_MESH_LIFE_RVA 3
#define JC_FRAMES_RVA 4
#define JC_APPEARANCE_RVA 5
#define JC_PERFORMANCE_RVA 6
#define JC_ROOM_RVA 7
#define JC_BODIES_RVA 8
#define JC_DUAL_RVA 9
#define JC_FLIGHT_RVA 10
#define JC_MOTION_RVA 11
static LONG link_data[16],command,life[152],frames,appearance[40],performance[30],room[20],bodies[48],dual[8],flight[JC_FLIGHT_FIELDS],motion_data[28];
static int remote_read(HANDLE process,uintptr_t location,void *result,size_t length) {
 (void)process; const void *p=NULL;
 switch(location) {
 case 1:p=link_data;break;case 2:p=&command;break;case 3:p=life;break;
 case 4:p=&frames;break;case 5:p=appearance;break;case 6:p=performance;break;case 7:p=room;break;case 8:p=bodies;break;case 9:p=dual;break;case 10:p=flight;break;case 11:p=motion_data;break;
 default:return 0;
 }
 memcpy(result,p,length);return 1;
}
static const wchar_t *level_label(LONG level) {
    static const wchar_t *names[]={L"Level_Graveyard (chapter select)",L"Level_Barrens",L"Level_Desert",
        L"Level_Canyon",L"Level_Cave",L"Level_Ruins",L"Level_Mountain",L"Level_Summit",
        L"Level_Credits",L"Level_Bryan",L"Level_Matt",L"Level_Chris"};
    return level>=0 && level<12 ? names[level] : L"not sampled / unavailable";
}

static const wchar_t *appearance_parameter(LONG index) {
    static const wchar_t *names[]={L"specular",L"alphaHardness",L"localDudeGlow",L"remoteDudeGlow",
        L"textureAtlasA",L"textureAtlasB",L"textureDetail",L"dudeDecoGlow",L"textureMask",
        L"hsvHue",L"hsvSaturation",L"hsvValue",L"envOccCube"};
    return index>=0 && index<13 ? names[index] : L"none";
}
static const wchar_t *ownership_field(LONG index) {
    static const wchar_t *names[]={L"none",L"object",L"clone",L"source",L"slot",L"model",L"control",
        L"entries",L"count",L"manager",L"cache",L"head",L"clone pair",L"pool",L"state",L"game",
        L"thread",L"barn",L"post-cleanup",L"lock",L"restoration",L"checkpoint",L"budget"};
    return index>=0 && index<23 ? names[index] : L"not checked";
}
static int body_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG dual[8],body[48],flight[JC_FLIGHT_FIELDS],motion[28];
    if (!remote_read(process,base+JC_FLIGHT_RVA,flight,sizeof(flight)) ||
        !remote_read(process,base+JC_DUAL_RVA,dual,sizeof(dual)) ||
        !remote_read(process,base+JC_BODIES_RVA,body,sizeof(body)) ||
        !remote_read(process,base+JC_MOTION_RVA,motion,sizeof(motion))) return -1;
    int n=swprintf(text,cap,L"DUAL BODY: ready %ld (1 both checked; -1 failed). Preparation stage %ld (1 first; 2 second; 3 pair checked).\r\n"
        L"Keep Scarfhelper open: automatic process-exit capture is active. Fault observer: %ld (1 active). Last socket error: %ld.\r\n"
        L"Cross-body storage / mesh attempt / final ownership codes: %ld / %ld / %ld. Callback baseline: %ld.\r\n"
        L"Mesh attempt: 0 not reached; 2 cleaned without retention; 4 independence rejected; -2/-3 allocation rejected. Other codes: 0 accepted/not reached.\r\n",
        (long)dual[0],(long)dual[1],(long)flight[8],(long)flight[11],(long)dual[2],(long)dual[3],(long)dual[4],(long)dual[5]);
    if (n<=0 || (size_t)n>=cap) return -1;
    size_t used=(size_t)n;
    n=swprintf(text+used,cap-used,L"Animation capture result / active clips / total clips / ms: %ld / %ld / %ld / %ld. Target publish rate: 10 Hz.\r\n",
        (long)motion[0],(long)motion[1],(long)motion[2],(long)motion[3]);
    if (n<=0 || (size_t)n>=cap-used) return -1;
    used+=(size_t)n;
    n=swprintf(text+used,cap-used,L"Capture field / zero-based entry / raw value: %ld / %ld / 0x%08lx%08lx.\r\n"
        L"Fields: 0 none; 1 count; 2 descriptor; 3 additive; 4 weight; 5 name; 6 name capacity; 7 time; 8 duration; 9 speed; 10 active count. Native duration -1 is accepted.\r\n",
        (long)motion[18],(long)motion[19],(unsigned long)(uint32_t)motion[21],(unsigned long)(uint32_t)motion[20]);
    if (n<=0 || (size_t)n>=cap-used) return -1;
    used+=(size_t)n;
    for (unsigned i=0;i<2;++i) {
        LONG *b=body+24*i;
        n=swprintf(text+used,cap-used,
            L"Body %u: retained %ld; render %ld; draws %ld; render failure %ld; call stage %ld.\r\n"
            L"  Owner result/field %ld / %ls. Last pool slots/occupied/free %ld / %ld / %ld. Appearance/torso %ld / %ld.\r\n"
            L"  Pose ms last/peak %ld / %ld; draw ms last/peak %ld / %ld; budget skips/consecutive/recoveries %ld / %ld / %ld.\r\n"
            L"  Geometry accepted %ld; lifetime failure %ld; idle preparation %ld; checkpoint failure %ld; callback rejection %ld.\r\n",
            i+1,(long)b[0],(long)b[1],(long)b[2],(long)b[3],(long)b[23],(long)b[4],ownership_field(b[5]),
            (long)b[6],(long)b[7],(long)b[8],(long)b[9],(long)b[10],(long)b[13],(long)b[14],
            (long)b[15],(long)b[16],(long)b[17],(long)b[18],(long)b[19],(long)b[20],(long)b[21],(long)b[22],(long)b[11],(long)b[12]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
        LONG *m=motion+4+i*7;
        n=swprintf(text+used,cap-used,L"  Animation result / active clips / prepared / consumed / declines / ms: %ld / %ld / %ld / %ld / %ld / %ld. Relative spine changes: %ld.\r\n",
            (long)m[0],(long)m[1],(long)m[2],(long)m[5],(long)m[4],(long)m[3],(long)m[6]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
        LONG *cost=motion+22+i*3;
        n=swprintf(text+used,cap-used,L"  Animation reads last/peak: %ld / %ld. Last apply attempted writes: %ld. Untouched timeouts retry; partial writes stop.\r\n",
            (long)cost[0],(long)cost[1],(long)cost[2]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
    }
    n=swprintf(text+used,cap-used,L"Animation: -1 unavailable; 0 accepted; 1 read; 2 name; 3 changed; 4 bounds; 5 capacity; 6 schema/storage/binding; 7 write; 8 postcheck; 9 budget. Consumed verifies native clip input, not visible skeletal movement.\r\n"
        L"Callback rejection: 0 none; 1 context/read; 2 duplicate/state; 3 early callback; 4 entry range; 5 caller.\r\n");
    return n>0 && (size_t)n<cap-used ? (int)(used+(size_t)n) : -1;
}
static int link_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG link[16],life[152],frames,pending,appearance[40],performance[30];
    if (!remote_read(process,base+JC_LINK_RVA,link,sizeof(link)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!link[2] && !link[0] && pending!=11 && pending!=12 && pending!=13 && pending!=14 && pending!=15 && pending!=16 && pending!=17) return 0;
    if (!remote_read(process,base+JC_MESH_LIFE_RVA,life,sizeof(life)) ||
        !remote_read(process,base+JC_FRAMES_RVA,&frames,sizeof(frames)) ||
        !remote_read(process,base+JC_APPEARANCE_RVA,appearance,sizeof(appearance)) ||
        !remote_read(process,base+JC_PERFORMANCE_RVA,performance,sizeof(performance))) return -1;
    if (link[2]==2) {
        int n=swprintf(text,cap,
            L"LOCAL POSE LINK - SOURCE - Scarfhelper 0.72\r\n"
            L"State: %ld (1 publishing; 3 closed; -1 failed). Error: %ld.\r\n"
            L"Game update: %ld. Pending command: %ld. Session seconds remaining: %ld.\r\n\r\n"
            L"Packets sent: %ld. Last capture/send code: %ld. Last published chapter: %ls.\r\n"
            L"Code 0 means a packet was sent; 7 means the local pose/chapter could not be validated.\r\n"
            L"Link errors: 1 mode/configuration; 2 socket startup; 3 cryptography; 4 socket setup; 5 socket I/O; 6 encoding/sequence; 7 game/thread changed.\r\n\r\n"
            L"Move this character in Desert, then return promptly to devishaz to look for the extra body.\r\n"
            L"This source does not allocate or render a trial. Its send count is not an acknowledgement: check devishaz's received packets, applied poses and render status.\r\n"
            L"Background pausing stops publishing. The receiver hides the body after five seconds without fresh input.\r\n"
            L"Use Stop Link to finish; restart both games for another link. No need to connect through vanilla matchmaking.\r\n\r\n"
            L"One-way body-state test, not three-player co-op. No cape/scarf, active animation or shared interactions. IPv4 localhost; no encryption. Native matchmaking is unchanged.\r\n"
            L"Do not copy invite secrets into logs/chat. Status snapshots are not atomic.",
            (long)link[0],(long)link[1],(long)frames,(long)pending,(long)link[10],
            (long)link[3],(long)link[5],level_label(link[8]));
        return n>0 && (size_t)n<cap ? 1 : -1;
    }
    int room_mode=link[2]>=4 || pending==15 || pending==16;
    if (room_mode) {
        LONG room[20];
        if (!remote_read(process,base+JC_ROOM_RVA,room,sizeof(room))) return -1;
        int prefix=swprintf(text,cap,
            L"THREE-CLIENT ROOM: %ld / 2 peers known. Relayed packets: %ld. Deferred UDP errors: %ld.\r\n"
            L"Peer 1: known %ld, fresh %ld, age %ld ms, chapter %ls, received %ld, host %ld, applied %ld, presence %ld.\r\n"
            L"Peer 2: known %ld, fresh %ld, age %ld ms, chapter %ls, received %ld, host %ld, applied %ld, presence %ld.\r\n"
            L"Presence: 0 unknown; 1 fresh; 2 silent; 3 left; 4 other chapter; 5 room ended. Silence is not confirmed departure.\r\n"
            L"Both fresh peers draw together as independent bodies. Detailed diagnostics below the body summary describe Body 1.\r\n"
            L"On host, peers are numbered by admission order. On joiners, peer 1 is host and peer 2 is the other joiner.\r\n\r\n",
            (long)room[0],(long)room[2],(long)room[3],
            (long)room[4],(long)room[5],(long)room[6],level_label(room[7]),(long)room[8],(long)room[9],(long)room[10],(long)room[11],
            (long)room[12],(long)room[13],(long)room[14],level_label(room[15]),(long)room[16],(long)room[17],(long)room[18],(long)room[19]);
        if (prefix<=0 || (size_t)prefix>=cap) return -1;
        text+=prefix; cap-=(size_t)prefix;
        int bodies=body_report(process,base,text,cap);
        if (bodies<0) return -1;
        text+=bodies; cap-=(size_t)bodies;
    }
    int n=swprintf(text,cap,
        L"LOCAL POSE LINK - Scarfhelper 0.72\r\n"
        L"Role: %ls. State: %ld (0 pending; 1 active; 3 closed; -1 failed). Error: %ld.\r\n"
        L"Game update: %ld. Pending command: %ld. Session seconds remaining: %ld.\r\n\r\n"
        L"Packets received: %ld. Rejected: %ld. Last receive code: %ld.\r\n"
        L"Packets published: %ld. Capture/send code: %ld (0 sent; 7 invalid capture; 9 busy; -1 waiting). Published chapter: %ls.\r\n"
        L"Packet codes: 0 accepted; 1 clock; 2 rate; 3 format/build; 4 authentication; 5 identity; 6 sequence; 7 invalid capture/pose; 8 different chapter; 10 room full; 11 departed identity.\r\n"
        L"Receiver fresh input: %ld. Last accepted packet age: %ld ms. Source chapter: %ls.\r\n"
        L"Distance of the most recently applied pose: %.3f world units (-0.001 unavailable).\r\n"
        L"Applied poses total: %ld. Body 1 render state: %ld (1 waiting; 2 drawing; 3 ended; -1 failed).\r\n"
        L"Validated consumer calls: %ld / 10800. Render failure: %ld. Call stage: %ld.\r\n"
        L"Appearance result/stage: %ld / %ld. Torso result/stage: %ld / %ld.\r\n"
        L"Render failures: 3 context; 4 ownership; 5 queue; 10 budget; 15 checkpoint; 19 chapter; 20/21 appearance; 22 torso; 23 pose/references; 24 sibling pose changed/unreadable; 25 animation update.\r\n"
        L"Checkpoint attempts: %ld / 128. First checkpoint failure: %ld. Windows error: %lu.\r\n"
        L"Checkpoint codes: 0 none; 1 busy; 2 handle; 3 record cap; 4 line length; 5 line write; 6 newline write; 7 flush.\r\n"
        L"Pose preflight result/step: %ld / %ld (-1 pending; 0 accepted result).\r\n\r\n"
        L"PERFORMANCE AND RESOURCE CHECK\r\n"
        L"Inspection reader: %ld (1 guarded local; 0 ReadProcessMemory). Self-test: %ld (-1 disabled for compatibility; 0 passed).\r\n"
        L"Preview check/draw time last/peak: %ld / %ld ms. Changed-pose update last/peak: %ld / %ld ms.\r\n"
        L"Unchanged poses skipped: %ld. Freshness and resource checks still run.\r\n"
        L"Budget skips total/consecutive: %ld / %ld. Recovered draw runs: %ld. Eight consecutive timeouts stop drawing.\r\n"
        L"Last blocked check/result: %ld / %ld (check uses render-failure numbers; may have recovered).\r\n"
        L"Ownership result: %ld. Field: %ls. Check last/peak: %ld / %ld ms. Reads: %ld.\r\n"
        L"Ownership pool valid: %ld. Slots/occupied/free: %ld / %ld / %ld. Comparison valid/same pool/changed mask: %ld / %ld / %ld.\r\n"
        L"Check results: -1 not reached; 0 accepted; 1 read; 2 type; 3 changed; 4 range; 9 budget.\r\n"
        L"Borrowed-resource result: %ld.\r\n"
        L"Stage: %ld. Parameter: %ls. Material role: %ld (-1 not selected; 0 original source; 1 trial).\r\n"
        L"Stages: 0 accepted; 1 definition read; 2 definition texture; 3 borrowed source texture; 4 bindings read; 5 storage changed; 6 texture mismatch; 7 live texture; 8 definition pointer.\r\n"
        L"Borrow check last/peak: %ld / %ld ms. Times measure CPU checks/native calls, not GPU completion.\r\n\r\n"
        L"%ls\r\n"
        L"Pausing hides that ghost after five seconds; newer input can resume it. Stop Link sends a best-effort departure: a joiner leaves the other connection running; the host ends the room. No rejoin or sit/dust effect yet. Stay in Desert.\r\n"
        L"Admission lasts ten minutes; drawing ends after three minutes or 10800 calls. Stop Link ends this run; restart participating games for another run.\r\n"
        L"Replicated clip playback is experimental. No robe/scarf, chirp effects or shared interactions. Two independent ghosts per room client; gameplay interactions remain unfinished. Localhost only; no encryption. Vanilla matchmaking unchanged.\r\n"
        L"Invite secrets are copied only by Copy Invite; do not paste them into logs/chat. Resources remain until Journey exits. Status snapshots are not atomic.",
        link[2]==4 ? L"room host" : link[2]==5 ? L"room join" : link[2]==1 ? L"host" : link[2]==3 ? L"reciprocal join" : L"pending",
        (long)link[0],(long)link[1],(long)frames,(long)pending,(long)link[10],
        (long)link[3],(long)link[4],(long)link[5],(long)link[12],(long)link[13],level_label(link[14]),
        (long)link[6],(long)link[7],level_label(link[8]),
        link[11]/1000.0,(long)link[9],(long)life[25],(long)life[26],(long)life[27],(long)life[28],
        (long)appearance[0],(long)appearance[1],(long)appearance[34],(long)appearance[36],
        (long)life[115],(long)life[116],(unsigned long)life[117],(long)life[110],(long)life[113],
        (long)performance[0],(long)performance[1],(long)performance[2],(long)performance[3],
        (long)performance[4],(long)performance[5],(long)performance[12],
        (long)performance[13],(long)performance[14],(long)performance[15],
        (long)performance[16],(long)performance[17],(long)performance[18],ownership_field(performance[19]),
        (long)performance[20],(long)performance[21],(long)performance[22],(long)performance[26],
        (long)performance[23],(long)performance[24],(long)performance[25],
        (long)performance[29],(long)performance[27],(long)performance[28],(long)performance[6],
        (long)performance[7],appearance_parameter(performance[8]),(long)performance[9],(long)performance[10],(long)performance[11],
        room_mode ? L"ALL THREE clients need Capacity Test and Prepare Trial while solo. Devishaz: Host Room / Copy Invite. Smooviepuffs then rebishaz: Join Room with the SAME invite. Both remote ghosts should be visible together. Move all three characters independently. Each peer targets ten publishes per second; host forwards only newly accepted packets. Copy all three reports."
        : L"BOTH clients need Capacity Test and Prepare Trial while solo. Link Trial / Copy Invite hosts; Join Trial joins. Send Pose is publish-only. Each client renders one peer. Rebishaz's unmodified vanilla view does not receive a third traveler.");
    return n>0 && (size_t)n<cap ? 1 : -1;
}
int main(void) {
 wchar_t text[10000]; size_t largest=0;
 bodies[36]=5;dual[3]=2;
 assert(body_report(NULL,0,text,4000)>0);
 assert(wcsstr(text,L"callback rejection 5") && wcsstr(text,L"2 cleaned without retention"));
 assert(wcsstr(text,L"Callback baseline: 0"));
 for (unsigned i=0;i<28;++i) motion_data[i]=INT32_MAX;
 for (unsigned i=0;i<16;++i) link_data[i]=INT32_MAX;
 for (unsigned i=0;i<152;++i) life[i]=INT32_MAX;
 for (unsigned i=0;i<40;++i) appearance[i]=INT32_MAX;
 for (unsigned i=0;i<30;++i) performance[i]=INT32_MAX;
 for (unsigned i=0;i<20;++i) room[i]=INT32_MAX;
 for (unsigned i=0;i<48;++i) bodies[i]=INT32_MAX;
 for (unsigned i=0;i<8;++i) dual[i]=INT32_MAX;
 for (int mode=1;mode<=5;++mode) {
  link_data[2]=mode;
  assert(link_report(NULL,0,text,10000)==1);
  size_t n=wcslen(text);if(n>largest)largest=n;
  assert((wcsstr(text,L"THREE-CLIENT ROOM")!=NULL)==(mode>=4));
  if(mode>=4)assert(wcsstr(text,L"Relative spine changes:") && wcsstr(text,L"Animation capture result") && wcsstr(text,L"Peer 1:") && wcsstr(text,L"Peer 2:") && wcsstr(text,L"Body 2:") && wcsstr(text,L"applied") && wcsstr(text,L"Deferred UDP errors") && wcsstr(text,L"Silence is not confirmed departure"));
  for(unsigned i=0;i<10000;++i)text[i]=L'!';
  assert(link_report(NULL,0,text,16)==-1 && text[16]==L'!');
 }
 link_data[0]=link_data[2]=0;command=15;
 assert(link_report(NULL,0,text,10000)==1 && wcsstr(text,L"THREE-CLIENT ROOM"));
 command=16;assert(link_report(NULL,0,text,10000)==1 && wcsstr(text,L"THREE-CLIENT ROOM"));
 command=0;assert(link_report(NULL,0,text,10000)==0);
 printf("Production status formatter: all five roles, pending room actions and truncation checked; largest report %zu / 10000 characters.\n",largest);
 return 0;
}
