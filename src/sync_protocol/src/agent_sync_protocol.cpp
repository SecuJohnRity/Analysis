#include "agent_sync_protocol.hpp"

#include <flatbuffers/flatbuffers.h>
#include <iostream>

extern "C"
{
#include "defs.h"

#define SYNC_MQ 's'

    int StartMQ(const char* key, short int type, short int n_attempts);
    int SendMSG(int queue, const char* message, const char* locmsg, char loc);
}

using namespace Wazuh::SyncSchema;

void AgentSyncProtocol::persistDifference(const std::string& module,
                                          const std::string& id,
                                          Operation operation,
                                          const std::string& index,
                                          const std::string& data)
{
    m_data[module].emplace_back(
        PersistedData {.seq = ++m_seqCounter, .id = id, .index = index, .data = data, .operation = operation});
}

void AgentSyncProtocol::synchronizeModule(const std::string& module, Mode mode, bool realtime)
{
    if (!ensureQueueAvailable())
    {
        std::cerr << "Failed to open queue: " << DEFAULTQUEUE << std::endl;
        return;
    }

    uint64_t session = 0;
    if (!sendStartAndWaitAck(module, mode, realtime, session))
    {
        std::cerr << "Failed to send start message" << std::endl;
        return;
    }

    if (!sendDataMessages(module, session))
    {
        std::cerr << "Failed to send data messages" << std::endl;
        return;
    }

    if (!sendEndAndWaitAck(module, session))
    {
        std::cerr << "Failed to send end message" << std::endl;
        return;
    }

    clearPersistedDifferences(module);
}

bool AgentSyncProtocol::ensureQueueAvailable()
{
    if (m_queue < 0)
    {
        m_queue = StartMQ(DEFAULTQUEUE, WRITE, 0);
        if (m_queue < 0)
        {
            return false;
        }
    }
    return true;
}

bool AgentSyncProtocol::sendStartAndWaitAck(const std::string& module, Mode mode, bool realtime, uint64_t& session)
{
    flatbuffers::FlatBufferBuilder builder;
    auto moduleStr = builder.CreateString(module);

    StartBuilder startBuilder(builder);
    startBuilder.add_mode(mode);
    startBuilder.add_size(static_cast<uint64_t>(m_data[module].size()));
    startBuilder.add_realtime(realtime);
    startBuilder.add_module_(moduleStr);
    auto startOffset = startBuilder.Finish();

    auto message = CreateMessage(builder, MessageType::Start, startOffset.Union());
    builder.Finish(message);

    return sendFlatBufferMessageAsString(builder.GetBufferSpan(), module) && receiveStartAck(session);
}

bool AgentSyncProtocol::receiveStartAck(uint64_t& session)
{
    // Simulated StartAck
    session = 99999;
    return true;
}

bool AgentSyncProtocol::sendDataMessages(const std::string& module,
                                         uint64_t session,
                                         const std::vector<std::pair<uint64_t, uint64_t>>* ranges)
{
    for (const auto& item : m_data[module])
    {
        bool inRange = true;
        if (ranges)
        {
            inRange = false;
            for (const auto& [begin, end] : *ranges)
            {
                if (item.seq >= begin && item.seq <= end)
                {
                    inRange = true;
                    break;
                }
            }
        }

        if (!inRange)
        {
            continue;
        }

        flatbuffers::FlatBufferBuilder builder;
        auto idStr = builder.CreateString(item.id);
        auto idxStr = builder.CreateString(item.index);
        auto dataVec = builder.CreateVector(reinterpret_cast<const int8_t*>(item.data.data()), item.data.size());

        DataBuilder dataBuilder(builder);
        dataBuilder.add_seq(item.seq);
        dataBuilder.add_session(session);
        dataBuilder.add_id(idStr);
        dataBuilder.add_index(idxStr);
        dataBuilder.add_operation(item.operation);
        dataBuilder.add_data(dataVec);
        auto dataOffset = dataBuilder.Finish();

        auto message = CreateMessage(builder, MessageType::Data, dataOffset.Union());
        builder.Finish(message);

        if (!sendFlatBufferMessageAsString(builder.GetBufferSpan(), module))
        {
            return false;
        }
    }

    return true;
}

bool AgentSyncProtocol::sendEndAndWaitAck(const std::string& module, uint64_t session)
{
    flatbuffers::FlatBufferBuilder builder;
    EndBuilder endBuilder(builder);
    endBuilder.add_session(session);
    auto endOffset = endBuilder.Finish();

    auto message = CreateMessage(builder, MessageType::End, endOffset.Union());
    builder.Finish(message);

    if (!sendFlatBufferMessageAsString(builder.GetBufferSpan(), module))
    {
        return false;
    }

    while (true)
    {
        const auto ranges = receiveReqRet();
        if (!ranges.empty())
        {
            if (!sendDataMessages(module, session, &ranges))
            {
                return false;
            }
            continue;
        }

        return receiveEndAck();
    }
}

bool AgentSyncProtocol::receiveEndAck()
{
    // Simulated EndAck
    return true;
}

std::vector<std::pair<uint64_t, uint64_t>> AgentSyncProtocol::receiveReqRet()
{
    // Simulated ReqRet
    static int callCount = 0;
    callCount++;

    if (callCount == 1)
    {
        return {{1, 1}, {3, 3}};
    }
    else if (callCount == 2)
    {
        return {{2, 2}};
    }

    return {};
}

void AgentSyncProtocol::clearPersistedDifferences(const std::string& module)
{
    m_data.erase(module);
}

bool AgentSyncProtocol::sendFlatBufferMessageAsString(flatbuffers::span<uint8_t> fbData, const std::string& module)
{
    std::string message(reinterpret_cast<const char*>(fbData.data()), fbData.size());

    if (SendMSG(m_queue, message.c_str(), module.c_str(), SYNC_MQ) < 0)
    {
        std::cerr << "SendMSG failed, attempting to reinitialize queue..." << std::endl;
        m_queue = StartMQ(DEFAULTQUEUE, WRITE, 0);
        if (m_queue < 0 || SendMSG(m_queue, message.c_str(), module.c_str(), SYNC_MQ) < 0)
        {
            std::cerr << "Failed to send message after retry" << std::endl;
            return false;
        }
    }
    return true;
}
