#include "TCPSocket.h"

namespace SIK {
    TCPSocket::TCPSocket(uint16_t port) {
        listenerDescriptor = socket(AF_INET, SOCK_STREAM, 0);

        if (listenerDescriptor < 0) {
            throw SocketCreateException{};
        }

        sockaddr_in socketAddress{};

        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        socketAddress.sin_port = htons(port);

        if (bind(listenerDescriptor,
                 reinterpret_cast<struct sockaddr *>(&socketAddress),
                 sizeof(socketAddress)) < 0) {
            throw SocketBindException{};
        }

        if (listen(listenerDescriptor, MAX_LISTEN_QUEUE) < 0) {
            throw SocketListenException{};
        }
    }

    ssize_t TCPSocket::ClientConnection::readData(char *buffer, size_t count) const {
        ssize_t bytesRead;

        do {
            errno = 0;
            bytesRead = ::read(clientDescriptor, buffer, count);
        } while (bytesRead < 0 && errno == EINTR);

        if (bytesRead < 0) {
            throw ClientSocketReadException{};
        }

        return bytesRead;
    }

    void TCPSocket::ClientConnection::sendText(const char *buffer, size_t count) const {
        auto bytesLeft = count;

        const char *ptr = buffer;

        while (bytesLeft > 0) {
            errno = 0;
            auto bytesWritten = write(clientDescriptor, ptr, bytesLeft);

            if (bytesWritten <= 0) {
                if (bytesWritten < 0 && errno == EINTR) {
                    bytesWritten = 0;
                } else {
                    throw ClientSocketWriteException{};
                }
            }

            ptr += bytesWritten;
            bytesLeft -= bytesWritten;
        }
    }

    void TCPSocket::ClientConnection::sendFile(const std::filesystem::path &filePath) const {
        auto fileSize = std::filesystem::file_size(filePath);

        int fileDescriptor = open(filePath.c_str(), O_RDONLY);

        if (fileDescriptor < 0) {
            throw OpeningFileException{};
        }

        off64_t offset = 0;

        auto bytesLeft = fileSize;

        while (bytesLeft > 0) {
            errno = 0;
            auto bytesWritten = sendfile64(clientDescriptor, fileDescriptor,
                                           &offset, bytesLeft);

            if (bytesWritten <= 0) {
                if (bytesWritten < 0 && errno == EINTR) {
                    bytesWritten = 0;
                } else {
                    throw ClientSocketWriteException{};
                }
            }

            bytesLeft -= bytesWritten;
        }
    }
}
