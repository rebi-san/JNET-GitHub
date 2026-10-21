#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <assert.h>
typedef int32_t LONG;
typedef void *HANDLE;
#define JC_LINK_RVA 0
#define JC_COMMAND_RVA 64
#define JC_MESH_LIFE_RVA 68
#define JC_FRAMES_RVA 676
#define JC_APPEARANCE_RVA 680
#define JC_PERFORMANCE_RVA 840
static LONG link_data[16],life_data[152],appearance_data[40],performance_data[30],pending_data,frames_data;
static int remote_read(HANDLE process,uintptr_t location,void *result,size_t length) {
    (void)process; const void *source=NULL; size_t expected=0;
    switch (location) {
    case JC_LINK_RVA: source=link_data; expected=sizeof(link_data); break;
    case JC_COMMAND_RVA: source=&pending_data; expected=sizeof(pending_data); break;
    case JC_MESH_LIFE_RVA: source=life_data; expected=sizeof(life_data); break;
    case JC_FRAMES_RVA: source=&frames_data; expected=sizeof(frames_data); break;
    case JC_APPEARANCE_RVA: source=appearance_data; expected=sizeof(appearance_data); break;
    case JC_PERFORMANCE_RVA: source=performance_data; expected=sizeof(performance_data); break;
    }
    assert(source && length==expected); memcpy(result,source,length); return 1;
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
static int link_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG link[16],life[152],frames,pending,appearance[40],performance[30];
    if (!remote_read(process,base+JC_LINK_RVA,link,sizeof(link)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!link[2] && !link[0] && pending!=11 && pending!=12 && pending!=13 && pending!=14) return 0;
    if (!remote_read(process,base+JC_MESH_LIFE_RVA,life,sizeof(life)) ||
        !remote_read(process,base+JC_FRAMES_RVA,&frames,sizeof(frames)) ||
        !remote_read(process,base+JC_APPEARANCE_RVA,appearance,sizeof(appearance)) ||
        !remote_read(process,base+JC_PERFORMANCE_RVA,performance,sizeof(performance))) return -1;
    if (link[2]==2) {
        int n=swprintf(text,cap,
            L"LOCAL POSE LINK - SOURCE - Scarfhelper 0.63\r\n"
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
    int n=swprintf(text,cap,
        L"LOCAL POSE LINK - Scarfhelper 0.63\r\n"
        L"Role: %ls. State: %ld (0 pending; 1 active; 3 closed; -1 failed). Error: %ld.\r\n"
        L"Game update: %ld. Pending command: %ld. Session seconds remaining: %ld.\r\n\r\n"
        L"Packets received: %ld. Rejected: %ld. Last receive code: %ld.\r\n"
        L"Packets published: %ld. Capture/send code: %ld (0 sent; 7 invalid capture; 9 busy; -1 waiting). Published chapter: %ls.\r\n"
        L"Peer supports reciprocal poses: %ld.\r\n"
        L"Packet codes: 0 accepted; 1 clock; 2 rate; 3 format/build; 4 authentication; 5 identity; 6 sequence; 7 invalid capture/pose; 8 different chapter.\r\n"
        L"Receiver fresh input: %ld. Last accepted packet age: %ld ms. Source chapter: %ls.\r\n"
        L"Source distance at last applied pose: %.3f world units (-0.001 unavailable).\r\n"
        L"Applied source poses: %ld. Render state: %ld (1 waiting; 2 drawing; 3 ended; -1 failed).\r\n"
        L"Validated consumer calls: %ld / 10800. Render failure: %ld. Call stage: %ld.\r\n"
        L"Appearance result/stage: %ld / %ld. Torso result/stage: %ld / %ld.\r\n"
        L"Render failures: 3 context; 4 ownership; 5 queue; 10 budget; 15 checkpoint; 19 chapter; 20/21 appearance; 22 torso; 23 pose/references.\r\n"
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
        L"Reciprocal test: BOTH clients need Capacity Test and a retained trial prepared while solo. Host uses Link Trial / Copy Invite; the other uses Join Trial. Send Pose is the old publish-only mode.\r\n"
        L"Each prepared client renders one peer at its actual position. Move each separately and inspect BOTH reports. Updates are limited to 2 per second in each direction.\r\n"
        L"If a game pauses in the background, no state advances. Receiver hides the body after 5 seconds without fresh input and resumes on a valid newer packet.\r\n"
        L"Link admission lasts ten minutes; drawing ends after three minutes or 10800 calls. Stop Link ends this run; restart both games to create another link. Do not change the receiver's chapter.\r\n\r\n"
        L"Body movement only: no cape/scarf, active animation or shared interactions. Rebishaz's vanilla view does not receive the third traveler. Not full three-player co-op. Localhost only; no encryption. Vanilla matchmaking unchanged.\r\n"
        L"Invite secrets are copied only by Copy Invite; do not paste them into logs/chat. Resources remain until Journey exits. Status snapshots are not atomic.",
        link[2]==1 ? L"host" : link[2]==3 ? L"reciprocal join" : L"pending",
        (long)link[0],(long)link[1],(long)frames,(long)pending,(long)link[10],
        (long)link[3],(long)link[4],(long)link[5],(long)link[12],(long)link[13],level_label(link[14]),
        (long)link[15],(long)link[6],(long)link[7],level_label(link[8]),
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
        (long)performance[7],appearance_parameter(performance[8]),(long)performance[9],(long)performance[10],(long)performance[11]);
    return n>0 && (size_t)n<cap ? 1 : -1;
}
int main(void) {
    wchar_t text[6144];
    for (unsigned i=0;i<16;++i) link_data[i]=INT32_MAX;
    for (unsigned i=0;i<152;++i) life_data[i]=INT32_MAX;
    for (unsigned i=0;i<40;++i) appearance_data[i]=INT32_MAX;
    for (unsigned i=0;i<30;++i) performance_data[i]=INT32_MIN;
    for (LONG mode=1;mode<=3;++mode) {
        link_data[2]=mode; link_data[8]=link_data[14]=0;
        assert(link_report(NULL,0,text,6144)==1);
        assert(wcslen(text)<6144);
        if (mode!=2) {
            assert(wcsstr(text,L"Packets published:") && wcsstr(text,L"Ownership result:"));
            assert(wcsstr(text,L"Join Trial"));
        }
        printf("Status role %d: %zu characters, full formatter passed.\n",mode,wcslen(text));
    }
    memset(link_data,0,sizeof(link_data)); pending_data=14;
    assert(link_report(NULL,0,text,6144)==1);
    assert(wcsstr(text,L"Role: pending"));
    pending_data=0; assert(link_report(NULL,0,text,6144)==0);
    assert(link_report(NULL,0,text,1)==0);
    link_data[2]=3; assert(link_report(NULL,0,text,128)==-1);
    return 0;
}
