#include <wchar.h>
#include <string.h>
#include <assert.h>
typedef unsigned long DWORD;
typedef void *HMODULE;
#define JC_PATH_CAP 1024
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
static int mode;
static DWORD GetModuleFileNameW(HMODULE m,wchar_t *p,DWORD cap){(void)m;(void)cap;wcscpy(p,L"C:\\Journey\\Journet\\JNET.exe");return wcslen(p);}
static DWORD GetFileAttributesW(const wchar_t *p){(void)p;return mode==1?0:INVALID_FILE_ATTRIBUTES;}
static DWORD GetLastError(void){return mode==2?5:ERROR_FILE_NOT_FOUND;}
static const wchar_t *choice=L"";
static DWORD GetPrivateProfileStringW(const wchar_t *s,const wchar_t *k,const wchar_t *d,wchar_t *out,DWORD cap,const wchar_t *file){(void)s;(void)k;(void)d;(void)file;wcsncpy(out,choice,cap-1);out[cap-1]=0;return wcslen(out);}
static inline int jc_parent(wchar_t *path) {
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash) return 0;
    *slash = 0;
    return 1;
}

static inline int jc_paths(HMODULE module, wchar_t *directory, wchar_t *game) {
    DWORD length = GetModuleFileNameW(module, directory, JC_PATH_CAP);
    if (!length || length >= JC_PATH_CAP || !jc_parent(directory)) return 0;
    wchar_t settings[JC_PATH_CAP];
    if (swprintf(settings,JC_PATH_CAP,L"%ls\\JNET-launch.ini",directory)<0) return 0;
    DWORD selected=GetPrivateProfileStringW(L"Launch",L"Executable",L"",game,JC_PATH_CAP,settings);
    if (selected) return selected < JC_PATH_CAP-1 &&
        ((game[0] && game[1]==L':' && game[2]==L'\\') || (game[0]==L'\\' && game[1]==L'\\'));
    wcscpy(game, directory);
    if (wcslen(game) + 13 >= JC_PATH_CAP) return 0;
    wcscat(game, L"\\Journey.exe");
    if (GetFileAttributesW(game) == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) return 0;
        wcscpy(game, directory);
        if (!jc_parent(game)) return 0;
        wcscat(game, L"\\Journey.exe");
    }
    return 1;
}

#include "diagnostic-build/target_build.h"
int main(void){wchar_t d[1024],g[1024];
mode=0;assert(jc_paths(0,d,g));assert(!wcscmp(g,L"C:\\Journey\\Journey.exe"));
mode=1;assert(jc_paths(0,d,g));assert(!wcscmp(g,L"C:\\Journey\\Journet\\Journey.exe"));
mode=2;assert(!jc_paths(0,d,g));
assert(jc_executable_size(jc_game_hash)==JC_ORIGINAL_IMAGE_SIZE);
assert(jc_executable_size(jc_audio_hash)==JC_IMAGE_SIZE);
choice=L"Z:\\Downloads\\Journey-sound-fix-0.3.1.exe";assert(jc_paths(0,d,g));assert(!wcscmp(g,choice));
choice=L"relative.exe";assert(!jc_paths(0,d,g));
unsigned char bad[32]={0};assert(!jc_executable_size(bad));return 0;}
