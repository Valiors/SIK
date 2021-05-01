#include <iostream>
#include <csignal>

#include "HTTPServer.h"

namespace {
    bool isNonNegativeNumber(const char *str) {
        std::size_t len = strlen(str);

        return len > 0 && std::all_of(str, str + len, [](unsigned char ch) {
            return isdigit(ch);
        });
    }
}

int main(int argc, char *argv[]) {
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, [](int) { exit((0)); });

    uint16_t port = SIK::DEFAULT_HTTP_PORT;

    if (argc != 3 && argc != 4) {
        std::cout << "Wrong argument count!\n"
                  << "Run program by: ./serwer <files directory> <correlated servers> [<port number>]"
                  << std::endl;

        return EXIT_FAILURE;
    }

    if (argc == 4) {
        if (isNonNegativeNumber(argv[3])) {
            errno = 0;
            auto number = strtol(argv[3], nullptr, 10);

            if (errno == ERANGE || number < 0 || number > std::numeric_limits<uint16_t>::max()) {
                std::cout << "Wrong port number" << std::endl;
                return EXIT_FAILURE;
            }

            port = static_cast<uint16_t>(number);
        } else {
            std::cout << "Wrong port number!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    try {
        SIK::HTTPServer server{argv[1], argv[2], port};
        server.start();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        std::cout << "Server failure!" << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cout << "Unknown server error!" << std::endl;
        return EXIT_FAILURE;
    }
}
