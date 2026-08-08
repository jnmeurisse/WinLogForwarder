#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>
#include <guiddef.h>

#include <cstdint>
#include <vector>

namespace wlf::evt {
    
    class EventVariants {
    public:
        EventVariants(const uint8_t* buffer, size_t property_count);

        ::UINT8 get_byte(unsigned int property_id) const;
        ::UINT16 get_uint16(unsigned int property_id) const;
        ::UINT32 get_uint32(unsigned int property_id) const;
        ::UINT64 get_uint64(unsigned int property_id) const;
        ::GUID*  get_guid(unsigned int property_id) const;
        ::LPCWSTR get_string(unsigned int property_id) const;
        ::FILETIME get_time(unsigned int property_id) const;
        ::SID* get_sid(unsigned int property_id) const;

    private:
        const ::EVT_VARIANT* const _variants;
        const size_t _property_count;
    };
}