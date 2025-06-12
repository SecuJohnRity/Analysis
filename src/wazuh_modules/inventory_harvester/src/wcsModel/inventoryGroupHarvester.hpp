#ifndef _INVENTORY_GROUP_HARVESTER_HPP
#define _INVENTORY_GROUP_HARVESTER_HPP

#include "reflectiveJson.hpp"
#include "wcsClasses/agent.hpp" // Assuming Agent and Wazuh structs are common
#include "wcsClasses/wazuh.hpp"
#include <string>
#include <vector> // For users if it's a list

struct Group {
    std::string description;
    unsigned long id = 0; // Corresponds to group.id
    long id_signed = 0;   // Corresponds to group.id_signed
    bool is_hidden = false;
    std::string name;
    std::vector<std::string> users; // Corresponds to group.users
    std::string uuid;

    REFLECTABLE(MAKE_FIELD("description", &Group::description),
                MAKE_FIELD("id", &Group::id),
                MAKE_FIELD("id_signed", &Group::id_signed),
                MAKE_FIELD("is_hidden", &Group::is_hidden),
                MAKE_FIELD("name", &Group::name),
                MAKE_FIELD("users", &Group::users),
                MAKE_FIELD("uuid", &Group::uuid));
};

// Main harvester struct
struct InventoryGroupHarvester final {
    Agent agent; // Assuming Agent and Wazuh structs are common and available
    Group group;
    Wazuh wazuh;

    REFLECTABLE(MAKE_FIELD("agent", &InventoryGroupHarvester::agent),
                MAKE_FIELD("group", &InventoryGroupHarvester::group),
                MAKE_FIELD("wazuh", &InventoryGroupHarvester::wazuh));
};

#endif // _INVENTORY_GROUP_HARVESTER_HPP
