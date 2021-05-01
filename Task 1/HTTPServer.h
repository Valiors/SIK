#ifndef SIKZAD1_HTTPSERVER_H
#define SIKZAD1_HTTPSERVER_H

#include <string>
#include <filesystem>
#include <iostream>
#include <utility>

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "Auxiliary.h"
#include "CorrelatedServers.h"
#include "TCPSocket.h"

namespace SIK {
    constexpr uint16_t DEFAULT_HTTP_PORT = 8080;

    class RootPathIsNotDirectoryException : ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Root passed as an argument is not a directory!";
        }
    };

    class ReadFromRootDirectoryException : ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Reading from root directory has failed!";
        }
    };

    /* Class managing HTTP 1.1 server. */
    class HTTPServer {
    public:
        /* Creates new HTTP server, loads correlated servers
         * and starts listening for client connections. */
        HTTPServer(const std::string &filesFolderName,
                   const std::string &correlatedServersFileName,
                   uint16_t portNumber);

        /* Copy and move semantics are disabled due to the nature of connection. */
        HTTPServer(const HTTPServer &) = delete;

        HTTPServer &operator=(const HTTPServer &) = delete;

        /* Starts up server. */
        void start();

    private:
        enum class RequestState {
            OK,
            WRONG_FORMAT,
            NOT_IMPLEMENTED
        };

        enum class RequestKind {
            NA,
            GET,
            HEAD
        };

        struct Request {
            RequestState state;
            RequestKind kind;
            std::string file;
            bool keepAlive;
        };

        inline static const Request WrongRequest = {RequestState::WRONG_FORMAT,
                                                    RequestKind::NA, "", false};

        inline static const Request NotImplementedRequest = {RequestState::NOT_IMPLEMENTED,
                                                             RequestKind::NA, "", true};

        /* Fetches request from the client. */
        Request getRequest();

        /* Performs client's request. Returns true if the connection is to be kept alive.
         * Returns false otherwise. */
        bool performRequest(const Request &request);

        /* Returns absolute path to the resource. */
        std::optional<std::filesystem::path>
        relativeResourcePathToAbsolute(const std::filesystem::path &relativeFilePath);

        /* Sends 200 OK to the client. */
        void sendOK(const std::filesystem::path &filePath);

        /* Sends 302 Found to the client. */
        void sendFound(const std::string &httpAddress);

        /* Sends 400 Bad Request to the client. */
        void sendBadRequest();

        /* Sends 404 Not Found to the client. */
        void sendNotFound();

        /* Sends 500 Internal Server Error to the client. */
        void sendInternalServerError();

        /* Sends 501 Not Implemented to the client. */
        void sendNotImplemented();

        friend class CRLFLineReader;

        /* Class managing buffer for reading CRLF-ended (carriage return, line feed) lines from the client. */
        class CRLFLineReader {
        public:
            explicit CRLFLineReader(HTTPServer &serverRef) : buffer{}, begin{}, end{}, serverRef(serverRef) {}

            /* Fetches CRLF-ended from the client. */
            std::optional<std::string> readLine();

        private:
            /* Size of the buffer. */
            static constexpr size_t BUFFER_SIZE = 16384;

            /* Buffer for storing read CRLF line. */
            char buffer[BUFFER_SIZE];

            /* Begin and end of currently read CRLF line. */
            const char *begin;
            const char *end;

            /* Reference to containing HTTP server. */
            HTTPServer &serverRef;
        };

        static constexpr const char *httpVersionOfServer = "HTTP/1.1";
        static constexpr const char *serverName = "NaimadServer";

        /* Object containing HTTP addresses of relocated resources. */
        CorrelatedServers correlatedServers;

        /* TCP socket through which HTTP server communicates. */
        TCPSocket socket;

        /* Directory from which server fetches files to send to the client. */
        std::filesystem::path rootDirectory;

        /* Pointer to the client being currently served. */
        TCPSocket::ClientConnection *client;

        /* Object for reading CRLF-ended lines from client. */
        CRLFLineReader lineReader;
    };
}

#endif //SIKZAD1_HTTPSERVER_H
