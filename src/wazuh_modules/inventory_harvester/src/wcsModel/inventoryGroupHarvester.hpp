#ifndef INVENTORY_GROUP_HARVESTER_HPP
#define INVENTORY_GROUP_HARVESTER_HPP

#include "wcsModel/wcsClasses/wcsBase.hpp" // Assuming a base class
#include "utils/jsonIO.hpp"
#include "utils/osPrimitives.hpp"

// Other necessary includes
// Needs Wazuh::Inventory::Harvester::SystemContext for AffectedComponentType
#include "systemInventory/systemContext.hpp"

namespace Wazuh {
namespace Inventory {
namespace Harvester {

class InventoryGroupHarvester : public WcsBase { // Or appropriate base class
public:
    InventoryGroupHarvester() : WcsBase(Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::Group) {} // Set appropriate type

    std::vector<cJSON*> collectData(int agent_id, const std::string& command, const std::string& options) override {
        std::vector<cJSON*> collected_data;

        cJSON* group_payload = cJSON_CreateObject(); // Root object for GroupElement.fromJson

        // Group object
        cJSON* group_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(group_obj, "description", "Dummy Group Description");
        cJSON_AddStringToObject(group_obj, "id", "2001"); // Group ID
        cJSON_AddNumberToObject(group_obj, "id_signed", 2001);
        cJSON_AddBoolToObject(group_obj, "is_hidden", false);
        cJSON_AddStringToObject(group_obj, "name", "dummygroup");

        cJSON* users_array = cJSON_CreateArray();
        cJSON_AddItemToArray(users_array, cJSON_CreateString("dummy"));
        cJSON_AddItemToArray(users_array, cJSON_CreateString("anotheruser"));
        cJSON_AddItemToObject(group_obj, "users", users_array);

        cJSON_AddStringToObject(group_obj, "uuid", "dummy-uuid-group-456");
        cJSON_AddItemToObject(group_payload, "group", group_obj);

        // Host object
        cJSON* host_obj = cJSON_CreateObject();
        cJSON* ips_array = cJSON_CreateArray();
        cJSON_AddItemToArray(ips_array, cJSON_CreateString("127.0.0.1"));
        cJSON_AddItemToObject(host_obj, "ip", ips_array);
        std::string hostname = Os::Host::getHostname("dummy_host"); // Assuming Os::Host::getHostname exists
        cJSON_AddStringToObject(host_obj, "hostname", hostname.c_str());
        cJSON_AddItemToObject(group_payload, "host", host_obj);

        collected_data.push_back(group_payload);
        return collected_data;
    }

    // std::string getCommand() const override { return "get_groups"; } // Example
    // AffectedComponentType getType() const override { return AffectedComponentType::Group; } // Example
};

} // namespace Harvester
} // namespace Inventory
} // namespace Wazuh

#endif // INVENTORY_GROUP_HARVESTER_HPP
