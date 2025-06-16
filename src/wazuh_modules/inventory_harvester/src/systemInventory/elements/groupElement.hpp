#ifndef GROUP_ELEMENT_HPP
#define GROUP_ELEMENT_HPP

#include "../systemContext.hpp"
#include "../../wcsModel/data.hpp"
#include "../../wcsModel/noData.hpp"
#include "../../wcsModel/inventoryGroupHarvester.hpp" // Uses string_view
#include "../policyHarvesterManager.hpp"
#include "timeHelper.h"
#include "stringHelper.h" // For Utils::splitView

#include <stdexcept> // For std::runtime_error, std::invalid_argument, std::out_of_range
// <string>, <vector>, <memory> removed

template <typename TContext>
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

        auto groupItemId_sv = context->groupItemId();
        if (groupItemId_sv.empty())
        {
            throw std::runtime_error("GroupElement::build: Group Item ID is empty.");
        }

        DataHarvester<InventoryGroupHarvester> element;
        // Construct ID using string_view directly if possible, or ensure string concat is efficient.
        // Assuming DataHarvester::id is std::string.
        element.id = std::string(agentId_sv) + "_" + std::string(groupItemId_sv);
        element.operation = "INSERTED";

        InventoryGroupHarvester& wcsGroup = element.data;

        wcsGroup.agent.id = agentId_sv;
        wcsGroup.agent.name = context->agentName();
        wcsGroup.agent.version = context->agentVersion();

        auto agentIp_sv = context->agentIp();
        if (!agentIp_sv.empty() && agentIp_sv.compare("any") != 0)
        {
            wcsGroup.agent.ip = agentIp_sv; // Direct assignment
        } else {
            wcsGroup.agent.ip = "";
        }

        auto& instancePolicyManager = PolicyHarvesterManager::instance();
        wcsGroup.wazuh.cluster.name = instancePolicyManager.getClusterName();
        if (instancePolicyManager.getClusterStatus())
        {
            // Assuming getClusterNodeType() and getClusterNodeName() return string_view or compatible
            wcsGroup.wazuh.cluster.node_type = instancePolicyManager.getClusterNodeType();
            wcsGroup.wazuh.cluster.node = instancePolicyManager.getClusterNodeName();
        }

        wcsGroup.group.name = context->groupName();

        long gid_val = 0;
        unsigned long ugid_val = 0;
        std::string_view groupId_sv = context->groupId();
        if(!groupId_sv.empty()){
            std::string temp_str(groupId_sv); // stol needs null-terminated string
            try {
                gid_val = std::stol(temp_str);
                if (gid_val >= 0) {
                    ugid_val = static_cast<unsigned long>(gid_val);
                }
            } catch (const std::invalid_argument&) { /* default 0 */ }
              catch (const std::out_of_range&) { /* default 0 */ }
        }
        wcsGroup.group.id_signed = gid_val;
        wcsGroup.group.id = ugid_val;

        wcsGroup.group.description = context->groupDescription();
        wcsGroup.group.uuid = context->groupUuid();
        wcsGroup.group.is_hidden = context->groupIsHidden();

        auto users_sv = context->groupUsers();
        if (!users_sv.empty()) {
            Utils::splitView(users_sv, ',', wcsGroup.group.users); // Populates std::vector<std::string_view>
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

        auto groupItemId_sv = context->groupItemId();
        if (groupItemId_sv.empty())
        {
            throw std::runtime_error("GroupElement::deleteElement: Group Item ID is empty.");
        }

        NoDataHarvester element;
        element.id = std::string(agentId_sv) + "_" + std::string(groupItemId_sv);
        element.operation = "DELETED";
        return element;
    }
};

#endif // GROUP_ELEMENT_HPP
