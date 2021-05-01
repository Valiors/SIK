#ifndef SIKZAD1_TCPSOCKET_H
#define SIKZAD1_TCPSOCKET_H

#include <cstdint>
#include <memory>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <regex>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Auxiliary.h"

namespace SIK {
    class SocketCreateException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Creating TCP socket failed!";
        }
    };

    class SocketBindException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Binding TCP socket failed!";
        }
    };

    class SocketListenException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Turning TCP socket into listen mode failed!";
        }
    };

    class ClientSocketCreationException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Accepting connection with TCP socket failed!";
        }
    };

    class ClientSocketReadException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Read from client socket failed!";
        }
    };

    class ClientSocketWriteException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Writing to client socket failed!";
        }
    };

    class OpeningFileException : public ServerException {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Opening file has failed!";
        }
    };

    /* Class for managing TCP socket. */
    class TCPSocket {
    public:
        /* Starts listening for TCP connections on given port. */
        explicit TCPSocket(uint16_t port);

        /* Closes TCP socket. */
        ~TCPSocket() {
            close(listenerDescriptor);
        }

        /* Class for managing socket connection with client. */
        class ClientConnection {
        public:
            /* Accepts awaiting connection on listener socket. */
            explicit ClientConnection(int listenerDescriptor) {
                clientDescriptor = accept(listenerDescriptor, nullptr, nullptr);

                if (clientDescriptor < 0) {
                    throw ClientSocketCreationException{};
                }
            }

            /* Closes connection with a client. */
            ~ClientConnection() {
                close(clientDescriptor);
            }

            /* Reads UP TO count bytes from client to the given buffer.
             * Returns the amount of bytes read. */
            ssize_t readData(char *buffer, size_t count) const;

            /* Sends count bytes from the buffer to the client.
             * Retries until everything is sent. */
            void sendText(const char *buffer, size_t count) const;

            /* Calls sendText(buffer, count). */
            void sendText(const std::string &text) const {
                sendText(text.c_str(), text.size());
            }

            /* Sends file of size fileSize pointed by fileDescriptor to client.
             * Retries until everything is sent. */
            void sendFile(const std::filesystem::path &filePath) const;

        private:
            /* Descriptor of the client socket. */
            int clientDescriptor;
        };

        /* Creates new client connection adn returns std::unique_ptr to it. */
        [[nodiscard]] auto acceptConnection() const {
            return std::make_unique<ClientConnection>(listenerDescriptor);
        }

    private:
        /* Maximum size of the queue of clients awaiting for connection. */
        static constexpr int MAX_LISTEN_QUEUE = 1024;

        /* Descriptor of the listening socket. */
        int listenerDescriptor;
    };
}

#endif //SIKZAD1_TCPSOCKET_H
