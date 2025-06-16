#ifndef _INVENTORY_GROUP_HARVESTER_HPP
#define _INVENTORY_GROUP_HARVESTER_HPP

#include "reflectiveJson.hpp"
#include "wcsClasses/agent.hpp"
#include "wcsClasses/wazuh.hpp"
#include "wcsClasses/group.hpp"
// <string> and <vector> removed

// Main harvester struct
struct InventoryGroupHarvester final {
    Agent agent;
    Group group;
    Wazuh wazuh;

    REFLECTABLE(MAKE_FIELD("agent", &InventoryGroupHarvester::agent),
                MAKE_FIELD("group", &InventoryGroupHarvester::group),
                MAKE_FIELD("wazuh", &InventoryGroupHarvester::wazuh));
};

#endif // _INVENTORY_GROUP_HARVESTER_HPP
