#include "event_variants.h"


namespace wlf::evt {

    EventVariants::EventVariants(const uint8_t* buffer, size_t property_count)
        : _variants(reinterpret_cast<const ::EVT_VARIANT*>(buffer))
        , _property_count(property_count)
    {
    }


    ::UINT8 EventVariants::get_byte(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeByte != _variants[property_id].Type)
            ? 0
            : _variants[property_id].ByteVal;
    }


    ::UINT16 EventVariants::get_uint16(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeUInt16 != _variants[property_id].Type)
            ? 0
            : _variants[property_id].UInt16Val;
    }


    ::UINT32 EventVariants::get_uint32(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeUInt32 != _variants[property_id].Type)
            ? 0
            : _variants[property_id].UInt32Val;
    }


    ::UINT64 EventVariants::get_uint64(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeUInt64 != _variants[property_id].Type)
            ? 0
            : _variants[property_id].UInt64Val;
    }


    ::GUID* EventVariants::get_guid(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeGuid != _variants[property_id].Type)
            ? nullptr
            : _variants[property_id].GuidVal;
    }


    ::LPCWSTR EventVariants::get_string(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeString != _variants[property_id].Type)
            ? nullptr
            : _variants[property_id].StringVal;
    }


    ::FILETIME EventVariants::get_time(unsigned int property_id) const
    {
        ULARGE_INTEGER file_time{};
        file_time.QuadPart = (property_id >= _property_count) || (EvtVarTypeFileTime != _variants[property_id].Type)
            ? 0
            : _variants[property_id].FileTimeVal;

        return { file_time.LowPart, file_time.HighPart };
    }


    ::SID* EventVariants::get_sid(unsigned int property_id) const
    {
        return (property_id >= _property_count) || (EvtVarTypeSid != _variants[property_id].Type)
            ? nullptr
            : _variants[property_id].SidVal;
    }

}