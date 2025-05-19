#pragma once

#include <string>

namespace WinLogon::CustomActions::Constants
{
    class UUIDs
    {
    public:
        static inline constexpr std::wstring_view APPLICATION_UUID = L"{BCB72349-6C97-4E3F-94B5-6EA045F85CA5}";
        static inline constexpr std::wstring_view FACE_RECOGNITION_UUID = L"{22AD5268-00F7-427E-A4F7-87C7CF161BA7}";
        static inline constexpr std::wstring_view PIN_UUID = L"{5D76DE6C-7F65-4431-89AC-2D43EBE72298}";
        static inline constexpr std::wstring_view FINGERPRINT_UUID = L"{B88420D8-BAB2-4451-A2D3-A2F99AF9854A}";
        static inline constexpr std::wstring_view SMARTCARD_UUID = L"{2BFD34AC-10D7-4C70-94F5-3F7EA9025B0E}";

    private:
        UUIDs() = delete; // Prevents instantiation
    };
}