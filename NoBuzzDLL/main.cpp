/*
 * NoBuzz
 *
 * Copyright (c) 2015-2016, rustyx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "stdafx.h"
#include "minhook/include/MinHook.h"

int unhookLoadLibrary();

HANDLE hfile = INVALID_HANDLE_VALUE;
wchar_t modname[256];
int logRecCount;

typedef HMODULE(WINAPI *P_LOAD_LIBRARY_A)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI *P_LOAD_LIBRARY_W)(LPWSTR lpLibFileName);
typedef HMODULE(WINAPI *P_LOAD_LIBRARY_EX_A)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI *P_LOAD_LIBRARY_EX_W)(LPWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef MMRESULT(WINAPI *P_TIME_BEGIN_PERIOD)(UINT uPeriod);
HMODULE hKernel = ::GetModuleHandleW(L"kernel32");
HMODULE hWinMM;
P_LOAD_LIBRARY_A pLoadLibraryA;    // original address of the function, calls the hooked API after hooking
P_LOAD_LIBRARY_A origLoadLibraryA; // address of the trampoline to the original function (use to call the original API)
P_LOAD_LIBRARY_W pLoadLibraryW;
P_LOAD_LIBRARY_W origLoadLibraryW;
P_LOAD_LIBRARY_EX_A pLoadLibraryExA;
P_LOAD_LIBRARY_EX_A origLoadLibraryExA;
P_LOAD_LIBRARY_EX_W pLoadLibraryExW;
P_LOAD_LIBRARY_EX_W origLoadLibraryExW;
P_TIME_BEGIN_PERIOD pTimeBeginPeriod;
P_TIME_BEGIN_PERIOD origTimeBeginPeriod;
P_TIME_BEGIN_PERIOD pTimeEndPeriod;
P_TIME_BEGIN_PERIOD origTimeEndPeriod;

void openLogFile()
{
    if (hfile == INVALID_HANDLE_VALUE) {
        wchar_t tempPath[1024];
        wchar_t buf[1024];
        GetTempPathW(_countof(tempPath), tempPath);
        swprintf_s(buf, L"%snobuzz.log", tempPath);
        hfile = CreateFileW(buf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
}

void log(const char*fmt, ...)
{
    char buf[4096];
    int ofst;
#ifdef _M_X64
    ofst = 6;
    strcpy_s(buf, "(x64) ");
#else
    ofst = 6;
    strcpy_s(buf, "(x86) ");
#endif
    va_list argp;
    va_start(argp, fmt);
    _vsnprintf(buf + ofst, sizeof(buf) - ofst - 4, fmt, argp);
    va_end(argp);
    DWORD n, len = (DWORD)strlen(buf);
    buf[len++] = '\r';
    buf[len++] = '\n';
    buf[len] = 0;
    openLogFile();
    if (hfile != INVALID_HANDLE_VALUE) {
        SetFilePointer(hfile, 0, NULL, FILE_END);
        WriteFile(hfile, buf, len, &n, NULL);
    }
}

MMRESULT WINAPI hookedTimeEndPeriod(UINT uPeriod)
{
    if (uPeriod < 16) {
        return TIMERR_NOERROR;
        //return TIMERR_NOCANDO;
    }
    return origTimeEndPeriod(uPeriod);
}

template <typename T>
inline bool hookApi(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppTarget, T** ppOriginal)
{
    MH_STATUS rc = MH_CreateHookApiEx(pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal), reinterpret_cast<LPVOID*>(ppTarget));
    if (rc != MH_OK) {
        //if (rc != MH_ERROR_MODULE_NOT_FOUND && rc != MH_ERROR_FUNCTION_NOT_FOUND)
        log("MH_CreateHook for %s failed: %d", pszProcName, rc);
        return false;
    }
    rc = MH_EnableHook(reinterpret_cast<LPVOID*>(*ppTarget));
    if (rc != MH_OK) {
        log("MH_EnableHook for %s failed: %d", pszProcName, rc);
        return false;
    }
    return true;
}

template <typename T>
inline bool unhookApi(LPCSTR pszProcName, T** ppTarget)
{
    if (!*ppTarget)
        return true;
    MH_STATUS rc = MH_DisableHook(reinterpret_cast<LPVOID*>(*ppTarget));
    if (rc != MH_OK) {
        log("MH_DisableHook for %s failed: %d", pszProcName, rc);
        return false;
    }
    *ppTarget = nullptr;
    return true;
}

MMRESULT WINAPI hookedTimeBeginPeriod(UINT uPeriod)
{
    if (hfile != INVALID_HANDLE_VALUE && logRecCount++ < 10) {
        log("%ls : timeBeginPeriod(%d)", modname, uPeriod);
    }
    if (uPeriod < 16) {
        return TIMERR_NOERROR;
        //return TIMERR_NOCANDO;
    }
    return origTimeBeginPeriod(uPeriod);
}

int hookWinMM(LPCSTR originA, LPWSTR originW)
{
    if (pTimeBeginPeriod)
        return 0;
    hWinMM = ::GetModuleHandleW(L"winmm");
    if (hWinMM && !pTimeBeginPeriod && ::GetProcAddress(hWinMM, "timeBeginPeriod")) {
        if (originA)
            log("Attached: %ls (after %s)", modname, originA);
        else if (originW)
            log("Attached: %ls (after %ls)", modname, originW);
        else
            log("Attached: %ls", modname);
        bool hooked = hookApi(L"winmm", "timeBeginPeriod", hookedTimeBeginPeriod, &pTimeBeginPeriod, &origTimeBeginPeriod);
        hookApi(L"winmm", "timeEndPeriod", hookedTimeEndPeriod, &pTimeEndPeriod, &origTimeEndPeriod);
        return hooked;
    }
    return 0;
}

int unhookWinMM()
{
    if (!pTimeBeginPeriod)
        return 0;
    unhookApi("timeBeginPeriod", &pTimeBeginPeriod);
    unhookApi("timeEndPeriod", &pTimeEndPeriod);
    return 1;
}

HMODULE WINAPI hookedLoadLibraryA(LPCSTR lpLibFileName)
{
    HMODULE m = origLoadLibraryA(lpLibFileName);
    if (!pTimeBeginPeriod) {
        if (hookWinMM(lpLibFileName, NULL))
            unhookLoadLibrary();
    }
    return m;
}

HMODULE WINAPI hookedLoadLibraryW(LPWSTR lpLibFileName)
{
    HMODULE m = origLoadLibraryW(lpLibFileName);
    if (!pTimeBeginPeriod) {
        if (hookWinMM(NULL, lpLibFileName))
            unhookLoadLibrary();
    }
    return m;
}

HMODULE WINAPI hookedLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE m = origLoadLibraryExA(lpLibFileName, hFile, dwFlags);
    if (!pTimeBeginPeriod) {
        if (hookWinMM(lpLibFileName, NULL))
            unhookLoadLibrary();
    }
    return m;
}

HMODULE WINAPI hookedLoadLibraryExW(LPWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE m = origLoadLibraryExW(lpLibFileName, hFile, dwFlags);
    if (!pTimeBeginPeriod) {
        if (hookWinMM(NULL, lpLibFileName))
            unhookLoadLibrary();
    }
    return m;
}

int hookLoadLibrary()
{
    if (origLoadLibraryA || !::GetProcAddress(hKernel, "LoadLibraryA"))
        return 0;
    openLogFile();
    bool hooked = hookApi(L"kernel32", "LoadLibraryA", hookedLoadLibraryA, &pLoadLibraryA, &origLoadLibraryA);
    hookApi(L"kernel32", "LoadLibraryW", hookedLoadLibraryW, &pLoadLibraryW, &origLoadLibraryW);
    hookApi(L"kernel32", "LoadLibraryExA", hookedLoadLibraryExA, &pLoadLibraryExA, &origLoadLibraryExA);
    return hooked;
}

int unhookLoadLibrary()
{
    int rc = 0;
    unhookApi("LoadLibraryA", &pLoadLibraryA);
    unhookApi("LoadLibraryW", &pLoadLibraryW);
    unhookApi("LoadLibraryExA", &pLoadLibraryExA);
    return rc;
}

bool init()
{
    GetModuleFileNameW(NULL, modname, 256);
    if (MH_Initialize() != MH_OK) {
        return false;
    }
    if (!hookWinMM(NULL, NULL))
        hookLoadLibrary();
    return true;
}

void deinit()
{
    unhookLoadLibrary();
    unhookWinMM();
    if (hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(hfile);
        hfile = 0;
    }
    MH_Uninitialize();
}

BOOL WINAPI DllMain(
    __in HINSTANCE  hInstance,
    __in DWORD      Reason,
    __in LPVOID     Reserved
    )
{        
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        init();
        break;

    case DLL_PROCESS_DETACH:
        deinit();
        break;
    }

    return TRUE;
}
