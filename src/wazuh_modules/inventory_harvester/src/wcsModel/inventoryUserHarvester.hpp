#ifndef INVENTORY_USER_HARVESTER_HPP
#define INVENTORY_USER_HARVESTER_HPP

#include <string>
#include <vector>
#include "reflectiveJson.hpp"
#include "wcsClasses/agent.hpp"
#include "wcsClasses/wazuh.hpp"

namespace wazuh {
namespace inventory {
namespace harvester {

// Define the structure for User data
struct UserDataFields {
    REFLECTIVE_JSON_OBJECT;
    std::string uid;
    std::string gid;
    std::string name;
    std::string home;
    std::string shell;
    // std::vector<std::string> groups; // Kept simple for now

    static void declare_reflective_fields() {
        REFLECTIVE_JSON_ADD_FIELD(uid);
        REFLECTIVE_JSON_ADD_FIELD(gid);
        REFLECTIVE_JSON_ADD_FIELD(name);
        REFLECTIVE_JSON_ADD_FIELD(home);
        REFLECTIVE_JSON_ADD_FIELD(shell);
        // REFLECTIVE_JSON_ADD_FIELD(groups);
    }
};

struct InventoryUserHarvester final {
    Agent agent;
    UserDataFields user;
    Wazuh wazuh;

    REFLECTABLE(MAKE_FIELD("user", &InventoryUserHarvester::user),
                MAKE_FIELD("agent", &InventoryUserHarvester::agent),
                MAKE_FIELD("wazuh", &InventoryUserHarvester::wazuh));
};

} // namespace harvester
} // namespace inventory
} // namespace wazuh

#endif // INVENTORY_USER_HARVESTER_HPP
