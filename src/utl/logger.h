#pragma once

#include <cstdarg>
#include <fstream>
#include <list>
#include <mutex>
#include <string>


#if defined(_DEBUG)
#define DEBUG_CTOR(logger)	logger.debug("<Enter> ctor::%s", __class__)
#define DEBUG_DTOR(logger)	logger.debug("<Leave> dtor::%s", __class__)
#else
#define DEBUG_CTOR(logger)
#define DEBUG_DTOR(logger)
#endif


namespace wlf::utl {

    class LogWriter;


	/**
	* The application Logger.
	*/
	class Logger final
	{
	public:
        enum class Level { LL_DEBUG = 0, LL_INFO = 1, LL_WARNING = 2, LL_ERROR = 3 };

        /**
		 * Returns the instance of the logger.
		*/
		static Logger& instance() noexcept;

		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		/**
		 * Logs a message.
		*/
        void debug(const char* format, ...) noexcept;
        void info(const char* format, ...) noexcept;
        void warning(const char* format, ...) noexcept;
        void error(const char* format, ...) noexcept;

		/**
		 * Sets the threshold to 'level'.
		 *
		 * Logging message than are less severe than the specified level are ignored.
		*/
		void set_level(Level level) noexcept;
		
		/**
		 * Returns the current level.
		*/
		inline Level get_level() const noexcept { return _level; }

		/** 
		 * Checks if the specified level is more severe than the current level
		*/
		inline bool is_enabled(Level level) const noexcept { return level >= _level; }

		/**
		 * Adds a writer to this logger.
		*/
		void add_writer(LogWriter* writer);

		/**
		 * Removes a writer from this logger.
		*/
		void remove_writer(LogWriter* writer);

	private:
		Logger() noexcept;
		
		// A list of writers.
		std::list<LogWriter *> _writers;

		// A mutex to protect access to the list of writers.
		std::mutex _mutex;

		// The current logger level.
		Level _level;

        // Helper
        void log(Level level, const char* text) noexcept;
        void log(Level level, const char* format, ...) noexcept;
        void log(Level level, const char* format, va_list args) noexcept;

		/**
		 * Writes a message to the log writers.
		*/
		void write(Level level, const char* str) noexcept;
		void write(Level level, const char* format, va_list args) noexcept;
	};


	/**
	 * An abstract log writer.
	*/
	class LogWriter abstract
	{
	public:
        LogWriter() = delete;
		explicit LogWriter(Logger::Level level) noexcept;
		virtual ~LogWriter() = default;

		virtual void write(Logger::Level level, const char* str) = 0 ;
		virtual void flush() { return; }

		/**
		 * Checks if the specified level is more severe than the current level
		*/
		inline bool is_enabled(Logger::Level level) const noexcept { return level >= _level; }

    protected:
        const char level_code(Logger::Level level) const;

	private:
		// The current writer log level.
		const Logger::Level _level;
	};


	/**
	 * A file log writer.
	*/
	class FileLogWriter final: public LogWriter
	{
	public:
		explicit FileLogWriter(Logger::Level level) noexcept;
		~FileLogWriter() override = default;

		bool open(const std::wstring& filename);
		void write(Logger::Level level, const char* str) override;
		void flush() override;

	private:
		std::ofstream _ofs;
	};


    /**
     * A console log writer.
    */
    class ConsoleLogWriter : public utl::LogWriter
    {
    public:
        explicit ConsoleLogWriter(Logger::Level level) noexcept;
        ~ConsoleLogWriter() override = default;

        void write(Logger::Level level, const char* str) override;
        void flush() override;
    };

}
