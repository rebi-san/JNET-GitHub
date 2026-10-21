
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "diagnostic_fault.h"
typedef int32_t LONG; typedef uint32_t DWORD; typedef uintptr_t HANDLE;
#define WINAPI
#define WAIT_TIMEOUT 258u
#define WAIT_OBJECT_0 0u
#define WAIT_FAILED 0xffffffffu
#define JC_FLIGHT_RVA 0x100
static unsigned polls,limit,closes,flushed,queries;static int wait_fail,query_fail,read_fail;
static char output[4096];static size_t used;
static DWORD WaitForSingleObject(HANDLE h,DWORD ms){assert(h==1 && ms==100);return polls++<limit ? WAIT_TIMEOUT : wait_fail ? WAIT_FAILED : WAIT_OBJECT_0;}
static int GetExitCodeProcess(HANDLE h,DWORD *code){assert(h==1);++queries;*code=0xc0000005u;return !query_fail;}
static DWORD GetLastError(void){return 6;}
static int remote_read(HANDLE h,uintptr_t addr,void *out,size_t n){assert(h==1 && addr==0x1100 && n==JC_FLIGHT_FIELDS*sizeof(LONG));LONG v[JC_FLIGHT_FIELDS]={20,2,0,77,0,0,0,0,1,0,0,0};v[2]=(LONG)polls;memcpy(out,v,n);return !read_fail;}
static int WriteFile(HANDLE h,const void *p,DWORD n,DWORD *written,void *overlap){assert(h==2 && !overlap && used+n<sizeof(output));memcpy(output+used,p,n);used+=n;output[used]=0;*written=n;return 1;}
static int FlushFileBuffers(HANDLE h){assert(h==2);++flushed;return 1;}
static int CloseHandle(HANDLE h){assert(h==1 || h==2);++closes;return 1;}
struct exit_watch {
    HANDLE process,file,thread;
    uintptr_t base;
    DWORD pid;
};
static struct exit_watch exit_watch;
static DWORD WINAPI watch_exit(void *opaque) {
    struct exit_watch *watch=opaque;
    LONG samples[8][JC_FLIGHT_FIELDS]={0}; unsigned captured=0,next=0;
    DWORD waited;
    while ((waited=WaitForSingleObject(watch->process,100))==WAIT_TIMEOUT) {
        LONG sample[JC_FLIGHT_FIELDS];
        if (remote_read(watch->process,watch->base+JC_FLIGHT_RVA,sample,sizeof(sample))) {
            memcpy(samples[next],sample,sizeof(sample)); next=(next+1)%8;
            if (captured<8) ++captured;
        }
    }
    DWORD code=0,error=0,written;
    int obtained=waited==WAIT_OBJECT_0 && GetExitCodeProcess(watch->process,&code);
    if (!obtained) error=GetLastError();
    char line[256];
    int n=snprintf(line,sizeof(line),"[JC-exit] pid=%lu wait=%lu obtained=%d code=%08lx error=%lu samples=%u\r\n",
        (unsigned long)watch->pid,(unsigned long)waited,obtained,(unsigned long)code,(unsigned long)error,captured);
    WriteFile(watch->file,line,(DWORD)n,&written,NULL);
    for (unsigned i=0;i<captured;++i) {
        LONG *v=samples[(next+8-captured+i)%8];
        n=snprintf(line,sizeof(line),"[JC-exit] phase=%ld body=%ld frame=%ld thread=%ld serial=%ld fault=%08lx module=%ld rva=%08lx observer=%ld\r\n",
            (long)v[0],(long)v[1],(long)v[2],(long)v[3],(long)v[4],(unsigned long)(uint32_t)v[5],
            (long)v[6],(unsigned long)(uint32_t)v[7],(long)v[8]);
        WriteFile(watch->file,line,(DWORD)n,&written,NULL);
    }
    FlushFileBuffers(watch->file);
    CloseHandle(watch->file); CloseHandle(watch->process);
    return 0;
}

int main(void){
 for(unsigned mode=0;mode<5;++mode){
  polls=closes=flushed=queries=0;used=0;output[0]=0;limit=mode==0 ? 3 : 10;
  wait_fail=mode==2;query_fail=mode==3;read_fail=mode==4;
  exit_watch=(struct exit_watch){.process=1,.file=2,.base=0x1000,.pid=123};
  assert(!watch_exit(&exit_watch));assert(closes==2 && flushed==1);
  if(mode<2){assert(strstr(output,"obtained=1 code=c0000005"));assert(queries==1);}
  if(mode==0){assert(strstr(output,"samples=3"));assert(strstr(output,"frame=1 ") && strstr(output,"frame=3 "));}
  if(mode==1){assert(strstr(output,"samples=8"));assert(!strstr(output,"frame=1 ") && !strstr(output,"frame=2 "));assert(strstr(output,"frame=3 ")<strstr(output,"frame=10 "));}
  if(mode==2){assert(strstr(output,"obtained=0") && !queries);}
  if(mode==3){assert(strstr(output,"obtained=0") && queries==1);}
  if(mode==4){assert(strstr(output,"samples=0"));assert(!strstr(output,"phase="));}
 }
 puts("Production exit watcher: exit code, chronological bounded samples, failed wait/query/read, flush and handle closure passed; Windows calls mocked.");
}
