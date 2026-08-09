#include "event_message.h"

#include <combaseapi.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstring>
#include <memory>



namespace wlf::evt {

	size_t EventMessage::size() const
	{
		size_t size = 0;
		for (auto& fragment : fragments())
            size += fragment.size();

		return size;
	}


	EventMessage::Fragment::Fragment(const size_t capacity)
		: _capacity(capacity)
		, _size(0)
        , _buffer{u8'\0'}
	{
	}


    std::span<const char8_t> EventMessage::Fragment::readable_chars() const
    {
        return std::span<const char8_t>(_buffer.data(), size());
    }


    std::span<char8_t> EventMessage::Fragment::writable_chars()
    {
        return std::span<char8_t>(&_buffer[_size], free_space());
    }


    void EventMessage::Fragment::advance(size_t count)
	{
		_size += count;
		assert(_size <= _capacity);
	}


	EventMessageBuilder::EventMessageBuilder(size_t capacity) 
		: EventMessage()
        , _capacity(capacity)
	{
	}


	bool EventMessageBuilder::write_chars(const char8_t* str, size_t size)
	{
        if (!str)
            return true;

        // index in source string
        size_t i = 0;

        while (size > 0) {
            // Allocate a new fragment if none exists or if the current fragment is full.
            if ((_tail == _fragments.end() || _tail->free_space() < 4) && !append_fragment())
                return false;

            // Get the current segment
            auto segment = _tail->writable_chars();
            assert(segment.size() >= 4);

            // index in the destination segment
            size_t j = 0;

            while (size > 0 && j < segment.size() - 4)
            {
                // 0xxxxxxx -> U+0000..U+007F
                if ((str[i] & 0x80) == 0x00) {
                    segment.data()[j++] = str[i++];
                    size -= 1;
                }
                // 110xxxxx -> U+0080..U+07FF
                else if ((str[i] & 0xE0) == 0xC0 && size >= 2) {
                    segment.data()[j++] = str[i++];
                    segment.data()[j++] = str[i++];
                    size -= 2;
                }
                // 1110xxxx -> U+0800-U+FFFF
                else if ((str[i] & 0xF0) == 0xE0 && size >= 3) {
                    segment.data()[j++] = str[i++];
                    segment.data()[j++] = str[i++];
                    segment.data()[j++] = str[i++];
                    size -= 3;
                }
                // 11110xxx -> U+10000..U+10FFFF
                else if ((str[i] & 0xF8) == 0xF0 && size >= 4) {
                    segment.data()[j++] = str[i++];
                    segment.data()[j++] = str[i++];
                    segment.data()[j++] = str[i++];
                    segment.data()[j++] = str[i++];
                    size -= 4;
                }
                // 10xxxxxx or 11111xxx -> invalid
                else {
                    i += 1;
                    size -= 1;
                }
            }

            _tail->advance(j);
        }

		return true;
	}


    bool EventMessageBuilder::write_chars(const wchar_t* str, size_t size)
    {
        constexpr size_t max_char_per_chunk = 512;
        std::array<char8_t, max_char_per_chunk * 4> utf8_buffer;

        size_t processed_chars = 0;

        while (size > 0) {
            // Determine chunk size
            size_t chunk_chars = std::min(size, max_char_per_chunk);
            
            const wchar_t* current = str + processed_chars;

            // Prevent splitting a UTF-16 surrogate pair at the chunk boundary
            if (chunk_chars < size) {
                // Check if the last character in the chunk is a high surrogate
                wchar_t last_char = current[chunk_chars - 1];
                if (last_char >= 0xD800 && last_char <= 0xDBFF) {
                    // Backtrack by 1 character to keep the surrogate pair together
                    chunk_chars--;
                }
            }

            // Safety check to prevent infinite loops if a single character is huge/malformed
            if (chunk_chars == 0)
                break;

            int converted_bytes = ::WideCharToMultiByte(
                CP_UTF8,
                0,
                current,
                static_cast<int>(chunk_chars),
                reinterpret_cast<char*>(utf8_buffer.data()),
                static_cast<int>(utf8_buffer.size()),
                nullptr,
                nullptr
            );

            if (converted_bytes > 0) {
                if (!write_chars(utf8_buffer.data(), converted_bytes))
                    return false;
            }
            else {
                // conversion error
                return false;
            }

            processed_chars += chunk_chars;
        }

        return true;
    }


    bool evt::EventMessageBuilder::append(std::u8string_view strv)
    {
        return write_chars(strv.data(), strv.size());
    }


    bool EventMessageBuilder::append(std::wstring_view strv)
    {
        return write_chars(strv.data(), strv.size());
    }


    bool EventMessageBuilder::append_fragment()
	{
        size_t fragment_capacity = std::min(max_fragment_size, free_space());

        if (fragment_capacity > 0)
		    _tail = _fragments.emplace_after(_tail, fragment_capacity);

		return fragment_capacity > 0;
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

		return write_chars((char8_t *)ts.data(), ts.size());
	}


    bool evt::EventMessageBuilder::append(const::GUID* guid) noexcept
    {
        constexpr int buffer_size = 256;
        std::array<wchar_t, buffer_size> buffer{};
        
        const int guid_size = StringFromGUID2(*guid, buffer.data(), buffer_size);
        return write_chars(buffer.data(), guid_size);
    }

    
    bool EventMessageBuilder::append(const EventMessage& message) noexcept
	{
		for (const auto& fragment : message.fragments()) {
			const auto fragment_data = fragment.readable_chars();
			if (!write_chars(fragment_data.data(), fragment_data.size()))
				return false;
		}

		return true;
	}

}
