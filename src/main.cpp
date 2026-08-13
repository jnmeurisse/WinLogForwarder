#include <cstring>
#include "WindowsLogService.h"

#include "evt/event_queue.h"
#include "utl/user_cache.h"


// eventcreate /ID 100 /L application /T INFORMATION /SO "CustomApp" /D "This is a custom event logged via Command Prompt."

int wmain(int argc, wchar_t* argv[]) {

    // Define the internal name and the human-readable display name
    const std::wstring serviceName = L"winlogfwd";
    const std::wstring displayName = L"Windows Log Forwarder";

    // Initialize our service wrapper class
    wlf::utl::UserCache user_cache(512);
    wlf::evt::EventQueue event_queue(32);
    wlf::LogCollector collector(event_queue, { { 32000, 32000 }, user_cache });
    WindowsLogService service(serviceName, displayName, collector);

    int result = service.run(argc, argv);

    return result;
}