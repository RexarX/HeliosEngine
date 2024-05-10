#pragma once

#include <chrono>

namespace VoxelEngine
{
	class Timer
	{
	public:
		void Start();
		void Stop();
		double GetElapsedSec();

	private:
		std::chrono::high_resolution_clock::time_point m_TimeStamp;
		std::chrono::duration<double> m_Elapsed = std::chrono::duration<double>(0.0);
	};
}