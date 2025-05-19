#pragma once

#include <memory>

#include "ILogger.h"
#include "MSILogger.h"
#include "ConsoleLogger.h"

namespace WinLogon::CustomActions::Logger
{
    class LoggerFactory
    {
    public:
        static std::shared_ptr<ILogger> createLogger(MSIHANDLE handle)

        {
#ifdef _WINDLL
            return std::make_unique<MSILogger>(handle);
#else
            return std::make_unique<ConsoleLogger>();
#endif
        }

    private:
        LoggerFactory() = delete;  // Prevents instantiation
    };
}