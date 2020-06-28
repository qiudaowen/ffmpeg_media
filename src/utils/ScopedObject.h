#pragma once

#include <functional>

class ScopedObject
{
public:
	ScopedObject(std::function<void()>&& endFun)
		: m_endFun(std::move(endFun))
	{

	}
	~ScopedObject()
	{
		m_endFun();
	}
protected:
	std::function<void()> m_endFun;
};