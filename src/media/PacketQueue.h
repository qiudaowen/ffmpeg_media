#pragma once

class PacketQueue
{
public:
	PacketQueue();
	bool enQueue(const AVPacketPtr& packet);
	bool deQueue(AVPacketPtr& packet, bool block);
private:
	std::queue<AVPacketPtr> queue;
	uint32_t    nb_packets = 0;
	uint32_t    packetSize = 0;
};