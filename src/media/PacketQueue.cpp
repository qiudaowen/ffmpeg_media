#include "PacketQueue.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
};
#endif


PacketQueue::PacketQueue()
{

}

bool PacketQueue::push(const AVPacketPtr& packet)
{
    m_queue.push(packet);
    m_packetSize += packet->size;
    return true;
}

bool PacketQueue::pop(AVPacketPtr& packet)
{
    if (m_queue.size())
    {
        packet = m_queue.front();
        m_packetSize -= packet->size;
        m_queue.pop();
        return true;
    }
    return false;
}

void PacketQueue::clear()
{
    m_queue.swap(std::queue<AVPacketPtr>());
    m_packetSize = 0;
}
