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

            if (handle == NULL)
            {
                // Fallback
                OutputDebugStringW(L"Warning: Invalid MSI handle provided to logger");
            }
        }

        void log(LogLevel level, const std::wstring& message) override
        {
            if (msiHandle == NULL)
            {
                // Fallback
                OutputDebugStringW((formatLogMessage(level, message) + L"\n").data( ));
                return;
            }

            std::wstring formatted = formatLogMessage(level, message);

            INSTALLMESSAGE messageType = getMsiMessageType(level);

            PMSIHANDLE hRecord = MsiCreateRecord(1);
            if (hRecord)
            {
                MsiRecordSetStringW(hRecord, 0, formatted.data( ));
                UINT result = MsiProcessMessage(msiHandle, messageType, hRecord);

                // Fallback
                if (result != ERROR_SUCCESS)
                {
                    std::wofstream fallbackLog("C:\\Windows\\Temp\\MSILogFallback.log",
                                               std::ios::app);
                    if (fallbackLog)
                    {
                        fallbackLog << formatted << std::endl;
                        fallbackLog.close( );
                    }
                }
            }
        }

    private:
        MSIHANDLE msiHandle;

        INSTALLMESSAGE getMsiMessageType(LogLevel level) const
        {
            switch (level)
            {
                case LogLevel::LOG_ERROR:
                    return INSTALLMESSAGE_ERROR;

                case LogLevel::LOG_WARNING:
                    return INSTALLMESSAGE_WARNING;

                case LogLevel::LOG_TRACE:

                    return INSTALLMESSAGE_ACTIONDATA;

                case LogLevel::LOG_INFO:
                default:
                    return INSTALLMESSAGE_INFO;
            }
        }
    };
}