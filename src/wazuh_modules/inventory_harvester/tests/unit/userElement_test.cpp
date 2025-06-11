#include "gtest/gtest.h"
#include "wazuh_modules/inventory_harvester/src/systemInventory/elements/userElement.hpp" // Adjust path as necessary
#include "utils/jsonIO.hpp" // For cJSON manipulation and cleanup
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

// Mock SystemContext if UserElement::build were to be tested directly with complex context interaction.
// For fromJson, we directly provide the cJSON.
// class MockSystemContext { // Example, if needed later
// public:
//     MOCK_METHOD(const cJSON*, getRawData, (), (const));
// };


// Test fixture for UserElement tests
class UserElementTest : public ::testing::Test {
protected:
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> jsonData = {nullptr, cJSON_Delete};

    void SetUp() override {
        // Setup common to tests can go here
    }

    void TearDown() override {
        jsonData.reset(); // Ensure cJSON object is deleted
    }

    // Helper to create a full valid JSON for UserElement for testing
    cJSON* createValidUserJson() {
        cJSON* root = cJSON_CreateObject();

        cJSON* user = cJSON_CreateObject();
        cJSON_AddStringToObject(user, "name", "testuser");
        cJSON_AddNumberToObject(user, "uid_signed", 1001);
        cJSON_AddStringToObject(user, "id", "testuser_uid");
        cJSON_AddStringToObject(user, "uuid", "test-uuid-123");
        cJSON_AddStringToObject(user, "home", "/home/testuser");
        cJSON_AddStringToObject(user, "shell", "/bin/bash");
        cJSON_AddStringToObject(user, "type", "regular");
        cJSON_AddBoolToObject(user, "is_hidden", false);
        cJSON_AddBoolToObject(user, "is_remote", false);
        cJSON_AddStringToObject(user, "created", "2023-01-01T12:00:00Z");
        cJSON_AddStringToObject(user, "last_login", "2023-01-10T10:00:00Z");
        cJSON_AddStringToObject(user, "full_name", "Test User Full");


        cJSON* auth_failures = cJSON_CreateObject();
        cJSON_AddNumberToObject(auth_failures, "count", 5);
        cJSON_AddStringToObject(auth_failures, "timestamp", "2023-01-09T00:00:00Z");
        cJSON_AddItemToObject(user, "auth_failures", auth_failures);

        cJSON* group_info = cJSON_CreateObject();
        cJSON_AddStringToObject(group_info, "id", "100");
        cJSON_AddNumberToObject(group_info, "id_signed", 100);
        cJSON_AddItemToObject(user, "group", group_info);

        cJSON* groups = cJSON_CreateArray();
        cJSON_AddItemToArray(groups, cJSON_CreateString("users"));
        cJSON_AddItemToArray(groups, cJSON_CreateString("audio"));
        cJSON_AddItemToObject(user, "groups", groups);

        cJSON* password = cJSON_CreateObject();
        cJSON_AddStringToObject(password, "status", "enabled");
        cJSON_AddStringToObject(password, "expiration_date", "2025-01-01T00:00:00Z");
        cJSON_AddStringToObject(password, "hash_algorithm", "sha512");
        cJSON_AddNumberToObject(password, "inactive_days", 10);
        cJSON_AddNumberToObject(password, "last_change", 1670000000);
        cJSON_AddStringToObject(password, "last_set_time", "2022-12-01T00:00:00Z");
        cJSON_AddNumberToObject(password, "max_days_between_changes", 90);
        cJSON_AddNumberToObject(password, "min_days_between_changes", 1);
        cJSON_AddNumberToObject(password, "warning_days_before_expiration", 7);
        cJSON_AddItemToObject(user, "password", password);

        cJSON* roles = cJSON_CreateArray();
        cJSON_AddItemToArray(roles, cJSON_CreateString("admin"));
        cJSON_AddItemToObject(user, "roles", roles);

        cJSON_AddItemToObject(root, "user", user);

        cJSON* host = cJSON_CreateObject();
        cJSON* ips = cJSON_CreateArray();
        cJSON_AddItemToArray(ips, cJSON_CreateString("192.168.1.10"));
        cJSON_AddItemToObject(host, "ip", ips);
        cJSON_AddStringToObject(host, "hostname", "testhost");
        cJSON_AddItemToObject(root, "host", host);

        cJSON* process = cJSON_CreateObject();
        cJSON_AddNumberToObject(process, "pid", 1234);
        cJSON_AddItemToObject(root, "process", process);

        cJSON* login = cJSON_CreateObject();
        cJSON_AddBoolToObject(login, "status", true);
        cJSON_AddStringToObject(login, "tty", "pts/0");
        cJSON_AddStringToObject(login, "type", "interactive");
        cJSON_AddItemToObject(root, "login", login);

        return root;
    }
};

// Test case for parsing a valid JSON
TEST_F(UserElementTest, FromJson_ValidData) {
    jsonData.reset(createValidUserJson());
    ASSERT_NE(jsonData.get(), nullptr);

    // Assuming SystemContext is not strictly needed for UserElement direct instantiation for test,
    // or a simple one can be passed if the template requires it.
    // For now, UserElement is not templated in its definition in the prompt.
    // If it becomes `UserElement<MockSystemContext>` then adjust.
    Wazuh::Inventory::Harvester::UserElement<void> userElement; // Use void or a dummy type for TContext if not used by fromJson
    userElement.fromJson(jsonData.get());

    // User assertions
    EXPECT_EQ(userElement.user.name, "testuser");
    EXPECT_EQ(userElement.user.uid_signed, 1001);
    EXPECT_EQ(userElement.user.id, "testuser_uid");
    EXPECT_EQ(userElement.user.uuid, "test-uuid-123");
    EXPECT_EQ(userElement.user.home, "/home/testuser");
    EXPECT_EQ(userElement.user.shell, "/bin/bash");
    EXPECT_EQ(userElement.user.auth_failures.count, 5);
    EXPECT_EQ(userElement.user.auth_failures.timestamp, "2023-01-09T00:00:00Z");
    EXPECT_EQ(userElement.user.group.id_signed, 100);
    ASSERT_EQ(userElement.user.groups.size(), 2);
    EXPECT_EQ(userElement.user.groups[0], "users");
    EXPECT_EQ(userElement.user.password.status, "enabled");
    EXPECT_EQ(userElement.user.full_name, "Test User Full");


    // Host assertions
    EXPECT_EQ(userElement.host.hostname, "testhost");
    ASSERT_EQ(userElement.host.ip.size(), 1);
    EXPECT_EQ(userElement.host.ip[0], "192.168.1.10");

    // Process assertions
    EXPECT_EQ(userElement.process.pid, 1234);

    // Login assertions
    EXPECT_TRUE(userElement.login.status);
    EXPECT_EQ(userElement.login.tty, "pts/0");
    EXPECT_EQ(userElement.login.type, "interactive");
}

// Test case for parsing JSON with missing optional fields
TEST_F(UserElementTest, FromJson_MissingOptionalFields) {
    cJSON* root = cJSON_CreateObject();
    cJSON* user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "name", "minimaluser");
    cJSON_AddNumberToObject(user, "uid_signed", 1002);
    cJSON_AddItemToObject(root, "user", user);
    jsonData.reset(root);

    Wazuh::Inventory::Harvester::UserElement<void> userElement;
    userElement.fromJson(jsonData.get());

    EXPECT_EQ(userElement.user.name, "minimaluser");
    EXPECT_EQ(userElement.user.uid_signed, 1002);
    EXPECT_TRUE(userElement.user.home.empty()); // Optional fields should be empty or default
    EXPECT_EQ(userElement.user.auth_failures.count, 0); // Default
}

// Test case for parsing a null JSON
TEST_F(UserElementTest, FromJson_NullJson) {
    Wazuh::Inventory::Harvester::UserElement<void> userElement;
    userElement.fromJson(nullptr);
    // Expect default values (or no change from initial state)
    EXPECT_TRUE(userElement.user.name.empty());
    EXPECT_EQ(userElement.user.uid_signed, 0);
}

TEST_F(UserElementTest, ToJson_Serialization) {
    Wazuh::Inventory::Harvester::UserElement<void> userElement;

    // Populate userElement with data (example)
    userElement.user.name = "serializeUser";
    userElement.user.uid_signed = 2001;
    userElement.user.id = "serializeUser_uid";
    userElement.user.uuid = "serialize-uuid-456";
    userElement.user.home = "/home/serialize";
    userElement.user.shell = "/bin/zsh";
    userElement.user.type = "admin";
    userElement.user.is_hidden = true;
    userElement.user.is_remote = true;
    userElement.user.created = "2023-02-01T00:00:00Z";
    userElement.user.last_login = "2023-02-10T00:00:00Z";
    userElement.user.full_name = "Serialize User Full";


    userElement.user.auth_failures.count = 3;
    userElement.user.auth_failures.timestamp = "2023-02-09T00:00:00Z";

    userElement.user.group.id = "200";
    userElement.user.group.id_signed = 200;

    userElement.user.groups.push_back("dev");
    userElement.user.groups.push_back("staff");

    userElement.user.password.status = "disabled";
    userElement.user.password.expiration_date = "2024-02-01T00:00:00Z";
    userElement.user.password.hash_algorithm = "md5";
    userElement.user.password.inactive_days = 0;
    userElement.user.password.last_change = 1672000000;
    userElement.user.password.last_set_time = "2022-11-01T00:00:00Z";
    userElement.user.password.max_days_between_changes = 180;
    userElement.user.password.min_days_between_changes = 0;
    userElement.user.password.warning_days_before_expiration = 14;

    userElement.user.roles.push_back("editor");

    userElement.host.hostname = "serializeHost";
    userElement.host.ip.push_back("10.0.0.1");

    userElement.process.pid = 5678;

    userElement.login.status = false;
    userElement.login.tty = "tty1";
    userElement.login.type = "console";

    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> generatedJson(userElement.toJson(), cJSON_Delete);
    ASSERT_NE(generatedJson.get(), nullptr);

    // Assertions for user object
    cJSON* user_json = cJSON_GetObjectItemCaseSensitive(generatedJson.get(), "user");
    ASSERT_NE(user_json, nullptr);
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(user_json, "name")->valuestring, "serializeUser");
    EXPECT_EQ(cJSON_GetObjectItemCaseSensitive(user_json, "uid_signed")->valuedouble, 2001); // valuedouble for numbers from cJSON_AddNumberToObject
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(user_json, "id")->valuestring, "serializeUser_uid");
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(user_json, "shell")->valuestring, "/bin/zsh");
    EXPECT_TRUE(cJSON_GetObjectItemCaseSensitive(user_json, "is_hidden")->valueint); // valueint for bool

    cJSON* auth_json = cJSON_GetObjectItemCaseSensitive(user_json, "auth_failures");
    ASSERT_NE(auth_json, nullptr);
    EXPECT_EQ(cJSON_GetObjectItemCaseSensitive(auth_json, "count")->valuedouble, 3);

    cJSON* user_groups_json = cJSON_GetObjectItemCaseSensitive(user_json, "groups");
    ASSERT_NE(user_groups_json, nullptr);
    ASSERT_EQ(cJSON_GetArraySize(user_groups_json), 2);
    EXPECT_STREQ(cJSON_GetArrayItem(user_groups_json, 0)->valuestring, "dev");

    // Assertions for host object
    cJSON* host_json = cJSON_GetObjectItemCaseSensitive(generatedJson.get(), "host");
    ASSERT_NE(host_json, nullptr);
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(host_json, "hostname")->valuestring, "serializeHost");

    // Assertions for process object
    cJSON* process_json = cJSON_GetObjectItemCaseSensitive(generatedJson.get(), "process");
    ASSERT_NE(process_json, nullptr);
    EXPECT_EQ(cJSON_GetObjectItemCaseSensitive(process_json, "pid")->valuedouble, 5678);

    // Assertions for login object
    cJSON* login_json = cJSON_GetObjectItemCaseSensitive(generatedJson.get(), "login");
    ASSERT_NE(login_json, nullptr);
    EXPECT_FALSE(cJSON_GetObjectItemCaseSensitive(login_json, "status")->valueint);
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(login_json, "tty")->valuestring, "tty1");

    // Add more assertions for other fields as needed...
}
