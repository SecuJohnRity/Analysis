#include "gtest/gtest.h"
#include "wazuh_modules/inventory_harvester/src/wcsModel/inventoryGroupHarvester.hpp" // Adjust path
#include "wazuh_modules/inventory_harvester/src/systemInventory/elements/groupElement.hpp" // For parsing
#include "utils/jsonIO.hpp"
#include <memory>

class InventoryGroupHarvesterTest : public ::testing::Test {
protected:
    Wazuh::Inventory::Harvester::InventoryGroupHarvester groupHarvester;
};

TEST_F(InventoryGroupHarvesterTest, CollectData_ReturnsValidAndParsableGroupJson) {
    std::vector<cJSON*> collectedData = groupHarvester.collectData(1, "test_command", "test_options");

    ASSERT_FALSE(collectedData.empty());
    ASSERT_NE(collectedData[0], nullptr);

    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> jsonDataItem(collectedData[0], cJSON_Delete);

    char* jsonStr = cJSON_PrintUnformatted(jsonDataItem.get());
    ASSERT_NE(jsonStr, nullptr);
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> parsedJsonFromString(cJSON_Parse(jsonStr), cJSON_Delete);
    free(jsonStr);
    ASSERT_NE(parsedJsonFromString.get(), nullptr);

    Wazuh::Inventory::Harvester::GroupElement<void> groupElement; // TContext as void
    // GroupElement::fromJson expects the root of the payload (which includes "group", "host")
    groupElement.fromJson(parsedJsonFromString.get());

    // Spot check some fields based on dummy data in inventoryGroupHarvester.hpp
    // Dummy data: group.name = "dummygroup"
    EXPECT_EQ(groupElement.group.name, "dummygroup");
    // Dummy data: group.id_signed = 2001
    EXPECT_EQ(groupElement.group.id_signed, 2001);
    EXPECT_FALSE(groupElement.host.hostname.empty());
    // Dummy data: group.users has "dummy" and "anotheruser"
    ASSERT_EQ(groupElement.group.users.size(), 2);
    EXPECT_EQ(groupElement.group.users[0], "dummy");
    EXPECT_EQ(groupElement.group.users[1], "anotheruser");


    for (size_t i = 1; i < collectedData.size(); ++i) {
        cJSON_Delete(collectedData[i]);
    }
}
