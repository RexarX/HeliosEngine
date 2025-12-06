#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/logger.hpp>

#include <concepts>
#include <memory>
#include <utility>

namespace helios::ecs::details {

/**
 * @brief Command buffer to record operations to be executed later.
 * @details All operations are recorded to system local storage and executed
 * in the order they were recorded when World::Update() is called.
 * @note Command buffer is not thread-safe but doesn't need to be since each system has its own local storage.
 */
class CmdBuffer {
public:
  explicit CmdBuffer(SystemLocalStorage& local_storage) noexcept : local_storage_(local_storage) {}
  CmdBuffer(const CmdBuffer&) = delete;
  CmdBuffer(CmdBuffer&&) = delete;
  ~CmdBuffer() noexcept = default;

  CmdBuffer& operator=(const CmdBuffer&) = delete;
  CmdBuffer& operator=(CmdBuffer&&) = delete;

  /**
   * @brief Pushes a pre-constructed command to the buffer.
   * @param command Unique pointer to command
   */
  void Push(std::unique_ptr<Command>&& command);

  /**
   * @brief Constructs and pushes a command to the buffer.
   * @tparam T Command type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to command constructor
   */
  template <CommandTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Args&&... args) {
    local_storage_.EmplaceCommand<T>(std::forward<Args>(args)...);
  }

private:
  SystemLocalStorage& local_storage_;
};

inline void CmdBuffer::Push(std::unique_ptr<Command>&& command) {
  HELIOS_ASSERT(command != nullptr, "Failed to push command to command buffer: command is nullptr!");
  local_storage_.AddCommand(std::move(command));
}

}  // namespace helios::ecs::details
