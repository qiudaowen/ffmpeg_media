#pragma once

template <class T>
struct QcAtomicPointer
{
	T * p;
	volatile long lock;
	~QcAtomicPointer()
	{
		release();
	}
	void release()
	{
		while (lock == 1){}// busy locking while init
		if (::InterlockedCompareExchange(&lock, 3, 2) == 2)
		{
			//QmLogFinal << "Begin: " << __FUNCTION__;
			delete p;
			p = NULL;

			//QmLogFinal << "End: " << __FUNCTION__;
		}
	}
	inline T * get()
	{
		if (lock == 2 || lock == 3) // short path
			return p;

		if (::InterlockedCompareExchange(&lock, 1, 0) == 0)
		{
			p = new T;
			::InterlockedExchange(&lock, 2);
		}
		else
		{
			while (lock == 1) Sleep(0); // busy locking while init
		}
		return p;
	}
	inline operator T *() { return get(); }
	inline T *operator->() { return get(); }
};