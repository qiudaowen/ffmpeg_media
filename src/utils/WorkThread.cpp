#include "WorkThread.h"
#include "CoreRunloop.h"

WorkThread::WorkThread()
{

}
WorkThread::~WorkThread()
{
	waitStop();
}

void WorkThread::setInitFunc(std::function<void()> initFunc)
{
	m_initFunc = std::move(initFunc);
}
void WorkThread::setRunFunc(std::function<void()> Func)
{
	m_runFunc = std::move(Func);
}
void WorkThread::setUnitFunc(std::function<void()> uninitFunc)
{
	m_uninitFunc = std::move(uninitFunc);
}

void WorkThread::start()
{
	waitStop();

	m_loop = new CoreRunloop();
	m_thread = std::thread([this] {
		run();
		});
}

void WorkThread::run()
{
	if (m_initFunc)
		m_initFunc();

	if (m_runFunc)
		m_runFunc();
	else
		m_loop->run();

	if (m_uninitFunc)
		m_uninitFunc();
}

void WorkThread::post(std::function<void(void)> func)
{
	m_loop->post(std::move(func));
}

void WorkThread::stop()
{
	if (m_loop) {
		m_loop->quit();
	}
}
void WorkThread::waitStop()
{
	stop();
	if (m_thread.joinable())
		m_thread.join();
	if (m_loop) {
		delete m_loop;
		m_loop = nullptr;
	}
}