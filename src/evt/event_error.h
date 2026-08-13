#pragma once

#include <cstdint>
#include <stdexcept>

namespace wlf::evt {

    class event_error : public std::runtime_error
    {
    public:
        explicit event_error(const char* message, uint32_t error_code)
            : runtime_error(message)
            , _error_code(error_code)
        {}

        inline uint32_t error_code() const noexcept { return _error_code; }

    private:
        uint32_t _error_code;
    };

}