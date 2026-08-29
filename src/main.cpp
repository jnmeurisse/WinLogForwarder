/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#include "service.h"


// eventcreate /ID 100 /L application /T INFORMATION /SO "CustomApp" /D "This is a custom event logged via Command Prompt."

int wmain(int argc, wchar_t* argv[]) 
{
    return wlf::Service::instance().run(argc, argv);
}