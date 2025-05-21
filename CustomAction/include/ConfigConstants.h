#pragma once

#include <string>
#include <filesystem>

namespace WinLogon::CustomActions::Constants
{
    class ConfigConstants
    {
    public:
        static inline const std::wstring DEFAULT_CONFIG_FILE_NAME = L"wlconfig.cfg";

        static inline const std::filesystem::path SYSTEM32_CONFIG_PATH = L"C:\\Windows\\System32\\wlconfig.cfg";
        static inline const std::filesystem::path DRIVERS_ETC_CONFIG_PATH = L"C:\\Windows\\System32\\drivers\\etc\\wlconfig.cfg";

        static inline const std::filesystem::path CONFIG_PROGRAM_DATA_DESTINATION = L"C:\\ProgramData\\WatchGuard\\Logon App";
        static inline const std::filesystem::path CONFIG_PROGRAM_FILES_DESTINATION = L"C:\\Program Files\\WatchGuard\\Logon App\\Resources";


        static inline const std::wstring TEMP_INSTALL_CONFIG_NAME = L"InstallConfig.cfg";
        static inline const std::wstring TEMP_LOCAL_CONFIG_NAME = L"LocalConfig.cfg";

        // Get the temporary directory for config files
        static inline std::filesystem::path GetTempConfigDir( )
        {
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            return std::filesystem::path(tempPath) / L"WatchGuardLogonAppConfig";
        }

    private:
        ConfigConstants( ) = delete; // Prevents instantiation
    };
}