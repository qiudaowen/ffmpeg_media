#pragma once

#include <string>
#include "libtime.h"

class LogTimeElapsed
{
public:
	LogTimeElapsed(std::wstring&& name)
		: m_name(std::move(name))
	{

	}
	~LogTimeElapsed()
	{
		std::wstring sLog = m_name + L"=" + std::to_wstring(m_time.elapsedMS()) + L"\n";
		OutputDebugStringW(sLog.c_str());
	}
protected:
	std::wstring m_name;
	libtime::Time m_time;
};