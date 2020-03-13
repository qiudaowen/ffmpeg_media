#pragma once

#include <Windows.h>

class QcMutex
{
public:
	QcMutex()
		: m_hMutex(0)
	{}

	~QcMutex()
	{
		if (m_hMutex)
		{
			CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}
	}

	void init(const wchar_t* key = NULL, bool bChangeName = false)
	{
		std::wstring sKey = key != NULL ? key : L"";
		if (sKey.size() && bChangeName)
		{
			sKey += L"_Mutex";
		}
		if (!m_hMutex && sKey.size())
		{
			m_hMutex = CreateMutex(NULL, FALSE, key == NULL ? NULL : sKey.c_str());
		}
	}
	void lock(int timeout = -1) const
	{
		if (m_hMutex)
			WaitForSingleObject(m_hMutex, timeout);
	}
	void unLock() const
	{
		if (m_hMutex)
			ReleaseMutex(m_hMutex);
	}

	operator HANDLE() const
	{
		return m_hMutex;
	}
	operator bool() const
	{
		return m_hMutex != NULL;
	}
private:
	HANDLE m_hMutex;
};

class QcAutoMutexLock
{
public:
	QcAutoMutexLock(const QcMutex &m) : _lock(m)
	{
		_lock.lock();
	}
	~QcAutoMutexLock()
	{
		_lock.unLock();
	}
private:
	const QcMutex& _lock;
};

#ifndef QmMutexLocker
#define QmMutexLocker(m) QcAutoMutexLock QmUniqueVarName(m)
#endif