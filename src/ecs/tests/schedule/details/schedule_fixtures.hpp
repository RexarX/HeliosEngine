#pragma once

#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>
#include <helios/ecs/world_view.hpp>

namespace helios::ecs::schedule_test {

struct SystemAlpha {
  void operator()() {}
};

struct SystemBeta {
  void operator()() {}
};

struct SetOne {};
struct SetTwo {};

struct CounterResource {
  int value = 0;
};

struct FlagResource {
  bool triggered = false;
};

struct IncrementSystem {
  void operator()(Res<CounterResource> counter) const { ++counter->value; }
};

struct SetFlagSystem {
  void operator()(Res<FlagResource> flag) const { flag->triggered = true; }
};

struct ReadOnlySystem {
  void operator()(Res<const CounterResource> /*counter*/) const {}
};

struct ConditionalRunSystem {
  void operator()(Res<CounterResource> counter,
                  Res<const FlagResource> flag) const {
    if (flag->triggered) {
      ++counter->value;
    }
  }
};

struct OrderSystemA {
  int* slot = nullptr;
  int* index = nullptr;

  void operator()(Res<CounterResource> /*counter*/) const {
    *slot = (*index)++;
  }
};

struct OrderSystemB {
  int* slot = nullptr;
  int* index = nullptr;

  void operator()(Res<CounterResource> /*counter*/) const {
    *slot = (*index)++;
  }
};

struct OrderSystemC {
  int* slot = nullptr;
  int* index = nullptr;

  void operator()(Res<CounterResource> /*counter*/) const {
    *slot = (*index)++;
  }
};

// Components for Query scheduling tests.
struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;
};

struct Rotation {
  float angle = 0.0F;
};

// Thread-safe resource for AsyncRes tests.
struct ThreadSafeData {
  static constexpr bool kThreadSafe = true;
  int value = 0;
};

// Regular message for message passing tests.
struct TestMessage {
  int value = 0;
};

// Async message for async message passing tests.
struct AsyncTestEvent {
  static constexpr bool kAsync = true;
  int value = 0;
};

// Systems with Query parameters.
struct ReadPositionSystem {
  void operator()(Query<const Position&> query,
                  Res<CounterResource> counter) const {
    query.ForEach([&counter](const Position& /*pos*/) { ++counter->value; });
  }
};

struct ReadOtherPositionSystem {
  void operator()(Query<const Position&> query,
                  Res<CounterResource> counter) const {
    query.ForEach([&counter](const Position& /*pos*/) { ++counter->value; });
  }
};

struct WritePositionSystem {
  void operator()(Query<Position&> query, Res<CounterResource> counter) const {
    query.ForEach([&counter](Position& pos) {
      pos.x += 1.0F;
      ++counter->value;
    });
  }
};

struct WriteOtherPositionSystem {
  void operator()(Query<Position&> query, Res<CounterResource> counter) const {
    query.ForEach([&counter](Position& pos) {
      pos.x -= 1.0F;
      ++counter->value;
    });
  }
};

struct ReadVelocitySystem {
  void operator()(Query<const Velocity&> query) const {
    query.ForEach([](const Velocity& /*vel*/) {});
  }
};

struct WriteVelocitySystem {
  void operator()(Query<Velocity&> query) const {
    query.ForEach([](Velocity& vel) { vel.x += 1.0F; });
  }
};

struct WriteRotationSystem {
  void operator()(Query<Rotation&> query) const {
    query.ForEach([](Rotation& rot) { rot.angle += 1.0F; });
  }
};

// Systems with AsyncRes parameters.
struct AsyncResWriter {
  void operator()(AsyncRes<ThreadSafeData> data) const { ++data->value; }
};

struct AsyncResOtherWriter {
  void operator()(AsyncRes<ThreadSafeData> data) const { ++data->value; }
};

struct AsyncResReader {
  void operator()(AsyncRes<const ThreadSafeData> data,
                  Res<CounterResource> counter) const {
    counter->value += data->value;
  }
};

// Systems with WorldView parameter.
struct WorldViewSystem {
  void operator()(WorldView view, Res<CounterResource> counter) const {
    counter->value = static_cast<int>(view.EntityCount());
  }
};

struct WorldViewWithQuerySystem {
  void operator()(WorldView /*view*/, Query<const Position&> query,
                  Res<CounterResource> counter) const {
    query.ForEach([&counter](const Position& /*pos*/) { ++counter->value; });
  }
};

// Systems with message parameters.
struct TestMsgWriterSystem {
  void operator()(MessageWriter<TestMessage> writer) const {
    writer.Write(TestMessage{42});
  }
};

struct TestMsgReaderSystem {
  void operator()(MessageReader<TestMessage> reader,
                  Res<CounterResource> counter) const {
    const auto messages = reader.Collect();
    for (const auto& msg : messages) {
      counter->value += msg.value;
    }
  }
};

struct AsyncTestMsgWriterSystem {
  void operator()(AsyncMessageWriter<AsyncTestEvent> writer) const {
    writer.Write(AsyncTestEvent{42});
  }
};

struct AsyncTestMsgReaderSystem {
  void operator()(AsyncMessageReader<AsyncTestEvent> reader,
                  Res<CounterResource> counter) const {
    reader.ForEach(
        [&counter](AsyncTestEvent& event) { counter->value += event.value; });
  }
};

// Systems with combined parameter types.
struct QueryAndResSystem {
  void operator()(Query<Position&> query, Res<CounterResource> counter) const {
    query.ForEach([&counter](Position& pos) {
      pos.x += 1.0F;
      ++counter->value;
    });
  }
};

struct QueryAndMsgWriterSystem {
  void operator()(Query<const Position&> query,
                  MessageWriter<TestMessage> writer) const {
    writer.Write(TestMessage{7});
    query.ForEach([](const Position& /*pos*/) {});
  }
};

struct QueryAndAsyncResSystem {
  void operator()(Query<Position&> query, AsyncRes<ThreadSafeData> data) const {
    ++data->value;
    query.ForEach([](Position& pos) { pos.x += 1.0F; });
  }
};

struct FlushCreateEntityCmd {
  static void Execute(World& world) {
    [[maybe_unused]] const auto entity = world.CreateEntity();
  }
};

struct FlushCommandSystem {
  void operator()(Commands commands) const {
    commands.Enqueue(FlushCreateEntityCmd{});
  }
};

struct FlushMessageSystem {
  void operator()(MessageWriter<TestMessage> writer) const {
    writer.Write(TestMessage{99});
  }
};

struct FlushReadMessageSystem {
  void operator()(MessageReader<TestMessage> reader,
                  Res<CounterResource> counter) const {
    for (const auto msg : reader) {
      counter->value += msg->value;
    }
  }
};

}  // namespace helios::ecs::schedule_test
