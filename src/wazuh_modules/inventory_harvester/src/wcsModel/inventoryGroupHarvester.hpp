#ifndef INVENTORY_GROUP_HARVESTER_HPP
#define INVENTORY_GROUP_HARVESTER_HPP

#include <string>
#include <vector>
#include "reflectiveJson.hpp"
#include "wcsClasses/agent.hpp"
#include "wcsClasses/wazuh.hpp"

namespace wazuh {
namespace inventory {
namespace harvester {

// Define the structure for Group data
struct GroupDataFields {
    REFLECTIVE_JSON_OBJECT;
    std::string gid;
    std::string name;
    // std::vector<std::string> members; // Kept simple for now

    static void declare_reflective_fields() {
        REFLECTIVE_JSON_ADD_FIELD(gid);
        REFLECTIVE_JSON_ADD_FIELD(name);
        // REFLECTIVE_JSON_ADD_FIELD(members);
    }
};

struct InventoryGroupHarvester final {
    Agent agent;
    GroupDataFields group;
    Wazuh wazuh;

    REFLECTABLE(MAKE_FIELD("group", &InventoryGroupHarvester::group),
                MAKE_FIELD("agent", &InventoryGroupHarvester::agent),
                MAKE_FIELD("wazuh", &InventoryGroupHarvester::wazuh));
};

} // namespace harvester
} // namespace inventory
} // namespace wazuh

#endif // INVENTORY_GROUP_HARVESTER_HPP
