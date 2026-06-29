#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/log/logger.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <thread>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct FrameCount {
  size_t count = 0;
};

struct ChatMessage {
  std::string text;
};

struct AsyncPing {
  static constexpr bool kAsync = true;
  int id = 0;
};

struct BumpFrame {
  void operator()(hecs::Res<FrameCount> frames) const { ++frames->count; }
};

struct WriteRegularMessage {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<ChatMessage> writer) const {
    if (frames->count == 1) {
      writer.Write(ChatMessage{.text = "hello from frame 1"});
      hlog::Info("messages: wrote regular ChatMessage");
    }
  }
};

struct ReadRegularMessage {
  void operator()(hecs::MessageReader<ChatMessage> reader) const {
    reader.ForEach([](const auto msg) {
      hlog::Info("messages: read regular '{}'", msg->text);
    });
  }
};

struct ReadAsyncMessage {
  void operator()(hecs::AsyncMessageReader<AsyncPing> reader) const {
    reader.ForEach([](const AsyncPing& ping) {
      hlog::Info("messages: read async ping id={}", ping.id);
    });
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 4) {
      exit_writer.Write({.code = happ::ExitCode::kSuccess});
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddMessages<ChatMessage, AsyncPing>();
  app.InsertResources(FrameCount{});
  app.AddSystem(happ::kFirst, BumpFrame{});
  app.AddSystem(happ::kPreUpdate, WriteRegularMessage{});
  app.AddSystems(happ::kUpdate, ReadRegularMessage{}, ReadAsyncMessage{});
  app.AddSystem(happ::kLast, ExitAfterFrames{});

  std::thread async_writer([&app]() {
    auto writer = app.GetWorld().WriteAsyncMessages<AsyncPing>();
    for (int i = 0; i < 3; ++i) {
      writer.Write(AsyncPing{.id = i});
      hlog::Info("messages: wrote async ping id={}", i);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  const auto code = app.Run();
  async_writer.join();
  return std::to_underlying(code);
}
