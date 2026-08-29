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
	 * A Thread is an abstract class that enables creation of separate threads 
	 * of execution. Each new instance of a Thread descendant is a new 
	 * thread of execution.
	*/
	class Thread
	{
	public:
		/**
		 * Allocates a new execution thread.
		 * 
		 * The thread is created in suspended state. The caller must call `start`
		 * to resume the execution.
		*/
		explicit Thread();

		/**
		* Deletes this thread.
		*/
		virtual ~Thread();

		/**
		 * Starts the execution of the thread.
		 * 
		 * The method returns true if the function succeed to start the
		 * execution of this thread.
		*/
		virtual bool start() noexcept;

		/**
		 * Waits 'timeout' milliseconds for the thread to finish.
		 * 
		 * The function returns true if the thread has finished and false if
		 * the thread is still running after the specified time.
         * 
         * @throw os_error
		*/
		bool wait(DWORD timeout);

		/**
		 * Returns the thread identifier
		*/
		inline unsigned int get_id() const noexcept { return _id; }

		/**
		 * Returns the thread handle
		*/
		inline HANDLE handle() const noexcept { return _handle; }

	protected:
		virtual unsigned int run() = 0;

	private:
		// Handle to the windows thread.
		HANDLE _handle;

		// Thread id.
		unsigned int _id;

		friend unsigned __stdcall thread_entry_point(void *data);
	};

}
