#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace WinLogon::CustomActions::Constants
{
    class PathConstants
    {
    public:
        static inline const std::vector<std::wstring> logonAppFoldersPath = {
            L"C:\\ProgramData\\WatchGuard\\Logon App",
            L"C:\\Program Files\\WatchGuard\\Logon App"
        };

        static inline const std::vector<std::wstring> watchGuardFoldersPath = {
            L"C:\\ProgramData\\WatchGuard",
            L"C:\\Program Files\\WatchGuard"
        };

        // Uninstall
        static inline const std::vector<std::filesystem::path> filesFromV3ToRemove = {
            L"C:\\Windows\\System32\\WLcacert.pem",
            L"C:\\Windows\\System32\\wlconfig.cfg",
            L"C:\\Windows\\System32\\WLlibcurl.dll",
            L"C:\\Windows\\System32\\WLCredProv.dll",
            L"C:\\Windows\\System32\\drivers\\etc\\wlconfig.cfg",
            L"C:\\Windows\\System32\\drivers\\etc\\wlconfigbkp.cfg"
        };

    private:
        PathConstants() = delete; // Prevents instantiation
    };
}