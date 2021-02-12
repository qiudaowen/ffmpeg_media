#pragma once

#include "media_global.h"
#include "QsMediaInfo.h"
#include <queue>

class PacketQueue
{
public:
	PacketQueue();
	bool push(const AVPacketPtr& packet);
	bool pop(AVPacketPtr& packet);
    void clear();
	int size() const { return m_queue.size();  }

    int packetSize() const { return m_packetSize; }
private:
	std::queue<AVPacketPtr> m_queue;
	uint32_t    m_nb_packets = 0;
	uint32_t    m_packetSize = 0;
};