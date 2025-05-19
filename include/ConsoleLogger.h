#pragma once

#include <string>
#include <iostream>

#include "BaseLogger.h"

namespace WinLogon::CustomActions::Logger
{
    class ConsoleLogger : public BaseLogger
    {
    public:
        void log(LogLevel level, const std::wstring& message) override
        {
            std::wstring formatted = formatLogMessage(level, message);
            std::wcout << formatted << std::endl;
        }
    };
}