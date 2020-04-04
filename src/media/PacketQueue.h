#pragma once

#include "mediaPub.h"
#include "QsMediaInfo.h"
#include <queue>

class MEDIA_API PacketQueue
{
public:
	PacketQueue();
	bool push(const AVPacketPtr& packet);
	bool pop(AVPacketPtr& packet);

    void clear();
    int packetCount() { return m_nb_packets; }
    int packetSize() { return m_packetSize; }
private:
	std::queue<AVPacketPtr> m_queue;
	uint32_t    m_nb_packets = 0;
	uint32_t    m_packetSize = 0;
};