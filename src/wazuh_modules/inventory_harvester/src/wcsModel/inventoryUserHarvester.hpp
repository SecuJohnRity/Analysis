#ifndef _INVENTORY_USER_HARVESTER_HPP
#define _INVENTORY_USER_HARVESTER_HPP

#include "reflectiveJson.hpp"
#include "wcsClasses/agent.hpp"
#include "wcsClasses/wazuh.hpp"
#include "wcsClasses/user.hpp" // Added this line
#include <string>
#include <vector>

// Main harvester struct
struct InventoryUserHarvester final {
    Agent agent;
    User user; // User and related structs are now included from wcsClasses/user.hpp
    Wazuh wazuh;
    Host host;
    Login login;
    Process process;

    REFLECTABLE(MAKE_FIELD("agent", &InventoryUserHarvester::agent),
                MAKE_FIELD("user", &InventoryUserHarvester::user),
                MAKE_FIELD("wazuh", &InventoryUserHarvester::wazuh),
                MAKE_FIELD("host", &InventoryUserHarvester::host),
                MAKE_FIELD("login", &InventoryUserHarvester::login),
                MAKE_FIELD("process", &InventoryUserHarvester::process));
};

#endif // _INVENTORY_USER_HARVESTER_HPP
