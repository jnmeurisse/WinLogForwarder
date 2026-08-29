#include "service_config.h"

#include <stdexcept>
#include <limits>
#include <memory>
#include <vector>
#include <mbedtls/ssl.h>

#include "global.h"
#include "evt/event_facility.h"
#include "evt/event_idlist.h"
#include "evt/event_properties.h"
#include "evt/event_types.h"
#include "utl/string.h"
#include "utl/system.h"
#include <cstdlib>
#include <cctype>
#include <cstdint>


namespace wlf::svc {

    struct range { size_t lower; size_t upper; };


    static bool has_key(const cpptoml::table& table, const std::string& key)
    {
        return table.contains(key);
    }


    static int in_range(int value, const range& r) noexcept
    {
        return value >= r.lower && value <= r.upper;
    }


    static bool is_valid_enterprise_id(const std::string& id)
    {
        if (id.empty())
            return false;

        std::vector<std::string> segments;
        if (utl::split(id.c_str(), '.', segments) == 0)
            return false;

        for (const auto& segment : segments) {
            // Catches consecutive dots like "32473..1"
            if (segment.empty())
                return false;

            // Catches unnecessary leading zeros
            if (segment.size() > 1 && segment[0] == 0)
                return false;

            // Check if convertible to integer
            for (char c : segment) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    return false;
                }
            }

            errno = 0;
            long long value = std::strtoll(segment.c_str(), nullptr, 10);
            if (errno != 0 || value < 0 || value > std::numeric_limits<uint32_t>::max())
                return false;
        }
    
        return true;
    }


    ServiceConfig::ServiceConfig()
        : _logger(utl::Logger::instance())
    {
    }


    std::optional<int> ServiceConfig::load_int(const cpptoml::table& table, const std::string& key)
    {
        if (!has_key(table, key))
            return std::nullopt;

        const auto result = table.get_as<int>(key);
        if (!result) {
            _logger.error("'%s' : parameter syntax error", key.c_str());
            return std::nullopt;
        }

        if (*result < 0 || *result > std::numeric_limits<int>::max()) {
            _logger.error("'%s' : parameter out of range", key.c_str());
            return std::nullopt;
        }

        return { *result };
    }


    std::optional<std::string> ServiceConfig::load_string(const cpptoml::table& table, const std::string& key)
    {
        if (!has_key(table, key))
            return std::nullopt;

        const auto result = table.get_as<std::string>(key);
        if (!result) {
            _logger.error("'%s' : parameter syntax error", key.c_str());
            return std::nullopt;
        }

        return { utl::trim(*result) };
    }


    bool ServiceConfig::load_global_section(const cpptoml::table& table)
    {
        bool valid = true;
        auto agent_id = load_string(table, "agent_id");

        if (!agent_id || agent_id->empty()) {
            _logger.error("Global section - 'agent_id' parameter missing");
            valid = false;
        }
        else
            _agent_id = agent_id.value();

        auto enterprise_id = load_string(table, "enterprise_id");
        if (!enterprise_id || enterprise_id->empty()) {
            _logger.error("Global section -'enterprise_id' parameter missing");
            valid = false;
        }
        else if (!is_valid_enterprise_id(enterprise_id.value())) {
            _logger.error("Global section - %s is an invalid enterprise_id", enterprise_id.value().c_str());
            valid = false;
        }
        else
            _enterprise_id = enterprise_id.value();

        return valid;
    }


    bool ServiceConfig::load_limit_section(const cpptoml::table& table)
    {
        bool valid = true;

        // load system_data_buffer_size
        {
            static range max_system_data_buffer_size_range{
                .lower = MinSystemDataBufferSize,
                .upper = MaxSystemDataBufferSize
            };

            auto system_data_buffer_size = load_int(table, "system_data_buffer_size");
            if (!system_data_buffer_size) {
                _logger.error("limits section - 'system_data_buffer_size' parameter missing");
                valid = false;
            }
            else if (!in_range(system_data_buffer_size.value(), max_system_data_buffer_size_range)) {
                _logger.error("limits section - 'system_data_buffer_size' out of range");
                valid = false;
            }
            else
                _limit_config.max_system_data_buffer_size = system_data_buffer_size.value();
        }

        // load event_data_buffer_size
        {
            static range max_event_data_buffer_size_range{
                .lower = MinEventDataBufferSize,
                .upper = MaxEventDataBufferSize
            };

            auto event_data_buffer_size = load_int(table, "event_data_buffer_size");
            if (!event_data_buffer_size) {
                _logger.error("limits section - 'event_data_buffer_size' parameter missing");
                valid = false;
            }
            else if (!in_range(event_data_buffer_size.value(), max_event_data_buffer_size_range)) {
                _logger.error("limits section - 'event_data_buffer_size' out of range");
                valid = false;
            }
            else
                _limit_config.max_event_data_buffer_size = event_data_buffer_size.value();
        }

        // load user_cache_size
        {
            static range user_cache_size_range{
                .lower = MinUserCacheSize,
                .upper = MaxUserCacheSize
            };

            auto user_cache_size = load_int(table, "user_cache_size");
            if (!user_cache_size) {
                _logger.error("limits section - 'user_cache_size' parameter missing");
                valid = false;
            }
            else if (!in_range(user_cache_size.value(), user_cache_size_range)) {
                _logger.error("limits section - 'user_cache_size' out of range");
                valid = false;
            }
            else
                _limit_config.user_cache_size = user_cache_size.value();
        }

        // load queue_size
        {
            static range queue_size_range{
            .lower = MinQueueSize,
            .upper = MaxQueueSize
            };

            auto queue_size = load_int(table, "queue_size");
            if (!queue_size) {
                _logger.error("limits section - 'queue_size' parameter missing");
                valid = false;
            }
            else if (!in_range(queue_size.value(), queue_size_range)) {
                _logger.error("limits section - 'queue_size' out of range");
                valid = false;
            }
            else
                _limit_config.queue_size = queue_size.value();
        }

        // load max_syslog_msg_length
        {
            const range max_syslog_msg_length_range{
                .lower = MinMaxSyslogMsgLength,
                .upper = std::numeric_limits<long long>::max()
            };

            auto max_syslog_msg_length = load_int(table, "max_syslog_msg_length");
            if (!max_syslog_msg_length) {
                _logger.error("limits section - 'max_syslog_msg_length' parameter missing");
                valid = false;
            }
            else if (!in_range(max_syslog_msg_length.value(), max_syslog_msg_length_range)) {
                _logger.error("limits section - 'max_syslog_msg_length' out of range");
                valid = false;
            }
            else
                _limit_config.max_syslog_msg_length_default = max_syslog_msg_length.value();
        }

        // load max_syslog_sd_length
        {
            const range max_syslog_sd_length_range{
                .lower = MinMaxSDLength,
                .upper = _limit_config.max_syslog_msg_length_default
            };

            auto max_syslog_sd_length = load_int(table, "max_syslog_sd_length");
            if (!max_syslog_sd_length) {
                _logger.error("limits section - 'max_syslog_sd_length' parameter missing");
                valid = false;
            }
            else if (!in_range(max_syslog_sd_length.value(), max_syslog_sd_length_range)) {
                _logger.error("limits section - 'max_syslog_sd_length' out of range");
                valid = false;
            }
            else
                _limit_config.max_syslog_sd_length_default = max_syslog_sd_length.value();
        }

        return valid;
    }


    bool ServiceConfig::load_forwarder_section(const cpptoml::table& table)
    {
        bool valid = true;
        auto collector = load_string(table, "collector");

        if (!collector || collector->empty()) {
            _logger.error("'collector' parameter missing");
            valid = false;
        }
        else if (collector->size() > MBEDTLS_SSL_MAX_HOST_NAME_LEN) {
            _logger.error("'collector' host name length > 255");
            valid = false;
        }
        else {
            try {
                _forwarder_config.collector = net::Endpoint(collector.value(), 6443);
            }
            catch (const std::invalid_argument&) {
                _logger.error("%s is an invalid 'collector' definition", collector.value().c_str());
                valid = false;
            }
        }

        auto protocol = load_string(table, "protocol");
        if (!protocol || utl::iequal(*protocol, "TLS"))
            _forwarder_config.protocol = evt::EventForwarderProtocol::TLS;
        else if (utl::iequal(*protocol, "TCP"))
            _forwarder_config.protocol = evt::EventForwarderProtocol::TCP;
        else {
            _logger.error("'%s' is an invalid 'protocol' specification", protocol.value().c_str());
            valid = false;
        }

        return valid;
    }


    bool ServiceConfig::load_formatters_array(const cpptoml::table_array& table)
    {
        bool valid = true;
        evt::EventFormatterConfig formatter_config;

        for (auto it = table.begin(); it != table.end(); ++it) {
            const auto& formatter = it->get();

            auto id = load_string(*formatter, "id");

            if (!id || id->empty()) {
                _logger.error("formatter 'id' missing");
                valid = false;
                continue;
            }

            if (_formatter_configs.count(*id) > 0) {
                _logger.error("formatter '%s' configured more than once", id->c_str());
                valid = false;
                continue;
            }

            formatter_config.id = id.value();

            // load facility
            {
                auto facility = load_string(*formatter, "facility");
                if (!facility || facility->empty()) {
                    _logger.error("formatter '%s' - 'facility' parameter is missing", id->c_str());
                    valid = false;
                }
                else {
                    if (!evt::from_string(*facility, formatter_config.facility)) {
                        _logger.error("formatter '%s' - '%s' is not a valid facility",
                            id->c_str(),
                            facility->c_str());
                        valid = false;
                    }
                }
            }

            // load sd_id_sys
            {
                auto sd_id_sys = load_string(*formatter, "sd_id_sys");

                if (!sd_id_sys || sd_id_sys->empty())
                    formatter_config.sd_id_sys = utl::str2wstr("sys@" + _enterprise_id);
                else
                    formatter_config.sd_id_sys = utl::str2wstr(sd_id_sys.value());

                //TODO validate the SD-ID
            }

            // load sd_exclude_sys_properties
            {
                auto excluded_properties = formatter->get_array_of<std::string>("sd_exclude_sys_properties");
                formatter_config.system_properties = evt::EventSystemProperties::all();

                if (excluded_properties) {
                    for (const auto& excluded_property : *excluded_properties) {
                        evt::EventSystemProperty property;
                        if (evt::from_string(excluded_property, property))
                            formatter_config.system_properties.exclude(property);
                        else {
                            _logger.error(
                                "formatter '%s' - '%s' is not a valid system property",
                                id->c_str(),
                                excluded_property.c_str());
                            valid = false;
                            continue;
                        }
                    }
                }
            }

            // load sd_id_evd
            {
                auto sd_id_evd = load_string(*formatter, "sd_id_evd");

                if (!sd_id_evd || sd_id_evd->empty())
                    formatter_config.sd_id_evd = utl::str2wstr("evd@" + _enterprise_id);
                else
                    formatter_config.sd_id_evd = utl::str2wstr(sd_id_evd.value());

                //TODO validate the SD-ID
            }

            // load max_syslog_msg_length
            {
                const range max_syslog_msg_length_range{
                    .lower = MinMaxSyslogMsgLength,
                    .upper = std::numeric_limits<long long>::max()
                };

                auto max_syslog_msg_length = load_int(*formatter, "max_syslog_msg_length");
                if (!max_syslog_msg_length)
                    formatter_config.max_syslog_msg_length = _limit_config.max_syslog_msg_length_default;
                else if (!in_range(max_syslog_msg_length.value(), max_syslog_msg_length_range)) {
                    _logger.error("formatter '%s' - 'max_syslog_msg_length' out of range",
                        id->c_str());
                    valid = false;
                }
                else
                    formatter_config.max_syslog_msg_length = max_syslog_msg_length.value();
            }

            // load max_syslog_sd_length
            {
                const range max_syslog_sd_length_range{
                    .lower = MinMaxSDLength,
                    .upper = _limit_config.max_syslog_msg_length_default
                };

                auto max_syslog_sd_length = load_int(*formatter, "max_syslog_sd_length");
                if (!max_syslog_sd_length)
                    formatter_config.max_syslog_sd_length = _limit_config.max_syslog_sd_length_default;
                else if (!in_range(max_syslog_sd_length.value(), max_syslog_sd_length_range)) {
                    _logger.error("formatter '%s' - 'max_syslog_sd_length' out of range",
                        id->c_str());
                    valid = false;
                }
                else
                    _limit_config.max_syslog_sd_length_default = max_syslog_sd_length.value();
            }

            if (valid) {
                formatter_config.app_name = utl::str2wstr(_agent_id);
                formatter_config.hostname = utl::get_computer_name();

                _formatter_configs.emplace(id.value(), formatter_config);
            }
        }

        return valid;
    }


    bool ServiceConfig::load_subscriptions_array(const cpptoml::table_array& table)
    {
        bool valid = true;

        for (auto it = table.begin(); it != table.end(); ++it) {
            auto subscription = it->get();

            auto id = load_string(*subscription, "id");

            if (!id || id->empty()) {
                _logger.error("subscription 'id' missing");
                valid = false;
                continue;
            }

            if (_subscription_configs.count(*id) > 0) {
                _logger.error("subscription 'id' %s configured more than once", id->c_str());
                valid = false;
                continue;
            }

            std::wstring channel_name;
            auto channel = load_string(*subscription, "channel");
            if (!channel || channel->empty()) {
                _logger.error("subscription %s : 'channel' is missing", id->c_str());
                valid = false;
                continue;
            }
            else
                channel_name = utl::str2wstr(channel.value());

            std::wstring provider_name;
            auto provider = load_string(*subscription, "provider");
            if (provider && !provider->empty())
                provider_name = utl::str2wstr(provider.value());

            std::string filter_name;
            auto filter = load_string(*subscription, "filter");
            if (filter && !filter->empty())
                filter_name = filter.value();

            std::string formatter_name;
            auto formatter = load_string(*subscription, "formatter");
            if (!formatter || formatter->empty()) {
                _logger.error("subscription %s : 'formatter' is missing", id->c_str());
                valid = false;
                continue;
            }
            else
                formatter_name = formatter.value();

            auto event_id_filter = std::make_shared<evt::EventIDList>(provider_name);
            auto events = subscription->get_array_of<std::string>("events");
            if (!events) {
                _logger.error("subscription %s : 'events' is missing", id->c_str());
                valid = false;
            } else if (events->size() > 0) {
                for (auto event = events->cbegin(); event != events->cend(); ++event) {
                    auto is_valid_id = [](int id) {
                        return (0 <= id) && (id <= std::numeric_limits<::DWORD>::max());
                    };

                    std::vector<std::string> parts;
                    switch (utl::split(event->c_str(), '-', parts))
                    {
                    case 1:
                        int event_id;
                        if (!utl::str2i(parts[0], event_id) || !is_valid_id(event_id))
                            _logger.error("subscription %s : '%s' is an invalid event ID",
                                id->c_str(),
                                event->c_str());
                        else
                            event_id_filter->add(event_id);
                        break;

                    case 2:
                        int event_id_lower, event_id_upper;
                        if (!utl::str2i(parts[0], event_id_lower) || !is_valid_id(event_id_lower) ||
                            !utl::str2i(parts[1], event_id_upper) || !is_valid_id(event_id_upper))
                            _logger.error("subscription %s : '%s' is an invalid event ID range",
                                id->c_str(),
                                event->c_str());
                        else
                            event_id_filter->add(event_id_lower, event_id_upper);
                        break;

                    default:
                        _logger.error("subscription %s : '%s' is an invalid event ID",
                            id->c_str(),
                            event->c_str());
                        valid = false;
                        break;
                    }
                }
            }

            if (valid) {
                evt::EventSubscriptionConfig subscription_config{
                    .id= id->c_str(),
                    .channel=channel_name,
                    .provider_name=provider_name,
                    .event_id_filter=event_id_filter,
                    .filter=filter_name,
                    .formatter=formatter_name
                };

                _subscription_configs.emplace(id.value(), subscription_config);
            }
        }

        return valid;
    }


    bool ServiceConfig::load(const cpptoml::table& config_table)
    {
        bool valid = true;

        const auto global_section = config_table.get_table("global");
        if (!global_section) {
            _logger.error("'global' section missing");
            valid = false;
        }
        else
            valid &= load_global_section(*global_section);

        const auto limit_section = config_table.get_table("limits");
        if (!limit_section) {
            _logger.error("'limit' section missing");
            valid = false;
        }
        else
            valid &= load_limit_section(*limit_section);

        const auto forwarder_section = config_table.get_table("forwarder");
        if (!forwarder_section) {
            _logger.error("'forwarder' section missing");
            valid = false;
        }
        else
            valid &= load_forwarder_section(*forwarder_section);

        const auto formatters_config = config_table.get_table_array("formatters");
        if (!formatters_config) {
            _logger.error("'formatters' table missing");
            valid = false;
        }
        else {
            valid &= load_formatters_array(*formatters_config);
        }

        const auto subscriptions_config = config_table.get_table_array("subscriptions");
        if (!subscriptions_config) {
            _logger.error("'subscriptions' table missing");
            valid = false;
        }
        else
            valid &= load_subscriptions_array(*subscriptions_config);

        return valid;
    }

}