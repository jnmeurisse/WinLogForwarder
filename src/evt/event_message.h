#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <charconv>
#include <forward_list>
#include <memory>
#include <span>
#include <string_view>


namespace wlf::evt {
    constexpr size_t max_fragment_size = 4;

    /**
     * Represents an event message composed of fragmented character buffers.
    */
	class EventMessage {
	public:

		/**
		 * Represents a fragment of an event message.
		 */
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
			inline bool is_full() const noexcept { return _size == _capacity; }

            /**
             * Returns the amount of free space remaining in the fragment.
             */
			inline size_t free_space() const noexcept { return _capacity - _size; }

            /**
             * Returns the current number of characters stored in the fragment.
             */
            inline size_t size() const noexcept { return _size; }

		private:
			friend class EventMessageBuilder;

            /**
             * Returns a writable view of the unpopulated portion of the fragment buffer.
             */
            std::span<char8_t> writable_chars();
			
            /**
             * Advances the internal size counter by the given count.
             */
            void advance(size_t count) noexcept;

            /**
             * Retreats the internal size counter by the given count.
             */
            void retreat(size_t count) noexcept;

		private:
            // Maximum capacity of the fragment.
			const size_t _capacity;

            // Current number of characters stored in this fragment
			size_t _size;

            // Underlying characters buffer
            std::array<char8_t, max_fragment_size> _buffer;
        };

		using FragmentList = std::forward_list<Fragment>;

        /**
         * Returns a constant reference to the underlying list of fragments.
         */
		inline const FragmentList& fragments() const noexcept { return _fragments; }

        /**
         * Calculates and returns the total size of the message across all fragments. 
         */
        size_t size() const noexcept;

	protected:
        // The Collection of message fragments
		FragmentList _fragments;

        // Allocates a message with an initial fragment of the given capacity.
        EventMessage(size_t capacity);
	};

	using EventMessagePtr = std::unique_ptr<EventMessage> ;


	/**
	 * A message builder.
	 */
	class EventMessageBuilder : public EventMessage {
	public:

        class Savepoint {
        public:
            Savepoint(EventMessageBuilder& builder) noexcept;
            Savepoint(const Savepoint&) = delete;
            ~Savepoint();

            /**
             * Commits changes in the message builder.
             * @return true if the changes have been committed.
             */
            bool commit() noexcept;

        private:
            EventMessageBuilder& _builder;
            bool _committed;
        };

        /**
         * Constructs a builder with a maximum capacity (in bytes).
         */
        explicit EventMessageBuilder(size_t capacity);

        /**
         * Writes a sequence of characters to the message builder.
         * 
         * @param str A sequence of characters.
         * @param size The number of characters in the given string buffer.
         * @return true if successful; false if buffer is full.
         */
		bool write_chars(const char8_t* str, size_t size);

        /**
         * Append UTF-16 characters converted to UTF-8.
         *
         * The input is interpreted as UTF-16 code units. The input must contain
         * valid UTF-16; malformed surrogate sequences cause the operation to fail.
         *
         * The conversion is performed incrementally using a fixed-size temporary
         * buffer, so the function does not allocate dynamically.
         * @return true if successful; false if buffer is full or if the input
         *         is malformed. 
         */
        bool write_chars(const wchar_t* str, size_t size);

        /**
		 * Appends a single character.
		 * @return true if successful; false if the buffer is full.
		*/
        inline bool append(char8_t c) { return write_chars(&c, 1); }
        inline bool append(wchar_t c) { return write_chars(&c, 1); }

        /**
         * Appends a sequence of UTF-8 characters from a string view into
         * the message builder.
         * 
         * @return true if successful; false if buffer is full.
         */
        bool append(std::u8string_view strv);
        bool append(std::wstring_view strv);

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
                   buffer is full.
         */
        bool append(const ::SYSTEMTIME& st) noexcept;

        /**
         * Formats and appends a GUID as an ASCII string.
         * @return false if truncated, true in all other cases even if
         *         the GUID can not be converted.
         */
        bool append(const ::GUID* guid) noexcept;

        /**
         * Appends the contents of another EventMessage.
         * @return true if successful; false if buffer is full.
         */
        bool append(const EventMessage& message) noexcept;

        /**
         * Returns the capacity of this builder (in bytes).
         */
        inline size_t capacity() const noexcept { return _capacity;  }

        /**
        * Returns the remaining space (in bytes) in this builder.
        */
        size_t free_space() const noexcept;

        /**
         * Creates a save point.
         */
        Savepoint savepoint() noexcept;

	private:
        // Maximum allowed capacity for the builder
		const size_t _capacity;

        // A reference to the last fragment of the list
		FragmentList::iterator _tail;

        // A reference to the current fragment
        FragmentList::iterator _current;

        // Saved position (iterator to a fragment and occupied space)
        FragmentList::iterator _savepoint;
        size_t _offset;

        // Append a new fragment.  Returns false if buffer is full.
		bool append_fragment();

        // Commit pending changes
        void commit() noexcept;

        // Rollback pending changes.
        void rollback() noexcept;
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
		return write_chars((char8_t*)buffer.data(), len);
	}

}