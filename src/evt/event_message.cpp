#include "event_message.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstring>



namespace wlf::evt {

	size_t EventMessage::length() const
	{
		size_t length = 0;
		for (auto& fragment : fragments())
			length += fragment.size();

		return length;
	}


	EventMessage::Fragment::Fragment(const size_t capacity)
		: _capacity(capacity)
		, _size(0)
		, _data(std::make_unique<char8_t[]>(capacity))
	{
	}


    std::span<const char8_t> evt::EventMessage::Fragment::readable_chars() const
    {
        return std::span<const char8_t>(&_data[0], size());
    }


    std::span<char8_t> evt::EventMessage::Fragment::writable_chars()
    {
        return std::span<char8_t>(&_data[_size], free_space());
    }


    void EventMessage::Fragment::advance(size_t count)
	{
		_size += count;
		assert(_size <= _capacity);
	}


	EventMessageBuilder::EventMessageBuilder(size_t capacity, size_t fragment_capacity) 
		: EventMessage()
        , _capacity(capacity)
        , _fragment_capacity(fragment_capacity)
        , _size(0)
	{
	}

	bool EventMessageBuilder::write_chars(const char8_t* str, const size_t size, bool escape)
	{
        if (!str)
            return true;

        // index in source string
        size_t i = 0;

        // Track pending character if an escape sequence splits across segments
        bool has_pending = false;
        char8_t pending_char = u8'\0';

        // A lambda that checks whether escape is required 
        auto needs_escape = [escape](char8_t c) {
            return escape && (c == u8'"' || c == u8'\\');
        };

        while (i < size || has_pending) {
            // Allocate a new fragment if none exists or if the current one is full.
            if ((_tail == _fragments.end() || _tail->is_full()) && !append_fragment())
                return false;

            // Get the current segment
            auto segment = _tail->writable_chars();
            assert(segment.size() > 0);

            // index in the destination segment
            size_t j = 0;

            // Copy characters from the source string to the fragment until
            // the fragment is full.  When escaping characters, it could occur
            // that the backslash is in the current segment and the escaped
            // character is in the next fragment.
            if (has_pending) {
                segment.data()[j++] = pending_char;
                has_pending = false;
            }

            while (i < size && j < segment.size()) {
                const char8_t c = str[i++];

                if (needs_escape(c))
                    segment.data()[j++] = u8'\\';

                if (j < segment.size()) {
                    segment.data()[j++] = c;
                }
                else {
                    // segment is now full; carry the character over
                    has_pending = true;
                    pending_char = c;
                }
            }

            _tail->advance(j);
        }

		return true;
	}


    bool EventMessageBuilder::append_fragment()
	{
		if (_size + _fragment_capacity > _capacity)
			return false;

		_tail = _fragments.emplace_after(_tail, _fragment_capacity);
        _size += _fragment_capacity;

		return true;
	}


	bool EventMessageBuilder::append(const char8_t* str, size_t count, bool escape) noexcept
	{
		if (!str)
			return true;

		const size_t len = std::min(count, std::strlen((char*)str));
		return write_chars(str, len, escape);
	}


	static char* write_num(char* ptr, unsigned short val, int digits) noexcept
	{
		// Pre-fill the exact slice with '0' characters
		for (int i = 0; i < digits; ++i) {
			ptr[i] = '0';
		}

		// Find how many digits the actual number needs
		int width = 1;
		if (val >= 10)
			width = (val < 100) ? 2 : ((val < 1000) ? 3 : 4);

		// Write the number aligned to the right side of the slot
		std::to_chars(ptr + (digits - width), ptr + digits, val);
		return ptr + digits;
	}


	bool EventMessageBuilder::append(const::SYSTEMTIME& st) noexcept
	{
		// Format: YYYY-MM-DDTHH:MM:SS.mmmZ (24 characters)
		std::array<char, 24> ts = { 0 };
		char* p = ts.data();

		p = write_num(p, st.wYear, 4);
		*p++ = '-';
		p = write_num(p, st.wMonth, 2);
		*p++ = '-';
		p = write_num(p, st.wDay, 2);
		*p++ = 'T';
		p = write_num(p, st.wHour, 2);
		*p++ = ':';
		p = write_num(p, st.wMinute, 2);
		*p++ = ':';
		p = write_num(p, st.wSecond, 2);
		*p++ = '.';
		p = write_num(p, st.wMilliseconds, 3);
		*p++ = 'Z';

		return write_chars((char8_t *)ts.data(), ts.size(), false);
	}


	bool EventMessageBuilder::append(const::FILETIME& ft) noexcept
	{
		SYSTEMTIME st;
		if (!FileTimeToSystemTime(&ft, &st)) {
			return append('-');
		}
		else {
			return append(st);
		}
	}


	bool EventMessageBuilder::append(const EventMessage& message) noexcept
	{
		for (const auto& fragment : message.fragments()) {
			const auto fragment_data = fragment.readable_chars();
			if (!write_chars(fragment_data.data(), fragment_data.size(), false))
				return false;
		}

		return true;
	}

}
