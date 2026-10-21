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

