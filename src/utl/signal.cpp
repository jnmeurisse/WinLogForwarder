#include "signal.h"

#include <utility>
#include "utl/exception.h"


namespace wlf::utl {

    Signal::Signal(bool manual_reset) :
        _handle(::CreateEvent(nullptr, manual_reset, false, nullptr))

	{
		if (_handle == nullptr)
			throw utl::os_error("Signal: CreateEvent error", ::GetLastError());
	}


    Signal::Signal(Signal&& other) noexcept
		: _handle(std::exchange(other._handle, nullptr))
	{
	}


    Signal& Signal::operator=(Signal&& other) noexcept
	{
		if (this != &other) {
			if (_handle)
				::CloseHandle(_handle);

			_handle = std::exchange(other._handle, nullptr);
		}

		return *this;
	}


    Signal::~Signal()
	{
		if (_handle)
			::CloseHandle(_handle);
	}


	void Signal::set()
	{
        if (::SetEvent(_handle) == 0)
            throw utl::os_error("Signal: SetEvent error", ::GetLastError());
	}


	void Signal::reset()
	{
		if (::ResetEvent(_handle) == 0)
            throw utl::os_error("Signal: ResetEvent error", ::GetLastError());
    }


	bool Signal::is_set() const
	{
		return wait(0);
	}


	bool Signal::wait(DWORD millisec) const
	{
		switch (::WaitForSingleObject(_handle, millisec)) {
		case WAIT_OBJECT_0: // The event is set.
			return true;

		case WAIT_TIMEOUT: // The event is not yet signaled.
			return false;

		case WAIT_FAILED:
            throw utl::os_error("Signal: WaitForSingleObject error", ::GetLastError());

		default:
			return false;
		}
	}

}
