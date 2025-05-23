#pragma once

#include <Windows.h>
#include <filesystem>
#include <format>

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
                                std::format(L"  File {} not found.", filePath.wstring( )));
                    return true;
                }

                // Get file attributes
                DWORD fileAttributes = GetFileAttributesW(filePath.c_str( ));
                if (fileAttributes == INVALID_FILE_ATTRIBUTES)
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                std::format(L"  Could not get attributes for: {}.", filePath.wstring( )));
                    return false;
                }

                // Remove read-only attributes if necessary
                if ((fileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) &&
                    (!SetFileAttributesW(filePath.c_str( ), FILE_ATTRIBUTE_NORMAL)))
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                std::format(L"  Could not modify attributes for: {}.", filePath.wstring( )));
                    return false;
                }

                // Try to delete the file
                if (std::filesystem::remove(filePath))
                {
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"  File {} successfully removed.", filePath.wstring( )));
                    return true;
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                std::format(L"  Failed to remove: {}.", filePath.wstring( )));
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"  Exception when removing file: {}. - {}",
                                        filePath.wstring( ),
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return false;
            }
        }

    public:
        // Specific implementation will be provided by derived classes
        bool execute(std::shared_ptr<Logger::ILogger> logger) override = 0;
        std::wstring getName( ) const override = 0;
    };
}