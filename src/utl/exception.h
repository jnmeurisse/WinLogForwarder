/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <stdexcept>


namespace wlf::utl {

    class os_error : public std::runtime_error
    {
    public:
        os_error(const char* message, uint32_t error_code)
            : runtime_error(message)
            , _error_code(error_code)
        {
        }

        inline uint32_t error_code() const noexcept { return _error_code; }

    private:
        uint32_t _error_code;
    };
}