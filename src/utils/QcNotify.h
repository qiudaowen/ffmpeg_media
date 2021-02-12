#pragma once

#include <functional>
#include <vector>
#include <mutex>

template <class NotifyCb>
class QcNotify
{
	std::vector<NotifyCb> m_cbList;

	std::recursive_mutex m_mutex;
public:
	int32_t addNotify(NotifyCb cb)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		for (int i = 0; i < (int)m_cbList.size(); ++i) {
			if (m_cbList[i] == nullptr)
			{
				m_cbList[i] = cb;
				return i + 1;
			}
		}
		m_cbList.push_back(cb);
		return m_cbList.size();
	}
	void removeNotify(int32_t id)
	{
		id -= 1;
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		if (id >= 0 && id < (int32_t)m_cbList.size())
		{
			m_cbList[id] = nullptr;
		}
	}

	template <class ...Args>
	void invoke(Args... args)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		for (auto cb : m_cbList)
		{
			if (cb)
				cb(std::forward<Args>(args)...);
		}
	}
};