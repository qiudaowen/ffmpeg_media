#include "FrameQueue.h"

FrameQueue::FrameQueue(int maxSize)
	: m_maxSize(maxSize)
{

}

bool FrameQueue::push(const AVFrameRef& frame, bool bOverwrite)
{
	if (m_maxSize > 0 && m_queue.size() == m_maxSize)
	{
		if (!bOverwrite)
			return false;

		m_queue.pop();
	}
	m_queue.push(frame);
	return true;
}

bool FrameQueue::pushLock(const AVFrameRef& packet, bool bOverwrite)
{
	QmStdMutexLocker(mutex());
	return push(packet, bOverwrite);
}

std::queue<AVFrameRef> FrameQueue::takeAll()
{
	return std::move(m_queue);
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
