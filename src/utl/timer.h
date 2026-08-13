#pragma once
#include <cstdint>

namespace wlf::utl {

	class Timer final
	{
	public:
		/**
		 * Constructs a timer.
		 * 
		 * The timer must be started by calling start.
		*/
		Timer() noexcept;

		/**
		 * Constructs and starts the timer for the specified duration (ms).
		*/
		explicit Timer(uint32_t duration) noexcept;

		/**
		 * Restarts the timer for the specified duration (ms).
		*/
		void start(uint32_t duration) noexcept;

		/**
		 * Returns true if the timer has elapsed.
		*/
		bool is_elapsed() const noexcept;

		/**
		 * Returns the remaining time (ms) before the timer elapses.
		*/
		uint32_t remaining_time() const noexcept;

	private:
		// End time of the timer.
		uint64_t _due_time;
	};

}
