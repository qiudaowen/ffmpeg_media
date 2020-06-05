#include "CoreRunloop.h"

CoreRunloop::CoreRunloop()
{
    m_exitEvent.init();
    m_wakeupEvent.init();
}

CoreRunloop::~CoreRunloop()
{

}

void CoreRunloop::post(const Callback0& func)
{
    QmCsLocker(m_csLock);
    m_asynCallList.push_back(func);
    m_wakeupEvent.setEvent();
}

void CoreRunloop::quit()
{
    m_exitEvent.setEvent();
}

void CoreRunloop::run()
{
	bool bLoop = true;
	MSG msg;
	HANDLE handle[] = { m_exitEvent, m_wakeupEvent };
	while (bLoop)
	{
		runOnce();
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				bLoop = false;
			}
			else
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		DWORD result = ::MsgWaitForMultipleObjectsEx(2, handle, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
		if (result == (WAIT_OBJECT_0 + 0))
		{
			bLoop = false;
		}
	}
	runOnce();
}

void CoreRunloop::runOnce()
{
	std::vector<Callback0> temp;
	{
        QmCsLocker(m_csLock);
		temp.swap(m_asynCallList);
        m_wakeupEvent.resetEvent();
	}
	if (!temp.empty())
	{
		for (auto& cb :temp)
		{
			cb();
		}
	}
}

