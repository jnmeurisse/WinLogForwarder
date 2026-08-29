/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
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
		 * Creates a signal.

         * @throw os_error.
        */
		explicit Signal(bool manual_reset);

        Signal(const Signal&) = delete;
        Signal& operator=(const Signal&) = delete;
        Signal(Signal&& other) noexcept;
        Signal& operator=(Signal&& other) noexcept;

		/**
		 * Destroys this signal.
		*/
		~Signal();

		/**
		 * Sets the signal to a non-signaled state.
		 *
         * @throw os_error.
        */
		void reset();

		/**
		 * Sets the signal to a signaled state.
		 * 
         * @throw os_error.
        */
		void set();

		/**
		 * Returns true if this signal is in a signaled state.
		 *
         * @throw os_error.
        */
		bool is_set() const;

		/**
		 * Waits until this signal is in a signaled state.
		 * 
         * @throw os_error.
        */
		bool wait(DWORD millisec = INFINITE) const;

		/**
		 * Returns the signal handle.
		*/
		inline HANDLE handle() const noexcept { return _handle; }

	private:
		// The event handle.
		HANDLE _handle;
	};

}
