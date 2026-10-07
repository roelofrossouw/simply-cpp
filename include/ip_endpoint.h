#pragma once

#include <string>

namespace sc {
    struct ip_endpoint {
        std::string host;
        int port = 0;
    };
}
