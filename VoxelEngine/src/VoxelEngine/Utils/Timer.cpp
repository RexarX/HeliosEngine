#include "Timer.h"

namespace Engine
{
	void Timer::Start()
	{
		m_TimeStamp = std::chrono::steady_clock::now();
	}

	void Timer::Stop()
	{
		auto now = std::chrono::steady_clock::now();
    m_Elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_TimeStamp);
	}

	const double Timer::GetElapsedSec() const
	{
		return m_Elapsed.count() / 1000000.0;
	}

	const double Timer::GetElapsedMillisec() const
	{
    return m_Elapsed.count() / 1000.0;
	}

	const uint64_t Timer::GetElapsedMicrosec() const
  {
		return m_Elapsed.count();
  }
}