#include "service_context.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>

#include "evt/event_forwarder.h"
#include "evt/event_types.h"
#include "utl/exception.h"


namespace wlf::svc {

    ServiceContext::ServiceContext(const ServiceConfig& config)
        : _logger(utl::Logger::instance())
        , _config(config)
        , _queue(_config.limit_config().queue_size)
        , _user_cache(_config.limit_config().user_cache_size)
    {
        DEBUG_CTOR(_logger);
    }


    ServiceContext::~ServiceContext()
    {
        DEBUG_DTOR(_logger);
    }


    void ServiceContext::start() noexcept
    {
        try {
            _logger.info("Starting service...");

            _forwarder = std::make_unique<evt::EventForwarder>(
                _queue,
                _config.forwarder_config()
            );
            if (!_forwarder->start())
                throw utl::os_error("start forwarder thread error", ::GetLastError());

            evt::EventCollectorConfig collector_config{
                {
                    _config.limit_config().max_system_data_buffer_size,
                    _config.limit_config().max_event_data_buffer_size,
                    _user_cache
                },
            };

            _collector = std::make_unique<evt::EventCollector>(
                _queue,
                collector_config
            );

            // Register all formatters
            for (auto& config : _config.formatter_configs()) {
                _collector->add_formatter(config.second);
            }

            // Register all subscription
            for (auto& config : _config.subscription_configs()) {
                _collector->add_subscription(config.second);
            }

            _collector->start();
            if (!_collector->start())
                throw utl::os_error("start collector thread error", ::GetLastError());

            _collector->wait(INFINITE);
            _forwarder->wait(INFINITE);
        }
        catch (const utl::os_error& e) {
            _logger.error("stop service error: %s (%x)", e.what(), e.error_code());
        }
        catch (const std::exception& e) {
            _logger.error("stop service error: %s", e.what());
        }
    }


    void ServiceContext::stop() noexcept
    {
        _logger.info("Stopping service...");
        try {
            _queue.stop();

            if (_forwarder)
                _forwarder->stop();
            if (_collector)
                _collector->stop();

            if (_forwarder->wait(5000) && _collector->wait(5000))
                _logger.info("service stopped");
            else
                _logger.error("service still busy");
        }
        catch (const utl::os_error& e) {
            _logger.error("stop service error: %s (%x)", e.what(), e.error_code());
        }
        catch (...) {
            _logger.error("stop service error: unexpected");
        }
    }

    const char* ServiceContext::__class__ = "ServiceContext";

}