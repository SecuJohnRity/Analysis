#ifndef USER_ELEMENT_HPP
#define USER_ELEMENT_HPP

#include "../systemContext.hpp" // TContext will be SystemContext
#include "../../wcsModel/data.hpp"
#include "../../wcsModel/noData.hpp"
#include "../../wcsModel/inventoryUserHarvester.hpp"
#include "../policyHarvesterManager.hpp" // For cluster info
#include "timeHelper.h"   // For Utils::getCurrentTimeISO8601, if needed
#include "stringHelper.h" // For Utils::splitView

#include <stdexcept> // For std::runtime_error, std::invalid_argument, std::out_of_range
#include <string>
#include <vector>
#include <memory>

template <typename TContext> // TContext will be SystemContext
class UserElement final
{
public:
    ~UserElement() = default;

    static DataHarvester<InventoryUserHarvester> build(TContext* context)
    {
        auto agentId_sv = context->agentId();
        if (agentId_sv.empty())
        {
            throw std::runtime_error("UserElement::build: Agent ID is empty.");
        }
        std::string agentId = std::string(agentId_sv);

        auto userItemId_sv = context->userItemId(); // Assuming userItemId gives UID or unique ID for user
        if (userItemId_sv.empty())
        {
            throw std::runtime_error("UserElement::build: User Item ID is empty.");
        }
        std::string userItemId = std::string(userItemId_sv);

        DataHarvester<InventoryUserHarvester> element;
        element.id = agentId + "_" + userItemId;
        element.operation = "INSERTED"; // Or "UPDATED" - system usually uses INSERTED for upsert

        // Populate element.data (InventoryUserHarvester)
        InventoryUserHarvester& wcsUser = element.data;

        // Agent info
        wcsUser.agent.id = agentId;
        wcsUser.agent.name = std::string(context->agentName());
        wcsUser.agent.version = std::string(context->agentVersion());
        if (auto agentIp_sv = context->agentIp(); !agentIp_sv.empty() && agentIp_sv.compare("any") != 0)
        {
            // Assuming InventoryUserHarvester has agent.host.ip or similar structure
            // Based on inventoryUserHarvester.hpp, it's wcsUser.host.ip, and agent.ip directly
            wcsUser.agent.ip = std::string(agentIp_sv);
            wcsUser.host.ip = std::string(agentIp_sv);
        } else {
            wcsUser.agent.ip = ""; // Set to empty if "any" or empty
            wcsUser.host.ip = "";
        }


        // Wazuh info (PolicyHarvesterManager is used in PackageElement for this)
        auto& instancePolicyManager = PolicyHarvesterManager::instance();
        wcsUser.wazuh.cluster.name = instancePolicyManager.getClusterName();
        if (instancePolicyManager.getClusterStatus())
        {
            wcsUser.wazuh.cluster.node_type = instancePolicyManager.getClusterNodeType(); // Assuming node_type from policy manager
            wcsUser.wazuh.cluster.node = instancePolicyManager.getClusterNodeName();
        }
        // wcsUser.wazuh.timestamp = Utils::getCurrentTimeISO8601(); // Set by serializer usually

        // Login info
        wcsUser.login.status = context->userLoginStatus();
        wcsUser.login.tty = std::string(context->userLoginTty());
        wcsUser.login.type = std::string(context->userLoginType());

        // Process info (pid of the process related to the user event, if applicable)
        // wcsUser.process.pid = ... // Defaulted if not set

        // User details
        wcsUser.user.id = std::string(context->userId());
        wcsUser.user.name = std::string(context->userName());

        long gid_val = 0;
        unsigned long ugid_val = 0;
        std::string userGroupId_str = std::string(context->userGroupId());
        if (!userGroupId_str.empty()) {
            try {
                gid_val = std::stol(userGroupId_str);
                if (gid_val >= 0) {
                    ugid_val = static_cast<unsigned long>(gid_val);
                }
            } catch (const std::invalid_argument&) { /* default 0 */ }
              catch (const std::out_of_range&) { /* default 0 */ }
        }
        wcsUser.user.group.id_signed = gid_val;
        wcsUser.user.group.id = ugid_val;


        wcsUser.user.home = std::string(context->userHome());
        wcsUser.user.shell = std::string(context->userShell());
        wcsUser.user.uuid = std::string(context->userUuid());
        wcsUser.user.full_name = std::string(context->userFullName());
        wcsUser.user.is_hidden = context->userIsHidden();
        wcsUser.user.is_remote = context->userIsRemote();
        wcsUser.user.created = std::string(context->userCreated());
        wcsUser.user.last_login = std::string(context->userLastLogin());
        // wcsUser.user.type remains default as it's not in SystemContext

        long uid_signed_val = 0;
        std::string userId_str = std::string(context->userId()); // Assuming userId() is the string representation of UID
        if (!userId_str.empty()) {
            try {
                uid_signed_val = std::stol(userId_str);
            } catch (const std::invalid_argument&) { /* default 0 */ }
              catch (const std::out_of_range&) { /* default 0 */ }
        }
        wcsUser.user.uid_signed = uid_signed_val;

        // Password details
        wcsUser.user.password.status = std::string(context->userPasswordStatus());
        wcsUser.user.password.last_change = context->userPasswordLastChange();
        wcsUser.user.password.expiration_date = std::string(context->userPasswordExpirationDate());
        wcsUser.user.password.hash_algorithm = std::string(context->userPasswordHashAlgorithm());
        wcsUser.user.password.inactive_days = context->userPasswordInactiveDays();
        wcsUser.user.password.last_set_time = std::string(context->userPasswordLastSetTime());
        wcsUser.user.password.max_days_between_changes = context->userPasswordMaxDays();
        wcsUser.user.password.min_days_between_changes = context->userPasswordMinDays();
        wcsUser.user.password.warning_days_before_expiration = context->userPasswordWarningDays();

        // Roles and Groups
        auto roles_sv = context->userRoles();
        if (!roles_sv.empty()) { Utils::splitView(roles_sv, ',', wcsUser.user.roles); }
        auto groups_sv = context->userGroups();
        if (!groups_sv.empty()) { Utils::splitView(groups_sv, ',', wcsUser.user.groups); }

        // Auth Failures
        wcsUser.user.auth_failures.count = context->userAuthFailuresCount();
        wcsUser.user.auth_failures.timestamp = std::string(context->userAuthFailuresTimestamp());

        return element;
    }

    static NoDataHarvester deleteElement(TContext* context)
    {
        auto agentId_sv = context->agentId();
        if (agentId_sv.empty())
        {
            throw std::runtime_error("UserElement::deleteElement: Agent ID is empty.");
        }
        std::string agentId = std::string(agentId_sv);

        auto userItemId_sv = context->userItemId();
        if (userItemId_sv.empty())
        {
            throw std::runtime_error("UserElement::deleteElement: User Item ID is empty.");
        }
        std::string userItemId = std::string(userItemId_sv);

        NoDataHarvester element;
        element.id = agentId + "_" + userItemId;
        element.operation = "DELETED";
        return element;
    }
};

#endif // USER_ELEMENT_HPP
