#include <windows.h>
#include <msi.h>

#include "CustomAction.h"


#pragma comment(lib, "msi.lib")


#ifdef _DEBUG
#pragma comment(lib, "msvcrtd.lib")
#pragma comment(lib, "vcruntimed.lib")
#pragma comment(lib, "ucrtd.lib")
#else //Release
#pragma comment(lib, "msvcrt.lib")
#pragma comment(lib, "vcruntime.lib")
#endif



BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason, LPVOID lpReserved)
{
    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInst);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}


#ifdef __cplusplus
extern "C" {
#endif

    __declspec(dllexport) UINT __stdcall CreateInstallationLogFile(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::createInstallationLogFile(hInstall);
    }

    __declspec(dllexport) UINT __stdcall ExecuteFullCleanup(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::executeFullCleanup(hInstall);
    }

    __declspec(dllexport) UINT __stdcall ExecuteV3Cleanup(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::executeV3Cleanup(hInstall);
    }

    __declspec(dllexport) UINT __stdcall ExecuteV4Cleanup(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::executeV4Cleanup(hInstall);
    }

    __declspec(dllexport) UINT __stdcall CopyConfigFileToDestination(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::copyConfigFileToDestination(hInstall);
    }

    __declspec(dllexport) UINT __stdcall OpenFileChooser(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::openFileChooser(hInstall);
    }

    __declspec(dllexport) UINT __stdcall CopyConfigFiles(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::copyConfigFiles(hInstall);
    }

    __declspec(dllexport) UINT __stdcall RestoreConfigFiles(MSIHANDLE hInstall)
    {
        return WinLogon::CustomActions::CustomActions::restoreConfigFiles(hInstall);
    }

#ifdef __cplusplus
}
#endif