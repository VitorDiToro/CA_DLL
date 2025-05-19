#pragma once

#include <map>
#include <string>

#include "ILogger.h"

namespace WinLogon::CustomActions::Logger
{
    // Base class that implements common functionality for loggers
    class BaseLogger : public ILogger
    {
    protected:
        static inline const std::map<LogLevel, std::wstring> levelPrefixes = {
            {LogLevel::LOG_TRACE, L"[TRACE]:\t  "},
            {LogLevel::LOG_INFO, L"[INFO]:\t   "},
            {LogLevel::LOG_WARNING, L"[WARNING]  "},
            {LogLevel::LOG_ERROR, L"[ERROR]\t  "}
        };

        std::wstring formatLogMessage(LogLevel level, const std::wstring& message) const
        {
            return levelPrefixes.at(level) + message;
        }

    public:
        virtual ~BaseLogger( ) = default;
    };
}