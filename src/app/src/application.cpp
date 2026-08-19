#include <pch.hpp>

#include <helios/app/application.hpp>

#include <helios/app/details/profile.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/plugin_group.hpp>
#include <helios/app/runners.hpp>
#include <helios/assert.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/scheduler.hpp>
#include <helios/log/logger.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>

#include <string>

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#include <format>
#endif

namespace helios::app {

namespace {

constexpr auto kPluginReadyPollMinSleep = std::chrono::microseconds(50);
constexpr auto kPluginReadyPollMaxSleep = std::chrono::microseconds(8000);

}  // namespace

App::App()
    : main_sub_app_(std::string(SubAppNameOf(kMainSubApp))),
      runner_(RunDefault) {
  RegisterBuiltinSchedules(main_sub_app_.GetScheduler());
  RegisterBuiltinFrameOrders(main_sub_app_.GetWorld());
}

App::App(size_t worker_thread_count)
    : executor_(worker_thread_count),
      main_sub_app_(std::string(SubAppNameOf(kMainSubApp))),
      runner_(RunDefault) {
  RegisterBuiltinSchedules(main_sub_app_.GetScheduler());
  RegisterBuiltinFrameOrders(main_sub_app_.GetWorld());
}

App::~App() {
  // Ensure we're not running (should already be false if properly shut down)
  is_running_.store(false, std::memory_order_release);

  for (auto&& [_, sub_app] : sub_apps_) {
    if (sub_app.IsAsync()) {
      sub_app.RequestAsyncLoopStop();
    }
    sub_app.WaitUntilFullyIdle();
  }

  scheduler_.StopAsyncLoops(*this);
  scheduler_.WaitForSubApps();
  executor_.WaitForAll();
}

void App::Clear() {
  HELIOS_ASSERT(!IsRunning(), "Cannot clear app while it is running!");

  is_running_.store(false, std::memory_order_relaxed);

  // Stop background work before tearing down sub-apps / scheduler state.
  if (is_initialized_) {
    scheduler_.Shutdown(*this);
  }

  scheduler_.Stop(executor_);
  scheduler_.Clear();

  is_initialized_ = false;
  plugins_.clear();
  dynamic_plugins_.clear();
  main_sub_app_.Clear();
  sub_apps_.clear();

  RegisterBuiltinSchedules(main_sub_app_.GetScheduler());
  RegisterBuiltinFrameOrders(main_sub_app_.GetWorld());
}

void App::Initialize() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::Initialize");

  HELIOS_ASSERT(!IsInitialized(), "App is already initialized!");
  HELIOS_ASSERT(!IsRunning(), "Cannot initialize while app is running!");

  log::Info("Initializing application...");

  is_initialized_ = true;

  BuildPlugins();
  WaitForPluginsReady();
  FinishPlugins();

  RegisterMessages();

  scheduler_.Build(*this);
  scheduler_.RunStartup(*this);
}

void App::Update() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::Update");

  HELIOS_ASSERT(is_initialized_, "App is not initialized!");
  scheduler_.RunFrame(*this);

  HELIOS_APP_PROFILE_FRAME();
}

ExitCode App::Run() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::Run");

  HELIOS_ASSERT(!IsRunning(), "App is already running!");

  Initialize();

  log::Info("Running application...");

  is_running_.store(true, std::memory_order_release);
  const ExitCode exit_code = runner_(*this);
  is_running_.store(false, std::memory_order_release);

  CleanUp();

  return exit_code;
}

void App::RunFrameOrder(const FrameOrder& order) {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::RunFrameOrder");

  HELIOS_ASSERT(is_initialized_, "App is not initialized!");
  scheduler_.RunFrameOrder(*this, order);
}

void App::NotifyPluginReadinessChanged() noexcept {
  {
    const std::scoped_lock lock(plugins_ready_mutex_);
  }
  plugins_ready_cv_.notify_all();
}

auto App::AddSchedule(ecs::ScheduleTypeId id, ecs::Schedule&& schedule)
    -> ecs::ScheduleOrdering {
  HELIOS_ASSERT(!IsInitialized(),
                "Cannot add schedule after app initialization!");
  HELIOS_ASSERT(!IsRunning(), "Cannot add schedule while app is running!");

  return main_sub_app_.AddSchedule(id, std::move(schedule));
}

auto App::ShouldExit() const noexcept -> std::optional<ExitCode> {
  const auto& world = GetWorld();
  if (!world.HasMessage<AppExit>()) [[unlikely]] {
    return std::nullopt;
  }

  // Fresh IncludeBacklog cursor: poll currently retained AppExit messages each
  // check without implying once-only delivery across ShouldExit calls.
  auto cursor = ecs::MessageCursor<AppExit>::IncludeBacklog();
  const auto reader = world.ReadMessages<AppExit>(cursor);
  if (reader.Empty()) {
    return std::nullopt;
  }

  const bool any_failure = std::ranges::any_of(
      reader, [](auto wrapper) { return wrapper->code == ExitCode::kFailure; });

  return any_failure ? ExitCode::kFailure : ExitCode::kSuccess;
}

void App::CleanUp() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::Clear");

  if (!IsInitialized()) {
    return;
  }

  log::Info("Cleaning up application...");

  scheduler_.WaitForSubApps();
  scheduler_.Shutdown(*this);
  DestroyPlugins();

  is_initialized_ = false;
  is_running_.store(false, std::memory_order_relaxed);
}

void App::RegisterMessages() {
  main_sub_app_.AddMessages<AppExit>();
  for (auto&& [_, sub_app] : sub_apps_) {
    sub_app.AddMessages<AppExit>();
  }
}

void App::BuildPlugins() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::BuildPlugins");

  log::Info("Building plugins...");

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  std::string zone_name;
  zone_name.reserve(128);
#endif

  for (auto&& [index, storage] : plugins_) {
    HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
    zone_name.clear();
    std::format_to(std::back_inserter(zone_name),
                   "helios::app::Plugin::Build{{name: {}}}", storage.name);
#endif
    HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
    log::Debug("Building '{}' plugin", storage.name);

    storage.plugin->Build(*this);
  }

  for (auto&& [index, dynamic_plugin] : dynamic_plugins_) {
    if (dynamic_plugin.Loaded()) [[likely]] {
      HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
      zone_name.clear();
      std::format_to(std::back_inserter(zone_name),
                     "helios::app::Plugin::Build{{name: {}}}",
                     dynamic_plugin.GetPluginName());
#endif
      HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
      HELIOS_APP_PROFILE_ZONE_TEXT("dynamic");
      log::Debug("Building '{}' plugin", dynamic_plugin.GetPluginName());

      dynamic_plugin.GetPlugin().Build(*this);
    }
  }
}

void App::PollPlugins() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::PollPlugins");

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  std::string zone_name;
  zone_name.reserve(128);
#endif

  for (auto&& [index, storage] : plugins_) {
    HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
    zone_name.clear();
    std::format_to(std::back_inserter(zone_name),
                   "helios::app::Plugin::Poll{{name: {}}}", storage.name);
#endif
    HELIOS_APP_PROFILE_ZONE_NAME(zone_name);

    storage.plugin->Poll(*this);
  }

  for (auto&& [index, dynamic_plugin] : dynamic_plugins_) {
    if (dynamic_plugin.Loaded()) [[likely]] {
      HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
      zone_name.clear();
      std::format_to(std::back_inserter(zone_name),
                     "helios::app::Plugin::Poll{{name: {}}}",
                     dynamic_plugin.GetPluginName());
#endif
      HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
      HELIOS_APP_PROFILE_ZONE_TEXT("dynamic");

      dynamic_plugin.GetPlugin().Poll(*this);
    }
  }
}

void App::WaitForPluginsReady() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::WaitForPluginsReady");

  log::Info("Waiting for plugins to be ready...");

  if (PluginsReady()) {
    return;
  }

  auto sleep_duration = kPluginReadyPollMinSleep;

  while (!PluginsReady()) {
    PollPlugins();

    {
      std::unique_lock lock(plugins_ready_mutex_);
      plugins_ready_cv_.wait_for(lock, sleep_duration,
                                 [this] { return PluginsReady(); });
    }

    sleep_duration = std::min(sleep_duration * 2, kPluginReadyPollMaxSleep);
  }
}

bool App::PluginsReady() const {
  for (const auto& [index, storage] : plugins_) {
    if (!storage.plugin->IsReady(*this)) {
      return false;
    }
  }

  return std::ranges::all_of(dynamic_plugins_, [this](const auto& pair) {
    return !pair.second.Loaded() || pair.second.GetPlugin().IsReady(*this);
  });
}

void App::FinishPlugins() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::FinishPlugins");

  log::Info("Finishing plugins...");

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  std::string zone_name;
  zone_name.reserve(128);
#endif

  for (auto&& [index, storage] : plugins_) {
    HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
    zone_name.clear();
    std::format_to(std::back_inserter(zone_name),
                   "helios::app::Plugin::Finish{{name: {}}}", storage.name);
#endif
    HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
    log::Debug("Finishing '{}' plugin", storage.name);

    storage.plugin->Finish(*this);
  }

  for (auto&& [index, dynamic_plugin] : dynamic_plugins_) {
    if (dynamic_plugin.Loaded()) [[likely]] {
      HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
      zone_name.clear();
      std::format_to(std::back_inserter(zone_name),
                     "helios::app::Plugin::Finish{{name: {}}}",
                     dynamic_plugin.GetPluginName());
#endif
      HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
      HELIOS_APP_PROFILE_ZONE_TEXT("dynamic");
      log::Debug("Finishing '{}' plugin", dynamic_plugin.GetPluginName());

      dynamic_plugin.GetPlugin().Finish(*this);
    }
  }
}

void App::DestroyPlugins() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::DestroyPlugins");

  log::Info("Destroying plugins...");

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  std::string zone_name;
  zone_name.reserve(128);
#endif

  for (auto&& [index, storage] : plugins_) {
    HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
    zone_name.clear();
    std::format_to(std::back_inserter(zone_name),
                   "helios::app::Plugin::Destroy{{name: {}}}", storage.name);
#endif
    HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
    log::Debug("Destroying '{}' plugin", storage.name);

    storage.plugin->Destroy(*this);
  }

  for (auto&& [index, dynamic_plugin] : dynamic_plugins_) {
    if (dynamic_plugin.Loaded()) [[likely]] {
      HELIOS_APP_PROFILE_SCOPE();
#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
      zone_name.clear();
      std::format_to(std::back_inserter(zone_name),
                     "helios::app::Plugin::Destroy{{name: {}}}",
                     dynamic_plugin.GetPluginName());
#endif
      HELIOS_APP_PROFILE_ZONE_NAME(zone_name);
      HELIOS_APP_PROFILE_ZONE_TEXT("dynamic");
      log::Debug("Destroying '{}' plugin", dynamic_plugin.GetPluginName());

      dynamic_plugin.GetPlugin().Destroy(*this);
    }
  }
}

}  // namespace helios::app
