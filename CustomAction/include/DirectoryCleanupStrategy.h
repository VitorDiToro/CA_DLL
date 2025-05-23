#pragma once

#include <format>

#include <Windows.h>
#include <filesystem>

#include "ICleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup
{
    class DirectoryCleanupStrategy : public ICleanupStrategy
    {
    protected:
        static bool isDirectoryEmpty(const std::filesystem::path& path)
        {
            return std::filesystem::is_empty(path);
        }

        bool removeDirectory(const std::filesystem::path& path, std::shared_ptr<Logger::ILogger> logger, bool forceRemove = true) const
        {
            using enum WinLogon::CustomActions::Logger::LogLevel;
            try
            {
                if (!std::filesystem::exists(path))
                {
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"  Directory already deleted (not found): {}", path.wstring( )));
                    return true; // Consider it a success if the directory already doesn't exist
                }

                if (!forceRemove && !isDirectoryEmpty(path))
                {
                    logger->log(LOG_WARNING,
                                std::format(L"  The {} folder contains other files or sub-folders.", path.wstring( )));
                    logger->log(LOG_INFO,
                                L"  Listing remaining content:");

                    // List the remaining items in the parent folder
                    for (const auto& entry : std::filesystem::directory_iterator(path))
                    {
                        logger->log(LOG_INFO,
                                    std::format(L"    - {}", entry.path( ).filename( ).wstring( )));
                    }

                    logger->log(LOG_WARNING,
                                std::format(L"  The {} folder will not be removed to preserve the data above.", path.wstring( )));
                    return true; // Return true since this is expected behavior
                }

                std::error_code errorCode;
                std::size_t itemsRemoved = std::filesystem::remove_all(path, errorCode);

                if (errorCode)
                {
                    logger->log(LOG_ERROR,
                                std::format(L"  Failed to remove directory: {} - Error: {}",
                                            path.wstring( ),
                                            std::wstring(errorCode.message( ).begin( ), errorCode.message( ).end( ))));
                    return false;
                }

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Directory successfully removed: {} ({} items deleted)",
                                        path.wstring( ), itemsRemoved));
                return true;
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                logger->log(LOG_ERROR,
                            std::format(L"  Exception while removing directory: {} - {}",
                                        path.wstring( ),
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));

                // Try to identify if there are files in use
                logProblematicFiles(path, logger);

                return false;
            }
        }

    private:
        void logProblematicFiles(const std::filesystem::path& path, std::shared_ptr<Logger::ILogger> logger) const
        {
            logger->log(Logger::LogLevel::LOG_INFO, L"  Attempting to identify problematic files...");

            try
            {
                // Iterate through the files in the folder to identify which ones might be causing problems
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
                {
                    try
                    {
                        // Try to change file attributes to check if it's accessible
                        if (std::filesystem::is_regular_file(entry))
                        {
                            DWORD attributes = GetFileAttributesW(entry.path( ).c_str( ));

                            if ((attributes != INVALID_FILE_ATTRIBUTES) &&
                                (!SetFileAttributesW(entry.path( ).c_str( ), FILE_ATTRIBUTE_NORMAL)))
                            {
                                logger->log(Logger::LogLevel::LOG_WARNING,
                                            std::format(L"  - File possibly in use: {}", entry.path( ).wstring( )));
                            }

                        }
                    }
                    catch (...)
                    {
                        logger->log(Logger::LogLevel::LOG_WARNING,
                                    std::format(L"  - Unable to access: {}", entry.path( ).wstring( )));
                    }
                }
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_INFO, L"  - Unable to analyze directory contents.");
            }
        }

    public:
        // Implementation will be provided by derived classes
        bool execute(std::shared_ptr<Logger::ILogger> logger) override = 0;
        std::wstring getName( ) const override = 0;
    };
}