#ifndef JNET_APPEARANCE_LIVE_H
#define JNET_APPEARANCE_LIVE_H
static int (*jnet_character_apply_native)(uintptr_t,uintptr_t,unsigned);
static int jnet_save_newer(const unsigned char *local,const unsigned char *loaded) {
    unsigned short version;unsigned outfit,symbol,capacity,energy,level;uint64_t a,b;
    memcpy(&version,local,2);memcpy(&outfit,local+8,4);memcpy(&symbol,local+12,4);
    memcpy(&capacity,local+16,4);memcpy(&energy,local+20,4);memcpy(&level,local+24,4);
    memcpy(&a,local+0x40,8);memcpy(&b,loaded+0x40,8);
    return version==22 && outfit<=6 && symbol<=20 && capacity<=JNET_SCARF_MAX && energy<=capacity && level<12 && a>b;
}
static unsigned jnet_appearance_initialized,jnet_appearance_saving;
static unsigned jnet_appearance_saved[3],jnet_appearance_has_save;
static unsigned char jnet_appearance_save_expected[0x1ba8];
static unsigned jnet_appearance_save_captured;
static void jnet_appearance_loaded(uintptr_t save) {
    unsigned values[3];
    if(own_read(NULL,save+0x10,values,sizeof(values)) && values[0]<=6 && values[1]<=20 && values[2]<=JNET_SCARF_MAX){
        memcpy(jnet_appearance_saved,values,sizeof(values));jnet_appearance_has_save=1;
        jnet_appearance_observe(&jnet_menu_edit,values[0],values[2],values[1],!jnet_appearance_initialized);
        jnet_appearance_initialized=1;
    }
}
#ifdef _WIN32
static void (*jnet_save_load_original)(uintptr_t,uintptr_t);
static void jnet_save_load(uintptr_t save,uintptr_t files) {
    unsigned char local[0x8000],loaded[0x1ba8];char folder[MAX_PATH],path[MAX_PATH];
    HMODULE shell=LoadLibraryA("shell32.dll");
    HRESULT (WINAPI *folder_path)(HWND,int,HANDLE,DWORD,LPSTR)=shell?(void *)GetProcAddress(shell,"SHGetFolderPathA"):NULL;
    if(folder_path && SUCCEEDED(folder_path(NULL,0x1c,NULL,0,folder)) &&
       snprintf(path,sizeof(path),"%s\\Annapurna Interactive\\Journey\\Steam\\SAVE.BIN",folder)<(int)sizeof(path)){
        HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE){DWORD got=0;
            if(GetFileSize(file,NULL)==sizeof(local) && ReadFile(file,local,sizeof(local),&got,NULL) && got==sizeof(local) &&
               own_read(NULL,files+0xb8,loaded,sizeof(loaded)) && jnet_save_newer(local,loaded) &&
               jc_local_windows_write(files+0xb8,local,sizeof(loaded)))
                message("[JNET] restored newer local SAVE.BIN instead of older Steam save");
            CloseHandle(file);
        }
    }
    if(shell)FreeLibrary(shell);
    jnet_save_load_original(save,files);
    jnet_appearance_loaded(save);
}
#endif
#ifdef _WIN32
static void (*jnet_save_poll_original)(uintptr_t);
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
static int jnet_appearance_boot(void) {
    unsigned char flags[2];
    return jnet_chapter_menu && own_read(NULL,jnet_chapter_menu+0x20,flags,2) && (flags[0] || !flags[1]);
}
#else
static unsigned jnet_appearance_test_boot;
static int jnet_appearance_boot(void){return jnet_appearance_test_boot;}
#endif
static int jnet_appearance_context(uintptr_t game,uintptr_t *barn,uintptr_t *save) {
    uintptr_t type=0,actor=0;unsigned char pending=1;int state=0,resume=0;
    return own_read(NULL,game+0x1c0330,&pending,1) && !pending &&
        own_read(NULL,game+0x278,&state,4) && state>0 &&
        own_read(NULL,game+0x27c,&resume,4) && state==resume &&
        own_read(NULL,game+0x1e0,barn,8) && *barn &&
        own_read(NULL,*barn,&type,8) && type==fault_image+0x5da340 &&
        own_read(NULL,*barn+0xb0,&actor,8) && actor==fault_image+0x5d0380 &&
        own_read(NULL,game+0x170,save,8) && *save &&
        own_read(NULL,*save,&type,8) && type==fault_image+0x5e3178;
}
static void jnet_appearance_game(uintptr_t game) {
    uintptr_t barn=0,save=0;unsigned values[4];
    if(jnet_appearance_boot() && jnet_appearance_has_save)
        jnet_appearance_observe(&jnet_menu_edit,jnet_appearance_saved[0],jnet_appearance_saved[2],jnet_appearance_saved[1],0);
    if(!jnet_appearance_context(game,&barn,&save)){
        if(jnet_menu_edit.awaiting)strcpy(jnet_appearance_status,"Waiting for a loaded character");
        return;
    }
    if(
       !own_read(NULL,barn+0x118,values,sizeof(values)) ||
       values[0]>JNET_SCARF_MAX || values[1]>JNET_SCARF_MAX || values[2]>6 || values[3]>20 ||
       (values[0]>values[1] && !jnet_menu_edit.applied.locked_scarf && !jnet_menu_edit.applied.infinite_scarf))return;
    if(!jnet_appearance_initialized){
        jnet_appearance_observe(&jnet_menu_edit,values[2],values[1],values[3],1);
        jnet_appearance_initialized=1;
    }
    if(jnet_menu_edit.awaiting){
        unsigned changed[4]={jnet_menu_edit.pending.scarf_segments,jnet_menu_edit.pending.scarf_segments,jnet_menu_edit.pending.outfit,jnet_menu_edit.pending.symbol};
        int save_changed=changed[1]!=values[1] || changed[2]!=values[2] || changed[3]!=values[3];
        int ok=changed[1]<=JNET_SCARF_MAX && changed[2]<=6 && changed[3]<=20 &&
            jc_local_windows_write(barn+0x118,changed,sizeof(changed));
        if(ok && !(jnet_character_apply_native?jnet_character_apply_native(game,barn,jnet_menu_edit.pending.character):!jnet_menu_edit.pending.character)){
            jc_local_windows_write(barn+0x118,values,sizeof(values));
            ok=0;
        }
        jnet_appearance_complete(&jnet_menu_edit,ok);
        if(ok){jnet_appearance_applied_at=jnet_menu_edit.applied.character?0:GetTickCount64();jnet_appearance_saved[0]=changed[2];jnet_appearance_saved[1]=changed[3];jnet_appearance_saved[2]=changed[1];jnet_appearance_has_save=1;memcpy(values,changed,sizeof(values));if(save_changed)jnet_save_requested=1;strcpy(jnet_appearance_status,jnet_menu_edit.applied.character?"Activating character":"");}
        else strcpy(jnet_appearance_status,"Appearance could not be applied");
    }
    if(jnet_menu_edit.applied.locked_scarf){
        unsigned length=jnet_menu_edit.applied.scarf_segments;
        if(values[1]!=length || values[0]>length){values[1]=length;if(values[0]>length)values[0]=length;
            if(!jc_local_windows_write(barn+0x118,values,8))return;}
    }
    if(jnet_menu_edit.applied.infinite_scarf && values[0]!=values[1]){
        if(!jc_local_windows_write(barn+0x118,values+1,4))return;
        values[0]=values[1];
    }
    if(!jnet_appearance_boot())jnet_appearance_observe(&jnet_menu_edit,values[2],values[1],values[3],0);
    if(jnet_save_requested && !jnet_appearance_saving){
        uintptr_t files=0;unsigned state=0;unsigned char disabled=1,enabled=0;
        if(!own_read(NULL,game+0x50,&files,8) || !files ||
           !own_read(NULL,files+0x200bc,&state,4) || state!=1 ||
           !own_read(NULL,files+0x202ad,&disabled,1) || disabled ||
           !own_read(NULL,fault_image+0x3c9420e,&enabled,1) || !enabled)return;
        unsigned level=jnet_current_level;
        if(level>=12 || !jc_local_windows_write(save+0x20,&level,4) ||
           !jc_local_windows_write(save+0x19a8,values+1,4))return;
        void (*save_game)(uintptr_t)=(void *)(fault_image+JC_RVA_SaveGame);
        save_game(game);
        if(own_read(NULL,files+0x200bc,&state,4) && state==2){
            jnet_appearance_save_captured=own_read(NULL,files+0xb8,jnet_appearance_save_expected,sizeof(jnet_appearance_save_expected));
            jnet_save_requested=0;
            jnet_appearance_saving=1;
        }
    }
}
#endif
