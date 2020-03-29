#pragma once

struct IRef
{
	virtual int addRef() = 0;
	virtual int release() = 0;
};