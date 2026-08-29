/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#include "thread.h"

#include <process.h>
#include "utl/exception.h"

namespace wlf::utl {

	/* Internal functions */
	unsigned __stdcall thread_entry_point(void *);

	Thread::Thread()
        : _handle((HANDLE)::_beginthreadex(nullptr, 0, thread_entry_point, this, CREATE_SUSPENDED, &_id))
	{
		if (_handle == NULL)
			throw utl::os_error("Thread::_beginthreadex error", ::GetLastError());
	}


	Thread::~Thread()
	{
		if (_handle != NULL)
		{
			::CloseHandle(_handle);
		}
	}


	bool Thread::start() noexcept
	{
		const DWORD status = ::ResumeThread(_handle);
		
		return status != (DWORD)-1;
	}


	bool Thread::wait(DWORD timeout)
	{
		switch (::WaitForSingleObject(_handle, timeout)) {
		case WAIT_OBJECT_0: // the thread has ended.
			return true;

		case WAIT_TIMEOUT: // the thread is still running.
			return false;

		case WAIT_FAILED:
            throw utl::os_error("Thread: WaitForSingleObject error", ::GetLastError());

		default:
			return false;
		}
	}


	/**
	* Thread entry point.
	*/
	unsigned __stdcall thread_entry_point(void* data)
	{
		const auto thread = static_cast<Thread*>(data);
		
		if (thread) {
			const unsigned int rc = thread->run();
			::_endthreadex(rc);
		}

		return 0;
	}

}
