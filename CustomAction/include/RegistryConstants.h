#pragma once

#include <map>
#include <string>
#include <vector>
#include <Windows.h>
#include <string_view>

#include "UUIDs.h"

namespace WinLogon::CustomActions::Constants
{
    class RegistryConstants
    {
    public:
        static inline const std::map<HKEY, std::wstring> hKeyToWStr = {
            {HKEY_CLASSES_ROOT, L"HKEY_CLASSES_ROOT"},
            {HKEY_CURRENT_USER, L"HKEY_CURRENT_USER"},
            {HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE"},
            {HKEY_USERS, L"HKEY_USERS"},
            {HKEY_CURRENT_CONFIG, L"HKEY_CURRENT_CONFIG"}
        };

        static inline std::wstring makeAuthenticationPath(const std::wstring& extra_path, std::wstring_view uuid)
        {
            return std::wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\") + extra_path + L"\\" + std::wstring(uuid);
        }

        static inline std::vector<std::pair<HKEY, std::wstring>> getInstallationKeysToDelete()
        {
            return {
                // Logon App Entries
                { HKEY_LOCAL_MACHINE, L"SOFTWARE\\WatchGuard\\Logon App"},

                // Credential Provider Filter
                { HKEY_LOCAL_MACHINE, makeAuthenticationPath(L"Credential Provider Filters", UUIDs::APPLICATION_UUID) },

                // Password
                { HKEY_LOCAL_MACHINE, makeAuthenticationPath(L"Credential Providers\\", UUIDs::APPLICATION_UUID) },
                { HKEY_CLASSES_ROOT, std::wstring(L"CLSID\\") + std::wstring(UUIDs::APPLICATION_UUID) },

                // Face Recognition
                { HKEY_LOCAL_MACHINE, makeAuthenticationPath(L"Credential Providers\\", UUIDs::FACE_RECOGNITION_UUID) },
                { HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\Classes\\CLSID\\") + std::wstring(UUIDs::FACE_RECOGNITION_UUID) },

                // PIN
                { HKEY_LOCAL_MACHINE, makeAuthenticationPath(L"Credential Providers\\", UUIDs::PIN_UUID) },
                { HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\Classes\\CLSID\\") + std::wstring(UUIDs::PIN_UUID) },

                // Fingerprint
                { HKEY_LOCAL_MACHINE, makeAuthenticationPath(L"Credential Providers\\", UUIDs::FINGERPRINT_UUID) },
                { HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\Classes\\CLSID\\") + std::wstring(UUIDs::FINGERPRINT_UUID) },

                // SmartCard
                { HKEY_LOCAL_MACHINE, makeAuthenticationPath(L"Credential Providers\\", UUIDs::SMARTCARD_UUID) },
                { HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\Classes\\CLSID\\") + std::wstring(UUIDs::SMARTCARD_UUID) }
            };
        }

    private:
        RegistryConstants() = delete; // Prevents instantiation
    };
}