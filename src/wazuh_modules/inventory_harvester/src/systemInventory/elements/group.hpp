#ifndef _GROUP_HPP
#define _GROUP_HPP

#include "../../wcsModel/data.hpp"
#include "../../wcsModel/inventoryGroupHarvester.hpp" // Will be created/updated in next step
#include "../../wcsModel/noData.hpp"
#include "../policyHarvesterManager.hpp"
#include <stdexcept>
#include <string>
#include <vector>

// Forward declaration
// namespace wazuh { namespace inventory { namespace harvester { class InventoryGroupHarvester; }}}


template<typename TContext>
class Group final // Changed from GroupElement to Group
{
public:
    ~Group() = default;

    static DataHarvester<wazuh::inventory::harvester::InventoryGroupHarvester> build(TContext* data)
    {
        auto agentId = data->agentId();
        if (agentId.empty())
        {
            throw std::runtime_error("Agent ID is empty, cannot upsert group element.");
        }

        auto groupId = data->groupId(); // Assuming TContext will have a groupId() method
        if (groupId.empty())
        {
            throw std::runtime_error("Group ID is empty, cannot upsert group element.");
        }

        DataHarvester<wazuh::inventory::harvester::InventoryGroupHarvester> element;
        element.id = agentId + "_" + groupId;
        element.operation = "INSERTED"; // Or "UPDATED"
        element.data.agent.id = agentId;
        element.data.agent.name = data->agentName();
        element.data.agent.version = data->agentVersion();

        if (auto agentIp = data->agentIp(); agentIp.compare("any") != 0)
        {
            element.data.agent.host.ip = agentIp;
        }

        // Assuming TContext provides these methods for group data
        element.data.group.gid = data->groupGid();
        element.data.group.name = data->groupName();
        // For members, TContext might return a std::vector<std::string>
        // element.data.group.members = data->groupMembers();

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
            throw std::runtime_error("Agent ID is empty, cannot delete group element.");
        }

        auto groupId = data->groupId(); // Assuming TContext will have a groupId() method
        if (groupId.empty())
        {
            throw std::runtime_error("Group ID is empty, cannot delete group element.");
        }
        NoDataHarvester element;
        element.operation = "DELETED";
        element.id = agentId + "_" + groupId;
        return element;
    }
};

#endif // _GROUP_HPP
