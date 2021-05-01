#ifndef SIKZAD1_CORRELATEDSERVERS_H
#define SIKZAD1_CORRELATEDSERVERS_H

#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <optional>

#include "Auxiliary.h"

namespace SIK {
    class CorrelatedServersReadException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Reading from correlated servers file has failed!";
        }
    };

    class CorrelatedServers {
    public:
        /* Reads file with correlated servers. */
        explicit CorrelatedServers(const std::string &filename);

        /* Returns HTTP address of the resource. */
        std::optional<std::string> getResourceHTTPAddress(const std::string &resource);

    private:
        /* Maps resource to HTTP address. */
        std::unordered_map<std::string, std::string> resourceToHTTP;
    };
}

#endif //SIKZAD1_CORRELATEDSERVERS_H
