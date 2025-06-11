#include <gtest/gtest.h>
#include "systemInventory/elements/userElement.hpp"
#include "systemInventory/elements/groupElement.hpp"
#include "wcsModel/inventoryUserHarvester.hpp"
#include "wcsModel/inventoryGroupHarvester.hpp"

// Test fixture for Inventory User and Group tests
class InventoryUserGroupTest : public ::testing::Test {
protected:
    // Add any setup code here if needed
};

// Test case for UserElement instantiation
TEST_F(InventoryUserGroupTest, UserElementInstantiation) {
    wazuh::inventory::harvester::UserElement userElement;
    // Add assertions here if needed, e.g., check default values
    SUCCEED(); // Placeholder for actual assertions
}

// Test case for GroupElement instantiation
TEST_F(InventoryUserGroupTest, GroupElementInstantiation) {
    wazuh::inventory::harvester::GroupElement groupElement;
    // Add assertions here if needed
    SUCCEED(); // Placeholder for actual assertions
}

// Test case for InventoryUserHarvester instantiation and getType
TEST_F(InventoryUserGroupTest, InventoryUserHarvesterInstantiationAndGetType) {
    wazuh::inventory::harvester::InventoryUserHarvester userHarvester;
    ASSERT_EQ(userHarvester.getType(), "user");
}

// Test case for InventoryGroupHarvester instantiation and getType
TEST_F(InventoryUserGroupTest, InventoryGroupHarvesterInstantiationAndGetType) {
    wazuh::inventory::harvester::InventoryGroupHarvester groupHarvester;
    ASSERT_EQ(groupHarvester.getType(), "group");
}

// Main function to run the tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
