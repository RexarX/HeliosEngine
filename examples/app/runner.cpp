#include <helios/app/app.hpp>
#include <helios/log/log.hpp>

#include <chrono>
#include <utility>

namespace happ = helios::app;
namespace hlog = helios::log;

namespace {

// App runners are functions or lamdas that execute app's main loop and
// return `helios::app::ExitCode`. They can be used to wrap the app's `Run()`
// call with additional logic, such as timing or error handling.

// Runner that runs the app for a fixed duration of 100 milliseconds.
happ::ExitCode TimedRunner(happ::App& app) {
  using namespace std::chrono_literals;

  hlog::Info("TimedRunner: starting");

  const auto start_time = std::chrono::steady_clock::now();
  const auto exit_time = start_time + 10ms;

  std::optional<happ::ExitCode> exit_code;
  while (exit_code = app.ShouldExit(),
         !exit_code && std::chrono::steady_clock::now() < exit_time) {
    const auto& frame_count = app.GetWorld().ReadResource<happ::FrameCount>();
    hlog::Info("TimedRunner: {} frame", frame_count.count);
    app.Update();
  }

  const auto millisec = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start_time)
                            .count();
  hlog::Info("TimedRunner: stopping after {} ms", millisec);

  return exit_code.value_or(happ::ExitCode::kSuccess);
}

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.SetRunner(TimedRunner);

  const auto code = app.Run();
  return std::to_underlying(code);
}
