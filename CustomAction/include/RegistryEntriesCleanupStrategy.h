#pragma once

#include <format>

#include "RegistryConstants.h"
#include "RegistryCleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup::Strategies
{
    class RegistryEntriesCleanupStrategy : public RegistryCleanupStrategy
    {
    public:
        bool execute(std::shared_ptr<Logger::ILogger> logger) override
        {
            using enum WinLogon::CustomActions::Logger::LogLevel;
            logger->log(Logger::LogLevel::LOG_INFO, L"=== Deleting Registry Entries - Started ===");

            bool result = true;
            for (const auto& keyPair : Constants::RegistryConstants::getInstallationKeysToDelete( ))
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"- Processing: {}.", formatKeyPath(keyPair)));
                result &= deleteRegistryKey(keyPair.first, keyPair.second, logger);
            }

            logger->log(Logger::LogLevel::LOG_INFO, L"=== Deleting Registry Entries - Finished! ===\n");
            return result;
        }

        std::wstring getName( ) const override
        {
            return L"Registry Entries Cleanup Strategy";
        }
    };
}