#include "datagram_socket.hpp"

#include <stdexcept>
#include <system_error>
#include <sys/stat.h>
#include <cerrno>

namespace httpsrv {

DatagramSocket::DatagramSocket(SocketType type, const std::string& path, mode_t perm, uid_t owner, gid_t group)
    : socketPath_(path) {
    initialized_ = initSocket(type, path, perm, owner, group);
    if (!initialized_) {
        throw std::runtime_error("Failed to initialize socket");
    }
}

DatagramSocket::~DatagramSocket() {
    if (sockfd_ != -1) {
        closeSocket();
    }
    // It's good practice to also remove the socket file if it was created.
    // However, this might not always be desired, e.g. if the socket is meant to persist.
    // For this example, let's remove it.
    if (initialized_ && !socketPath_.empty()) {
        unlink(socketPath_.c_str());
    }
}

bool DatagramSocket::initSocket(SocketType type, const std::string& path, mode_t perm, uid_t owner, gid_t group) {
    sockfd_ = socket(AF_UNIX, static_cast<int>(type), 0);
    if (sockfd_ < 0) {
        // Consider logging the error: strerror(errno)
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    // Remove existing socket file if it exists
    unlink(path.c_str());

    if (bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        // Consider logging the error: strerror(errno)
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    if (chmod(path.c_str(), perm) < 0) {
        // Consider logging the error: strerror(errno)
        close(sockfd_);
        sockfd_ = -1;
        unlink(path.c_str()); // Clean up
        return false;
    }

    if (chown(path.c_str(), owner, group) < 0) {
        // Consider logging the error: strerror(errno)
        close(sockfd_);
        sockfd_ = -1;
        unlink(path.c_str()); // Clean up
        return false;
    }

    return true;
}

bool DatagramSocket::setReceiveBufferSize(int size) {
    if (sockfd_ == -1) return false;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
        // Consider logging the error: strerror(errno)
        return false;
    }
    return true;
}

ssize_t DatagramSocket::receive(char* buffer, size_t bufferSize, int flags) {
    if (sockfd_ == -1) return -1;
    ssize_t bytesReceived = recv(sockfd_, buffer, bufferSize, flags);
    if (bytesReceived < 0) {
        // Consider logging the error: strerror(errno)
    }
    return bytesReceived;
}

void DatagramSocket::closeSocket() {
    if (sockfd_ != -1) {
        close(sockfd_);
        sockfd_ = -1;
        // Optionally, also unlink the socket path here if it's always desired upon explicit close
        // unlink(socketPath_.c_str());
        // However, the destructor already handles unlinking if the path is set.
        // Duplicating unlink might be problematic if closeSocket is called before destruction
        // and the path is needed for re-initialization or other purposes.
        // For now, let destructor handle the unlink.
    }
}

} // namespace httpsrv
