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

struct ChatMessage {
  std::string text;
};

struct AsyncPing {
  static constexpr bool kAsync = true;
  int id = 0;
};

struct WriteRegularMessage {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<ChatMessage> writer) const {
    if (frames->count == 0) {
      writer.Write(ChatMessage{.text = "hello from frame 0"});
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
  app.AddMessages<ChatMessage, AsyncPing>();
  app.AddSystem(happ::kPreUpdate, WriteRegularMessage{});
  app.AddSystems(happ::kUpdate, ReadRegularMessage{}, ReadAsyncMessage{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

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
