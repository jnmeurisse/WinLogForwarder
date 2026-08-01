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


    bool EventHandle::render_xml(std::vector<wchar_t>& buffer, const size_t size_limit) const
    {
		DWORD buffer_size = static_cast<::DWORD>(buffer.size() * sizeof(wchar_t));

		if (!render_xml(buffer.data(), buffer_size)) {
			if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				// Convert required bytes back to element count
				const size_t required_elements = buffer_size / sizeof(wchar_t);

				// Enforce the size limit safeguard
				if (required_elements > size_limit) {
					return false;
				}

				buffer.resize(required_elements);

				if (!render_xml(buffer.data(), buffer_size))
					return false;
			}
			else {
				return false;
			}
		}

		return true;
	}


    bool EventHandle::render_xml(wchar_t* buffer, DWORD& buffer_size) const
    {
        DWORD property_count = 0;

        return ::EvtRender(
            nullptr,
            _handle,
            EvtRenderEventXml,
            buffer_size,
            buffer,
            &buffer_size,
            &property_count
        ) != FALSE;
    }

}