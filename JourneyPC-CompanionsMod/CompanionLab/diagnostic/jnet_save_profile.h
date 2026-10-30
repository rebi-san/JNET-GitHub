#ifndef JNET_SAVE_PROFILE_H
#define JNET_SAVE_PROFILE_H
struct jnet_save_profile {
    uint64_t identity;
    unsigned outfit,symbol,scarf,energy,level,chapter;
};
static unsigned jnet_profile_hash(const unsigned char *p,size_t n) {
    unsigned h=2166136261u;
    for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;
    return h;
}
static int jnet_profile_valid(const struct jnet_save_profile *p) {
    return p->identity && p->outfit<=6 && p->symbol<=20 && p->scarf<=JNET_SCARF_MAX &&
        p->energy<=p->scarf && p->level<12 && p->chapter<=1;
}
static int jnet_profile_decode(struct jnet_save_profile *p,const unsigned char bytes[48],uint64_t identity) {
    unsigned hash=0;struct jnet_save_profile candidate={0};
    if(memcmp(bytes,"JNTSAVE1",8))return 0;
    memcpy(&hash,bytes+44,4);if(hash!=jnet_profile_hash(bytes,44))return 0;
    memcpy(&candidate.identity,bytes+8,8);
    memcpy(&candidate.outfit,bytes+16,4);memcpy(&candidate.symbol,bytes+20,4);
    memcpy(&candidate.scarf,bytes+24,4);memcpy(&candidate.energy,bytes+28,4);
    memcpy(&candidate.level,bytes+32,4);memcpy(&candidate.chapter,bytes+36,4);
    if(candidate.identity!=identity || !jnet_profile_valid(&candidate))return 0;
    *p=candidate;return 1;
}
static int jnet_profile_encode(unsigned char bytes[48],const struct jnet_save_profile *p) {
    if(!jnet_profile_valid(p))return 0;
    memset(bytes,0,48);memcpy(bytes,"JNTSAVE1",8);memcpy(bytes+8,&p->identity,8);
    memcpy(bytes+16,&p->outfit,4);memcpy(bytes+20,&p->symbol,4);
    memcpy(bytes+24,&p->scarf,4);memcpy(bytes+28,&p->energy,4);
    memcpy(bytes+32,&p->level,4);memcpy(bytes+36,&p->chapter,4);
    unsigned hash=jnet_profile_hash(bytes,44);memcpy(bytes+44,&hash,4);return 1;
}
#ifdef _WIN32
static int jnet_profile_path(wchar_t path[MAX_PATH],uint64_t identity,int create) {
    wchar_t folder[MAX_PATH];int ok=0;
    HMODULE shell=LoadLibraryW(L"shell32.dll");
    HRESULT (WINAPI *get_folder)(HWND,int,HANDLE,DWORD,LPWSTR)=shell?(void *)GetProcAddress(shell,"SHGetFolderPathW"):NULL;
    if(identity && get_folder && SUCCEEDED(get_folder(NULL,0x1c,NULL,0,folder))){
        int n=swprintf(path,MAX_PATH,L"%ls\\JNET",folder);
        if(n>0 && n<MAX_PATH && (!create || CreateDirectoryW(path,NULL) || GetLastError()==ERROR_ALREADY_EXISTS)){
            n=swprintf(path,MAX_PATH,L"%ls\\JNET\\638230-%llu.save",folder,(unsigned long long)identity);
            ok=n>0 && n<MAX_PATH;
        }
    }
    if(shell)FreeLibrary(shell);return ok;
}
static int jnet_profile_read(struct jnet_save_profile *p,uint64_t identity) {
    wchar_t path[MAX_PATH];unsigned char bytes[48];DWORD got=0;int ok=0;
    if(!jnet_profile_path(path,identity,0))return 0;
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return 0;
    if(GetFileSize(f,NULL)==sizeof(bytes) && ReadFile(f,bytes,sizeof(bytes),&got,NULL) && got==sizeof(bytes))
        ok=jnet_profile_decode(p,bytes,identity);
    CloseHandle(f);return ok;
}
static int jnet_profile_write(const struct jnet_save_profile *p) {
    wchar_t path[MAX_PATH],temp[MAX_PATH];unsigned char bytes[48];DWORD wrote=0;int ok=0;
    if(!jnet_profile_encode(bytes,p) || !jnet_profile_path(path,p->identity,1))return 0;
    int n=swprintf(temp,MAX_PATH,L"%ls.tmp",path);if(n<=0 || n>=MAX_PATH)return 0;
    HANDLE f=CreateFileW(temp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return 0;
    ok=WriteFile(f,bytes,sizeof(bytes),&wrote,NULL) && wrote==sizeof(bytes) && FlushFileBuffers(f);
    CloseHandle(f);
    if(ok)ok=MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temp);
    struct jnet_save_profile verified={0};
    return ok && jnet_profile_read(&verified,p->identity) && verified.outfit==p->outfit && verified.symbol==p->symbol &&
        verified.scarf==p->scarf && verified.energy==p->energy && verified.level==p->level && verified.chapter==p->chapter;
}
#endif
#endif
