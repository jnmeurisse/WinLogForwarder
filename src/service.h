#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>

#include "svc/service_context.h"
#include "utl/logger.h"

namespace wlf {

    class Service {
    public:
        ~Service();

        // Service instance
        static Service& instance() noexcept;

        // Main entry point to parse command line and run
        int run(int argc, wchar_t* argv[]);

    private:
        Service();

        // The application logger
        utl::Logger& _logger;

        ::SERVICE_STATUS_HANDLE _status_handle;
        ::SERVICE_STATUS _status;

        std::unique_ptr<svc::ServiceContext> _service_context;

        // Static member function required for Windows Service Control Manager (SCM) callbacks
        static void WINAPI ServiceMain(::DWORD argc, ::LPTSTR* argv);
        static void WINAPI ServiceCtrlHandler(::DWORD ctrlCode);

        // Internal helper methods
        bool install();
        bool uninstall();
        bool register_service();
        void run_interactive();
        void stop_interactive();
        bool init_service() noexcept;
        void start_service();
        void stop_service();
        void set_status(DWORD currentState, DWORD win32ExitCode = NO_ERROR, DWORD waitHint = 0);
    };

}