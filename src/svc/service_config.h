#pragma once

#include <map>
#include <optional>
#include <string>

#include "evt/event_types.h"

#define CPPTOML_NO_RTTI
#include "utl/cpptoml.h"
#include "utl/logger.h"


namespace wlf::svc {

    struct LimitConfig {
        // Forwarder queue size
        size_t queue_size = 0;

        // Maximum number of entries in the global user cache
        size_t user_cache_size = 0;

        // Maximum buffer size used to render system properties
        // associated with an event.
        size_t max_system_data_buffer_size = 0;

        // Maximum buffer size used to render event or user data
        // associated with an event.
        size_t max_event_data_buffer_size = 0;

        // Default Structured Data parameter size limit. 
        // Formatters can override this value.
        size_t max_syslog_sd_length_default = 0;

        // Default rfc5424 SYSLOG-MSG size limit.
        size_t max_syslog_msg_length_default = 0;
    };

    // A collection of subscription configurations
    using SubscriptionConfigs = std::map<std::string, evt::EventSubscriptionConfig>;

    // A collection of formatter configurations
    using FormatterConfigs = std::map<std::string, evt::EventFormatterConfig>;

    class ServiceConfig {
    public:
        ServiceConfig();
        bool load(const cpptoml::table& config_table);

        inline const std::string& agent_id() const noexcept { return _agent_id; }

        inline const LimitConfig& limit_config() const noexcept
            { return _limit_config; }

        inline const evt::EventForwarderConfig& forwarder_config() const noexcept
            { return _forwarder_config; }

        inline const FormatterConfigs& formatter_configs() const noexcept
            { return _formatter_configs; }

        inline const SubscriptionConfigs& subscription_configs() const noexcept
            { return _subscription_configs; }

    private:
        // A reference to the application logger
        utl::Logger& _logger;

        std::string _agent_id;

        // Enterprise ID as specified in RFC5424 Section 7.2.2
        // This ID are used to build SD-ID
        std::string _enterprise_id;

        svc::LimitConfig _limit_config;
        evt::EventForwarderConfig _forwarder_config;
        svc::FormatterConfigs _formatter_configs;
        svc::SubscriptionConfigs _subscription_configs;

        bool load_global_section(const cpptoml::table& table);
        bool load_limit_section(const cpptoml::table& table);
        bool load_forwarder_section(const cpptoml::table& table);
        bool load_formatters_array(const cpptoml::table_array& table);
        bool load_subscriptions_array(const cpptoml::table_array& table);

        std::optional<int> load_int(const cpptoml::table& table, const std::string& key);
        std::optional<std::string> load_string(const cpptoml::table& table, const std::string& key);
        //bool load_bool(std::ostream& outs, const cpptoml::table& table, const std::string& key, bool& value)

    };



}