#pragma once

#include <format>

#include "PathConstants.h"
#include "FileCleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup::Strategies
{
    class V3FilesCleanupStrategy : public FileCleanupStrategy
    {
    public:
        bool execute(std::shared_ptr<Logger::ILogger> logger) override
        {
            using enum WinLogon::CustomActions::Logger::LogLevel;
            logger->log(LOG_INFO,
                        L"=== Deleting V3 Files - Started ===");

            bool success = true;
            for (const auto& filePath : Constants::PathConstants::filesFromV3ToRemove)
            {
                logger->log(LOG_INFO,
                            std::format(L"- Processing: {}", filePath.wstring( )));
                success &= removeFile(filePath, logger);
            }

            logger->log(LOG_INFO,
                        L"=== Deleting V3 Files - Finished! ===\n");
            return success;
        }

        std::wstring getName( ) const override
        {
            return L"V3 Files Cleanup Strategy";
        }
    };
}