/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once
#include <functional>


namespace wlf::utl {

    using ctrlc_cb = std::function<void()>;


	/* 
     * @class CtrlcHandler
     * 
     * Implements a Ctrl+C handler.
	*/
	class CtrlcHandler {
	public:
        CtrlcHandler(ctrlc_cb cb);
        ~CtrlcHandler();
    };

}