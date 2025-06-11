#ifndef INVENTORY_USER_HARVESTER_HPP
#define INVENTORY_USER_HARVESTER_HPP

#include "wcsModel/wcsClasses/wcsBase.hpp" // Assuming a base class for WCS harvesters
#include "utils/jsonIO.hpp" // For cJSON manipulation
#include "utils/osPrimitives.hpp" // For OS-specific information like hostname

// Other necessary includes for data fetching (e.g., system headers)
// Needs Wazuh::Inventory::Harvester::SystemContext for AffectedComponentType
#include "systemInventory/systemContext.hpp"


namespace Wazuh {
namespace Inventory {
namespace Harvester {

class InventoryUserHarvester : public WcsBase { // Or appropriate base class
public:
    InventoryUserHarvester() : WcsBase(Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::User) {} // Set appropriate type

    std::vector<cJSON*> collectData(int agent_id, const std::string& command, const std::string& options) override {
        std::vector<cJSON*> collected_data;

        cJSON* user_payload = cJSON_CreateObject(); // This is the root object expected by UserElement.fromJson

        // User object
        cJSON* user_obj = cJSON_CreateObject();
        // Auth Failures
        cJSON* auth_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(auth_obj, "count", 0);
        cJSON_AddStringToObject(auth_obj, "timestamp", "2023-01-01T00:00:00Z");
        cJSON_AddItemToObject(user_obj, "auth_failures", auth_obj);

        cJSON_AddStringToObject(user_obj, "created", "2023-01-01T00:00:00Z");
        cJSON_AddStringToObject(user_obj, "full_name", "Dummy User Full Name");

        cJSON* group_info_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(group_info_obj, "id", "1000");
        cJSON_AddNumberToObject(group_info_obj, "id_signed", 1000);
        cJSON_AddItemToObject(user_obj, "group", group_info_obj);

        cJSON* groups_array = cJSON_CreateArray();
        cJSON_AddItemToArray(groups_array, cJSON_CreateString("dummygroup1"));
        cJSON_AddItemToArray(groups_array, cJSON_CreateString("dummygroup2"));
        cJSON_AddItemToObject(user_obj, "groups", groups_array);

        cJSON_AddStringToObject(user_obj, "home", "/home/dummy");
        cJSON_AddStringToObject(user_obj, "id", "dummy_uid_123"); // User ID
        cJSON_AddBoolToObject(user_obj, "is_hidden", false);
        cJSON_AddBoolToObject(user_obj, "is_remote", false);
        cJSON_AddStringToObject(user_obj, "last_login", "2023-01-15T10:00:00Z");
        cJSON_AddStringToObject(user_obj, "name", "dummy"); // Login name

        cJSON* pass_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(pass_obj, "expiration_date", "2024-01-01T00:00:00Z");
        cJSON_AddStringToObject(pass_obj, "hash_algorithm", "sha512");
        cJSON_AddNumberToObject(pass_obj, "inactive_days", 0);
        cJSON_AddNumberToObject(pass_obj, "last_change", 1672531200); // Example epoch
        cJSON_AddStringToObject(pass_obj, "last_set_time", "2023-01-01T00:00:00Z");
        cJSON_AddNumberToObject(pass_obj, "max_days_between_changes", 90);
        cJSON_AddNumberToObject(pass_obj, "min_days_between_changes", 1);
        cJSON_AddStringToObject(pass_obj, "status", "enabled");
        cJSON_AddNumberToObject(pass_obj, "warning_days_before_expiration", 7);
        cJSON_AddItemToObject(user_obj, "password", pass_obj);

        cJSON* roles_array = cJSON_CreateArray();
        cJSON_AddItemToArray(roles_array, cJSON_CreateString("dummyrole"));
        cJSON_AddItemToObject(user_obj, "roles", roles_array);

        cJSON_AddStringToObject(user_obj, "shell", "/bin/bash");
        cJSON_AddStringToObject(user_obj, "type", "regular");
        cJSON_AddNumberToObject(user_obj, "uid_signed", 1001);
        cJSON_AddStringToObject(user_obj, "uuid", "dummy-uuid-user-123");
        cJSON_AddItemToObject(user_payload, "user", user_obj);

        // Host object
        cJSON* host_obj = cJSON_CreateObject();
        cJSON* ips_array = cJSON_CreateArray();
        cJSON_AddItemToArray(ips_array, cJSON_CreateString("127.0.0.1"));
        cJSON_AddItemToObject(host_obj, "ip", ips_array);
        std::string hostname = Os::Host::getHostname("dummy_host"); // Assuming Os::Host::getHostname exists
        cJSON_AddStringToObject(host_obj, "hostname", hostname.c_str());
        cJSON_AddItemToObject(user_payload, "host", host_obj);

        // Process object
        cJSON* process_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(process_obj, "pid", 0); // Or a dummy PID
        cJSON_AddItemToObject(user_payload, "process", process_obj);

        // Login object
        cJSON* login_obj = cJSON_CreateObject();
        cJSON_AddBoolToObject(login_obj, "status", true);
        cJSON_AddStringToObject(login_obj, "tty", "pts/0");
        cJSON_AddStringToObject(login_obj, "type", "interactive");
        cJSON_AddItemToObject(user_payload, "login", login_obj);

        collected_data.push_back(user_payload);
        return collected_data;
    }

    // It's likely WcsBase or a similar class would define the interface
    // for command, type, and potentially methods to get the data.
    // This is a simplified example.
    // std::string getCommand() const override { return "get_users"; } // Example
    // AffectedComponentType getType() const override { return AffectedComponentType::User; } // Example
};

} // namespace Harvester
} // namespace Inventory
} // namespace Wazuh

#endif // INVENTORY_USER_HARVESTER_HPP
