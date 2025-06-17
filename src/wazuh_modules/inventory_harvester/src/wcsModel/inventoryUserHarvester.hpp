#ifndef _INVENTORY_USER_HARVESTER_HPP
#define _INVENTORY_USER_HARVESTER_HPP

#include "reflectiveJson.hpp"
#include "wcsClasses/agent.hpp"
#include "wcsClasses/wazuh.hpp"
#include "wcsClasses/user.hpp" // This now provides User::Host, User::Login, User::Process
// host.hpp and process.hpp includes removed
// <string> and <vector> removed

// Main harvester struct
struct InventoryUserHarvester final {
    Agent agent;
    User user;
    Wazuh wazuh;
    User::Host host;     // CHANGED
    User::Login login;   // CHANGED
    User::Process process; // CHANGED

    REFLECTABLE(MAKE_FIELD("agent", &InventoryUserHarvester::agent),
                MAKE_FIELD("user", &InventoryUserHarvester::user),
                MAKE_FIELD("wazuh", &InventoryUserHarvester::wazuh),
                MAKE_FIELD("host", &InventoryUserHarvester::host),
                MAKE_FIELD("login", &InventoryUserHarvester::login),
                MAKE_FIELD("process", &InventoryUserHarvester::process));
};

#endif // _INVENTORY_USER_HARVESTER_HPP
