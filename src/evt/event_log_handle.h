#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>

#include <cstdint>
#include <vector>
#include "evt/event_handle.h"


namespace wlf::evt {

    class RenderingBuffer
    {
    public:
        RenderingBuffer() = delete;
        explicit RenderingBuffer(::DWORD size_limit) noexcept;

        inline void clear() noexcept { _b.clear(); }
        inline uint8_t* data() noexcept { return _b.data(); }
        inline ::DWORD size() noexcept { return static_cast<::DWORD>(_b.size()); }
        bool resize(::DWORD new_size);

    private:
        std::vector<uint8_t> _b;
        const ::DWORD _size_limit;
    };


    class EventLogHandle : public EventHandle
    {
    public:
        class RenderContext : public EventHandle
        {
        public:
            static RenderContext create_system_context() noexcept;
            static RenderContext create_data_context() noexcept;

        private:
            using EventHandle::EventHandle;
        };

        /**
         * Default constructor. Initializes an empty handle.
         */
        EventLogHandle() = default;

        /**
         * Constructs an EventLogHandle taking ownership of a native handle.
         */
        explicit EventLogHandle(::EVT_HANDLE handle) noexcept;

        /**
         * Renders the event values as variants into buffer.
         *
         * @param context Rendering context.
         * @param buffer A buffer to store the rendered values.
         *
         * @return number of values stored in the buffer.  In case of failure,
         * the function returns 0.
         */
        ::DWORD render_values(const RenderContext& context, RenderingBuffer& buffer) const;

        /**
         * Renders the event as XML into buffer.
         *
         * @param buffer A buffer to store the rendered values.
         *
         * @return True on success, false on failure.
         */
        bool render_xml(RenderingBuffer& buffer) const;

    private:
        // Low-level helper to render this event
        bool render(const RenderContext& context, ::EVT_RENDER_FLAGS flag, uint8_t* buffer, ::DWORD& buffer_size, ::DWORD& property_count) const;
    };

}