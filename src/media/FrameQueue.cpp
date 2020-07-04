#include "FrameQueue.h"

FrameQueue::FrameQueue()
{

}

bool FrameQueue::push(const AVFrameRef& frame)
{
	m_queue.push(frame);
	return true;
}

bool FrameQueue::pop(AVFrameRef& frame)
{
	if (m_queue.size())
	{
		frame = m_queue.front();
		m_queue.pop();
		return true;
	}
	return false;
}

bool FrameQueue::front(AVFrameRef& frame)
{
	if (m_queue.size())
	{
		frame = m_queue.front();
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
