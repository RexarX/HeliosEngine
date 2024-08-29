#pragma once

#include "Core.h"

#include <chrono>

namespace Helios::Utils
{
	class HELIOSENGINE_API Timer
	{
	public:
		using steady_clock = std::chrono::steady_clock;

		void Start() {
			m_TimeStamp = steady_clock::now();
		}

		void Stop() {
			m_Elapsed = steady_clock::now() - m_TimeStamp;
		}

		inline double GetElapsedSec() const {
			return GetElapsed<std::chrono::duration<double>>();
		}

		inline double GetElapsedMillisec() const {
			return GetElapsed<std::chrono::duration<double, std::milli>>();
		}

		inline uint64_t GetElapsedMicrosec() const {
			return GetElapsed<std::chrono::microseconds>();
		}

		inline uint64_t GetElapsedNanosec() const {
			return GetElapsed<std::chrono::nanoseconds>();
		}

	private:
		template<typename Duration>
		inline auto GetElapsed() const {
			return std::chrono::duration_cast<Duration>(m_Elapsed).count();
		}

	private:
		steady_clock::time_point m_TimeStamp;
		steady_clock::duration m_Elapsed = steady_clock::duration::zero();
	};
}