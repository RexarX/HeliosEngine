#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/runners.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string_view>
#include <thread>

using namespace helios;
using namespace helios::app;
using namespace helios::ecs;

namespace {

struct CounterResource {
  int value = 0;
};

struct IncrementSystem {
  void operator()(Res<CounterResource> counter) const { ++counter->value; }
};

struct ExitSystem {
  ExitCode code = ExitCode::kSuccess;

  void operator()(MessageWriter<AppExit> exits) const {
    exits.Write(AppExit{.code = code});
  }
};

struct OrderSystem {
  int* order_slot = nullptr;
  int* call_index = nullptr;

  void operator()(Res<CounterResource> /*counter*/) const {
    *order_slot = (*call_index)++;
  }
};

struct ExtractCounterResource {
  int value = 0;
};

struct RenderSubAppLabel {};

struct RegularSubAppLabel {};
constexpr RegularSubAppLabel kRegularSubApp{};

struct OverlappingRenderSubAppLabel {
  static constexpr bool kAllowOverlappingUpdates = true;
};

struct LimitedSkipRenderSubAppLabel {
  static constexpr bool kAllowOverlappingUpdates = true;
  static constexpr size_t kMaxOverlappingUpdates = 1;
};

struct AsyncSoundSubAppLabel {
  static constexpr bool kAsync = true;
};

struct BlockingSoundSubAppLabel {};

struct SlowUpdateSystem {
  void operator()() const {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
};

struct ShutdownCounterResource {
  int value = 0;
};

struct ShutdownSystem {
  void operator()(Res<ShutdownCounterResource> counter) const {
    ++counter->value;
  }
};

struct StartupCounterResource {
  int value = 0;
};

struct StartupSystem {
  void operator()(Res<StartupCounterResource> counter) const {
    ++counter->value;
  }
};

struct UpdateCountResource {
  int value = 0;
};

struct CountingUpdateSystem {
  void operator()(Res<UpdateCountResource> counter) const { ++counter->value; }
};

struct NamedTestPlugin final : public Plugin {
  static constexpr std::string_view kName = "NamedTestPlugin";

  void Build(App& app) override { app.InsertResources(CounterResource{5}); }

  [[nodiscard]] bool IsReady(const App& /*app*/) const noexcept override {
    return ready_;
  }

  bool ready_ = true;
};

struct LifecycleTestPlugin final : public Plugin {
  inline static int build_count = 0;
  inline static int finish_count = 0;
  inline static int destroy_count = 0;

  static void Reset() noexcept {
    build_count = 0;
    finish_count = 0;
    destroy_count = 0;
  }

  void Build(App& /*app*/) override { ++build_count; }
  void Finish(App& /*app*/) override { ++finish_count; }
  void Destroy(App& /*app*/) override { ++destroy_count; }
};

struct AsyncReadyPlugin final : public Plugin {
  void Build(App& app) override {
    app.GetExecutor().SilentAsync([ready = ready_, &app]() {
      ready->store(true, std::memory_order_release);
      app.NotifyPluginReadinessChanged();
    });
  }

  [[nodiscard]] bool IsReady(const App& /*app*/) const noexcept override {
    return ready_->load(std::memory_order_acquire);
  }

private:
  std::shared_ptr<std::atomic<bool>> ready_ =
      std::make_shared<std::atomic<bool>>(false);
};

struct SecondTestPlugin final : public Plugin {
  void Build(App& /*app*/) override {}
};

struct CustomSchedule {
  static constexpr std::string_view kName = "CustomSchedule";
};

struct SetOne {};
struct SetTwo {};

}  // namespace

namespace helios::app::test_plugins {
struct TestDynamicPlugin;
}

TEST_SUITE("helios::app::App") {
  TEST_CASE("app::App::ctor") {
    SUBCASE("Default-constructed app is not initialized or running") {
      App app;
      CHECK_FALSE(app.IsInitialized());
      CHECK_FALSE(app.IsRunning());
    }

    SUBCASE("Default-constructed app registers built-in update schedule") {
      App app;
      CHECK_NE(app.TryGetSchedule(kUpdate), nullptr);
    }

    SUBCASE("Worker thread count ctor leaves app uninitialized") {
      App app(4);
      CHECK_FALSE(app.IsInitialized());
      CHECK_FALSE(app.IsRunning());
    }
  }

  TEST_CASE("app::App::Clear") {
    SUBCASE("Clear removes resources, systems state, and initialization flag") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      app.Clear();

      CHECK_FALSE(app.GetWorld().HasResource<CounterResource>());
      CHECK_FALSE(app.IsInitialized());
    }

    SUBCASE("Clear re-registers built-in schedules") {
      App app;
      app.Clear();
      CHECK_NE(app.TryGetSchedule(kUpdate), nullptr);
    }
  }

  TEST_CASE("app::App::Initialize") {
    SUBCASE("Initialize runs startup systems on main sub-app") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystem(kStartup, IncrementSystem{});
      app.Initialize();

      CHECK(app.IsInitialized());
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Initialize runs startup on registered sub-apps") {
      App app;

      SubApp render;
      render.InsertResources(StartupCounterResource{});
      render.AddSystem(kStartup, StartupSystem{});

      app.InsertSubApp(RenderSubAppLabel{}, std::move(render));
      app.Initialize();

      CHECK_EQ(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<StartupCounterResource>()
                   .value,
               1);
    }

    SUBCASE("Initialize runs plugin Build and Finish hooks") {
      LifecycleTestPlugin::Reset();
      App app;
      app.AddPlugins(LifecycleTestPlugin{});
      app.Initialize();

      CHECK_EQ(LifecycleTestPlugin::build_count, 1);
      CHECK_EQ(LifecycleTestPlugin::finish_count, 1);
    }

    SUBCASE("Initialize waits for async plugin readiness") {
      App app(2);
      app.AddPlugins(AsyncReadyPlugin{});
      app.Initialize();
      CHECK(app.IsInitialized());
    }

    SUBCASE("Initialize starts background updates for async sub-apps") {
      App app(4);

      SubApp sound;
      sound.SetRunner(RunDefaultSubApp);
      sound.InsertResources(UpdateCountResource{});
      sound.AddSystem(kUpdate, CountingUpdateSystem{});

      app.InsertSubApp(AsyncSoundSubAppLabel{}, std::move(sound));
      app.Initialize();

      std::this_thread::sleep_for(std::chrono::milliseconds{100});

      CHECK_GE(app.GetSubApp<AsyncSoundSubAppLabel>()
                   .GetWorld()
                   .ReadResource<UpdateCountResource>()
                   .value,
               1);

      app.GetScheduler().Shutdown(app);
      CHECK_FALSE(app.GetSubApp<AsyncSoundSubAppLabel>().IsUpdating());
    }
  }

  TEST_CASE("app::App::Update") {
    SUBCASE("Update runs update-stage systems on initialized app") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      app.Update();

      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Multiple updates accumulate system effects") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      app.Update();
      app.Update();

      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 2);
    }

    SUBCASE("Update extracts main-world data into blocking sub-app") {
      App app(2);
      app.InsertResources(CounterResource{42});

      SubApp sound;
      sound.SetExtractFunction([](const World& main, World& sub) {
        sub.InsertResources(
            ExtractCounterResource{main.ReadResource<CounterResource>().value});
      });

      app.InsertSubApp(BlockingSoundSubAppLabel{}, std::move(sound));
      app.Initialize();
      app.Update();

      CHECK_FALSE(
          app.GetSubApp<BlockingSoundSubAppLabel>().AllowsOverlappingUpdates());
      CHECK_EQ(app.GetSubApp<BlockingSoundSubAppLabel>()
                   .GetWorld()
                   .ReadResource<ExtractCounterResource>()
                   .value,
               42);
    }

    SUBCASE("Update copies extracted resources from main into sub-app world") {
      App app;
      app.InsertResources(CounterResource{10});

      SubApp render;
      render.TryInsertResources(ExtractCounterResource{0});
      render.SetExtractFunction([](const World& main, World& sub) {
        sub.WriteResource<ExtractCounterResource>().value =
            main.ReadResource<CounterResource>().value;
      });

      app.InsertSubApp(RenderSubAppLabel{}, std::move(render));
      app.Initialize();
      app.Update();

      CHECK_EQ(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<ExtractCounterResource>()
                   .value,
               10);
    }

    SUBCASE("Update runs systems on sub-app with custom runner") {
      App app(2);

      std::atomic<int> runner_calls{0};

      SubApp render;
      render.SetRunner(
          [&runner_calls](SubApp& sub_app, async::Executor& executor) {
            ++runner_calls;
            RunOnceSubApp(sub_app, executor);
          });
      render.InsertResources(UpdateCountResource{});
      render.AddSystem(kUpdate, CountingUpdateSystem{});

      app.InsertSubApp(RenderSubAppLabel{}, std::move(render));
      app.Initialize();
      app.Update();

      CHECK_GE(runner_calls.load(), 1);
      CHECK_GE(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<UpdateCountResource>()
                   .value,
               1);
    }

    SUBCASE(
        "Update with overlapping sub-app may skip extraction while updating") {
      App app;
      app.InsertResources(CounterResource{10});

      SubApp render;
      render.TryInsertResources(ExtractCounterResource{0});
      render.SetExtractFunction([](const World& main, World& sub) {
        sub.WriteResource<ExtractCounterResource>().value =
            main.ReadResource<CounterResource>().value;
      });

      app.InsertSubApp(OverlappingRenderSubAppLabel{}, std::move(render));
      app.Initialize();

      SubApp& overlapping = app.GetSubApp<OverlappingRenderSubAppLabel>();
      app.Update();
      app.Update();

      CHECK_EQ(
          overlapping.GetWorld().ReadResource<ExtractCounterResource>().value,
          10);
    }

    SUBCASE("Update respects limited extraction-skip budget on sub-app") {
      App app(4);
      app.InsertResources(CounterResource{1});

      SubApp render;
      render.TryInsertResources(ExtractCounterResource{0});
      render.AddSystem(kUpdate, SlowUpdateSystem{});
      render.SetExtractFunction([](const World& /*main*/, World& sub) {
        sub.WriteResource<ExtractCounterResource>().value = 99;
      });

      app.InsertSubApp(LimitedSkipRenderSubAppLabel{}, std::move(render));
      app.Initialize();

      app.Update();
      app.GetSubApp<LimitedSkipRenderSubAppLabel>().WaitUntilFullyIdle();

      CHECK_EQ(app.GetSubApp<LimitedSkipRenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<ExtractCounterResource>()
                   .value,
               99);
    }
  }

  TEST_CASE("app::App::Run") {
    SUBCASE(
        "Run initializes, invokes runner, cleans up, and returns exit code") {
      App app;
      app.InsertResources(ShutdownCounterResource{});
      app.AddSystem(kShutdown, ShutdownSystem{});
      app.SetRunner([](App& /*application*/) { return ExitCode::kSuccess; });

      const ExitCode code = app.Run();

      CHECK_EQ(code, ExitCode::kSuccess);
      CHECK_FALSE(app.IsInitialized());
      CHECK_FALSE(app.IsRunning());
      CHECK_EQ(app.GetWorld().ReadResource<ShutdownCounterResource>().value, 1);
    }

    SUBCASE("Run sets IsRunning for the duration of the runner") {
      App app;
      bool saw_running = false;
      app.SetRunner([&saw_running](App& application) {
        saw_running = application.IsRunning();
        return ExitCode::kSuccess;
      });

      app.Run();
      CHECK(saw_running);
      CHECK_FALSE(app.IsRunning());
    }

    SUBCASE("Run executes plugin Destroy during cleanup") {
      LifecycleTestPlugin::Reset();
      App app;
      app.AddPlugins(LifecycleTestPlugin{});
      app.SetRunner([](App& /*application*/) { return ExitCode::kSuccess; });
      app.Run();

      CHECK_EQ(LifecycleTestPlugin::destroy_count, 1);
    }
  }

  TEST_CASE("app::App::NotifyPluginReadinessChanged") {
    SUBCASE("Wakes WaitForPluginsReady so async plugin can finish Initialize") {
      App app(2);
      app.AddPlugins(AsyncReadyPlugin{});
      app.Initialize();
      CHECK(app.IsInitialized());
    }
  }

  TEST_CASE("app::App::AddPlugins") {
    SUBCASE("Single plugin registers and runs Build on Initialize") {
      App app;
      app.AddPlugins(NamedTestPlugin{});
      CHECK(app.HasPlugins<NamedTestPlugin>());
      app.Initialize();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 5);
    }

    SUBCASE("Multiple plugins register independently") {
      App app;
      app.AddPlugins(NamedTestPlugin{}, SecondTestPlugin{});
      const auto has = app.HasPlugins<NamedTestPlugin, SecondTestPlugin>();
      CHECK(has[0]);
      CHECK(has[1]);
    }
  }

  TEST_CASE("app::App::AddDynamicPlugin") {
    SUBCASE("Unloaded plugin is not registered") {
      App app;
      DynamicPlugin dyn;
      app.AddDynamicPlugin(std::move(dyn));
      CHECK_FALSE(app.HasPlugins<NamedTestPlugin>());
    }

#ifdef HELIOS_TEST_PLUGIN_PATH
    SUBCASE("Second AddDynamicPlugin does not replace existing entry") {
      DynamicPlugin first;
      REQUIRE(first.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                  .has_value());
      const PluginTypeIndex index = PluginTypeIndex::From(first);

      DynamicPlugin second;
      REQUIRE(second.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                  .has_value());

      App app;
      app.AddDynamicPlugin(std::move(first));
      app.AddDynamicPlugin(std::move(second));
      CHECK(app.HasPlugin(index));
    }
#endif
  }

  TEST_CASE("app::App::InsertSubApp") {
    SUBCASE("InsertSubApp registers sub-app retrievable by label") {
      App app;
      SubApp sub;
      sub.InsertResources(CounterResource{3});
      app.InsertSubApp(RenderSubAppLabel{}, std::move(sub));
      CHECK(app.HasSubApp<RenderSubAppLabel>());
      CHECK_EQ(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               3);
    }

    SUBCASE("InsertSubApp applies overlapping label traits") {
      App app;
      app.InsertSubApp(OverlappingRenderSubAppLabel{}, SubApp{});
      SubApp& sub = app.GetSubApp<OverlappingRenderSubAppLabel>();
      CHECK(sub.AllowsOverlappingUpdates());
      CHECK_EQ(sub.MaxExtractionSkips(), 0);
    }

    SUBCASE("InsertSubApp applies limited skip budget from label trait") {
      App app;
      app.InsertSubApp(LimitedSkipRenderSubAppLabel{}, SubApp{});
      CHECK_EQ(
          app.GetSubApp<LimitedSkipRenderSubAppLabel>().MaxExtractionSkips(),
          1);
    }

    SUBCASE("InsertSubApp applies async label trait") {
      App app;
      app.InsertSubApp(AsyncSoundSubAppLabel{}, SubApp{});
      CHECK(app.GetSubApp<AsyncSoundSubAppLabel>().IsAsync());
    }

    SUBCASE("First insertion wins; duplicate label is ignored") {
      App app;
      SubApp first;
      first.InsertResources(CounterResource{1});
      SubApp second;
      second.InsertResources(CounterResource{99});
      app.InsertSubApp(RenderSubAppLabel{}, std::move(first));
      app.InsertSubApp(RenderSubAppLabel{}, std::move(second));
      CHECK_EQ(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               1);
    }
  }

  TEST_CASE("app::App::RemoveSubApp") {
    SUBCASE("RemoveSubApp erases registered sub-app") {
      App app;
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      CHECK(app.RemoveSubApp<RenderSubAppLabel>());
      CHECK_FALSE(app.HasSubApp<RenderSubAppLabel>());
    }

    SUBCASE("RemoveSubApp returns false when label is absent") {
      App app;
      CHECK_FALSE(app.RemoveSubApp<RenderSubAppLabel>());
    }
  }

  TEST_CASE("app::App::AddSchedule") {
    SUBCASE("AddSchedule registers custom schedule on main sub-app") {
      App app;
      ecs::Schedule custom;
      [[maybe_unused]] const auto ordering =
          app.AddSchedule(CustomSchedule{}, std::move(custom));
      CHECK_NE(app.TryGetSchedule<CustomSchedule>(), nullptr);
    }

    SUBCASE("AddSchedule placement runs between kFirst and kLast on Update") {
      App app;
      app.InsertResources(CounterResource{});

      int call_index = 0;
      std::array<int, 2> order{};

      app.AddSystem(kFirst, OrderSystem{.order_slot = order.data(),
                                        .call_index = &call_index});
      app.AddSchedule(CustomSchedule{}, ecs::Schedule{})
          .InStage(kUpdateStage)
          .After(kFirst)
          .Before(kLast);
      app.AddSystem(CustomSchedule{}, OrderSystem{.order_slot = &order[1],
                                                  .call_index = &call_index});

      app.Initialize();
      app.Update();

      CHECK_LT(order[0], order[1]);
    }
  }

  TEST_CASE("app::App::InitSchedule") {
    SUBCASE("InitSchedule creates empty schedule when missing") {
      App app;
      app.InitSchedule(CustomSchedule{});
      CHECK_NE(app.TryGetSchedule(CustomSchedule{}), nullptr);
    }
  }

  TEST_CASE("app::App::EditSchedule") {
    SUBCASE("EditSchedule adds systems and marks scheduler dirty") {
      App app;
      app.InitSchedule(CustomSchedule{});
      app.EditSchedule(CustomSchedule{}, [](Schedule& schedule) {
        schedule.Add(IncrementSystem{});
      });
      CHECK(app.GetMainSubApp().GetScheduler().IsDirty());
    }
  }

  TEST_CASE("app::App::AddSystem") {
    SUBCASE("AddSystem runs on Update after Initialize") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("app::App::AddSystems") {
    SUBCASE("AddSystems registers both systems in the same schedule") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystems(kUpdate, IncrementSystem{}, IncrementSystem{});
      app.Initialize();
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 2);
    }

    SUBCASE("AddSystems with InSet runs both systems in the named set") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystems(kUpdate, IncrementSystem{}, IncrementSystem{})
          .InSet(SetOne{});
      app.Initialize();
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 2);
    }

    SUBCASE("Configured set ordering affects bulk-added systems") {
      App app;
      app.InsertResources(CounterResource{});

      auto movement =
          app.AddSystems(kUpdate, IncrementSystem{}, IncrementSystem{})
              .InSet(SetOne{});
      app.ConfigureSet(kUpdate, SetTwo{}).After(movement);

      app.Initialize();
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 2);
    }
  }

  TEST_CASE("app::App::ConfigureSet") {
    SUBCASE("ConfigureSet marks main sub-app scheduler dirty") {
      App app;
      [[maybe_unused]] const auto set = app.ConfigureSet(kUpdate, SetOne{});
      CHECK(app.GetMainSubApp().GetScheduler().IsDirty());
    }
  }

  TEST_CASE("app::App::InsertResources") {
    SUBCASE("InsertResources replaces existing resource values") {
      App app;
      app.InsertResources(CounterResource{10});
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 10);
    }
  }

  TEST_CASE("app::App::TryInsertResources") {
    SUBCASE("TryInsertResources keeps first value when resource exists") {
      App app;
      app.InsertResources(CounterResource{1});
      app.TryInsertResources(CounterResource{99});
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("TryInsertResources inserts when resource is absent") {
      App app;
      app.TryInsertResources(CounterResource{7});
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 7);
    }
  }

  TEST_CASE("app::App::AddMessages") {
    SUBCASE("AddMessages registers message type on main world") {
      App app;
      app.AddMessages<AppExit>();
      CHECK(app.GetWorld().HasMessage<AppExit>());
    }
  }

  TEST_CASE("app::App::SetRunner") {
    SUBCASE("SetRunner replaces default runner used by Run") {
      App app;
      app.SetRunner([](App& /*application*/) { return ExitCode::kSuccess; });
      const ExitCode code = app.Run();
      CHECK_EQ(code, ExitCode::kSuccess);
    }
  }

  TEST_CASE("app::App::ShouldExit") {
    SUBCASE("Returns nullopt without AppExit message") {
      App app;
      app.Initialize();
      CHECK_FALSE(app.ShouldExit().has_value());
    }

    SUBCASE("Returns success after AppExit success is written") {
      App app;
      app.AddSystem(kFirst, ExitSystem{});
      app.Initialize();
      RunOnce(app);
      CHECK_EQ(app.ShouldExit(), ExitCode::kSuccess);
    }

    SUBCASE("Returns failure when failure exit is present") {
      App app;
      app.AddSystem(kFirst, ExitSystem{ExitCode::kFailure});
      app.Initialize();
      RunOnce(app);
      CHECK_EQ(app.ShouldExit(), ExitCode::kFailure);
    }
  }

  TEST_CASE("app::App::IsInitialized") {
    SUBCASE("False before Initialize, true after") {
      App app;
      CHECK_FALSE(app.IsInitialized());
      app.Initialize();
      CHECK(app.IsInitialized());
    }
  }

  TEST_CASE("app::App::IsRunning") {
    SUBCASE("False before and after Run completes") {
      App app;
      CHECK_FALSE(app.IsRunning());
      app.SetRunner([](App& /*application*/) { return ExitCode::kSuccess; });
      app.Run();
      CHECK_FALSE(app.IsRunning());
    }
  }

  TEST_CASE("app::App::HasPlugin") {
    SUBCASE("HasPlugin returns true for registered static plugin") {
      App app;
      app.AddPlugins(NamedTestPlugin{});
      CHECK(app.HasPlugin(PluginTypeIndex::From<NamedTestPlugin>()));
    }

    SUBCASE("HasPlugin returns false for unregistered plugin") {
      App app;
      CHECK_FALSE(app.HasPlugin(PluginTypeIndex::From<NamedTestPlugin>()));
    }
  }

  TEST_CASE("app::App::HasPlugins") {
    SUBCASE("HasPlugins reports each plugin registration state") {
      App app;
      app.AddPlugins(NamedTestPlugin{}, SecondTestPlugin{});
      CHECK(app.HasPlugins<NamedTestPlugin>());
      const auto results = app.HasPlugins<NamedTestPlugin, SecondTestPlugin>();
      CHECK(results[0]);
      CHECK(results[1]);
    }
  }

  TEST_CASE("app::App::HasSubApp") {
    SUBCASE("HasSubApp reflects InsertSubApp state") {
      App app;
      CHECK_FALSE(app.HasSubApp<RenderSubAppLabel>());
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      CHECK(app.HasSubApp<RenderSubAppLabel>());
    }
  }

  TEST_CASE("app::App::GetMainSubApp") {
    SUBCASE("GetMainSubApp shares world with GetWorld") {
      App app;
      app.InsertResources(CounterResource{11});
      CHECK_EQ(&app.GetMainSubApp().GetWorld(), &app.GetWorld());
      CHECK_EQ(
          app.GetMainSubApp().GetWorld().ReadResource<CounterResource>().value,
          11);
    }
  }

  TEST_CASE("app::App::GetSubApp") {
    SUBCASE("GetSubApp returns inserted sub-app by label") {
      App app;
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      CHECK_EQ(&app.GetSubApp<RenderSubAppLabel>(),
               &app.GetSubApp(SubAppTypeIndex::From(RenderSubAppLabel{})));
      CHECK_EQ(&std::as_const(app).GetSubApp<RenderSubAppLabel>(),
               &app.GetSubApp(RenderSubAppLabel{}));
    }
  }

  TEST_CASE("app::App::TryGetSchedule") {
    SUBCASE("TryGetSchedule returns built-in update schedule") {
      App app;
      CHECK_NE(app.TryGetSchedule(kUpdate), nullptr);
      CHECK_NE(std::as_const(app).TryGetSchedule(kUpdate), nullptr);
    }

    SUBCASE("TryGetSchedule returns nullptr for unknown label") {
      App app;
      CHECK_EQ(app.TryGetSchedule(CustomSchedule{}), nullptr);
    }
  }

  TEST_CASE("app::App::GetExecutor") {
    SUBCASE("GetExecutor returns same executor instance") {
      App app(2);
      CHECK_EQ(&app.GetExecutor(), &app.GetExecutor());
      CHECK_EQ(&std::as_const(app).GetExecutor(), &app.GetExecutor());
    }
  }

  TEST_CASE("app::App::GetWorld") {
    SUBCASE("GetWorld aliases main sub-app world") {
      App app;
      app.InsertResources(CounterResource{4});
      CHECK_EQ(&app.GetWorld(), &app.GetMainSubApp().GetWorld());
      CHECK_EQ(
          std::as_const(app).GetWorld().ReadResource<CounterResource>().value,
          4);
    }
  }

  TEST_CASE("app::App::GetScheduler") {
    SUBCASE(
        "GetScheduler returns stable application frame scheduler reference") {
      App app;
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      app.Initialize();
      CHECK_EQ(&app.GetScheduler(), &app.GetScheduler());
      CHECK_EQ(&std::as_const(app).GetScheduler(), &app.GetScheduler());
    }

    SUBCASE("Shutdown on scheduler runs shutdown stage on sub-apps") {
      App app;

      SubApp render;
      render.InsertResources(ShutdownCounterResource{});
      render.AddSystem(kShutdown, ShutdownSystem{});

      app.InsertSubApp(RenderSubAppLabel{}, std::move(render));
      app.Initialize();
      app.GetScheduler().Shutdown(app);

      CHECK_EQ(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<ShutdownCounterResource>()
                   .value,
               1);
    }
  }
}
