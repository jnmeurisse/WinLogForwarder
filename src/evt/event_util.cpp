#include "event_util.h"


namespace wlf::evt {

    static constexpr const char8_t* property_name[] = {
        u8"ProviderName",
        u8"ProviderGuid",
        u8"EventID",
        u8"Qualifiers",
        u8"Level",
        u8"Task",
        u8"Opcode",
        u8"Keywords",
        u8"TimeCreated",
        u8"EventRecordID",
        u8"ActivityID",
        u8"RelatedActivityID",
        u8"ProcessID",
        u8"ThreadID",
        u8"Channel",
        u8"Computer",
        u8"AccountName",
        u8"Domain",
        u8"AccountType",
        u8"Version"
    };


    const char8_t* EventSystemPropertyName(EventSystemProperty property) noexcept
    {
        return property_name[static_cast<int>(property)];
    }

}