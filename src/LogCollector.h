#pragma once

#include "evt/event_collector.h"
#include "evt/event_queue.h"
#include "utl/signal.h"
#include "utl/user_cache.h"



namespace wlf {

    class LogCollector: public evt::EventCollector {
    public:
	    LogCollector(evt::EventQueue& queue, const evt::EventCollectorConfig& config);

    protected:
        unsigned int run() override;
    };

}