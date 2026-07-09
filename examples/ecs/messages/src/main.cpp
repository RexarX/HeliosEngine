#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// Regular messages are frame-lifetime events stored in the World's message
// buffers. They are visible to readers after the message lifecycle advances.
struct ChatMessage {
  std::string text;
};

struct AsyncPing {
  // Async messages use a lock-free queue and can be written from another
  // thread. They are not part of the regular double-buffered lifecycle.
  static constexpr bool kAsync = true;
  int id = 0;
};

struct WriteRegularMessage {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<ChatMessage> writer) const {
    if (frames->count == 0) {
      // This write enters the current message buffer for ChatMessage.
      writer.Write(ChatMessage{.text = "hello from frame 0"});
      hlog::Info("messages: wrote regular ChatMessage");
    }
  }
};

struct ReadRegularMessage {
  void operator()(hecs::MessageReader<ChatMessage> reader) const {
    // MessageReader iterates the readable buffer and exposes lazy adapters like
    // queries do. Here we only print every message.
    reader.ForEach([](const auto msg) {
      hlog::Info("messages: read regular '{}'", msg->text);
    });
  }
};

struct ReadAsyncMessage {
  void operator()(hecs::AsyncMessageReader<AsyncPing> reader) const {
    // Async reads drain whatever the background writer has produced so far.
    reader.ForEach([](const AsyncPing& ping) {
      hlog::Info("messages: read async ping id={}", ping.id);
    });
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 4) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // Message types must be registered before systems request their readers or
  // writers.
  app.AddMessages<ChatMessage, AsyncPing>();
  app.AddSystem(happ::kPreUpdate, WriteRegularMessage{});
  app.AddSystems(happ::kUpdate, ReadRegularMessage{}, ReadAsyncMessage{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  // This producer writes directly to the World's async queue while the app is
  // running, demonstrating cross-thread event input.
  std::thread async_writer([&app]() {
    auto writer = app.GetWorld().WriteAsyncMessages<AsyncPing>();
    for (int i = 0; i < 3; ++i) {
      writer.Write(AsyncPing{.id = i});
      hlog::Info("messages: wrote async ping id={}", i);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  const auto code = app.Run();

  // Join before returning so the background writer cannot outlive the app.
  async_writer.join();
  return std::to_underlying(code);
}
