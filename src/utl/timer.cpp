/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Timer.h"

namespace wlf::utl {

	Timer::Timer() noexcept :
		Timer(0)
	{
	}


	Timer::Timer(uint32_t duration) noexcept
	{
		start(duration);
	}


	void Timer::start(uint32_t duration) noexcept
	{
		_due_time = ::GetTickCount64() + (uint64_t) duration;
	}


	bool Timer::is_elapsed() const noexcept
	{
		return ::GetTickCount64() > _due_time;
	}


	uint32_t Timer::remaining_time() const noexcept
	{
		const uint64_t now = ::GetTickCount64();
		return now >= _due_time ? 0 : static_cast<uint32_t>(_due_time - now);
	}

}
