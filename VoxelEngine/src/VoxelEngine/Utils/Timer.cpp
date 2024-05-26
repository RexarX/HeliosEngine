#include "Timer.h"

namespace VoxelEngine
{
	void Timer::Start()
	{
		m_TimeStamp = std::chrono::high_resolution_clock::now();
	}

	void Timer::Stop()
	{
		m_Elapsed = std::chrono::high_resolution_clock::now() - m_TimeStamp;
	}

	double Timer::GetElapsedSec()
	{
		return m_Elapsed.count();
	}
}