#include "gtest/gtest.h"
#include "systemInventory/systemContext.hpp"
#include "flatbuffers/include/syscollector_deltas_generated.h"
#include "flatbuffers/include/rsync_generated.h"
#include "flatbuffers/flatbuffers.h"
#include <json.hpp>
#include <string>
#include <vector>
#include "systemInventory/elements/userElement.hpp"   // Added
#include "systemInventory/elements/groupElement.hpp" // Added
#include "wcsModel/inventoryUserHarvester.hpp"
#include "wcsModel/inventoryGroupHarvester.hpp"
#include "wcsModel/data.hpp"      // Added for DataHarvester
#include "wcsModel/noData.hpp"    // Added for NoDataHarvester
#include "timeHelper.h"           // For Utils::getCurrentTimeISO8601 in User/Group Element
#include "policyHarvesterManager.hpp" // For PolicyHarvesterManager::instance()

// --- Mock Data Structures ---
struct MockUserData {
    std::string item_id = "user-item-001";
    std::string name = "testuser";
    std::string uid = "1001";
    std::string gid = "1001";
    std::string home = "/home/testuser";
    std::string shell = "/bin/bash";
    std::string uuid = "user-uuid-123";
    std::string full_name = "Test User Full Name";
    bool is_hidden = false;
    bool is_remote = false;
    std::string password_hash_algorithm = "sha512";
    long password_last_change = 1672531200; // Example timestamp
    int password_max_days_between_changes = 90;
    int password_min_days_between_changes = 1;
    int password_warning_days_before_expiration = 7;
    std::string password_expiration_date = "2025-03-31T00:00:00Z";
    std::string password_status = "active";
    std::string password_last_set_time = "2024-01-01T10:00:00Z";
    int password_inactive_days = 30;
    std::string created = "2023-01-01T12:00:00Z";
    std::string last_login = "2024-02-01T10:30:00Z";
    std::string roles = "admin,user";
    std::string groups = "users,adm";
    int auth_failures_count = 2;
    std::string auth_failures_timestamp = "2024-02-01T09:00:00Z";
    bool login_status = true;
    std::string login_type = "interactive";
    std::string login_tty = "pts/0";
};

struct MockGroupData {
    std::string item_id = "group-item-001";
    std::string name = "testgroup";
    std::string gid = "2002";
    std::string description = "Test Group Description";
    std::string uuid = "group-uuid-456";
    bool is_hidden = false;
    std::string users = "user1,user2,testuser";
};


// Updated helper function to create mock User data for Delta messages
flatbuffers::Offset<SyscollectorDeltas::DbsyncUsers>
CreateMockDbsyncUsers(flatbuffers::FlatBufferBuilder& builder, const MockUserData& data)
{
    auto fb_item_id = builder.CreateString(data.item_id);
    auto fb_name = builder.CreateString(data.name);
    auto fb_uid = builder.CreateString(data.uid);
    auto fb_gid = builder.CreateString(data.gid);
    auto fb_home = builder.CreateString(data.home);
    auto fb_shell = builder.CreateString(data.shell);
    auto fb_uuid = builder.CreateString(data.uuid);
    auto fb_full_name = builder.CreateString(data.full_name);
    auto fb_password_hash_algorithm = builder.CreateString(data.password_hash_algorithm);
    auto fb_password_expiration_date = builder.CreateString(data.password_expiration_date);
    auto fb_password_status = builder.CreateString(data.password_status);
    auto fb_password_last_set_time = builder.CreateString(data.password_last_set_time);
    auto fb_created = builder.CreateString(data.created);
    auto fb_last_login = builder.CreateString(data.last_login);
    auto fb_roles = builder.CreateString(data.roles);
    auto fb_groups = builder.CreateString(data.groups);
    auto fb_auth_failures_timestamp = builder.CreateString(data.auth_failures_timestamp);
    auto fb_login_type = builder.CreateString(data.login_type);
    auto fb_login_tty = builder.CreateString(data.login_tty);

    return SyscollectorDeltas::CreateDbsyncUsers(builder, fb_item_id, fb_name, fb_uid, fb_gid, fb_home, fb_shell,
                                                 fb_uuid, fb_full_name, data.is_hidden, data.is_remote,
                                                 fb_password_hash_algorithm, data.password_last_change,
                                                 data.password_max_days_between_changes,
                                                 data.password_min_days_between_changes,
                                                 data.password_warning_days_before_expiration,
                                                 fb_password_expiration_date, fb_password_status,
                                                 fb_password_last_set_time, data.password_inactive_days,
                                                 fb_created, fb_last_login, fb_roles, fb_groups,
                                                 data.auth_failures_count, fb_auth_failures_timestamp,
                                                 data.login_status, fb_login_type, fb_login_tty);
}

// Updated helper function to create mock Group data for Delta messages
flatbuffers::Offset<SyscollectorDeltas::DbsyncGroups>
CreateMockDbsyncGroups(flatbuffers::FlatBufferBuilder& builder, const MockGroupData& data)
{
    auto fb_item_id = builder.CreateString(data.item_id);
    auto fb_name = builder.CreateString(data.name);
    auto fb_gid = builder.CreateString(data.gid);
    auto fb_description = builder.CreateString(data.description);
    auto fb_uuid = builder.CreateString(data.uuid);
    auto fb_users = builder.CreateString(data.users);

    return SyscollectorDeltas::CreateDbsyncGroups(builder, fb_item_id, fb_name, fb_gid, fb_description, fb_uuid,
                                                  data.is_hidden, fb_users);
}

// Updated helper function to create mock User data for Sync (State) messages
flatbuffers::Offset<Synchronization::SyscollectorUsers>
CreateMockSyncUsers(flatbuffers::FlatBufferBuilder& builder, const MockUserData& data)
{
    auto fb_item_id = builder.CreateString(data.item_id);
    auto fb_name = builder.CreateString(data.name);
    auto fb_uid = builder.CreateString(data.uid);
    auto fb_gid = builder.CreateString(data.gid);
    auto fb_home = builder.CreateString(data.home);
    auto fb_shell = builder.CreateString(data.shell);
    auto fb_uuid = builder.CreateString(data.uuid);
    auto fb_full_name = builder.CreateString(data.full_name);
    auto fb_password_hash_algorithm = builder.CreateString(data.password_hash_algorithm);
    auto fb_password_expiration_date = builder.CreateString(data.password_expiration_date);
    auto fb_password_status = builder.CreateString(data.password_status);
    auto fb_password_last_set_time = builder.CreateString(data.password_last_set_time);
    auto fb_created = builder.CreateString(data.created);
    auto fb_last_login = builder.CreateString(data.last_login);
    auto fb_roles = builder.CreateString(data.roles);
    auto fb_groups = builder.CreateString(data.groups);
    auto fb_auth_failures_timestamp = builder.CreateString(data.auth_failures_timestamp);
    auto fb_login_type = builder.CreateString(data.login_type);
    auto fb_login_tty = builder.CreateString(data.login_tty);

    return Synchronization::CreateSyscollectorUsers(builder, fb_item_id, fb_name, fb_uid, fb_gid, fb_home, fb_shell,
                                                    fb_uuid, fb_full_name, data.is_hidden, data.is_remote,
                                                    fb_password_hash_algorithm, data.password_last_change,
                                                    data.password_max_days_between_changes,
                                                    data.password_min_days_between_changes,
                                                    data.password_warning_days_before_expiration,
                                                    fb_password_expiration_date, fb_password_status,
                                                    fb_password_last_set_time, data.password_inactive_days,
                                                    fb_created, fb_last_login, fb_roles, fb_groups,
                                                    data.auth_failures_count, fb_auth_failures_timestamp,
                                                    data.login_status, fb_login_type, fb_login_tty);
}

// Updated helper function to create mock Group data for Sync (State) messages
flatbuffers::Offset<Synchronization::SyscollectorGroups>
CreateMockSyncGroups(flatbuffers::FlatBufferBuilder& builder, const MockGroupData& data)
{
    auto fb_item_id = builder.CreateString(data.item_id);
    auto fb_name = builder.CreateString(data.name);
    auto fb_gid = builder.CreateString(data.gid);
    auto fb_description = builder.CreateString(data.description);
    auto fb_uuid = builder.CreateString(data.uuid);
    auto fb_users = builder.CreateString(data.users);

    return Synchronization::CreateSyscollectorGroups(builder, fb_item_id, fb_name, fb_gid, fb_description, fb_uuid,
                                                     data.is_hidden, fb_users);
}


TEST(SystemContextUserGroupTest, BuildDeltaContext_UserInsert)
{
    flatbuffers::FlatBufferBuilder builder;
    auto operation = builder.CreateString("INSERTED");
    // Assuming AgentInfo is needed by SystemContext general fields, mock it simply
    auto agent_id_str = "001";
    auto agent_name_str = "test-agent";
    auto agent_ip_str = "127.0.0.1";
    auto agent_version_str = "W_AGENT_VERSION";

    flatbuffers::FlatBufferBuilder builder;
    auto operation = builder.CreateString("INSERTED");
    auto agent_id = builder.CreateString(agent_id_str);
    auto agent_name = builder.CreateString(agent_name_str);
    auto agent_ip = builder.CreateString(agent_ip_str);
    auto agent_version = builder.CreateString(agent_version_str);
    auto agent_info = SyscollectorDeltas::CreateAgentInfo(builder, agent_id, agent_name, agent_ip, agent_version);

    MockUserData mock_user_data; // Using default values from struct
    auto user_data_fb = CreateMockDbsyncUsers(builder, mock_user_data);

    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info, SyscollectorDeltas::Provider_dbsync_users, user_data_fb.Union());
    builder.Finish(delta_offset);

    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    SystemContext context(delta_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Upsert);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::User);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Users);
    ASSERT_EQ(context.userName(), mock_user_data.name);
    ASSERT_EQ(context.userId(), mock_user_data.uid);
    ASSERT_EQ(context.userGroupId(), mock_user_data.gid);
    ASSERT_EQ(context.userHome(), mock_user_data.home);
    ASSERT_EQ(context.userShell(), mock_user_data.shell);
    ASSERT_EQ(context.userUuid(), mock_user_data.uuid);
    ASSERT_EQ(context.userFullName(), mock_user_data.full_name);
    ASSERT_EQ(context.userIsHidden(), mock_user_data.is_hidden);
    ASSERT_EQ(context.userIsRemote(), mock_user_data.is_remote);
    ASSERT_EQ(context.userItemId(), mock_user_data.item_id);
    // Test a few more detailed fields
    ASSERT_EQ(context.userPasswordStatus(), mock_user_data.password_status);
    ASSERT_EQ(context.userCreated(), mock_user_data.created);
    ASSERT_EQ(context.userRoles(), mock_user_data.roles);
}

TEST(SystemContextUserGroupTest, BuildDeltaContext_GroupInsert)
{
    flatbuffers::FlatBufferBuilder builder;
    auto operation = builder.CreateString("INSERTED");
    auto agent_id_str = "001";
    auto agent_name_str = "test-agent";
    auto agent_ip_str = "127.0.0.1";
    auto agent_version_str = "W_AGENT_VERSION";
    auto agent_id = builder.CreateString(agent_id_str);
    auto agent_name = builder.CreateString(agent_name_str);
    auto agent_ip = builder.CreateString(agent_ip_str);
    auto agent_version = builder.CreateString(agent_version_str);
    auto agent_info = SyscollectorDeltas::CreateAgentInfo(builder, agent_id, agent_name, agent_ip, agent_version);

    MockGroupData mock_group_data;
    auto group_data_fb = CreateMockDbsyncGroups(builder, mock_group_data);

    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info, SyscollectorDeltas::Provider_dbsync_groups, group_data_fb.Union());
    builder.Finish(delta_offset);

    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    SystemContext context(delta_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Upsert);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::Group);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Groups);
    ASSERT_EQ(context.groupName(), mock_group_data.name);
    ASSERT_EQ(context.groupId(), mock_group_data.gid);
    ASSERT_EQ(context.groupDescription(), mock_group_data.description);
    ASSERT_EQ(context.groupUuid(), mock_group_data.uuid);
    ASSERT_EQ(context.groupIsHidden(), mock_group_data.is_hidden);
    ASSERT_EQ(context.groupUsers(), mock_group_data.users);
    ASSERT_EQ(context.groupItemId(), mock_group_data.item_id);
}

TEST(SystemContextUserGroupTest, BuildDeltaContext_UserDelete)
{
    flatbuffers::FlatBufferBuilder builder;
    auto operation = builder.CreateString("DELETED");
    auto agent_id_str = "001";
    auto agent_name_str = "test-agent";
    auto agent_ip_str = "127.0.0.1";
    auto agent_version_str = "W_AGENT_VERSION";
    auto agent_id = builder.CreateString(agent_id_str);
    auto agent_name = builder.CreateString(agent_name_str);
    auto agent_ip = builder.CreateString(agent_ip_str);
    auto agent_version = builder.CreateString(agent_version_str);
    auto agent_info = SyscollectorDeltas::CreateAgentInfo(builder, agent_id, agent_name, agent_ip, agent_version);

    MockUserData mock_user_data_for_delete; // item_id is the key part for delete context
    mock_user_data_for_delete.name = ""; // Other fields might be empty on delete

    auto user_data_fb = CreateMockDbsyncUsers(builder, mock_user_data_for_delete);

    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info, SyscollectorDeltas::Provider_dbsync_users, user_data_fb.Union());
    builder.Finish(delta_offset);
    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    SystemContext context(delta_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Delete);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::User);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Users);
    ASSERT_EQ(context.userItemId(), mock_user_data_for_delete.item_id);
}


TEST(SystemContextUserGroupTest, BuildSyncContext_UserState)
{
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id_str = "002";
    auto agent_name_str = "sync-agent";
    auto agent_ip_str = "192.168.1.100";
    auto agent_version_str = "W_AGENT_SYNC_VERSION";
    auto agent_id = builder.CreateString(agent_id_str);
    auto agent_name = builder.CreateString(agent_name_str);
    auto agent_ip = builder.CreateString(agent_ip_str);
    auto agent_version = builder.CreateString(agent_version_str);
    auto agent_info = Synchronization::CreateAgentInfo(builder, agent_id, agent_name, agent_ip, agent_version);

    MockUserData mock_user_data;
    auto user_attrs_fb = CreateMockSyncUsers(builder, mock_user_data);

    auto state_msg = Synchronization::CreateState(builder, agent_info, Synchronization::AttributesUnion_syscollector_users, user_attrs_fb.Union());
    auto sync_msg_offset = Synchronization::CreateSyncMsg(builder, Synchronization::DataUnion_state, state_msg.Union());
    builder.Finish(sync_msg_offset);

    auto sync_ptr = Synchronization::GetSyncMsg(builder.GetBufferPointer());
    SystemContext context(sync_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Upsert);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::User);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Users);
    ASSERT_EQ(context.userName(), mock_user_data.name);
    ASSERT_EQ(context.userId(), mock_user_data.uid);
    ASSERT_EQ(context.userItemId(), mock_user_data.item_id);
}

TEST(SystemContextUserGroupTest, BuildSyncContext_GroupState)
{
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id_str = "002";
    auto agent_name_str = "sync-agent";
    auto agent_ip_str = "192.168.1.100";
    auto agent_version_str = "W_AGENT_SYNC_VERSION";
    auto agent_id = builder.CreateString(agent_id_str);
    auto agent_name = builder.CreateString(agent_name_str);
    auto agent_ip = builder.CreateString(agent_ip_str);
    auto agent_version = builder.CreateString(agent_version_str);
    auto agent_info = Synchronization::CreateAgentInfo(builder, agent_id, agent_name, agent_ip, agent_version);

    MockGroupData mock_group_data;
    auto group_attrs_fb = CreateMockSyncGroups(builder, mock_group_data);

    auto state_msg = Synchronization::CreateState(builder, agent_info, Synchronization::AttributesUnion_syscollector_groups, group_attrs_fb.Union());
    auto sync_msg_offset = Synchronization::CreateSyncMsg(builder, Synchronization::DataUnion_state, state_msg.Union());
    builder.Finish(sync_msg_offset);

    auto sync_ptr = Synchronization::GetSyncMsg(builder.GetBufferPointer());
    SystemContext context(sync_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Upsert);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::Group);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Groups);
    ASSERT_EQ(context.groupName(), mock_group_data.name);
    ASSERT_EQ(context.groupItemId(), mock_group_data.item_id);
}


TEST(SystemContextUserGroupTest, BuildSyncContext_UserIntegrityClear)
{
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id = builder.CreateString("003");
    auto agent_info = Synchronization::CreateAgentInfo(builder, agent_id);
    auto attributes_type_str = builder.CreateString("syscollector_users");
    auto integrity_clear_msg = Synchronization::CreateIntegrityClear(builder, agent_info, attributes_type_str);
    auto sync_msg_offset = Synchronization::CreateSyncMsg(builder, Synchronization::DataUnion_integrity_clear, integrity_clear_msg.Union());
    builder.Finish(sync_msg_offset);

    auto sync_ptr = Synchronization::GetSyncMsg(builder.GetBufferPointer());
    SystemContext context(sync_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::DeleteAllEntries);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::User);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Users);
}

TEST(SystemContextUserGroupTest, BuildSyncContext_GroupIntegrityClear)
{
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id = builder.CreateString("003");
    auto agent_info = Synchronization::CreateAgentInfo(builder, agent_id);
    auto attributes_type_str = builder.CreateString("syscollector_groups");
    auto integrity_clear_msg = Synchronization::CreateIntegrityClear(builder, agent_info, attributes_type_str);
    auto sync_msg_offset = Synchronization::CreateSyncMsg(builder, Synchronization::DataUnion_integrity_clear, integrity_clear_msg.Union());
    builder.Finish(sync_msg_offset);

    auto sync_ptr = Synchronization::GetSyncMsg(builder.GetBufferPointer());
    SystemContext context(sync_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::DeleteAllEntries);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::Group);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Groups);
}

TEST(SystemContextUserGroupTest, BuildSyncContext_UserIntegrityCheckGlobal)
{
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id = builder.CreateString("004");
    auto agent_info = Synchronization::CreateAgentInfo(builder, agent_id);
    auto attributes_type_str = builder.CreateString("syscollector_users");
    auto integrity_check_msg = Synchronization::CreateIntegrityCheckGlobal(builder, agent_info, attributes_type_str);
    auto sync_msg_offset = Synchronization::CreateSyncMsg(builder, Synchronization::DataUnion_integrity_check_global, integrity_check_msg.Union());
    builder.Finish(sync_msg_offset);

    auto sync_ptr = Synchronization::GetSyncMsg(builder.GetBufferPointer());
    SystemContext context(sync_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::IndexSync);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::User);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Users);
}

TEST(SystemContextUserGroupTest, BuildSyncContext_GroupIntegrityCheckGlobal)
{
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id = builder.CreateString("004");
    auto agent_info = Synchronization::CreateAgentInfo(builder, agent_id);
    auto attributes_type_str = builder.CreateString("syscollector_groups");
    auto integrity_check_msg = Synchronization::CreateIntegrityCheckGlobal(builder, agent_info, attributes_type_str);
    auto sync_msg_offset = Synchronization::CreateSyncMsg(builder, Synchronization::DataUnion_integrity_check_global, integrity_check_msg.Union());
    builder.Finish(sync_msg_offset);

    auto sync_ptr = Synchronization::GetSyncMsg(builder.GetBufferPointer());
    SystemContext context(sync_ptr);

    ASSERT_EQ(context.operation(), SystemContext::Operation::IndexSync);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::Group);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Groups);
}

TEST(SystemContextUserGroupTest, BuildJsonContext_DeleteUser)
{
    nlohmann::json j;
    j["action"] = "deleteUser";
    j["agent_info"]["agent_id"] = "005";
    j["data"]["item_id"] = "json-user-uid-to-delete"; // Assuming item_id is used for deletion

    SystemContext context(&j);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Delete);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::User);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Users);
    ASSERT_EQ(context.userItemId(), "json-user-uid-to-delete");
}

TEST(SystemContextUserGroupTest, BuildJsonContext_DeleteGroup)
{
    nlohmann::json j;
    j["action"] = "deleteGroup";
    j["agent_info"]["agent_id"] = "005";
    j["data"]["item_id"] = "json-group-gid-to-delete"; // Assuming item_id is used for deletion

    SystemContext context(&j);

    ASSERT_EQ(context.operation(), SystemContext::Operation::Delete);
    ASSERT_EQ(context.affectedComponentType(), SystemContext::AffectedComponentType::Group);
    ASSERT_EQ(context.originTable(), SystemContext::OriginTable::Groups);
    ASSERT_EQ(context.groupItemId(), "json-group-gid-to-delete");
}


// --- Tests for UserElement and GroupElement ---

TEST(UserElementTest, CollectsUserDataCorrectly_Delta) {
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id_str = "agent007";
    auto agent_name_str = "james-bond";
    auto agent_ip_str = "10.0.0.7";
    auto agent_version_str = "W_AGENT_007";

    auto operation = builder.CreateString("INSERTED");
    auto agent_id_fb = builder.CreateString(agent_id_str);
    auto agent_name_fb = builder.CreateString(agent_name_str);
    auto agent_ip_fb = builder.CreateString(agent_ip_str);
    auto agent_version_fb = builder.CreateString(agent_version_str);
    auto agent_info_fb = SyscollectorDeltas::CreateAgentInfo(builder, agent_id_fb, agent_name_fb, agent_ip_fb, agent_version_fb);

    MockUserData mock_user_data; // Use default mock data
    auto user_data_fb = CreateMockDbsyncUsers(builder, mock_user_data);

    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info_fb, SyscollectorDeltas::Provider_dbsync_users, user_data_fb.Union());
    builder.Finish(delta_offset);

    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    auto system_context = std::make_shared<SystemContext>(delta_ptr);

    DataHarvester<InventoryUserHarvester> result = UserElement<SystemContext>::build(system_context.get());

    std::string expected_doc_id = agent_id_str + "_" + mock_user_data.item_id;
    ASSERT_EQ(result.id, expected_doc_id);
    ASSERT_EQ(result.operation, "INSERTED");

    // Agent info
    ASSERT_EQ(result.data.agent.id, agent_id_str);
    ASSERT_EQ(result.data.agent.name, agent_name_str);
    ASSERT_EQ(result.data.agent.ip, agent_ip_str);
    ASSERT_EQ(result.data.agent.version, agent_version_str);

    // Wazuh cluster info (assuming PolicyHarvesterManager is default initialized for tests or mocked)
    // These might be empty if PolicyHarvesterManager isn't fully set up in test env.
    // For now, just check they exist or match default if known.
    // ASSERT_FALSE(result.data.wazuh.cluster.name.empty());

    // Host info
    ASSERT_EQ(result.data.host.ip, agent_ip_str);

    // User info
    ASSERT_EQ(result.data.user.name, mock_user_data.name);
    ASSERT_EQ(result.data.user.id, mock_user_data.uid);
    ASSERT_EQ(result.data.user.uid_signed, std::stol(mock_user_data.uid));
    ASSERT_EQ(result.data.user.group.id_signed, std::stol(mock_user_data.gid));
    ASSERT_EQ(result.data.user.group.id, static_cast<unsigned long>(std::stoul(mock_user_data.gid)));
    ASSERT_EQ(result.data.user.home, mock_user_data.home);
    ASSERT_EQ(result.data.user.shell, mock_user_data.shell);
    ASSERT_EQ(result.data.user.uuid, mock_user_data.uuid);
    ASSERT_EQ(result.data.user.full_name, mock_user_data.full_name);
    ASSERT_EQ(result.data.user.is_hidden, mock_user_data.is_hidden);
    ASSERT_EQ(result.data.user.is_remote, mock_user_data.is_remote);
    ASSERT_EQ(result.data.user.created, mock_user_data.created);
    ASSERT_EQ(result.data.user.last_login, mock_user_data.last_login);
    ASSERT_EQ(result.data.user.type, ""); // Not mapped

    // Password
    ASSERT_EQ(result.data.user.password.status, mock_user_data.password_status);
    ASSERT_EQ(result.data.user.password.last_change, mock_user_data.password_last_change);
    ASSERT_EQ(result.data.user.password.expiration_date, mock_user_data.password_expiration_date);
    ASSERT_EQ(result.data.user.password.hash_algorithm, mock_user_data.password_hash_algorithm);
    ASSERT_EQ(result.data.user.password.inactive_days, mock_user_data.password_inactive_days);
    ASSERT_EQ(result.data.user.password.last_set_time, mock_user_data.password_last_set_time);
    ASSERT_EQ(result.data.user.password.max_days_between_changes, mock_user_data.password_max_days_between_changes);
    ASSERT_EQ(result.data.user.password.min_days_between_changes, mock_user_data.password_min_days_between_changes);
    ASSERT_EQ(result.data.user.password.warning_days_before_expiration, mock_user_data.password_warning_days_before_expiration);

    std::vector<std::string> expected_roles;
    Utils::splitView(mock_user_data.roles, ',', expected_roles);
    ASSERT_EQ(result.data.user.roles, expected_roles);

    std::vector<std::string> expected_groups;
    Utils::splitView(mock_user_data.groups, ',', expected_groups);
    ASSERT_EQ(result.data.user.groups, expected_groups);

    // Auth Failures
    ASSERT_EQ(result.data.user.auth_failures.count, mock_user_data.auth_failures_count);
    ASSERT_EQ(result.data.user.auth_failures.timestamp, mock_user_data.auth_failures_timestamp);

    // Login
    ASSERT_EQ(result.data.login.status, mock_user_data.login_status);
    ASSERT_EQ(result.data.login.type, mock_user_data.login_type);
    ASSERT_EQ(result.data.login.tty, mock_user_data.login_tty);
}


TEST(GroupElementTest, CollectsGroupDataCorrectly_Delta) {
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id_str = "agent008";
    auto agent_name_str = "group-agent";
    auto agent_ip_str = "10.0.0.8";
    auto agent_version_str = "W_AGENT_008";

    auto operation = builder.CreateString("INSERTED");
    auto agent_id_fb = builder.CreateString(agent_id_str);
    auto agent_name_fb = builder.CreateString(agent_name_str);
    auto agent_ip_fb = builder.CreateString(agent_ip_str);
    auto agent_version_fb = builder.CreateString(agent_version_str);
    auto agent_info_fb = SyscollectorDeltas::CreateAgentInfo(builder, agent_id_fb, agent_name_fb, agent_ip_fb, agent_version_fb);

    MockGroupData mock_group_data; // Use default mock data
    auto group_data_fb = CreateMockDbsyncGroups(builder, mock_group_data);

    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info_fb, SyscollectorDeltas::Provider_dbsync_groups, group_data_fb.Union());
    builder.Finish(delta_offset);

    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    auto system_context = std::make_shared<SystemContext>(delta_ptr);

    DataHarvester<InventoryGroupHarvester> result = GroupElement<SystemContext>::build(system_context.get());

    std::string expected_doc_id = agent_id_str + "_" + mock_group_data.item_id;
    ASSERT_EQ(result.id, expected_doc_id);
    ASSERT_EQ(result.operation, "INSERTED");

    // Agent info
    ASSERT_EQ(result.data.agent.id, agent_id_str);
    ASSERT_EQ(result.data.agent.name, agent_name_str);
    ASSERT_EQ(result.data.agent.ip, agent_ip_str);
    ASSERT_EQ(result.data.agent.version, agent_version_str);

    // Group info
    ASSERT_EQ(result.data.group.name, mock_group_data.name);
    ASSERT_EQ(result.data.group.id_signed, std::stol(mock_group_data.gid));
    ASSERT_EQ(result.data.group.id, static_cast<unsigned long>(std::stoul(mock_group_data.gid)));
    ASSERT_EQ(result.data.group.description, mock_group_data.description);
    ASSERT_EQ(result.data.group.uuid, mock_group_data.uuid);
    ASSERT_EQ(result.data.group.is_hidden, mock_group_data.is_hidden);

    std::vector<std::string> expected_users;
    Utils::splitView(mock_group_data.users, ',', expected_users);
    ASSERT_EQ(result.data.group.users, expected_users);
}

TEST(UserElementTest, DeletesUserDataCorrectly_Delta) {
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id_str = "agent007";
    MockUserData mock_user_data; // item_id is the key part

    auto operation = builder.CreateString("DELETED"); // Context operation for delete
    auto agent_id_fb = builder.CreateString(agent_id_str);
    auto agent_info_fb = SyscollectorDeltas::CreateAgentInfo(builder, agent_id_fb); // Simpler agent info for delete

    // DbsyncUsers data for delete context might be minimal, mostly needing item_id
    auto user_data_fb = CreateMockDbsyncUsers(builder, mock_user_data);


    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info_fb, SyscollectorDeltas::Provider_dbsync_users, user_data_fb.Union());
    builder.Finish(delta_offset);

    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    auto system_context = std::make_shared<SystemContext>(delta_ptr);

    NoDataHarvester result = UserElement<SystemContext>::deleteElement(system_context.get());

    std::string expected_doc_id = agent_id_str + "_" + mock_user_data.item_id;
    ASSERT_EQ(result.id, expected_doc_id);
    ASSERT_EQ(result.operation, "DELETED");
}

TEST(GroupElementTest, DeletesGroupDataCorrectly_Delta) {
    flatbuffers::FlatBufferBuilder builder;
    auto agent_id_str = "agent008";
    MockGroupData mock_group_data; // item_id is the key part

    auto operation = builder.CreateString("DELETED"); // Context operation for delete
    auto agent_id_fb = builder.CreateString(agent_id_str);
    auto agent_info_fb = SyscollectorDeltas::CreateAgentInfo(builder, agent_id_fb);

    auto group_data_fb = CreateMockDbsyncGroups(builder, mock_group_data);

    auto delta_offset = SyscollectorDeltas::CreateDelta(
        builder, operation, agent_info_fb, SyscollectorDeltas::Provider_dbsync_groups, group_data_fb.Union());
    builder.Finish(delta_offset);

    auto delta_ptr = SyscollectorDeltas::GetDelta(builder.GetBufferPointer());
    auto system_context = std::make_shared<SystemContext>(delta_ptr);

    NoDataHarvester result = GroupElement<SystemContext>::deleteElement(system_context.get());

    std::string expected_doc_id = agent_id_str + "_" + mock_group_data.item_id;
    ASSERT_EQ(result.id, expected_doc_id);
    ASSERT_EQ(result.operation, "DELETED");
}

// Main function for GTest
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
