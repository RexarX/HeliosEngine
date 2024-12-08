#pragma once

#include "Core.h"

#include <chrono>
#include <type_traits>

namespace Helios::Utils {
	class HELIOSENGINE_API Timer {
	public:
		Timer() noexcept = default;
		~Timer() noexcept = default;

		void Start() noexcept {
			m_TimeStamp = std::chrono::steady_clock::now();
			m_IsRunning = true;
		}

		void Stop() noexcept {
			if (!m_IsRunning) { CORE_ASSERT(false, "Timer is not running!"); return; }
			m_Elapsed = std::chrono::steady_clock::now() - m_TimeStamp;
		}

		template <typename Type = std::chrono::steady_clock::duration::rep, typename Units = std::chrono::nanoseconds>
		requires std::is_arithmetic_v<Type> && std::is_same_v<Units, std::chrono::duration<typename Units::rep, typename Units::period>>
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
		std::chrono::steady_clock::time_point m_TimeStamp;
		std::chrono::steady_clock::duration m_Elapsed = std::chrono::steady_clock::duration::zero();
		bool m_IsRunning = false;
	};
}