#ifndef GROUP_ELEMENT_HPP
#define GROUP_ELEMENT_HPP

#include "../systemContext.hpp" // TContext will be SystemContext
#include "../../wcsModel/data.hpp"
#include "../../wcsModel/noData.hpp"
#include "../../wcsModel/inventoryGroupHarvester.hpp"
#include "../policyHarvesterManager.hpp" // For cluster info
#include "timeHelper.h"   // For Utils::getCurrentTimeISO8601, if needed
#include "stringHelper.h" // For Utils::splitView

#include <stdexcept> // For std::runtime_error, std::invalid_argument, std::out_of_range
#include <string>
#include <vector>
#include <memory>

template <typename TContext> // TContext will be SystemContext
class GroupElement final
{
public:
    ~GroupElement() = default;

    static DataHarvester<InventoryGroupHarvester> build(TContext* context)
    {
        auto agentId_sv = context->agentId();
        if (agentId_sv.empty())
        {
            throw std::runtime_error("GroupElement::build: Agent ID is empty.");
        }
        std::string agentId = std::string(agentId_sv);

        auto groupItemId_sv = context->groupItemId(); // Assuming this gives GID or unique ID for group
        if (groupItemId_sv.empty())
        {
            throw std::runtime_error("GroupElement::build: Group Item ID is empty.");
        }
        std::string groupItemId = std::string(groupItemId_sv);

        DataHarvester<InventoryGroupHarvester> element;
        element.id = agentId + "_" + groupItemId;
        element.operation = "INSERTED";

        InventoryGroupHarvester& wcsGroup = element.data;

        // Agent info
        wcsGroup.agent.id = agentId;
        wcsGroup.agent.name = std::string(context->agentName());
        wcsGroup.agent.version = std::string(context->agentVersion());
        if (auto agentIp_sv = context->agentIp(); !agentIp_sv.empty() && agentIp_sv.compare("any") != 0)
        {
            wcsGroup.agent.ip = std::string(agentIp_sv);
        } else {
            wcsGroup.agent.ip = ""; // Set to empty if "any" or empty
        }

        // Wazuh info
        auto& instancePolicyManager = PolicyHarvesterManager::instance();
        wcsGroup.wazuh.cluster.name = instancePolicyManager.getClusterName();
        if (instancePolicyManager.getClusterStatus())
        {
            wcsGroup.wazuh.cluster.node_type = instancePolicyManager.getClusterNodeType();
            wcsGroup.wazuh.cluster.node = instancePolicyManager.getClusterNodeName();
        }
        // wcsGroup.wazuh.timestamp = Utils::getCurrentTimeISO8601(); // Set by serializer usually

        // Group details
        wcsGroup.group.name = std::string(context->groupName());

        long gid_val = 0;
        unsigned long ugid_val = 0;
        std::string groupId_str = std::string(context->groupId()); // Assuming groupId() returns string GID
        if(!groupId_str.empty()){
            try {
                gid_val = std::stol(groupId_str);
                if (gid_val >= 0) {
                    ugid_val = static_cast<unsigned long>(gid_val);
                }
            } catch (const std::invalid_argument&) { /* default 0 */ }
              catch (const std::out_of_range&) { /* default 0 */ }
        }
        wcsGroup.group.id_signed = gid_val;
        wcsGroup.group.id = ugid_val;

        wcsGroup.group.description = std::string(context->groupDescription());
        wcsGroup.group.uuid = std::string(context->groupUuid());
        wcsGroup.group.is_hidden = context->groupIsHidden();

        auto users_sv = context->groupUsers();
        if (!users_sv.empty()) {
            Utils::splitView(users_sv, ',', wcsGroup.group.users);
        }

        return element;
    }

    static NoDataHarvester deleteElement(TContext* context)
    {
        auto agentId_sv = context->agentId();
        if (agentId_sv.empty())
        {
            throw std::runtime_error("GroupElement::deleteElement: Agent ID is empty.");
        }
        std::string agentId = std::string(agentId_sv);

        auto groupItemId_sv = context->groupItemId();
        if (groupItemId_sv.empty())
        {
            throw std::runtime_error("GroupElement::deleteElement: Group Item ID is empty.");
        }
        std::string groupItemId = std::string(groupItemId_sv);

        NoDataHarvester element;
        element.id = agentId + "_" + groupItemId;
        element.operation = "DELETED";
        return element;
    }
};

#endif // GROUP_ELEMENT_HPP
