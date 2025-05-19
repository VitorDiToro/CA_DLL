#pragma once

#include <string>

namespace WinLogon::CustomActions::Logger
{
    enum class LogLevel
    {
        LOG_TRACE,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR
    };

    class ILogger
    {
    public:
        virtual ~ILogger( ) = default;
        virtual void log(LogLevel level, const std::wstring& message) = 0;
    };
}