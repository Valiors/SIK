#include "CorrelatedServers.h"

namespace SIK {
    CorrelatedServers::CorrelatedServers(const std::string &filename) {
        std::ifstream file(filename);

        if (!file.is_open()) {
            throw CorrelatedServersReadException{};
        }

        std::string resource;
        std::string server;
        std::string port;

        while (file >> resource >> server >> port) {
            std::ostringstream stream;

            stream << "http://" << server << ':' << port << resource;

            resourceToHTTP.emplace(std::move(resource), stream.str());
        }

        if (file.bad()) {
            throw CorrelatedServersReadException{};
        }
    }

    std::optional<std::string> CorrelatedServers::getResourceHTTPAddress(const std::string &resource) {
        auto it = resourceToHTTP.find(resource);

        if (it != resourceToHTTP.end()) {
            return it->second;
        } else {
            return {};
        }
    }
}