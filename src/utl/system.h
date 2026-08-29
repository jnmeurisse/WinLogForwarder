/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <string>
#include <filesystem>

namespace wlf::utl {

    std::wstring get_computer_name() noexcept;
    std::wstring get_module_path() noexcept;
    std::filesystem::path get_program_data() noexcept;

}