#pragma once

#include <memory>
#include <vector>
#include <format>

#include "LoggerFactory.h"
#include "ICleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup
{
    class CleanupManager
    {
    public:
        explicit CleanupManager(MSIHANDLE handle) : logger(Logger::LoggerFactory::createLogger(handle)) {}

        void addStrategy(std::unique_ptr<ICleanupStrategy> strategy)
        {
            strategies.emplace_back(std::move(strategy));
        }

        bool executeAll( )
        {
            bool overallSuccess = true;

            for (const auto& strategy : strategies)
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Executing cleanup strategy: {}", strategy->getName( )));

                bool success = strategy->execute(logger);
                if (!success)
                {
                    logger->log(Logger::LogLevel::LOG_WARNING,
                                std::format(L"Strategy {} reported issues.", strategy->getName( )));
                }

                overallSuccess &= success;
            }

            return overallSuccess;
        }

    private:
        std::shared_ptr<Logger::ILogger> logger;
        std::vector<std::unique_ptr<ICleanupStrategy>> strategies;
    };
}