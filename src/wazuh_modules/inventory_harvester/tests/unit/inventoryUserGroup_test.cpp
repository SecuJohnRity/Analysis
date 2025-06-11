#include <gtest/gtest.h>
#include <gmock/gmock.h> // For GMock if more complex mocking is needed later

// Assuming these are the correct paths after refactoring
#include "systemInventory/elements/user.hpp"
#include "systemInventory/elements/group.hpp"
#include "wcsModel/inventoryUserHarvester.hpp"
#include "wcsModel/inventoryGroupHarvester.hpp"
#include "wcsModel/data.hpp" // For DataHarvester
#include "wcsModel/noData.hpp" // For NoDataHarvester

// Forward declare SystemContext or include its actual header if available and simple enough
// For now, creating a minimal mock version for testing purposes.
namespace wazuh {
namespace inventory {
namespace harvester {
// Minimal Mock SystemContext for testing User and Group elements
class MockSystemContext {
public:
    // Agent related methods - consistent with PackageElement usage
    std::string_view agentId() const { return m_agentId; }
    std::string_view agentName() const { return m_agentName; }
    std::string_view agentVersion() const { return m_agentVersion; }
    std::string_view agentIp() const { return m_agentIp; }

    // User specific data methods
    std::string_view userId() const { return m_userId; } // For ID of the user record itself
    std::string_view userUid() const { return m_userUid; }
    std::string_view userGid() const { return m_userGid; } // Primary GID of user
    std::string_view userName() const { return m_userName; }
    std::string_view userHome() const { return m_userHome; }
    std::string_view userShell() const { return m_userShell; }
    // std::vector<std::string> userGroups() const { return m_userGroups; } // If enabling groups for user

    // Group specific data methods
    std::string_view groupId() const { return m_groupId; } // For ID of the group record
    std::string_view groupGid() const { return m_groupGid; } // Actual GID of group
    std::string_view groupName() const { return m_groupName; }
    // std::vector<std::string> groupMembers() const { return m_groupMembers; } // If enabling members for group

    // --- Mock data setters ---
    void setAgentData(std::string_view id, std::string_view name, std::string_view version, std::string_view ip) {
        m_agentId = id; m_agentName = name; m_agentVersion = version; m_agentIp = ip;
    }
    void setUserData(std::string_view id, std::string_view uid, std::string_view gid, std::string_view name, std::string_view home, std::string_view shell) {
        m_userId = id; m_userUid = uid; m_userGid = gid; m_userName = name; m_userHome = home; m_userShell = shell;
    }
    void setGroupData(std::string_view id, std::string_view gid, std::string_view name) {
        m_groupId = id; m_groupGid = gid; m_groupName = name;
    }

private:
    std::string m_agentId = "001";
    std::string m_agentName = "test-agent";
    std::string m_agentVersion = "v1.0";
    std::string m_agentIp = "192.168.1.100";

    std::string m_userId = "user123";
    std::string m_userUid = "1001";
    std::string m_userGid = "1001";
    std::string m_userName = "testuser";
    std::string m_userHome = "/home/testuser";
    std::string m_userShell = "/bin/bash";
    // std::vector<std::string> m_userGroups;

    std::string m_groupId = "group456";
    std::string m_groupGid = "2002";
    std::string m_groupName = "testgroup";
    // std::vector<std::string> m_groupMembers;
};

// Need to ensure PolicyHarvesterManager can be instantiated or mocked if its instance is used.
// For simplicity, if it's only getClusterName/Status, we might not hit issues in these specific tests.
// Otherwise, a proper mock or a way to initialize it would be needed.
// For now, assuming it's accessible without complex setup for these element tests.
class PolicyHarvesterManager { // Minimal stub if needed by element code
public:
    static PolicyHarvesterManager& instance() {
        static PolicyHarvesterManager inst;
        return inst;
    }
    std::string getClusterName() const { return "test_cluster"; }
    bool getClusterStatus() const { return true; }
    std::string getClusterNodeName() const { return "node01"; }
};


} // namespace harvester
} // namespace inventory
} // namespace wazuh


// Test fixture for Inventory User and Group tests
class InventoryUserGroupRefactoredTest : public ::testing::Test {
protected:
    wazuh::inventory::harvester::MockSystemContext mockContext;

    void SetUp() override {
        // Setup default mock data
        mockContext.setAgentData("001", "TestAgent", "4.8.0", "127.0.0.1");
        mockContext.setUserData("user1001", "1001", "100", "john.doe", "/home/john.doe", "/bin/bash");
        mockContext.setGroupData("group2002", "2002", "developers");
    }
};

// Test User Element Build
TEST_F(InventoryUserGroupRefactoredTest, UserElementBuild) {
    auto userHarvesterData = wazuh::inventory::harvester::User<wazuh::inventory::harvester::MockSystemContext>::build(&mockContext);

    ASSERT_EQ(userHarvesterData.id, "001_user1001");
    ASSERT_EQ(userHarvesterData.operation, "INSERTED");
    ASSERT_EQ(userHarvesterData.data.agent.id, "001");
    ASSERT_EQ(userHarvesterData.data.agent.name, "TestAgent");
    ASSERT_EQ(userHarvesterData.data.user.uid, "1001");
    ASSERT_EQ(userHarvesterData.data.user.gid, "100");
    ASSERT_EQ(userHarvesterData.data.user.name, "john.doe");
    ASSERT_EQ(userHarvesterData.data.user.home, "/home/john.doe");
    ASSERT_EQ(userHarvesterData.data.user.shell, "/bin/bash");
    ASSERT_EQ(userHarvesterData.data.wazuh.cluster.name, "test_cluster");
}

// Test Group Element Build
TEST_F(InventoryUserGroupRefactoredTest, GroupElementBuild) {
    auto groupHarvesterData = wazuh::inventory::harvester::Group<wazuh::inventory::harvester::MockSystemContext>::build(&mockContext);

    ASSERT_EQ(groupHarvesterData.id, "001_group2002");
    ASSERT_EQ(groupHarvesterData.operation, "INSERTED");
    ASSERT_EQ(groupHarvesterData.data.agent.id, "001");
    ASSERT_EQ(groupHarvesterData.data.group.gid, "2002");
    ASSERT_EQ(groupHarvesterData.data.group.name, "developers");
    ASSERT_EQ(groupHarvesterData.data.wazuh.cluster.name, "test_cluster");
}

// Test User Element Delete
TEST_F(InventoryUserGroupRefactoredTest, UserElementDelete) {
    auto noData = wazuh::inventory::harvester::User<wazuh::inventory::harvester::MockSystemContext>::deleteElement(&mockContext);
    ASSERT_EQ(noData.id, "001_user1001");
    ASSERT_EQ(noData.operation, "DELETED");
}

// Test Group Element Delete
TEST_F(InventoryUserGroupRefactoredTest, GroupElementDelete) {
    auto noData = wazuh::inventory::harvester::Group<wazuh::inventory::harvester::MockSystemContext>::deleteElement(&mockContext);
    ASSERT_EQ(noData.id, "001_group2002");
    ASSERT_EQ(noData.operation, "DELETED");
}

// Main function to run the tests (if not already in another file linked)
// int main(int argc, char **argv) {
// ::testing::InitGoogleTest(&argc, argv);
// return RUN_ALL_TESTS();
// }
