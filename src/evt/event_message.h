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
    constexpr size_t max_fragment_size = 512;

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
			inline size_t free_space() const { return _capacity - size(); }

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

            void undo(size_t count);

		private:
            // Maximum capacity of the fragment.
			const size_t _capacity;

            // Current number of characters stored in this fragment
			size_t _size;

            // Underlying character buffer
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
        size_t size() const;

	protected:
        // The Collection of message fragments
		FragmentList _fragments;

        // Allocate a message with an empty fragment.
        EventMessage();
	};

	using EventMessagePtr = std::shared_ptr<EventMessage> ;


	class EventMessageBuilder : public EventMessage {
	public:
        class Guard {
        public:
            Guard(EventMessageBuilder& builder);
            ~Guard();

            bool commit(size_t free_space);

        private:
            EventMessageBuilder& _builder;
            bool _commited;
        };


        /**
         * Constructs a builder with a maximum capacity.
         */
        explicit EventMessageBuilder(size_t capacity);

        /**
         * Writes a sequence of UTF-8 characters to the message buffer.
         * @return true if successful; false if the buffer is full.
         */
		bool write_chars(const char8_t* str, size_t size);
        bool write_chars(const wchar_t* str, size_t size);

        /**
		 * Appends a single character.
		 * @return true if successful; false if the buffer is full.
		*/
        inline bool append(char8_t c) { return write_chars(&c, 1); }
        inline bool append(wchar_t c) { return write_chars(&c, 1); }

        /**
         * Appends at most count character from a null-terminated UTF-8/ASCII string.
         * @return true if successful; false if the buffer is full.
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
                   truncated.
         */
        bool append(const ::SYSTEMTIME& st) noexcept;

        /**
         * Formats and appends a GUID as an ASCII string.
         * @return true if successfully formatted and appended; false if
         *         truncated.
         */
        bool append(const ::GUID* guid) noexcept;

        /**
         * Appends the contents of another EventMessage.
         * @return true if successful; false if the buffer is full.
         */
        bool append(const EventMessage& message) noexcept;

        inline size_t capacity() const noexcept { return _capacity;  }

        /**
        * @return
        */
        size_t free_space() const noexcept;


        Guard mark() noexcept;

	private:
        // Maximum allowed capacity for the builder
		const size_t _capacity;

        // A reference to the last fragment of the list
		FragmentList::iterator _tail;

        // A reference to the current fragment
        FragmentList::iterator _current;

        // Saved position.
        FragmentList::iterator _mark;
        size_t _offset;


		bool append_fragment(size_t min_size);
        void commit();
        void rollback();
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