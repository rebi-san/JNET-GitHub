#ifndef JNET_FRAME_PACING_H
#define JNET_FRAME_PACING_H
static void jnet_frame_wait(unsigned fps) {
    static LARGE_INTEGER frequency;
    static LONGLONG previous;
    static unsigned timer_active;
    static unsigned previous_fps;
    if(fps!=previous_fps){previous=0;previous_fps=fps;}
    if(fps!=30 && fps!=55){
        previous=0;
        if(timer_active){timeEndPeriod(1);timer_active=0;}
        return;
    }
    if(!frequency.QuadPart && (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0))return;
    if(!timer_active)timer_active=timeBeginPeriod(1)==TIMERR_NOERROR;
    LARGE_INTEGER now;
    if(!QueryPerformanceCounter(&now))return;
    LONGLONG period=(frequency.QuadPart+fps-1)/fps;
    if(previous && now.QuadPart>=previous && now.QuadPart-previous<period){
        LONGLONG deadline=previous+period;
        while(now.QuadPart<deadline){
            double ms=(double)(deadline-now.QuadPart)*1000.0/(double)frequency.QuadPart;
            if(timer_active && ms>2.0)Sleep((DWORD)(ms-1.0));
            else SwitchToThread();
            if(!QueryPerformanceCounter(&now))return;
        }
    }
    previous=now.QuadPart;
}
#endif
