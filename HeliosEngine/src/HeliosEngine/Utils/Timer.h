#pragma once

#include <chrono>

namespace Helios
{
	using steady_clock = std::chrono::steady_clock;

	class HELIOSENGINE_API Timer
	{
	public:
		void Start() {
      m_TimeStamp = steady_clock::now();
		}

		void Stop() {
			m_Elapsed = steady_clock::now() - m_TimeStamp;
		}

		inline const double GetElapsedSec() const {
			return GetElapsed<std::chrono::duration<double>>();
		}

		inline const double GetElapsedMillisec() const {
			return GetElapsed<std::chrono::duration<double, std::milli>>();
		}

		inline const uint64_t GetElapsedMicrosec() const {
			return GetElapsed<std::chrono::microseconds>();
		}

		inline const uint64_t GetElapsedNanosec() const {
			return GetElapsed<std::chrono::nanoseconds>();
		}

	private:
		template<typename Duration = std::chrono::milliseconds>
		inline const auto GetElapsed() const {
			return std::chrono::duration_cast<Duration>(m_Elapsed).count();
		}

	private:
		steady_clock::time_point m_TimeStamp;
		steady_clock::duration m_Elapsed = steady_clock::duration::zero();
	};
}