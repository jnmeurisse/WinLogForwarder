#include <LogCollector.h>

#include <memory>

#include "evt/event_queue.h"
#include "evt/event_filter.h"
#include "evt/event_formatter.h"
#include "evt/event_util.h"
#include "utl/system.h"


namespace wlf {

    LogCollector::LogCollector(evt::EventQueue& queue, const evt::EventCollectorConfig& config)
        : EventCollector(queue, config)
    {
    }


    unsigned int LogCollector::run()
    {
        evt::EventIdFilter selected_id;
        evt::EventSystemProperties selected_properties;
        selected_properties.add(evt::EventSystemProperty::ProviderName);
        selected_properties.add(evt::EventSystemProperty::TimeCreated);

        evt::RFC5424EventFormatter::Options format_options = {
            23,
            utl::get_computer_name(),
            L"APP",
            L"",
            L"",
            selected_properties,
        };

        evt::IEventFilter filter = nullptr;
        evt::IEventFormatter formatter = std::make_shared<evt::RFC5424EventFormatter>(format_options);

        evt::EventSubscriptionConfig application = {
            L"Application",
            selected_id,
            { 32 * 1024, filter, formatter }
        };

        evt::EventSubscriptionConfig security = {
            L"Security",
            selected_id,
            { 32 * 1024, filter, formatter }
        };

        try {
            add_subscription(application);
        }
        catch (std::exception& ) {
        }

        try {
            add_subscription(security);
        }
        catch (std::exception&) {
        }

        return EventCollector::run();
    }

}