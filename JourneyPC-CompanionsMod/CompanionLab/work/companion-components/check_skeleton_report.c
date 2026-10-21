#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
typedef int32_t LONG;
typedef void *HANDLE;
#define JC_SKELETON_RVA 1
#define JC_COMMAND_RVA 2
#define JC_BODIES_RVA 3
#define JC_MOTION_RVA 4
static LONG sk[32],pending,body[48],motion[28];
static int remote_read(HANDLE p,uintptr_t at,void *out,size_t n) {
    (void)p; const void *src=at==1 ? (void *)sk : at==2 ? (void *)&pending : at==3 ? (void *)body : at==4 ? (void *)motion : NULL;
    if (!src) return 0;
    memcpy(out,src,n); return 1;
}
static int skeleton_report(HANDLE process,uintptr_t base,wchar_t *text,size_t cap) {
    LONG sk[32],bodies[48],motion[28],pending;
    if (!remote_read(process,base+JC_SKELETON_RVA,sk,sizeof(sk)) ||
        !remote_read(process,base+JC_COMMAND_RVA,&pending,sizeof(pending))) return -1;
    if (!sk[0] && pending!=18) return 0;
    if (!remote_read(process,base+JC_BODIES_RVA,bodies,sizeof(bodies)) ||
        !remote_read(process,base+JC_MOTION_RVA,motion,sizeof(motion))) return -1;
    int n=swprintf(text,cap,L"SOLO SKELETON PLAYBACK - Scarfhelper 0.76\r\n"
        L"State: %ld (1 selected; 2 stopped; -1 preparation failed). Pending command: %ld.\r\n"
        L"Mapping result/stage: %ld / %ld. Named bones local/native remote/body 1/body 2: %ld / %ld / %ld / %ld.\r\n"
        L"Codes: 0 accepted; 1 read; 2 names/type; 3 changed; 4 bounds; 5 capacity; 6 mapping/ownership; 7 write; 8 postcheck; 9 budget.\r\n"
        L"Stages: 1 collect; 2 independence/mapping; 3 recheck; 4 live ownership; 5 stable source; 6 copy; 7 copy postcheck; 8 native-render postcheck; 0 accepted.\r\n\r\n",
        (long)sk[0],(long)pending,(long)sk[1],(long)sk[2],(long)sk[3],(long)sk[4],(long)sk[5],(long)sk[6]);
    if (n<=0 || (size_t)n>=cap) return -1;
    size_t used=(size_t)n;
    for (unsigned i=0;i<2;++i) {
        unsigned r=8+i*8,b=i*24;
        n=swprintf(text+used,cap-used,L"Body %u: render %ld (1 waiting; 2 drawing; 3 ended; -1 failed); draws %ld; failure %ld (26 skeleton copy).\r\n"
            L"Copy result/stage: %ld / %ld. Accepted copies: %ld. Copies with changed bones: %ld. Relative spine changes: %ld.\r\n"
            L"Copy ms last/peak: %ld / %ld. Reads: %ld. Attempted bone writes: %ld.\r\n"
            L"Post-render check ms last/peak: %ld / %ld. Reads: %ld.\r\n"
            L"Whole pose update ms last/peak: %ld / %ld. Check/draw ms last/peak: %ld / %ld.\r\n\r\n",
            i+1,(long)bodies[b+1],(long)bodies[b+2],(long)bodies[b+3],(long)sk[r],(long)sk[r+1],
            (long)sk[r+2],(long)sk[r+3],(long)motion[10+i*7],(long)sk[r+4],(long)sk[r+5],(long)sk[r+6],(long)sk[r+7],
            (long)sk[24+i*4],(long)sk[25+i*4],(long)sk[26+i*4],
            (long)bodies[b+13],(long)bodies[b+14],(long)bodies[b+15],(long)bodies[b+16]);
        if (n<=0 || (size_t)n>=cap-used) return -1;
        used+=(size_t)n;
    }
    n=swprintf(text+used,cap-used,L"Devishaz only: walk, turn and jump/glide for sixty seconds. Two offset bodies target 20 pose updates/sec each; actual rate depends on frame cost. Compare responsiveness with 0.76 and copy this report. Stop Link also stops this test.\r\n"
        L"Uses the local player's native evaluated bones and Journey's mesh renderer. It does not send bone poses over the room network yet. No robe/scarf, chirp effects or departure effects. Stay in Desert; restart Journey for another mode. Keep Scarfhelper open for exit capture.");
    return n>0 && (size_t)n<cap-used ? 1 : -1;
}


int main(void) {
    wchar_t output[10000]; assert(!skeleton_report(NULL,0,output,10000));
    pending=18; assert(skeleton_report(NULL,0,output,10000)==1);
    for (unsigned i=0;i<32;++i) sk[i]=INT32_MAX;
    for (unsigned i=0;i<48;++i) body[i]=INT32_MAX;
    for (unsigned i=0;i<28;++i) motion[i]=INT32_MAX;
    assert(skeleton_report(NULL,0,output,10000)==1); size_t size=wcslen(output)+1;
    assert(wcsstr(output,L"Body 2") && wcsstr(output,L"Relative spine changes"));
    for (size_t cap=0;cap<size;++cap) {
        output[cap]=L'Z'; assert(skeleton_report(NULL,0,output,cap)==-1); assert(output[cap]==L'Z');
    }
    printf("production skeleton report: both bodies, pending/inactive, integer extremes, every truncation; %zu/10000 characters\n",size);
}
