#include "gtest/gtest.h"
#include "wazuh_modules/inventory_harvester/src/systemInventory/elements/groupElement.hpp" // Adjust path
#include "utils/jsonIO.hpp"
#include <string>
#include <vector>
#include <memory>

// Test fixture for GroupElement tests
class GroupElementTest : public ::testing::Test {
protected:
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> jsonData = {nullptr, cJSON_Delete};

    void SetUp() override {}
    void TearDown() override {
        jsonData.reset();
    }

    cJSON* createValidGroupJson() {
        cJSON* root = cJSON_CreateObject();

        cJSON* group = cJSON_CreateObject();
        cJSON_AddStringToObject(group, "name", "testgroup");
        cJSON_AddStringToObject(group, "id", "2001");
        cJSON_AddNumberToObject(group, "id_signed", 2001);
        cJSON_AddStringToObject(group, "uuid", "group-uuid-456");
        cJSON_AddStringToObject(group, "description", "A test group");
        cJSON_AddBoolToObject(group, "is_hidden", false);

        cJSON* users = cJSON_CreateArray();
        cJSON_AddItemToArray(users, cJSON_CreateString("user1"));
        cJSON_AddItemToArray(users, cJSON_CreateString("user2"));
        cJSON_AddItemToObject(group, "users", users);
        cJSON_AddItemToObject(root, "group", group);

        cJSON* host = cJSON_CreateObject();
        cJSON* ips = cJSON_CreateArray();
        cJSON_AddItemToArray(ips, cJSON_CreateString("192.168.1.11"));
        cJSON_AddItemToObject(host, "ip", ips);
        cJSON_AddStringToObject(host, "hostname", "grouphost");
        cJSON_AddItemToObject(root, "host", host);

        return root;
    }
};

// Test case for parsing a valid JSON
TEST_F(GroupElementTest, FromJson_ValidData) {
    jsonData.reset(createValidGroupJson());
    ASSERT_NE(jsonData.get(), nullptr);

    Wazuh::Inventory::Harvester::GroupElement<void> groupElement; // Use void for TContext if not used by fromJson
    groupElement.fromJson(jsonData.get());

    // Group assertions
    EXPECT_EQ(groupElement.group.name, "testgroup");
    EXPECT_EQ(groupElement.group.id, "2001");
    EXPECT_EQ(groupElement.group.id_signed, 2001);
    EXPECT_EQ(groupElement.group.uuid, "group-uuid-456");
    EXPECT_EQ(groupElement.group.description, "A test group");
    EXPECT_FALSE(groupElement.group.is_hidden);
    ASSERT_EQ(groupElement.group.users.size(), 2);
    EXPECT_EQ(groupElement.group.users[0], "user1");

    // Host assertions
    EXPECT_EQ(groupElement.host.hostname, "grouphost");
    ASSERT_EQ(groupElement.host.ip.size(), 1);
    EXPECT_EQ(groupElement.host.ip[0], "192.168.1.11");
}

// Test case for parsing JSON with missing optional fields
TEST_F(GroupElementTest, FromJson_MissingOptionalFields) {
    cJSON* root = cJSON_CreateObject();
    cJSON* group = cJSON_CreateObject();
    cJSON_AddStringToObject(group, "name", "minimalgroup");
    cJSON_AddNumberToObject(group, "id_signed", 2002);
    cJSON_AddItemToObject(root, "group", group);
    jsonData.reset(root);

    Wazuh::Inventory::Harvester::GroupElement<void> groupElement;
    groupElement.fromJson(jsonData.get());

    EXPECT_EQ(groupElement.group.name, "minimalgroup");
    EXPECT_EQ(groupElement.group.id_signed, 2002);
    EXPECT_TRUE(groupElement.group.description.empty());
    EXPECT_TRUE(groupElement.group.users.empty());
}

// Test case for parsing a null JSON
TEST_F(GroupElementTest, FromJson_NullJson) {
    Wazuh::Inventory::Harvester::GroupElement<void> groupElement;
    groupElement.fromJson(nullptr);

    EXPECT_TRUE(groupElement.group.name.empty());
    EXPECT_EQ(groupElement.group.id_signed, 0);
}

TEST_F(GroupElementTest, ToJson_Serialization) {
    Wazuh::Inventory::Harvester::GroupElement<void> groupElement;

    // Populate groupElement with data
    groupElement.group.name = "serializeGroup";
    groupElement.group.id = "3001";
    groupElement.group.id_signed = 3001;
    groupElement.group.uuid = "serialize-uuid-group-789";
    groupElement.group.description = "Serialized Test Group";
    groupElement.group.is_hidden = true;
    groupElement.group.users.push_back("s_user1");
    groupElement.group.users.push_back("s_user2");

    groupElement.host.hostname = "serializeGroupHost";
    groupElement.host.ip.push_back("10.0.0.2");

    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> generatedJson(groupElement.toJson(), cJSON_Delete);
    ASSERT_NE(generatedJson.get(), nullptr);

    // Assertions for group object
    cJSON* group_json = cJSON_GetObjectItemCaseSensitive(generatedJson.get(), "group");
    ASSERT_NE(group_json, nullptr);
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(group_json, "name")->valuestring, "serializeGroup");
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(group_json, "id")->valuestring, "3001");
    EXPECT_EQ(cJSON_GetObjectItemCaseSensitive(group_json, "id_signed")->valuedouble, 3001); // Use valuedouble for numbers from cJSON_AddNumberToObject
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(group_json, "description")->valuestring, "Serialized Test Group");
    EXPECT_TRUE(cJSON_GetObjectItemCaseSensitive(group_json, "is_hidden")->valueint); // valueint for bool

    cJSON* users_json = cJSON_GetObjectItemCaseSensitive(group_json, "users");
    ASSERT_NE(users_json, nullptr);
    ASSERT_EQ(cJSON_GetArraySize(users_json), 2);
    EXPECT_STREQ(cJSON_GetArrayItem(users_json, 0)->valuestring, "s_user1");
    EXPECT_STREQ(cJSON_GetArrayItem(users_json, 1)->valuestring, "s_user2");

    // Assertions for host object
    cJSON* host_json = cJSON_GetObjectItemCaseSensitive(generatedJson.get(), "host");
    ASSERT_NE(host_json, nullptr);
    EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(host_json, "hostname")->valuestring, "serializeGroupHost");
    cJSON* ip_json = cJSON_GetObjectItemCaseSensitive(host_json, "ip");
    ASSERT_NE(ip_json, nullptr);
    ASSERT_EQ(cJSON_GetArraySize(ip_json), 1);
    EXPECT_STREQ(cJSON_GetArrayItem(ip_json, 0)->valuestring, "10.0.0.2");
}
