#include "HTTPServer.h"

namespace {
    /* Trims spaces from left side of the string. */
    inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    /* Trims spaces from right side of the string. */
    inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    /* Trims spaces from both sides of the string. */
    inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }
}

namespace SIK {
    namespace fs = std::filesystem;

    HTTPServer::HTTPServer(const std::string &filesFolderName, const std::string &correlatedServersFileName,
                           uint16_t portNumber) : correlatedServers{correlatedServersFileName}, socket{portNumber},
                                                  rootDirectory{filesFolderName}, client{}, lineReader{*this} {

        rootDirectory = fs::canonical(rootDirectory);

        if (!fs::is_directory(rootDirectory)) {
            throw RootPathIsNotDirectoryException{};
        }

        /* Checking read permission for root directory. */
        int rootDescriptor = open(rootDirectory.c_str(), O_RDONLY);

        if (rootDescriptor < 0) {
            throw ReadFromRootDirectoryException{};
        } else {
            close(rootDescriptor);
        }
    }

    void HTTPServer::start() {
        std::cout << "Server has started running and is accepting client connections." << std::endl;

        while (true) {
            std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            std::cout << "Awaiting client connection." << std::endl;

            std::unique_ptr<TCPSocket::ClientConnection> uniqueClient;

            try {
                uniqueClient = socket.acceptConnection();
            } catch (const ClientSocketCreationException &e) {
                std::cout << e.what() << std::endl;
                continue;
            }

            client = uniqueClient.get();

            std::cout << "Client connection established." << std::endl;

            try {
                bool keepAlive = true;

                while (keepAlive) {
                    std::cout << "---------------------------------------------------------------" << std::endl;
                    std::cout << "Getting request from client." << std::endl;

                    Request request = getRequest();

                    keepAlive = performRequest(request);

                    std::cout << "Finished performing request." << std::endl;
                }
            } catch (const std::exception &e) {
                std::cout << e.what() << std::endl;
            }

            std::cout << "Connection with client ended." << std::endl;
        }
    }

    HTTPServer::Request HTTPServer::getRequest() {
        std::optional<std::string> requestLine = lineReader.readLine();

        if (!requestLine) {
            return WrongRequest;
        }

        std::regex requestLineRegex{R"(([^ ]+) (\/.*) ([^ ]+)\r\n)"};

        std::smatch matches;
        if (!std::regex_match(requestLine.value(), matches, requestLineRegex)) {
            return WrongRequest;
        }

        std::string method = matches.str(1);
        std::string requestTarget = matches.str(2);
        std::string httpVersionOfRequest = matches.str(3);

        std::string connectionFieldValue;
        std::string contentLengthFieldValue;

        while (true) {
            std::optional<std::string> headerField = lineReader.readLine();

            if (!headerField) {
                return WrongRequest;
            }

            std::string headerFieldString = headerField.value();

            if (headerFieldString == "\r\n") {
                break;
            }

            auto colonPosition = headerFieldString.find(':');

            if (colonPosition == std::string::npos) {
                return WrongRequest;
            }

            std::string fieldName = headerFieldString.substr(0, colonPosition);
            std::string fieldValue = headerFieldString.substr(colonPosition + 1);

            trim(fieldValue);

            transform(fieldName.begin(), fieldName.end(), fieldName.begin(), [](char c) { return tolower(c); });

            if (fieldName == "connection") {
                if (!connectionFieldValue.empty()) {
                    return WrongRequest;
                }

                connectionFieldValue = std::move(fieldValue);
            } else if (fieldName == "content-length") {
                if (!contentLengthFieldValue.empty()) {
                    return WrongRequest;
                }

                contentLengthFieldValue = std::move(fieldValue);
            }
        }

        if (httpVersionOfRequest != "HTTP/1.1") {
            return WrongRequest;
        }

        if (method != "GET" && method != "HEAD") {
            return NotImplementedRequest;
        }

        if (!contentLengthFieldValue.empty() && contentLengthFieldValue != "0") {
            return WrongRequest;
        }

        if (!connectionFieldValue.empty() && connectionFieldValue != "close" &&
            connectionFieldValue != "keep-alive") {
            return NotImplementedRequest;
        }

        return {RequestState::OK, method == "GET" ? RequestKind::GET : RequestKind::HEAD,
                std::move(requestTarget), connectionFieldValue != "close"};
    }

    bool HTTPServer::performRequest(const Request &request) {
        if (request.state == RequestState::OK) {
            try { // Trying to send file.
                auto filePath = relativeResourcePathToAbsolute(request.file);

                sendOK(filePath.value());
                if (request.kind == RequestKind::GET) {
                    client->sendFile(filePath.value());
                }
            } catch (const std::exception &e) {
                auto httpAddress = correlatedServers.getResourceHTTPAddress(request.file);

                if (httpAddress) {
                    sendFound(httpAddress.value());
                } else {
                    sendNotFound();
                }
            }
        } else if (request.state == RequestState::WRONG_FORMAT) {
            sendBadRequest();
        } else if (request.state == RequestState::NOT_IMPLEMENTED) {
            sendNotImplemented();
        }

        return request.keepAlive;
    }

    std::optional<fs::path> HTTPServer::relativeResourcePathToAbsolute(const fs::path &relativeFilePath) {
        fs::path filePath;

        try {
            filePath = rootDirectory;
            filePath += relativeFilePath;
            filePath = fs::canonical(filePath);
        } catch (const std::exception &e) {
            return std::nullopt;
        }

        /* Iterator rootEnd points to the first mismatch between rootDirectory and filePath.
         * If rootEnd != rootDirectory.end() then it means that client has tried to reach
         * above rootDirectory. */
        auto rootEnd = std::mismatch(rootDirectory.begin(), rootDirectory.end(), filePath.begin()).first;

        if (rootEnd != rootDirectory.end()) {
            std::cout << "Trying to reach above root server directory!" << std::endl;
            return std::nullopt;
        }

        return filePath;
    }

    std::optional<std::string> HTTPServer::CRLFLineReader::readLine() {
        std::string line;

        while (line.size() < 2 || line.back() != '\n' || line[line.size() - 2] != '\r') {
            if (begin == end) {
                ssize_t bytesRead = serverRef.client->readData(buffer, BUFFER_SIZE);

                begin = buffer;
                end = buffer + bytesRead;
            }

            line.push_back(*begin++);

            if (line.length() > BUFFER_SIZE) {
                return std::nullopt;
            }
        }

        return line;
    }

    void HTTPServer::sendOK(const fs::path &filePath) {
        std::ostringstream stream;

        stream << httpVersionOfServer << " 200 OK\r\n"
               << "Content-Type: application/octet-stream\r\n"
               << "Content-Length: " << fs::file_size(filePath) << "\r\n"
               << "Server: " << serverName << "\r\n"
               << "\r\n";

        client->sendText(stream.str());

        std::cout << "200 OK sent." << std::endl;
    }

    void HTTPServer::sendFound(const std::string &httpAddress) {
        std::ostringstream stream;

        stream << httpVersionOfServer << " 302 Found\r\n"
               << "Location: " << httpAddress << "\r\n"
               << "Server: " << serverName << "\r\n"
               << "\r\n";

        client->sendText(stream.str());

        std::cout << "302 Found sent." << std::endl;
    }

    void HTTPServer::sendBadRequest() {
        std::ostringstream stream;

        stream << httpVersionOfServer << " 400 Bad Request\r\n"
               << "Connection: close\r\n"
               << "Server: " << serverName << "\r\n"
               << "\r\n";

        client->sendText(stream.str());

        std::cout << "400 Bad Request sent." << std::endl;
    }

    void HTTPServer::sendNotFound() {
        std::ostringstream stream;

        stream << httpVersionOfServer << " 404 Not Found\r\n"
               << "Server: " << serverName << "\r\n"
               << "\r\n";

        client->sendText(stream.str());

        std::cout << "404 Not Found sent." << std::endl;
    }

    void HTTPServer::sendInternalServerError() {
        std::ostringstream stream;

        stream << httpVersionOfServer << " 500 Bad Internal Server Error\r\n"
               << "Connection: close\r\n"
               << "Server: " << serverName << "\r\n"
               << "\r\n";

        client->sendText(stream.str());

        std::cout << "500 Internal Server Error sent." << std::endl;
    }

    void HTTPServer::sendNotImplemented() {
        std::ostringstream stream;

        stream << httpVersionOfServer << " 501 Not Implemented\r\n"
               << "Server: " << serverName << "\r\n"
               << "\r\n";

        client->sendText(stream.str());

        std::cout << "501 Not Implemented sent." << std::endl;
    }
}