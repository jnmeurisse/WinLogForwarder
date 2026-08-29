#include "service.h"


// eventcreate /ID 100 /L application /T INFORMATION /SO "CustomApp" /D "This is a custom event logged via Command Prompt."

int wmain(int argc, wchar_t* argv[]) 
{
    return wlf::Service::instance().run(argc, argv);
}