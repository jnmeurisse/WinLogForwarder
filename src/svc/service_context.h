/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <memory>
#include "evt/event_collector.h"
#include "evt/event_forwarder.h"
#include "evt/event_queue.h"
#include "svc/service_config.h"
#include "utl/user_cache.h"
#include "utl/logger.h"


namespace wlf::svc {

    class ServiceContext {
    public:
        ServiceContext() = delete;
        ServiceContext(const ServiceConfig& config);
        ~ServiceContext();

        void start() noexcept;
        void stop() noexcept;

    private:
        // The class name
        static const char* __class__;

        // A reference to the application logger
        utl::Logger& _logger;
        
        const svc::ServiceConfig _config;
        
        evt::EventQueue _queue;
        utl::UserCache _user_cache;

        std::unique_ptr<evt::EventCollector> _collector;
        std::unique_ptr<evt::EventForwarder> _forwarder;
    };

}