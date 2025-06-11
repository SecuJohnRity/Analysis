#ifndef _USER_HPP
#define _USER_HPP

#include "../../wcsModel/data.hpp"
#include "../../wcsModel/inventoryUserHarvester.hpp" // Will be created/updated in next step
#include "../../wcsModel/noData.hpp"
#include "../policyHarvesterManager.hpp"
#include <stdexcept>
#include <string>
#include <vector>

// Forward declaration of InventoryUserHarvester if not fully defined yet elsewhere
// namespace wazuh { namespace inventory { namespace harvester { class InventoryUserHarvester; }}}

template<typename TContext>
class User final // Changed from UserElement to User
{
public:
    ~User() = default;

    static DataHarvester<wazuh::inventory::harvester::InventoryUserHarvester> build(TContext* data)
    {
        auto agentId = data->agentId();
        if (agentId.empty())
        {
            throw std::runtime_error("Agent ID is empty, cannot upsert user element.");
        }

        auto userId = data->userId(); // Assuming TContext will have a userId() method
        if (userId.empty())
        {
            throw std::runtime_error("User ID is empty, cannot upsert user element.");
        }

        DataHarvester<wazuh::inventory::harvester::InventoryUserHarvester> element;
        element.id = agentId + "_" + userId;
        element.operation = "INSERTED"; // Or "UPDATED" based on logic in TContext or caller
        element.data.agent.id = agentId;
        element.data.agent.name = data->agentName();
        element.data.agent.version = data->agentVersion();

        if (auto agentIp = data->agentIp(); agentIp.compare("any") != 0)
        {
            element.data.agent.host.ip = agentIp;
        }

        // Assuming TContext provides these methods for user data
        element.data.user.uid = data->userUid();
        element.data.user.gid = data->userGid();
        element.data.user.name = data->userName();
        element.data.user.home = data->userHome();
        element.data.user.shell = data->userShell();
        // For groups, TContext might return a std::vector<std::string>
        // element.data.user.groups = data->userGroups();

        auto& instancePolicyManager = wazuh::inventory::harvester::PolicyHarvesterManager::instance();
        element.data.wazuh.cluster.name = instancePolicyManager.getClusterName();
        if (instancePolicyManager.getClusterStatus())
        {
            element.data.wazuh.cluster.node = instancePolicyManager.getClusterNodeName();
        }

        return element;
    }

    static NoDataHarvester deleteElement(TContext* data)
    {
        auto agentId = data->agentId();
        if (agentId.empty())
        {
            throw std::runtime_error("Agent ID is empty, cannot delete user element.");
        }

        auto userId = data->userId(); // Assuming TContext will have a userId() method
        if (userId.empty())
        {
            throw std::runtime_error("User ID is empty, cannot delete user element.");
        }
        NoDataHarvester element;
        element.operation = "DELETED";
        element.id = agentId + "_" + userId;
        return element;
    }
};

#endif // _USER_HPP
