#include "event_handle.h"

#include <utility>


namespace wlf::evt {

    EventHandle::EventHandle(::EVT_HANDLE handle) noexcept
        : _handle(handle)
    {
    }


    EventHandle::EventHandle(EventHandle&& other) noexcept
        : _handle(std::exchange(other._handle, nullptr))
    {
    }


    EventHandle::~EventHandle()
    {
        if (_handle)
            ::EvtClose(_handle);
    }


    EventHandle& EventHandle::operator=(EventHandle&& other) noexcept
    {
		if (this != &other) {
			if (_handle) {
				::EvtClose(_handle);
			}
			_handle = std::exchange(other._handle, nullptr);
		}
		return *this;
	}

}