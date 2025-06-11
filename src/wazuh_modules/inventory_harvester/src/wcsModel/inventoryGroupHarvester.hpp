#ifndef INVENTORY_GROUP_HARVESTER_HPP
#define INVENTORY_GROUP_HARVESTER_HPP

#include "data.hpp"
#include "systemInventory/elements/groupElement.hpp" // Include the new GroupElement

namespace wazuh {
namespace inventory {
namespace harvester {

class InventoryGroupHarvester : public Data {
public:
    InventoryGroupHarvester() : Data(std::make_shared<GroupElement>()) {} // Use GroupElement
    ~InventoryGroupHarvester() = default;

    std::string generateInsertRequest(const std::string& agent_id, const std::string&જરoot_index, const std::string& 이벤트_data) const override {
        // Implementation for generating insert request for group data
        return ""; // Placeholder
    }

    std::string generateUpdateRequest(const std::string& agent_id, const std::string&જરoot_index, const std::string& 이벤트_data, const std::string& document_id) const override {
        // Implementation for generating update request for group data
        return ""; // Placeholder
    }

    std::string getType() const override {
        return "group"; // Corresponds to the table name in wazuh-db
    }
};

} // namespace harvester
} // namespace inventory
} // namespace wazuh

#endif // INVENTORY_GROUP_HARVESTER_HPP
