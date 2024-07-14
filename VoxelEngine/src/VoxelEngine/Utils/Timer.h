#pragma once

#include <chrono>

namespace Engine
{
	class Timer
	{
	public:
		void Start();
		void Stop();

		const double GetElapsedSec() const;
		const double GetElapsedMillisec() const;
		const uint64_t GetElapsedMicrosec() const;

	private:
		std::chrono::steady_clock::time_point m_TimeStamp;
		std::chrono::microseconds m_Elapsed = std::chrono::microseconds(0);
	};
}