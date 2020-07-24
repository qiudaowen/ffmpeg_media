#pragma once

#include <windows.h>
#include <functional>

struct QsMonitorInfo
{
	HMONITOR hMonitor;
	HDC dc; 
	LPRECT rect;
};

using MonitorCb = std::function<bool(const QsMonitorInfo&)>;
inline void enumMonitors(MonitorCb && cb)
{
	EnumDisplayMonitors(NULL, NULL, static_cast<MONITORENUMPROC>([](HMONITOR hMonitor, HDC dc, LPRECT rect, LPARAM para)->BOOL {
		MonitorCb cb = *(MonitorCb*)para;

		QsMonitorInfo info = { hMonitor, dc, rect};
		return cb(info);
	}), (LPARAM)&cb);
}

inline bool getMonitorInfo(int index, QsMonitorInfo& info)
{
	if (index < 0)
		return false;

	enumMonitors([&](const QsMonitorInfo& monitorInfo)->bool {
		if (--index < 0)
		{
			info = monitorInfo;
			return false;
		}
		return true;
	});

	return index < 0;
}