#ifndef JNET_APPEARANCE_LIVE_H
#define JNET_APPEARANCE_LIVE_H
#include <wchar.h>
#include "jnet_save_profile.h"
static struct jnet_save_profile jnet_profile;
static uint64_t jnet_profile_identity;
static unsigned jnet_profile_present,jnet_profile_dirty;
static int jnet_appearance_title(unsigned level,unsigned visible,unsigned playable) {
    return level==0 && (visible || !playable);
}
static int (*jnet_character_apply_native)(uintptr_t,uintptr_t,unsigned);
static int jnet_save_newer(const unsigned char *local,const unsigned char *loaded) {
    unsigned short version;unsigned outfit,symbol,capacity,energy,level;uint64_t a,b;
    memcpy(&version,local,2);memcpy(&outfit,local+8,4);memcpy(&symbol,local+12,4);
    memcpy(&capacity,local+16,4);memcpy(&energy,local+20,4);memcpy(&level,local+24,4);
    memcpy(&a,local+0x40,8);memcpy(&b,loaded+0x40,8);
    return version==22 && outfit<=6 && symbol<=20 && capacity<=JNET_SCARF_MAX && energy<=capacity && level<12 && a>b;
}
static unsigned jnet_appearance_initialized,jnet_appearance_saving,jnet_save_bootstrapped;
static unsigned jnet_appearance_saved[3],jnet_appearance_has_save,jnet_appearance_saved_energy;
static unsigned char jnet_appearance_save_expected[0x1ba8];
static unsigned jnet_appearance_save_captured;
static void jnet_appearance_loaded(uintptr_t save) {
    unsigned values[3];
    if(own_read(NULL,save+0x10,values,sizeof(values)) && values[0]<=6 && values[1]<=20 && values[2]<=JNET_SCARF_MAX){
        memcpy(jnet_appearance_saved,values,sizeof(values));jnet_appearance_has_save=1;
        if(!own_read(NULL,save+0x1c,&jnet_appearance_saved_energy,4) || jnet_appearance_saved_energy>values[2])jnet_appearance_saved_energy=values[2];
        jnet_appearance_observe(&jnet_menu_edit,values[0],values[2],values[1],!jnet_appearance_initialized);
        jnet_appearance_initialized=1;
    }
}
static int jnet_profile_publish(uintptr_t save) {
    unsigned fields[4]={jnet_profile.outfit,jnet_profile.symbol,jnet_profile.scarf,jnet_profile.energy};
    unsigned char chapter=(unsigned char)jnet_profile.chapter;
    return jc_local_windows_write(save+0x10,fields,sizeof(fields)) &&
        jc_local_windows_write(save+0x19a8,&jnet_profile.scarf,4) &&
        jc_local_windows_write(save+0x20,&jnet_profile.level,4) &&
        jc_local_windows_write(save+0x50,&chapter,1);
}
static int jnet_profile_restore(uintptr_t save) {
#ifdef _WIN32
    uint64_t identity=jnet_steam_account_identity;
#else
    uint64_t identity=jnet_profile_test_identity;
#endif
    if(!identity)return 0;
    if(jnet_profile_identity!=identity){
        memset(&jnet_profile,0,sizeof(jnet_profile));jnet_profile_dirty=0;
        jnet_profile_present=jnet_profile_read(&jnet_profile,identity);
        jnet_profile_identity=identity;
    }
    if(jnet_profile_present){
        if(!jnet_profile_publish(save))return 0;
        jnet_appearance_loaded(save);
    }
    return 1;
}
static int jnet_profile_commit(uintptr_t save,const unsigned values[4],unsigned level,int boot) {
    struct jnet_save_profile next={.identity=jnet_profile_identity,.outfit=values[2],.symbol=values[3],
        .scarf=values[1],.energy=values[0],.level=level,.chapter=level==0};
    if(boot){
        unsigned char chapter=0;
        if(!own_read(NULL,save+0x20,&next.level,4) || !own_read(NULL,save+0x50,&chapter,1))return 0;
        next.chapter=chapter;
    }
    if(!jnet_profile_valid(&next))return 0;
    if(!jnet_profile_present || memcmp(&next,&jnet_profile,sizeof(next))){
        jnet_profile=next;jnet_profile_present=1;jnet_profile_dirty=1;
    }
    if(!jnet_profile_dirty)return jnet_profile_publish(save);
#ifdef _WIN32
    static uint64_t retry;
    uint64_t now=GetTickCount64();if(now<retry)return 0;
    retry=now+1000;
#endif
    if(!jnet_profile_write(&jnet_profile))return 0;
    jnet_profile_dirty=0;
#ifdef _WIN32
    retry=0;
    char line[192];snprintf(line,sizeof(line),"[JNET] account save verified: robe=%u symbol=%u scarf=%u energy=%u level=%u chapter=%u",
        next.outfit,next.symbol,next.scarf,next.energy,next.level,next.chapter);message(line);
#endif
    return jnet_profile_publish(save);
}
#ifdef _WIN32
static int jnet_save_restore_local(uintptr_t payload) {
    unsigned char local[0x8000],loaded[0x1ba8];wchar_t folder[MAX_PATH],path[MAX_PATH];int restored=0,length=-1;
    HMODULE shell=LoadLibraryW(L"shell32.dll");
    HRESULT (WINAPI *folder_path)(HWND,int,HANDLE,DWORD,LPWSTR)=shell?(void *)GetProcAddress(shell,"SHGetFolderPathW"):NULL;
    if(folder_path && SUCCEEDED(folder_path(NULL,0x1c,NULL,0,folder)) &&
       (length=swprintf(path,sizeof(path)/sizeof(*path),L"%ls\\Annapurna Interactive\\Journey\\Steam\\SAVE.BIN",folder))>=0 &&
       (size_t)length<sizeof(path)/sizeof(*path)){
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE){DWORD got=0;
            if(GetFileSize(file,NULL)==sizeof(local) && ReadFile(file,local,sizeof(local),&got,NULL) && got==sizeof(local) &&
               own_read(NULL,payload,loaded,sizeof(loaded)) && jnet_save_newer(local,loaded) &&
               jc_local_windows_write(payload,local,sizeof(loaded))){
                message("[JNET] restored newer local SAVE.BIN instead of older loaded save");restored=1;
            }
            CloseHandle(file);
        }
    }
    if(shell)FreeLibrary(shell);
    return restored;
}
static void (*jnet_save_load_original)(uintptr_t,uintptr_t);
static void jnet_save_load(uintptr_t save,uintptr_t files) {
    jnet_save_restore_local(files+0xb8);
    jnet_save_load_original(save,files);
    jnet_appearance_loaded(save);jnet_save_bootstrapped=0;
}

#endif
#ifdef _WIN32
static void (*jnet_save_serialize_original)(uintptr_t,uintptr_t);
static void jnet_save_serialize(uintptr_t save,uintptr_t files) {
    unsigned state=0;unsigned char payload[0x8000]={0};
    wchar_t folder[MAX_PATH],path[MAX_PATH],temporary[MAX_PATH];int committed=0,length=-1;
    HMODULE shell=LoadLibraryW(L"shell32.dll");
    HRESULT (WINAPI *folder_path)(HWND,int,HANDLE,DWORD,LPWSTR)=shell?(void *)GetProcAddress(shell,"SHGetFolderPathW"):NULL;
    if(own_read(NULL,files+0x200bc,&state,4) && state==1 && folder_path &&
       SUCCEEDED(folder_path(NULL,0x1c,NULL,0,folder)) &&
       (length=swprintf(path,sizeof(path)/sizeof(*path),L"%ls\\Annapurna Interactive\\Journey\\Steam\\SAVE.BIN",folder))>=0 &&
       (size_t)length<sizeof(path)/sizeof(*path) &&
       (length=swprintf(temporary,sizeof(temporary)/sizeof(*temporary),L"%ls.jnet-tmp",path))>=0 &&
       (size_t)length<sizeof(temporary)/sizeof(*temporary)){
        HANDLE old=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        int valid=old==INVALID_HANDLE_VALUE && GetLastError()==ERROR_FILE_NOT_FOUND;
        if(old!=INVALID_HANDLE_VALUE){DWORD got=0;
            valid=GetFileSize(old,NULL)==sizeof(payload) && ReadFile(old,payload,sizeof(payload),&got,NULL) && got==sizeof(payload);
            CloseHandle(old);
        }
        if(valid && own_read(NULL,save+8,payload,0x1ba8)){
            unsigned short version=0;memcpy(&version,payload,2);
            HANDLE file=version==22?CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL):INVALID_HANDLE_VALUE;
            if(file!=INVALID_HANDLE_VALUE){DWORD wrote=0;
                int ok=WriteFile(file,payload,sizeof(payload),&wrote,NULL) && wrote==sizeof(payload) && FlushFileBuffers(file);
                CloseHandle(file);
                if(ok)committed=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
                if(!committed)DeleteFileW(temporary);
            }
        }
    }
    if(shell)FreeLibrary(shell);
    if(committed){
        unsigned fields[5];char line[192];memcpy(fields,payload+8,sizeof(fields));
        snprintf(line,sizeof(line),"[JNET] local save committed: robe=%u symbol=%u scarf=%u energy=%u level=%u chapter=%u",fields[0],fields[1],fields[2],fields[3],fields[4],payload[0x48]);
        message(line);
    }else message("[JNET] local save commit unavailable; native save will still run");
    jnet_save_serialize_original(save,files);
}
static void (*jnet_save_poll_original)(uintptr_t);
static int jnet_appearance_local_saved(void) {
    unsigned char local[0x1ba8];wchar_t folder[MAX_PATH],path[MAX_PATH];int ok=0,length=-1;
    HMODULE shell=LoadLibraryW(L"shell32.dll");
    HRESULT (WINAPI *folder_path)(HWND,int,HANDLE,DWORD,LPWSTR)=shell?(void *)GetProcAddress(shell,"SHGetFolderPathW"):NULL;
    if(jnet_appearance_save_captured && folder_path && SUCCEEDED(folder_path(NULL,0x1c,NULL,0,folder)) &&
       (length=swprintf(path,sizeof(path)/sizeof(*path),L"%ls\\Annapurna Interactive\\Journey\\Steam\\SAVE.BIN",folder))>=0 &&
       (size_t)length<sizeof(path)/sizeof(*path)){
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
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
            if(!failed_at){
                unsigned level=0;char line[128];memcpy(&level,jnet_appearance_save_expected+24,4);
                snprintf(line,sizeof(line),"[JNET] appearance/checkpoint save: native completed; local payload verified=%d level=%u",local_saved,level);
                message(line);
            }
            if(local_saved && !jnet_save_requested && !jnet_menu_edit.applied.character)jnet_appearance_applied_at=GetTickCount64();
            if(jnet_menu_page==1)strcpy(jnet_appearance_status,local_saved?(jnet_save_requested?"Saving":""):"Save failed");
            failed_at=0;
        }
    }
}
static int jnet_appearance_boot(void) {
    unsigned char flags[2];
    return jnet_chapter_menu && own_read(NULL,jnet_chapter_menu+0x20,flags,2) &&
        jnet_appearance_title(jnet_current_level,flags[0],flags[1]);
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
static int jnet_save_bootstrap(uintptr_t game) {
    uintptr_t save=0,type=0,files=0;unsigned state=0;
    if(jnet_save_bootstrapped){
#ifdef _WIN32
        return jnet_profile_identity && jnet_profile_identity==jnet_steam_account_identity;
#else
        return jnet_profile_identity && jnet_profile_identity==jnet_profile_test_identity;
#endif
    }
    if(!own_read(NULL,game+0x170,&save,8) || !save ||
       !own_read(NULL,save,&type,8) || type!=fault_image+0x5e3178 ||
       !own_read(NULL,game+0x50,&files,8) || !files ||
       !own_read(NULL,files+0x200bc,&state,4) || state!=1)return 0;
#ifdef _WIN32
    jnet_save_restore_local(save+8);
#endif
    jnet_appearance_loaded(save);
    if(!jnet_appearance_has_save)return 0;
    if(!jnet_profile_restore(save))return 0;
    jnet_save_bootstrapped=1;
    return 1;
}
static void jnet_appearance_game(uintptr_t game) {
    if(!jnet_save_bootstrap(game))return;
    static uintptr_t observed_barn;
    static unsigned observed_level=~0u;
    unsigned char loading=0;
    if(own_read(NULL,game+0x1c0330,&loading,1) && loading)observed_barn=0;
    uintptr_t barn=0,save=0;unsigned values[4],profile_update=0;
    if(jnet_appearance_boot() && jnet_appearance_has_save)
        jnet_appearance_observe(&jnet_menu_edit,jnet_appearance_saved[0],jnet_appearance_saved[2],jnet_appearance_saved[1],0);
    if(!jnet_appearance_context(game,&barn,&save)){
        if(jnet_menu_edit.awaiting)strcpy(jnet_appearance_status,"Waiting for character");
        return;
    }
    if(
       !own_read(NULL,barn+0x118,values,sizeof(values)) ||
       values[0]>JNET_SCARF_MAX || values[1]>JNET_SCARF_MAX || values[2]>6 || values[3]>20 ||
       (values[0]>values[1] && !jnet_menu_edit.applied.locked_scarf && !jnet_menu_edit.applied.infinite_scarf))return;
    int boot=jnet_appearance_boot();
    if(boot)observed_barn=0;
    else if(barn!=observed_barn || jnet_current_level!=observed_level){
        if(jnet_appearance_has_save && !jnet_menu_edit.awaiting){
            unsigned restored[4]={jnet_appearance_saved_energy,jnet_appearance_saved[2],jnet_appearance_saved[0],jnet_appearance_saved[1]};
            if(!jc_local_windows_write(barn+0x118,restored,sizeof(restored)))return;
            memcpy(values,restored,sizeof(values));
        }
        observed_barn=barn;observed_level=jnet_current_level;jnet_save_requested=1;profile_update=1;
    }
    if(!jnet_appearance_initialized){
        jnet_appearance_observe(&jnet_menu_edit,values[2],values[1],values[3],1);
        jnet_appearance_initialized=1;
    }
    if(jnet_menu_edit.awaiting){
        unsigned changed[4]={jnet_menu_edit.pending.scarf_segments,jnet_menu_edit.pending.scarf_segments,jnet_menu_edit.pending.outfit,jnet_menu_edit.pending.symbol};
        int ok=changed[1]<=JNET_SCARF_MAX && changed[2]<=6 && changed[3]<=20 &&
            jc_local_windows_write(barn+0x118,changed,sizeof(changed));
        if(ok && !(jnet_character_apply_native?jnet_character_apply_native(game,barn,jnet_menu_edit.pending.character):!jnet_menu_edit.pending.character)){
            jc_local_windows_write(barn+0x118,values,sizeof(values));
            ok=0;
        }
        jnet_appearance_complete(&jnet_menu_edit,ok);
        if(ok){profile_update=1;jnet_appearance_applied_at=0;jnet_appearance_saved[0]=changed[2];jnet_appearance_saved[1]=changed[3];jnet_appearance_saved[2]=changed[1];jnet_appearance_saved_energy=changed[0];jnet_appearance_has_save=1;memcpy(values,changed,sizeof(values));jnet_save_requested=1;strcpy(jnet_appearance_status,jnet_menu_edit.applied.character?"Applying character":"Saving");}
        else strcpy(jnet_appearance_status,"Could not apply appearance");
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
    if(!boot){
        if(!jnet_appearance_has_save || jnet_appearance_saved[0]!=values[2] ||
           jnet_appearance_saved[1]!=values[3] || jnet_appearance_saved[2]!=values[1]){
            profile_update=1;jnet_save_requested=1;
        }
        jnet_appearance_observe(&jnet_menu_edit,values[2],values[1],values[3],0);
        jnet_appearance_saved[0]=values[2];jnet_appearance_saved[1]=values[3];jnet_appearance_saved[2]=values[1];
        jnet_appearance_saved_energy=values[0];jnet_appearance_has_save=1;
    }
    if(profile_update || (!jnet_profile_present && jnet_save_requested) || jnet_profile_dirty){
        unsigned checkpoint_values[4]={values[0],values[1],values[2],values[3]};
        if(boot && jnet_appearance_has_save){
            checkpoint_values[0]=jnet_appearance_saved_energy;checkpoint_values[1]=jnet_appearance_saved[2];
            checkpoint_values[2]=jnet_appearance_saved[0];checkpoint_values[3]=jnet_appearance_saved[1];
        }
        if(!jnet_profile_commit(save,checkpoint_values,jnet_current_level,boot)){
            strcpy(jnet_appearance_status,"Save failed");return;
        }
    }
    if(jnet_save_requested && !jnet_appearance_saving){
        uintptr_t files=0;unsigned state=0;unsigned char disabled=1,enabled=0;
        if(!own_read(NULL,game+0x50,&files,8) || !files ||
           !own_read(NULL,files+0x200bc,&state,4) || state!=1 ||
           !own_read(NULL,files+0x202ad,&disabled,1) || disabled ||
           !own_read(NULL,fault_image+0x3c9420e,&enabled,1) || !enabled)return;
        unsigned level=jnet_current_level;
        if(boot && !own_read(NULL,save+0x20,&level,4))return;
        unsigned char chapter=level==0,previous_chapter=0;
        if(!own_read(NULL,save+0x50,&previous_chapter,1))return;
        unsigned saved_values[4]={values[0],values[1],values[2],values[3]};
        if(boot && jnet_appearance_has_save){
            saved_values[0]=jnet_appearance_saved_energy;saved_values[1]=jnet_appearance_saved[2];
            saved_values[2]=jnet_appearance_saved[0];saved_values[3]=jnet_appearance_saved[1];
        }
        if(level>=12 || !jc_local_windows_write(save+0x20,&level,4) ||
           !jc_local_windows_write(save+0x19a8,saved_values+1,4))return;
        if(boot && !jc_local_windows_write(barn+0x118,saved_values,sizeof(saved_values)))return;
        if(!boot && !jc_local_windows_write(save+0x50,&chapter,1)){
            if(boot)jc_local_windows_write(barn+0x118,values,sizeof(values));
            return;
        }
        void (*save_game)(uintptr_t)=(void *)(fault_image+JC_RVA_SaveGame);
        save_game(game);
        if(boot)jc_local_windows_write(barn+0x118,values,sizeof(values));
        if(boot)jc_local_windows_write(save+0x50,&previous_chapter,1);
        if(own_read(NULL,files+0x200bc,&state,4) && state==2){
            jnet_appearance_save_captured=own_read(NULL,files+0xb8,jnet_appearance_save_expected,sizeof(jnet_appearance_save_expected));
            jnet_save_requested=0;
            jnet_appearance_saving=1;
        }
    }
}
#endif
