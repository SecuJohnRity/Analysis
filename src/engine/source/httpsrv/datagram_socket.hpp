#pragma once

#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace httpsrv {

enum class SocketType {
    STREAM = SOCK_STREAM,
    DGRAM = SOCK_DGRAM
};

class DatagramSocket {
public:
    DatagramSocket(SocketType type, const std::string& path, mode_t perm, uid_t owner, gid_t group);
    ~DatagramSocket();

    bool setReceiveBufferSize(int size);
    ssize_t receive(char* buffer, size_t bufferSize, int flags = 0);
    void closeSocket();

private:
    int sockfd_ = -1;
    std::string socketPath_;
    bool initialized_ = false;

    bool initSocket(SocketType type, const std::string& path, mode_t perm, uid_t owner, gid_t group);
};

} // namespace httpsrv
