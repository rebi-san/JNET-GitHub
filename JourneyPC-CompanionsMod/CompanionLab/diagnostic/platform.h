#ifndef JC_PLATFORM_H
#define JC_PLATFORM_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define JC_PATH_CAP 32768

static inline int jc_same_file(const wchar_t *a, const wchar_t *b) {
    HANDLE left = CreateFileW(a, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, 0, NULL);
    HANDLE right = CreateFileW(b, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, 0, NULL);
    BY_HANDLE_FILE_INFORMATION x, y;
    int same = left != INVALID_HANDLE_VALUE && right != INVALID_HANDLE_VALUE &&
        GetFileInformationByHandle(left, &x) && GetFileInformationByHandle(right, &y) &&
        (x.nFileIndexHigh || x.nFileIndexLow) && (y.nFileIndexHigh || y.nFileIndexLow) &&
        x.dwVolumeSerialNumber == y.dwVolumeSerialNumber &&
        x.nFileIndexHigh == y.nFileIndexHigh && x.nFileIndexLow == y.nFileIndexLow;
    if (left != INVALID_HANDLE_VALUE) CloseHandle(left);
    if (right != INVALID_HANDLE_VALUE) CloseHandle(right);
    return same;
}

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

static inline int jc_hash(const wchar_t *path, unsigned char result[32]) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    unsigned char buffer[65536];
    DWORD read = 0;
    int success = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) goto done;
    if (BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) < 0) goto done;
    for (;;) {
        if (!ReadFile(file, buffer, sizeof(buffer), &read, NULL)) goto done;
        if (!read) break;
        if (BCryptHashData(hash, buffer, read, 0) < 0) goto done;
    }
    success = BCryptFinishHash(hash, result, 32, 0) >= 0;
done:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return success;
}

static inline int jc_readable(const void *address, size_t length) {
    MEMORY_BASIC_INFORMATION info;
    uintptr_t start = (uintptr_t)address;
    if (!address || !length || start + length < start) return 0;
    if (!VirtualQuery(address, &info, sizeof(info))) return 0;
    if (info.State != MEM_COMMIT || info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return 0;
    if (!(info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                         PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return 0;
    return start >= (uintptr_t)info.BaseAddress &&
        length <= info.RegionSize && start - (uintptr_t)info.BaseAddress <= info.RegionSize - length;
}

#endif
