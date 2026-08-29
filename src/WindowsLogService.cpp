#include "WindowsLogService.h"
#include <iostream>


// Initialize the static instance pointer
WindowsLogService* WindowsLogService::s_instance = nullptr;


WindowsLogService::WindowsLogService(const std::wstring& service_name, const std::wstring& display_name)
    : _service_name(service_name)
    , _display_name(display_name)
    , _status_handle(nullptr)
{
    s_instance = this;

    _status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    _status.dwCurrentState = SERVICE_STOPPED;
    _status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    _status.dwWin32ExitCode = 0;
    _status.dwServiceSpecificExitCode = 0;
    _status.dwCheckPoint = 0;
    _status.dwWaitHint = 0;
}


WindowsLogService::~WindowsLogService() {
}


int WindowsLogService::run(int argc, wchar_t* argv[]) {
    if (argc == 2) {
        std::wstring flag = argv[1];
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
    }
    else if (argc == 1) {
        // Default behavior: Try to connect to SCM (runs as a background service)
        ::SERVICE_TABLE_ENTRY serviceTable[] = {
            { const_cast<LPWSTR>(_service_name.c_str()), ServiceMain },
            { nullptr, nullptr }
        };

        if (!StartServiceCtrlDispatcherW(serviceTable)) {
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


bool WindowsLogService::install() {
    wchar_t path[MAX_PATH];
    if (!::GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        std::wcout << L"Failed to get binary path.\n";

        return false;
    }

    ::SC_HANDLE scm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        std::wcout << L"Failed to open SCM. Try running as Administrator.\n";

        return false;
    }

    ::SC_HANDLE service = ::CreateServiceW(
        scm, _service_name.c_str(), _display_name.c_str(),
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        path, nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (!service) {
        std::wcout << L"Failed to create service. Error: " << GetLastError() << L"\n";
        ::CloseHandle(scm);

        return false;
    }

    std::wcout << L"Service installed successfully.\n";
    ::CloseServiceHandle(service);
    ::CloseServiceHandle(scm);

    return true;
}


bool WindowsLogService::uninstall() {
    ::SC_HANDLE scm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        std::wcout << L"Failed to open SCM. Try running as Administrator.\n";

        return false;
    }

    ::SC_HANDLE service = ::OpenService(scm, _service_name.c_str(), SERVICE_STOP | DELETE);
    if (!service) {
        std::wcout << L"Service not found or access denied.\n";
        ::CloseServiceHandle(scm);
        
        return false;
    }

    // Attempt to stop it first if running
    ::SERVICE_STATUS status;
    ::ControlService(service, SERVICE_CONTROL_STOP, &status);

    if (!::DeleteService(service)) {
        std::wcout << L"Failed to delete service. Error: " << GetLastError() << L"\n";
        ::CloseServiceHandle(service);
        ::CloseServiceHandle(scm);
        
        return false;
    }

    std::wcout << L"Service uninstalled successfully.\n";
    ::CloseServiceHandle(service);
    ::CloseServiceHandle(scm);
    
    return true;
}


void WindowsLogService::run_interactive() {
    std::wcout << L"Running in interactive mode. Press Ctrl+C to exit...\n";
}


void WINAPI WindowsLogService::ServiceMain(DWORD argc, LPTSTR* argv) {
    if (!s_instance) return;

    s_instance->_status_handle = ::RegisterServiceCtrlHandler(s_instance->_service_name.c_str(), ServiceCtrlHandler);
    if (!s_instance->_status_handle) return;

    s_instance->set_status(SERVICE_RUNNING);
    s_instance->start_service();
    s_instance->set_status(SERVICE_STOPPED);
}


void WINAPI WindowsLogService::ServiceCtrlHandler(DWORD ctrlCode) {
    if (!s_instance) return;

    switch (ctrlCode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        s_instance->set_status(SERVICE_STOP_PENDING);
        s_instance->stop_service();
        break;
    default:
        break;
    }
}


void WindowsLogService::set_status(DWORD currentState, DWORD win32ExitCode, DWORD waitHint) {
    _status.dwCurrentState = currentState;
    _status.dwWin32ExitCode = win32ExitCode;
    _status.dwWaitHint = waitHint;
    ::SetServiceStatus(_status_handle, &_status);
}


void WindowsLogService::start_service() {
}


void WindowsLogService::stop_service() {
}

