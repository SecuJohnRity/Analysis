#include "datagram_server.hpp"
#include "base/baseTypes.hpp"
#include "base/logging.hpp" // For logging errors/info
#include <iostream> // For temporary error logging

namespace engine {
namespace datagramsrv {

DatagramServer::DatagramServer(
    std::shared_ptr<httpsrv::DatagramSocket> socket,
    std::shared_ptr<engine::parser::LegacyProtocolParser> parser,
    std::shared_ptr<engine::router::Orchestrator> orchestrator,
    size_t maxQueueSize)
    : socket_(std::move(socket)),
      parser_(std::move(parser)),
      orchestrator_(std::move(orchestrator)),
      maxQueueSize_(maxQueueSize) {
    if (!socket_) {
        throw std::invalid_argument("DatagramSocket cannot be null.");
    }
    if (!parser_) {
        throw std::invalid_argument("LegacyProtocolParser cannot be null.");
    }
    if (!orchestrator_) {
        throw std::invalid_argument("Orchestrator cannot be null.");
    }
}

DatagramServer::~DatagramServer() {
    stop();
}

void DatagramServer::start() {
    if (running_) {
        return; // Already running
    }
    running_ = true;
    serverThread_ = std::thread(&DatagramServer::run, this);
    // TODO: Add logging - SPDLOG(info, "DatagramServer started.");
    std::cout << "DatagramServer started." << std::endl;
}

void DatagramServer::stop() {
    running_ = false;
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    // TODO: Add logging - SPDLOG(info, "DatagramServer stopped.");
    std::cout << "DatagramServer stopped." << std::endl;
}

void DatagramServer::run() {
    char buffer[65536]; // Max UDP packet size, adjust as needed

    while (running_) {
        ssize_t bytesReceived = socket_->receive(buffer, sizeof(buffer) - 1);

        if (!running_) break; // Check again in case stop() was called during receive

        if (bytesReceived < 0) {
            // Error or timeout
            // TODO: Add proper error handling and logging - SPDLOG(error, "Error receiving data: {}", strerror(errno));
            std::cerr << "Error receiving data." << std::endl;
            // Consider adding a small delay here to prevent busy-looping on persistent errors
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (bytesReceived == 0) {
            // Typically means connection closed for stream sockets, but for datagram it might mean an empty packet.
            // Depending on protocol, this might be an error or ignorable.
            continue;
        }

        buffer[bytesReceived] = '\0'; // Null-terminate the received data
        std::string rawMessage(buffer);

        // TODO: Add logging - SPDLOG(debug, "Received raw message: {}", rawMessage);
        // std::cout << "Received raw message: " << rawMessage << std::endl;

        auto parsedOptional = parser_->parse(rawMessage);

        if (!parsedOptional) {
            // TODO: Add logging - SPDLOG(warn, "Failed to parse message: {}", rawMessage);
            std::cerr << "Failed to parse message: " << rawMessage << std::endl;
            continue;
        }

        // Check for queue overflow before processing further
        // Assuming orchestrator_->getEventQueueSize() is a method that will be added to Orchestrator
        // to get the current size of its m_eventQueue.
        // If getEventQueueSize() is not available, this part of the logic is a placeholder.
        // -- Placeholder removed, getEventQueueSize() is now available. --

        // Check for queue overflow before processing further
        if (orchestrator_->getEventQueueSize() >= maxQueueSize_) {
            // TODO: Replace std::cerr with SPDLOG(warn, ...)
            std::cerr << "DatagramServer: Orchestrator event queue full (size: " << orchestrator_->getEventQueueSize()
                      << ", max: " << maxQueueSize_ << "). Discarding message: " << rawMessage << std::endl;
            continue;
        }

        const auto& parsedMsg = *parsedOptional;

        // Convert ParsedLegacyMessage to base::Event (std::shared_ptr<json::Json>)
        auto eventJson = std::make_shared<json::Json>();
        try {
            (*eventJson)["command"] = parsedMsg.queue;

            json::Json params;
            params["full_location"] = parsedMsg.full_location;
            params["message_payload"] = parsedMsg.message;
            params["origin_module"] = "datagram_server_legacy";

            (*eventJson)["parameters"] = params;

            // TODO: Replace std::cout with SPDLOG(debug, ...)
            // std::cout << "Posting event: " << eventJson->str() << std::endl;

            // Note: There's a potential race condition here. The queue size might change
            // between the check above and the postEvent call.
            // For true atomicity, postEvent would ideally return a status if the queue was full
            // upon attempting the push, or the queue itself would handle this.
            // However, for this subtask, the current check is as requested.
            orchestrator_->postEvent(std::move(eventJson));

        } catch (const std::exception& e) {
            // TODO: Replace std::cerr with SPDLOG(error, ...)
             std::cerr << "DatagramServer: Error creating JSON event or posting to orchestrator: " << e.what()
                       << ". Message: " << rawMessage << std::endl;
        }
    }
}

} // namespace datagramsrv
} // namespace engine
