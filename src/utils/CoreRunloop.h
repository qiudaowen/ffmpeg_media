#pragma once

#include <mutex>
#include <vector>
#include <functional>

using Callback0 = std::function<void(void)>;
class CoreRunloop
{
protected:
	std::mutex m_mutex;
	std::condition_variable m_mutexNotify;
	std::vector<Callback0> m_asynCallList;
	bool m_quit = false;
public:
	CoreRunloop();
	~CoreRunloop();

	void post(const Callback0& func);

	void quit();
	void run();
	void runOnce();
};
