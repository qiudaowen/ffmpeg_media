#pragma once

#include "QmMacro.h"

template<class Base>
class QcRef : public Base
{
public:
	QcRef():mRefCount(0)
	{}
	virtual ~QcRef()
	{
	}

	virtual int addRef() { return InterlockedIncrement(&mRefCount); }
	virtual int release()
	{ 
		if (InterlockedDecrement(&mRefCount) == 0)
		{
			if (toDelete())
			{
				delete this;
			}
			return 0;
		}
		else
		{
			QmAssert(mRefCount > 0);
		}
			
		return mRefCount;
	}

	virtual bool toDelete(){return true;}
protected:
	volatile long mRefCount;
};