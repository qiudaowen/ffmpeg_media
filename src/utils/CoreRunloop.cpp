#include "CoreRunloop.h"
#include "QmMacro.h"

CoreRunloop::CoreRunloop()
{

}

CoreRunloop::~CoreRunloop()
{

}

void CoreRunloop::post(const Callback0& func)
{
	QmStdMutexLocker(m_mutex);
    m_asynCallList.push_back(func);
	m_mutexNotify.notify_all();
}

void CoreRunloop::quit()
{
	m_quit = true;
}

void CoreRunloop::run()
{
	bool bLoop = true;
	while (!m_quit)
	{
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_mutexNotify.wait(lock);
		}
		runOnce();
	}
	runOnce();
}

void CoreRunloop::runOnce()
{
	std::vector<Callback0> temp;
	{
        QmStdMutexLocker(m_mutex);
		temp.swap(m_asynCallList);
	}
	if (!temp.empty())
	{
		for (auto& cb :temp)
		{
			cb();
		}
	}
}

