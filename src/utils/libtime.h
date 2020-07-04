#pragma once

#include <chrono>

namespace libtime
{
	using namespace std::chrono;
	class Time
	{
	public:
		Time()
		{
			restart();
		}
		void restart()
		{
			m_startTime = steady_clock::now();
		}
		uint32_t elapsedMS(bool bRestart = false)
		{
			auto diff = elapsed(bRestart);
			return duration_cast<milliseconds>(diff).count();
		}
		steady_clock::duration elapsed(bool bRestart = false)
		{
			auto ret = steady_clock::now() - m_startTime;
			if (bRestart)
				restart();
			return ret;
		}
	protected:
		steady_clock::time_point m_startTime;
	};

	class FpsTimer
	{
	public:
		FpsTimer()
		{
			reset();
		}
		void reset()
		{
			m_frames = 0;
			m_fps = 0.0;
			m_startTime = steady_clock::now();
		}
		bool tick()
		{
			++m_frames;
			auto now = steady_clock::now();
			steady_clock::duration diff = now - m_startTime;
			auto delta = duration_cast<milliseconds>(diff).count();
			if (delta >= 1000)
			{
				m_fps = m_frames * 1000.0/delta;
				m_startTime = now;
				m_frames = 0;
				return true;
			}
			return false;
		}
		double fps() const
		{
			return m_fps;
		}
	protected:
		steady_clock::time_point m_startTime;
		uint32_t m_frames = 0;
		double m_fps = 0;
	};
}