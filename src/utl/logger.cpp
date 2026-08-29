/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#include "logger.h"

#include <array>
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <ostream>
#include <thread>
#include <cstdio>
#include <iostream>


namespace wlf::utl {

	Logger& Logger::instance() noexcept
	{
		static Logger logger;
		return logger;
	}


	Logger::Logger() noexcept :
		_writers(),
		_mutex(),
		_level(Level::LL_INFO)
	{
	}


	void Logger::log(Level level, const char* text) noexcept
	{
		if (is_enabled(level)) {
			write(level, text);
		}
	}


	void Logger::log(Level level, const char* format, ...) noexcept
	{
		if (is_enabled(level)) {
			va_list args;
			va_start(args, format);
			write(level, format, args);
			va_end(args);
		}
	}


	void Logger::log(Level level, const char* format, va_list args) noexcept
	{
		if (is_enabled(level))
			write(level, format, args);
	}


    void Logger::debug(const char* format, ...) noexcept
    {
        if (is_enabled(Level::LL_DEBUG)) {
            va_list args;
            va_start(args, format);
            write(Level::LL_DEBUG, format, args);
            va_end(args);
        }
    }


    void Logger::info(const char* format, ...) noexcept
    {
        if (is_enabled(Level::LL_INFO)) {
            va_list args;
            va_start(args, format);
            write(Level::LL_INFO, format, args);
            va_end(args);
        }
    }


	void Logger::warning(const char* format, ...) noexcept
	{
		if (is_enabled(Level::LL_WARNING)) {
			va_list args;
			va_start(args, format);
			write(Level::LL_WARNING, format, args);
			va_end(args);
		}
	}


	void Logger::error(const char* format, ...) noexcept
	{
		va_list args;
		va_start(args, format);
		write(Level::LL_ERROR, format, args);
		va_end(args);
	}


	void Logger::set_level(Level level) noexcept
	{
		_level = level;
	}


	void Logger::add_writer(LogWriter* writer)
	{
		if (writer) {
			std::lock_guard<std::mutex> lock(_mutex);

			_writers.push_back(writer);
			_writers.unique();
		}
	}


	void Logger::remove_writer(LogWriter* writer)
	{
		if (writer) {
			std::lock_guard<std::mutex> lock(_mutex);

			_writers.remove(writer);
		}
	}


	void Logger::write(Level level, const char* str) noexcept
	{
		try {
			std::lock_guard<std::mutex> lock(_mutex);

			for (auto& writer : _writers) {
				writer->write(level, str);
				writer->flush();
			}
		}
		catch (...) {
			return;
		}
	}


	void Logger::write(Level level, const char* format, va_list args) noexcept
	{
		try {
            std::array<char, 2048> buffer{ '\0' };
            if (std::vsnprintf(buffer.data(), buffer.size(), format, args) > 0) {
                write(level, buffer.data());
            }
		}
		catch (...) {
			return;
		}
	}


    LogWriter::LogWriter(Logger::Level level) noexcept :
		_level(level)
	{
	}


    const char LogWriter::level_code(Logger::Level level) const
    {
        switch (level) {
        case Logger::Level::LL_INFO: return 'I';
        case Logger::Level::LL_WARNING: return 'W';
        case Logger::Level::LL_ERROR: return 'E';
        default:
        case Logger::Level::LL_DEBUG: return 'D';
        }
    }


	FileLogWriter::FileLogWriter(Logger::Level level) noexcept :
		LogWriter(level),
		_ofs()
	{
	}


	bool FileLogWriter::open(const std::wstring& filename)
	{
		_ofs.open(filename, std::ostream::out);
		return _ofs.is_open();
	}


	void FileLogWriter::write(Logger::Level level, const char* str)
	{
		if (_ofs.is_open() && is_enabled(level)) {
            tm local_time;
            const time_t now = time(nullptr);
            localtime_s(&local_time, &now);

            const char* date_time = "-";
            std::array<char, 128> buffer = { '\0' };
            if (std::strftime(buffer.data(), buffer.size(), "%F %T", &local_time) > 0)
                date_time = buffer.data();
        
            _ofs
                << '[' << date_time << "] "
                << '[' << level_code(level) << "] "
                << '[' << std::setw(5) << std::setfill('0') << std::this_thread::get_id() << "] "
                << " "
                << str
                << std::endl;
        }
	}


	void FileLogWriter::flush()
	{
		if (_ofs.is_open()) {
			_ofs.flush();
		}
	}


    ConsoleLogWriter::ConsoleLogWriter(Logger::Level level) noexcept
        : LogWriter(level)
    {
    }


    void ConsoleLogWriter::write(Logger::Level level, const char* str)
    {
        if (str && is_enabled(level))
            std::wcout 
                << '[' << level_code(level) << "] "
                << str
                << std::endl;
    }


    void utl::ConsoleLogWriter::flush()
    {
        std::wcout.flush();
    }

}
