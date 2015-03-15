/*
 * NoBuzz v1.0
 *
 * Copyright (c) 2015, rustyx.org
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
#include "mhook/mhook-lib/mhook.h"

int unhookLoadLibrary();

HANDLE hfile = INVALID_HANDLE_VALUE;
char modname[256];
int logRecCount;

#ifdef _M_X64
    #define ARCH_BITS_STR " (x64)"
#else
    #define ARCH_BITS_STR ""
#endif

typedef HMODULE(WINAPI *P_LOAD_LIBRARY_A)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI *P_LOAD_LIBRARY_W)(LPWSTR lpLibFileName);
typedef MMRESULT(WINAPI *P_TIME_BEGIN_PERIOD)(UINT uPeriod);
HMODULE hKernel = ::GetModuleHandle(L"kernel32");
HMODULE hWinMM;
P_LOAD_LIBRARY_A origLoadLibraryA;
P_LOAD_LIBRARY_W origLoadLibraryW;
P_TIME_BEGIN_PERIOD origTimeBeginPeriod;
P_TIME_BEGIN_PERIOD origTimeEndPeriod;

void openLogFile()
{
    if (hfile == INVALID_HANDLE_VALUE) {
        char tempPath[1024];
        char buf[1024];
        GetTempPathA((DWORD)sizeof(tempPath), tempPath);
        _snprintf_s(buf, sizeof(buf), "%snobuzz.log", tempPath);
        hfile = CreateFileA(buf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
}

MMRESULT WINAPI hookedTimeBeginPeriod(UINT uPeriod)
{
    if (hfile != INVALID_HANDLE_VALUE && logRecCount++ < 10) {
        char buf[256];
        DWORD n;
        _snprintf_s(buf, sizeof(buf), "%s : timeBeginPeriod(%d)\r\n", modname, uPeriod);
        SetFilePointer(hfile, 0, NULL, FILE_END);
        WriteFile(hfile, buf, (DWORD)strlen(buf), &n, NULL);
    }
    if (uPeriod < 16) {
        return TIMERR_NOERROR;
    //    return TIMERR_NOCANDO;
    }
    return origTimeBeginPeriod(uPeriod);
}

MMRESULT WINAPI hookedTimeEndPeriod(UINT uPeriod)
{
    if (uPeriod < 16) {
        return TIMERR_NOERROR;
        //return TIMERR_NOCANDO;
    }
    return origTimeEndPeriod(uPeriod);
}

int hookWinMM(LPCSTR originA, LPWSTR originW)
{
    if (origTimeBeginPeriod && origTimeEndPeriod)
        return 0;
    hWinMM = ::GetModuleHandle(L"winmm");
    if (hWinMM) {
        origTimeBeginPeriod = (P_TIME_BEGIN_PERIOD)::GetProcAddress(hWinMM, "timeBeginPeriod");
        origTimeEndPeriod = (P_TIME_BEGIN_PERIOD)::GetProcAddress(hWinMM, "timeEndPeriod");
    }
    if (origTimeBeginPeriod && origTimeEndPeriod) {
        char buf[1024];
        DWORD n;
        openLogFile();
        if (hfile != INVALID_HANDLE_VALUE) {
            if (originA)
                _snprintf_s(buf, sizeof(buf), "Attached" ARCH_BITS_STR ": %s (after %s)\r\n", modname, originA);
            else if (originW)
                _snprintf_s(buf, sizeof(buf), "Attached" ARCH_BITS_STR ": %s (after %ls)\r\n", modname, originW);
            else
                _snprintf_s(buf, sizeof(buf), "Attached" ARCH_BITS_STR ": %s\r\n", modname);
            SetFilePointer(hfile, 0, NULL, FILE_END);
            WriteFile(hfile, buf, (DWORD)strlen(buf), &n, NULL);
        }
        Mhook_SetHook((PVOID*)&origTimeBeginPeriod, hookedTimeBeginPeriod);
        Mhook_SetHook((PVOID*)&origTimeEndPeriod, hookedTimeEndPeriod);
        return 1;
    }
    return 0;
}

int unhookWinMM()
{
    if (!origTimeBeginPeriod)
        return 0;
    Mhook_Unhook((PVOID*)&origTimeBeginPeriod);
    if (origTimeEndPeriod)
        Mhook_Unhook((PVOID*)&origTimeEndPeriod);
    origTimeBeginPeriod = origTimeEndPeriod = NULL;
    return 1;
}

HMODULE WINAPI hookedLoadLibraryA(LPCSTR lpLibFileName)
{
    HMODULE m = origLoadLibraryA(lpLibFileName);
    if (!origTimeBeginPeriod) {
        if (hookWinMM(lpLibFileName, NULL))
            unhookLoadLibrary();
    }
    return m;
}

HMODULE WINAPI hookedLoadLibraryW(LPWSTR lpLibFileName)
{
    HMODULE m = origLoadLibraryW(lpLibFileName);
    if (!origTimeBeginPeriod) {
        if (hookWinMM(NULL, lpLibFileName))
            unhookLoadLibrary();
    }
    return m;
}

int hookLoadLibrary()
{
    openLogFile();
    if (origLoadLibraryA)
        return 0;
    origLoadLibraryA = (P_LOAD_LIBRARY_A)::GetProcAddress(hKernel, "LoadLibraryA");
    origLoadLibraryW = (P_LOAD_LIBRARY_W)::GetProcAddress(hKernel, "LoadLibraryW");
    if (!origLoadLibraryA || !origLoadLibraryW)
        return 0;
    else {
        Mhook_SetHook((PVOID*)&origLoadLibraryA, hookedLoadLibraryA);
        Mhook_SetHook((PVOID*)&origLoadLibraryW, hookedLoadLibraryW);
        return 1;
    }
}

int unhookLoadLibrary()
{
    if (!origLoadLibraryA || !origLoadLibraryW)
        return 0;
    Mhook_Unhook((PVOID*)&origLoadLibraryA);
    Mhook_Unhook((PVOID*)&origLoadLibraryW);
    origLoadLibraryA = NULL;
    origLoadLibraryW = NULL;
    return 1;
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
        GetModuleFileNameA(NULL, modname, (DWORD)sizeof(modname));
        if (!hookWinMM(NULL, NULL))
            hookLoadLibrary();
        break;

    case DLL_PROCESS_DETACH:
        unhookLoadLibrary();
        unhookWinMM();
        if (hfile != INVALID_HANDLE_VALUE) {
            CloseHandle(hfile);
            hfile = 0;
        }
        break;
    }

    return TRUE;
}
