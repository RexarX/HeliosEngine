#pragma once

#include <concepts>

namespace helios::ecs {

/**
 * @brief Base class for all commands to be executed in the world.
 * @details Commands are used to modify the world in a thread-safe manner.
 * They are recorded in a command buffer and executed in the order they were recorded when World::Update() is called.
 */
class Command {
public:
  virtual constexpr ~Command() noexcept = default;

  virtual void Execute(class World& world) = 0;
};

template <typename T>
concept CommandTrait = std::derived_from<T, Command>;

}  // namespace helios::ecs
