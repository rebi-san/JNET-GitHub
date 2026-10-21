#include "platform.h"
#include <commdlg.h>
#include "target_build.h"
#include "built_dll.h"
#include "scarf_banners.h"
#include "remote_pe.h"
#include "native_layout.h"
#include "native_resources.h"
#include "native_geometry_pool.h"
#include "diagnostic_fault.h"
#include "jnet_session_log.h"

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
static const wchar_t *appearance_source_reason(LONG code) {
    static const wchar_t *names[]={L"accepted",L"required source texture missing",L"required trial texture missing",
        L"source texture binding invalid",L"source texture changed before setters"};
    return code>=0 && code<5 ? names[code] : L"not checked";
}

static struct jnet_memory_log helper_log,exit_log,hang_log;
static const char *failure_stage = "unknown";
static DWORD failure_error, failure_detail;
static DWORD module_error;
static int requested_action;
static RECT result_position;
static int result_position_valid;
static HANDLE preview_process;
static uintptr_t preview_base;
static unsigned preview_polls;
#define JC_REPORT_CAP 40000
static wchar_t preview_text[JC_REPORT_CAP];
static wchar_t *report_target=preview_text;
static wchar_t preview_last_report[20000];
static int remote_read(HANDLE process, uintptr_t location, void *result, size_t length);
static int traveler_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap);
static int native_playback_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap);
static int append_game_context_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    SYSTEMTIME sampled;GetSystemTime(&sampled);
    LONG status=-1,frames=-1,phase=-1,observation=-1,state=-1,resume=-1;
    uintptr_t game=0;unsigned char pending=255;unsigned readable=0;
    char name[32]={0};wchar_t level[40]=L"unavailable";
    if(remote_read(process,base+JC_STATUS_RVA,&status,sizeof(status)))readable|=1;
    if(remote_read(process,base+JC_FRAMES_RVA,&frames,sizeof(frames)))readable|=2;
    if(remote_read(process,base+JC_PHASE_RVA,&phase,sizeof(phase)))readable|=4;
    if(remote_read(process,base+JC_OBSERVATION_RVA,&observation,sizeof(observation)))readable|=8;
    if(remote_read(process,base+JC_GAME_RVA,&game,sizeof(game)))readable|=16;
    if((readable&16) && game) {
        if(remote_read(process,game+0x1c02f0,name,sizeof(name)) && memchr(name,0,sizeof(name)) &&
           MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name,-1,level,40))readable|=32;
        if(remote_read(process,game+0x1c0330,&pending,1))readable|=64;
        if(remote_read(process,game+0x278,&state,sizeof(state)))readable|=128;
        if(remote_read(process,game+0x27c,&resume,sizeof(resume)))readable|=256;
    }
    return swprintf(text,cap,
        L"Report sampled UTC %04u-%02u-%02u %02u:%02u:%02u.\r\n"
        L"Game context: PID %lu; loader %ld; updates %ld; phase %ld; observation %ld; game observed %d; readable mask %03x.\r\n"
        L"Native level %ls; pending load byte %u; state/resume %ld/%ld. Readable bits: 1 loader, 2 updates, 4 phase, 8 observation, 16 game, 32 level, 64 pending, 128 state, 256 resume.\r\n"
        L"Zero updates means no game update has been observed; the travel current-level default is not a detected level. Steam readiness alone does not establish gameplay readiness. Snapshot reads are not atomic.\r\n",
        (unsigned)sampled.wYear,(unsigned)sampled.wMonth,(unsigned)sampled.wDay,
        (unsigned)sampled.wHour,(unsigned)sampled.wMinute,(unsigned)sampled.wSecond,
        (unsigned long)GetProcessId(process),(long)status,(long)frames,(long)phase,(long)observation,
        (readable&16)?(game!=0):-1,readable,(readable&32)?level:L"unavailable",
        (unsigned)pending,(long)state,(long)resume);
}
static int append_menu_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap,int result) {
    if(result<0 || !cap)return result;
    LONG menu[19];
    if(!remote_read(process,base+JC_MENU_RVA,menu,sizeof(menu)))return result;
    size_t used=result?wcslen(text):0;
    if(used>=cap)return -1;
    int n=append_game_context_report(process,base,text+used,cap-used);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=(size_t)n;
    n=swprintf(text+used,cap-used,
        L"\r\nJNET " JC_HELPER_VERSION L" title/pause menu: hook enabled %ld; entries %ld; caller RVA %08lx.\r\n"
        L"Accepted frames %ld; drawn %ld; last result %ld; glyph quads %ld.\r\n"
        L"Native mapping detail %ld: 0 valid; 1 pointer mismatch; 2 resource read; 3 not mapped; 4 byte length.\r\n"
        L"Results apply after an accepted frame: 0 drawn; 1 hidden/submenu; 2 invalid state; 3 missing/invalid glyph; 4 unavailable mapping; 5 insufficient space.\r\n"
        L"Image renderer result/frames: %ld/%ld (-1 suspended, 2 initialization unavailable, 3 target unavailable, 4 invalid saved state).\r\n"
        L"Menu checkpoint %ld: 1 entry; 2 native build; 3 native returned; 4 input; 5 publish; 6 text; 7 complete.\r\n"
        L"Image checkpoint %ld; last failing HRESULT %08lx. Stages: 1 renderer; 2 device; 3 compiler; 4 compile VS; 5 atlas; 6 pipeline; 7 target; 8 save; 9 draw; 10 restore; 11 complete; 12 compile PS; 13 create VS; 14 create PS; 15 quad input; 16-28 restore (see runtime log).\r\n"
        L"Inactive legacy local lobby: identity %ld; host %ld; population %ld/3; Desert ready %ld; admission pending %ld; discovery ready %ld.\r\n"
        L"Legacy local lobby controls are disabled. Appearance arrows edit previews; Appearance application remains unavailable.\r\n",
        (long)menu[6],(long)menu[4],(unsigned long)(uint32_t)menu[5],
        (long)menu[0],(long)menu[2],(long)menu[1],(long)menu[3],(long)menu[7],(long)menu[8],(long)menu[9],(long)menu[10],(long)menu[11],(unsigned long)(uint32_t)menu[12],(long)menu[13],(long)menu[14],(long)menu[15],(long)menu[16],(long)menu[17],(long)menu[18]);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;
    LONG steam[24]={0};
    if(!remote_read(process,base+JC_STEAM_RVA,steam,sizeof(steam)))return 1;
    uint64_t identity=(uint32_t)steam[7]|((uint64_t)(uint32_t)steam[8]<<32);
    n=swprintf(text+used,cap-used,
        L"Steam capability: state %ld; callback hook %ld; native callback calls %ld; user/pipe %ld/%ld.\r\n"
        L"Steam AppID %ld; online %ld; SteamID %llu. Matchmaking009/Sockets012/NetworkingUtils004: %ld/%ld/%ld.\r\n"
        L"States: 1 waiting for native callbacks; 2 not initialized; 3 offline; 4 required interface unavailable; 5 interfaces available.\r\n"
        L"Errors: -1 missing module/export; -2 hook unavailable; -3 missing binding; -4 user/utils unavailable; -5 unexpected AppID; -6 missing identity.\r\n"
        L"Steam services: checkpoint %ld; result %ld; relay/config/router/auth %ld/%ld/%ld/%ld; measuring ping %ld; initializations %ld; elapsed %ld ms.\r\n"
        L"Service checkpoints: 0 not sampled this poll; 1 bind; 2 relay init; 3 authentication init; 4 relay query; 5 authentication query; 6 complete. Result: -1 invalid binding; 0 untested; 1 ready; 2 pending; 3 unavailable/retrying.\r\n"
        L"Steam availability: 100 current; 3 attempting; 2 waiting; 1 never tried; 0 unknown; -10 retrying; -100 previously available; -101 failed; -102 cannot try. Service values are historical if capability state is not 5.\r\n"
        L"Steam group-travel candidate. MEPAL level travel is enabled for testing; invitations are unavailable.\r\n",
        (long)steam[0],(long)steam[1],(long)steam[2],(long)steam[3],(long)steam[4],
        (long)steam[5],(long)steam[6],(unsigned long long)identity,(long)steam[9],(long)steam[10],(long)steam[11],
        (long)steam[12],(long)steam[13],(long)steam[14],(long)steam[15],(long)steam[16],(long)steam[18],
        (long)steam[17],(long)steam[19],(long)steam[20]);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;char service_text[2][256]={{0}};
    if(!remote_read(process,base+JC_STEAM_TEXT_RVA,service_text,sizeof(service_text)))return 1;
    service_text[0][255]=service_text[1][255]=0;
    n=swprintf(text+used,cap-used,L"Steam relay detail: %hs\r\nSteam authentication detail: %hs\r\n",service_text[0],service_text[1]);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;LONG lobby[58]={0};char lobby_text[128]={0};
    if(!remote_read(process,base+JC_STEAM_LOBBY_RVA,lobby,sizeof(lobby)) ||
       !remote_read(process,base+JC_STEAM_LOBBY_TEXT_RVA,lobby_text,sizeof(lobby_text)))return 1;
    lobby_text[127]=0;
    n=swprintf(text+used,cap-used,
        L"Steam lobby: state %ld; isolation %ld; native calls held %ld; operation %ld; result %ld; public rows %ld; members %ld/3; generation %ld; draining %ld.\r\n"
        L"Level ready %ld; native ownership %ld (0 clear, 1 vanilla connection, -1 unreadable).\r\n"
        L"P2P state %ld; handshakes %ld; sent/received %ld/%ld; last result %ld.\r\n"
        L"Lobby state: 1 waiting; 2 ready; 3 joined; -1 binding failure. Operations: 1 browser; 2 code availability; 3 code lookup; 4 create; 5 join; 6 code collision recheck.\r\n"
        L"P2P state: 1 listening/connecting; 2 connected; 3 handshake and messages; -1 binding; -2 listen; -3 connect; -4 callback registration. Fresh validated Desert state automatically feeds native travelers.\r\n"
        L"Isolation 1 requires native search/create/join/publication and legacy P2P guards. Live vanilla isolation still needs verification.\r\n"
        L"Discovery: completed %ld; returned %ld; rejected %ld; first rejection %ld (1 mod marker, 2 protocol, 3 capacity, 4 status, 5 mode/level, 6 visibility, 7 advertised host, 8 nonce, 9 build mismatch).\r\n"
        L"Steam session detail: %hs\r\n",
        (long)lobby[0],(long)lobby[1],(long)lobby[2],(long)lobby[3],(long)lobby[4],(long)lobby[5],(long)lobby[6],(long)lobby[7],(long)lobby[8],
        (long)lobby[9],(long)lobby[10],(long)lobby[11],(long)lobby[12],(long)lobby[13],(long)lobby[14],(long)lobby[15],(long)lobby[16],(long)lobby[17],(long)lobby[18],(long)lobby[19],lobby_text);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;LONG p2p[305]={0};char detail[128]={0};
    if(!remote_read(process,base+JC_STEAM_P2P_RVA,p2p,sizeof(p2p)) || !remote_read(process,base+JC_STEAM_P2P_TEXT_RVA,detail,sizeof(detail)))return 1;
    detail[127]=0;
    n=swprintf(text+used,cap-used,
        L"P2P diagnostics: callbacks %ld; incoming requests %ld; accepted %ld; rejected %ld; last Steam state %ld; flags %ld; end reason %ld; Accept result %ld.\r\n"
        L"P2P callback registration %ld; ignored unrelated events %ld; registrations %ld.\r\n"
        L"Steam connection detail: %hs\r\n",
        (long)p2p[0],(long)p2p[1],(long)p2p[2],(long)p2p[3],(long)p2p[4],(long)p2p[5],(long)p2p[6],(long)p2p[7],(long)p2p[8],(long)p2p[9],(long)p2p[10],detail);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;
    n=swprintf(text+used,cap-used,
        L"Steam transport poll last/peak %ld/%ld ms; membership checks %ld; roster API calls %ld; cached roster reuses %ld.\r\n",
        (long)p2p[11],(long)p2p[12],(long)p2p[13],(long)p2p[14],(long)p2p[15]);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;
    n=swprintf(text+used,cap-used,
        L"Transport operations last/peak ms: roster read %ld/%ld; receive/process %ld/%ld; send call %ld/%ld. Membership notifications %ld; roster fallback 500 ms. Receive/process may include roster reads and chirp forwarding; peaks are not additive.\r\n",
        (long)p2p[17],(long)p2p[18],(long)p2p[19],(long)p2p[20],(long)p2p[21],(long)p2p[22],(long)p2p[16]);
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;
    n=swprintf(text+used,cap-used,L"All phases lobby + transport last/peak %ld/%ld ms; connection statistics query %ld/%ld ms. Statistics sampled once per second; peaks are not additive.\r\n",(long)lobby[20],(long)lobby[21],(long)p2p[23],(long)p2p[24]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,L"Session checks last/peak ms: metadata %ld/%ld; owner %ld/%ld. Foreground full validations %ld; metadata cache reuses %ld; data notifications %ld. Metadata fallback 2000 ms; owner/membership fallback 500 ms.\r\n",(long)lobby[22],(long)lobby[23],(long)lobby[24],(long)lobby[25],(long)lobby[26],(long)lobby[27],(long)lobby[28]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    for(unsigned bucket=0;bucket<3;++bucket) {
        unsigned at=29+bucket*7;
        n=swprintf(text+used,cap-used,L"Lobby timing %ls: last/peak %ld/%ld ms; polls %ld; <50/50-99/100-249/>=250 ms %ld/%ld/%ld/%ld. Cumulative this process, retained after leave.\r\n",bucket==0?L"steady admitted gameplay":bucket==1?L"idle/startup":L"discovery/join/leave/waiting",(long)lobby[at],(long)lobby[at+1],(long)lobby[at+2],(long)lobby[at+3],(long)lobby[at+4],(long)lobby[at+5],(long)lobby[at+6]);
        if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    }
    n=swprintf(text+used,cap-used,L"Background lobby checks: worker %ld (1 ready, -1 unavailable); read last/peak %ld/%ld ms; accepted %ld; fallback eligibility %ld; stale results %ld; cache uses %ld; invalid results %ld. Worker read time is off the game thread; snapshots expire after 500 ms.\r\n",(long)lobby[50],(long)lobby[51],(long)lobby[52],(long)lobby[53],(long)lobby[54],(long)lobby[55],(long)lobby[56],(long)lobby[57]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    int historical[7]={0};
    for(unsigned i=0;i<7;++i)if(!p2p[25+i*12+2] && p2p[165+i*20+2]) {
        historical[i]=1;
        for(unsigned j=0;j<12;++j)p2p[25+i*12+j]=p2p[165+i*20+j];
        for(unsigned j=0;j<8;++j)p2p[109+i*8+j]=p2p[177+i*20+j];
    }
    for(unsigned i=0;i<7;++i) {
        unsigned at=25+i*12;if(!p2p[at+2])continue;
        uint64_t id=(uint32_t)p2p[at]|((uint64_t)(uint32_t)p2p[at+1]<<32);
        n=swprintf(text+used,cap-used,L"Steam peer %llu (%ls): query result %ld; RTT last/peak %ld/%ld ms; estimated send queue last/peak %ld/%ld us; pending unreliable/reliable %ld/%ld bytes; unacked reliable %ld bytes; samples %ld.\r\n",(unsigned long long)id,historical[i]?L"ended; historical":L"current",(long)p2p[at+3],(long)p2p[at+4],(long)p2p[at+5],(long)p2p[at+6],(long)p2p[at+7],(long)p2p[at+8],(long)p2p[at+9],(long)p2p[at+10],(long)p2p[at+11]);
        if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    }
    for(unsigned i=0;i<7;++i)if(p2p[25+i*12+2]) {
        unsigned at=109+i*8;uint64_t id=(uint32_t)p2p[25+i*12]|((uint64_t)(uint32_t)p2p[26+i*12]<<32);
        n=swprintf(text+used,cap-used,L"Delivery peer %llu (%ls) last/peak ms: Steam receipt gap %ld/%ld; processing gap %ld/%ld; receive queue residence %ld/%ld; messages %ld; full receive batches %ld. Receipt timestamps include Steam scheduling, not pure wire jitter.\r\n",(unsigned long long)id,historical[i]?L"ended; historical":L"current",(long)p2p[at],(long)p2p[at+1],(long)p2p[at+2],(long)p2p[at+3],(long)p2p[at+4],(long)p2p[at+5],(long)p2p[at+6],(long)p2p[at+7]);
        if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    }
    n=swprintf(text+used,cap-used,L"Peer query: 1 success, 0 not sampled, -1 unavailable; other values are Steam results. -1 timing means unavailable. Peaks reset on connection replacement; failed samples leave byte counters historical. Queue estimate excludes Nagle delay; RTT is not animation delay.\r\n");
    if(n<=0 || (size_t)n>=cap-used)return -1;
    used+=n;LONG gameplay[99]={0};
    if(!remote_read(process,base+JC_STEAM_GAME_RVA,gameplay,sizeof(gameplay)))return 1;
    n=swprintf(text+used,cap-used,L"Group travel busy %ld; destination/current level %ld/%ld; epoch %ld; queue stage %ld; local loaded %ld; loaded mask %lx; members %ld; transition packets held/discarded %ld. Queue: 0 pending, 1 native load queued, 2 already at destination. Completion requires initialized native travelers.\r\n",(long)gameplay[90],(long)gameplay[91],(long)gameplay[92],(long)gameplay[93],(long)gameplay[94],(long)gameplay[95],(long)gameplay[96],(long)gameplay[97],(long)gameplay[98]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,L"Gameplay worker %ld (1 ready, -1 unavailable); enabled %ld; services %ld; busy/ineligible skips %ld/%ld; service last/peak %ld/%ld ms; service gap %ld/%ld ms; callback lock wait %ld/%ld ms. Target wake 8 ms. Motion blend %ld ms (0 native baseline).\r\n",(long)gameplay[62],(long)gameplay[73],(long)gameplay[63],(long)gameplay[64],(long)gameplay[65],(long)gameplay[66],(long)gameplay[67],(long)gameplay[68],(long)gameplay[69],(long)gameplay[70],(long)gameplay[71],(long)gameplay[72]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,L"Diagnostic scans %ld; observation logs %ld (1 enabled); scan last/peak %ld/%ld ms; scans %ld; suppressed observation lines %ld. Safety checks and fault reporting remain enabled.\r\n",(long)gameplay[74],(long)gameplay[75],(long)gameplay[76],(long)gameplay[77],(long)gameplay[78],(long)gameplay[79]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,L"Playback handoffs %ld; post-update fallback frames %ld. State/chirps apply before native input when that hook runs; capture remains after update. Lobby identities persist across level separation; same-level fresh state required; no group load barrier. Vanilla connection-memory timing is unverified.\r\n",(long)gameplay[55],(long)gameplay[56]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,
        L"Steam gameplay: state %ld; state/chirp messages sent/received/rejected %ld/%ld/%ld; last send failure %ld; capture result %ld; captures %ld.\r\n"
        L"Native receiver state %ld; extra traveler trial %ld; ownership guard %ld. Test visibility, motion and chirps in Desert.\r\n"
        L"Gameplay timing last/peak ms: send spacing %ld/%ld; capture age at send %ld/%ld; received state age at game update %ld/%ld. Ages exclude unknown network transit; these are not ping measurements.\r\n"
        L"Chirp queue overflow/expired: %ld/%ld.\r\n",
        (long)gameplay[0],(long)gameplay[1],(long)gameplay[2],(long)gameplay[3],(long)gameplay[4],(long)gameplay[5],(long)gameplay[6],(long)gameplay[7],(long)gameplay[8],(long)gameplay[9],(long)gameplay[10],(long)gameplay[11],(long)gameplay[12],(long)gameplay[13],(long)gameplay[14],(long)gameplay[15],(long)gameplay[16],(long)gameplay[17]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,
        L"Gameplay rejection counts: envelope %ld; admission %ld; payload %ld; identity/lease %ld; old state %ld; old chirp %ld. Unchanged guest sends avoided: %ld.\r\n",
        (long)gameplay[18],(long)gameplay[19],(long)gameplay[20],(long)gameplay[21],(long)gameplay[22],(long)gameplay[23],(long)gameplay[40]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,L"Game bridge update gap last/peak %ld/%ld ms; interval counts <50/<100/<250/>=250 ms: %ld/%ld/%ld/%ld. New state processing-to-input last/peak ms extra %ld/%ld, native %ld/%ld. Unchanged host sends avoided %ld (100 ms refresh retained).\r\n",(long)gameplay[49],(long)gameplay[50],(long)gameplay[51],(long)gameplay[52],(long)gameplay[53],(long)gameplay[54],(long)gameplay[57],(long)gameplay[58],(long)gameplay[59],(long)gameplay[60],(long)gameplay[61]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    for(unsigned slot=0;slot<2;++slot) {
        unsigned row=24+slot*8;unsigned long long id=(uint32_t)gameplay[row]|((unsigned long long)(uint32_t)gameplay[row+1]<<32);
        n=swprintf(text+used,cap-used,
            L"%ls traveler SteamID %llu: active %ld; distinct states %ld; state age last/peak %ld/%ld ms; new-state gap last/peak %ld/%ld ms.\r\n",
            slot?L"Native":L"Extra",id,(long)gameplay[row+2],(long)gameplay[row+3],(long)gameplay[row+4],(long)gameplay[row+5],(long)gameplay[row+6],(long)gameplay[row+7]);
        if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    }
    for(unsigned slot=0;slot<2;++slot) {
        unsigned at=80+slot*5;
        n=swprintf(text+used,cap-used,L"%ls traveler incoming position steps last/peak %ld/%ld milli-units; changes %ld; steps above 1 unit %ld; blend bypasses %ld. Sender target changes, not measured visible corrections.\r\n",slot?L"Native":L"Extra",(long)gameplay[at],(long)gameplay[at+1],(long)gameplay[at+2],(long)gameplay[at+3],(long)gameplay[at+4]);
        if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    }
    n=swprintf(text+used,cap-used,
        L"Active update sections last/peak ms: frame wait %ld/%ld; original update %ld/%ld; native finish %ld/%ld; state bridge/capture %ld/%ld. Original update includes its nested hooks and Steam callbacks; section peaks need not share a frame.\r\n",
        (long)gameplay[41],(long)gameplay[42],(long)gameplay[43],(long)gameplay[44],(long)gameplay[45],(long)gameplay[46],(long)gameplay[47],(long)gameplay[48]);
    if(n<=0 || (size_t)n>=cap-used)return -1;used+=n;
    n=swprintf(text+used,cap-used,L"Per-traveler timing is sampled at game updates, includes local frame stalls, and is retained after departure; it is not network latency.\r\n");
    return n>0 && (size_t)n<cap-used?1:-1;
}

struct exit_watch {
    HANDLE process,thread,mapping;
    struct jnet_session_logs *logs;
    uintptr_t base;
    DWORD pid;
    FILETIME created;
    wchar_t last_report[20000];
};
static struct exit_watch exit_watch;
static DWORD WINAPI watch_exit(void *opaque) {
    struct exit_watch *watch=opaque;
    LONG samples[8][JC_FLIGHT_FIELDS]={0}; unsigned captured=0,next=0;
    wchar_t last_report[20000]={0},report[20000];
    DWORD waited;
    while ((waited=WaitForSingleObject(watch->process,100))==WAIT_TIMEOUT) {
        LONG sample[JC_FLIGHT_FIELDS];
        if (remote_read(watch->process,watch->base+JC_FLIGHT_RVA,sample,sizeof(sample))) {
            memcpy(samples[next],sample,sizeof(sample)); next=(next+1)%8;
            if (captured<8) ++captured;
        }
        int reported=traveler_report(watch->process,watch->base,report,20000)==1 ||
            native_playback_report(watch->process,watch->base,report,20000)==1;
        if (append_menu_report(watch->process,watch->base,report,20000,reported)==1)
            wcscpy(last_report,report);
    }
    DWORD code=0,error=0;
    int obtained=waited==WAIT_OBJECT_0 && GetExitCodeProcess(watch->process,&code);
    if (!obtained) error=GetLastError();
    char line[256];
    int n=snprintf(line,sizeof(line),"[JC-exit] pid=%lu wait=%lu obtained=%d code=%08lx error=%lu samples=%u\r\n",
        (unsigned long)watch->pid,(unsigned long)waited,obtained,(unsigned long)code,(unsigned long)error,captured);
    jnet_log_append(&exit_log,line,(size_t)n);
    for (unsigned i=0;i<captured;++i) {
        LONG *v=samples[(next+8-captured+i)%8];
        n=snprintf(line,sizeof(line),"[JC-exit] phase=%ld body=%ld frame=%ld thread=%ld serial=%ld fault=%08lx module=%ld rva=%08lx observer=%ld\r\n",
            (long)v[0],(long)v[1],(long)v[2],(long)v[3],(long)v[4],(unsigned long)(uint32_t)v[5],
            (long)v[6],(unsigned long)(uint32_t)v[7],(long)v[8]);
        jnet_log_append(&exit_log,line,(size_t)n);
    }
    wcscpy(watch->last_report,last_report);
    CloseHandle(watch->process);
    return 0;
}
static int watch_exit_start(HANDLE process,uintptr_t base,DWORD pid,const wchar_t *directory) {
    (void)directory;
    if (exit_watch.thread) {
        DWORD state=WaitForSingleObject(exit_watch.thread,0);
        if (state==WAIT_TIMEOUT) return exit_watch.pid==pid;
        if (state!=WAIT_OBJECT_0) return 0;
        CloseHandle(exit_watch.thread);
        if(exit_watch.logs)UnmapViewOfFile(exit_watch.logs);
        if(exit_watch.mapping)CloseHandle(exit_watch.mapping);
        memset(&exit_watch,0,sizeof(exit_watch));
    }
    HANDLE copy=NULL;
    if (!DuplicateHandle(GetCurrentProcess(),process,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS)) return 0;
    memset(&exit_log,0,sizeof(exit_log));memset(&hang_log,0,sizeof(hang_log));
    exit_watch=(struct exit_watch){.process=copy,.base=base,.pid=pid};
    exit_watch.logs=jnet_session_open(process,pid,&exit_watch.mapping);
    FILETIME ended,kernel,user;
    GetProcessTimes(process,&exit_watch.created,&ended,&kernel,&user);
    exit_watch.thread=CreateThread(NULL,0,watch_exit,&exit_watch,0,NULL);
    if (!exit_watch.thread) {
        CloseHandle(copy);
        if(exit_watch.logs)UnmapViewOfFile(exit_watch.logs);
        if(exit_watch.mapping)CloseHandle(exit_watch.mapping);
        memset(&exit_watch,0,sizeof(exit_watch));return 0;
    }
    return 1;
}

static int body_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG dual[8],body[48],flight[JC_FLIGHT_FIELDS],motion[28];
    if (!remote_read(process,base+JC_FLIGHT_RVA,flight,sizeof(flight)) ||
        !remote_read(process,base+JC_DUAL_RVA,dual,sizeof(dual)) ||
        !remote_read(process,base+JC_BODIES_RVA,body,sizeof(body)) ||
        !remote_read(process,base+JC_MOTION_RVA,motion,sizeof(motion))) return -1;
    int n=swprintf(text,cap,L"DUAL BODY: ready %ld (1 both checked; -1 failed). Preparation stage %ld (1 first; 2 second; 3 pair checked).\r\n"
        L"Keep JNET open: automatic process-exit capture is active. Fault observer: %ld (1 active). Last socket error: %ld.\r\n"
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
static int traveler_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG actor[32],trial[5],pending,features[40],peer[14],network[24];
    if(!remote_read(process,base+JC_NETWORK_RVA,network,sizeof(network)))return -1;
    if (!remote_read(process,base+JC_ACTOR_TRIAL_RVA,trial,sizeof(trial)) ||
        !remote_read(process,base+JC_ACTOR_RVA,actor,sizeof(actor)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (actor[0] || trial[0] || pending==22 || pending==23 || pending==24 || pending==25 || pending==26 || actor[16] || network[0] || pending==27) {
        if (!remote_read(process,base+JC_PEER_LIFECYCLE_RVA,peer,sizeof(peer))) return -1;
        if (!remote_read(process,base+JC_COMPANION_FEATURES_RVA,features,sizeof(features))) return -1;
        if (!cap) return 1;
        wchar_t outfit[160]=L"",coexist[2048]=L"";
        if(actor[16])swprintf(coexist,2048,L"Local 3: state/result %ld / %ld. Native samples %ld; overlap updates %ld. Native outfit/symbol %ld / %ld; present %ld; distinct symbols %ld.\r\nNative clean cycles: %ld/%ld. Local 3 overall: %ls. Each slot cycles independently. States: 1 queued; 2 feeding; 3 native departure; 4 native dust complete; 5 declined/stopped on error; 6 chapter interrupted. UDP state counters; accepted/rejected/source-dropped: %ld/%ld/%ld and %ld/%ld/%ld. Authenticated state/chirp UDP; no Internet transport.\r\n",(long)actor[16],(long)actor[17],(long)actor[18],(long)actor[22],(long)actor[19],(long)actor[20],(long)actor[21],(long)actor[23],(long)actor[24],(long)actor[25],actor[16]==4 && actor[24]==7 && peer[8]==2?L"complete":actor[16]>=5 || peer[8]<0?L"interrupted":L"running",(long)actor[26],(long)actor[27],(long)actor[28],(long)actor[29],(long)actor[30],(long)actor[31]);
        if(network[0])swprintf(coexist,2048,L"SEPARATE SESSIONS: state %ld, identity %ld (1 the first test client; 2 the second test client; 3 the third test client), result %ld.\r\nSent/received/rejected: %ld/%ld/%ld. Capture result: %ld.\r\nExtra actor peer %ld: authenticated %ld, state age %ld ms. Native companion peer %ld: authenticated %ld, state age %ld ms.\r\nChirps delivered to inbox: %ld; capture result %ld. State: 1 connected; 2 paused by Dust; 3 chapter interrupted; -1 error. Fresh state requires age below 2000 ms. Connect resumes after Dust.\r\nTransport callback last/peak/mean: %ld/%ld/%ld us. State send interval target: %ld ms (frame-limited). Receive budget hits: %ld; replay/duplicate rejects: %ld; expired waiting chirps: %ld.\r\nNative receiver state/result: %ld/%ld; samples %ld; completed departures %ld. States: 0 idle; 1 waiting; 2 active; 3 departing; 4 reusable; 5 error; 6 chapter interrupted.\r\nFirst ownership guard: %ld (0 none; 1 context; 2 matchmaking pointer read; 3 missing matchmaking object; 4 connection flag read; 5 vanilla connection). Errors 123/124 require restarting Journey; Connect preserves the error.\r\n",
            (long)network[0],(long)network[1],(long)network[2],(long)network[3],(long)network[4],(long)network[5],(long)network[6],
            (long)network[8],(long)network[9],(long)network[10],(long)network[11],(long)network[12],(long)network[13],(long)network[14],(long)network[15],(long)network[16],(long)network[17],(long)network[18],(long)network[19],(long)network[20],(long)network[21],(long)network[22],(long)actor[16],(long)actor[17],(long)actor[18],(long)actor[24],(long)network[23]);
        if(peer[12] && peer[13]>=0 && peer[13]<=6) {
            unsigned index=(unsigned)peer[13];
            swprintf(outfit,160,L"Outfits 7: %ls tier %u (outfit %u/7). Scarf state comes from the selected diagnostic source.\r\n",
                index<4?L"red":L"white",index<4?index:index-3,index+1);
        }
        const wchar_t *procedure=network[0] ?
            L"Connect streams real players until Dust, silence or a chapter change. There is no sixty-second cutoff. After a reconnect, test the first short and charged chirps once the traveler has arrived. Copy All while connected to record transport timing.\r\n" :
            L"Active Traveler runs for sixty seconds; Repeat 10 runs ten cycles with fifteen seconds of activity each. "
            L"Outfits 7 cycles red tiers 0-3 then white tiers 1-3. Other trials follow the diagnostic outfit; each symbol differs from both native slots. Online participants must supply their own identity.\r\n"
            L"Walk, turn, glide, and exchange short/charged chirps. Check robe, scarf, chest/chirp symbols and responsiveness. "
            L"Dust starts meditation and dust, and cancels any remaining repeat cycles. Repeat 10 waits for phase 0 (cleanup complete) before each new admission. For Local 3, wait for Local 3 overall: complete and BOTH counters at 7/7. Other repeats use Repeat test: complete. Then Copy All.\r\n";
        int n=swprintf(text,cap,L"ACTIVE TRAVELER - JNET " JC_HELPER_VERSION L"\r\n"
            L"Legacy traveler diagnostic counters; see the Steam lobby/P2P report below.\r\n%ls\r\n"
            L"Trial state/result: %ld / %ld. Elapsed: %ld ms.\r\n"
            L"Trial: 1 active; 2 departing; 3 reusable; -1 declined.\r\n"
            L"Peer phase/result: %ld / %ld. Generation: %ld. Joins/leaves/abrupt: %ld / %ld / %ld. Dissolve: %ld/1000. Restart queued: %ld.\r\n"
            L"Phases: 0 vacant; 1 active; 2 ReadyToMed; 3 Med; 4 hold; 5 dust; 6 fading; 7 retiring; 8 quarantined.\r\n"
            L"Repeat test: %ls. Clean cycles: %ld/%ld. Result: %ld.\r\n"
            L"%lsActor state/result: %ld / %ld. Pending command: %ld.\r\n"
            L"States: 0 unbound; 1 waiting; 2 active; 3 draining; 4 retired; 5 quarantined.\r\n"
            L"Native input/update/render/finish frame IDs: %ld / %ld / %ld / %ld.\r\n"
            L"Material guard/result rows: %ld / %ld / %ld.\r\n"
            L"First actor scope failure phase/step/code/frame: %ld / %ld / %ld / %ld.\r\n"
            L"Scope steps: 1 backend; 2 cloth enable; 3 routing; 4 world capture; 5 appearance; 6 world restore; 7 materials; 8 backend end.\r\n"
            L"Input sequence: %ld. Changed spans: %ld. Publication quarantine: %ld.\r\n"
            L"Reader/check: %ld / %ld (1 protected local; 2 compatibility).\r\n"
            L"Active game updates: %ld. Last/peak update cost: %ld / %ld ms.\r\n"
            L"Cloth prepared/registered: %ld / %ld. Declines/skipped workers: %ld / %ld.\r\n"
            L"Cloth last result/stage/detail: %ld / %ld / %ld.\r\n"
            L"Finish stages: 100 ownership; 101 workers; 102 counters; 103 native finish; 104 unmap; 105 render registration.\r\n"
            L"If quarantined, Copy All then close Journey: this force-exits only Journey (E04A4301).\r\n\r\n"
            L"%ls"
            L"Saved visibility log lines distinguish registration, dispatch, command building and forwarding. Native counts do not prove visible correctness, three-player interactions or Internet transport. Status is sampled, not atomic.",
            coexist,(long)trial[0],(long)trial[1],(long)trial[2],
            (long)peer[0],(long)peer[1],(long)peer[2],(long)peer[3],(long)peer[4],(long)peer[5],(long)peer[6],(long)peer[7],
            peer[8]==1?L"running":peer[8]==2?L"complete":peer[8]==3?L"cancelled":peer[8]<0?L"failed":L"idle",
            (long)peer[9],(long)peer[10],(long)peer[11],outfit,(long)actor[0],(long)actor[1],(long)pending,(long)actor[2],(long)actor[3],(long)actor[4],(long)actor[8],
            (long)actor[9],(long)actor[10],(long)actor[11],
            (long)actor[12],(long)actor[13],(long)actor[14],(long)actor[15],
            (long)actor[5],(long)actor[6],(long)actor[7],
            (long)features[8],(long)features[9],(long)features[39],(long)features[37],(long)features[38],
            (long)features[35],(long)features[36],(long)features[30],(long)features[31],
            (long)features[32],(long)features[33],(long)features[34],procedure);
        if(n<0 || (size_t)n>=cap) text[cap-1]=0;
        return 1;
    }
    LONG r[44];
    if (!remote_read(process,base+JC_TRAVELER_RVA,r,sizeof(r)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!r[0] && !r[19] && pending!=21) return 0;
    if (!cap) return 1;
    const wchar_t *state=r[19] ? L"Waiting for normal Desert initialization; enter Desert" : pending==21 ? L"Queued; return to Journey" : r[0]==1 ? L"Armed; chapter select, then Desert portal" :
        r[0]==2 ? L"Native operation in progress" : r[0]==3 ? L"Full object retained; select Active Traveler" :
        r[0]==4 ? L"Native cleanup and deletion completed" : r[0]==-2 ? L"Quarantined; restart Journey" : L"Declined; restart Journey";
    int n=swprintf(text,cap,
        L"TRAVELER LIFECYCLE - JNET " JC_HELPER_VERSION L"\r\n%ls.\r\n\r\n"
        L"State / stage / result: %ld / %ld / %ld. Pending: %ld.\r\n"
        L"Context detail / guard detail: %ld / %ld. Initial geometry slots/occupied/free: %ld / %ld / %ld.\r\n"
        L"Presence snapshot: layout %ld; flags a2/a3/a4 %ld/%ld/%ld; blocked %ld; chapters %ld/%ld; native present %ld; transport %ld (-1 unavailable).\r\n"
        L"Native preparation calls: %ld. Controller points to own mesh before/after: %ld / %ld (-1 unread).\r\n"
        L"Full initialization / cleanup / deletion calls: %ld / %ld / %ld. Calls alone do not prove successful ownership.\r\n"
        L"Glyph pool count before/after: %ld / %ld. Distinct sound handles: %ld.\r\n"
        L"Separate clone / pose / animation storage: %ld / %ld / %ld. Callback check / cleanup restoration: %ld / %ld.\r\n\r\n"
        L"Callback scan/result %ld / %ld; originals before %ld/%ld, after %ld/%ld; extra %ld; shared resource %ld. Detail: 1 scan; 2 originals changed; 3 source; 4 subscriber mismatch.\r\n"
        L"Geometry cleanup: occupied before/after %ld/%ld; code %ld; phase %ld (1 capture, 2 release check).\r\n"
        L"This creates one dormant, fully initialized extra Dude using Journey's complete initializer. It does not draw a third player or run its simulation. No outfit is copied from the host.\r\n"
        L"Stages: 1 arm; 2 context/capacity; 3 factory; 4 full initializer; 5 ownership; 6 cleanup; 7 restoration; 8 deleted; 9 native preparation (between factory and full initializer).\r\n"
        L"Results: 1 read; 2 type/controller; 3 changed; 4 bounds; 5 glyph; 6 table overlap; 7 sound alias; 8 context/capacity; 9 timeout; 10 factory; 11 mesh callback; 12 clone; 13 pose; 14 animation; 15 callbacks; 16 cleanup fields; 17 geometry occupancy; 18 checkpoint log; 19 preparation/ownership.\r\n\r\n"
        L"Glyph guard detail: 80 hook/state; 81 bounds; 82 read; 83 occupied; 84 anchor; 85 capacity; 86 changed pool; 87 slot; 88 storage; 89 alias; 90 thread; 91 assignment.\r\n"
        L"Context: 23 native companion present; 24 transport connected; 30 layout unreadable/changed (layout: 1 read, 2 type, 3 changed, 4 bounds); 31 transport unreadable.\r\n"
        L"Keep this window open. Select Active Traveler for the new local test. After it retires, chapter select checks cleanup. Quit normally, then Copy All once.\r\n"
        L"Chapter changes test resource setup/teardown only. Online departure and same-level replacement must reuse a resident slot; they must not require chapter select. Full cloth routing, player updates and peer admission are not enabled here. Status snapshots are not atomic.",
        state,(long)r[0],(long)r[1],(long)r[2],(long)pending,
        (long)r[15],(long)r[16],(long)r[17],(long)r[3],(long)r[18],
        (long)r[28],(long)r[20],(long)r[21],(long)r[22],(long)r[23],(long)r[24],(long)r[25],(long)r[26],(long)r[27],
        (long)r[29],(long)r[30],(long)r[31],(long)r[4],(long)r[5],(long)r[6],
        (long)r[7],(long)r[8],(long)r[9],(long)r[10],(long)r[11],(long)r[12],(long)r[13],(long)r[14],
        (long)r[32],(long)r[39],(long)r[33],(long)r[34],(long)r[35],(long)r[36],(long)r[37],(long)r[38],(long)r[40],(long)r[41],(long)r[42],(long)r[43]);
    if (n<0 || (size_t)n>=cap) text[cap-1]=0;
    return 1;
}
static int native_playback_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG playback[48],bodies[48],pending,features[40],room[20],outputs[16];
    if (!remote_read(process,base+JC_NATIVE_PLAYBACK_RVA,playback,sizeof(playback)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!playback[0] && !playback[24] && pending!=20) return 0;
    if (!remote_read(process,base+JC_BODIES_RVA,bodies,sizeof(bodies)) ||
        !remote_read(process,base+JC_COMPANION_FEATURES_RVA,features,sizeof(features)) ||
        !remote_read(process,base+JC_ROOM_RVA,room,sizeof(room)) ||
        !remote_read(process,base+JC_NATIVE_OUTPUTS_RVA,outputs,sizeof(outputs))) return -1;
    int n=swprintf(text,cap,L"NATIVE COMPANIONS - JNET " JC_HELPER_VERSION L"\r\n%ls.\r\n\r\n"
        L"Two retained bodies, independent native clip evaluation and skin updates. No evaluated-bone copying. Target: up to 10 evaluations/sec/body; inspections costing 8 ms or more yield three times their cost to normal game updates; companion sampling slows accordingly. Inexpensive position following alternates bodies once per game frame during inspection rest.\r\n",
        pending==20 ? L"Queued; return to gameplay" : playback[23] ? L"Room input; three-minute window" : L"Local input; sixty-second window");
    if (n<=0 || (size_t)n>=cap) return -1;
    size_t used=(size_t)n;
    for (unsigned i=0;i<2;++i) {
        const LONG *p=playback+i*24,*b=bodies+i*24;
        const wchar_t *state=p[0]==1 && p[1]==17 ? L"active; allocator retry pending" : p[0]==1 ? L"active/waiting" : p[0]==2 ? L"stopped" : p[0]==3 ? L"complete" :
            p[0]<0 && !p[16] ? L"declined before native submission" : p[0]<0 ? L"stopped after native submission" : L"not selected";
        n=swprintf(text+used,cap-used,L"\r\nBody %u: %ls. Render %ld; draws %ld; render failure %ld.\r\n"
            L"Elapsed %ld ms; accepted %ld; changed-bone samples %ld; accepted/sec %.2f; submissions %ld.\r\n"
            L"Result / stage / detail: %ld / %ld / %ld. Guard ready %ld; postcheck %ld.\r\n"
            L"Evaluation ms last/peak %ld / %ld; reads %ld; active clips %ld. Graph nodes/bindings/clocks %ld / %ld / %ld; scheduler %ld; scratch/temp %ld / %ld bytes.\r\n"
            L"Bind row %ld; stamp rearms %ld; bind code %ld.\r\n"
            L"Position-only updates %ld; last/peak %ld / %ld ms; reads %ld; result %ld.\r\n",
            i+1,state,(long)b[1],(long)b[2],(long)b[3],(long)p[6],(long)p[4],(long)p[5],
            p[6]>0 ? (double)p[4]*1000.0/(double)p[6] : 0.0,(long)p[16],
            (long)p[1],(long)p[2],(long)p[3],(long)p[19],(long)p[18],
            (long)p[7],(long)p[8],(long)p[9],(long)p[17],(long)p[10],(long)p[11],(long)p[12],
            (long)p[13],(long)p[14],(long)p[15],(long)p[20],(long)p[21],(long)p[22],
            (long)features[14+i],(long)features[22+i],(long)features[24+i],(long)features[28+i],(long)features[26+i]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
        if (p[23]) {
            unsigned at=30+i*5;
            n=swprintf(text+used,cap-used,L"Sender appearance: %ls; outfit %ld (red0-3, white4-6); scarf %ld/%ld; symbol %ld. Metadata only; room cloth is disabled.\r\n",
                features[at] ? L"captured" : L"unavailable",(long)features[at+3],
                (long)features[at+1],(long)features[at+2],(long)features[at+4]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
        if (p[2]==2 && p[3]>=1 && p[3]<=7) {
            const wchar_t *checks[]={L"scheduler identity/queue",L"scratch capacity",L"work-unit capacity",L"allocator lock",L"ring layout",L"ring reservation",L"task pool"};
            n=swprintf(text+used,cap-used,L"Scheduler guard: %ls.\r\n",checks[(unsigned)p[3]-1]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
        if (p[2]==9 && p[3]>=4 && p[3]<24) {
            const wchar_t *checks[]={L"counter pointer",L"four-byte alignment",L"counter read",L"live reference count",L"counter storage"};
            const wchar_t *owners[]={L"Local",L"vanilla Remote",L"body 1",L"body 2"};
            n=swprintf(text+used,cap-used,L"Reference guard: %ls; owner %ls.\r\n",
                checks[(unsigned)p[3]/4-1],owners[(unsigned)p[3]%4]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
        if (p[2]==4 && p[3]>=20*1024 && p[3]<29*1024) {
            const wchar_t *checks[]={L"mapping index",L"binding index",L"unsupported output type",
                L"unsupported binding kind",L"destination outside owned skeleton/copy sources",L"destination alignment",
                L"destination not writable",L"no bound outputs",L"model copy table"};
            unsigned check=(unsigned)p[3]/1024,row=(unsigned)p[3]%1024;
            n=swprintf(text+used,cap-used,L"Output guard: %ls; %ls row %u (zero-based).\r\n",
                checks[check-20],check==28 ? L"model owner" : check<23 ? L"mapping" : L"binding",row);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
            const LONG *o=outputs+i*8;
            n=swprintf(text+used,cap-used,L"Output context: items/bindings %lu/%lu; index/binding %lu/%lu; kind %lu; type RVA 0x%08lX; copy bytes %lu; destination owners %lu.\r\n",
                (unsigned long)(uint32_t)o[0],(unsigned long)(uint32_t)o[1],
                (unsigned long)(uint32_t)o[2],(unsigned long)(uint32_t)o[3],
                (unsigned long)(uint32_t)o[4],(unsigned long)(uint32_t)o[5],
                (unsigned long)(uint32_t)o[6],(unsigned long)(uint32_t)o[7]);
            if (n<=0 || (size_t)n>=cap-used) return -1;
            used+=(size_t)n;
        }
    }
    n=swprintf(text+used,cap-used,L"\r\nCloth state/result/stage: %ld / %ld / %ld. Updates %ld; retirements %ld.\r\n"
        L"Cloth capture detail %ld. Reader %ld (2 batched compatibility; 1 protected local; 0 compatibility); isolated reader check %ld (-1 not run; 0 passed; 1-6 failed; 100 launch; 101 timeout; 102 wait; 103 abnormal exit).\r\n"
        L"Cloth original failure stage %ld; call detail %ld; storage code %ld. Inspection rest interval %ld ms.\r\n"
        L"Chirp state/result: %ld / %ld. Captured %ld; body 1 / body 2 queued %ld / %ld; skipped %ld.\r\n"
        L"Room peers %ld; pending command %ld.\r\n\r\n"
        L"Cloth: state 0 off, 1 waiting, 2 live, 3 retired, -1 declined, -2 quarantined. Codes 1 read, 2 identity, 3 changed, 4 busy, 5 native peer/occupied role, 6 capacity, 7 ownership, 8 call, 9 phase, 10 properties, 11 context, 12 definition, 13 finish, 14 body. Stages 1 init, 2 attach, 3 collisions, 4 appearance, 5 release, 6 pool scan, 7 materials, 8 empty handles, 9 properties, 10 atlas resources, 11 scarf appearance. Atlas call detail: 30/31 context; 40-45 resource preflight; 100-156 binding check; 200-256 postcheck. For the latter two, subtract 100/200: tens identify robe base/glow/mask then scarf base/glow/mask; units 1 read, 2 binding, 3 payload, 4 texture, 5 scarf sampler, 6 changed, 0 writable/postcheck.\r\n"
        L"Chirps: state 0 off, 1 listening, 2 queued, 3 stopped, -1 disabled. Codes 1 read, 2 argument, 3 changed, 4 bounds, 5 full, 6 sound, 7 writable, 8 postcheck, 9 native peer.\r\n\r\n"
        L"Result codes: 0 accepted; 1 read; 2 type; 3 changed; 4 bounds; 5 capacity; 6 overlap; 7 pending; 8 context; 9 budget; 10 argument; 11 scheduler; 12 busy; 13 space; 14 clock; 15 graph; 16 writable; 17 allocator retry; 20 lifetime; 21 pose; 22 animation input; 23 binding/queue; 24 submission; 25 postcheck; 26 allocation.\r\n"
        L"Stages: 1 layout; 2 scheduler; 3 clocks; 4 outputs; 5 execution; 6 nodes; 7 disjoint/writable; 8 fresh recheck; 9 reference counters; 100 lifetime; 101 capture; 102 apply; 103 bind; 104 submit; 105 finish; 106 skin; 107 shader; 108 postcheck; 109 position-only update.\r\n"
        L"Bind details: 1 argument; 2 clone; 3 overlap; 4 model/control; 5 frame; 6 fresh check; 7 stamp write; 8 stamp postcheck; 9 native stamp; 10 sample; 11 row read; 12/13 request flags; 14 weight; 15 additive; 16 time; 17 duration; 18 speed; 101 count; 102 queue pair; 103 ref read; 104 ref count.\r\n\r\n"
        L"Stay in Desert. Walk, turn, jump/glide and chirp. Stop Link stops the test. Copy All, then quit Journey normally; restart before another mode. Counts do not prove visible animation. Status snapshots are not atomic.\r\n"
        L"Chirp pulses use native environmental effects: no player glyph or scarf charging. Cloth is a single-body remote-role experiment. Shared triggers, chapter changes and Internet transport are unfinished.",
        (long)features[0],(long)features[1],(long)features[2],(long)features[3],(long)features[4],
        (long)features[7],(long)features[8],(long)features[9],(long)features[10],(long)features[11],(long)features[12],(long)features[13],
        (long)features[16],(long)features[17],(long)features[18],(long)features[19],(long)features[20],(long)features[21],
        (long)room[0],(long)pending);
    return n>0 && (size_t)n<cap-used ? 1 : -1;
}
static int skeleton_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG sk[32],bodies[48],motion[28],pending;
    if (!remote_read(process,base+JC_SKELETON_RVA,sk,sizeof(sk)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!sk[0] && pending!=18) return 0;
    if (!remote_read(process,base+JC_BODIES_RVA,bodies,sizeof(bodies)) ||
        !remote_read(process,base+JC_MOTION_RVA,motion,sizeof(motion))) return -1;
    int n=swprintf(text,cap,L"SOLO SKELETON PLAYBACK - JNET " JC_HELPER_VERSION L"\r\n"
        L"State: %ld (1 selected; 2 stopped; 3 timed window complete; -1 preparation failed; -2 playback failed). Pending command: %ld.\r\n"
        L"Measured playback window: %ld ms (excludes the initial three-second delay).\r\n"
        L"Mapping result/stage: %ld / %ld. Named bones local/native remote/body 1/body 2: %ld / %ld / %ld / %ld.\r\n"
        L"Codes: 0 accepted; 1 read; 2 names/type; 3 changed; 4 bounds; 5 capacity; 6 mapping/ownership; 7 write; 8 postcheck; 9 budget.\r\n"
        L"Stages: 1 collect; 2 independence/mapping; 3 recheck; 4 live ownership; 5 stable source; 6 copy; 7 copy postcheck; 8 native-update postcheck; 0 accepted.\r\n\r\n",
        (long)sk[0],(long)pending,(long)sk[7],(long)sk[1],(long)sk[2],(long)sk[3],(long)sk[4],(long)sk[5],(long)sk[6]);
    if (n<=0 || (size_t)n>=cap) return -1;
    size_t used=(size_t)n;
    for (unsigned i=0;i<2;++i) {
        unsigned r=8+i*8,b=i*24;
        n=swprintf(text+used,cap-used,L"Body %u: render %ld (1 waiting; 2 drawing; 3 ended; -1 failed); draws %ld; failure %ld (26 skeleton copy).\r\n"
            L"Copy result/stage: %ld / %ld. Accepted copies: %ld. Copies with changed bones: %ld. Relative spine changes: %ld.\r\n"
            L"Accepted copies/sec averaged over measured window: %.2f.\r\n"
            L"Copy ms last/peak: %ld / %ld. Reads: %ld. Attempted bone writes: %ld.\r\n"
            L"Post-native-update check ms last/peak: %ld / %ld. Reads: %ld.\r\n"
            L"Whole pose update ms last/peak: %ld / %ld. Inclusive check/draw ms last/peak: %ld / %ld.\r\n\r\n",
            i+1,(long)bodies[b+1],(long)bodies[b+2],(long)bodies[b+3],(long)sk[r],(long)sk[r+1],
            (long)sk[r+2],(long)sk[r+3],(long)motion[10+i*7],sk[7]>0 ? (double)sk[r+2]*1000.0/(double)sk[7] : 0.0,
            (long)sk[r+4],(long)sk[r+5],(long)sk[r+6],(long)sk[r+7],
            (long)sk[24+i*4],(long)sk[25+i*4],(long)sk[26+i*4],
            (long)bodies[b+13],(long)bodies[b+14],(long)bodies[b+15],(long)bodies[b+16]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
    }
    n=swprintf(text+used,cap-used,L"Optional playback: the first test client only, walk, turn and jump/glide during the timed window. Two offset bodies target 20 pose updates/sec each; actual rate depends on frame cost. Copy the measured report. Stop Link also stops this test.\r\n"
        L"Uses the local player's native evaluated bones and Journey's mesh renderer. It does not send bone poses over the room network yet. No robe/scarf, chirp effects or departure effects. Stay in Desert; restart Journey for another mode. Keep JNET open for exit capture.");
    return n>0 && (size_t)n<cap-used ? 1 : -1;
}
static int evaluation_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG status[40],again[40],pending,link[3];
    if (!remote_read(process,base+JC_EVALUATION_RVA,status,sizeof(status)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending)) ||
        !remote_read(process,base+JC_LINK_RVA,link,sizeof(link))) return -1;
    if ((!status[0] && pending!=19) || link[0] || link[2]) return 0;
    if (!remote_read(process,base+JC_EVALUATION_RVA,again,sizeof(again))) return -1;
    if (pending==19 || status[0]==2 || memcmp(status,again,sizeof(status))) {
        int n=swprintf(text,cap,L"Evaluation Check is inspecting retained storage. Return to gameplay, then Status and Copy all. No native evaluator call is enabled.");
        return n>0 && (size_t)n<cap ? 1 : -1;
    }
    int n=swprintf(text,cap,L"NATIVE EVALUATION STORAGE - JNET " JC_HELPER_VERSION L"\r\n"
        L"State: %ld (0 waiting; 1 observed; -1 declined). Pending command: %ld.\r\n"
        L"Result/stage: %ld / %ld. Observed storage disjoint: %ld (-1 incomplete; 0 overlap; 1 observed).\r\n"
        L"Inspection ms: %ld. Reads: %ld.\r\n"
        L"Owner: %ld (0 local; 1 native remote; 2 body 1; 3 body 2; 4 common). States inspected: %ld.\r\n"
        L"Stages: 1 skeleton metadata; 2 jobs and outputs; 3 overlap check; 4 fresh recheck; 0 complete/not entered.\r\n"
        L"Codes: 0 observed; 1 read; 2 type; 3 changed; 4 bounds; 5 capacity/allocation; 6 overlap; 7 pending native job; 8 preparation/context; 9 budget.\r\n\r\n",
        (long)status[0],(long)pending,(long)status[1],(long)status[2],(long)status[3],(long)status[4],(long)status[5],
        (long)status[6],(long)status[7]);
    if (n<=0 || (size_t)n>=cap) return -1;
    size_t used=(size_t)n;
    const wchar_t *labels[]={L"Local",L"Native remote",L"Body 1",L"Body 2"};
    for (unsigned i=0;i<4;++i) {
        const LONG *row=status+8+i*8;
        n=swprintf(text+used,cap-used,L"%ls: records %ld; bindings %ld; slots %ld; scratch bytes %ld; execution bytes %ld; output pairs %ld; shape mutations %ld; pending %ld.\r\n",
            labels[i],(long)row[0],(long)row[1],(long)row[2],(long)row[3],(long)row[4],(long)row[5],(long)row[6],(long)row[7]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
    }
    n=swprintf(text+used,cap-used,L"\r\nCounts can be partial on a decline. Copy this report, then close Journey. No movement, room or second client is needed.\r\n"
        L"This checks selected native animation allocations and matrix destinations. It does not validate all graph nodes, scheduler thread/phase, or complete traveler ownership. Native evaluation stays disabled. No game-owned memory is written by this check; no robe/scarf, chirp or Internet feature is enabled.");
    return n>0 && (size_t)n<cap-used ? 1 : -1;
}
static int link_report_body(HANDLE process,uintptr_t base,wchar_t *text,size_t cap);
static int link_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    int result=link_report_body(process,base,text,cap);
    return append_menu_report(process,base,text,cap,result);
}
static int link_report_body(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    int traveler=traveler_report(process,base,text,cap); if (traveler) return traveler;
    int native=native_playback_report(process,base,text,cap); if (native) return native;
    int mirror=skeleton_report(process,base,text,cap); if (mirror) return mirror;
    int evaluation=evaluation_report(process,base,text,cap); if (evaluation) return evaluation;
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
            L"LOCAL POSE LINK - SOURCE - JNET " JC_HELPER_VERSION L"\r\n"
            L"State: %ld (1 publishing; 3 closed; -1 failed). Error: %ld.\r\n"
            L"Game update: %ld. Pending command: %ld. Session seconds remaining: %ld.\r\n\r\n"
            L"Packets sent: %ld. Last capture/send code: %ld. Last published chapter: %ls.\r\n"
            L"Code 0 means a packet was sent; 7 means the local pose/chapter could not be validated.\r\n"
            L"Link errors: 1 mode/configuration; 2 socket startup; 3 cryptography; 4 socket setup; 5 socket I/O; 6 encoding/sequence; 7 game/thread changed.\r\n\r\n"
            L"Move this character in Desert, then return promptly to the first test client to look for the extra body.\r\n"
            L"This source does not allocate or render a trial. Its send count is not an acknowledgement: check the first test client's received packets, applied poses and render status.\r\n"
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
        L"LOCAL POSE LINK - JNET " JC_HELPER_VERSION L"\r\n"
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
        room_mode ? L"ALL THREE clients need Capacity Test and Prepare Trial while solo. First test client: Host Room / Copy Invite. Third test client then the second test client: Join Room with the SAME invite. Both remote ghosts should be visible together. Move all three characters independently. Each peer targets ten publishes per second; host forwards only newly accepted packets. Copy all three reports."
        : L"BOTH clients need Capacity Test and Prepare Trial while solo. Link Trial / Copy Invite hosts; Join Trial joins. Send Pose is publish-only. Each client renders one peer. Rebishaz's unmodified vanilla view does not receive a third traveler.");
    return n>0 && (size_t)n<cap ? 1 : -1;
}
static void restore_result_position(HWND window);
static unsigned char invite_config[36];
static INT_PTR CALLBACK invite_dialog(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    (void)lp;
    if (message==WM_INITDIALOG) {
        restore_result_position(window);
        SetWindowTextW(window,L"Join link - paste the host invite");
        CreateWindowExW(0,L"STATIC",L"Paste the invite copied in the receiver bottle. Local Mac only.",WS_CHILD|WS_VISIBLE,10,10,470,22,window,NULL,NULL,NULL);
        HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,10,38,470,28,window,(HMENU)(INT_PTR)100,NULL,NULL);
        SendMessageW(edit,EM_SETLIMITTEXT,80,0);
        CreateWindowExW(0,L"BUTTON",L"Join",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,240,80,110,28,window,(HMENU)(INT_PTR)IDOK,NULL,NULL);
        CreateWindowExW(0,L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,365,80,110,28,window,(HMENU)(INT_PTR)IDCANCEL,NULL,NULL);
        SetFocus(edit); return FALSE;
    }
    if (message==WM_COMMAND && LOWORD(wp)==IDCANCEL) { EndDialog(window,0); return TRUE; }
    if (message==WM_COMMAND && LOWORD(wp)==IDOK) {
        wchar_t text[82]; GetDlgItemTextW(window,100,text,82);
        size_t i=4; unsigned port=0,any=0; int valid=!wcsncmp(text,L"JC1-",4);
        if (valid && (text[i]<L'1' || text[i]>L'9')) valid=0;
        while (valid && text[i]>=L'0' && text[i]<=L'9') {
            port=port*10+(unsigned)(text[i++]-L'0'); if (port>65535) valid=0;
        }
        if (!valid || !port || text[i]!=L'-' || wcslen(text+i+1)!=64) valid=0;
        if (valid) {
            ++i;
            for (unsigned b=0;b<32;++b) {
                unsigned value=0;
                for (unsigned digit=0;digit<2;++digit) {
                    wchar_t c=text[i++]; int v=c>=L'0' && c<=L'9' ? c-L'0' : c>=L'a' && c<=L'f' ? c-L'a'+10 : c>=L'A' && c<=L'F' ? c-L'A'+10 : -1;
                    if (v<0) { valid=0; v=0; } value=(value<<4)|(unsigned)v;
                }
                invite_config[4+b]=(unsigned char)value; any|=value;
            }
        }
        SecureZeroMemory(text,sizeof(text));
        if (!valid || !any) { SecureZeroMemory(invite_config,sizeof(invite_config)); SetWindowTextW(window,L"Invalid invite - paste JC1-port-key from Copy Invite"); return TRUE; }
        for (unsigned b=0;b<4;++b) invite_config[b]=(unsigned char)(port>>(b*8));
        SetDlgItemTextW(window,100,L""); EndDialog(window,1); return TRUE;
    }
    return FALSE;
}
static int request_invite(void) {
    struct { DLGTEMPLATE dialog; WORD menu,window_class,title; } layout={0};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
    if (!result_position_valid) layout.dialog.style|=DS_CENTER;
    layout.dialog.cx=335; layout.dialog.cy=90;
    SecureZeroMemory(invite_config,sizeof(invite_config));
    return DialogBoxIndirectParamW(GetModuleHandleW(NULL),&layout.dialog,NULL,invite_dialog,0)==1;
}
static int copy_link_invite(HANDLE process,uintptr_t base) {
    unsigned char config[36]={0},again[36]={0}; LONG state[3]; int copied=0;
    wchar_t text[82]={0}; HGLOBAL memory=NULL;
    if (!remote_read(process,base+JC_LINK_RVA,state,sizeof(state)) || state[0]!=1 || (state[2]!=1 && state[2]!=4) ||
        !remote_read(process,base+JC_LINK_CONFIG_RVA,config,sizeof(config)) ||
        !remote_read(process,base+JC_LINK_CONFIG_RVA,again,sizeof(again)) || memcmp(config,again,36)) goto done;
    unsigned port=(unsigned)config[0]+((unsigned)config[1]<<8);
    if (!port || config[2] || config[3]) goto done;
    int n=swprintf(text,82,L"JC1-%u-",port); if (n<0 || n>15) goto done;
    for (unsigned i=0;i<32;++i) { text[n++]=L"0123456789abcdef"[config[i+4]>>4]; text[n++]=L"0123456789abcdef"[config[i+4]&15]; }
    text[n]=0; memory=GlobalAlloc(GMEM_MOVEABLE,(n+1)*sizeof(wchar_t));
    void *buffer=memory ? GlobalLock(memory) : NULL;
    if (buffer) {
        memcpy(buffer,text,(n+1)*sizeof(wchar_t)); GlobalUnlock(memory);
        if (OpenClipboard(NULL)) {
            if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT,memory)) { copied=1; memory=NULL; }
            CloseClipboard();
        }
    }
done:
    if (memory) GlobalFree(memory);
    SecureZeroMemory(config,sizeof(config)); SecureZeroMemory(again,sizeof(again)); SecureZeroMemory(text,sizeof(text));
    return copied;
}
static void format_exit_report(wchar_t *text,size_t cap,int obtained,uint32_t code,const wchar_t *last) {
    if (!cap) return;
    text[0]=0;
    int n=obtained ? swprintf(text,cap,L"Journey exited. Process exit code: 0x%08lX.\r\n",(unsigned long)code) :
        swprintf(text,cap,L"Journey process unavailable; exit code could not be read.\r\n");
    if (n<0 || (size_t)n>=cap) { text[cap-1]=0; return; }
    size_t used=(size_t)n;
    const wchar_t *parts[]={L"Keep JNET open briefly while its exit recorder finishes. Session diagnostics stay in memory until JNET closes. Use Save Session Report to keep them. No crash cause is inferred from the exit code alone.\r\n\r\n",
        last[0] ? L"Last readable live report (sampled before exit):\r\n" : L"No live report was retained in this window.\r\n",last};
    for (unsigned i=0;i<3;++i) {
        size_t count=wcslen(parts[i]); if (count>cap-used-1) count=cap-used-1;
        wmemcpy(text+used,parts[i],count); used+=count; text[used]=0;
    }
}
static void append_exit_text(const wchar_t *value) {
    size_t used=wcslen(report_target),count=wcslen(value);
    if (count>JC_REPORT_CAP-1-used) count=JC_REPORT_CAP-1-used;
    wmemcpy(report_target+used,value,count);report_target[used+count]=0;
}
static void append_memory_tail(struct jnet_memory_log *log,const wchar_t *label,size_t limit) {
    char bytes[6001];wchar_t decoded[6001];
    if(limit>6000)limit=6000;
    size_t got=jnet_log_tail(log,bytes,limit+1);
    if(!MultiByteToWideChar(CP_UTF8,0,bytes,-1,decoded,6001))return;
    append_exit_text(L"\r\n\r\n");append_exit_text(label);append_exit_text(L"\r\n");
    append_exit_text(got?decoded:L"No entries recorded.");
    if(log && atomic_load(&log->dropped))append_exit_text(L"\r\nRecent entries retained; older or busy entries omitted.");
}
static void append_session_memory(void) {
    append_memory_tail(&exit_log,L"EXIT RECORDER",2000);
    append_memory_tail(&hang_log,L"FORCED-CLOSE DIAGNOSTICS",6000);
    if(exit_watch.logs && atomic_load(&exit_watch.logs->ready)){
        append_memory_tail(&exit_watch.logs->runtime,L"RUNTIME",6000);
        append_memory_tail(&exit_watch.logs->fault,L"FAULTS",2*JC_FAULT_BYTES);
    }else append_exit_text(L"\r\nNative session diagnostics unavailable.");
    append_memory_tail(&helper_log,L"CLIENT",2000);
}
static const wchar_t *session_report_text(const wchar_t *text) {
    static wchar_t report[JC_REPORT_CAP];
    wcsncpy(report,text?text:L"",JC_REPORT_CAP-1);report[JC_REPORT_CAP-1]=0;
    report_target=report;
    if(!wcsstr(report,L"EXIT RECORDER"))append_session_memory();
    report_target=preview_text;
    return report;
}
static int session_write(HANDLE file,const char *text,size_t bytes) {
    DWORD written=0;
    return bytes<=MAXDWORD && WriteFile(file,text,(DWORD)bytes,&written,NULL) && written==bytes;
}
static int session_write_log(HANDLE file,const char *label,struct jnet_memory_log *log) {
    if(!session_write(file,label,strlen(label)))return 0;
    if(!log)return session_write(file,"Unavailable.\r\n",14);
    uint32_t count=atomic_load(&log->count),start=count>JNET_LOG_ROWS?count-JNET_LOG_ROWS:0;
    char bytes[JNET_LOG_BYTES];
    for(uint32_t i=start;i<count;++i){
        size_t n=jnet_log_read_row(log,i,bytes);
        if(n && !session_write(file,bytes,n))return 0;
    }
    if(atomic_load(&log->dropped)){
        const char note[]="\r\nRecent entries retained; older or busy entries omitted.\r\n";
        if(!session_write(file,note,sizeof(note)-1))return 0;
    }
    return 1;
}
static int refresh_preview(void);
static void save_session_report(HWND window) {
    wchar_t path[JC_PATH_CAP]=L"JNET-session-report.txt";
    OPENFILENAMEW picker={0};picker.lStructSize=sizeof(picker);picker.hwndOwner=window;
    picker.lpstrFilter=L"Session report (*.txt)\0*.txt\0\0";
    picker.lpstrFile=path;picker.nMaxFile=JC_PATH_CAP;picker.lpstrDefExt=L"txt";
    picker.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(!GetSaveFileNameW(&picker))return;
    if(preview_process){refresh_preview();SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)preview_text);}
    const wchar_t *text=(const wchar_t *)GetWindowLongPtrW(window,GWLP_USERDATA);
    int bytes=WideCharToMultiByte(CP_UTF8,0,text?text:L"",-1,NULL,0,NULL,NULL);
    char *utf8=bytes>0?HeapAlloc(GetProcessHeap(),0,(SIZE_T)bytes):NULL;
    HANDLE file=INVALID_HANDLE_VALUE;int ok=0;
    if(utf8 && WideCharToMultiByte(CP_UTF8,0,text?text:L"",-1,utf8,bytes,NULL,NULL)){
        file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE)ok=session_write(file,utf8,(size_t)bytes-1) &&
            session_write_log(file,"\r\n\r\nCLIENT\r\n",&helper_log) &&
            session_write_log(file,"\r\n\r\nEXIT RECORDER\r\n",&exit_log) &&
            session_write_log(file,"\r\n\r\nFORCED-CLOSE DIAGNOSTICS\r\n",&hang_log) &&
            session_write_log(file,"\r\n\r\nRUNTIME\r\n",exit_watch.logs && atomic_load(&exit_watch.logs->ready)?&exit_watch.logs->runtime:NULL) &&
            session_write_log(file,"\r\n\r\nFAULTS\r\n",exit_watch.logs && atomic_load(&exit_watch.logs->ready)?&exit_watch.logs->fault:NULL) && FlushFileBuffers(file);
    }
    if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
    if(utf8)HeapFree(GetProcessHeap(),0,utf8);
    SetDlgItemTextW(window,117,ok?L"Session report saved":L"Could not save the complete report; please retry");
}
static void record_hang_threads(HANDLE process,DWORD pid) {
    uintptr_t image=0;DWORD image_size=0;
    HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    if(modules==INVALID_HANDLE_VALUE)return;
    MODULEENTRY32W module={.dwSize=sizeof(module)};
    if(Module32FirstW(modules,&module))do {
        if(module.th32ProcessID==pid){
            image=(uintptr_t)module.modBaseAddr;image_size=module.modBaseSize;break;
        }
    }while(Module32NextW(modules,&module));
    CloseHandle(modules);if(!image || !image_size)return;
    HANDLE threads=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    THREADENTRY32 entry={.dwSize=sizeof(entry)};unsigned count=0;
    if(threads!=INVALID_HANDLE_VALUE && Thread32First(threads,&entry))do {
        if(entry.th32OwnerProcessID!=pid || count++>=24)continue;
        HANDLE thread=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT,FALSE,entry.th32ThreadID);
        CONTEXT context={.ContextFlags=CONTEXT_CONTROL};uint64_t stack[128];SIZE_T bytes=0;
        int captured=0;DWORD error=0;
        if(thread){
            if(SuspendThread(thread)!=(DWORD)-1){
                captured=GetThreadContext(thread,&context)!=0;
                if(!captured)error=GetLastError();
                if(captured)ReadProcessMemory(process,(void *)(uintptr_t)context.Rsp,stack,sizeof(stack),&bytes);
                ResumeThread(thread);
            }else error=GetLastError();
            CloseHandle(thread);
        }else error=GetLastError();
        char line[640];int used=snprintf(line,sizeof(line),"thread=%lu captured=%d error=%lu rip=%llx game_rva=%llx stack_candidates=",
            (unsigned long)entry.th32ThreadID,captured,(unsigned long)error,(unsigned long long)context.Rip,
            (unsigned long long)(context.Rip>=image && context.Rip-image<image_size?context.Rip-image:0));
        unsigned found=0;
        for(size_t i=0;captured && i<bytes/8 && found<12;++i){
            if(stack[i]<image+0x1000 || stack[i]-image>=0x550000)continue;
            used+=snprintf(line+used,sizeof(line)-(size_t)used," %llx",(unsigned long long)(stack[i]-image));++found;
        }
        used+=snprintf(line+used,sizeof(line)-(size_t)used,"\r\n");
        jnet_log_append(&hang_log,line,(size_t)used);
    }while(threads!=INVALID_HANDLE_VALUE && Thread32Next(threads,&entry));
    if(threads!=INVALID_HANDLE_VALUE)CloseHandle(threads);
}
static DWORD force_close_journey(HANDLE process,DWORD expected_pid) {
    if (!process || !expected_pid || GetProcessId(process)!=expected_pid) return ERROR_INVALID_HANDLE;
    DWORD waited=WaitForSingleObject(process,0);
    if (waited==WAIT_OBJECT_0) return 0;
    if (waited!=WAIT_TIMEOUT) return GetLastError();
    record_hang_threads(process,expected_pid);
    return TerminateProcess(process,0xe04a4302u) ? 0 : GetLastError();
}
static int refresh_preview(void) {
    LONG life[152], appearance[40], frames, pending;
    if (WaitForSingleObject(preview_process,0)!=WAIT_TIMEOUT) {
        DWORD code=0;
        int obtained=GetExitCodeProcess(preview_process,&code);
        int recorded=exit_watch.thread && WaitForSingleObject(exit_watch.thread,0)==WAIT_OBJECT_0;
        const wchar_t *last=recorded && exit_watch.last_report[0] ? exit_watch.last_report : preview_last_report;
        format_exit_report(preview_text,JC_REPORT_CAP,obtained,code,last);
        if (obtained && code==0xe04a4302u) append_exit_text(L"\r\nForced closed with JNET. This exit code records your button press, not a crash.\r\n");
        if (!recorded) return 1;
        append_exit_text(L"\r\nExit recorder finished. Copy All or Save Session Report to keep this report before closing JNET.\r\n");
        append_session_memory();
        return 0;
    }
    if (!remote_read(preview_process,preview_base+JC_MESH_LIFE_RVA,life,sizeof(life)) ||
        !remote_read(preview_process,preview_base+JC_APPEARANCE_RVA,appearance,sizeof(appearance)) ||
        !remote_read(preview_process,preview_base+JC_FRAMES_RVA,&frames,sizeof(frames)) ||
        !remote_read(preview_process,preview_base+JC_COMMAND_RVA,&pending,sizeof(pending))) {
        wcscpy(preview_text,L"Live preview status could not be read; no outcome is inferred.\r\nUse Status for another read or save the session report after quitting Journey. Do not queue Show Trial again in this process.");
        return 0;
    }
    int linked=link_report(preview_process,preview_base,preview_text,20000);
    if (linked>0) { wcscpy(preview_last_report,preview_text); return 1; }
    if (linked<0) { wcscpy(preview_text,L"Pose link status unreadable. Quit and restart before retrying."); return 0; }
    const wchar_t *state=pending==10 ? L"Queued; waiting for Journey to process the request" :
        life[25]<0 || life[0]<0 ? L"Stopped or failed; copy the result and quit Journey" :
        life[25]==3 ? L"Draw window ended; report whether a mesh was visible, then quit Journey" :
        life[25]==2 ? L"Draw window active; look around nearby in Journey" :
        life[25]==1 ? L"Delay or render callback pending; return to gameplay" :
        L"Waiting for the game's preparation callback";
    int length=swprintf(preview_text,20000,
        L"SHOW TRIAL - LIVE STATUS\r\n%ls.\r\n\r\n"
        L"Selected chapter: %ls.\r\nChapter sampled at Show Trial: %ls.\r\n"
        L"Game update: %ld. Pending action: %ld. Delay: %ld seconds.\r\n\r\n"
        L"Appearance result: %ld (-1 pending; 0 accepted; 6 source validation). Stage: %ld (0 no writes; 3 checks passed).\r\n"
        L"Setter calls: %ld. Last inspected material list / unique instances: %ld / %ld.\r\n"
        L"Original material buffers unchanged: %ld (1 yes; -1 not checked).\r\n\r\n"
        L"Source validation: %ls. Parameter: %ls. Resource check: %ld.\r\n"
        L"Resource codes: -1 pending; 0 accepted; 1 read; 2 type; 3 changed/mismatch; 4 range.\r\n"
        L"Source/trial parameter masks: %ld / %ld. Missing texture masks: %ld / %ld.\r\n"
        L"Source textures differing from definition: %ld (not itself an error).\r\n"
        L"Validated cached/current cube slot: %ld / %ld of %ld. Refreshed-index mask: %ld.\r\n"
        L"Slot values require appearance result 0; an older cached index can be valid.\r\n\r\n"
        L"Trial torso shader: result %ld; step %ld; write stage %ld. Original buffers unchanged: %ld; trial-only changes: %ld.\r\n"
        L"Result 0 and stage 3 mean CPU values checked, not visible pixels.\r\n\r\n"
        L"Validated consumer calls: %ld / 600. Failure: %ld. Call stage: %ld.\r\n"
        L"A returned consumer call does not prove visible pixels. Use Status for full failure details.\r\n\r\n"
        L"Placement bounds code: %ld (-1 disabled; native culling is unhooked).\r\n"
        L"Center relative to captured player: %.3f / %.3f / %.3f world units; use only when code is 0.\r\n"
        L"Placement is captured after the delay, intended +3 world X. It stays there if you move.\r\n\r\n"
        L"%ls Do not change chapters or click Show Trial again.\r\n"
        L"This panel reads once per second for at most three minutes; selecting text pauses its refresh. Copy all copies the displayed result. Status gives the full report.\r\n"
        L"Snapshots are not atomic. No third player is connected. Resources stay allocated until Journey exits.",
        state,level_label(life[108]),level_label(life[96]),(long)frames,(long)pending,(long)life[109],
        (long)appearance[0],(long)appearance[1],(long)appearance[2],(long)appearance[16],(long)appearance[21],
        (long)appearance[3],appearance_source_reason(appearance[22]),appearance_parameter(appearance[23]),(long)appearance[24],
        (long)appearance[4],(long)appearance[25],(long)appearance[27],(long)appearance[28],(long)appearance[29],
        (long)appearance[31],(long)appearance[32],(long)appearance[33],(long)appearance[30],
        (long)appearance[34],(long)appearance[35],(long)appearance[36],(long)appearance[37],(long)appearance[38],(long)life[26],(long)life[27],(long)life[28],(long)life[98],
        life[99]/1000.0,life[100]/1000.0,life[101]/1000.0,
        life[25]<0 || life[0]<0 || life[25]==3 ? L"Copy the report and quit Journey; this test has ended." :
        L"Return to gameplay to advance the test; Journey may pause in the background.");
    if (length<0 || length>=20000) {
        wcscpy(preview_text,L"Preview status formatting failed. Click Status for the full report."); return 0;
    }
    return pending==10 || (life[25]>=0 && life[25]!=3 && life[0]>=0);
}
static const wchar_t *action_modes[] = {
    L"--traveler-startup", NULL, NULL, NULL, NULL, NULL, NULL, L"--status"
};
static const wchar_t *action_labels[] = {
    L"Launch Journet", L"Export DDS", L"Export PNG", L"Import DDS", L"Import PNG", L"Preview", L"Restore Default", L"Status"
};
enum { JC_ACTION_COUNT = sizeof(action_modes) / sizeof(action_modes[0]) };
_Static_assert(sizeof(action_labels) / sizeof(action_labels[0]) == JC_ACTION_COUNT, "Action labels must match modes");

static void trace(const char *stage, DWORD error, DWORD detail) {
    char line[192];
    int length = snprintf(line, sizeof(line), "[JC-helper] stage=%s error=%lu detail=%lu\r\n",
                          stage, (unsigned long)error, (unsigned long)detail);
    if (length > 0 && (size_t)length < sizeof(line) ) {
        jnet_log_append(&helper_log,line,(size_t)length);
    }
}

static uintptr_t fail(const char *stage, DWORD error, DWORD detail) {
    failure_stage = stage;
    failure_error = error;
    failure_detail = detail;
    trace(stage, error, detail);
    return 0;
}

static void open_helper_log(const wchar_t *directory) {
    (void)directory;
    trace("helper_version", 0, 9100);
}

static void restore_result_position(HWND window) {
    if (!result_position_valid) return;
    RECT current;
    if (!GetWindowRect(window, &current)) return;
    LONG x = result_position.left, y = result_position.top;
    MONITORINFO monitor = {0};
    monitor.cbSize = sizeof(monitor);
    if (GetMonitorInfoW(MonitorFromRect(&result_position, MONITOR_DEFAULTTONEAREST), &monitor)) {
        LONG right = monitor.rcWork.right - (current.right - current.left);
        LONG bottom = monitor.rcWork.bottom - (current.bottom - current.top);
        if (x > right) x = right;
        if (y > bottom) y = bottom;
        if (x < monitor.rcWork.left) x = monitor.rcWork.left;
        if (y < monitor.rcWork.top) y = monitor.rcWork.top;
    }
    SetWindowPos(window, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

#include "connection_recorder.h"

static struct {
    unsigned dpi;
    HFONT title,body,small;
    HBRUSH sand,paper,accent,pressed;
} result_theme;
static int result_scale(int value) { return MulDiv(value,(int)result_theme.dpi,96); }
static void result_move(HWND window,int id,int x,int y,int width,int height) {
    MoveWindow(GetDlgItem(window,id),result_scale(x),result_scale(y),result_scale(width),result_scale(height),TRUE);
}
static void result_layout(HWND window) {
    RECT area; GetClientRect(window,&area);
    int width=MulDiv(area.right,96,(int)result_theme.dpi),height=MulDiv(area.bottom,96,(int)result_theme.dpi);
    result_move(window,110,24,55,width-190,42);
    result_move(window,111,26,101,width-52,22);
    result_move(window,112,26,142,184,20);
    result_move(window,113,26,262,184,20);
    result_move(window,114,26,454,184,20);
    static const int button_y[]={168,288,327,366,405,480,519};
    for (int i=0;i<7;++i) result_move(window,200+i,24,button_y[i],184,34);
    result_move(window,207,width-138,126,112,34);
    result_move(window,208,24,207,184,34);
    result_move(window,115,232,138,width-382,22);
    result_move(window,100,232,168,width-258,height-314);
    result_move(window,101,232,height-129,136,34);
    result_move(window,103,380,height-129,184,34);
    result_move(window,102,width-312,height-129,184,34);
    result_move(window,117,24,height-88,width-48,20);
    result_move(window,IDOK,width-112,height-129,86,34);
    result_move(window,116,24,height-65,width-48,18);
    result_move(window,118,0,8,width,36);
    result_move(window,119,0,height-44,width,36);
    HWND edit=GetDlgItem(window,100);
    if (edit) {
        RECT paper; GetClientRect(edit,&paper);
        paper.left+=result_scale(14); paper.top+=result_scale(12); paper.right-=result_scale(12);
        SendMessageW(edit,EM_SETRECT,0,(LPARAM)&paper);
    }
}
static HWND result_control(HWND window,const wchar_t *type,const wchar_t *text,DWORD style,int id,HFONT font) {
    HWND child=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|style,0,0,1,1,
        window,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    if (child) SendMessageW(child,WM_SETFONT,(WPARAM)font,FALSE);
    return child;
}
static void result_theme_clear(void) {
    if (result_theme.title) DeleteObject(result_theme.title);
    if (result_theme.body) DeleteObject(result_theme.body);
    if (result_theme.small) DeleteObject(result_theme.small);
    if (result_theme.sand) DeleteObject(result_theme.sand);
    if (result_theme.paper) DeleteObject(result_theme.paper);
    if (result_theme.accent) DeleteObject(result_theme.accent);
    if (result_theme.pressed) DeleteObject(result_theme.pressed);
    memset(&result_theme,0,sizeof(result_theme));
}
static void result_heading(HWND window,const wchar_t *text) {
    const wchar_t *repeat=wcsstr(text,L"Local 3 overall: ");
    if(!repeat)repeat=wcsstr(text,L"Repeat test: ");
    if(!repeat || wcsstr(repeat,L"Repeat test: idle.")==repeat)return;
    wchar_t summary[160];size_t count=wcscspn(repeat,L"\r\n");
    if(wcsstr(repeat,L"Local 3 overall: ")==repeat)count=wcscspn(repeat,L".\r\n");
    if(count>=160)count=159;
    wmemcpy(summary,repeat,count);summary[count]=0;
    SetDlgItemTextW(window,115,summary);
}
static void result_scarf(const DRAWITEMSTRUCT *item) {
    BITMAPINFO info={0};
    info.bmiHeader.biSize=sizeof(info.bmiHeader);
    info.bmiHeader.biWidth=JC_SCARF_WIDTH;info.bmiHeader.biHeight=-JC_SCARF_HEIGHT;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    int height=item->rcItem.bottom-item->rcItem.top;
    int tile=MulDiv(JC_SCARF_WIDTH,height,JC_SCARF_HEIGHT);
    if(tile<1)return;
    int mode=SetStretchBltMode(item->hDC,COLORONCOLOR);
    for(int x=item->rcItem.left;x<item->rcItem.right;x+=tile)
        StretchDIBits(item->hDC,x,item->rcItem.top,tile,height,0,0,JC_SCARF_WIDTH,JC_SCARF_HEIGHT,
            item->CtlID==118?jc_scarf_red:jc_scarf_white,&info,DIB_RGB_COLORS,SRCCOPY);
    SetStretchBltMode(item->hDC,mode);
}
static HANDLE find_game(const wchar_t *expected, DWORD *pid, int read_only);
static void select_executable(HWND window) {
    wchar_t directory[JC_PATH_CAP],current[JC_PATH_CAP],selected[JC_PATH_CAP]={0},settings[JC_PATH_CAP];
    if (!jc_paths(NULL,directory,current)) {
        DWORD n=GetModuleFileNameW(NULL,directory,JC_PATH_CAP);
        if(!n || n>=JC_PATH_CAP || !jc_parent(directory))return;
        current[0]=0;
    }
    DWORD pid=0;HANDLE running=current[0]?find_game(current,&pid,1):NULL;
    if(running || (preview_process && WaitForSingleObject(preview_process,0)==WAIT_TIMEOUT)) {
        if(running)CloseHandle(running);
        MessageBoxW(window,L"Close Journey before changing the selected executable.",L"Select executable",MB_OK);return;
    }
    OPENFILENAMEW picker={0};picker.lStructSize=sizeof(picker);picker.hwndOwner=window;
    picker.lpstrFilter=L"Journey executable (*.exe)\0*.exe\0\0";
    picker.lpstrFile=selected;picker.nMaxFile=JC_PATH_CAP;picker.lpstrInitialDir=directory;
    picker.lpstrTitle=L"Select Journey executable";
    picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
    if(!GetOpenFileNameW(&picker)) {
        if(CommDlgExtendedError())MessageBoxW(window,L"The file picker could not open.",L"Select executable",MB_OK|MB_ICONERROR);
        return;
    }
    unsigned char hash[32];
    if(!jc_hash(selected,hash) || !jc_executable_size(hash)) {
        MessageBoxW(window,L"Unsupported executable. Select the supported original Journey.exe or Journey Sound Fix 0.3.1. Your previous selection is unchanged.",L"Select executable",MB_OK|MB_ICONERROR);return;
    }
    running=find_game(selected,&pid,1);
    if(running){CloseHandle(running);MessageBoxW(window,L"Close the selected Journey process before changing the selection.",L"Select executable",MB_OK);return;}
    if(swprintf(settings,JC_PATH_CAP,L"%ls\\JNET-launch.ini",directory)<0 ||
       !WritePrivateProfileStringW(L"Launch",L"Executable",selected,settings)) {
        MessageBoxW(window,L"Could not save the executable selection.",L"Select executable",MB_OK|MB_ICONERROR);return;
    }
    MessageBoxW(window,selected,L"Executable selected for the next launch",MB_OK);
}
static INT_PTR CALLBACK result_dialog(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_INITDIALOG) {
        const wchar_t *text = (const wchar_t *)lparam;
        SetWindowLongPtrW(window, GWLP_USERDATA, lparam);
        SetWindowTextW(window, L"JNET " JC_HELPER_VERSION L" | Journet");
        HDC dc=GetDC(window);
        result_theme.dpi=dc ? (unsigned)GetDeviceCaps(dc,LOGPIXELSY) : 96;
        if (dc) ReleaseDC(window,dc);
        if (!result_theme.dpi) result_theme.dpi=96;
        result_theme.sand=CreateSolidBrush(RGB(237,225,203));
        result_theme.paper=CreateSolidBrush(RGB(252,248,237));
        result_theme.accent=CreateSolidBrush(RGB(191,139,77));
        result_theme.pressed=CreateSolidBrush(RGB(105,70,43));
        result_theme.title=CreateFontW(-result_scale(31),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_ROMAN,L"Georgia");
        result_theme.body=CreateFontW(-result_scale(15),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
        result_theme.small=CreateFontW(-result_scale(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
        if (!result_theme.sand || !result_theme.paper || !result_theme.accent || !result_theme.pressed ||
            !result_theme.title || !result_theme.body || !result_theme.small) { EndDialog(window,-1); return TRUE; }
        int armed=wcsstr(text,L"Visible Trial: Armed:")!=NULL || wcsstr(text,L"Mesh Lifetime: Armed:")!=NULL || wcsstr(text,L"Idle Pose: Armed:")!=NULL || wcsstr(text,L"Chapter Refs: Armed:")!=NULL;
        const wchar_t *retained=wcsstr(text,L"Visible Trial: Trial retained in ");
        if (!result_control(window,L"STATIC",L"",SS_OWNERDRAW,118,result_theme.small) ||
            !result_control(window,L"STATIC",L"",SS_OWNERDRAW,119,result_theme.small) ||
            !result_control(window,L"STATIC",L"JOURNET",0,110,result_theme.title) ||
            !result_control(window,L"STATIC",L"JNET  /  " JC_HELPER_VERSION,0,111,result_theme.small) ||
            !result_control(window,L"STATIC",L"LAUNCH",0,112,result_theme.small) ||
            !result_control(window,L"STATIC",L"Robe Editor",0,113,result_theme.small) ||
            !result_control(window,L"STATIC",L"Coming soon",0,114,result_theme.small) ||
            !result_control(window,L"STATIC",armed ? L"Armed - return through the Desert portal" :
                retained ? L"Trial retained - both bodies ready" : L"SESSION REPORT",0,115,result_theme.body) ||
            !result_control(window,L"STATIC",L"Use Journey’s World menu to create or join a lobby. Save a session report before closing JNET if needed.",0,116,result_theme.small) ||
            !result_control(window,L"STATIC",L"",0,117,result_theme.small)) {
            EndDialog(window,-1); return TRUE;
        }
        HWND edit=result_control(window,L"EDIT",text,WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,100,result_theme.body);
        HWND copy=result_control(window,L"BUTTON",L"Copy All",WS_TABSTOP|BS_OWNERDRAW,101,result_theme.body);
        HWND save=result_control(window,L"BUTTON",L"Save Session Report",WS_TABSTOP|BS_OWNERDRAW,103,result_theme.body);
        HWND close=result_control(window,L"BUTTON",L"Close",WS_TABSTOP|BS_OWNERDRAW,IDOK,result_theme.body);
        HWND kill=result_control(window,L"BUTTON",L"Force Quit Journey",WS_TABSTOP|BS_OWNERDRAW,102,result_theme.body);
        if (kill) EnableWindow(kill,preview_process && WaitForSingleObject(preview_process,0)==WAIT_TIMEOUT);
        if (!edit || !copy || !save || !close || !kill) { EndDialog(window, -1); return TRUE; }
        for (int i = 0; i < JC_ACTION_COUNT; ++i) {
            HWND button=result_control(window,L"BUTTON",action_labels[i],WS_TABSTOP|BS_OWNERDRAW,200+i,result_theme.body);
            if (!button) { EndDialog(window, -1); return TRUE; }
            if (!action_modes[i]) EnableWindow(button,FALSE);
        }
        if(!result_control(window,L"BUTTON",L"Select .exe...",WS_TABSTOP|BS_OWNERDRAW,208,result_theme.body)) {EndDialog(window,-1);return TRUE;}
        RECT bounds,client; GetWindowRect(window,&bounds); GetClientRect(window,&client);
        MONITORINFO monitor={0}; monitor.cbSize=sizeof(monitor);
        int width=result_position_valid ? result_position.right-result_position.left :
            MulDiv(JC_SCARF_WIDTH,result_scale(36),JC_SCARF_HEIGHT)+
            (bounds.right-bounds.left)-(client.right-client.left);
        int height=result_position_valid ? result_position.bottom-result_position.top : result_scale(730);
        if (width<result_scale(900)) width=result_scale(900);
        if (height<result_scale(710)) height=result_scale(710);
        if (GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor)) {
            if (width>monitor.rcWork.right-monitor.rcWork.left) width=monitor.rcWork.right-monitor.rcWork.left;
            if (height>monitor.rcWork.bottom-monitor.rcWork.top) height=monitor.rcWork.bottom-monitor.rcWork.top;
            bounds.left=monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-width)/2;
            bounds.top=monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-height)/2;
        }
        SetWindowPos(window,NULL,bounds.left,bounds.top,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
        restore_result_position(window);
        result_layout(window);
        result_heading(window,text);
        if (preview_process) {
            if (!SetTimer(window,1,1000,NULL)) SetDlgItemTextW(window,101,L"Copy (refresh unavailable)");
        }
        SendMessageW(edit, EM_SETSEL, 0, 0);
        SetFocus(edit);
        return FALSE;
    }
    if (message==WM_SIZE && wparam!=SIZE_MINIMIZED && result_theme.dpi) { result_layout(window); return TRUE; }
    if (message==WM_GETMINMAXINFO && result_theme.dpi) {
        MINMAXINFO *size=(MINMAXINFO *)lparam;
        size->ptMinTrackSize.x=result_scale(900); size->ptMinTrackSize.y=result_scale(710);
        return TRUE;
    }
    if (message==WM_CTLCOLORDLG && result_theme.sand) return (INT_PTR)result_theme.sand;
    if ((message==WM_CTLCOLORSTATIC || message==WM_CTLCOLOREDIT) && result_theme.sand) {
        int report=GetDlgCtrlID((HWND)lparam)==100;
        SetTextColor((HDC)wparam,RGB(70,49,35));
        SetBkColor((HDC)wparam,report ? RGB(252,248,237) : RGB(237,225,203));
        return (INT_PTR)(report ? result_theme.paper : result_theme.sand);
    }
    if (message==WM_DRAWITEM && result_theme.paper) {
        DRAWITEMSTRUCT *item=(DRAWITEMSTRUCT *)lparam;
        if (item->CtlType==ODT_STATIC && (item->CtlID==118 || item->CtlID==119)) { result_scarf(item);return TRUE; }
        if (item->CtlType!=ODT_BUTTON) return FALSE;
        int pressed=(item->itemState&ODS_SELECTED)!=0,primary=item->CtlID==200;
        FillRect(item->hDC,&item->rcItem,result_theme.sand);
        HGDIOBJ oldbrush=SelectObject(item->hDC,pressed ? result_theme.pressed : primary ? result_theme.accent : result_theme.paper);
        HPEN pen=CreatePen(PS_SOLID,1,RGB(206,183,149));
        HGDIOBJ oldpen=SelectObject(item->hDC,pen ? pen : GetStockObject(NULL_PEN));
        RoundRect(item->hDC,item->rcItem.left,item->rcItem.top,item->rcItem.right,item->rcItem.bottom,result_scale(8),result_scale(8));
        SelectObject(item->hDC,oldpen); SelectObject(item->hDC,oldbrush); if (pen) DeleteObject(pen);
        SetBkMode(item->hDC,TRANSPARENT);
        SetTextColor(item->hDC,(item->itemState&ODS_DISABLED) ? RGB(150,137,120) : pressed ? RGB(252,248,237) : RGB(70,49,35));
        HGDIOBJ oldfont=SelectObject(item->hDC,result_theme.body);
        wchar_t label[96]; GetWindowTextW(item->hwndItem,label,96);
        RECT text=item->rcItem; InflateRect(&text,-result_scale(8),0);
        DrawTextW(item->hDC,label,-1,&text,DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_END_ELLIPSIS);
        if (item->itemState&ODS_FOCUS) { RECT focus=item->rcItem; InflateRect(&focus,-4,-4); DrawFocusRect(item->hDC,&focus); }
        SelectObject(item->hDC,oldfont);
        return TRUE;
    }
    if (message==WM_TIMER && wparam==1 && preview_process) {
        int exited=WaitForSingleObject(preview_process,0)==WAIT_OBJECT_0;
        EnableWindow(GetDlgItem(window,102),!exited);
        DWORD start=0,end=0;
        SendDlgItemMessageW(window,100,EM_GETSEL,(WPARAM)&start,(LPARAM)&end);
        if (start!=end && !exited) return TRUE;
        int again=refresh_preview();
        LRESULT scroll=SendDlgItemMessageW(window,100,EM_GETFIRSTVISIBLELINE,0,0);
        SetDlgItemTextW(window,100,preview_text);
        result_heading(window,preview_text);
        SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)preview_text);
        SendDlgItemMessageW(window,100,EM_LINESCROLL,0,scroll);
        SetDlgItemTextW(window,101,L"Copy All");
        if (!again && exited) {
            KillTimer(window,1);SetDlgItemTextW(window,117,L"Exit report ready");
            SetWindowTextW(window,L"JNET - Journey closed; Copy All ready");
        }
        return TRUE;
    }
    if (message==WM_DESTROY) {
        KillTimer(window,1);
        RECT position;
        if (!IsIconic(window) && GetWindowRect(window, &position)) {
            result_position = position;
            result_position_valid = 1;
        }
    }
    if (message == WM_COMMAND) {
        if(LOWORD(wparam)==103 && HIWORD(wparam)==BN_CLICKED){save_session_report(window);return TRUE;}
        if(LOWORD(wparam)==208 && HIWORD(wparam)==BN_CLICKED){select_executable(window);return TRUE;}
        if (LOWORD(wparam)==102 && HIWORD(wparam)==BN_CLICKED) {
            if (!preview_process) return TRUE;
            refresh_preview();
            DWORD error=force_close_journey(preview_process,exit_watch.pid);
            if (error) {
                wchar_t failure[160];swprintf(failure,160,L"Could not close the attached Journey process (error %lu).",(unsigned long)error);
                SetDlgItemTextW(window,117,failure);return TRUE;
            }
            EnableWindow(GetDlgItem(window,102),FALSE);
            SetDlgItemTextW(window,117,L"Collecting exit report...");
            preview_polls=0;SendDlgItemMessageW(window,100,EM_SETSEL,0,0);
            SetTimer(window,1,1000,NULL);return TRUE;
        }
        if (HIWORD(wparam) == BN_CLICKED && LOWORD(wparam) >= 200 && LOWORD(wparam) < 200 + JC_ACTION_COUNT) {
            if (!action_modes[(int)LOWORD(wparam)-200]) return TRUE;
            requested_action = (int)LOWORD(wparam) - 199;
            EndDialog(window, 0);
            return TRUE;
        }
        if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL) { EndDialog(window, 0); return TRUE; }
        if (LOWORD(wparam) == 101) {
            if(preview_process){refresh_preview();SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)preview_text);}
            const wchar_t *text = session_report_text((const wchar_t *)GetWindowLongPtrW(window, GWLP_USERDATA));
            SIZE_T bytes = (wcslen(text) + 1) * sizeof(wchar_t);
            HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
            void *buffer = memory ? GlobalLock(memory) : NULL;
            int copied = 0;
            if (buffer) {
                memcpy(buffer, text, bytes);
                GlobalUnlock(memory);
                if (OpenClipboard(window)) {
                    if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory)) { copied = 1; memory = NULL; }
                    CloseClipboard();
                }
            }
            if (memory) GlobalFree(memory);
            SetDlgItemTextW(window, 101, copied ? L"Copied!" : L"Copy failed - retry");
            return TRUE;
        }
    }
    if (message == WM_CLOSE) { EndDialog(window, 0); return TRUE; }
    return FALSE;
}

static void show_result(const wchar_t *text, int code) {
    struct { DLGTEMPLATE dialog; WORD menu, window_class, title; } layout = {0};
    layout.dialog.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | DS_MODALFRAME;
    if (!result_position_valid) layout.dialog.style |= DS_CENTER;
    layout.dialog.cx = 360;
    layout.dialog.cy = 320;
    /* Standard edit controls need CRLF for displayed line breaks. */
    wchar_t normalized[JC_REPORT_CAP];
    size_t length = 0;
    for (size_t i = 0; text[i] && length + 2 < sizeof(normalized)/sizeof(normalized[0]); ++i) {
        if (text[i] == L'\n' && (!i || text[i-1] != L'\r')) normalized[length++] = L'\r';
        normalized[length++] = text[i];
    }
    normalized[length] = 0;
    INT_PTR shown=DialogBoxIndirectParamW(GetModuleHandleW(NULL), &layout.dialog, NULL, result_dialog, (LPARAM)normalized);
    result_theme_clear();
    if (shown == -1)
        MessageBoxW(NULL, text, L"JNET diagnostics", MB_OK | (code ? MB_ICONWARNING : MB_ICONINFORMATION));
}

static int finish(int code, const wchar_t *text) {
    trace("helper_exit", 0, (DWORD)code);
    fwprintf(code ? stderr : stdout, L"%ls\n", text);
    show_result(text, code);
    return code;
}

static int failure_message(void) {
    wchar_t stage[96], message[768];
    size_t i = 0;
    for (; failure_stage[i] && i < 95; ++i) stage[i] = (unsigned char)failure_stage[i];
    stage[i] = 0;
    swprintf(message, 768, L"Diagnostic step failed: %ls.\nWindows error: %lu. Detail: %lu.\n\nUse Save Session Report to include client diagnostics. Restart Journey before another attach attempt. Version " JC_HELPER_VERSION L".",
             stage, (unsigned long)failure_error, (unsigned long)failure_detail);
    return finish(1, message);
}

static int remote_read(HANDLE process, uintptr_t location, void *result, size_t length) {
    SIZE_T received = 0;
    return ReadProcessMemory(process, (void *)location, result, length, &received) && received == length;
}

static int layout_read(void *opaque, uint64_t address, void *out, size_t size) {
    return remote_read((HANDLE)opaque, (uintptr_t)address, out, size);
}

static ULONGLONG startup_lookup_deadline;
static int module_info(DWORD pid, const wchar_t *name, const wchar_t *path, MODULEENTRY32W *found) {
    module_error=0;
    for (unsigned attempt=0;attempt<64;++attempt) {
        HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
        if (snapshot==INVALID_HANDLE_VALUE) module_error=GetLastError();
        else {
            MODULEENTRY32W module={0}, candidate={0};
            module.dwSize=sizeof(module);
            unsigned count=0;
            BOOL more=Module32FirstW(snapshot,&module);
            while (more) {
                if (path ? jc_same_file(module.szExePath,path) : !_wcsicmp(module.szModule,name)) {
                    candidate=module;
                    ++count;
                }
                more=Module32NextW(snapshot,&module);
            }
            DWORD enumeration_error=GetLastError();
            CloseHandle(snapshot);
            if (enumeration_error==ERROR_NO_MORE_FILES) {
                if (count==1) { *found=candidate; module_error=0; return 1; }
                if (count>1) { module_error=ERROR_DUP_NAME; return 0; }
                module_error=ERROR_MOD_NOT_FOUND;
            } else module_error=enumeration_error ? enumeration_error : ERROR_PARTIAL_COPY;
        }
        int startup= startup_lookup_deadline && GetTickCount64()<startup_lookup_deadline;
        int retry=startup ? (module_error==ERROR_BAD_LENGTH || module_error==ERROR_NOACCESS ||
                            module_error==ERROR_PARTIAL_COPY || (module_error==ERROR_MOD_NOT_FOUND && _wcsicmp(name,L"JNET.dll"))) :
                           (module_error==ERROR_BAD_LENGTH && attempt<3);
        if (!retry || attempt==63) return 0;
        trace("module_snapshot_retry",module_error,attempt+1);
        Sleep(10);
    }
    return 0;
}

static HANDLE find_game(const wchar_t *expected, DWORD *pid, int read_only) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return NULL;
    PROCESSENTRY32W entry = {0};
    entry.dwSize = sizeof(entry);
    unsigned count = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            const wchar_t *base=wcsrchr(expected,L'\\');base=base?base+1:expected;
            if (_wcsicmp(entry.szExeFile,base)) continue;
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
            wchar_t image[JC_PATH_CAP];
            DWORD length = JC_PATH_CAP;
            if (process && QueryFullProcessImageNameW(process, 0, image, &length) && jc_same_file(image, expected)) {
                *pid = entry.th32ProcessID;
                ++count;
            }
            if (process) CloseHandle(process);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    if (count != 1) return NULL;
    DWORD rights = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE | PROCESS_TERMINATE;
    if (!read_only) rights |= PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE;
    HANDLE process = OpenProcess(rights, FALSE, *pid);
    wchar_t image[JC_PATH_CAP];
    DWORD length = JC_PATH_CAP;
    if (process && (!QueryFullProcessImageNameW(process, 0, image, &length) || !jc_same_file(image, expected))) {
        CloseHandle(process);
        return NULL;
    }
    return process;
}

struct startup_wait { const wchar_t *game; ULONGLONG deadline; DWORD pid; };
static INT_PTR CALLBACK startup_wait_dialog(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    struct startup_wait *state=(struct startup_wait *)(uintptr_t)GetWindowLongPtrW(window,GWLP_USERDATA);
    if (message==WM_INITDIALOG) {
        restore_result_position(window);
        state=(struct startup_wait *)lp;
        SetWindowLongPtrW(window,GWLP_USERDATA,lp);
        SetWindowTextW(window,L"JNET - preparing Journey");
        CreateWindowExW(0,L"STATIC",L"Waiting up to 60 seconds for Journey in this bottle.\nClose this window to cancel before attachment.",WS_CHILD|WS_VISIBLE,
            12,12,440,75,window,NULL,GetModuleHandleW(NULL),NULL);
        if (!SetTimer(window,1,100,NULL)) EndDialog(window,-1);
        return TRUE;
    }
    if (message==WM_TIMER && state) {
        HANDLE process=find_game(state->game,&state->pid,1);
        if (process) { CloseHandle(process); KillTimer(window,1); EndDialog(window,1); }
        else if (GetTickCount64()>=state->deadline) { KillTimer(window,1); EndDialog(window,0); }
        return TRUE;
    }
    if (message==WM_CLOSE || (message==WM_COMMAND && LOWORD(wp)==IDCANCEL)) {
        KillTimer(window,1); EndDialog(window,0); return TRUE;
    }
    return FALSE;
}
static DWORD wait_for_startup(const wchar_t *game) {
    struct { DLGTEMPLATE dialog; WORD menu, window_class, title; } layout={0};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
    if (!result_position_valid) layout.dialog.style|=DS_CENTER;
    layout.dialog.cx=300; layout.dialog.cy=80;
    struct startup_wait state={game,GetTickCount64()+60000,0};
    return DialogBoxIndirectParamW(GetModuleHandleW(NULL),&layout.dialog,NULL,startup_wait_dialog,(LPARAM)&state)==1 ? state.pid : 0;
}

struct remote_image { HANDLE process; uintptr_t base; DWORD error; };

static int read_image(void *opaque, uint32_t rva, void *out, size_t length) {
    struct remote_image *image = opaque;
    SIZE_T received = 0;
    if (!ReadProcessMemory(image->process, (void *)(image->base + rva), out, length, &received)) {
        image->error = GetLastError();
        return 0;
    }
    return received == length;
}

static int remote_control(HANDLE process,uintptr_t base,unsigned operation,LONG value) {
    uintptr_t packet=((uint64_t)(uint32_t)value<<32)|operation;
    HANDLE thread=CreateRemoteThread(process,NULL,0,(LPTHREAD_START_ROUTINE)(base+JC_CONTROL_RVA),
        (void *)packet,0,NULL);
    if (!thread) return 0;
    DWORD result=1;
    int ok=WaitForSingleObject(thread,15000)==WAIT_OBJECT_0 && GetExitCodeThread(thread,&result) && !result;
    CloseHandle(thread);
    return ok;
}

static uintptr_t remote_export(HANDLE process, DWORD pid, const wchar_t *module_name, const char *symbol, unsigned depth) {
    if (depth > 4) return fail("forward_depth", 0, depth);
    MODULEENTRY32W module;
    if (!module_info(pid, module_name, NULL, &module)) return fail("export_module_lookup", module_error, depth);
    struct remote_image image = {process, (uintptr_t)module.modBaseAddr, 0};
    struct jc_export_result resolved;
    int kind = jc_pe_export(read_image, &image, module.modBaseSize, symbol, &resolved);
    trace("export_parse", image.error, (DWORD)kind);
    if (kind == JC_PE_FORWARD) {
        char *separator = strrchr(resolved.forward, '.');
        if (!separator || separator == resolved.forward || !separator[1]) return fail("invalid_forward", 0, depth);
        wchar_t forwarded_module[132];
        size_t length = (size_t)(separator - resolved.forward);
        if (length > 120) return fail("invalid_forward", 0, depth);
        for (size_t i = 0; i < length; ++i) {
            unsigned char c = (unsigned char)resolved.forward[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.')) return fail("invalid_forward", 0, depth);
            forwarded_module[i] = c;
        }
        forwarded_module[length] = 0;
        if (length < 4 || _wcsicmp(forwarded_module + length - 4, L".dll")) wcscat(forwarded_module, L".dll");
        return remote_export(process, pid, forwarded_module, separator + 1, depth + 1);
    }
    if (kind != JC_PE_FUNCTION) return fail("export_resolution", image.error, (DWORD)kind);
    uintptr_t address = image.base + resolved.rva;
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQueryEx(process, (void *)address, &info, sizeof(info))) return fail("export_page_query", GetLastError(), 0);
    if (info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
        !(info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        return fail("export_not_executable", 0, info.Protect);
    trace("target_export_resolved", 0, depth);
    return address;
}

static int inject(HANDLE process, DWORD pid, const wchar_t *dll) {
    uintptr_t entry = remote_export(process, pid, L"kernel32.dll", "LoadLibraryW", 0);
    if (!entry) return 0;
    SIZE_T length = (wcslen(dll) + 1) * sizeof(wchar_t), written = 0;
    void *memory = VirtualAllocEx(process, NULL, length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!memory) return (int)fail("allocate_dll_path", GetLastError(), 0);
    if (!WriteProcessMemory(process, memory, dll, length, &written) || written != length) {
        fail("write_dll_path", GetLastError(), 0);
        VirtualFreeEx(process, memory, 0, MEM_RELEASE);
        return 0;
    }
    HANDLE thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)entry, memory, 0, NULL);
    if (!thread) {
        fail("create_load_thread", GetLastError(), 0);
        VirtualFreeEx(process, memory, 0, MEM_RELEASE);
        return 0;
    }
    trace("load_thread_created", 0, 0);
    DWORD waited = WaitForSingleObject(thread, 15000);
    DWORD wait_error = waited == WAIT_FAILED ? GetLastError() : 0;
    DWORD exit_code = 0;
    if (waited == WAIT_OBJECT_0 && GetExitCodeThread(thread, &exit_code))
        trace("load_thread_returned_zero", 0, exit_code == 0);
    CloseHandle(thread);
    if (waited != WAIT_OBJECT_0) { fail("wait_load_thread", wait_error, waited); return -1; }
    VirtualFreeEx(process, memory, 0, MEM_RELEASE);
    MODULEENTRY32W module;
    if (!module_info(pid, L"JNET.dll", dll, &module)) return (int)fail("confirm_loaded_dll", module_error, exit_code == 0);
    trace("dll_present", 0, 0);
    return 1;
}

#define JC_READER_PASS_EXIT 0x4a430001u
static int reader_check_child(void) {
    wchar_t directory[JC_PATH_CAP],game[JC_PATH_CAP],dll[JC_PATH_CAP]; unsigned char hash[32];
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
    if (!jc_paths(NULL,directory,game) || wcslen(directory)+13>=JC_PATH_CAP) return 5;
    wcscpy(dll,directory); wcscat(dll,L"\\JNET.dll");
    if (!jc_hash(dll,hash) || memcmp(hash,jc_dll_hash,32)) return 5;
    HMODULE module=LoadLibraryW(dll);
    if (!module) return 6;
    const unsigned char expected[32]=JC_BUILD_ID;
    const void *id=(const void *)(uintptr_t)GetProcAddress(module,"JNETBuildId");
    DWORD (WINAPI *check)(void)=(void *)(uintptr_t)GetProcAddress(module,"JNETReaderCheck");
    if (!id || memcmp(id,expected,32) || !check) { FreeLibrary(module); return 6; }
    DWORD result=check(); FreeLibrary(module);
    return result ? (int)result : (int)JC_READER_PASS_EXIT;
}
static LONG reader_check_isolated(void) {
    wchar_t executable[JC_PATH_CAP],command[JC_PATH_CAP+64];
    DWORD length=GetModuleFileNameW(NULL,executable,JC_PATH_CAP);
    if (!length || length>=JC_PATH_CAP) return 100;
    int n=swprintf(command,JC_PATH_CAP+64,L"\"%ls\" --reader-check-child",executable);
    if (n<=0 || n>=JC_PATH_CAP+64) return 100;
    STARTUPINFOW startup={.cb=sizeof(startup)}; PROCESS_INFORMATION child={0};
    if (!CreateProcessW(executable,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&startup,&child)) return 100;
    CloseHandle(child.hThread);
    DWORD waited=WaitForSingleObject(child.hProcess,5000),exit_code=STILL_ACTIVE;
    LONG result=102;
    if (waited==WAIT_OBJECT_0 && GetExitCodeProcess(child.hProcess,&exit_code))
        result=exit_code==JC_READER_PASS_EXIT ? 0 : exit_code>=1 && exit_code<=6 ? (LONG)exit_code : 103;
    else {
        result=waited==WAIT_TIMEOUT ? 101 : 102;
        TerminateProcess(child.hProcess,104);
        WaitForSingleObject(child.hProcess,1000);
    }
    CloseHandle(child.hProcess);
    trace("isolated_reader_check",exit_code,(DWORD)result);
    return result;
}

static int run_action(const wchar_t *mode) {
    int native_playback_request=!wcscmp(mode,L"--native-playback");
    int evaluation_request=!wcscmp(mode,L"--evaluation-check");
    int traveler_startup=!wcscmp(mode,L"--traveler-startup");
    int traveler_request=!wcscmp(mode,L"--traveler-trial");
    int active_request=!wcscmp(mode,L"--active-traveler");
    int drop_request=!wcscmp(mode,L"--drop-traveler");
    int outfit_request=!wcscmp(mode,L"--outfit-traveler");
    int connection_request=!wcscmp(mode,L"--connect-sessions");
    int local_three_request=!wcscmp(mode,L"--local-three");
    int repeat_request=connection_request || local_three_request || outfit_request || !wcscmp(mode,L"--repeat-traveler");
    int mirror_request=!wcscmp(mode,L"--mirror");
    int lifecycle_mode=!wcscmp(mode,L"--lifecycle-scan");
    int connection_mode=lifecycle_mode || !wcscmp(mode,L"--connection-scan") || !wcscmp(mode,L"--cloth-check");
    if ((!repeat_request && !drop_request && !active_request && !traveler_startup && !traveler_request && !native_playback_request && !evaluation_request && !mirror_request && !connection_mode && wcscmp(mode,L"--link-trial") && wcscmp(mode,L"--copy-invite") && wcscmp(mode,L"--send-pose") && wcscmp(mode,L"--join-trial") && wcscmp(mode,L"--host-room") && wcscmp(mode,L"--join-room") && wcscmp(mode,L"--next-peer") && wcscmp(mode,L"--stop-link") && wcscmp(mode, L"--startup-watch") && wcscmp(mode, L"--check") && wcscmp(mode, L"--load-test") && wcscmp(mode, L"--inspect") && wcscmp(mode, L"--attach") &&
        wcscmp(mode, L"--sample") && wcscmp(mode, L"--watch") &&
        wcscmp(mode, L"--preview") && wcscmp(mode, L"--render-refs") && wcscmp(mode, L"--build-check") && wcscmp(mode, L"--resource-test") && wcscmp(mode, L"--lifetime-test") && wcscmp(mode, L"--allocate-test") && wcscmp(mode, L"--layout") && wcscmp(mode, L"--pause") && wcscmp(mode, L"--status") && wcscmp(mode, L"--bindings")))
        return finish(1, L"Use the visible setup, animation, room and report controls, or --native-playback, --evaluation-check, --mirror, --host-room, --join-room, --copy-invite, --stop-link or --status. Legacy diagnostic commands remain available. Default: --status.");
    wchar_t directory[JC_PATH_CAP], game[JC_PATH_CAP], dll[JC_PATH_CAP];
    unsigned char hash[32];
    if (!jc_paths(NULL, directory, game) || wcslen(directory) + 13 >= JC_PATH_CAP)
        return finish(1, L"Keep JNET.exe and JNET.dll together in the JNET folder directly inside the Journey game folder.");
    wcscpy(dll, directory);
    wcscat(dll, L"\\JNET.dll");
    open_helper_log(directory);
    if (!jc_hash(game, hash) || !jc_executable_size(hash))
        return finish(1, L"Journey.exe is unreadable or unsupported. Use Select .exe to choose the original game or Sound Fix 0.3.1. Nothing was attached.");
    unsigned expected_image_size=jc_executable_size(hash);
    if (!jc_hash(dll, hash) || memcmp(hash, jc_dll_hash, 32))
        return finish(1, L"JNET.dll is missing or does not match this helper. Keep the package files together. Nothing was attached.");
    trace("file_hashes_match", 0, 0);
    if (!wcscmp(mode, L"--check"))
        return finish(0, L"On-disk executable and diagnostic DLL hashes match. No process was opened or modified. Runtime compatibility is not yet verified.");
    if (!wcscmp(mode, L"--load-test")) {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        HMODULE test = LoadLibraryW(dll);
        if (!test) { fail("local_LoadLibraryW", GetLastError(), 0); return failure_message(); }
        const unsigned char *id = (const unsigned char *)(uintptr_t)GetProcAddress(test, "JNETBuildId");
        const unsigned char expected[32] = JC_BUILD_ID;
        int matched = id && jc_readable(id, sizeof(expected)) && !memcmp(id, expected, sizeof(expected));
        FreeLibrary(test);
        if (!matched) { fail("local_build_identity", 0, 0); return failure_message(); }
        trace("local_dll_load_passed", 0, 0);
        return finish(0, L"DLL loaded successfully inside this helper, then unloaded. No game process was opened and no hooks were installed. This confirms local dependencies, not remote attachment. Version " JC_HELPER_VERSION L".");
    }
    DWORD startup_pid=0;
    if (traveler_startup || !wcscmp(mode,L"--startup-watch")) {
        DWORD existing_pid=0;
        HANDLE existing=find_game(game,&existing_pid,1);
        if(!existing){
            wchar_t original[JC_PATH_CAP];wcscpy(original,directory);
            if(!jc_parent(original) || wcslen(original)+13>=JC_PATH_CAP)
                return finish(1,L"Cannot determine the original Journey folder.");
            wcscat(original,L"\\Journey.exe");
            existing=find_game(original,&existing_pid,1);
        }
        if (existing) {
            CloseHandle(existing);
            return finish(1,L"Startup setup requires Journey closed. Quit Journey, click Launch Journet. No attachment was attempted.");
        }
        DWORD launched_pid=0;
        if(traveler_startup) {
            wchar_t working[JC_PATH_CAP],command[JC_PATH_CAP+4];
            wcscpy(working,directory);
            if(!jc_parent(working))return finish(1,L"Cannot determine the Journey folder.");
            int length=swprintf(command,JC_PATH_CAP+4,L"\"%ls\"",game);
            if(length<=0 || length>=JC_PATH_CAP+4)return finish(1,L"Journey path is too long.");
            STARTUPINFOW startup={.cb=sizeof(startup)};PROCESS_INFORMATION child={0};
            if(!CreateProcessW(game,command,NULL,NULL,FALSE,0,NULL,working,&startup,&child)) {
                fail("direct_journey_launch",GetLastError(),0);return failure_message();
            }
            launched_pid=child.dwProcessId;
            CloseHandle(child.hThread);CloseHandle(child.hProcess);
            trace("direct_journey_launch",0,launched_pid);
        }
        startup_pid=wait_for_startup(game);
        if (!startup_pid) return finish(0,L"Startup wait cancelled or expired. No DLL was loaded. Journey may still be running; close it before using Launch Journet again.");
        if(launched_pid && startup_pid!=launched_pid)
            return finish(1,L"Journey changed processes after direct launch. No DLL was loaded. Close Journey before retrying.");
    }
    DWORD pid = 0;
    HANDLE process = find_game(game, &pid, !wcscmp(mode, L"--inspect") || !wcscmp(mode, L"--status") || !wcscmp(mode, L"--layout"));
    if (!process && !wcscmp(mode,L"--status"))
        return finish(0,L"SEPARATE JOURNEY SESSIONS\n\nStart with the first test client and the second test client. With Journey closed, use Launch Journet on each. Enter Desert without a vanilla companion. Once Full object retained appears, click Connect on both. Walk, turn, jump, glide and chirp separately. Dust pauses this session and starts normal departure; wait for cleanup, then Connect to resume. Copy All from both. If that passes, add the third test client and test all three, including one player reconnecting while the other two remain connected.\n\nStatus shows authenticated identities, state age, capture results and packet counts. This legacy diagnostic requires a separately configured local test session; use the World menu for Steam online play. Stay in Desert. Chapter changes pause the connection; use fresh sessions afterward. Shared triggers and three-player gameplay remain unvalidated.");
    if (!process)
        return finish(1, L"Could not identify exactly one accessible Journey process for this installation. Use Launch Journet, enter gameplay, then run this helper inside the same Windows session. Do not use administrator mode.");
    if (startup_pid && pid!=startup_pid) {
        CloseHandle(process);
        return finish(1,L"Journey changed processes during startup. No attachment attempted; restart before retrying.");
    }
    startup_lookup_deadline=startup_pid ? GetTickCount64()+1500 : 0;
    trace("game_process_opened", 0, 0);
    if (!wcscmp(mode, L"--inspect")) {
        uintptr_t entry = remote_export(process, pid, L"kernel32.dll", "LoadLibraryW", 0);
        CloseHandle(process);
        if (!entry) return failure_message();
        return finish(0, L"Resolved LoadLibraryW from the running game's own module exports. Read-only inspection: no DLL loaded, remote thread created or process memory changed. Version " JC_HELPER_VERSION L".");
    }
    MODULEENTRY32W module;
    if (!module_info(pid, L"JNET.dll", dll, &module)) {
        MODULEENTRY32W other;
        if (module_info(pid, L"JNET.dll", NULL, &other)) {
            CloseHandle(process);
            return finish(1, L"A diagnostic DLL from another location is already loaded. Restart Journey first.");
        }
        if (!wcscmp(mode, L"--pause") || !wcscmp(mode, L"--status") || !wcscmp(mode, L"--layout")) {
            CloseHandle(process);
            return finish(0,L"No diagnostic DLL is attached. Close Journey, click Launch Journet and enter Desert. Wait for Full object retained before selecting Active Traveler. This is a local native-actor diagnostic, not an online room. Status never attaches automatically.");
        }
        if (repeat_request || drop_request || active_request || traveler_request || native_playback_request || evaluation_request || mirror_request || !wcscmp(mode,L"--host-room") || !wcscmp(mode,L"--join-room") || !wcscmp(mode,L"--link-trial") || !wcscmp(mode,L"--join-trial")) {
            CloseHandle(process);
            return finish(1,L"This game has no prepared trial. Close Journey and use Launch Journet. Wait for Full object retained before selecting Active Traveler. No DLL was attached and no action was started.");
        }
        trace("reader_compatibility_mode",0,0);
        int installed = inject(process, pid, dll);
        if (installed <= 0 || !module_info(pid, L"JNET.dll", dll, &module)) {
            if (installed > 0) fail("recheck_loaded_dll", module_error, 0);
            CloseHandle(process);
            return failure_message();
        }
    }
    uintptr_t base = (uintptr_t)module.modBaseAddr;
    unsigned char build[32];
    const unsigned char expected_build[32] = JC_BUILD_ID;
    if (JC_NATIVE_OUTPUTS_RVA+16*sizeof(LONG)>module.modBaseSize || JC_NATIVE_PLAYBACK_RVA+48*sizeof(LONG)>module.modBaseSize || JC_COMPANION_FEATURES_RVA+40*sizeof(LONG)>module.modBaseSize || JC_EVALUATION_RVA+40*sizeof(LONG)>module.modBaseSize || JC_SKELETON_RVA+32*sizeof(LONG)>module.modBaseSize || JC_MOTION_RVA+28*sizeof(LONG)>module.modBaseSize || JC_FLIGHT_RVA+JC_FLIGHT_FIELDS*sizeof(LONG)>module.modBaseSize || JC_BODIES_RVA+48*sizeof(LONG)>module.modBaseSize || JC_DUAL_RVA+8*sizeof(LONG)>module.modBaseSize || JC_ROOM_RVA+20*sizeof(LONG)>module.modBaseSize || JC_PERFORMANCE_RVA+30*sizeof(LONG)>module.modBaseSize || JC_LINK_RVA+16*sizeof(LONG)>module.modBaseSize || JC_LINK_CONFIG_RVA+36>module.modBaseSize || JC_BUILD_RVA + sizeof(build) > module.modBaseSize ||
        JC_PEER_LIFECYCLE_RVA + 12*sizeof(LONG) > module.modBaseSize || JC_ACTOR_TRIAL_RVA + 5*sizeof(LONG) > module.modBaseSize || JC_ACTOR_RVA + 12*sizeof(LONG) > module.modBaseSize || JC_STATUS_RVA + sizeof(LONG) > module.modBaseSize || JC_COMMAND_RVA + sizeof(LONG) > module.modBaseSize || JC_INIT_RVA >= module.modBaseSize || JC_CONTROL_RVA >= module.modBaseSize || JC_OBSERVATION_RVA + sizeof(LONG) > module.modBaseSize || JC_PHASE_RVA + sizeof(LONG) > module.modBaseSize || JC_TRAVELER_RVA + 44*sizeof(LONG) > module.modBaseSize || JC_LIFECYCLE_RVA + sizeof(struct jc_lifecycle_ring) > module.modBaseSize || JC_FRAMES_RVA + sizeof(LONG) > module.modBaseSize || JC_GAME_RVA + sizeof(uint64_t) > module.modBaseSize || JC_TRIAL_RVA + sizeof(LONG) > module.modBaseSize || JC_LIFETIME_RVA + sizeof(LONG) > module.modBaseSize || JC_RESOURCE_RVA + sizeof(LONG) > module.modBaseSize || JC_POOL_RVA + 5*sizeof(LONG) > module.modBaseSize || JC_MESH_RVA + 6*sizeof(LONG) > module.modBaseSize || JC_MATERIAL_RVA + sizeof(LONG) > module.modBaseSize || JC_GEOMETRY_RVA + 9*sizeof(LONG) > module.modBaseSize || JC_BUILDER_RVA + 8*sizeof(LONG) > module.modBaseSize || JC_STARTUP_RVA + 5*sizeof(LONG) > module.modBaseSize || JC_COMMIT_RVA + 3*sizeof(LONG) > module.modBaseSize || JC_CALLBACKS_RVA + 11*sizeof(LONG) > module.modBaseSize || JC_POSE_RVA + 3*sizeof(LONG) > module.modBaseSize || JC_POSE_WRITES_RVA + 8*sizeof(LONG) > module.modBaseSize || JC_ANIMATION_RVA + 17*sizeof(LONG) > module.modBaseSize || JC_IDLE_RVA + 5*sizeof(LONG) > module.modBaseSize || JC_RENDER_RVA + 23*sizeof(LONG) > module.modBaseSize || JC_MESH_LIFE_RVA + 152*sizeof(LONG) > module.modBaseSize || JC_APPEARANCE_RVA + 40*sizeof(LONG) > module.modBaseSize ||
        !remote_read(process, base + JC_BUILD_RVA, build, sizeof(build)) || memcmp(build, expected_build, sizeof(build))) {
        CloseHandle(process);
        return finish(1, L"Loaded diagnostic build does not match the helper. Restart Journey and use a matching package.");
    }
    if (!watch_exit_start(process,base,pid,directory)) {
        CloseHandle(process);
        return finish(1,L"Could not start the exit recorder. No new action was queued. Close this helper and reopen it before testing.");
    }
    if (!wcscmp(mode,L"--copy-invite")) {
        int copied=copy_link_invite(process,base); CloseHandle(process);
        return finish(copied ? 0 : 1,copied ? L"Invite copied to clipboard. For a room, paste this SAME invite into Join Room on both other prepared clients. For a two-client Link Trial, use Join Trial (or Send Pose for publish-only). Do not paste it in chat or logs. The link expires after ten minutes; the clipboard is not automatically cleared. Return to Journey between helper actions." : L"No active receiver invite could be copied. Click Host Room or Link Trial after Trial retained, return to gameplay, then Copy Invite.");
    }
    if (!wcscmp(mode,L"--status")) {
        int linked=link_report(process,base,preview_text,20000);
        if (linked>0) {
            preview_process=process; preview_base=base; preview_polls=0;
            wcscpy(preview_last_report,preview_text);
            int result=finish(0,preview_text); preview_process=NULL; preview_base=0;
            CloseHandle(process); return result;
        }
        if (linked<0) { CloseHandle(process); return finish(1,L"Pose link status could not be read. Restart Journey before retrying."); }
    }
    LONG status = 0;
    if (!remote_read(process, base + JC_STATUS_RVA, &status, sizeof(status))) {
        fail("read_initial_status", GetLastError(), 0);
        CloseHandle(process);
        return failure_message();
    }
    LONG observation_mode=0;
    if (!remote_read(process,base+JC_OBSERVATION_RVA,&observation_mode,sizeof(observation_mode))) {
        CloseHandle(process); return finish(1,L"Could not read the loaded observation mode. Restart Journey before retrying.");
    }
    if (status && ((connection_mode && observation_mode!=(lifecycle_mode ? 5 : 4)) || (!connection_mode && (observation_mode==4 || observation_mode==5)))) {
        CloseHandle(process);
        return finish(0,L"Capture modes require separate fresh sessions. Native Lifecycle records native call order; --connection-scan and --cloth-check record state snapshots. Restart Journey before switching between Connection Scan and the legacy ghost-room setup.");
    }
    if (startup_pid) {
        if (status!=0 || !remote_control(process,base,3,1)) {
            CloseHandle(process);
            return finish(1,L"Startup observation could not be armed before initialization. Restart Journey before retrying.");
        }
    }
    if (!wcscmp(mode, L"--layout")) {
        uint64_t captured = 0, after = 0;
        MODULEENTRY32W main_module;
        struct jc_layout_result result;
        if (status != 1 || !remote_read(process, base + JC_GAME_RVA, &captured, sizeof(captured)) || !captured ||
            !module_info(pid, L"Journey.exe", game, &main_module)) {
            CloseHandle(process);
            return finish(1, L"Native capture is not ready. Click Attach, return to gameplay for a frame, then click Native Layout. No game-facing functions were called by this check.");
        }
        int checked = jc_native_layout(layout_read, process, (uint64_t)(uintptr_t)main_module.modBaseAddr, captured, &result);
        struct jc_resource_result resources;
        if (checked == JC_LAYOUT_OK)
            checked = jc_native_resources(layout_read, process, (uint64_t)(uintptr_t)main_module.modBaseAddr, captured, &resources);
        if (checked == JC_LAYOUT_OK && (!remote_read(process, base + JC_GAME_RVA, &after, sizeof(after)) || after != captured))
            checked = JC_LAYOUT_CHANGED;
        CloseHandle(process);
        trace("native_layout_result", 0, (DWORD)checked);
        if (checked != JC_LAYOUT_OK) {
            wchar_t error[512];
            swprintf(error, 512, L"Native layout/resource check declined (code %d). 1=unreadable/missing object; 2=unexpected object type; 3=state changed during the read; 4=invalid range. Return to stable gameplay before retrying. No memory was written, Lua run, or native game getter called.", checked);
            return finish(1, error);
        }
        trace("native_remote_gate", 0, (DWORD)result.remote_gate);
        trace("native_local_position_finite", 0, (DWORD)result.local_position_finite);
        trace("native_remote_position_finite", 0, (DWORD)result.remote_position_finite);
        wchar_t summary[1600];
        swprintf(summary, 1600, L"Native layout and mesh types match the pinned build.\nRemote getter gate: %ls.\nLocal position finite: %ls. Remote position finite: %ls.\n\nAnimation entries: local %u; remote %u.\nAnimation table ranges: %ls.\nController points to its own mesh: local %ls; remote %ls.\n\nRead-only metadata snapshots: no Lua, constructors or game getters invoked. No array contents or native addresses logged. Disjoint ranges support separate animation-entry storage only; they do not prove independent cloth, GPU resources, complete ownership or safe third-traveler creation. Reads are not atomic. No extra traveler was created. Version " JC_HELPER_VERSION L".",
            result.remote_gate ? L"open" : L"closed", result.local_position_finite ? L"yes" : L"no",
            result.remote_gate ? (result.remote_position_finite ? L"yes" : L"no") : L"not read (gate closed)",
            (unsigned)resources.counts[0], (unsigned)resources.counts[1],
            resources.tables_available ? (resources.tables_overlap ? L"OVERLAP" : L"disjoint") : L"not available (one or both empty)",
            resources.controller_owns_mesh[0] ? L"yes" : L"no", resources.controller_owns_mesh[1] ? L"yes" : L"no");
        return finish(0, summary);
    }
    if (!wcscmp(mode, L"--status")) {
        LONG observation = -1, phase = -1, frames = 0, pending = -1, trial = 0, lifetime=0, resource=0, pool[5]={-1,0,0,0,0}, mesh[6]={0}, material=0, geometry[9]={-1,0,0,0,0,-1,-1,-1,0}, builder[8]={0}, startup[5]={0}, commit[3]={-1,0,0}, callbacks[11]={-1,0,0,-1,0,-1,-1,-1,0,0,0}, pose[3]={-1,-1,-1}, pose_writes[8]={-1,-1,0,0,0,0,0,0}, animation[17]={-1,-1}, idle[5]={-1,0,-1,-1,-1}, render[23]={0}, mesh_life[152]={0}, appearance[40]={-1};
        int read_ok = remote_read(process, base + JC_OBSERVATION_RVA, &observation, sizeof(observation));
        read_ok = read_ok && remote_read(process, base + JC_PHASE_RVA, &phase, sizeof(phase)) &&
            remote_read(process, base + JC_FRAMES_RVA, &frames, sizeof(frames)) &&
            remote_read(process, base + JC_COMMAND_RVA, &pending, sizeof(pending)) &&
            remote_read(process, base + JC_TRIAL_RVA, &trial, sizeof(trial)) &&
            remote_read(process, base + JC_LIFETIME_RVA, &lifetime, sizeof(lifetime)) &&
            remote_read(process, base + JC_RESOURCE_RVA, &resource, sizeof(resource)) &&
            remote_read(process, base + JC_POOL_RVA, pool, sizeof(pool)) &&
            remote_read(process, base + JC_MESH_RVA, mesh, sizeof(mesh)) &&
            remote_read(process, base + JC_MATERIAL_RVA, &material, sizeof(material)) &&
            remote_read(process, base + JC_GEOMETRY_RVA, geometry, sizeof(geometry)) &&
            remote_read(process, base + JC_BUILDER_RVA, builder, sizeof(builder)) &&
            remote_read(process, base + JC_STARTUP_RVA, startup, sizeof(startup)) &&
            remote_read(process, base + JC_COMMIT_RVA, commit, sizeof(commit)) &&
            remote_read(process, base + JC_CALLBACKS_RVA, callbacks, sizeof(callbacks)) &&
            remote_read(process, base + JC_POSE_RVA, pose, sizeof(pose)) &&
            remote_read(process, base + JC_POSE_WRITES_RVA, pose_writes, sizeof(pose_writes)) &&
            remote_read(process, base + JC_ANIMATION_RVA, animation, sizeof(animation)) &&
            remote_read(process, base + JC_IDLE_RVA, idle, sizeof(idle)) &&
            remote_read(process, base + JC_RENDER_RVA, render, sizeof(render)) &&
            remote_read(process, base + JC_MESH_LIFE_RVA, mesh_life, sizeof(mesh_life)) &&
            remote_read(process, base + JC_APPEARANCE_RVA, appearance, sizeof(appearance));
        trace("snapshot_phase", 0, (DWORD)phase);
        trace("snapshot_frames", 0, (DWORD)frames);
        trace("snapshot_pending", 0, (DWORD)pending);
        if (read_ok && startup[0] && (resource || pending==7)) {
            wchar_t text[4000],state[256]; LONG dual[8];
            if (!remote_read(process,base+JC_DUAL_RVA,dual,sizeof(dual))) {
                CloseHandle(process); return finish(1,L"Could not read two-body preparation. Retry Status.");
            }
            if (dual[0]==1 && resource==5)
                swprintf(state,256,L"Trial retained in %ls. Both bodies ready. Native Companions evaluates both bodies; Evaluation Check inspects storage only.",level_label(mesh_life[108]));
            else if (resource<0 || dual[0]<0) wcscpy(state,L"Preparation failed. Copy this report and restart Journey.");
            else if (resource==1 || resource==2) wcscpy(state,L"Armed: use chapter select, then the Desert portal. Both bodies are created during native resource initialization.");
            else wcscpy(state,L"Waiting for game updates to finish two-body preparation.");
            int n=swprintf(text,4000,L"JNET " JC_HELPER_VERSION L"\nVisible Trial: %ls\nResource %ld. Pending command %ld. Game update %ld.\n"
                L"Capacity request/attempt/compiled/result/validated: %ld / %ld / %ld / %ld / %ld.\n"
                L"Preparation level: %ls. Chapter-select initializations skipped: %ld. Decline reason: %ld.\n\n",
                state,(long)resource,(long)pending,(long)frames,(long)startup[0],(long)startup[1],(long)startup[2],(long)startup[3],(long)startup[4],
                level_label(mesh_life[94]),(long)mesh_life[95],(long)mesh_life[107]);
            if (n<=0 || n>=4000 || body_report(process,base,text+n,4000-(size_t)n)<0) {
                CloseHandle(process); return finish(1,L"Two-body report could not be read/formatted. Retry Status.");
            }
            CloseHandle(process); return finish(resource<0 || dual[0]<0 ? 1 : 0,text);
        }
        if (read_ok && (mesh_life[25] || (startup[0] && resource))) {
            wchar_t text[7500],retained[192];
            swprintf(retained,192,L"Trial retained in %ls. Move to open ground, then Show Trial and switch back.",level_label(mesh_life[108]));
            const wchar_t *reasons[]={L"none (other native checks can still decline)",L"level unreadable/unknown/changed",
                L"preparation timed out",L"chapter outside this test's eligible list",L"chapter-select skip limit reached"};
            const wchar_t *reason=mesh_life[107]>=0 && mesh_life[107]<5 ? reasons[mesh_life[107]] : L"unknown";
            const wchar_t *input_fields[]={L"none",L"global weight threshold",L"model/root ownership",L"animation tables/reference count",
                L"mesh animation scale",L"animation entry read",L"animation entry index",L"animation activity/weight",
                L"animation input floats",L"descriptor/state storage",L"trial root transform",L"matrix table/count",
                L"matrix source",L"matrix destination",L"bounds source",L"ownership recheck"};
            const wchar_t *input_field=mesh_life[111]>=0 && mesh_life[111]<16 ? input_fields[mesh_life[111]] : L"unknown";
            const wchar_t *state=mesh_life[25]<0 || mesh_life[0]<0 || resource<0 ? L"Stopped or failed; restart required." :
                mesh_life[25]==3 ? L"Bounded draw window ended; copy the result and quit Journey." :
                mesh_life[25]==2 ? L"Drawing window active. Return to gameplay and look around." :
                mesh_life[25]==1 ? L"Five-second delay / waiting for render callback. Switch back to Journey now." :
                mesh_life[0]==1 ? retained :
                resource==1 && mesh_life[95] ? L"Armed: chapter select skipped. Take the Level_Desert portal, then Status." :
                resource==1 ? L"Armed: go to chapter select, then take the Level_Desert portal back." :
                L"Waiting for chapter initialization. Return to gameplay, then Status.";
            const wchar_t *layout_fields[]={L"none",L"material chain",L"variant",L"instance count",
                L"instance member",L"buffer size",L"descriptor count",L"descriptor/name",L"parameter type",
                L"parameter width",L"storage overlap",L"ownership reread"};
            const wchar_t *parameter_names[]={L"specular",L"alphaHardness",L"localDudeGlow",L"remoteDudeGlow",
                L"textureAtlasA",L"textureAtlasB",L"textureDetail",L"dudeDecoGlow",L"textureMask",
                L"hsvHue",L"hsvSaturation",L"hsvValue",L"envOccCube"};
            const wchar_t *layout_field=appearance[11]>=0 && appearance[11]<12 ? layout_fields[appearance[11]] : L"unknown";
            const wchar_t *parameter=appearance[12]>=0 && appearance[12]<13 ? parameter_names[appearance[12]] : L"none";
            swprintf(text,7500,
                L"JNET " JC_HELPER_VERSION L". Loader: %ld. Update: %ld.\nVisible Trial: %ls\n\n"
                L"CHAPTER\nSelected: %ls. At Show Trial: %ls. Read code: %ld. Hub skips: %ld.\n"
                L"Preparation rejection: %ls. Native preparation/allocation: %ld / %ld.\n\n"
                L"APPEARANCE PREPARATION\nResult: %ld. Stage: %ld. Setter calls: %ld. Check: %ld. Time: %ld ms.\n"
                L"Result: -1 pending; 0 accepted; 1 read; 2 type; 3 changed; 4 range/width; 5 alias; 6 source binding; 7 setter; 8 postcheck; 9 budget.\n"
                L"Stage: 0 no writes; 1 setters started; 2 setters returned; 3 postchecks passed.\n"
                L"Check: 1 definition; 2 local; 3 remote; 4 trial; 5 inputs; 6 setters; 7 original buffers; 8 trial buffer.\n"
                L"Selected original material buffers unchanged: %ld (1 yes). Parameter masks: source %ld; trial matching before %ld; after %ld.\n"
                L"Layout field: %ls. Parameter: %ls. Variant: %ld. List entries: %ld (cap 16). Buffer bytes: %ld. Unique instances: %ld.\n"
                L"Descriptor table/row: %ld / %ld. Offset / declared / required width: %ld / %ld / %ld.\n"
                L"Last inspected metadata, not live values. Widths describe a width rejection only. Setters select member zero; other buffers are checked for ownership and overlap.\n"
                L"Source validation: %ls. Parameter: %ls. Resource code: %ld (-1 pending; 0 valid; 1 read; 2 type; 3 changed/mismatch; 4 range).\n"
                L"Found source/trial: %ld / %ld. Source matches definition: %ld. Missing source/trial textures: %ld / %ld. Texture differences from definition: %ld.\n"
                L"Masks use parameter indices 0-12 in the thirteen-row appearance schema. Differences are allowed only after independent live-resource validation.\n"
                L"Validated cube cached/current slot/count: %ld / %ld / %ld. Refreshed-index mask: %ld. Valid only after appearance result 0. Slots are renderer resources, not player identities.\n"
                L"Borrowed texture check: %ld (0 accepted); passed checks: %ld. No resource ownership acquired.\n\n"
                L"RENDER AND PLACEMENT\nState: %ld. Validated consumer calls: %ld / 600. Failure: %ld. Call stage: %ld. Delay: %ld s.\n"
                L"Failures 20/21: appearance preparation/resource change; 22: torso shader; 15: checkpoint; 19: chapter. Restart on failure.\n"
                L"Checkpoint attempts/error: %ld / %ld. Bounds/dispatch/builder returns: %ld / %ld / %ld.\n"
                L"Renderer path: %ld. Executor state: %ld (3 returned; does not prove GPU output).\n"
                L"Bounds code: %ld. Center relative to captured player: %.3f / %.3f / %.3f. Size: %.3f / %.3f / %.3f.\n"
                L"Bounds meaningful only at code 0; intended placement +3 world X, stationary, not camera-relative.\n"
                L"Pose preflight: %ld; step %ld; field %ls; index %ld; %ld ms. Root matches: %ld.\n"
                L"Pose snapshot: %ld. Matrices/non-finite/singular: local %ld/%ld/%ld; trial %ld/%ld/%ld.\n\n"
                L"TRIAL TORSO SHADER\nResult: %ld. Step: %ld (1 spine; 2 writes; 3 spine recheck). Stage: %ld (0 no writes; 1 started; 2 returned; 3 postchecks).\n"
                L"Original buffers unchanged: %ld. Trial-only changes: %ld. Time: %ld ms. Result: -1 pending; 0 accepted; 1 read; 2 missing/type; 3 changed; 4 width/non-finite/singular; 5 tree cap/cycle; 6 overlap/active input; 7 write; 8 postcheck; 9 budget.\n"
                L"SHADER SNAPSHOTS\nCodes local/trial: %ld / %ld. Found/finite/zero masks: local %ld/%ld/%ld; trial %ld/%ld/%ld.\n"
                L"Bits 1 dissolve, 2 center, 4 inverse torso. Values require code 0. CPU snapshots only. CPU torso preparation does not establish skinning uploads or final render-pass correctness.\n\n"
                L"Capacity Test before launch; Continue into Desert; Prepare Trial; return to gameplay; Status until red Armed; chapter select then Desert portal; Status until Trial retained. On open ground click Show Trial and switch back immediately (five-second delay).\n"
                L"Look around, Status / Copy all, then quit Journey. Do not change chapters after Show Trial. Resources stay allocated until exit. No third-client networking, cloth or active animation. Original files unchanged. Close does not remove hooks.",
                (long)status,(long)frames,state,level_label(mesh_life[108]),level_label(mesh_life[96]),
                (long)mesh_life[97],(long)mesh_life[95],reason,(long)resource,(long)trial,
                (long)appearance[0],(long)appearance[1],(long)appearance[2],(long)appearance[7],(long)appearance[8],
                (long)appearance[3],(long)appearance[4],(long)appearance[5],(long)appearance[6],
                layout_field,parameter,(long)appearance[15],(long)appearance[16],(long)appearance[17],(long)appearance[21],
                (long)appearance[13],(long)appearance[14],(long)appearance[18],(long)appearance[19],(long)appearance[20],
                appearance_source_reason(appearance[22]),appearance_parameter(appearance[23]),(long)appearance[24],
                (long)appearance[4],(long)appearance[25],(long)appearance[26],(long)appearance[27],(long)appearance[28],(long)appearance[29],
                (long)appearance[31],(long)appearance[32],(long)appearance[33],(long)appearance[30],
                (long)appearance[9],(long)appearance[10],
                (long)mesh_life[25],(long)mesh_life[26],(long)mesh_life[27],(long)mesh_life[28],(long)mesh_life[109],
                (long)mesh_life[115],(long)mesh_life[116],(long)mesh_life[43],(long)mesh_life[45],(long)mesh_life[54],
                (long)mesh_life[84],(long)mesh_life[89],(long)mesh_life[98],
                mesh_life[99]/1000.0,mesh_life[100]/1000.0,mesh_life[101]/1000.0,
                mesh_life[102]/1000.0,mesh_life[103]/1000.0,mesh_life[104]/1000.0,
                (long)mesh_life[110],(long)mesh_life[113],input_field,(long)mesh_life[112],(long)mesh_life[114],(long)mesh_life[40],
                (long)mesh_life[58],(long)mesh_life[59],(long)mesh_life[60],(long)mesh_life[61],
                (long)mesh_life[62],(long)mesh_life[63],(long)mesh_life[64],
                (long)appearance[34],(long)appearance[35],(long)appearance[36],(long)appearance[37],(long)appearance[38],(long)appearance[39],
                (long)mesh_life[138],(long)mesh_life[144],(long)mesh_life[139],(long)mesh_life[140],(long)mesh_life[141],
                (long)mesh_life[145],(long)mesh_life[146],(long)mesh_life[147]);
            CloseHandle(process); return finish(mesh_life[25]<0 || mesh_life[0]<0 || resource<0 ? 1 : 0,text);
        }
        if (read_ok && (mesh_life[0] || (startup[0] && resource))) {
            wchar_t text[3800];
            const wchar_t *fields[]={L"none",L"object",L"clone chain",L"geometry source",L"geometry slot",L"model",L"control",L"animation entries",L"animation count",L"manager",L"material cache",L"source-list head",L"clone model/control pair",L"geometry pool",L"lifetime state",L"game",L"thread",L"PlayerBarn",L"post-cleanup allocation",L"ownership lock",L"restoration",L"checkpoint",L"read budget"};
            const wchar_t *phases[]={L"not started",L"initial capture",L"update ownership check",L"pre-cleanup validation",L"native cleanup entered",L"native cleanup returned",L"deleting destructor entered",L"deleting destructor returned"};
            const wchar_t *field=mesh_life[4]>=0 && mesh_life[4]<23 ? fields[mesh_life[4]] : L"unknown";
            const wchar_t *where=mesh_life[5]>=0 && mesh_life[5]<8 ? phases[mesh_life[5]] : L"unknown";
            const wchar_t *remaining=mesh_life[21]>=0 && mesh_life[21]<23 ? fields[mesh_life[21]] : L"unknown";
            const wchar_t *stage=mesh_life[0]<0 || resource<0 ? L"Failed or quarantined; restart required." :
                mesh_life[0]==1 ? L"Object retained: initialized, undrawn. Wait a few seconds, then use chapter select again." :
                mesh_life[0]==2 ? L"Completed: native mesh cleanup/deletion returned before original chapter cleanup." :
                mesh_life[0]==3 ? L"Cleanup validation or native cleanup in progress." :
                resource==1 ? L"Armed: use chapter select once to create the trial." : L"Waiting for initialization.";
            swprintf(text,3800,L"Loader status: %ld. Update counter: %ld.\nMesh Lifetime: %ls\nLifetime state: %ld (1 retained; 2 released; 3 cleaning; negative failed).\nOwnership checks across updates: %ld / 10. Updates retained: %ld.\nFailure code: %ld (0 none; 1 read; 2 type/model; 3 changed/cleanup validation; 4 range; 9 read budget; 10 checkpoint; 12 restoration).\nValidation field: %ls. Last stage: %ls.\nValidation pool: valid %ld; slots %ld; occupied %ld; free %ld. Counts meaningful only when valid is 1; this is the last validation snapshot, not a live pool read. Status fields are not atomic.\nSlot comparison valid: %ld. Same source/table: %ld. Changed-slot bitmask: %ld (slots 0/1/2 = bits 1/2/4).\nSlot heads at retention: %ld / %ld / %ld.\nSlot heads at validation: %ld / %ld / %ld.\nLabels: 0 empty; 1 local primary; 2 remote primary; 3 trial; 4 other; 5/6 local optional meshes; 7/8 remote optional meshes. Pointer-value matches only; not player identities or complete ownership. Compare slots only when valid and same source/table are both 1.\nRemaining trial comparison: valid %ld; code %ld; field %ls. Only meaningful when valid is 1. Chapter cleanup permits only the observed slot-0 departure with all trial fields matching.\nSlot metadata at retention: %ld / %ld / %ld.\nKinds: 0 empty/not checked; 1 property types/backlinks match; 2 also enclosing mesh type matches; 3 different property type; 4 changed/backlink mismatch; 5 unreadable/range. Capture only; no freed heads are read later.\n\nDistinct third geometry slot: %ld. Material commit stage: %ld (3 cache matched).\nInitial idle CPU update code/stage: %ld / %ld (0 / 3 accepted).\nCleanup cache cleared: %ld. Geometry pool restored: %ld. Free slots after cleanup: %ld.\nCallback registrations restored: %ld. Allocation/deletion result: %ld.\nRestoration fields are pending until cleanup; -1 means unavailable.\n\nNo explicit draw, shared render-queue submission or networking. The initialized mesh remains allocated until the next matching local chapter cleanup. At most ten ownership checks, one per 60 updates, plus a required pre-cleanup check. Background-paused frames do not advance checks. If any check fails, restart; uncertain ownership is never blindly freed.\n\nUse Capacity Test before starting fresh Journey, enter solo gameplay, then Mesh Lifetime. Return to gameplay, Status until red Armed, chapter select once. When Object retained appears, play a few seconds, then chapter select again. Status / Copy all afterward. Restart to remove hooks. Original files remain unchanged.",
                (long)status,(long)frames,stage,(long)mesh_life[0],(long)mesh_life[1],(long)mesh_life[3],(long)mesh_life[2],field,where,(long)mesh_life[9],(long)mesh_life[6],(long)mesh_life[7],(long)mesh_life[8],
                (long)mesh_life[10],(long)mesh_life[11],(long)mesh_life[12],
                (long)mesh_life[13],(long)mesh_life[14],(long)mesh_life[15],
                (long)mesh_life[16],(long)mesh_life[17],(long)mesh_life[18],
                (long)mesh_life[19],(long)mesh_life[20],remaining,(long)mesh_life[22],(long)mesh_life[23],(long)mesh_life[24],
                (long)geometry[6],(long)commit[1],(long)idle[0],(long)idle[1],(long)commit[2],(long)geometry[7],(long)geometry[8],(long)callbacks[7],(long)trial);
            CloseHandle(process);
            return finish(mesh_life[0]<0 || resource<0 ? 1 : 0,text);
        }
        if (read_ok && render[0]) {
            wchar_t text[2100];
            swprintf(text,2100,L"Loader status: %ld. Update counter: %ld.\nChapter Refs: %ls\nState: %ld. Snapshot code: %ld (0 accepted; 1 read; 2 type/pair; 3 changed ownership; 4 range; 9 read budget; 10 timeout; 11 mixed experiments).\n\nQueue count / active original-local matches / all stored original-local matches:\nAfter traveler submission: %ld / %ld / %ld\nBefore matching consumer: %ld / %ld / %ld\nAfter matching consumer: %ld / %ld / %ld\nAt update return: %ld / %ld / %ld\nBefore local traveler cleanup: %ld / %ld / %ld\nAfter local traveler cleanup: %ld / %ld / %ld\nAfter next traveler submission: %ld / %ld / %ld\n\nOnly completed stages are meaningful: state 6 first four snapshots, 8 cleanup snapshots, 9 all seven. Stored pairs are pointer-value matches, not proof of live ownership, stable identity or GPU completion. Snapshots are not atomic; original model/control pointers are never dereferenced after cleanup.\n\nStart solo without Capacity Test. Click Chapter Refs, return to gameplay, then Status until red Armed. Use chapter select once, enter gameplay, then Status / Copy all. Waiting expires after ten minutes from arming, checked on game updates. No trial object, capacity change, added draw, Lua sampling or networking. Restart afterward; Close does not remove hooks.",
                (long)status,(long)frames,render[0]==6 ? L"Armed: use chapter select once." : render[0]==9 ? L"Completed (state 9)." : render[0]<0 ? L"Declined or failed; restart required." : L"In progress; return to gameplay.",
                (long)render[0],(long)render[1],(long)render[2],(long)render[3],(long)render[4],(long)render[5],(long)render[6],(long)render[7],(long)render[8],(long)render[9],(long)render[10],(long)render[11],(long)render[12],(long)render[13],(long)render[14],(long)render[15],(long)render[16],(long)render[17],(long)render[18],(long)render[19],(long)render[20],(long)render[21],(long)render[22]);
            CloseHandle(process);
            return finish(render[0]<0 ? 1 : 0,text);
        }
        struct jc_geometry_pool pools[2]={{0},{0}};
        struct jc_geometry_build_state builds[2]={{0},{0}};
        int pool_codes[2]={JC_LAYOUT_READ,JC_LAYOUT_READ};
        if (startup[0] && read_ok) {
            uint64_t captured=0,barn=0,object=0,after=0;
            MODULEENTRY32W main_module;
            struct jc_layout_result layout;
            if (remote_read(process,base+JC_GAME_RVA,&captured,sizeof(captured)) && captured &&
                module_info(pid,L"Journey.exe",game,&main_module) &&
                !jc_native_layout(layout_read,process,(uintptr_t)main_module.modBaseAddr,captured,&layout) &&
                jc_layout_field(layout_read,process,captured,0x1e0,&barn,8)) {
                const uint64_t offsets[2]={0xb0,0x146f0};
                for (unsigned i=0;i<2;++i) {
                    pool_codes[i]=jc_geometry_pool_read(layout_read,process,barn+offsets[i]+0x160+0x80,&pools[i]);
                    if (!pool_codes[i] && !jc_layout_field(layout_read,process,pools[i].source,0x10,&object,8)) pool_codes[i]=JC_LAYOUT_READ;
                    if (!pool_codes[i]) pool_codes[i]=jc_geometry_build_read(layout_read,process,(uintptr_t)main_module.modBaseAddr,object,&builds[i]);
                    if (!pool_codes[i] && (!jc_layout_field(layout_read,process,pools[i].source,0x10,&after,8) || after!=object ||
                        builds[i].slots!=pools[i].slots || builds[i].occupied!=pools[i].occupied)) pool_codes[i]=JC_LAYOUT_CHANGED;
                }
                if (!remote_read(process,base+JC_GAME_RVA,&after,8) || after!=captured ||
                    !jc_layout_field(layout_read,process,captured,0x1e0,&after,8) || after!=barn)
                    pool_codes[0]=pool_codes[1]=JC_LAYOUT_CHANGED;
            }
        }
        CloseHandle(process);
        if (!read_ok || observation < 0 || observation > 8)
            return finish(1, L"Could not read a valid observation status. Nothing was changed.");
        if (startup[0]) {
            wchar_t text[3800];
            const wchar_t *outcome=startup[4]<0 ? L"Candidate validation/repetition check failed; restart required." :
                startup[2] ? (startup[3]==0 && startup[4]==1 ? L"Temporary capacity buffer compiled successfully." : L"Compiler returned an error; capacity not established.") :
                startup[1] ? L"Startup call entered; return not yet observed." : L"NOT APPLIED: startup interception not observed.";
            swprintf(text,3800,L"Loader status: %ld. Update counter: %ld.\nCapacity Test: %ls\nMatching loads: %ld. Return observed: %ld. Compiler result: %ld. Candidate state: %ld (1 validated; negative failed).\n\nLocal pool: code %d; limit %u; slots %u; occupied %u; free %u.\nRemote pool: code %d; limit %u; slots %u; occupied %u; free %u.\nPool values are valid only for code 0; other codes mean unavailable or inconsistent snapshots.\n\nMesh Lifetime: %ls\nTrial owns distinct third geometry slot: %ld (1 yes; 0 failed; -1 not checked).\nPool restored after cleanup: %ld (1 yes; 0 failed; -1 not checked). Post-cleanup free slots: %ld.\nMaterial geometry prefix: %ld (1 present). Allocation/deletion result: %ld.\nCommit input code: %ld (0 OK). Commit stage: %ld (1 entered; 2 returned; 3 cache matched). Cache cleared after cleanup: %ld (1 yes).\nCallback snapshots (before/during/after): %ld / %ld / %ld.\nCallback code 0 means accepted; counts unavailable otherwise.\nNodes visited (before/during/after): %ld / %ld / %ld.\nCounts are unavailable when their snapshot code is nonzero.\nOriginal callback counts (local/remote): %ld / %ld. Trial count: %ld.\nTrial callback types/counts match remote: %ld (1 yes). Registrations restored: %ld (1 yes).\n\nPose storage code: %ld (0 valid; 1 read; 2 missing/type; 3 changed; 4 range).\nModel prefixes disjoint: %ld. Root-transform prefixes disjoint: %ld.\nFlags: 1 yes; 0 overlap; -1 unavailable.\n\nMatrix destination code: %ld (0 valid; 1 read; 2 type; 3 changed; 4 range; 5 cap; 8 scratch; 9 time).\nMatrix counts (64-byte / 48-byte): local %ld / %ld; remote %ld / %ld; trial %ld / %ld.\nRoot and matrix destinations disjoint between travelers: %ld (1 yes; 0 overlap; -1 unavailable).\nCounts valid only for code 0. Zero counts do not establish initialized animation. Matrix inspection is read-only.\n\nAnimation storage code: %ld (same codes as matrix inspection).\nCounts (states / nodes A / nodes B / weights A / weights B):\nLocal: %ld / %ld / %ld / %ld / %ld.\nRemote: %ld / %ld / %ld / %ld / %ld.\nTrial: %ld / %ld / %ld / %ld / %ld.\nAnimation ranges disjoint between travelers: %ld (1 yes; 0 overlap; -1 unavailable).\nCounts valid only for code 0. This does not establish safe animation execution.\n\nIdle update input/result code: %ld.\nCodes: 0 accepted; 1 read; 2 type; 3 changed; 4 range; 5 count/reference; 6 active input; 7 index; 8 non-finite/unreadable input; 9 budget; 10 allocation; 11 unexpected references; 12 post-check; 13 prior check.\nCall stage: %ld (0 not called; 1 entered; 2 returned; 3 context released).\nQueued references: %ld. Reference count restored: %ld. Root matches input: %ld.\nFlags: 1 yes; 0 failed; -1 unavailable. One inactive-weight CPU update; no draw or active animation requested.\n\nOriginal files remain unchanged. One material commit call; no explicit draw or new networking. Ownership snapshots do not prove GPU independence. When capacity is verified, click Prepare Trial and follow its chapter-select then portal-return instructions.",
                (long)status,(long)frames,outcome,(long)startup[1],(long)startup[2],(long)startup[3],(long)startup[4],
                pool_codes[0],(unsigned)builds[0].limit,(unsigned)pools[0].slots,(unsigned)pools[0].occupied,(unsigned)pools[0].free_slots,
                pool_codes[1],(unsigned)builds[1].limit,(unsigned)pools[1].slots,(unsigned)pools[1].occupied,(unsigned)pools[1].free_slots,
                resource==0 ? L"Not requested." : resource==1 ? L"Armed: use chapter select." : resource==2 ? L"Waiting for remote initialization." : resource==3 ? L"In progress." : resource==4 ? L"Completed; idle CPU update and cleanup returned." : L"Declined or failed; restart required.",
                (long)geometry[6],(long)geometry[7],(long)geometry[8],(long)material,(long)trial,(long)commit[0],(long)commit[1],(long)commit[2],
                (long)callbacks[0],(long)callbacks[3],(long)callbacks[6],(long)callbacks[8],(long)callbacks[9],(long)callbacks[10],(long)callbacks[1],(long)callbacks[2],(long)callbacks[4],(long)callbacks[5],(long)callbacks[7],(long)pose[0],(long)pose[1],(long)pose[2],(long)pose_writes[0],(long)pose_writes[2],(long)pose_writes[3],(long)pose_writes[4],(long)pose_writes[5],(long)pose_writes[6],(long)pose_writes[7],(long)pose_writes[1],(long)animation[0],(long)animation[2],(long)animation[3],(long)animation[4],(long)animation[5],(long)animation[6],(long)animation[7],(long)animation[8],(long)animation[9],(long)animation[10],(long)animation[11],(long)animation[12],(long)animation[13],(long)animation[14],(long)animation[15],(long)animation[16],(long)animation[1],(long)idle[0],(long)idle[1],(long)idle[2],(long)idle[3],(long)idle[4]);
            return finish(status<0 || startup[4]<0 ? 1 : 0,text);
        }
        if (builder[1] || builder[6]) {
            wchar_t summary[1600];
            swprintf(summary,1600,L"Loader status: %ld. Pending command: %ld.\nGeometry Build Check: %ls\nMatching native build calls: %ld.\nBaseline, or latest build entry: instanceLimit %ld; slots %ld; occupied %ld.\nLatest build return: slots %ld; occupied %ld (zero is not observed if calls are zero).\nSnapshot code: %ld (0 OK; 1 read; 2 type/ownership; 3 changed; 4 range; 5 observation cap).\nResource at next remote initialization: %ls.\n\nNo third object was allocated and no pool count or resource was changed by this test. Only the captured resource is monitored; zero calls does not rule out rebuilding a replacement object. Copy this result and restart Journey.",
                (long)status,(long)pending,resource==1 ? L"Armed: select chapter select, wait, then Status." : resource==2 ?
                L"Waiting for remote resource initialization." : resource==4 ? L"Boundary observation complete." : L"Check declined or logging/context failed.",
                (long)builder[0],(long)builder[1],(long)builder[2],(long)builder[3],(long)builder[4],(long)builder[5],(long)builder[6],
                builder[7]==1 ? L"same captured object" : builder[7]==2 ? L"different object" : L"not compared yet");
            return finish(status<0 ? 1 : 0,summary);
        }
        const wchar_t *states[] = {
            L"Idle: no observation requested.",
            L"Armed: waiting for a Lua Tick. Return to gameplay.",
            L"Sampling: up to 15 samples in this window, one per 60 Lua Ticks.",
            L"Paused: no further samples. Click Watch for another bounded window.",
            L"Window complete: click Watch for another bounded window.",
            L"Session budget exhausted: restart Journey for more observations.",
            L"Observation failed: inspect the log and restart Journey before retrying.",
            L"Tick ownership changed: observation stopped; restart Journey.",
            L"Chapter changed: observation stopped. Click Watch after loading finishes."
        };
        const wchar_t *phase_name = L"Unknown";
        switch (phase) {
        case 0: phase_name = L"Between update callbacks"; break;
        case 1: phase_name = L"Original game update (includes Lua Tick)"; break;
        case 2: phase_name = L"Original update returned"; break;
        case 3: phase_name = L"Lua context check"; break;
        case 4: phase_name = L"Helper command dispatch"; break;
        case 10: phase_name = L"Sample entry"; break;
        case 11: phase_name = L"Chapter getter"; break;
        case 12: phase_name = L"Player container getter"; break;
        case 13: phase_name = L"Local traveler getter"; break;
        case 14: phase_name = L"Remote traveler getter"; break;
        case 15: phase_name = L"Local position getter"; break;
        case 16: phase_name = L"Remote position getter"; break;
        case 17: phase_name = L"Sample output"; break;
        case 18: phase_name = L"Sample finished"; break;
        case 20: phase_name = L"Native allocation/constructor rehearsal"; break;
        case 22: phase_name=L"Mesh source attachment"; break;
        case 23: phase_name=L"Mesh initialization"; break;
        case 30: phase_name=L"Material property clear"; break;
        case 31: phase_name=L"Material clone reference"; break;
        case 32: phase_name=L"Material property assignment"; break;
        case 33: phase_name=L"Material temporary reference release"; break;
        case 34: phase_name=L"Material geometry binding"; break;
        case 36: phase_name=L"Geometry source/pool observation"; break;
        case 35: phase_name=L"Material geometry-prefix observation"; break;
        case 24: phase_name=L"Mesh cleanup"; break;
        case 21: phase_name = L"Native deleting destructor rehearsal"; break;
        }
        const wchar_t *trial_text = L"Not requested.";
        switch (trial) {
        case 1: trial_text=L"Factory entered; validation/release not complete."; break;
        case 3: trial_text=L"Deleting destructor entered; return not confirmed."; break;
        case 4: trial_text=L"Validated object retained for the chapter lifetime test; no graphics or networking."; break;
        case 2: trial_text=L"Factory, constructor validation and deleting destructor returned. No visible traveler was created."; break;
        case -1: trial_text=L"Declined without allocation: use fresh native-only solo gameplay with valid resources and unchanged code."; break;
        case -2: trial_text=L"Factory returned null. Restart Journey before further diagnostics."; break;
        case -3: trial_text=L"Unexpected object/pointer: not blindly freed. Restart Journey; do not retry."; break;
        case -4: trial_text=L"PlayerBarn changed across rehearsal. Restart Journey; do not retry."; break;
        }
        if (lifetime==2 && trial==3)
            trial_text=L"Deleting destructor returned; confirmed by completed chapter cleanup.";
        const wchar_t *lifetime_text=lifetime==0 ? L"Not active." : lifetime==1 ?
            L"Object retained. Change chapters using chapter select, then return here and click Status." : lifetime==2 ?
            L"Object released at native local traveler cleanup. No visible traveler was created." : lifetime==3 ?
            L"Deleting destructor entered; return not yet confirmed." :
            L"Ownership or thread check failed; restart Journey. No retry in this process.";
        const wchar_t *resource_text=resource==0 ? L"Not requested." : resource==1 ?
            L"Armed. Use chapter select, then enter a playable chapter and click Status." : resource==2 ?
            L"Local cleanup observed. Enter a playable chapter, then click Status." : resource==3 ?
            L"Experimental native calls in progress; cleanup not confirmed." : resource==4 ?
            L"Material preparation returned; trial resources inspected, cleaned and deleted; render commit omitted; three distinct clone prefixes and animation ranges observed." : resource==5 ?
            L"Unexpected state; restart Journey." :
            resource==-2 ? L"Checkpoint logging failed or budget exhausted. Restart Journey." :
            L"Context, callback or resource validation failed; restart Journey. Check allocation result and log.";
        wchar_t pool_text[500];
        if (pool[0]==0 && pool[1]>=2 && pool[1]<=64 && pool[2]>=pool[1] && pool[2]<=64 &&
            pool[3]>=2 && pool[4]>=0 && pool[3]+pool[4]==pool[1])
            swprintf(pool_text,500,L"Pool before rehearsal: %ld slots; capacity field %ld; %ld occupied; %ld free.\n%ls",
                (long)pool[1],(long)pool[2],(long)pool[3],(long)pool[4],pool[4]==0 ?
                L"Exhausted pool observed before the guarded attempt." :
                L"A free slot was observed. This does not establish safe third-mesh attachment.");
        else if (resource==4) swprintf(pool_text,500,L"Pool snapshot declined (code %ld): 1 unreadable, 2 slot ownership mismatch, 3 changed, 4 range/cap rejected. No trial object allocated.",(long)pool[0]);
        else swprintf(pool_text,500,L"Pool snapshot not complete.");
        const wchar_t *material_text=material==0 ? L"Not reached." : material==1 ?
            L"Preparation returned; geometry prefix present. Render commit omitted; readiness unproven." : material==2 ?
            L"Preparation returned; geometry prefix unavailable. Render commit omitted." :
            L"Preparation incomplete. Last flushed checkpoint identifies the observed stage.";
        wchar_t geometry_text[600];
        if (geometry[0]==0 && geometry[1]>=1 && geometry[1]<=64 && geometry[2]>=1 &&
            geometry[3]>=0 && geometry[2]+geometry[3]==geometry[1])
            swprintf(geometry_text,600,L"Remote geometry pool before binding: %ld slots; %ld occupied; %ld free.\nTrial geometry source: %ls. Trial geometry slot: %ls.",
                (long)geometry[1],(long)geometry[2],(long)geometry[3],geometry[4]==1 ? L"same as original remote" :
                geometry[4]==2 ? L"different from original remote" : geometry[4]==3 ? L"read failed" : L"not bound/not observed",
                geometry[5]==1 ? L"present" : geometry[5]==0 ? L"absent" : L"not observed/read failed");
        else swprintf(geometry_text,600,L"Geometry pool snapshot code: %ld (-1 not reached; 1 unreadable; 2 ownership mismatch; 3 changed; 4 diagnostic range/cap).",(long)geometry[0]);
        wchar_t summary[4200];
        swprintf(summary, 4200, L"Loader status: %ld (1 means ready).\n%ls\nPhase: %ld - %ls\nUpdate counter: %lu. Pending command: %ld (0 means none).\n\nAllocation rehearsal: %ls\nChapter lifetime: %ls\nChapter boundary observation: %ls\n%ls\nGuard callbacks: deferred %ld; initialized callback entered %ld.\nAnimation entries (local / remote / trial): %ld / %ld / %ld.\nMaterial preparation: %ls\n%ls\nNo visible traveler or networking added.\n\nRead-only status snapshots; fields can change between reads. A stopped counter can also mean the game is paused in the background. Return to gameplay before comparing Status readings. Allocation Test permits one factory attempt per game process and does not initialize graphics, effects or networking. It is not a spawning or multiplayer demonstration. Close does not remove the game hook.",
                 (long)status, states[observation], (long)phase, phase_name, (unsigned long)(DWORD)frames, (long)pending, trial_text, lifetime_text, resource_text, pool_text, (long)mesh[1], (long)mesh[2], (long)mesh[3], (long)mesh[4], (long)mesh[5], material_text, geometry_text);
        return finish(status < 0 ? 1 : 0, summary);
    }
    if (status == 0) {
        HANDLE init_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)(base + JC_INIT_RVA), (void *)(uintptr_t)(traveler_startup ? 3 : lifecycle_mode ? 2 : connection_mode), 0, NULL);
        if (!init_thread) {
            fail("create_initialize_thread", GetLastError(), 0);
            CloseHandle(process);
            return failure_message();
        }
        trace("initialize_thread_created", 0, 0);
        DWORD waited = WaitForSingleObject(init_thread, 15000);
        DWORD wait_error = waited == WAIT_FAILED ? GetLastError() : 0;
        CloseHandle(init_thread);
        if (waited != WAIT_OBJECT_0) {
            fail("wait_initialize_thread", wait_error, waited);
            CloseHandle(process);
            return failure_message();
        }
    }
    ULONGLONG deadline = GetTickCount64() + 15000;
    do {
        if (!remote_read(process, base + JC_STATUS_RVA, &status, sizeof(status))) break;
        if (status == 1 || status < 0 || ((connection_mode || traveler_startup) && status==2)) break;
        Sleep(100);
    } while (GetTickCount64() < deadline && WaitForSingleObject(process, 0) == WAIT_TIMEOUT);
    if (status != 1 && !((connection_mode || traveler_startup) && status==2)) {
        wchar_t text[512];
        swprintf(text, 512, L"Diagnostic status: %ld. Use Save Session Report for runtime diagnostics. Status 2 means waiting for a game frame; return to Journey. Negative status means initialization failed; restart before retrying.", (long)status);
        CloseHandle(process);
        return finish(1, text);
    }
    if (connection_mode) {
        MODULEENTRY32W main_module;
        if (!remote_read(process,base+JC_OBSERVATION_RVA,&observation_mode,sizeof(observation_mode)) || observation_mode!=(lifecycle_mode ? 5 : 4) ||
            !module_info(pid,L"Journey.exe",game,&main_module) || main_module.modBaseSize!=expected_image_size) {
            CloseHandle(process); return finish(1,L"Connection Scan could not verify its isolated capture mode or the game module. Restart Journey before retrying.");
        }
        int recorded=connection_record(process,base,(uintptr_t)main_module.modBaseAddr,directory,pid,lifecycle_mode ? 2 : !wcscmp(mode,L"--cloth-check"));
        CloseHandle(process);
        return finish(recorded ? 0 : 1,connection_recorder.summary[0] ? connection_recorder.summary : L"Could not create the connection log beside JNET. No scan started; check folder access and retry.");
    }
    if (repeat_request || traveler_startup || traveler_request || active_request) {
        LONG reader_result=reader_check_isolated();
        if (!remote_control(process,base,1,reader_result)) {
            CloseHandle(process); return finish(1,L"Reader-check result could not be transferred. No traveler action was queued.");
        }
    }
    if (startup_pid && traveler_startup) {
        preview_process=process; preview_base=base; preview_polls=0; preview_last_report[0]=0;
        refresh_preview(); int result=finish(0,preview_text);
        preview_process=NULL; preview_base=0; CloseHandle(process); return result;
    }
    if (startup_pid) {
        CloseHandle(process);
        return finish(0,L"Capacity test attached. Let Journey finish loading and enter solo gameplay, then click Status and Copy all. The temporary startup candidate requests remote shader capacity 4; Status verifies whether it was compiled and inspects the live pools. Once Status confirms successful capacity compilation and remote pool 4 slots / 2 occupied / 2 free, click Prepare Trial and follow its chapter-select then portal-return instructions. Do not use Watch or other experiments.");
    }
    LONG command = connection_request ? 27 : local_three_request ? 26 : outfit_request ? 25 : repeat_request ? 24 : drop_request ? 23 : active_request ? 22 : traveler_request ? 21 : native_playback_request ? 20 : evaluation_request ? 19 : mirror_request ? 18 : !wcscmp(mode,L"--host-room") ? 15 : !wcscmp(mode,L"--join-room") ? 16 : !wcscmp(mode,L"--next-peer") ? 17 : !wcscmp(mode,L"--join-trial") ? 14 : !wcscmp(mode,L"--link-trial") ? 11 : !wcscmp(mode,L"--send-pose") ? 12 : !wcscmp(mode,L"--stop-link") ? 13 : !wcscmp(mode, L"--preview") ? 10 : !wcscmp(mode, L"--render-refs") ? 9 : !wcscmp(mode, L"--sample") ? 1 : !wcscmp(mode, L"--watch") ? 2 :
        !wcscmp(mode, L"--pause") ? 3 : !wcscmp(mode, L"--bindings") ? 4 : !wcscmp(mode, L"--allocate-test") ? 5 : !wcscmp(mode, L"--lifetime-test") ? 6 : !wcscmp(mode, L"--resource-test") ? 7 : !wcscmp(mode, L"--build-check") ? 8 : 0;
    if (command==20 || command==19 || command==18 || command==11 || command==14 || command==15 || command==16) {
        LONG startup[5],life[152],resource,link[16],queued,dual[8];
        if (!remote_read(process,base+JC_DUAL_RVA,dual,sizeof(dual)) ||
            !remote_read(process,base+JC_STARTUP_RVA,startup,sizeof(startup)) ||
            !remote_read(process,base+JC_MESH_LIFE_RVA,life,sizeof(life)) ||
            !remote_read(process,base+JC_RESOURCE_RVA,&resource,sizeof(resource)) ||
            !remote_read(process,base+JC_LINK_RVA,link,sizeof(link)) ||
            !remote_read(process,base+JC_COMMAND_RVA,&queued,sizeof(queued))) {
            CloseHandle(process); return finish(1,L"Could not read trial preparation. No action was queued. Click Status before retrying.");
        }
        if (link[0] || queued || life[25]) {
            CloseHandle(process); return finish(1,L"A link, render test or pending action already exists. Return to gameplay for pending actions; restart Journey to create a second link or reuse a drawn trial.");
        }
        if (!startup[0] || startup[1]!=1 || startup[2]!=1 || startup[3] || startup[4]!=1 || resource!=5 || life[0]!=1 || dual[0]!=1) {
            wchar_t reason[640];
            swprintf(reason,640,L"This game's two-body preparation is not ready: capacity %ld / %ld / %ld / %ld / %ld; resource %ld (need 5); retained %ld (need 1).\nComplete Capacity Test before launching Journey, then Prepare Trial and the Desert portal return. Wait for Status to confirm both bodies retained. No action or invite was consumed.",
                (long)startup[0],(long)startup[1],(long)startup[2],(long)startup[3],(long)startup[4],(long)resource,(long)life[0]);
            CloseHandle(process); return finish(1,reason);
        }
    }
    if (command==17) {
        LONG link[3];
        if (!remote_read(process,base+JC_LINK_RVA,link,sizeof(link)) || link[0]!=1 || link[2]<4) {
            CloseHandle(process); return finish(1,L"Next Peer requires an active Host Room or Join Room session.");
        }
    }
    if (command==12 || command==14 || command==16) {
        LONG link[16],queued; SIZE_T written=0;
        if (!remote_read(process,base+JC_LINK_RVA,link,sizeof(link)) || link[0] ||
            !remote_read(process,base+JC_COMMAND_RVA,&queued,sizeof(queued)) || queued || !request_invite()) {
            CloseHandle(process); return finish(0,L"Join cancelled or a link/action already exists. Restart Journey before a second link.");
        }
        int ok=WriteProcessMemory(process,(void *)(base+JC_LINK_CONFIG_RVA),invite_config,sizeof(invite_config),&written) && written==sizeof(invite_config);
        SecureZeroMemory(invite_config,sizeof(invite_config));
        if (!ok) { CloseHandle(process); return finish(1,L"Invite transfer failed; restart Journey. No send command queued."); }
    }
    if (command==20 || command==15 || command==16) {
        LONG reader_result=reader_check_isolated();
        if (!remote_control(process,base,1,reader_result)) {
            CloseHandle(process); return finish(1,L"Reader-check result could not be transferred. No playback command was queued.");
        }
    }
    trace("requested_command", 0, (DWORD)command);
    if (command) {
        LONG queued = -1;
        if (!remote_read(process, base + JC_COMMAND_RVA, &queued, sizeof(queued)) || queued != 0 ||
            !remote_control(process,base,2,command)) {
            trace("command_queue_failed", GetLastError(), (DWORD)queued);
            CloseHandle(process);
            return finish(1, L"Could not queue the diagnostic action, or another action is pending. Return to Journey so it can process a frame before retrying.");
        }
    }
    if (command==27 || command==26 || command==25 || command==24 || command==23 || command==22 || command==21 || command==20 || command==19 || command==18 || command==10 || command==11 || command==12 || command==13 || command==14 || command==15 || command==16 || command==17) {
        preview_process=process; preview_base=base; preview_polls=0;
        wcscpy(preview_last_report,preview_text);
        refresh_preview();
        int result=finish(0,preview_text);
        preview_process=NULL; preview_base=0;
        CloseHandle(process); return result;
    }
    CloseHandle(process);
    if (command==8) return finish(0,L"Build Check queued. Return to solo gameplay, then Status. Once Armed, select chapter select, wait for loading, then Status and Copy all. If still waiting, enter a playable chapter and check again. This observes the captured geometry resource's native build routine; it does not allocate a trial object or change instanceLimit. Avoid all other tests. Use a backed-up profile and restart after recording.");
    if (command==9) return finish(0,L"Chapter Refs queued. Return to solo gameplay, then click Status until red Armed. Use chapter select once and enter gameplay, then Status and Copy all. This observes the original local traveler's queue across cleanup; no extra traveler or capacity change. Do not use Capacity Test or other experiments. Restart after recording. Waiting expires after ten minutes from arming.");
    if (command==7) return finish(0,L"Prepare Trial queued for two independent bodies. Return to gameplay, then Status until Armed. Use chapter select and the pink Desert portal. Wait for Status to confirm both bodies are retained.\n\nNative Companions runs both native evaluators for sixty seconds, if all graph/scheduler checks pass. Walk, turn and jump/glide after returning to gameplay. Evaluation Check is a read-only storage inspection; Solo Playback copies local evaluated bones to two bodies. Use the first test client only and choose one playback mode per fresh process.\n\nNo invite or second client is needed for animation tests. Stay in Desert. Preparation expires after ten minutes. Bodies remain allocated until Journey exits. Stop Link stops playback; Copy All preserves the report.");
    if (command==6) return finish(0, L"Lifetime test queued. Return to Journey briefly, then Status. Once it says Object retained, use Journey's chapter select to change chapters, return to gameplay, then Status and Copy all. The object stays unregistered and invisible. Do not use Watch/Sample/Bindings or Allocation Test first. One factory attempt per fresh process; native faults can crash Journey. Use a disposable/backed-up profile. Restart Journey after recording the result.");
    if (command==5) return finish(0, L"Allocation rehearsal queued. Return to Journey briefly, then click Status. This calls the native Dude factory and deleting destructor once, without registering a visible traveler. It changes temporary process memory and advances constructor counters; native faults can still crash Journey. Use a disposable/backed-up profile. No Lua, graphics initialization or multiplayer traffic is added by this test.");
    return finish(0, command ?
        L"Diagnostic action queued. Return to Journey to let it run. Results are retained in session memory. Click Status to check progress. Watch records up to 15 samples per window and can be rearmed within the session budget. No additional companions are enabled." :
        L"Native capture is ready. Click Native Layout to check the recovered object layout without Lua sampling. Sample, Watch and Bindings explicitly enable the separate Lua observer. Keep this helper open and use its buttons. Restart Journey to remove the in-memory hook.");
}

int wmain(int argc, wchar_t **argv) {
    if (argc==2 && !wcscmp(argv[1],L"--reader-check-child")) return reader_check_child();
    if (argc==2 && !wcscmp(argv[1],L"--reader-check")) {
        wchar_t directory[JC_PATH_CAP],game[JC_PATH_CAP];
        if (!jc_paths(NULL,directory,game)) return 100;
        open_helper_log(directory);
        LONG checked=reader_check_isolated();
        return (int)checked;
    }
    const wchar_t *mode = argc == 1 ? L"--status" : argv[1];
    if (argc > 2) return finish(1, L"Use one optional argument, or launch normally and use the action buttons.");
    int result;
    do {
        requested_action = 0;
        failure_stage = "unknown";
        failure_error = failure_detail = 0;
        result = run_action(mode);
        if (requested_action > 0 && requested_action <= JC_ACTION_COUNT)
            mode = action_modes[requested_action - 1];
    } while (requested_action > 0 && requested_action <= JC_ACTION_COUNT);
    return result;
}
