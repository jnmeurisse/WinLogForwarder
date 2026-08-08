#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace wlf::utl {

	/**
	* A signal is a synchronization object that enables one thread to notify another
	* thread that an event has occurred.  This class is a wrapper around Windows event
	* synchronization object.
	*/
	class Signal final
	{
	public:
        Signal() = delete;

		/**
		 * Creates an event.
		*/
		explicit Signal(bool manual_reset);

        Signal(const Signal&) = delete;
        Signal& operator=(const Signal&) = delete;
        Signal(Signal&& other) noexcept;
        Signal& operator=(Signal&& other) noexcept;

		/**
		 * Destroys this signal
		*/
		~Signal();

		/**
		 * Sets the event to a non-signaled state.
		 *
		 * The method returns false if the function failed.
		*/
		bool reset() noexcept;

		/**
		 * Sets the signal to a signaled state.
		 * 
		 * The method returns false if the function failed. 
		*/
		bool set() noexcept;

		/**
		 * Returns true if this signal is in a signaled state.
		 *
		 * The method raises an winapi_error if an error has occurred.
		*/
		bool is_set() const;

		/**
		 * Waits until the event is in a signaled state.
		 * 
		 * The method raises an winapi_error if an error has occurred.
		*/
		bool wait(DWORD timeout = INFINITE) const;

		/**
		 * Returns the Signal handle.
		*/
		inline HANDLE get_handle() const noexcept { return _handle; }

	private:
		// The event handle.
		HANDLE _handle = nullptr;
	};

}
