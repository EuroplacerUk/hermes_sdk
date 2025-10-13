#pragma once

#include <boost/system/error_code.hpp>
#include <boost/system/detail/system_category_message_win32.hpp>

namespace Hermes
{
namespace boost_patch
{
    using namespace boost::system::detail;

    inline std::string system_category_message_win32(int ev, uint16_t langId)
    {
        using namespace boost::winapi;

        wchar_t* lpMsgBuf = 0;

        DWORD_ retval = boost::winapi::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER_ | FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
            NULL,
            ev,
            langId,
            (LPWSTR_)&lpMsgBuf,
            0,
            NULL
        );

        if (retval == 0)
        {
            return unknown_message_win32(ev);
        }

        local_free lf_ = { lpMsgBuf };
        (void)lf_;

        UINT_ const code_page = message_cp_win32();

        int r = boost::winapi::WideCharToMultiByte(code_page, 0, lpMsgBuf, -1, 0, 0, NULL, NULL);

        if (r == 0)
        {
            return unknown_message_win32(ev);
        }

        std::string buffer(r, char());

        r = boost::winapi::WideCharToMultiByte(code_page, 0, lpMsgBuf, -1, &buffer[0], r, NULL, NULL);

        if (r == 0)
        {
            return unknown_message_win32(ev);
        }

        --r; // exclude null terminator

        while (r > 0 && (buffer[r - 1] == '\n' || buffer[r - 1] == '\r'))
        {
            --r;
        }

        if (r > 0 && buffer[r - 1] == '.')
        {
            --r;
        }

        buffer.resize(r);

        return buffer;
    }

    inline std::string ErrorMessageKludge(const boost::system::error_code& ec)
    {
        using namespace boost::winapi;
        //NC - it's all chinese to me bug - exceptions with error messages in Chinese
        //This originates from Kernel32!FormatMessageW
        //This in turn calls into ntdll!RtlFindMessage
        //And this returns English initially, but then starts speaking Mandarin.
        //Either we're spuriously calling a Locale API (can't find it), or otherwise, a bug is overwriting the TEB.
        //The thread locale or some other locale variable is being overwritten
        //This is a workaround and forces English.
        const uint16_t ENGLISH_LCID = 1033;
        if (ec.category() == boost::system::system_category())
            return system_category_message_win32(ec.value(), ENGLISH_LCID);     //Force the error message to English
        else
            return ec.message();
    }
}
}