#pragma once

#include <Windows.h>

#include <format>
#include <string>
#include <string_view>
#include <optional>
#include <memory>

#include "ICleanupStrategy.h"
#include "RegistryConstants.h"

namespace WinLogon::CustomActions::Cleanup
{
    class RegistryCleanupStrategy : public ICleanupStrategy
    {
    protected:
        [[nodiscard]] std::wstring formatKeyPath(const std::pair<HKEY, std::wstring>& keyPair) const
        {
            if (const auto it = Constants::RegistryConstants::hKeyToWStr.find(keyPair.first);
                it != Constants::RegistryConstants::hKeyToWStr.end( ))
            {
                return std::format(L"{}\\{}", it->second, keyPair.second);
            }

            // Error fallback
            return std::format(L"UNKNOWN_KEY\\{}", keyPair.second);
        }

        [[nodiscard]] std::optional<std::wstring> getFriendlyErrorMessage(LONG errorCode) const noexcept
        {
            // RAII wrapper for LocalFree
            struct LocalFreeDeletor
            {
                void operator()(LPWSTR ptr) const noexcept
                {
                    if (ptr) LocalFree(ptr);
                }
            };

            LPWSTR messageBuffer = nullptr;
            const DWORD messageLength = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPWSTR>(&messageBuffer),
                0,
                nullptr
            );

            if (!messageBuffer || messageLength == 0)
            {
                return std::nullopt;
            }

            // Use RAII to ensure LocalFree is called
            std::unique_ptr<WCHAR, LocalFreeDeletor> messagePtr(messageBuffer);

            std::wstring errorMessage{ messagePtr.get( ) };

            // Remove trailing newlines
            while (!errorMessage.empty( ) &&
                   (errorMessage.back( ) == L'\n' || errorMessage.back( ) == L'\r'))
            {
                errorMessage.pop_back( );
            }

            return errorMessage.empty( ) ? std::nullopt : std::make_optional(std::move(errorMessage));
        }

        bool deleteRegistryKey(HKEY hKeyRoot, std::wstring_view subKey,
                               std::shared_ptr<Logger::ILogger> logger) const
        {
            using enum Logger::LogLevel;

            const LONG result = RegDeleteTreeW(hKeyRoot, subKey.data( ));

            switch (result)
            {
                case ERROR_SUCCESS:
                    logger->log(LOG_INFO, std::format(L"  Key deleted successfully: {}.", subKey));
                    return true;

                case ERROR_FILE_NOT_FOUND:
                    logger->log(LOG_INFO, std::format(L"  Key not found: {}.", subKey));
                    return true; // Consider it a success if the key doesn't exist

                default:
                {
                    const auto errorMsg = getFriendlyErrorMessage(result);
                    const std::wstring_view errorText = errorMsg.value_or(L"Unknown error");

                    logger->log(LOG_ERROR,
                                std::format(L"  Error deleting key: {} (Error Code: {} - {}).",
                                            subKey, result, errorText));
                    return false;
                }
            }
        }

    public:
        // Specific implementation will be provided by derived classes
        bool execute(std::shared_ptr<Logger::ILogger> logger) override = 0;
        [[nodiscard]] std::wstring getName( ) const override = 0;
    };
}