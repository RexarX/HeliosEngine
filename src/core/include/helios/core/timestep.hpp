#pragma once

namespace helios {

/**
 * @brief Represents a time step in seconds.
 * @details Provides utility functions to convert to milliseconds and framerate.
 */
class Timestep {
public:
  /**
   * @brief Default constructor, initializes time step to 0.
   */
  constexpr Timestep() noexcept = default;

  /**
   * @brief Constructs a Timestep with the given time in seconds.
   * @param time Time in seconds.
   */
  explicit constexpr Timestep(float time) noexcept : time_(time) {}
  constexpr Timestep(const Timestep&) noexcept = default;
  constexpr Timestep(Timestep&&) noexcept = default;
  constexpr ~Timestep() noexcept = default;

  constexpr Timestep& operator=(const Timestep&) noexcept = default;
  constexpr Timestep& operator=(Timestep&&) noexcept = default;

  explicit constexpr operator float() const noexcept { return time_; }

  /**
   * @brief Gets the time step in seconds.
   * @return Time step in seconds.
   */
  [[nodiscard]] constexpr float Sec() const noexcept { return time_; }

  /**
   * @brief Gets the time step in milliseconds.
   * @return Time step in milliseconds.
   */
  [[nodiscard]] constexpr float MilliSec() const noexcept { return time_ * 1000.0F; }

  /**
   * @brief Gets the framerate corresponding to the time step.
   * @return Framerate (frames per second).
   */
  [[nodiscard]] constexpr float Framerate() const noexcept { return 1.0F / time_; }

private:
  float time_ = 0.0F;
};

}  // namespace helios
