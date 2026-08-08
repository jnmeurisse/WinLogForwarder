#include "signal.h"

#include <stdexcept>
#include <utility>



namespace wlf::utl {

    Signal::Signal(bool manual_reset)
	{
		_handle = ::CreateEvent(nullptr, manual_reset, false, nullptr);
		if (_handle == nullptr)
			throw std::runtime_error("CreateEvent error");
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


	bool Signal::set() noexcept
	{
		return ::SetEvent(_handle) != 0;
	}


	bool Signal::reset() noexcept
	{
		return ::ResetEvent(_handle) != 0;
	}


	bool Signal::is_set() const
	{
		return wait(0);
	}


	bool Signal::wait(DWORD timeout) const
	{
		switch (::WaitForSingleObject(_handle, timeout)) {
		case WAIT_OBJECT_0: // The event is set.
			return true;

		case WAIT_TIMEOUT: // The event is not yet signaled.
			return false;

		case WAIT_FAILED:
            throw std::runtime_error("CreateEvent error");

		default:
			return false;
		}
	}

}
