#include "event_log_handle.h"

#include "evt/event_error.h"


namespace wlf::evt {

    RenderingBuffer::RenderingBuffer(::DWORD size_limit) noexcept
        : _b()
        , _size_limit(size_limit)
    {
    }


    bool RenderingBuffer::resize(::DWORD new_size)
    {
        if (new_size > _size_limit)
            return false;

        if (new_size > size()) {
            // clear before resize to avoid copying the content of the buffer.
            _b.clear();
            _b.resize(new_size);
        }

        return true;
    }


    EventRenderContext EventRenderContext::create_system_context()
    {
        const ::EVT_HANDLE handle = ::EvtCreateRenderContext(
            0,
            nullptr,
            ::EvtRenderContextSystem
        );

        if (!handle)
            throw event_error("EvtCreateRenderContext error", ::GetLastError());

        return EventRenderContext(handle);
    }


    EventRenderContext EventRenderContext::create_data_context() noexcept
    {
        return EventRenderContext();
    }


    EventLogHandle::EventLogHandle(::EVT_HANDLE handle) noexcept
        : EventHandle(handle)
    {
    }


    ::DWORD EventLogHandle::render_values(const EventRenderContext& context, RenderingBuffer& buffer) const
    {
        ::DWORD buffer_size = buffer.size();
        ::DWORD property_count = 0;

        // Render the message into the local buffer.
        if (!render(context, ::EvtRenderEventValues, buffer.data(), buffer_size, property_count)) {
            // If the buffer is too small, resize and try once more
            if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (!buffer.resize(buffer_size)) {
                    ::SetLastError(ERROR_BUFFER_OVERFLOW);
                    return 0;
                }

                if (!render(context, EvtRenderEventValues, buffer.data(), buffer_size, property_count))
                    // Failed even after resize
                    return 0;
            }
            else {
                // Failed for a reason other than buffer size
                return 0;
            }
        }

        return property_count;
    }


    bool EventLogHandle::render_xml(RenderingBuffer& buffer) const
    {
        ::DWORD buffer_size = buffer.size();
        ::DWORD property_count = 0;
        const EventRenderContext context{ EventRenderContext::create_data_context() };

        if (!render(context, EvtRenderEventXml, buffer.data(), buffer_size, property_count)) {
            // If the buffer is too small, resize and try once more
            if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (!buffer.resize(buffer_size)) {
                    ::SetLastError(ERROR_BUFFER_OVERFLOW);
                    return false;
                }

                if (!render(context, EvtRenderEventXml, buffer.data(), buffer_size, property_count))
                    return false;
            }
            else {
                return false;
            }
        }

        return true;
    }


    bool EventLogHandle::render(const EventRenderContext& context, ::EVT_RENDER_FLAGS flag,
        uint8_t* buffer, ::DWORD& buffer_size, ::DWORD& property_count) const noexcept
    {
        ::SetLastError(ERROR_SUCCESS);

        return ::EvtRender(
            context.get(),
            _handle,
            flag,
            buffer_size,
            buffer,
            &buffer_size,
            &property_count
        );
    }

}