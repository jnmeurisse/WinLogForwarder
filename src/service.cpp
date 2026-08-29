/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#include "service.h"

#include <iostream>
#include <filesystem>
#include <system_error>
#include "global.h"
#include "svc/service_config.h"
#include "utl/system.h"
#include "utl/ctrlchandler.h"


namespace wlf {

    Service::Service()
        : _logger(utl::Logger::instance())
        , _status_handle(nullptr)
        , _status{}
        , _service_context(nullptr)
    {
        _status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        _status.dwCurrentState = SERVICE_STOPPED;
        _status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        _status.dwWin32ExitCode = 0;
        _status.dwServiceSpecificExitCode = 0;
        _status.dwCheckPoint = 0;
        _status.dwWaitHint = 0;
    }


    Service::~Service()
    {
    }


    Service& Service::instance() noexcept
    {
        static Service service;

        return service;
    }


    int Service::run(int argc, wchar_t* argv[])
    {
        if (argc == 2 ) {
            const std::wstring flag = argv[1];

#if defined(_DEBUG)
            _logger.set_level(utl::Logger::Level::LL_DEBUG);
#endif

            auto console_writer = std::make_unique<utl::ConsoleLogWriter>(utl::Logger::Level::LL_DEBUG);
            _logger.add_writer(console_writer.get());

            if (flag == L"/install") {
                return install() ? 0 : 1;
            }
            else if (flag == L"/uninstall") {
                return uninstall() ? 0 : 1;
            }
            else if (flag == L"/interactive") {
                run_interactive();
                return 0;
            }

            _logger.remove_writer(console_writer.get());
        }
        else if (argc == 1) {
            static ::SERVICE_TABLE_ENTRY service_table[] = {
                { const_cast<LPWSTR>(ApplicationServiceName), ServiceMain },
                { nullptr, nullptr }
            };

            if (!StartServiceCtrlDispatcherW(service_table)) {
                std::wcout << L"Failed to start service context. Use /interactive for CLI mode.\n";
                return 1;
            }
        }
        else {
            std::wcout << L"Invalid usage.\n";
            return 1;
        }

        return 0;
    }


    bool Service::install()
    {
        // 
        wchar_t path[MAX_PATH];
        if (!::GetModuleFileName(nullptr, path, MAX_PATH)) {
            _logger.error("Failed to get binary path.");

            return false;
        }

        ::SC_HANDLE scm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (!scm) {
            _logger.error("Fatal: failed to open SCM. Try running as Administrator.");
            return false;
        }

        // Create an application folder if not yet done.
        std::error_code ec;
        auto app_data_path = utl::get_program_data();
        app_data_path.append(ApplicationServiceName);
        if (!std::filesystem::exists(app_data_path)) {
            if (!std::filesystem::create_directory(app_data_path, ec))
                _logger.info("Warning: failed to create an application folder.");
        }

        ::SC_HANDLE service = ::CreateServiceW(
            scm, ApplicationServiceName, ApplicationDisplayName,
            SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
            path, nullptr, nullptr, nullptr, nullptr, nullptr
        );

        if (!service) {
            _logger.error("Fatal: failed to create service. Error: %x", ::GetLastError());
            ::CloseHandle(scm);

            return false;
        }

        _logger.info("Service installed successfully.");
        ::CloseServiceHandle(service);
        ::CloseServiceHandle(scm);

        return true;
    }


    bool Service::uninstall() {
        ::SC_HANDLE scm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (!scm) {
            _logger.error("Fatal: failed to open SCM. Try running as Administrator.");
            return false;
        }

        ::SC_HANDLE service = ::OpenService(scm, ApplicationServiceName, SERVICE_STOP | DELETE);
        if (!service) {
            _logger.error("Fatal: Service not found or access denied.");
            ::CloseServiceHandle(scm);

            return false;
        }

        // Attempt to stop it first if running
        ::SERVICE_STATUS status;
        ::ControlService(service, SERVICE_CONTROL_STOP, &status);

        if (!::DeleteService(service)) {
            _logger.error("Fatal: Failed to delete service. Error: %x", ::GetLastError());
            ::CloseServiceHandle(service);
            ::CloseServiceHandle(scm);

            return false;
        }

        _logger.info("Service uninstalled successfully.");
        ::CloseServiceHandle(service);
        ::CloseServiceHandle(scm);

        return true;
    }


    bool Service::register_service()
    {
        _status_handle = ::RegisterServiceCtrlHandler(ApplicationServiceName, ServiceCtrlHandler);
        return _status_handle != nullptr;
    }


    void Service::run_interactive() 
    {
        utl::CtrlcHandler ctrlc_handler([this] {this->stop_interactive(); });

        try {
            std::wcout << L"Running in interactive mode. Press Ctrl+C to exit...\n";

            if (init_service())
                _service_context->start();

            _service_context.release();
        }
        catch (const std::exception& e) {
            std::wcout << e.what() << "\n";
        }
    }


    void Service::stop_interactive()
    {
        _logger.info("Ctrl+C pressed, stopping...");

        if (_service_context)
            _service_context->stop();
    }


    bool Service::init_service() noexcept
    {
        svc::ServiceConfig config;

        std::filesystem::path config_path = utl::get_module_path();
        config_path.replace_extension("ini");
        if (!std::filesystem::exists(config_path)) {
            _logger.error("Fatal : Configuration file '%S' not found.", config_path.native().c_str());
            return false;
        }

        try {
            _logger.info("Loading configuration file '%S'.", config_path.native().c_str());
            const auto root_table = cpptoml::parse_file(config_path.c_str());
            if (!root_table) {
                _logger.error("Fatal: configuration file empty.");
                return false;
            }

            if (!config.load(*root_table)) {
                _logger.error("Fatal: configuration error.");
                return false;
            }

            _service_context = std::make_unique<svc::ServiceContext>(config);
        }
        catch (const std::exception& e) {
            _logger.error("Fatal: configuration error.");
            _logger.error(e.what());
            return false;
        }

        return true;
    }


    void WINAPI Service::ServiceMain(::DWORD argc, ::LPTSTR* argv)
    {
        Service& instance = Service::instance();

        if (instance.register_service() && instance.init_service()) {
            instance.set_status(SERVICE_RUNNING);
            instance.start_service();
            instance.set_status(SERVICE_STOPPED);
        }
    }


    void WINAPI Service::ServiceCtrlHandler(::DWORD ctrlCode)
    {
        Service& instance = Service::instance();

        switch (ctrlCode) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            instance.set_status(SERVICE_STOP_PENDING);
            instance.stop_service();
            break;

        default:
            break;
        }
    }


    void Service::set_status(::DWORD currentState, ::DWORD win32ExitCode, ::DWORD waitHint) 
    {
        _status.dwCurrentState = currentState;
        _status.dwWin32ExitCode = win32ExitCode;
        _status.dwWaitHint = waitHint;
        ::SetServiceStatus(_status_handle, &_status);
    }


    void Service::start_service()
    {
        if (_service_context) {
            _service_context->start();
            _service_context.release();
        }
    }


    void Service::stop_service()
    {
        if (_service_context) {
            _service_context->stop();
        }
    }

}
