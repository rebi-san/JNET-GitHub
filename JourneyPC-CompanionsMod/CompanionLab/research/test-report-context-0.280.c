
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef int32_t LONG;
typedef void *HANDLE;
typedef struct {unsigned wYear,wMonth,wDay,wHour,wMinute,wSecond;} SYSTEMTIME;
static void GetSystemTime(SYSTEMTIME *s){*s=(SYSTEMTIME){2026,10,18,12,34,56};}
static unsigned GetProcessId(HANDLE p){(void)p;return 99;}
#define CP_UTF8 0
#define MB_ERR_INVALID_CHARS 0
#define JC_STATUS_RVA 0
#define JC_FRAMES_RVA 4
#define JC_PHASE_RVA 8
#define JC_OBSERVATION_RVA 12
#define JC_GAME_RVA 16
static unsigned char data[32],game[0x1c0400];
static int fail;
static int remote_read(HANDLE p,uintptr_t address,void *out,size_t size){
 (void)p;
 if(fail)return 0;
 uintptr_t a=(uintptr_t)data,b=(uintptr_t)game;
 if(!((address>=a && address-a<=sizeof(data) && size<=sizeof(data)-(address-a)) ||
      (address>=b && address-b<=sizeof(game) && size<=sizeof(game)-(address-b))))return 0;
 memcpy(out,(void*)address,size);return 1;
}
static int MultiByteToWideChar(unsigned cp,unsigned flags,const char *s,int n,wchar_t *o,int cap){
 (void)cp;(void)flags;(void)n;
 size_t len=strlen(s);if(len+1>(size_t)cap)return 0;
 for(size_t i=0;i<=len;++i)o[i]=(unsigned char)s[i];
 return (int)len+1;
}
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

int main(void){
 wchar_t out[3000];
 fail=1;assert(append_game_context_report(NULL,(uintptr_t)data,out,3000)>0);
 assert(wcsstr(out,L"game observed -1; readable mask 000"));
 assert(wcsstr(out,L"Native level unavailable"));
 fail=0;
 assert(append_game_context_report(NULL,(uintptr_t)data,out,3000)>0);
 assert(wcsstr(out,L"game observed 0; readable mask 01f"));
 uintptr_t ptr=(uintptr_t)game;memcpy(data+16,&ptr,8);
 LONG frame=123,state=1;memcpy(data+4,&frame,4);memcpy(game+0x278,&state,4);memcpy(game+0x27c,&state,4);
 strcpy((char*)game+0x1c02f0,"Level_Graveyard");
 assert(append_game_context_report(NULL,(uintptr_t)data,out,3000)>0);
 assert(wcsstr(out,L"updates 123"));assert(wcsstr(out,L"readable mask 1ff"));
 assert(wcsstr(out,L"Native level Level_Graveyard; pending load byte 0; state/resume 1/1"));
 assert(wcsstr(out,L"2026-10-18 12:34:56"));
 assert(append_game_context_report(NULL,(uintptr_t)data,out,8)<0);
 puts("Game context report: unavailable, pre-frame, playable Chapter Select, timestamp and bounded output passed.");
}
