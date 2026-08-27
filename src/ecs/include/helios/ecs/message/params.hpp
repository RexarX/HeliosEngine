#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/cursor.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/world.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class AccessPolicy;
class AccessPolicyBuilder;

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
#endif  // HELIOS_MODULE_CONSUMER_SHIM
