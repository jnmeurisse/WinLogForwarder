#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>

#include <vector>

namespace wlf::evt {

	/**
	 * Class EventHandle
	 * 
	 * RAII wrapper for Windows Event Log handles (EVT_HANDLE).
	 */
	class EventHandle {
	public:
		/**
		 * Default constructor. Initializes an empty handle.
		 */
		EventHandle() = default;
		
		/**
		 * Constructs an EventHandle taking ownership of a native handle.
		 */
		explicit EventHandle(::EVT_HANDLE handle) noexcept;
		
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

		/**
		 * Renders the event as XML into a safe vector buffer.
		 * @param buffer Vector to store the rendered XML wide characters.
		 * @param size_limit Maximum allowed size for the render operation.
		 * @return True on success, false on failure.
		 */
		bool render_xml(std::vector<wchar_t>& buffer, size_t size_limit) const;

	private:
		// The native Windows Event Log handle.
		::EVT_HANDLE _handle = nullptr;

		// Low-level helper to render event XML into a raw buffer.
		bool render_xml(wchar_t* buffer, ::DWORD& buffer_size) const;
	};

}
