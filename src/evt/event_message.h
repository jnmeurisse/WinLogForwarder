#pragma once

#include <Windows.h>

#include <charconv>
#include <forward_list>
#include <memory>
#include <span>


namespace wlt::evt {

    /**
     * Represents an event message composed of fragmented character buffers.
    */
	class EventMessage {

	public:
		class Fragment {
		public:
			Fragment() = delete;

            /**
             * Constructs a fragment with the specified capacity (in bytes).
             */
			explicit Fragment(const size_t capacity);

            /**
             * Returns a read-only view of the currently written characters.
             */
			std::span<const char8_t> readable_chars() const;

            /**
             * Checks if the fragment is fully occupied.
             */
			inline bool is_full() const { return _size == _capacity; }

            /**
             * Returns the amount of free space remaining in the fragment.
             */
			inline size_t free_space() const { return _capacity - _size; }

            /**
             * Returns the current number of characters stored in the fragment.
             */
            inline size_t size() const { return _size; }

		private:
			friend class EventMessageBuilder;

            /**
             * Returns a writable view of the unpopulated portion of the fragment buffer.
             */
            std::span<char8_t> writable_chars();
			
            /**
             * Advances the internal size counter by the given count.
             */
            void advance(size_t count);

		private:
            // Maximum capacity of the fragment.
			const size_t _capacity;

            // Current number of characters stored in this fragment
			size_t _size;

            // Underlying character buffer
			std::unique_ptr<char8_t[]> _data;
		};

		using FragmentList = std::forward_list<Fragment>;

        /**
         * Returns a constant reference to the underlying list of fragments.
         */
		inline const FragmentList& fragments() const noexcept { return _fragments; }

        /**
         * Calculates and returns the total length of the message across all fragments. 
         * 
         */
        size_t length() const;

	protected:
        // The Collection of message fragments
		FragmentList _fragments;
	};

	using EventMessagePtr = std::shared_ptr<EventMessage> ;


	class EventMessageBuilder : public EventMessage {
	public:
        /**
         * Constructs a builder with a maximum capacity and a fragment capacity.
         */
        explicit EventMessageBuilder(size_t capacity, size_t fragment_capacity);

        /**
         * Writes a sequence of characters to the message buffer.
         * @return true if successful; false if the buffer is full.
         */
		bool write_chars(const char8_t* str, size_t size, bool escape);
 
        /**
		 * Appends a single character.
		 * @return true if successful; false if the buffer is full.
		*/
        inline bool append(char8_t c) { return write_chars(&c, 1, false); }

        /**
         * Appends at most count character from a null-terminated UTF-8/ASCII string.
         * @return true if successful; false if the buffer is full.
         */
        bool append(const char8_t* str, size_t count, bool escape) noexcept;

        /**
         * Formats and appends an integer as an ASCII string.
         * @return true if successfully formatted and appended; false if
         *         truncated.
         */
        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        bool append(T value, size_t count) noexcept;

        /**
         * Formats a Windows SYSTEMTIME structure into an ISO-8601 UTC string
         *         (YYYY-MM-DDTHH:MM:SS.mmmZ) and appends it.
         * @return true if successfully formatted and appended; false if
                   truncated.
         */
        bool append(const ::SYSTEMTIME& st) noexcept;
        bool append(const ::FILETIME& ft) noexcept;

        /**
         * Appends the contents of another EventMessage.
         * @return true if successful; false if the buffer is full.
         */
        bool append(const EventMessage& message) noexcept;

	private:
        // Maximum allowed capacity for the builder
		const size_t _capacity;

        // Capacity of each allocated fragment
        const size_t _fragment_capacity;

        // Total allocated memory
		size_t _size;

        // A reference to the last fragment
		FragmentList::iterator _tail;

		bool append_fragment();
	};


	template <typename T, typename Enabler>
	bool EventMessageBuilder::append(T value, size_t count) noexcept
	{
		std::array<char, 64> buffer;  // large enough for any integral type

		auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
		if (ec != std::errc())
			return false;

		// Do not copy more than count characters
		const size_t len = std::min(count, static_cast<size_t>(ptr - buffer.data()));
		return append_chars((char8_t*)buffer.data(), len);
	}

}