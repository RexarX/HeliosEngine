#pragma once

#include "Core.h"

#include <random>
#include <numeric>
#include <type_traits>

namespace Helios::Utils {
  class HELIOSENGINE_API Random {
  public:
    template<typename T> requires std::is_arithmetic_v<T>
    static T GetValue() {
      if constexpr (std::is_integral_v<T>) {
        if constexpr (std::is_same_v<T, bool>) {
          std::uniform_int_distribution<int> distribution(0, 1);
          return distribution(m_Engine) == 1;
        } else {
          std::uniform_int_distribution<T> distribution(
            std::numeric_limits<T>::min(),
            std::numeric_limits<T>::max()
          );
          return distribution(m_Engine);
        }
      } else {
        std::uniform_real_distribution<T> distribution(
          std::numeric_limits<T>::min(),
          std::numeric_limits<T>::max()
        );
        return distribution(m_Engine);
      }
    }
    
    template<typename T, typename U> requires std::is_arithmetic_v<T> && std::is_arithmetic_v<U>
    static auto GetValueFromRange(T min, U max) {
      using CommonType = std::common_type_t<T, U>;

      if constexpr (std::is_integral_v<CommonType>) {
        using DistType = std::conditional_t<
          std::is_signed_v<CommonType>,
          int64_t,
          uint64_t
        >;
        std::uniform_int_distribution<DistType> distribution(
          static_cast<DistType>(min),
          static_cast<DistType>(max)
        );
        return distribution(m_Engine);
      } else {
        std::uniform_real_distribution<CommonType> distribution(
          static_cast<CommonType>(min),
          static_cast<CommonType>(max)
        );
        return distribution(m_Engine);
      }
    }

  private:
    static inline std::random_device m_RandomDevice;
    static inline std::mt19937_64 m_Engine = std::mt19937_64(m_RandomDevice());
  };
}