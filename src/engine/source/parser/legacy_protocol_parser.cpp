#include "legacy_protocol_parser.hpp"
#include <algorithm> // For std::replace

namespace engine {
namespace parser {

LegacyProtocolParser::LegacyProtocolParser() {
    // Constructor implementation, if any specific initialization is needed.
}

std::string LegacyProtocolParser::unescapeLocation(const std::string& escaped_location) {
    std::string unescaped = escaped_location;
    std::replace(unescaped.begin(), unescaped.end(), '|', ':');
    return unescaped;
}

std::optional<ParsedLegacyMessage> LegacyProtocolParser::parse(const std::string& raw_message) {
    ParsedLegacyMessage result;

    // Split by the first colon to separate queue
    size_t first_colon_pos = raw_message.find(':');
    if (first_colon_pos == std::string::npos) {
        // Malformed: No colon found
        return std::nullopt;
    }

    result.queue = raw_message.substr(0, first_colon_pos);
    std::string remaining_message = raw_message.substr(first_colon_pos + 1);

    // Split the remaining string by the last colon to separate full location from message
    size_t last_colon_pos = remaining_message.rfind(':');
    if (last_colon_pos == std::string::npos) {
        // Malformed: No second colon found for location/message split
        return std::nullopt;
    }

    // Check if the last colon is the very first character of the remaining_message
    // This would mean an empty location string, which might be valid or invalid based on requirements.
    // For now, let's assume it's valid.
    // Example: "queue::message_body" -> queue="", full_location="", message="message_body" (if first char is colon)
    // Example: "queue:location:message_body" -> queue="queue", full_location="location", message="message_body"

    result.full_location = remaining_message.substr(0, last_colon_pos);
    result.message = remaining_message.substr(last_colon_pos + 1);

    // Unescape full_location
    result.full_location = unescapeLocation(result.full_location);

    // Basic validation: ensure no key parts are unexpectedly empty if that's a requirement.
    // For this example, we'll allow empty parts as long as the structure is correct.
    // if (result.queue.empty() || result.message.empty()) {
    //     // Further validation if empty queue or message is not allowed.
    //     return std::nullopt;
    // }

    return result;
}

} // namespace parser
} // namespace engine
