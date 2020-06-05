#pragma once

#include "QcEvent.h"
#include "QcCriticalLock.h"
#include <vector>
#include <functional>

using Callback0 = std::function<void(void)>;
class CoreRunloop
{
protected:
	QcEvent m_exitEvent;
    QcEvent m_wakeupEvent;
    QcCriticalLock m_csLock;
	std::vector<Callback0> m_asynCallList;
public:
	CoreRunloop();
	~CoreRunloop();

	void post(const Callback0& func);

	void quit();
	void run();
	void runOnce();
protected:
    void calWaitTime();
};
