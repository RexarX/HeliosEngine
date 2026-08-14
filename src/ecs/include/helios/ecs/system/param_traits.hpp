#pragma once

#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/local_param.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/system_param.hpp>
#include <helios/ecs/world.hpp>
#include <helios/ecs/world_view.hpp>

#include <concepts>
#include <optional>
#include <tuple>

namespace helios::ecs {

namespace details {

/// @brief Helper to register query component access from a component tuple.
template <typename Tuple>
struct RegisterQueryAccess;

template <typename... Cs>
struct RegisterQueryAccess<std::tuple<Cs...>> {
  static constexpr void Apply(AccessPolicyBuilder& builder) {
    builder.Query<Cs...>();
  }
};

}  // namespace details

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

template <QueryArg... Args>
struct SystemParamTraits<Query<Args...>> {
  using ParamType = Query<Args...>;
  using Split = details::QueryArgSplit<Args...>;
  using Allocator = typename ParamType::allocator_type;

  static ParamType Make(World& world, SystemLocalData& data,
                        const AccessPolicy& /*policy*/) {
    return ParamType(world.Components(), &data.allocator);
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    details::RegisterQueryAccess<typename Split::Components>::Apply(builder);
  }
};

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

template <ResourceTrait T>
struct SystemParamTraits<Res<const T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept -> Res<const T> {
    return Res<const T>(world.ReadResource<T>());
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.ReadResources<T>();
    }
  }
};

template <ResourceTrait T>
struct SystemParamTraits<Res<T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept -> Res<T> {
    return Res<T>(world.WriteResource<T>());
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.WriteResources<T>();
    }
  }
};

template <ResourceTrait T>
struct SystemParamTraits<std::optional<Res<const T>>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept
      -> std::optional<Res<const T>> {
    if (const T* ptr = world.TryReadResource<T>()) {
      return Res<const T>(*ptr);
    }
    return std::nullopt;
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.ReadResources<T>();
    }
  }
};

template <ResourceTrait T>
struct SystemParamTraits<std::optional<Res<T>>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept
      -> std::optional<Res<T>> {
    if (T* ptr = world.TryWriteResource<T>()) {
      return Res<T>(*ptr);
    }
    return std::nullopt;
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.WriteResources<T>();
    }
  }
};

// ---------------------------------------------------------------------------
// Local resources
// ---------------------------------------------------------------------------

template <ResourceTrait T>
  requires std::default_initializable<T>
struct SystemParamTraits<Local<const T>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept -> Local<const T> {
    data.resource_manager.TryEmplace<T>();
    return Local<const T>(data.resource_manager.Get<T>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <ResourceTrait T>
  requires std::default_initializable<T>
struct SystemParamTraits<Local<T>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept -> Local<T> {
    data.resource_manager.TryEmplace<T>();
    return Local<T>(data.resource_manager.Get<T>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

// Special case for `LocalArena` to ensure it is always available in system
// local data. The `LocalArena` is constructed with the system's arena
// allocator, allowing for efficient temporary allocations during system
// execution.

template <>
struct SystemParamTraits<Local<const LocalArena>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> Local<const LocalArena> {
    data.resource_manager.TryEmplace<LocalArena>(data.allocator);
    return Local<const LocalArena>(data.resource_manager.Get<LocalArena>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <>
struct SystemParamTraits<Local<LocalArena>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> Local<LocalArena> {
    data.resource_manager.TryEmplace<LocalArena>(data.allocator);
    return Local<LocalArena>(data.resource_manager.Get<LocalArena>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

template <>
struct SystemParamTraits<Commands> {
  static constexpr Commands Make(World& world, SystemLocalData& data,
                                 const AccessPolicy& /*policy*/) {
    return {data.cmd_queue, world, &data.allocator};
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

template <>
struct SystemParamTraits<World> {
  static constexpr World& Make(World& world, SystemLocalData& /*data*/,
                               const AccessPolicy& /*policy*/) noexcept {
    return world;
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    builder.Exclusive();
  }
};

// ---------------------------------------------------------------------------
// WorldView
// ---------------------------------------------------------------------------

template <>
struct SystemParamTraits<WorldView> {
  static constexpr WorldView Make(World& world, SystemLocalData& /*data*/,
                                  const AccessPolicy& /*policy*/) noexcept {
    return WorldView(world);
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

// ---------------------------------------------------------------------------
// Messages
// ---------------------------------------------------------------------------

template <MessageTrait T>
struct SystemParamTraits<MessageReader<T>> {
  static auto Make(World& world, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> MessageReader<T> {
    data.resource_manager.template TryEmplace<MessageCursor<T>>();
    return world.ReadMessages<T>(
        data.resource_manager.template Get<MessageCursor<T>>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <ConsumableMessageTrait T>
struct SystemParamTraits<ConsumableMessageReader<T>> {
  static auto Make(World& world, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> ConsumableMessageReader<T> {
    data.resource_manager.template TryEmplace<MessageCursor<T>>();
    return ConsumableMessageReader<T>(
        world.Messages(),
        data.resource_manager.template Get<MessageCursor<T>>(),
        data.consumed_messages);
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <MessageTrait T>
struct SystemParamTraits<MessageWriter<T>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> MessageWriter<T> {
    if (!data.message_queue.IsRegistered<T>()) {
      data.message_queue.Register<T>();
    }
    return MessageWriter<T>(data.message_queue);
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <MessageTrait T, typename Allocator>
struct SystemParamTraits<BasicMessageWriter<T, Allocator>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> BasicMessageWriter<T, Allocator> {
    if (!data.message_queue.IsRegistered<T>()) {
      data.message_queue.Register<T>();
    }
    return BasicMessageWriter<T, Allocator>(data.message_queue);
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <AsyncMessageTrait T>
struct SystemParamTraits<AsyncMessageReader<T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept
      -> AsyncMessageReader<T> {
    return world.ReadAsyncMessages<T>();
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <AsyncMessageTrait T>
struct SystemParamTraits<AsyncMessageWriter<T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept
      -> AsyncMessageWriter<T> {
    return world.WriteAsyncMessages<T>();
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

}  // namespace helios::ecs
