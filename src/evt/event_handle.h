#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>


namespace wlf::evt {

	/**
	 * Class EventHandle
	 * 
	 * RAII wrapper for Windows Event Log handles (EVT_HANDLE).
	 */
	class EventHandle {
	public:
		/**
		 * Move constructor.
		 * Transfers ownership of the handle from another instance.
		 */
		EventHandle(EventHandle&& other) noexcept;

		/**
		 * Destructor.
		 * Automatically closes the underlying handle if valid.
		 */
		~EventHandle();

		// Copy operations are deleted to enforce unique ownership of the handle.
		EventHandle(const EventHandle&) = delete;
		EventHandle& operator=(const EventHandle&) = delete;

		/**
		 * Move assignment operator.
		 * Transfers ownership of the handle.
		 */
		EventHandle& operator=(EventHandle&& other) noexcept;

		/**
		 * Returns the underlying native handle.
		 */
		inline ::EVT_HANDLE get() const noexcept { return _handle; }

		/**
		 * Checks whether the wrapper holds a valid (non-null) handle.
		 * @return True if the handle is valid, false otherwise.
		*/
		explicit inline operator bool() const noexcept { return _handle != nullptr; }

	protected:
		// The native Windows Event Log handle.
		::EVT_HANDLE _handle = nullptr;

        /**
         * Default constructor. Initializes an empty handle.
         */
        EventHandle() = default;

        /**
         * Constructs an EventHandle taking ownership of a native handle.
         */
        explicit EventHandle(::EVT_HANDLE handle) noexcept;
    };

}
