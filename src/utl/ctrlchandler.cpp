#include "ctrlchandler.h"

#include <Windows.h>

#include <stdexcept>
#include "utl/exception.h"
#include <utility>

static wlf::utl::ctrlc_cb interrupt_callback;


static BOOL WINAPI HandlerRoutine(
	_In_ DWORD dwCtrlType
)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
        interrupt_callback();
        return TRUE;

	default:
		break;
	}

	return FALSE;
}


namespace wlf::utl {

	CtrlcHandler::CtrlcHandler(ctrlc_cb cb)
	{
        interrupt_callback = std::move(cb);

        if (!::SetConsoleCtrlHandler(HandlerRoutine, TRUE))
			throw utl::os_error("failed to set ctrl+c handler", ::GetLastError());
    }


	CtrlcHandler::~CtrlcHandler()
	{
		::SetConsoleCtrlHandler(HandlerRoutine, FALSE);
	}

}
