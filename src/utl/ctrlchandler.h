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
