#include "gtest/gtest.h"
#include "wazuh_modules/inventory_harvester/src/wcsModel/inventoryUserHarvester.hpp" // Adjust path
#include "wazuh_modules/inventory_harvester/src/systemInventory/elements/userElement.hpp" // For parsing
#include "utils/jsonIO.hpp" // For cJSON_Delete and other cJSON utils if needed
#include <memory> // For std::unique_ptr

class InventoryUserHarvesterTest : public ::testing::Test {
protected:
    Wazuh::Inventory::Harvester::InventoryUserHarvester userHarvester;
};

TEST_F(InventoryUserHarvesterTest, CollectData_ReturnsValidAndParsableUserJson) {
    std::vector<cJSON*> collectedData = userHarvester.collectData(1, "test_command", "test_options");

    ASSERT_FALSE(collectedData.empty());
    // The harvester is responsible for allocating the cJSON*, tests are responsible for deleting it.
    // The harvester's current dummy implementation returns one item.
    ASSERT_NE(collectedData[0], nullptr);

    // Use a unique_ptr for proper cleanup of the cJSON object from the harvester
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> jsonDataItem(collectedData[0], cJSON_Delete);

    // Print to string then parse back. This ensures UserElement::fromJson gets a cJSON object
    // that it can "own" if it were to modify it, and also validates the print/parse cycle.
    // It also decouples the test from whether fromJson itself might delete the input.
    char* jsonStr = cJSON_PrintUnformatted(jsonDataItem.get());
    ASSERT_NE(jsonStr, nullptr); // Check if printing was successful

    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> parsedJsonFromString(cJSON_Parse(jsonStr), cJSON_Delete);
    free(jsonStr); // Free string from cJSON_PrintUnformatted
    ASSERT_NE(parsedJsonFromString.get(), nullptr); // Check if re-parsing was successful


    Wazuh::Inventory::Harvester::UserElement<void> userElement; // TContext as void for fromJson
    // UserElement::fromJson expects the root of the payload (which includes "user", "host", "process", "login" objects)
    userElement.fromJson(parsedJsonFromString.get());

    // Spot check some fields to ensure dummy data is structured as expected by UserElement
    EXPECT_FALSE(userElement.user.name.empty());
    // From dummy data in inventoryUserHarvester.hpp: user.uid_signed = 1001
    EXPECT_EQ(userElement.user.uid_signed, 1001);
    EXPECT_FALSE(userElement.host.hostname.empty());
    // Check a nested field from the detailed structure (dummy data: password.status = "enabled")
    EXPECT_EQ(userElement.user.password.status, "enabled");
    // Dummy data: login.type = "interactive"
    EXPECT_EQ(userElement.login.type, "interactive");


    // If collectData could return multiple cJSON objects, clean them up.
    // Current dummy implementation only returns one, so this loop won't run for i=1.
    for (size_t i = 1; i < collectedData.size(); ++i) {
        cJSON_Delete(collectedData[i]);
    }
}
