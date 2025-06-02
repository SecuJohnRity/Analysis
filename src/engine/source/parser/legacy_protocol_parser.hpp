#pragma once

#include <string>
#include <optional>

namespace engine {
namespace parser {

struct ParsedLegacyMessage {
    std::string queue;
    std::string full_location;
    std::string message;
};

class LegacyProtocolParser {
public:
    LegacyProtocolParser(); // Constructor, if needed for initialization

    // Parses a raw message string according to the legacy 4.x protocol.
    // Returns an optional containing the parsed message if successful,
    // or std::nullopt if the message is malformed.
    std::optional<ParsedLegacyMessage> parse(const std::string& raw_message);

private:
    // Helper function to unescape '|' characters to ':'
    std::string unescapeLocation(const std::string& escaped_location);
};

} // namespace parser
} // namespace engine
