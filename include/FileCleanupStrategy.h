#pragma once

#include <Windows.h>
#include <filesystem>

#include "ICleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup
{
    class FileCleanupStrategy : public ICleanupStrategy
    {
    protected:
        bool removeFile(const std::filesystem::path& filePath, std::shared_ptr<Logger::ILogger> logger) const
        {
            try
            {
                if (!std::filesystem::exists(filePath))
                {
                    logger->log(Logger::LogLevel::LOG_INFO,
                                L"  File " + filePath.wstring( ) + L" not found.");
                    return true;
                }

                // Get file attributes
                DWORD fileAttributes = GetFileAttributesW(filePath.c_str( ));
                if (fileAttributes == INVALID_FILE_ATTRIBUTES)
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                L"  Could not get attributes for: " + filePath.wstring( ) + L".");
                    return false;
                }

                // Remove read-only attributes if necessary
                if ((fileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) &&
                    (!SetFileAttributesW(filePath.c_str( ), FILE_ATTRIBUTE_NORMAL)))
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                L"  Could not modify attributes for: " + filePath.wstring( ) + L".");
                    return false;
                }


                // Try to delete the file
                if (std::filesystem::remove(filePath))
                {
                    logger->log(Logger::LogLevel::LOG_INFO,
                                L"  File " + filePath.wstring( ) + L" successfully removed.");
                    return true;
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                L"  Failed to remove: " + filePath.wstring( ) + L".");
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            L"  Exception when removing file: " + filePath.wstring( ) + L". - " +
                            std::wstring(e.what( ), e.what( ) + strlen(e.what( ))));
                return false;
            }
        }

    public:
        // Implementação específica será fornecida por classes derivadas
        bool execute(std::shared_ptr<Logger::ILogger> logger) override = 0;
        std::wstring getName( ) const override = 0;
    };
}