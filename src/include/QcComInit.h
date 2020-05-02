#pragma once

#include <objbase.h>
struct QcComInit
{
	QcComInit(int flag = COINIT_MULTITHREADED)
        : m_bInitialized(SUCCEEDED(CoInitializeEx(NULL, flag)))
	{
	}
	~QcComInit()
	{
        if (m_bInitialized)
		    CoUninitialize();
	}
protected:
    const bool m_bInitialized;
};
#define QmComInit() QcComInit QmUniqueVarName;