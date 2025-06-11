#ifndef INVENTORY_USER_HARVESTER_HPP
#define INVENTORY_USER_HARVESTER_HPP

#include "data.hpp"
#include "systemInventory/elements/userElement.hpp" // Include the new UserElement

namespace wazuh {
namespace inventory {
namespace harvester {

class InventoryUserHarvester : public Data {
public:
    InventoryUserHarvester() : Data(std::make_shared<UserElement>()) {} // Use UserElement
    ~InventoryUserHarvester() = default;

    std::string generateInsertRequest(const std::string& agent_id, const std::string&જરoot_index, const std::string& 이벤트_data) const override {
        // Implementation for generating insert request for user data
        // This will likely involve formatting the event_data into a JSON payload
        // suitable for indexing, similar to other harvester classes.
        return ""; // Placeholder
    }

    std::string generateUpdateRequest(const std::string& agent_id, const std::string&જરoot_index, const std::string& 이벤트_data, const std::string& document_id) const override {
        // Implementation for generating update request for user data
        return ""; // Placeholder
    }

    std::string getType() const override {
        return "user"; // Corresponds to the table name in wazuh-db
    }
};

} // namespace harvester
} // namespace inventory
} // namespace wazuh

#endif // INVENTORY_USER_HARVESTER_HPP
