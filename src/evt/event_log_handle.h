#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>

#include <cstdint>
#include <vector>
#include "evt/event_handle.h"


namespace wlf::evt {

    /**
     * A byte buffer that has a maximum size.
     */
    class RenderingBuffer
    {
    public:
        RenderingBuffer() = delete;

        /**
         * Allocates an empty buffer.
         * 
         * @param size_limit The maximum size of this buffer.
         */
        explicit RenderingBuffer(::DWORD size_limit) noexcept;

        /**
         * Returns the size of this buffer.
         */
        inline ::DWORD size() const noexcept { return static_cast<::DWORD>(_b.size()); }
 
        /**
         * Increases the size of the buffer if 'new_size' is greater that the current size.
         * 
         * @return true if the new size is not greater than the maximum size.
         */
        bool resize(::DWORD new_size) noexcept;

        /**
         * Returns a pointer to the contiguous memory.  The function returns a
         * null pointer when the buffer is empty.  The memory location could be
         * different after a resize.
         */
        inline uint8_t* data() noexcept { return _b.data(); }

    private:
        std::vector<uint8_t> _b;
        const ::DWORD _size_limit;
    };

    /**
     * An event handle returned from a call to EvtCreateRenderContext.
     */
    class EventRenderContext : public EventHandle
    {
    public:
        static EventRenderContext create_system_context();
        static EventRenderContext create_data_context() noexcept;

    private:
        using EventHandle::EventHandle;
    };


    /**
     * An event handle returned from a call to EvtNext.
     */
    class EventLogHandle : public EventHandle
    {
    public:
        /**
         * Default constructor. Initializes an empty handle.
         */
        EventLogHandle() = default;

        /**
         * Constructs an EventLogHandle taking ownership of a native handle.
         */
        explicit EventLogHandle(::EVT_HANDLE handle) noexcept;

        /**
         * Renders the event values as variants into a buffer.
         *
         * @param context Rendering context.
         * @param buffer A buffer to store the rendered values.
         *
         * @return number of values stored in the buffer.  In case of failure,
         * the function returns 0.  GetLastError returns the error code.
         */
        ::DWORD render_values(const EventRenderContext& context, RenderingBuffer& buffer) const;

        /**
         * Renders the event as XML into buffer.
         *
         * @param buffer A buffer to store the rendered values.
         *
         * @return The size of the buffer on success, 0 on failure.  
         *         GetLastError returns the error code.
         */
        ::DWORD render_xml(RenderingBuffer& buffer) const;

    private:
        // Low-level helper to render this event
        bool render(const EventRenderContext& context, ::EVT_RENDER_FLAGS flag, uint8_t* buffer,
            ::DWORD& buffer_size, ::DWORD& property_count) const noexcept;
    };

}