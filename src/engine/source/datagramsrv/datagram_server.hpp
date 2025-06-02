#pragma once

#include "httpsrv/datagram_socket.hpp"
#include "parser/legacy_protocol_parser.hpp"
#include "router/router.hpp" // Assuming Orchestrator is part of router or accessible via it
#include "base/utils/wazuhProtocol/wazuhRequest.hpp"

#include <string>
#include <thread>
#include <atomic>

namespace engine {
namespace datagramsrv {

class DatagramServer {
public:
    DatagramServer(
        std::shared_ptr<httpsrv::DatagramSocket> socket,
        std::shared_ptr<engine::parser::LegacyProtocolParser> parser,
        std::shared_ptr<engine::router::Orchestrator> orchestrator // Placeholder for actual Orchestrator type/namespace
    );
    ~DatagramServer();

    void start();
    void stop();

private:
    void run(); // Main loop for receiving and processing messages

    std::shared_ptr<httpsrv::DatagramSocket> socket_;
    std::shared_ptr<engine::parser::LegacyProtocolParser> parser_;
    std::shared_ptr<engine::router::Orchestrator> orchestrator_;

    size_t maxQueueSize_;

    std::thread serverThread_;
    std::atomic<bool> running_{false};
};

} // namespace datagramsrv
} // namespace engine
