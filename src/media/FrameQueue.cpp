#include "FrameQueue.h"

FrameQueue::FrameQueue()
{

}

bool FrameQueue::push(const AVFrameRef& packet)
{
	m_queue.push(packet);
	return true;
}

bool FrameQueue::pop(AVFrameRef& packet)
{
	if (m_queue.size())
	{
		packet = m_queue.front();
		m_queue.pop();
		return true;
	}
	return false;
}

bool FrameQueue::front(AVFrameRef& packet)
{
	if (m_queue.size())
	{
		packet = m_queue.front();
		return true;
	}
	return false;
}

void FrameQueue::clear()
{
	m_queue.swap(std::queue<AVFrameRef>());
}

int FrameQueue::size()
{
	return m_queue.size();
}
