/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#include "system.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>

#include <array>

namespace wlf::utl {

    std::wstring get_computer_name() noexcept
    {
        std::array<wchar_t, 256> buffer = { L'\0' };
        ::DWORD buffer_size = static_cast<::DWORD>(buffer.size());
        
        return ::GetComputerName(buffer.data(), &buffer_size) != 0
            ? std::wstring(buffer.data(), buffer_size)
            : L"";
    }


    std::wstring get_module_path() noexcept
    {
        std::array<wchar_t, MAX_PATH> buffer = { L'\0' };
        ::DWORD buffer_size = static_cast<::DWORD>(buffer.size());

        return ::GetModuleFileName(nullptr, buffer.data(), buffer_size) != 0
            ? std::wstring(buffer.data(), buffer_size)
            : L"";
    }


    std::filesystem::path get_program_data() noexcept
    {
        ::PWSTR path = nullptr;
        std::filesystem::path program_data_path;

        const ::HRESULT hr = ::SHGetKnownFolderPath(
            ::FOLDERID_ProgramData,
            0,
            nullptr,
            &path
        );

        if (SUCCEEDED(hr)) {
            program_data_path = std::filesystem::path(path);
            ::CoTaskMemFree(path);
        }

        return program_data_path;
    }

}