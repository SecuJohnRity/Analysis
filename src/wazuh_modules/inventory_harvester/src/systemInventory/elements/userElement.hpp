#ifndef USER_ELEMENT_HPP
#define USER_ELEMENT_HPP

#include <string>
#include <vector>
#include "utils/jsonIO.hpp" // Assuming this is the correct path
// #include "systemInventory/systemContext.hpp" // If needed for TContext

namespace Wazuh {
namespace Inventory {
namespace Harvester {

template<typename TContext>
class UserElement {
public:
    // Nested structs to represent the detailed user information
    struct AuthFailures {
        int count = 0;
        std::string timestamp; // Consider date/time type if available
    };

    struct UserGroupInfo { // For user.group
        std::string id; // unsigned_long in template, using string for flexibility or long
        long id_signed = 0;
    };

    struct PasswordInfo {
        std::string expiration_date;
        std::string hash_algorithm;
        int inactive_days = 0;
        long last_change = 0; // Assuming numeric (e.g., epoch)
        std::string last_set_time;
        int max_days_between_changes = 0;
        int min_days_between_changes = 0;
        std::string status;
        int warning_days_before_expiration = 0;
    };

    struct UserData { // Corresponds to the "user" object
        AuthFailures auth_failures;
        std::string created;
        std::string full_name;
        UserGroupInfo group; // Primary group
        std::vector<std::string> groups; // Secondary groups
        std::string home;
        std::string id; // User ID (e.g., UID string or number)
        bool is_hidden = false;
        bool is_remote = false;
        std::string last_login;
        std::string name; // Login name
        PasswordInfo password;
        std::vector<std::string> roles;
        std::string shell;
        std::string type;
        long uid_signed = 0; // Signed UID
        std::string uuid;
    } user;

    // Host and Process structs remain as they are general context
    struct HostData { // Renamed to avoid conflict with a potential 'Host' class
        std::vector<std::string> ip;
        std::string hostname;
    } host;

    struct ProcessData { // Renamed
        long pid = 0; // Changed to long to match template for process.pid
    } process;

    struct LoginData { // New struct for login information based on template
        bool status = false;
        std::string tty;
        std::string type;
    } login;


    // Static build method - needs to parse the new detailed structure
    static UserElement<TContext> build(TContext* context) {
        UserElement<TContext> element;
        const cJSON* json = context->getRawData(); // Assuming context has getRawData()
        if (json) {
            element.fromJson(json); // Delegate parsing to fromJson
        }
        return element;
    }

    void fromJson(const cJSON* json) {
        if (!json) return;

        // Parse user object
        const cJSON* user_json = cJSON_GetObjectItemCaseSensitive(json, "user");
        if (user_json) {
            const cJSON* auth_json = cJSON_GetObjectItemCaseSensitive(user_json, "auth_failures");
            if (auth_json) {
                user.auth_failures.count = Utils::Json::getInt(auth_json, "count", 0);
                user.auth_failures.timestamp = Utils::Json::getString(auth_json, "timestamp", "");
            }
            user.created = Utils::Json::getString(user_json, "created", "");
            user.full_name = Utils::Json::getString(user_json, "full_name", "");
            const cJSON* group_info_json = cJSON_GetObjectItemCaseSensitive(user_json, "group");
            if (group_info_json) {
                user.group.id = Utils::Json::getString(group_info_json, "id", ""); // Or getInt/getLong
                user.group.id_signed = Utils::Json::getLong(group_info_json, "id_signed", 0);
            }
            user.groups = Utils::Json::getStringVector(cJSON_GetObjectItemCaseSensitive(user_json, "groups"));
            user.home = Utils::Json::getString(user_json, "home", "");
            user.id = Utils::Json::getString(user_json, "id", "");
            user.is_hidden = Utils::Json::getBool(user_json, "is_hidden", false);
            user.is_remote = Utils::Json::getBool(user_json, "is_remote", false);
            user.last_login = Utils::Json::getString(user_json, "last_login", "");
            user.name = Utils::Json::getString(user_json, "name", "");

            const cJSON* pass_json = cJSON_GetObjectItemCaseSensitive(user_json, "password");
            if (pass_json) {
                user.password.expiration_date = Utils::Json::getString(pass_json, "expiration_date", "");
                user.password.hash_algorithm = Utils::Json::getString(pass_json, "hash_algorithm", "");
                user.password.inactive_days = Utils::Json::getInt(pass_json, "inactive_days", 0);
                user.password.last_change = Utils::Json::getLong(pass_json, "last_change", 0);
                user.password.last_set_time = Utils::Json::getString(pass_json, "last_set_time", "");
                user.password.max_days_between_changes = Utils::Json::getInt(pass_json, "max_days_between_changes", 0);
                user.password.min_days_between_changes = Utils::Json::getInt(pass_json, "min_days_between_changes", 0);
                user.password.status = Utils::Json::getString(pass_json, "status", "");
                user.password.warning_days_before_expiration = Utils::Json::getInt(pass_json, "warning_days_before_expiration", 0);
            }
            user.roles = Utils::Json::getStringVector(cJSON_GetObjectItemCaseSensitive(user_json, "roles"));
            user.shell = Utils::Json::getString(user_json, "shell", "");
            user.type = Utils::Json::getString(user_json, "type", "");
            user.uid_signed = Utils::Json::getLong(user_json, "uid_signed", 0);
            user.uuid = Utils::Json::getString(user_json, "uuid", "");
        }

        // Parse host object
        const cJSON* host_json = cJSON_GetObjectItemCaseSensitive(json, "host");
        if (host_json) {
            host.ip = Utils::Json::getStringVector(cJSON_GetObjectItemCaseSensitive(host_json, "ip"));
            host.hostname = Utils::Json::getString(host_json, "hostname", "");
        }

        // Parse process object
        const cJSON* process_json = cJSON_GetObjectItemCaseSensitive(json, "process");
        if (process_json) {
            process.pid = Utils::Json::getLong(process_json, "pid", 0);
        }

        // Parse login object
        const cJSON* login_json = cJSON_GetObjectItemCaseSensitive(json, "login");
        if (login_json) {
            login.status = Utils::Json::getBool(login_json, "status", false);
            login.tty = Utils::Json::getString(login_json, "tty", "");
            login.type = Utils::Json::getString(login_json, "type", "");
        }
    }

    // toJson method needs to be updated to serialize all new fields
    cJSON* toJson() const {
        cJSON* root = cJSON_CreateObject();

        // User object
        cJSON* user_obj = cJSON_CreateObject();
        // Auth Failures
        cJSON* auth_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(auth_obj, "count", user.auth_failures.count);
        if (!user.auth_failures.timestamp.empty()) cJSON_AddStringToObject(auth_obj, "timestamp", user.auth_failures.timestamp.c_str());
        cJSON_AddItemToObject(user_obj, "auth_failures", auth_obj);

        if (!user.created.empty()) cJSON_AddStringToObject(user_obj, "created", user.created.c_str());
        if (!user.full_name.empty()) cJSON_AddStringToObject(user_obj, "full_name", user.full_name.c_str());
        // User Group Info
        cJSON* group_info_obj = cJSON_CreateObject();
        if (!user.group.id.empty()) cJSON_AddStringToObject(group_info_obj, "id", user.group.id.c_str()); // Or AddNumber
        cJSON_AddNumberToObject(group_info_obj, "id_signed", user.group.id_signed);
        cJSON_AddItemToObject(user_obj, "group", group_info_obj);

        if (!user.groups.empty()) cJSON_AddItemToObject(user_obj, "groups", Utils::Json::stringVectorToJson(user.groups));
        if (!user.home.empty()) cJSON_AddStringToObject(user_obj, "home", user.home.c_str());
        if (!user.id.empty()) cJSON_AddStringToObject(user_obj, "id", user.id.c_str());
        cJSON_AddBoolToObject(user_obj, "is_hidden", user.is_hidden);
        cJSON_AddBoolToObject(user_obj, "is_remote", user.is_remote);
        if (!user.last_login.empty()) cJSON_AddStringToObject(user_obj, "last_login", user.last_login.c_str());
        if (!user.name.empty()) cJSON_AddStringToObject(user_obj, "name", user.name.c_str());

        // Password Info
        cJSON* pass_obj = cJSON_CreateObject();
        if (!user.password.expiration_date.empty()) cJSON_AddStringToObject(pass_obj, "expiration_date", user.password.expiration_date.c_str());
        if (!user.password.hash_algorithm.empty()) cJSON_AddStringToObject(pass_obj, "hash_algorithm", user.password.hash_algorithm.c_str());
        cJSON_AddNumberToObject(pass_obj, "inactive_days", user.password.inactive_days);
        cJSON_AddNumberToObject(pass_obj, "last_change", user.password.last_change);
        if (!user.password.last_set_time.empty()) cJSON_AddStringToObject(pass_obj, "last_set_time", user.password.last_set_time.c_str());
        cJSON_AddNumberToObject(pass_obj, "max_days_between_changes", user.password.max_days_between_changes);
        cJSON_AddNumberToObject(pass_obj, "min_days_between_changes", user.password.min_days_between_changes);
        if (!user.password.status.empty()) cJSON_AddStringToObject(pass_obj, "status", user.password.status.c_str());
        cJSON_AddNumberToObject(pass_obj, "warning_days_before_expiration", user.password.warning_days_before_expiration);
        cJSON_AddItemToObject(user_obj, "password", pass_obj);

        if (!user.roles.empty()) cJSON_AddItemToObject(user_obj, "roles", Utils::Json::stringVectorToJson(user.roles));
        if (!user.shell.empty()) cJSON_AddStringToObject(user_obj, "shell", user.shell.c_str());
        if (!user.type.empty()) cJSON_AddStringToObject(user_obj, "type", user.type.c_str());
        cJSON_AddNumberToObject(user_obj, "uid_signed", user.uid_signed);
        if (!user.uuid.empty()) cJSON_AddStringToObject(user_obj, "uuid", user.uuid.c_str());
        cJSON_AddItemToObject(root, "user", user_obj);

        // Host object
        cJSON* host_obj = cJSON_CreateObject();
        if (!host.ip.empty()) cJSON_AddItemToObject(host_obj, "ip", Utils::Json::stringVectorToJson(host.ip));
        if (!host.hostname.empty()) cJSON_AddStringToObject(host_obj, "hostname", host.hostname.c_str());
        cJSON_AddItemToObject(root, "host", host_obj);

        // Process object
        cJSON* process_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(process_obj, "pid", process.pid);
        cJSON_AddItemToObject(root, "process", process_obj);

        // Login object
        cJSON* login_obj = cJSON_CreateObject();
        cJSON_AddBoolToObject(login_obj, "status", login.status);
        if (!login.tty.empty()) cJSON_AddStringToObject(login_obj, "tty", login.tty.c_str());
        if (!login.type.empty()) cJSON_AddStringToObject(login_obj, "type", login.type.c_str());
        cJSON_AddItemToObject(root, "login", login_obj);

        return root;
    }
};

} // namespace Harvester
} // namespace Inventory
} // namespace Wazuh

#endif // USER_ELEMENT_HPP
