#pragma once

#include "mediaPub.h"
#include "QsMediaInfo.h"
#include "AVFrameRef.h"
#include <queue>
#include <mutex>

class FrameQueue
{
public:
	FrameQueue();

	bool push(const AVFrameRef& packet);
	bool pop(AVFrameRef& packet);
	bool front(AVFrameRef& packet);
	void clear();
	int size();

	std::mutex& mutex() { return m_mutex; }
protected:
	std::queue<AVFrameRef> m_queue;
	std::mutex m_mutex;
};