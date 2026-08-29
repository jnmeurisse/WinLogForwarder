#pragma once

#include <string>
#include <cstdint>
#include <map>
#include <vector>

namespace wlf {

    /* 
    * Configuration example
    
        [global]
        agent_id = "agent-node-01"

        # Define distinct filtering rules
        [filters.drop_noise]
        action = "drop"
        match_field = "AccountName"
        match_values = ["NT AUTHORITY\\SYSTEM", "LOCAL SERVICE"]

        [format.rfc5424]
        max_sd_length = 1024

        [[subscriptions]]
        id = "application"
        enabled = true
        channel = "Application"
        format = "format.rfc5424"
        events = ["1000", "1200-1255"]

        [[subscriptions]]
        id = "security_audit"
        enabled = true
        channel = "Security"
        filter = "drop_noise"
        format = "format.rfc5424"
        events = ["4624", "4625", "2276", "1233"]

        [collector]
        enabled = true
        subscriptions = ["application", "security_audit"]
        filters = ["drop_noise"]
        formatter = "rfc-5425"

        [forwarder]
        server = "siem.internal.company.com"
        port = 443
        protocol = "rfc-5425"
        ca_certificate = "internal_ca.crt"
        client_certificate = "/agent_client.crt"
    */
    struct FilterRuleConfig {
    };

    struct FormatConfig {
        std::string format;
        size_t sd_value_max_size{ 1024 };
    };

    struct SubscriptionConfig {
        std::string id;
        bool enabled{ false };
        std::wstring channel;
        struct FormatConfig format_config;
        std::vector<uint32_t> selected_event_id;
    };

    struct CollectorConfig {
        bool enabled{ false };
        std::vector<SubscriptionConfig> subscriptions;
    };

    struct ForwarderConfig {
        std::string server;
        int16_t port{ 443 };
        std::string protocol;
        std::string ca_certificate;
        std::string client_certificate;
    };

    struct WlfConfig {
        std::string agent_id{};
        std::vector<SubscriptionConfig> subscriptions = {};
        std::map<std::string, FormatConfig> formats = {};
        std::map<std::string, FilterRuleConfig> filters = {};
        CollectorConfig collector;
        ForwarderConfig forwarder_config;
    };

    void load_config(const std::string& filename, WlfConfig& wlf_config);
}