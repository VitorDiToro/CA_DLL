#pragma once

#include <memory>

#include "ILogger.h"

namespace WinLogon::CustomActions::Cleanup
{
    class ICleanupStrategy
    {
    public:
        virtual ~ICleanupStrategy( ) = default;
        virtual bool execute(std::shared_ptr<Logger::ILogger> logger) = 0;

        virtual std::wstring getName( ) const = 0;
    };
}