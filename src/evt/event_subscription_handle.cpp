#include "event_subscription_handle.h"

#include "utl/exception.h"


namespace wlf::evt {

    EventSubscriptionHandle EventSubscriptionHandle::create(const utl::Signal& signal, const std::wstring& channel, const std::wstring& query)
    {
        const ::EVT_HANDLE handle = ::EvtSubscribe(
            nullptr,
            signal.handle(),
            channel.c_str(),
            query.c_str(),
            nullptr,
            nullptr,
            nullptr,
            ::EvtSubscribeToFutureEvents
        );

        if (!handle)
            throw utl::os_error("EventSubscriptionHandle: EvtSubscribe error", ::GetLastError());

        return EventSubscriptionHandle(handle);
    }


    bool EventSubscriptionHandle::next(std::span<::EVT_HANDLE> event_handles, ::DWORD timeout, ::DWORD& count) const noexcept
    {
        const DWORD events_size = static_cast<DWORD>(event_handles.size());
        return ::EvtNext(_handle, events_size, event_handles.data(), timeout, 0, &count) == TRUE;
    }


    bool EventSubscriptionHandle::cancel() const noexcept
    {
        return ::EvtCancel(_handle) == TRUE;
    }

}