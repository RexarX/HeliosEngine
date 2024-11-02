#pragma once

#include "Core.h"

#include <random>

namespace Helios::Utils
{
  class HELIOSENGINE_API Random
  {
  public:
    template<typename T> requires std::is_integral<T>::value || std::is_floating_point<T>::value
    static T GetValue() {
      if constexpr (std::is_integral<T>::value) {
        std::uniform_int_distribution<T> distribution(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        return distribution(m_Engine);
      } else {
        std::uniform_real_distribution<T> distribution(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        return distribution(m_Engine);
      }
    }
    
    template<typename T> requires std::is_integral<T>::value || std::is_floating_point<T>::value
    static T GetValueFromRange(T min, T max) {
      if constexpr (std::is_integral<T>::value) {
        std::uniform_int_distribution<T> distribution(min, max);
        return distribution(m_Engine);
      } else {
        std::uniform_real_distribution<T> distribution(min, max);
        return distribution(m_Engine);
      }
    }

  private:
    static std::random_device m_RandomDevice;
    static std::mt19937_64 m_Engine;
  };
}