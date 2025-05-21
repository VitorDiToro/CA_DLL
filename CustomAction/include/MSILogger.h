#pragma once

#include <Windows.h>
#include <msi.h>
#include <msiquery.h>

#include "BaseLogger.h"

namespace WinLogon::CustomActions::Logger
{
    class MSILogger : public BaseLogger
    {
    public:
        explicit MSILogger(MSIHANDLE handle) : msiHandle(handle)
        {
        }

        void log(LogLevel level, const std::wstring& message) override
        {
            std::wstring formatted = formatLogMessage(level, message);

            PMSIHANDLE hRecord = MsiCreateRecord(1);
            MsiRecordSetString(hRecord, 0, formatted.data( ));
            MsiProcessMessage(msiHandle, INSTALLMESSAGE_INFO, hRecord);
        }

    private:
        MSIHANDLE msiHandle;
    };
}