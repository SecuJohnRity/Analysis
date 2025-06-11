#ifndef GROUP_ELEMENT_HPP
#define GROUP_ELEMENT_HPP

#include <string>
#include <vector>
#include "utils/jsonIO.hpp" // Assuming this is the correct path
// #include "systemInventory/systemContext.hpp" // If needed for TContext

namespace Wazuh {
namespace Inventory {
namespace Harvester {

template<typename TContext>
class GroupElement {
public:
    struct GroupData { // Corresponds to the "group" object
        std::string description;
        std::string id; // Using string for id due to unsigned_long in template
        long id_signed = 0;
        bool is_hidden = false;
        std::string name;
        std::vector<std::string> users;
        std::string uuid;
    } group;

    // Host struct remains as a general context piece
    struct HostData { // Renamed to avoid conflict
        std::vector<std::string> ip;
        std::string hostname;
    } host;

    // Static build method
    static GroupElement<TContext> build(TContext* context) {
        GroupElement<TContext> element;
        const cJSON* json = context->getRawData(); // Assuming context has getRawData()
        if (json) {
            element.fromJson(json); // Delegate parsing
        }
        return element;
    }

    void fromJson(const cJSON* json) {
        if (!json) return;

        // Parse group object
        const cJSON* group_json = cJSON_GetObjectItemCaseSensitive(json, "group");
        if (group_json) {
            group.description = Utils::Json::getString(group_json, "description", "");
            group.id = Utils::Json::getString(group_json, "id", ""); // Or getUnsignedLong if available
            group.id_signed = Utils::Json::getLong(group_json, "id_signed", 0);
            group.is_hidden = Utils::Json::getBool(group_json, "is_hidden", false);
            group.name = Utils::Json::getString(group_json, "name", "");
            group.users = Utils::Json::getStringVector(cJSON_GetObjectItemCaseSensitive(group_json, "users"));
            group.uuid = Utils::Json::getString(group_json, "uuid", "");
        }

        // Parse host object
        const cJSON* host_json = cJSON_GetObjectItemCaseSensitive(json, "host");
        if (host_json) {
            host.ip = Utils::Json::getStringVector(cJSON_GetObjectItemCaseSensitive(host_json, "ip"));
            host.hostname = Utils::Json::getString(host_json, "hostname", "");
        }
    }

    cJSON* toJson() const {
        cJSON* root = cJSON_CreateObject();

        // Group object
        cJSON* group_obj = cJSON_CreateObject();
        if (!group.description.empty()) cJSON_AddStringToObject(group_obj, "description", group.description.c_str());
        if (!group.id.empty()) cJSON_AddStringToObject(group_obj, "id", group.id.c_str()); // Or AddNumber
        cJSON_AddNumberToObject(group_obj, "id_signed", group.id_signed);
        cJSON_AddBoolToObject(group_obj, "is_hidden", group.is_hidden);
        if (!group.name.empty()) cJSON_AddStringToObject(group_obj, "name", group.name.c_str());
        if (!group.users.empty()) cJSON_AddItemToObject(group_obj, "users", Utils::Json::stringVectorToJson(group.users));
        if (!group.uuid.empty()) cJSON_AddStringToObject(group_obj, "uuid", group.uuid.c_str());
        cJSON_AddItemToObject(root, "group", group_obj);

        // Host object
        cJSON* host_obj = cJSON_CreateObject();
        if (!host.ip.empty()) cJSON_AddItemToObject(host_obj, "ip", Utils::Json::stringVectorToJson(host.ip));
        if (!host.hostname.empty()) cJSON_AddStringToObject(host_obj, "hostname", host.hostname.c_str());
        cJSON_AddItemToObject(root, "host", host_obj);

        return root;
    }
};

} // namespace Harvester
} // namespace Inventory
} // namespace Wazuh

#endif // GROUP_ELEMENT_HPP
