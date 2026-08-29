#include "WlfConfig.h"
#include "utl/cpptoml.h"

namespace wlf {

    void load_config(const std::string& filename, WlfConfig& wlf_config)
    {
        auto config_data = cpptoml::parse_file(filename);
        auto config = config_data.get();

        auto server_config = config->get_table("forwarder");
        if (server_config) {
            if (server_config->contains("host"))
                wlf_config.forwarder.server = server_config->get("server")->as();
        }
    }

}

