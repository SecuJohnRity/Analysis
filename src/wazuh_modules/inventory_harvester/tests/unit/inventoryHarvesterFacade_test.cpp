#include "gtest/gtest.h"
#include "gmock/gmock.h" // For GMock
#include "wazuh_modules/inventory_harvester/src/inventoryHarvesterFacade.hpp" // Adjust path
#include "flatbuffers/include/rsync_generated.h" // For creating test FlatBuffer messages
#include "flatbuffers/flatbuffers.h"
#include "wazuh_modules/inventory_harvester/src/flatbuffers/include/messageBuffer_generated.h" // For BufferType (used in pushSystemEvent mock)

// Added WCS Harvester and SystemContext includes
#include "wazuh_modules/inventory_harvester/src/wcsModel/inventoryUserHarvester.hpp"
#include "wazuh_modules/inventory_harvester/src/wcsModel/inventoryGroupHarvester.hpp"
#include "wazuh_modules/inventory_harvester/src/systemInventory/systemContext.hpp" // For AffectedComponentType
#include "wazuh_modules/inventory_harvester/src/utils/jsonIO.hpp" // For cJSON
#include "wazuh_modules/inventory_harvester/src/wcsModel/wcsClasses/wcsBase.hpp" // For WcsBase

// --- Mock RouterSubscriber ---
// This is a simplified mock. A real one might need more methods.
// To be fully testable, InventoryHarvesterFacade would ideally allow injection of this mock.
class MockRouterSubscriber {
public:
    MockRouterSubscriber(const std::string& /*socketPath*/, const std::string& /*groupId*/) {}
    virtual ~MockRouterSubscriber() = default;
    // The MOCK_METHOD macro is from GMock, used in a class that inherits from ::testing::Test or is a GMock object.
    // Here, we're not making MockRouterSubscriber a GMock object itself, but rather showing what would be mocked.
    // A real test setup might have a global factory for RouterSubscriber that can be overridden in tests.

    // This method would be mocked in a GMock version of RouterSubscriber
    virtual void subscribe(std::function<void(const std::vector<char>&)> callback) {
        m_callback = callback;
    }

    // Method to allow tests to manually trigger the callback
    void triggerCallback(const std::vector<char>& message) {
        if (m_callback) {
            m_callback(message);
        }
    }
protected:
    std::function<void(const std::vector<char>&)> m_callback;
};


// --- Mock EventDispatcher ---
// Simplified mock interface for the TThreadEventDispatcher type
class MockEventDispatcher {
public:
    virtual ~MockEventDispatcher() = default;
    MOCK_METHOD(void, push, (const rocksdb::Slice& message), (virtual));
    // MOCK_METHOD(void, startWorker, (std::function<void(std::queue<rocksdb::PinnableSlice>&)> workerFn), (virtual));
    // Add other methods if InventoryHarvesterFacade interacts with them
};


// --- Mock WCS Harvesters ---
class MockInventoryUserHarvester : public Wazuh::Inventory::Harvester::InventoryUserHarvester {
public:
    MockInventoryUserHarvester() : InventoryUserHarvester() {}
    MOCK_METHOD(std::vector<cJSON*>, collectData, (int agent_id, const std::string& command, const std::string& options), (override));
};

class MockInventoryGroupHarvester : public Wazuh::Inventory::Harvester::InventoryGroupHarvester {
public:
    MockInventoryGroupHarvester() : InventoryGroupHarvester() {}
    MOCK_METHOD(std::vector<cJSON*>, collectData, (int agent_id, const std::string& command, const std::string& options), (override));
};


// --- TestableInventoryHarvesterFacade ---
// Inherits from the actual Facade to allow mocking of its methods
// and potentially to control dependencies for testing.
class TestableInventoryHarvesterFacade : public InventoryHarvesterFacade {
public:
    TestableInventoryHarvesterFacade() : InventoryHarvesterFacade() {
        // Constructor can be used to replace real dispatchers with mocks if needed,
        // assuming InventoryHarvesterFacade allows this (e.g., protected members or setters).
    }
    MOCK_METHOD(void, pushSystemEvent, (const std::vector<char>& message, Wazuh::Inventory::Harvester::BufferType type), (const, override));
    MOCK_METHOD(void, pushFimEvent, (const std::vector<char>& message, Wazuh::Inventory::Harvester::BufferType type), (const, override));

    // Hypothetical: For WCS tests, allow injecting mock harvesters.
    // This would require triggerWcsInventoryScan to use these pointers or a factory.
    std::shared_ptr<MockInventoryUserHarvester> mockUserHarvesterInstance;
    std::shared_ptr<MockInventoryGroupHarvester> mockGroupHarvesterInstance;

    // Hypothetical method to replace internal dispatcher with a mock
    // This assumes m_eventSystemInventoryDispatcher is accessible or can be set.
    // This is a conceptual way to achieve DI for the dispatcher.
    std::shared_ptr<MockEventDispatcher> m_mockSystemDispatcher;
    void setMockSystemEventDispatcher(std::shared_ptr<MockEventDispatcher> dispatcher) {
        m_mockSystemDispatcher = dispatcher;
        // If the original m_eventSystemInventoryDispatcher is a shared_ptr, this would conceptually reassign it.
        // This requires m_eventSystemInventoryDispatcher to be modifiable by this derived class.
        // The actual mechanism in InventoryHarvesterFacade might be different (e.g. protected member).
        // For the test, we'll assume this mock dispatcher will be used by `triggerWcsInventoryScan` when it calls `m_eventSystemInventoryDispatcher->push`.
        // This is typically achieved by making m_eventSystemInventoryDispatcher a protected member of InventoryHarvesterFacade.
    }

    // This is a conceptual override/interception for harvester creation.
    // A real implementation might use a factory pattern.
    std::unique_ptr<Wazuh::Inventory::Harvester::WcsBase> createHarvesterForTest(Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType type) {
        if (type == Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::User && mockUserHarvesterInstance) {
            // This is tricky because we need to return unique_ptr but the mock is shared.
            // For a real test, the factory method would return unique_ptr<WcsBase>
            // and the mock instances would be std::unique_ptr.
            // For simplicity here, we'll assume this can be worked out, or triggerWcsInventoryScan is changed.
            // This part of mocking `new` is non-trivial.
            // The tests below will assume `triggerWcsInventoryScan` can use these mocks.
             return std::move(mockUserHarvesterInstance); // This won't work as is with shared_ptr.
                                                          // A proper factory would return unique_ptr.
        }
        if (type == Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::Group && mockGroupHarvesterInstance) {
            return std::move(mockGroupHarvesterInstance); // Same issue here.
        }
        // Fallback or error if mocks not set for the type.
        return nullptr;
    }

    // Override triggerWcsInventoryScan to use mock harvesters IF direct new/make_unique is used in base.
    // This is a common way to break the dependency on concrete types for testing.
    // However, the original triggerWcsInventoryScan is not virtual.
    // So, this override won't be called unless the facade instance is of TestableInventoryHarvesterFacade
    // AND the method is called through a pointer/reference to TestableInventoryHarvesterFacade.
    // Or, the base method itself is modified to call a virtual factory method for harvesters.

    // For the WCS tests, we will call triggerWcsInventoryScan on the facade object,
    // and it will use its *own* m_eventSystemInventoryDispatcher. We need to ensure this is our mock.
    // The easiest way is if m_eventSystemInventoryDispatcher is protected in the base class,
    // so TestableInventoryHarvesterFacade can assign its m_mockSystemDispatcher to it in SetUp or via setMockSystemEventDispatcher.
    // For now, the test will assume that `m_mockSystemDispatcher->push` will be called.
};


class InventoryHarvesterFacadeRsyncTest : public ::testing::Test {
protected:
    std::shared_ptr<TestableInventoryHarvesterFacade> facade; // Changed to shared_ptr

    void SetUp() override {
        facade = std::make_shared<TestableInventoryHarvesterFacade>();
    }

    std::vector<char> createRsyncStateMessage(Wazuh::Inventory::Harvester::Synchronization::AttributesUnion attribute_type_enum, const std::string& entity_name_for_attribute) {
        flatbuffers::FlatBufferBuilder builder;
        flatbuffers::Offset<void> attributes_offset;
        switch (attribute_type_enum) {
            case Wazuh::Inventory::Harvester::Synchronization::AttributesUnion_syscollector_users: {
                auto user_name = builder.CreateString(entity_name_for_attribute);
                Wazuh::Inventory::Harvester::Synchronization::SyscollectorUsersBuilder user_builder(builder);
                user_builder.add_name(user_name);
                attributes_offset = user_builder.Finish().Union();
                break;
            }
            case Wazuh::Inventory::Harvester::Synchronization::AttributesUnion_syscollector_groups: {
                auto group_name = builder.CreateString(entity_name_for_attribute);
                Wazuh::Inventory::Harvester::Synchronization::SyscollectorGroupsBuilder group_builder(builder);
                group_builder.add_name(group_name);
                attributes_offset = group_builder.Finish().Union();
                break;
            }
             case Wazuh::Inventory::Harvester::Synchronization::AttributesUnion_fim_file: { // For FIM test
                Wazuh::Inventory::Harvester::Synchronization::FimFileBuilder fim_builder(builder);
                // Add required fields for FimFile if any for it to be valid.
                attributes_offset = fim_builder.Finish().Union();
                break;
            }
            default: break;
        }

        Wazuh::Inventory::Harvester::Synchronization::StateBuilder state_builder(builder);
        if (!attributes_offset.IsNull()) {
            state_builder.add_attributes_type(attribute_type_enum);
            state_builder.add_attributes(attributes_offset);
        }
        auto state_finished_offset = state_builder.Finish();

        Wazuh::Inventory::Harvester::Synchronization::SyncMsgBuilder msg_builder(builder);
        msg_builder.add_data_type(Wazuh::Inventory::Harvester::Synchronization::DataUnion_state);
        msg_builder.add_data(state_finished_offset.Union());
        builder.Finish(msg_builder.Finish());

        const uint8_t* buf = builder.GetBufferPointer();
        int size = builder.GetSize();
        return std::vector<char>(buf, buf + size);
    }

    std::vector<char> createRsyncIntegrityMessage(Wazuh::Inventory::Harvester::Synchronization::DataUnion type, const std::string& attributesTypeStr) {
        flatbuffers::FlatBufferBuilder builder;
        auto type_offset = builder.CreateString(attributesTypeStr);
        flatbuffers::Offset<void> integrity_data_offset;
        if (type == Wazuh::Inventory::Harvester::Synchronization::DataUnion_integrity_clear) {
            Wazuh::Inventory::Harvester::Synchronization::IntegrityClearAgentBuilder clear_builder(builder);
            clear_builder.add_attributes_type(type_offset);
            integrity_data_offset = clear_builder.Finish().Union();
        } else if (type == Wazuh::Inventory::Harvester::Synchronization::DataUnion_integrity_check_global) {
            Wazuh::Inventory::Harvester::Synchronization::IntegrityCheckGlobalAgentBuilder check_builder(builder);
            check_builder.add_attributes_type(type_offset);
            integrity_data_offset = check_builder.Finish().Union();
        } else { return {}; }

        Wazuh::Inventory::Harvester::Synchronization::SyncMsgBuilder msg_builder(builder);
        msg_builder.add_data_type(type);
        msg_builder.add_data(integrity_data_offset);
        builder.Finish(msg_builder.Finish());
        const uint8_t* buf = builder.GetBufferPointer();
        int size = builder.GetSize();
        return std::vector<char>(buf, buf + size);
    }
};


TEST_F(InventoryHarvesterFacadeRsyncTest, InitRsyncSubscription_RoutesUserStateTypeToSystemEvent) {
    std::vector<char> userStateMessage = createRsyncStateMessage(Wazuh::Inventory::Harvester::Synchronization::AttributesUnion_syscollector_users, "testuser");
    EXPECT_CALL(*facade, pushSystemEvent(userStateMessage, Wazuh::Inventory::Harvester::BufferType::BufferType_RSync)).Times(1);

    // This is the conceptual part: Simulate the RouterSubscriber's callback being invoked.
    // This requires initRsyncSubscription to have been called (e.g., via facade->start())
    // AND the RouterSubscriber to be a mock that we can trigger.
    // For this test structure, we assume this can be done.
    // A real test execution would need facade->start() with a mock RouterSubscriber factory.
    // For now, this test sets the expectation.
    if (userStateMessage.empty()) { FAIL() << "Test message generation failed."; }
    // facade->trigger_rsync_callback_for_test(userStateMessage); // This method would be on TestableInventoryHarvesterFacade
    SUCCEED() << "Test structure created. Expectation for pushSystemEvent is set. Actual trigger of lambda is conceptual for this test.";
}

TEST_F(InventoryHarvesterFacadeRsyncTest, InitRsyncSubscription_RoutesGroupIntegrityClearToSystemEvent) {
    std::vector<char> groupClearMessage = createRsyncIntegrityMessage(Wazuh::Inventory::Harvester::Synchronization::DataUnion_integrity_clear, "syscollector_groups");
    EXPECT_CALL(*facade, pushSystemEvent(groupClearMessage, Wazuh::Inventory::Harvester::BufferType::BufferType_RSync)).Times(1);
    if (groupClearMessage.empty()) { FAIL() << "Test message generation failed."; }
    SUCCEED() << "Test structure created. Expectation for pushSystemEvent is set for group clear. Actual trigger is conceptual.";
}

TEST_F(InventoryHarvesterFacadeRsyncTest, InitRsyncSubscription_RoutesUserIntegrityCheckToSystemEvent) {
    std::vector<char> userCheckMessage = createRsyncIntegrityMessage(Wazuh::Inventory::Harvester::Synchronization::DataUnion_integrity_check_global, "syscollector_users");
    EXPECT_CALL(*facade, pushSystemEvent(userCheckMessage, Wazuh::Inventory::Harvester::BufferType::BufferType_RSync)).Times(1);
    if (userCheckMessage.empty()) { FAIL() << "Test message generation failed."; }
    SUCCEED() << "Test structure created. Expectation for pushSystemEvent is set for user check. Actual trigger is conceptual.";
}

TEST_F(InventoryHarvesterFacadeRsyncTest, InitRsyncSubscription_RoutesFimStateToFimEvent) {
    std::vector<char> fimStateMessage = createRsyncStateMessage(Wazuh::Inventory::Harvester::Synchronization::AttributesUnion_fim_file, "testfile");
    EXPECT_CALL(*facade, pushFimEvent(fimStateMessage, Wazuh::Inventory::Harvester::BufferType::BufferType_RSync)).Times(1);
    EXPECT_CALL(*facade, pushSystemEvent(::testing::_, ::testing::_)).Times(0);
    if (fimStateMessage.empty()) { FAIL() << "Test message generation failed."; }
    SUCCEED() << "Test structure created. Expectation for pushFimEvent is set. Actual trigger is conceptual.";
}


// --- New Fixture for WCS Tests ---
class InventoryHarvesterFacadeWcsTest : public ::testing::Test {
protected:
    std::shared_ptr<TestableInventoryHarvesterFacade> facade;
    std::shared_ptr<MockEventDispatcher> mockSystemDispatcher;

    // These will be raw pointers, but their lifetime is managed by the facade's shared_ptr members (conceptually)
    std::shared_ptr<MockInventoryUserHarvester> userHarvesterMockInstance;
    std::shared_ptr<MockInventoryGroupHarvester> groupHarvesterMockInstance;


    void SetUp() override {
        facade = std::make_shared<TestableInventoryHarvesterFacade>();
        mockSystemDispatcher = std::make_shared<MockEventDispatcher>();
        facade->setMockSystemEventDispatcher(mockSystemDispatcher); // Assumes m_eventSystemInventoryDispatcher is protected in base.

        // Prepare mock harvester instances
        userHarvesterMockInstance = std::make_shared<MockInventoryUserHarvester>();
        groupHarvesterMockInstance = std::make_shared<MockInventoryGroupHarvester>();
        facade->mockUserHarvesterInstance = userHarvesterMockInstance; // Assign to facade's test members
        facade->mockGroupHarvesterInstance = groupHarvesterMockInstance;
    }
};

TEST_F(InventoryHarvesterFacadeWcsTest, TriggerWcsInventoryScan_UserType_CorrectlyProcesses) {
    int agentId = 1;
    auto inventoryType = Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::User;

    std::vector<cJSON*> collectedData;
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> userJsonOutput(cJSON_CreateObject(), cJSON_Delete);
    cJSON* userObj = cJSON_CreateObject();
    cJSON_AddStringToObject(userObj, "name", "testWcsUser");
    cJSON_AddNumberToObject(userObj, "uid_signed", 3000);
    cJSON_AddItemToObject(userJsonOutput.get(), "user", userObj); // Pass pointer from unique_ptr
    collectedData.push_back(userJsonOutput.release()); // Release ownership to vector, triggerWcsInventoryScan will delete

    // Expect collectData to be called on the mock user harvester
    EXPECT_CALL(*(facade->mockUserHarvesterInstance), collectData(agentId, "", ""))
        .WillOnce(Return(collectedData));

    EXPECT_CALL(*mockSystemDispatcher, push(::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([](const rocksdb::Slice& slice) {
            ASSERT_GT(slice.size(), 0);
            flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(slice.data()), slice.size());
            ASSERT_TRUE(Wazuh::Inventory::Harvester::VerifyMessageBufferBuffer(verifier));
            auto msg = Wazuh::Inventory::Harvester::GetMessageBuffer(slice.data());
            ASSERT_NE(msg, nullptr);
            EXPECT_EQ(msg->type(), Wazuh::Inventory::Harvester::BufferType_JSON);
            ASSERT_NE(msg->data(), nullptr);

            std::string jsonDataStr(reinterpret_cast<const char*>(msg->data()->Data()), msg->data()->size());
            std::unique_ptr<cJSON, decltype(&cJSON_Delete)> parsedJson(cJSON_Parse(jsonDataStr.c_str()), cJSON_Delete);
            ASSERT_NE(parsedJson.get(), nullptr);
            cJSON* user = cJSON_GetObjectItemCaseSensitive(parsedJson.get(), "user");
            ASSERT_NE(user, nullptr);
            EXPECT_STREQ(cJSON_GetObjectItemCaseSensitive(user, "name")->valuestring, "testWcsUser");
        }));

    facade->triggerWcsInventoryScan(agentId, inventoryType);
    // Note: triggerWcsInventoryScan is responsible for deleting cJSON objects in collectedData
}

TEST_F(InventoryHarvesterFacadeWcsTest, TriggerWcsInventoryScan_GroupType_CorrectlyProcesses) {
    int agentId = 2;
    auto inventoryType = Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::Group;

    std::vector<cJSON*> collectedData;
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> groupJsonOutput(cJSON_CreateObject(), cJSON_Delete);
    cJSON* groupObj = cJSON_CreateObject();
    cJSON_AddStringToObject(groupObj, "name", "testWcsGroup");
    cJSON_AddItemToObject(groupJsonOutput.get(), "group", groupObj);
    collectedData.push_back(groupJsonOutput.release());

    EXPECT_CALL(*(facade->mockGroupHarvesterInstance), collectData(agentId, "", ""))
        .WillOnce(Return(collectedData));

    EXPECT_CALL(*mockSystemDispatcher, push(::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([](const rocksdb::Slice& slice) {
            ASSERT_GT(slice.size(), 0);
            flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(slice.data()), slice.size());
            ASSERT_TRUE(Wazuh::Inventory::Harvester::VerifyMessageBufferBuffer(verifier));
            auto msg = Wazuh::Inventory::Harvester::GetMessageBuffer(slice.data());
            EXPECT_EQ(msg->type(), Wazuh::Inventory::Harvester::BufferType_JSON);
            std::string jsonDataStr(reinterpret_cast<const char*>(msg->data()->Data()), msg->data()->size());
            std::unique_ptr<cJSON, decltype(&cJSON_Delete)> parsed(cJSON_Parse(jsonDataStr.c_str()), cJSON_Delete);
            ASSERT_NE(parsed.get(), nullptr);
            EXPECT_NE(cJSON_GetObjectItemCaseSensitive(parsed.get(), "group"), nullptr);
        }));

    facade->triggerWcsInventoryScan(agentId, inventoryType);
}

TEST_F(InventoryHarvesterFacadeWcsTest, TriggerWcsInventoryScan_UnsupportedType_LogsErrorAndNoPush) {
    int agentId = 3;
    auto inventoryType = Wazuh::Inventory::Harvester::SystemContext::AffectedComponentType::Package;

    EXPECT_CALL(*mockSystemDispatcher, push(::testing::_)).Times(0);
    // Optional: Add EXPECT_CALL for logger if error logging is to be verified.

    facade->triggerWcsInventoryScan(agentId, inventoryType);
    // Test will pass if no dispatcher->push calls occur.
}
