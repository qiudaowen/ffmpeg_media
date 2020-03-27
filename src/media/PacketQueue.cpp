#include "PacketQueue.h"


PacketQueue::PacketQueue()
{

}

bool PacketQueue::enQueue(const AVPacketPtr& packet)
{
    m_queue.push(packet);
    ++m_nb_packets;
    m_packetSize += packet->size;
    return true;
}

bool PacketQueue::deQueue(AVPacketPtr& packet)
{
    if (m_queue.size())
    {
        packet = m_queue.front();
        m_packetSize -= packet->size;
        --m_nb_packets;
        m_queue.pop();
        return true;
    }
    return false;
}

bool PacketQueue::packetQueueFull()
{
    return m_nb_packets > 20;
}

void PacketQueue::clear()
{
    m_queue.swap(std::queue<AVPacketPtr>());
    m_nb_packets = 0;
    m_packetSize = 0;
}
