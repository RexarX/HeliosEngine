#pragma once

#include "Core.h"

#include <chrono>

namespace Helios::Utils {
	class HELIOSENGINE_API Timer {
	public:
		using clock = std::chrono::steady_clock;

		Timer() noexcept = default;

		void Start() noexcept {
			m_TimeStamp = clock::now();
			m_IsRunning = true;
		}

		void Stop() noexcept {
			if (!m_IsRunning) { CORE_ASSERT(false, "Timer is not running!"); return; }
			m_Elapsed = clock::now() - m_TimeStamp;
		}

		template <typename Type = clock::duration::rep, typename Units = std::chrono::nanoseconds>
		requires std::is_arithmetic_v<Type> && std::chrono::_Is_duration_v<Units>
		inline Type GetElapsed() const noexcept {
			return static_cast<Type>(std::chrono::duration_cast<Units>(m_Elapsed).count());
		}

		inline double GetElapsedSec() const noexcept {
			return GetElapsed<double, std::chrono::duration<double>>();
		}

    inline double GetElapsedMilliSec() const noexcept {
      return GetElapsed<double, std::chrono::duration<double, std::milli>>();
    }

    inline uint64_t GetElapsedMicroSec() const noexcept {
      return GetElapsed<uint64_t, std::chrono::microseconds>();
    }

    inline uint64_t GetElapsedNanoSec() const noexcept {
      return GetElapsed<uint64_t, std::chrono::nanoseconds>();
    }

	private:
		clock::time_point m_TimeStamp;
		clock::duration m_Elapsed = clock::duration::zero();
		bool m_IsRunning = false;
	};
}