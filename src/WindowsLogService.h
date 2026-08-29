#pragma once

#include <windows.h>
#include <string>


class WindowsLogService {
public:
    // Constructor takes the internal service name and the display name
    WindowsLogService(const std::wstring& service_name, const std::wstring& display_name);
    ~WindowsLogService();

    // Main entry point to parse command line and run
    int run(int argc, wchar_t* argv[]);

private:
    std::wstring _service_name;
    std::wstring _display_name;
    ::SERVICE_STATUS_HANDLE _status_handle;
    ::SERVICE_STATUS _status;

    // Static member function required for Windows Service Control Manager (SCM) callbacks
    static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD ctrlCode);

    // Singleton instance pointer for the static callbacks to access member data
    static WindowsLogService* s_instance;

    // Internal helper methods
    bool install();
    bool uninstall();
    void run_interactive();
    void start_service();
    void stop_service();
    void set_status(DWORD currentState, DWORD win32ExitCode = NO_ERROR, DWORD waitHint = 0);
};
