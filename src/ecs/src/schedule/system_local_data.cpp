#include <pch.hpp>

#include <helios/ecs/schedule/system_local_data.hpp>

#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/schedule/local_arena.hpp>
#include <helios/ecs/world.hpp>

#include <memory>
#include <utility>

namespace helios::ecs {

namespace {

[[nodiscard]] size_t ArenaCapacityForMove(
    const mem::ArenaAllocator& allocator) {
  const size_t capacity = allocator.InitialCapacity();
  return capacity != 0 ? capacity
                       : SystemLocalDataOptions::kDefaultPreallocatedSize;
}

}  // namespace

SystemLocalData::SystemLocalData(SystemLocalData&& other) noexcept
    : allocator(ArenaCapacityForMove(other.allocator)),
      cmd_queue(&allocator),
      message_queue(&allocator),
      consumed_messages(&allocator) {
  // Do not steal `other.allocator`: PMR queues bind the *object* address as
  // their resource. Moving the arena leaves those vectors with inverted
  // `_M_start`/`_M_finish` (UBSan: `vector::size` / reverse iterators).
  cmd_queue.Merge(std::move(other.cmd_queue));
  message_queue.Merge(std::move(other.message_queue));
  consumed_messages.MergeFrom(std::move(other.consumed_messages));
  resource_manager = std::move(other.resource_manager);
  AddLocalArena();
  other.ReleaseMovedFrom();
}

SystemLocalData& SystemLocalData::operator=(SystemLocalData&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  cmd_queue.Clear();
  message_queue.ClearAll();
  consumed_messages.Clear();
  resource_manager.Clear();

  cmd_queue.Merge(std::move(other.cmd_queue));
  message_queue.Merge(std::move(other.message_queue));
  consumed_messages.MergeFrom(std::move(other.consumed_messages));
  resource_manager = std::move(other.resource_manager);
  AddLocalArena();
  other.ReleaseMovedFrom();

  return *this;
}

void SystemLocalData::Apply(World& world, bool apply_commands,
                            bool merge_messages) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::SystemLocalData::Apply");

  if (apply_commands) {
    ExecuteCommands(world);
  }
  if (merge_messages) {
    MergeMessages(world);
  }
  if (!HasPendingWork()) {
    ResetArena();
  }
}

void SystemLocalData::MergeMessages(World& world) {
  auto& message_manager = world.Messages();

  if (!consumed_messages.Empty()) {
    message_manager.ApplyConsumed(consumed_messages);
  }
  consumed_messages.Clear();

  message_manager.MergeLocalMessages(std::move(message_queue));
  message_queue.ClearAll();
}

void SystemLocalData::ResetArena() noexcept {
  std::destroy_at(&cmd_queue);
  std::destroy_at(&message_queue);
  std::destroy_at(&consumed_messages);

  allocator.Reset();

  std::construct_at(&cmd_queue, &allocator);
  std::construct_at(&message_queue, &allocator);
  std::construct_at(&consumed_messages, &allocator);
}

void SystemLocalData::AddLocalArena() noexcept {
  resource_manager.Insert(LocalArena{allocator});
}

void SystemLocalData::ReleaseMovedFrom() noexcept {
  Reconstruct(cmd_queue, &allocator);
  Reconstruct(message_queue, &allocator);
  Reconstruct(consumed_messages, &allocator);
  resource_manager.Clear();
}

}  // namespace helios::ecs
