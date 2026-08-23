#pragma once

#include <helios/assert.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <format>
#include <iterator>
#include <ostream>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

namespace helios::input {

/**
 * @brief Concept for axis enums used by `Axis`.
 * @details Requires an enum type with a contiguous `kCount` sentinel.
 * @tparam T Axis enum type
 */
template <typename T>
concept AxisTrait = std::is_enum_v<T> && requires {
  { std::to_underlying(T::kCount) } -> std::convertible_to<size_t>;
};

/// @brief Deadzone / livezone / rescale parameters for analog axes.
struct AxisFilter {
  static constexpr float kDefaultDeadzone = 0.15F;
  static constexpr float kDefaultTriggerDeadzone = 0.05F;
  static constexpr float kDefaultLivezone = 1.0F;

  float deadzone = kDefaultDeadzone;
  float livezone = kDefaultLivezone;
  bool rescale = true;
};

/**
 * @brief Stores continuous float values for axis enums.
 * @tparam T Axis enum satisfying `AxisTrait`
 */
template <AxisTrait T>
class Axis {
public:
  static constexpr auto kSize =
      static_cast<size_t>(std::to_underlying(T::kCount));

  /// @brief Zeros all axis values.
  constexpr void Clear() noexcept { values_.fill(0.0F); }

  /**
   * @brief Sets an axis value.
   * @param axis Axis to update
   * @param value New axis value
   */
  constexpr void Set(T axis, float value) noexcept {
    values_[Index(axis)] = value;
  }

  /**
   * @brief Sets an axis value with a simple deadzone.
   * @param axis Axis to update
   * @param value Raw axis value
   * @param deadzone Absolute threshold below which the value becomes zero
   */
  constexpr void SetWithDeadzone(T axis, float value, float deadzone) noexcept;

  /**
   * @brief Reads an axis value.
   * @param axis Axis to read
   * @return Current axis value
   */
  [[nodiscard]] constexpr float Get(T axis) const noexcept {
    return values_[Index(axis)];
  }

  /**
   * @brief Returns all axis values.
   * @return Contiguous span of axis values
   */
  [[nodiscard]] constexpr auto Values() const noexcept
      -> std::span<const float> {
    return values_;
  }

private:
  /**
   * @brief Converts an axis enum to an array index.
   * @param axis Axis value
   * @return Zero-based index into `values_`
   * @warning Asserts when `axis` is out of range (`>= kCount`).
   */
  [[nodiscard]] static constexpr size_t Index(T axis) noexcept;

  std::array<float, kSize> values_{};
};

template <AxisTrait T>
constexpr void Axis<T>::SetWithDeadzone(T axis, float value,
                                        float deadzone) noexcept {
  if (std::fabs(value) < deadzone) {
    Set(axis, 0.0F);
    return;
  }
  Set(axis, value);
}

template <AxisTrait T>
constexpr size_t Axis<T>::Index(T axis) noexcept {
  const auto underlying = std::to_underlying(axis);
  HELIOS_ASSERT(underlying < std::to_underlying(T::kCount));
  return static_cast<size_t>(underlying);
}

/**
 * @brief Applies a 1D deadzone and optional rescale.
 * @details Values with magnitude below `deadzone` become `0`. Magnitudes at or
 * above `livezone` saturate to `+/- 1` when `rescale` is set, otherwise to
 * `+/- livezone`. Between the two, `rescale` maps the remaining range onto
 * `[0, 1]` (preserving sign).
 * @param value Axis value, typically in `[-1, 1]` or `[0, 1]`
 * @param deadzone Inner cutoff (exclusive)
 * @param livezone Outer saturate magnitude
 * @param rescale Remap `(deadzone, livezone)` onto `(0, 1)`
 * @return Filtered value
 */
[[nodiscard]] constexpr float ApplyLinearDeadzone(float value, float deadzone,
                                                  float livezone,
                                                  bool rescale) noexcept {
  if (livezone <= deadzone) {
    return 0.0F;
  }

  const float mag = std::fabs(value);
  if (mag < deadzone) {
    return 0.0F;
  }

  const float sign = (value < 0.0F) ? -1.0F : 1.0F;
  if (mag >= livezone) {
    return rescale ? sign : sign * livezone;
  }

  if (!rescale) {
    return value;
  }
  return sign * (mag - deadzone) / (livezone - deadzone);
}

/**
 * @brief Applies `ApplyLinearDeadzone` using an `AxisFilter`.
 * @param value Axis value
 * @param filter Deadzone settings
 * @return Filtered value
 */
[[nodiscard]] constexpr float ApplyLinearDeadzone(
    float value, const AxisFilter& filter) noexcept {
  return ApplyLinearDeadzone(value, filter.deadzone, filter.livezone,
                             filter.rescale);
}

/**
 * @brief Applies a circular (radial) deadzone to a stick pair.
 * @details If the vector length is below `deadzone`, returns `(0, 0)`. At or
 * above `livezone`, `rescale` saturates onto the unit circle; otherwise the
 * vector is scaled to length `livezone`. Between the two, `rescale` maps
 * length onto `(0, 1]` along the same direction.
 * @note When `livezone <= deadzone`, the whole range is treated as dead.
 * @param x X component
 * @param y Y component
 * @param deadzone Inner radial cutoff
 * @param livezone Outer radial saturate length
 * @param rescale Remap `(deadzone, livezone)` onto `(0, 1]`
 * @return Filtered `(x, y)` pair
 */
[[nodiscard]] constexpr auto ApplyRadialDeadzone(float x, float y,
                                                 float deadzone, float livezone,
                                                 bool rescale)
    -> std::pair<float, float> {
  if (livezone <= deadzone) {
    return {0.0F, 0.0F};
  }

  const float len = std::hypot(x, y);
  if (len < deadzone || len == 0.0F) {
    return {0.0F, 0.0F};
  }

  float scale = 1.0F;
  if (len >= livezone) {
    scale = rescale ? (1.0F / len) : (livezone / len);
  } else if (rescale) {
    scale = (len - deadzone) / (livezone - deadzone) / len;
  }

  return {x * scale, y * scale};
}

/**
 * @brief Applies `ApplyRadialDeadzone` using an `AxisFilter`.
 * @param x X component
 * @param y Y component
 * @param filter Deadzone settings
 * @return Filtered `(x, y)` pair
 */
[[nodiscard]] constexpr auto ApplyRadialDeadzone(float x, float y,
                                                 const AxisFilter& filter)
    -> std::pair<float, float> {
  return ApplyRadialDeadzone(x, y, filter.deadzone, filter.livezone,
                             filter.rescale);
}

/**
 * @brief Remaps a GLFW-style trigger axis to `[0, 1]`.
 * @details GLFW rest is `-1` and fully pressed is `+1`. `center` is the
 * calibrated rest value: `t = (raw - center) / (1 - center)`.
 * @param raw Raw trigger value
 * @param center Rest-center (typically `-1`)
 * @return Value clamped to `[0, 1]`, or `0` when `center >= 1`
 */
[[nodiscard]] constexpr float RemapTrigger(float raw, float center) noexcept {
  const float denom = 1.0F - center;
  if (denom <= 0.0F) {
    return 0.0F;
  }
  return std::clamp((raw - center) / denom, 0.0F, 1.0F);
}

/**
 * @brief Formats axis filter parameters using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param filter Axis filter
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const AxisFilter& filter) {
  return std::format_to(out,
                        "AxisFilter{{deadzone={}, livezone={}, rescale={}}}",
                        filter.deadzone, filter.livezone, filter.rescale);
}

/**
 * @brief Formats axis filter parameters as a string.
 * @param filter Axis filter
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const AxisFilter& filter) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), filter);
  return result;
}

/**
 * @brief Formats axis filter parameters as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param filter Axis filter
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const AxisFilter& filter) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), filter);
  return result;
}

/**
 * @brief Outputs axis filter parameters to an output stream.
 * @param os Output stream
 * @param filter Axis filter
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const AxisFilter& filter) {
  ToString(std::ostreambuf_iterator<char>(os), filter);
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::AxisFilter> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::AxisFilter& filter,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), filter);
  }
};

}  // namespace std
