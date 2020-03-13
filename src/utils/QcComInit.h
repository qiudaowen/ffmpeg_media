#pragma once

#include <objbase.h>
struct QcComInit
{
	QcComInit()
	{
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}
	~QcComInit()
	{
		CoUninitialize();
	}
};
#define QmComInit() QcComInit QmUniqueVarName;