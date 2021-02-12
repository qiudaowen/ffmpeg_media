#pragma once

#include "media_global.h"
#include "QsMediaInfo.h"
#include "AVFrameRef.h"
#include <queue>
#include <mutex>

class FrameQueue
{
public:
	FrameQueue(int maxSize = -1);

	bool push(const AVFrameRef& packet, bool bOverwrite = false);
	bool pushLock(const AVFrameRef& packet, bool bOverwrite = false);

	std::queue<AVFrameRef> takeAll();
	bool pop(AVFrameRef& packet);
	bool front(AVFrameRef& packet);
	void clear();
	int size();

	std::mutex& mutex() { return m_mutex; }
protected:
	std::mutex m_mutex;
	std::queue<AVFrameRef> m_queue;
	int m_maxSize;
};