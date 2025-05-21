#pragma once

#include <Windows.h>

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <functional>
#include <string_view>
#include <format>

#include "RegistryCleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup::Strategies
{
    // Strategy for cleaning registry entries related to AuthPoint/LogonApp
    class AuthPointRegistryCleanupStrategy : public RegistryCleanupStrategy
    {
    private:
        // Define a custom deleter for HKEY
        struct HKeyDeleter
        {
            void operator()(HKEY key) const
            {
                if (key) RegCloseKey(key);
            }
        };

        // Type alias for HKEY smart pointer
        using HKeyPtr = std::unique_ptr<HKEY__, HKeyDeleter>;

    public:
        bool execute(std::shared_ptr<Logger::ILogger> logger) override
        {
            using enum WinLogon::CustomActions::Logger::LogLevel;
            logger->log(LOG_INFO, L"=== AuthPoint/LogonApp Registry Cleanup - Started ===");

            // All found results
            std::vector<RegistryEntry> allEntries;

            // Searching in standard paths
            for (const auto& path : m_standardRegistryPaths)
            {
                logger->log(LOG_INFO, std::format(L"Searching in HKLM\\{}", path));
                auto entries = findStandardRegistryEntries(HKEY_LOCAL_MACHINE, path, logger);
                if (!entries.empty( ))
                {
                    logger->log(LOG_INFO,
                                std::format(L"  Found {} entries in this path.", entries.size( )));
                    allEntries.insert(allEntries.end( ), entries.begin( ), entries.end( ));
                }
                else
                {
                    logger->log(LOG_INFO, L"  No entries found in this path.");
                }
            }

            // Searching in products
            logger->log(LOG_INFO, std::format(L"Searching in HKLM\\{}", m_productsRegistryPath));
            auto productEntries = findProductsRegistryEntries(HKEY_LOCAL_MACHINE,
                                                             m_productsRegistryPath, logger);
            if (!productEntries.empty( ))
            {
                logger->log(LOG_INFO, std::format(L"  Found {} entries in Products.", productEntries.size( )));
                allEntries.insert(allEntries.end( ), productEntries.begin( ), productEntries.end( ));
            }
            else
            {
                logger->log(LOG_INFO, L"  No entries found in Products.");
            }

            // Summary and removal
            bool success = true;
            if (allEntries.empty( ))
            {
                logger->log(LOG_INFO, L"No AuthPoint/LogonApp related entries found.");
            }
            else
            {
                logger->log(LOG_INFO,
                            std::format(L"Total of {} entries found. Removing...", allEntries.size( )));

                for (const auto& entry : allEntries)
                {
                    logger->log(LOG_INFO, std::format(L"Removing: {} ({})",
                                                      entry.path, entry.displayName));

                    bool deleted = deleteRegistryKey(HKEY_LOCAL_MACHINE, entry.path, logger);
                    if (!deleted)
                    {
                        logger->log(LOG_WARNING, L"  Failed to remove entry.");
                        success = false;
                    }
                }
            }

            logger->log(LOG_INFO, L"=== AuthPoint/LogonApp Registry Cleanup - Finished ===\n");
            return success;
        }


        std::wstring getName( ) const override
        {
            return L"AuthPoint Registry Cleanup Strategy";
        }

    private:
        // Structure to store registry entry information
        struct RegistryEntry
        {
            std::wstring path;           // Full key path
            std::wstring displayName;    // Display name
            std::wstring guid;           // GUID for Product-type entries
            std::wstring type;           // Entry type (Standard or Product)
        };

        const std::vector<std::wstring> m_searchStrings = {
            L"AuthPoint", L"Logon App", L"LogonApp", L"WatchGuard"
        };

        const std::vector<std::wstring> m_standardRegistryPaths = {
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData",
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed"
        };

        const std::wstring m_productsRegistryPath = L"Software\\Classes\\Installer\\Products";


        bool matchesAnyPattern(const std::wstring& value) const
        {
            return std::any_of(m_searchStrings.begin( ), m_searchStrings.end( ),
                               [&value](const std::wstring& pattern)
            {
                return value.find(pattern) != std::wstring::npos;
            });
        }


        std::optional<std::wstring> getRegistryValue(HKEY rootKey, const std::wstring& keyPath, const std::wstring& valueName, std::shared_ptr<Logger::ILogger> logger) const
        {
            HKEY hKey = nullptr;
            if (RegOpenKeyExW(rootKey, keyPath.data( ), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            {
                return std::nullopt;
            }

            // RAII to ensure key closure
            HKeyPtr keyPtr(hKey);

            WCHAR buffer[1024];
            DWORD bufferSize = sizeof(buffer);
            DWORD type;

            if (RegQueryValueExW(hKey, valueName.data( ), nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS)
            {
                if (type == REG_SZ || type == REG_EXPAND_SZ)
                {
                    return std::wstring(buffer);
                }
            }

            return std::nullopt;
        }


        std::vector<RegistryEntry> findStandardRegistryEntries(HKEY rootKey, const std::wstring& path, std::shared_ptr<Logger::ILogger> logger) const
        {
            std::vector<RegistryEntry> results;

            HKEY hKey = nullptr;
            if (RegOpenKeyExW(rootKey, path.data( ), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            {
                return results;
            }

            // RAII to ensure key closure
            HKeyPtr keyPtr(hKey);

            searchRecursive(hKey, path, L"", [&](const std::wstring& keyPath, const std::wstring& relativePath)
            {
                // Try to get the DisplayName property
                auto displayName = getRegistryValue(rootKey, keyPath, L"DisplayName", logger);
                if (displayName && matchesAnyPattern(*displayName))
                {
                    results.push_back({
                        .path = keyPath,
                        .displayName = *displayName,
                        .type = L"Standard"
                                      });
                }
            }, logger);

            return results;
        }


        std::vector<RegistryEntry> findProductsRegistryEntries(HKEY rootKey, const std::wstring& path, std::shared_ptr<Logger::ILogger> logger) const
        {
            std::vector<RegistryEntry> results;

            HKEY hKey = nullptr;
            if (RegOpenKeyExW(rootKey, path.data( ), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            {
                return results;
            }

            // RAII to ensure key closure
            HKeyPtr keyPtr(hKey);

            WCHAR subKeyName[256];
            DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(WCHAR);

            // Enumerate all GUIDs directly under Products
            for (DWORD i = 0;
                 RegEnumKeyExW(hKey, i, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
                 ++i)
            {
                subKeyNameSize = sizeof(subKeyName) / sizeof(WCHAR);

                std::wstring guidKey = std::format(L"{}\\{}", path, subKeyName);

                // First try ProductName, then InstallProperties/DisplayName
                auto productName = getRegistryValue(rootKey, guidKey, L"ProductName", logger);
                if (!productName)
                {
                    std::wstring installPropsKey = std::format(L"{}\\InstallProperties", guidKey);
                    productName = getRegistryValue(rootKey, installPropsKey, L"DisplayName", logger);
                }

                if (productName && matchesAnyPattern(*productName))
                {
                    results.push_back({
                        .path = guidKey,
                        .displayName = *productName,
                        .guid = subKeyName,
                        .type = L"Product" });
                }
            }

            return results;
        }

        void searchRecursive(HKEY hKey, const std::wstring& keyPath, const std::wstring& relativePath,
                             const std::function<void(const std::wstring&, const std::wstring&)>& callback,
                             std::shared_ptr<Logger::ILogger> logger) const
        {
            // Call the callback for this key
            std::wstring currentPath = keyPath;
            if (!relativePath.empty( ))
            {
                currentPath = std::format(L"{}\\{}", keyPath, relativePath);
            }
            callback(currentPath, relativePath);

            HKEY hSubKey = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, currentPath.data( ), 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
            {
                return;
            }

            // RAII to ensure key closure
            HKeyPtr keyPtr(hSubKey);

            // Enumerate subkeys
            WCHAR subKeyName[256];
            DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(WCHAR);
            for (DWORD i = 0;
                 RegEnumKeyExW(hSubKey, i, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
                 ++i)
            {
                subKeyNameSize = sizeof(subKeyName) / sizeof(WCHAR);

                std::wstring newRelativePath = relativePath;
                if (!newRelativePath.empty( ))
                {
                    newRelativePath = std::format(L"{}\\{}", relativePath, subKeyName);
                }
                else
                {
                    newRelativePath = subKeyName;
                }

                // Recursive call for the subkey
                searchRecursive(hKey, keyPath, newRelativePath, callback, logger);
            }
        }
    };
}