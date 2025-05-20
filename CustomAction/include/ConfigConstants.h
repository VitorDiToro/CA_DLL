#pragma once

#include <string>
#include <filesystem>

namespace WinLogon::CustomActions::Constants
{
    class ConfigConstants
    {
    public:
        static inline const std::wstring DEFAULT_CONFIG_FILE_NAME = L"wlconfig.cfg";
        static inline const std::filesystem::path CONFIG_FILE_DESTINATION_DIR = L"C:\\Program Files\\WatchGuard\\Logon App\\Resources";

    private:
        ConfigConstants( ) = delete; // Prevents instantiation
    };
}