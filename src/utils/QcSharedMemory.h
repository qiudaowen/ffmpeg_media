#pragma once
#include <Windows.h>
#include <MMSystem.h>
#include <string>
#include "QcMutex.h"

#ifndef QmLogTimeOnce2
#define QmLogTimeOnce2(time, fmt, ...) void (0)
#endif
// 	static DWORD lastTime##__LINE__ = 0; \
// 	if (timeGetTime() - lastTime##__LINE__ >= time) \
//  {	\
//  lastTime##__LINE__ = timeGetTime(); \
//  printf(fmt, ## __VA_ARGS__); \
//   } \
//   void (0)

class QcSharedMemory
{
public:
	enum AccessMode
	{
		ReadOnly,
		ReadWrite
	};

	QcSharedMemory()
		: m_size(0)
		, m_memory(NULL)
		, m_hand(NULL)
	{
	}

	QcSharedMemory(const std::wstring &key, bool bChangeName = true)
		: m_size(0)
		, m_memory(NULL)
		, m_hand(NULL)
		, m_key(key)
	{
		setKey(key, bChangeName);
	}

	~QcSharedMemory()
	{
		detach();
	}

	void setKey2(const std::wstring &key, const std::wstring& muxKey)
	{
		m_mutex.init(muxKey.c_str(), false);
		m_key = key;
		m_bChangeName = false;
	}

	void setKey(const std::wstring &key, bool bChangeName = true)
	{
		m_mutex.init(key.c_str(), true);
		m_key = key;
		m_bChangeName = bChangeName;
	}

	const std::wstring& key() const
	{
		return m_key;
	}

	bool create(int size, AccessMode mode)
	{
		if (size <= 0)
			return false;

		if (!_create(size))
			return false;

		return attach(mode);
	}
	bool attach(AccessMode mode)
	{
		HANDLE hHandle = _handle(0);
		if (hHandle == NULL)
		{
			QmLogTimeOnce2(1000 * 60, "_handle is NULL err=%d", GetLastError());
			return false;
		}

		int permissions = (mode == QcSharedMemory::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE);
		m_memory = (void *)MapViewOfFile(hHandle, permissions, 0, 0, 0);
		if (0 == m_memory)
		{
			_cleanHandle();
			QmLogTimeOnce2(1000 * 60, "MapViewOfFile is NULL err=%d", GetLastError());
			return false;
		}

		// Grab the size of the memory we have been given (a multiple of 4K on windows)
		MEMORY_BASIC_INFORMATION info;
		if (!VirtualQuery(m_memory, &info, sizeof(info))) {
			// Windows doesn't set an error code on this one,
			// it should only be a kernel memory error.
			//error = QSharedMemory::UnknownError;
			//errorString = QSharedMemory::tr("%1: size query failed").arg(QLatin1String("QSharedMemory::attach: "));
			QmLogTimeOnce2(1000 * 60, "VirtualQuery is NULL err=%d", GetLastError());
			return false;
		}
		m_size = (uint32_t)info.RegionSize;
		return true;
	}

	bool isAttached() const
	{
		return (0 != m_memory);
	}
	bool detach()
	{
		if (m_memory)
		{
			::UnmapViewOfFile(m_memory);
			m_memory = NULL;
		}
		return _cleanHandle();
	}

	int size() const { return m_size; }
	void* data(){ return m_memory; }
	const void* data() const{ return m_memory; }
	bool lock()
	{
		m_mutex.lock();
		return true;
	}

	bool unlock()
	{
		m_mutex.unLock();
		return true;
	}
	HANDLE handle() const
	{
		return m_hand;
	}
protected:
	bool _create(size_t size)
	{
		return _handle(size) != NULL;
	}
	HANDLE _handle(size_t size)
	{
		if (!m_hand)
		{
			if (m_key.empty())
				return false;

			std::wstring key = m_key;
			if (m_bChangeName)
				key += L"_SharedMemory";

			if (size == 0)
				m_hand = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, key.c_str());
			else
			{
				HANDLE hHandle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, (DWORD)size, (wchar_t*)key.c_str());
				if (hHandle && GetLastError() == ERROR_ALREADY_EXISTS)
				{
					CloseHandle(hHandle);
					hHandle = NULL;
				}
				else
				{
					m_hand = hHandle;
				}
			}
		}
		return m_hand;
	}

	bool _cleanHandle()
	{
		if (m_hand != 0 && !CloseHandle(m_hand)) {
			m_hand = 0;
			return false;
		}
		m_hand = 0;
		return true;
	}
private:
	QcSharedMemory(const QcSharedMemory &);
	QcSharedMemory &operator=(const QcSharedMemory &);

	std::wstring m_key;
	bool		m_bChangeName;
	HANDLE       m_hand;
	DWORD        m_size;
	void*        m_memory;
	QcMutex		 m_mutex;
};


class QcAutoSharedMemoryLock
{
public:
	inline QcAutoSharedMemoryLock(QcSharedMemory *sharedMemory) : q_sm(sharedMemory)
	{
		q_sm->lock();
	}
	inline ~QcAutoSharedMemoryLock()
	{
		q_sm->unlock();
	}
private:
	QcSharedMemory *q_sm;
};
