#pragma once

#include <Windows.h>
#include <string>

class QcEvent
{
public:
	QcEvent()
		: m_hEvent(0)
	{}

	~QcEvent()
	{
		if (m_hEvent)
		{
			CloseHandle(m_hEvent);
			m_hEvent = NULL;
		}
	}

	void init(BOOL bManualReset = FALSE, BOOL bInitialState = FALSE, const wchar_t* key = NULL, bool bChangeName = false)
	{
		std::wstring sKey = key != NULL ? key : L"";
		if (sKey.size() && bChangeName)
		{
			sKey += L"_Event";
		}
		if (!m_hEvent)
		{
			m_hEvent = CreateEventW(NULL, bManualReset, bInitialState, key == NULL ? NULL : sKey.c_str());
		}
	}
	void setEvent() const
	{
		if (m_hEvent)
			SetEvent(m_hEvent);
	}
	void resetEvent() const
	{
		if (m_hEvent)
			ResetEvent(m_hEvent);
	}

	operator HANDLE() const
	{
		return m_hEvent;
	}
	operator bool() const
	{
		return m_hEvent != NULL;
	}
private:
	HANDLE m_hEvent;
};