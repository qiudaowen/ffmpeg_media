#pragma once

#include <functional>
#include <thread>

class CoreRunloop;
class WorkThread 
{
public:
	WorkThread();
	~WorkThread();

	void setInitFunc(std::function<void()> initFunc);
	void setRunFunc(std::function<void()> Func);
	void setUnitFunc(std::function<void()> uninitFunc);
	void start();
	void stop();
	void waitStop();

	void post(std::function<void(void)> func);
protected:
	void run();
protected:
	std::thread m_thread;
	CoreRunloop* m_loop;
	std::function<void()> m_initFunc;
	std::function<void()> m_runFunc;
	std::function<void()> m_uninitFunc;
};