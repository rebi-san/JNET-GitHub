#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef void *HMODULE;
typedef unsigned DWORD;
#define JC_PATH_CAP 32768
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
static const wchar_t *module_path,*selected_path=L"",*game_path;
static DWORD GetModuleFileNameW(HMODULE m,wchar_t *out,DWORD cap){(void)m;assert(wcslen(module_path)<cap);wcscpy(out,module_path);return (DWORD)wcslen(out);}
static DWORD GetPrivateProfileStringW(const wchar_t *section,const wchar_t *key,const wchar_t *fallback,wchar_t *out,DWORD cap,const wchar_t *path){
    (void)section;(void)key;(void)fallback;(void)path;assert(wcslen(selected_path)<cap);wcscpy(out,selected_path);return (DWORD)wcslen(out);
}
static DWORD GetFileAttributesW(const wchar_t *path){return wcscmp(path,game_path)?INVALID_FILE_ATTRIBUTES:0;}
static DWORD GetLastError(void){return ERROR_FILE_NOT_FOUND;}
#include "jnet_paths_under_test.h"
int main(void){
    wchar_t directory[JC_PATH_CAP],game[JC_PATH_CAP];
    const wchar_t *roots[]={L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Journey",L"D:\\My Steam Library\\steamapps\\common\\Journey",L"E:\\Games\\蒸汽\\steamapps\\common\\Journey"};
    wchar_t module[JC_PATH_CAP],expected[JC_PATH_CAP];
    for(unsigned i=0;i<3;++i)for(unsigned nested=0;nested<2;++nested){
        swprintf(module,JC_PATH_CAP,L"%ls%ls\\JNET.exe",roots[i],nested?L"\\JNET":L"");
        swprintf(expected,JC_PATH_CAP,L"%ls\\Journey.exe",roots[i]);
        module_path=module;game_path=expected;
        assert(jc_paths(NULL,directory,game) && !wcscmp(game,expected));
    }
    selected_path=L"F:\\Optional Sound Fix\\Journey.exe";
    assert(jc_paths(NULL,directory,game) && !wcscmp(game,selected_path));
    selected_path=L"relative\\Journey.exe";assert(!jc_paths(NULL,directory,game));
    puts("Portable paths: different drives, spaces, Unicode, direct/nested layout and explicit selection passed.");
}
