#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../diagnostic/native_local_read.h"
static uintptr_t JC_CONTROL_RVA;
#include "remote_control_fixture.h"

static int control_check(const char *path) {
    HMODULE dll=LoadLibraryA(path);
    if (!dll) return 10;
    uintptr_t base=(uintptr_t)dll;
    FARPROC entry=GetProcAddress(dll,"JCProbeControl");
    volatile LONG *command=(void *)GetProcAddress(dll,"JCProbeCommand");
    volatile LONG *features=(void *)GetProcAddress(dll,"JCProbeCompanionFeatures");
    volatile LONG *startup=(void *)GetProcAddress(dll,"JCProbeStartup");
    volatile LONG *status=(void *)GetProcAddress(dll,"JCProbeStatus");
    if (!entry || !command || !features || !startup || !status) return 11;
    JC_CONTROL_RVA=(uintptr_t)entry-base;
    HANDLE process=GetCurrentProcess();
    if (remote_control(process,base,99,0) || remote_control(process,base,2,23) || *command) return 12;
    if (!remote_control(process,base,1,-1) || features[9]!=-1 ||
        !remote_control(process,base,1,0) || features[9]) return 13;
    if (!remote_control(process,base,2,22) || *command!=22 ||
        remote_control(process,base,2,13) || *command!=22) return 14;
    InterlockedExchange(command,0);
    if (!remote_control(process,base,2,13) || *command!=13) return 15;
    if (!remote_control(process,base,3,1) || startup[0]!=1) return 16;
    InterlockedExchange(status,1);InterlockedExchange(startup,0);
    if (remote_control(process,base,3,1) || startup[0]) return 17;
    if (!FreeLibrary(dll)) return 18;
    puts("Production DLL control: reader gate, command queue, pending refusal, invalid requests and startup gate passed.");
    return 0;
}

__declspec(align(4096)) volatile LONG image_page[1024]={1};
static volatile LONG stopped, faults;
static LONG CALLBACK fault(EXCEPTION_POINTERS *p) {
    if (p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION &&
        p->ExceptionRecord->NumberParameters>=2 &&
        p->ExceptionRecord->ExceptionInformation[0]==0 &&
        p->ExceptionRecord->ExceptionInformation[1]==(ULONG_PTR)&image_page[1] &&
        InterlockedIncrement(&faults)<100000) return EXCEPTION_CONTINUE_EXECUTION;
    return EXCEPTION_CONTINUE_SEARCH;
}
static DWORD WINAPI reader(void *unused) {
    (void)unused;
    while (!InterlockedCompareExchange(&stopped,0,0)) {
        LONG n=image_page[1]; (void)n;
    }
    return 0;
}
int main(int argc,char **argv) {
    int direct=argc>1 && !strcmp(argv[1],"direct");
    int local=argc>1 && !strcmp(argv[1],"local");
    if (argc>2) { int code=control_check(argv[2]); if (code) return code; }
    if (local && jc_local_windows_test()) return 6;
    MEMORY_BASIC_INFORMATION info={0};
    VirtualQuery((void *)image_page,&info,sizeof(info));
    printf("image type=%lx protection=%lx mode=%s\n",info.Type,info.Protect,direct?"direct":local?"protected local":"WPM");
    fflush(stdout);
    void *handler=AddVectoredExceptionHandler(1,fault);
    HANDLE thread=CreateThread(NULL,0,reader,NULL,0,NULL);
    if (!handler || !thread) return 2;
    unsigned writes=0; DWORD start=GetTickCount();
    while (GetTickCount()-start<3000) {
        LONG value=(LONG)++writes; SIZE_T written=0;
        if (direct) InterlockedExchange(&image_page[0],value);
        else if (local) { if (!jc_local_windows_write((uintptr_t)image_page,&value,sizeof(value))) return 7; }
        else if (!WriteProcessMemory(GetCurrentProcess(),(void *)image_page,&value,sizeof(value),&written) || written!=sizeof(value)) return 3;
    }
    InterlockedExchange(&stopped,1);
    if (WaitForSingleObject(thread,5000)!=WAIT_OBJECT_0) return 4;
    CloseHandle(thread);RemoveVectoredExceptionHandler(handler);
    printf("writes=%u read_access_violations=%ld\n",writes,faults);
    return (direct || local) && faults ? 5 : 0;
}
