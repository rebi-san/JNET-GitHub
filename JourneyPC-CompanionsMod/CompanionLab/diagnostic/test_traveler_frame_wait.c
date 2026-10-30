#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint64_t ULONGLONG;
typedef int LONG;
typedef void *HANDLE;
#define FALSE 0
#define TIMERR_NOERROR 0
#define JC_CLOTH_FRAMES 2
#define JC_FRAME_FREE 0
static void *traveler_actor=(void *)1;
static struct { int failed; struct {struct {int phase;} frames[2];} frames; } backend,*active_cloth_backend=&backend;
static HANDLE cloth_progress_event;
static LONG cloth_progress_pending;
static unsigned now_ms,ready_after,polls,requests,waits,sleeps,begins,ends,event_failure,poll_failure;
static ULONGLONG GetTickCount64(void){return now_ms;}
static HANDLE CreateEventW(void *p,int manual,int initial,void *name){assert(!p && !manual && !initial && !name);return event_failure?NULL:(void *)1;}
static int traveler_frame_retired(void){return ++polls>=ready_after;}
static int jc_cloth_frames_poll(void *frames){assert(frames==&backend.frames);return poll_failure;}
static LONG InterlockedCompareExchange(LONG *p,LONG value,LONG expected){LONG old=*p;if(old==expected)*p=value;return old;}
static void cloth_progress_request(void){++requests;}
static void message(const char *s){assert(s && *s);}
static unsigned timeBeginPeriod(unsigned period){assert(period==1);++begins;return 0;}
static void timeEndPeriod(unsigned period){assert(period==1);++ends;}
static void WaitForSingleObject(HANDLE event,unsigned timeout){assert(event && timeout==1);++waits;++now_ms;}
static void Sleep(unsigned timeout){assert(timeout==1);++sleeps;++now_ms;}
#include "traveler_frame_wait_fixture.h"
static void reset(void){
    memset(&backend,0,sizeof(backend));cloth_progress_pending=0;cloth_progress_event=NULL;
    now_ms=ready_after=polls=requests=waits=sleeps=begins=ends=event_failure=poll_failure=0;
}
int main(void){
    reset();assert(traveler_frame_wait() && !begins && !waits);
    reset();ready_after=3;assert(traveler_frame_wait() && requests==2 && waits==2 && !sleeps && begins==1 && ends==1);
    reset();ready_after=3;event_failure=1;assert(traveler_frame_wait() && sleeps==2 && !waits && begins==ends);
    reset();ready_after=1000;assert(!traveler_frame_wait() && now_ms==500 && begins==1 && ends==1);
    reset();backend.failed=1;assert(!traveler_frame_wait() && !begins && !requests);
    reset();poll_failure=1;assert(!traveler_frame_wait() && !begins);
    reset();backend.frames.frames[1].phase=1;assert(!traveler_frame_wait() && now_ms==500 && begins==ends);
    reset();cloth_progress_pending=1;assert(!traveler_frame_wait() && now_ms==500 && begins==ends);
    puts("Production frame wait: completed frames do not sleep; event wait, allocation fallback, pending leases, timeout and failure preserve retirement checks and balance timer requests.");
}
