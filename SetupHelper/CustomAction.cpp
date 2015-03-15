
#include "stdafx.h"
#define SIZEOF(x)  (sizeof(x) / sizeof(x[0]))

static void logError(HRESULT hr, char*func) {
    DWORD err = GetLastError();
    TCHAR *lpMsgBuf = NULL;
    if (!err)
        err = hr;
    if (err) {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
    }
    if (lpMsgBuf) {
        WcaLogError(hr, "%s failed: %ls", func, lpMsgBuf);
        LocalFree(lpMsgBuf);
    } else {
        WcaLogError(hr, "%s failed.", func);
    }
}

UINT __stdcall addAppInitEntry(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;
    UINT er = ERROR_SUCCESS;
    TCHAR *dllPathLong = NULL, dllPathShort[4096], buf[4096], *lpMsgBuf = NULL;
    DWORD buflen = SIZEOF(dllPathLong), i, res;
    HKEY hkey = NULL;

    hr = WcaInitialize(hInstall, "addAppInitEntry");
    ExitOnFailure(hr, "Failed to initialize");
    
    hr = WcaGetProperty(L"CustomActionData", &dllPathLong);
    ExitOnFailure(hr, "Failed to get CustomActionData.");
    WcaLog(LOGMSG_STANDARD, "addAppInitEntry v1. CustomActionData=\"%ls\"", dllPathLong);
    dllPathShort[0] = 0;
    if (!GetShortPathName(dllPathLong, dllPathShort, SIZEOF(dllPathShort))) {
        logError(0, "GetShortPathName"); hr = -1; goto LExit;
    }
    //MessageBox(NULL, dllPathShort, L"addAppInitEntry", MB_OK);
    if (!dllPathShort[0])
        ExitOnFailure(hr=-1, "dllPathShort is empty");
    if (res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, KEY_ALL_ACCESS, &hkey)) {
        logError(res, "RegOpenKey"); hr = -1; goto LExit;
    }
    i = 1;
    if (res=RegSetValueEx(hkey, L"LoadAppInit_DLLs", 0, REG_DWORD, (LPBYTE)&i, sizeof(i))) {
        logError(res,"RegSetValue(LoadAppInit_DLLs)"); hr = -1; goto LExit;
    }
    i = 0;
    if (res=RegSetValueEx(hkey, L"RequireSignedAppInit_DLLs", 0, REG_DWORD, (LPBYTE)&i, sizeof(i))) {
        logError(res,"RegSetValue(RequireSignedAppInit_DLLs)"); hr = -1; goto LExit;
    }
    buflen = SIZEOF(buf);
    buf[0]=0;
    if (res=RegQueryValueEx(hkey, L"AppInit_DLLs", 0, &i, (LPBYTE)buf, &buflen))
        logError(res, "RegQueryValue(AppInit_DLLs)");
    else
        WcaLog(LOGMSG_STANDARD, "AppInit_DLLs=\"%ls\"", buf);
    if (_tcsstr(buf, dllPathShort))
        WcaLog(LOGMSG_STANDARD, "Error: AppInit_DLLs already contains \"%ls\"", dllPathShort);
    else {
        if (buf[0])
            _tcsncat_s(buf, L" ", SIZEOF(buf));
        _tcsncat_s(buf, dllPathShort, SIZEOF(buf));
        WcaLog(LOGMSG_STANDARD, "Setting AppInit_DLLs=\"%ls\"", buf);
        if (res=RegSetValueEx(hkey, L"AppInit_DLLs", 0, REG_SZ, (LPBYTE)buf, (DWORD)((_tcslen(buf) + 1) * 2))) {
            logError(res,"RegSetValue(AppInit_DLLs)"); hr = -1; goto LExit;
        }
        WcaLog(LOGMSG_STANDARD, "Registry updated.");
    }
    hr = 0;
LExit:
    ReleaseStr(dllPathLong);
    if (hkey) RegCloseKey(hkey);
    if (lpMsgBuf) LocalFree(lpMsgBuf);
    er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}

UINT __stdcall removeAppInitEntry(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;
    UINT er = ERROR_SUCCESS;
    TCHAR *dllPathLong = NULL, dllPathShort[4096], buf[4096], *pbuf1, *pbuf2, *lpMsgBuf = NULL;
    DWORD buflen = SIZEOF(dllPathLong), i, res;
    size_t j;
    HKEY hkey = NULL;

    hr = WcaInitialize(hInstall, "removeAppInitEntry");
    ExitOnFailure(hr, "Failed to initialize");
    
    hr = WcaGetProperty(L"CustomActionData", &dllPathLong);
    ExitOnFailure(hr, "Failed to get CustomActionData.");
    WcaLog(LOGMSG_STANDARD, "removeAppInitEntry v1. CustomActionData=\"%ls\"", dllPathLong);
    dllPathShort[0] = 0;
    if (!GetShortPathName(dllPathLong, dllPathShort, SIZEOF(dllPathShort))) {
        logError(0, "GetShortPathName"); hr = -1; goto LExit;
    }
    //MessageBox(NULL, dllPathShort, L"removeAppInitEntry", MB_OK);
    if (!dllPathShort[0])
        ExitOnFailure(hr=-1, "dllPathShort is empty");
    if (res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, KEY_ALL_ACCESS, &hkey)) {
        logError(res, "RegOpenKey"); hr = -1; goto LExit;
    }
    buflen = SIZEOF(buf);
    buf[0]=0;
    if (res=RegQueryValueEx(hkey, L"AppInit_DLLs", 0, &i, (LPBYTE)buf, &buflen))
        logError(res, "RegQueryValue(AppInit_DLLs)");
    else
        WcaLog(LOGMSG_STANDARD, "AppInit_DLLs=\"%ls\"", buf);
    if (!(pbuf1=_tcsstr(buf, dllPathShort)))
        WcaLog(LOGMSG_STANDARD, "Error: AppInit_DLLs does not contain \"%ls\"", dllPathShort);
    else {
        for(pbuf2=pbuf1+_tcslen(dllPathShort); pbuf1>buf && pbuf1[-1]==' '; pbuf1--)
            ;
        if (pbuf2>buf)
            for (j=pbuf1-buf; j<4096 && pbuf2[-1]; j++)
                *pbuf1++ = *pbuf2++;
        WcaLog(LOGMSG_STANDARD, "Setting AppInit_DLLs=\"%ls\"", buf);
        if (res=RegSetValueEx(hkey, L"AppInit_DLLs", 0, REG_SZ, (LPBYTE)buf, (DWORD)((_tcslen(buf) + 1) * 2))) {
            logError(res,"RegSetValue(AppInit_DLLs)"); hr = -1; goto LExit;
        }
        WcaLog(LOGMSG_STANDARD, "Registry updated.");
    }
    hr = 0;
LExit:
    ReleaseStr(dllPathLong);
    if (hkey) RegCloseKey(hkey);
    if (lpMsgBuf) LocalFree(lpMsgBuf);
    er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}


// DllMain - Initialize and cleanup WiX custom action utils.
extern "C" BOOL WINAPI DllMain(
        __in HINSTANCE hInst,
        __in ULONG ulReason,
        __in LPVOID
        )
{
    switch(ulReason)
    {
    case DLL_PROCESS_ATTACH:
        WcaGlobalInitialize(hInst);
        break;

    case DLL_PROCESS_DETACH:
        WcaGlobalFinalize();
        break;
    }

    return TRUE;
}
