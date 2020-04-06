#pragma once

#include <objbase.h>
struct QcComInit
{
	QcComInit(int flag = COINIT_MULTITHREADED)
	{
		CoInitializeEx(NULL, flag);
	}
	~QcComInit()
	{
		CoUninitialize();
	}
};
#define QmComInit() QcComInit QmUniqueVarName;