#include "event_subscription_handle.h"

#include "evt/event_error.h"


namespace wlf::evt {

    EventSubscriptionHandle EventSubscriptionHandle::create(const utl::Signal& signal, const std::wstring& channel, const std::wstring& query)
    {
        const ::EVT_HANDLE handle = ::EvtSubscribe(
            nullptr,
            signal.get_handle(),
            channel.c_str(),
            query.c_str(),
            nullptr,
            nullptr,
            nullptr,
            ::EvtSubscribeToFutureEvents
        );

        if (!handle)
            throw event_error("EvtSubscribe error", ::GetLastError());

        return EventSubscriptionHandle(handle);
    }


    bool EventSubscriptionHandle::next(std::span<::EVT_HANDLE> event_handles, ::DWORD timeout, ::DWORD& count) const noexcept
    {
        const DWORD events_size = static_cast<DWORD>(event_handles.size());
        return ::EvtNext(_handle, events_size, event_handles.data(), timeout, 0, &count);
    }


    bool EventSubscriptionHandle::cancel() const noexcept
    {
        return ::EvtCancel(_handle);
    }

}