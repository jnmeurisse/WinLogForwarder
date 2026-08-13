#include "system.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>

namespace wlf::utl {

    std::wstring get_computer_name() noexcept
    {
        std::array<wchar_t, 256> buffer = { L'\0' };
        ::DWORD buffer_size = static_cast<::DWORD>(buffer.size());
        
        return GetComputerName(buffer.data(), &buffer_size) != 0
            ? std::wstring(buffer.data(), buffer_size)
            : L"";
    }

}