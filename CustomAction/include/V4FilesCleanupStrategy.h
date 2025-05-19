#pragma once

#include <string>

#include "PathConstants.h"
#include "DirectoryCleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup::Strategies
{
    class V4FilesCleanupStrategy : public DirectoryCleanupStrategy
    {
    public:
        bool execute(std::shared_ptr<Logger::ILogger> logger) override
        {
            logger->log(Logger::LogLevel::LOG_INFO, L"=== Deleting V4 Files - Started ===");

            bool success = true;
            success &= cleanupLogonAppFolders(logger);
            success &= cleanupWatchGuardFolders(logger);

            logger->log(Logger::LogLevel::LOG_INFO, L"=== Deleting V4 Files - Finished! ===\n");
            return success;
        }

        std::wstring getName() const override
        {
            return L"V4 Files Cleanup Strategy";
        }

    private:
        bool cleanupLogonAppFolders(std::shared_ptr<Logger::ILogger> logger)
        {
            logger->log(Logger::LogLevel::LOG_INFO, L"Cleaning up Logon App folders:");

            bool result = true;
            for (const auto& path : Constants::PathConstants::logonAppFoldersPath)
            {
                logger->log(Logger::LogLevel::LOG_INFO, L"- Removing " + path + L" folder and its contents...");
                result &= removeDirectory(path, logger, true); // true = force remove
            }

            return result;
        }

        bool cleanupWatchGuardFolders(std::shared_ptr<Logger::ILogger> logger)
        {
            logger->log(Logger::LogLevel::LOG_INFO, L"Cleaning up WatchGuard folders:");

            bool result = true;
            for (const auto& path : Constants::PathConstants::watchGuardFoldersPath)
            {
                logger->log(Logger::LogLevel::LOG_INFO, L"- Removing " + path + L" folder...");
                result &= removeDirectory(path, logger, false); // false = only if empty
            }

            return result;
        }
    };
}