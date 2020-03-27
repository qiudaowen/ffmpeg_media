#pragma once

#include <queue>

class PacketQueue
{
public:
	PacketQueue();
	bool enQueue(const AVPacketPtr& packet);
	bool deQueue(AVPacketPtr& packet);

    bool packetQueueFull();
    void clear();
    int packetCount() { return m_nb_packets; }
    int packetSize() { return m_packetSize; }
private:
	std::queue<AVPacketPtr> m_queue;
	uint32_t    m_nb_packets = 0;
	uint32_t    m_packetSize = 0;
};