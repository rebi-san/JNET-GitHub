static int jnet_appearance_local_saved(void) {
    unsigned char local[0x1ba8];char folder[MAX_PATH],path[MAX_PATH];int ok=0;
    HMODULE shell=LoadLibraryA("shell32.dll");
    HRESULT (WINAPI *folder_path)(HWND,int,HANDLE,DWORD,LPSTR)=shell?(void *)GetProcAddress(shell,"SHGetFolderPathA"):NULL;
    if(jnet_appearance_save_captured && folder_path && SUCCEEDED(folder_path(NULL,0x1c,NULL,0,folder)) &&
       snprintf(path,sizeof(path),"%s\\Annapurna Interactive\\Journey\\Steam\\SAVE.BIN",folder)<(int)sizeof(path)){
        HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE){DWORD got=0;
            ok=GetFileSize(file,NULL)==0x8000 && ReadFile(file,local,sizeof(local),&got,NULL) && got==sizeof(local) &&
                !memcmp(local,jnet_appearance_save_expected,sizeof(local));
            CloseHandle(file);
        }
    }
    if(shell)FreeLibrary(shell);
    return ok;
}
static void jnet_save_poll(uintptr_t files) {
    static uint64_t failed_at;
    jnet_save_poll_original(files);
    unsigned state=0;
    if(jnet_appearance_saving && own_read(NULL,files+0x200bc,&state,4)){
        if(state==4 && !failed_at)failed_at=GetTickCount64();
        if(state==3 || (failed_at && GetTickCount64()-failed_at>=1000)){
            int local_saved=jnet_appearance_local_saved();
            jnet_appearance_saving=0;
            if(failed_at){
                unsigned error=0;char line[160];
                int readable=own_read(NULL,files+0x200c0,&error,4);
                snprintf(line,sizeof(line),"[JNET] appearance save: native error=%u readable=%d local payload verified=%d",error,readable,local_saved);
                message(line);
            }
            if(jnet_menu_page==1)strcpy(jnet_appearance_status,state==3 || local_saved?"":"Save failed");
            failed_at=0;
        }
    }
}
