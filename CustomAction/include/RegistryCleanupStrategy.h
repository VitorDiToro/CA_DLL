#pragma once

#include <Windows.h>

#include <format>
#include <string>

#include "ICleanupStrategy.h"
#include "RegistryConstants.h"

namespace WinLogon::CustomActions::Cleanup
{
    class RegistryCleanupStrategy : public ICleanupStrategy
    {
    protected:
        std::wstring formatKeyPath(const std::pair<HKEY, std::wstring>& keyPair) const
        {
            auto it = Constants::RegistryConstants::hKeyToWStr.find(keyPair.first);
            if (it == Constants::RegistryConstants::hKeyToWStr.end( ))
            {
                // Error fallback
                return L"UNKNOWN_KEY\\" + keyPair.second;
            }
            return it->second + L"\\" + keyPair.second;
        }

        std::wstring getFriendlyErrorMessage(LONG errorCode) const
        {
            LPWSTR messageBuffer = nullptr;
            DWORD messageLength = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPWSTR>(&messageBuffer),
                0,
                nullptr
            );

            std::wstring errorMessage;
            if (messageBuffer != nullptr && messageLength > 0)
            {
                // Remover quebras de linha/retornos da mensagem formatada
                errorMessage = std::wstring(messageBuffer);
                // Remover caracteres de nova linha no final da mensagem (comum nas mensagens de sistema)
                while (!errorMessage.empty() && (errorMessage.back() == L'\n' || errorMessage.back() == L'\r'))
                    errorMessage.pop_back();
                LocalFree(messageBuffer);
            }
            else
            {
                errorMessage = L"Unknown error";
            }

            return errorMessage;
        }

        bool deleteRegistryKey(HKEY hKeyRoot, const std::wstring& subKey, std::shared_ptr<Logger::ILogger> logger) const
        {
            LONG result = RegDeleteTreeW(hKeyRoot, subKey.c_str());
            if (result == ERROR_SUCCESS)
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                           L"  Key deleted successfully: " + subKey + L".");
                return true;
            }
            else if (result == ERROR_FILE_NOT_FOUND)
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                           L"  Key not found: " + subKey + L".");
                return false;
            }
            else
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Error deleting key: {} (Error Code: {} - {}).",
                                        subKey, result, getFriendlyErrorMessage(result)));
                return false;
            }
        }

    public:
        // Implementação específica será fornecida por classes derivadas
        bool execute(std::shared_ptr<Logger::ILogger> logger) override = 0;
        std::wstring getName() const override = 0;
    };
}