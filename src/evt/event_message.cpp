/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_message.h"

#include <combaseapi.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>


namespace wlf::evt {

	EventMessage::Fragment::Fragment(const size_t capacity)
		: _capacity(capacity)
		, _size(0)
	{
        assert(capacity > 0 && capacity <= max_fragment_size);
	}


    std::span<const char8_t> EventMessage::Fragment::readable_chars() const
    {
        return {
            _buffer.data(),
            size()
        };
    }


    std::span<char8_t> EventMessage::Fragment::writable_chars()
    {
        return {
            _buffer.data() + _size,
            free_space()
        };
    }


    void EventMessage::Fragment::advance(size_t count) noexcept
	{
        assert(count <= free_space());
		_size += count;
		assert(_size <= _capacity);
	}


    void evt::EventMessage::Fragment::retreat(size_t count) noexcept
    {
        assert(count <= _size);
        _size -= count;
    }


    evt::EventMessage::EventMessage(size_t capacity)
        : _fragments{ Fragment(capacity) }
    {
    }


    size_t EventMessage::size() const noexcept
    {
        size_t size = 0;
        for (auto& fragment : fragments())
            size += fragment.size();

        return size;
    }


	EventMessageBuilder::EventMessageBuilder(size_t capacity) 
		: EventMessage(std::min(max_fragment_size, capacity))
        , _capacity(capacity)
        , _tail(_fragments.begin())
        , _current(_fragments.begin())
        , _savepoint(_fragments.end())
        , _offset(0)
	{
	}

    bool EventMessageBuilder::write_chars(const char8_t* str, size_t size)
	{
        if (!str)
            return true;

        while (size > 0) {
            // Allocate a new fragment if the current fragment is full.
            // Stop writing if the buffer is full.
            if (_current->free_space() == 0 && !append_fragment())
                return false;

            // Get the current segment
            auto segment = _current->writable_chars();
            char8_t* segment_data = segment.data();

            // index in the destination segment
            size_t j = 0;

            while (size > 0 && j < segment.size()) {
                segment_data[j++] = *str++;
                size -= 1;
            }

            _current->advance(j);
        }

		return true;
	}


    bool EventMessageBuilder::write_chars(const wchar_t* str, size_t size)
    {
        if (!str)
            return true;

        // Number of UTF-16 code units converted in one iteration.
        constexpr size_t max_chunk_size = 512;

        // UTF-8 requires at most 3 bytes per UTF-16 code unit for BMP
        // characters, and 4 bytes for a surrogate pair.  Therefore 
        // a buffer of size = 3 * number of UTF16 code units is sufficient.
        constexpr size_t utf8_buffer_size = max_chunk_size * 3;
        std::array<char8_t, utf8_buffer_size> utf8_buffer;

        while (size > 0) {
            // Determine chunk size
            size_t chunk_size = std::min(size, max_chunk_size);

            // Do not split a UTF-16 surrogate pair between chunks.
            if (chunk_size < size &&
                IS_HIGH_SURROGATE(str[chunk_size - 1])) {
                --chunk_size;
            }

            int converted_bytes = ::WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                str,
                static_cast<int>(chunk_size),
                reinterpret_cast<char*>(utf8_buffer.data()),
                static_cast<int>(utf8_buffer.size()),
                nullptr,
                nullptr
            );

            if (converted_bytes <= 0)
                return false;       // conversion error

            if (!write_chars(utf8_buffer.data(), converted_bytes))
                return false;       // buffer overflow

            size -= chunk_size;
            str += chunk_size;
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


	static char* write_fixed_num(char* ptr, unsigned short val, int max_digits) noexcept
	{
        assert(val < 10000);

		// Pre-fill the exact slice with '0' characters
		for (int i = 0; i < max_digits; ++i) {
			ptr[i] = '0';
		}

		// Find how many digits the actual number needs
		int width = 1;
		if (val >= 10)
			width = (val < 100) ? 2 : ((val < 1000) ? 3 : 4);

		// Write the number aligned to the right side of the slot
		std::to_chars(ptr + (max_digits - width), ptr + max_digits, val);
		return ptr + max_digits;
	}


	bool EventMessageBuilder::append(const::SYSTEMTIME& st) noexcept
	{
		// Format: YYYY-MM-DDTHH:MM:SS.mmmZ (24 characters)
		std::array<char, 24> ts = { 0 };
		char* p = ts.data();

		p = write_fixed_num(p, st.wYear, 4);
		*p++ = '-';
		p = write_fixed_num(p, st.wMonth, 2);
		*p++ = '-';
		p = write_fixed_num(p, st.wDay, 2);
		*p++ = 'T';
		p = write_fixed_num(p, st.wHour, 2);
		*p++ = ':';
		p = write_fixed_num(p, st.wMinute, 2);
		*p++ = ':';
		p = write_fixed_num(p, st.wSecond, 2);
		*p++ = '.';
		p = write_fixed_num(p, st.wMilliseconds, 3);
		*p++ = 'Z';

		return write_chars((char8_t *)ts.data(), ts.size());
	}


    bool evt::EventMessageBuilder::append(const::GUID* guid) noexcept
    {
        if (!guid)
            return true;

        constexpr int buffer_size = 39;
        std::array<wchar_t, buffer_size> buffer{};
        
        const int guid_size = StringFromGUID2(*guid, buffer.data(), buffer_size);
        if (guid_size <= 1)
            return true;

        return write_chars(buffer.data(), static_cast<size_t>(guid_size) - 1);
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


    size_t EventMessageBuilder::free_space() const noexcept
    {
        return _capacity - size();
    }


    EventMessageBuilder::Savepoint EventMessageBuilder::savepoint() noexcept
    {
        assert(_savepoint == _fragments.end());

        _savepoint = _current;
        _offset = _savepoint->size();

        return Savepoint(*this);
    }


    bool EventMessageBuilder::append_fragment()
    {
        const size_t fragment_capacity = std::min(max_fragment_size, free_space());

        if (fragment_capacity > 0) {
            if (_current != _fragments.end())
                ++_current; // move inside an existing fragment list

            if (_current == _fragments.end()) {
                _tail = _fragments.emplace_after(_tail, fragment_capacity);
                _current = _tail;
            }
        }

        return fragment_capacity > 0;
    }


    EventMessageBuilder::Savepoint::Savepoint(EventMessageBuilder& builder) noexcept
        : _builder(builder)
        , _committed(false)
    {
    }


    EventMessageBuilder::Savepoint::~Savepoint()
    {
        if (!_committed)
            _builder.rollback();
    }


    bool EventMessageBuilder::Savepoint::commit() noexcept
    {
        _builder.commit();
        _committed = true;

        return _committed;
    }


    void EventMessageBuilder::commit() noexcept
    {
        assert(_savepoint != _fragments.end());

        _savepoint = _fragments.end();
        _offset = 0;
    }


    void EventMessageBuilder::rollback() noexcept
    {
        assert(_savepoint != _fragments.end());

        for (auto fragment = _savepoint; fragment != _fragments.end(); ++fragment)
            fragment->retreat(fragment->size());

        _savepoint->advance(_offset);
        _current = _savepoint;
        _savepoint = _fragments.end();
        _offset = 0;
    }

}