#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef int64_t LONGLONG;
typedef unsigned DWORD;
typedef struct {LONGLONG QuadPart;} LARGE_INTEGER;
#define TIMERR_NOERROR 0
static int64_t clock_ticks=1000000;
static unsigned begins,ends,sleeps;
static int QueryPerformanceFrequency(LARGE_INTEGER *v){v->QuadPart=1000000;return 1;}
static int QueryPerformanceCounter(LARGE_INTEGER *v){v->QuadPart=clock_ticks;return 1;}
static unsigned timeBeginPeriod(unsigned n){assert(n==1);++begins;return 0;}
static unsigned timeEndPeriod(unsigned n){assert(n==1);++ends;return 0;}
static void Sleep(DWORD ms){++sleeps;clock_ticks+=ms*1000;}
static int SwitchToThread(void){clock_ticks+=50;return 1;}
#include "jnet_frame_pacing.h"
int main(void){
 jnet_frame_wait(55);int64_t t=clock_ticks;
 clock_ticks+=1000;jnet_frame_wait(55);assert(clock_ticks-t>=18182 && clock_ticks-t<18250);
 jnet_frame_wait(30);t=clock_ticks;clock_ticks+=1000;jnet_frame_wait(30);assert(clock_ticks-t>=33334 && clock_ticks-t<33400);
 t=clock_ticks;clock_ticks+=100000;jnet_frame_wait(30);assert(clock_ticks==t+100000);
 t=clock_ticks;jnet_frame_wait(0);assert(clock_ticks==t && ends==1 && begins==1 && sleeps);
 jnet_frame_wait(55);assert(clock_ticks==t && begins==2);jnet_frame_wait(0);assert(ends==2);
 puts("Frame pacing: 30/55 deadlines, slow frames, Default, mode changes and timer pairing passed with mocked clock.");
}
